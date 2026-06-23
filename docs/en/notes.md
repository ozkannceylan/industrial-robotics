# Working Notes

> Concepts, mechanics, and lessons-from-mistakes accumulated during learning.
> Organized by topic, not chronologically.

---

## 1. Colcon workspace structure

In ROS2 a project consists of **packages**, and packages live inside a **workspace**. A workspace = any
folder containing a `src/` directory. When `colcon build` runs, it creates three sister directories:

| Directory | What it holds |
|---|---|
| `src/` | The original code **you wrote** (package sources). |
| `build/` | Intermediate build artifacts (object files, generated code, CMake cache). |
| `install/` | The **usable** result after building: binaries, headers, share data, lifecycle metadata. The ROS2 CLI reads from here. |
| `log/` | colcon log records. |

**Critical rule:** `ros2 run`, `ros2 interface show`, `ros2 launch` always work from the `install/`
space — **not from `src/`**. If you change a source and don't rebuild, the new state is "invisible".

```bash
# From the workspace root (the dir that contains src/, not inside it!):
colcon build --packages-select <pkg>     # build only the named package
source install/setup.bash                # introduce the workspace to the shell
ros2 ...                                 # the package is now visible
```

You must call `source install/setup.bash` in every new shell session (or add it to `.bashrc`).

In `.gitignore`, `build/`, `install/`, `log/` are ignored — they are **reproducible** and shouldn't take
up space in the repo.

---

## 2. `irp_msgs` — interface package

### 2.1 The three communication primitives in ROS2

| | Pattern | Lifetime | Feedback | Cancel | Typical use |
|---|---|---|---|---|---|
| **Topic** (`.msg`) | publish/subscribe | continuous stream | – | – | sensor, state, tf |
| **Service** (`.srv`) | request/response | instant (ms) | none | none | quick query/command |
| **Action** (`.action`) | goal + feedback + result | long (s–min) | periodic | yes | trajectory, navigation, manipulation |

Decision matrix:
- "Continuously flowing data" → topic
- "One question, one answer, done immediately" → service
- "Long-running job, progress should be observable, cancellable" → action

### 2.2 The `rosidl` pipeline — IDL and code generation

**IDL = Interface Definition Language.** A **language-independent** schema standard that defines how
software written in different languages (C++, Python, Java, ...) communicate. Protobuf, Cap'n Proto, and
OpenAPI are in the same category.

**`rosidl` = ROS Interface Definition Language.** ROS2's own IDL tooling. Its job: take the `.msg` /
`.srv` / `.action` files you wrote and turn them into source code that multiple languages (C++, Python)
can understand.

The `.msg` / `.srv` / `.action` files you write are **not code, they are schema**. During the build,
`rosidl` reads them and produces:
- C++ headers (`#include "irp_msgs/msg/robot_mode.hpp"`)
- Python module (`from irp_msgs.msg import RobotMode`)
- Wire-format type-support library (serialize/deserialize over the network)
- Introspection metadata (what `ros2 interface show` reads)

A 3-line `.msg` becomes thousands of lines of generated code after the build.

### 2.3 `package.xml` — dependency types

| Tag | Meaning | Analogy (Node.js) |
|---|---|---|
| `<buildtool_depend>X</buildtool_depend>` | The build system itself (`ament_cmake`, `cmake`) | build tool install |
| `<build_depend>X</build_depend>` | X is needed to **compile** this package. Not at runtime. | `devDependencies` |
| `<exec_depend>X</exec_depend>` | X is needed to **use** this package (runtime). Not at compile time. | `dependencies` (runtime only) |
| `<depend>X</depend>` | Both. Shortcut. | `dependencies` (common case) |
| `<test_depend>X</test_depend>` | Needed only in tests. | `devDependencies` (test part) |

The correct split for `irp_msgs`:
- `rosidl_default_generators` → **`<build_depend>`** (the generator itself, unneeded after compiling)
- `rosidl_default_runtime` → **`<exec_depend>`** (the library the generated code links against)

### 2.4 `member_of_group rosidl_interface_packages`

```xml
<member_of_group>rosidl_interface_packages</member_of_group>
```

Must be **top-level** (a direct child of `<package>`), **NOT** inside `<export>`.

**Rule:** Standard, official tags at the top level; tool-specific settings inside `<export>`.

`<member_of_group>` is ROS's official metadata (REP-149), while `<export>` is where build/tool ecosystems
"leave messages for their own tooling". The `rosidl_cmake` checker does not look inside `<export>` — put
it in the wrong place and the build says:
```
Packages installing interfaces must include
'<member_of_group>rosidl_interface_packages</member_of_group>' in their
package.xml
```

### 2.5 `CMakeLists.txt` — `rosidl_generate_interfaces`

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotMode.msg"
  "srv/SetRobotMode.srv"
  "action/MoveToHome.action"
  # DEPENDENCIES std_msgs  # if the messages use a type from another package
)
```

- `find_package(...)` loads the macro into scope. Without it, `rosidl_generate_interfaces` says "unknown function".
- `rosidl_generate_interfaces` must be called **BEFORE `ament_package()`**. `ament_package()` finalizes the
  package; no new target can be added afterward.
- `DEPENDENCIES` list: if a message uses another package's type **inside** it (e.g. `std_msgs/Header header`),
  that package goes here **and** also into `package.xml` as `<depend>std_msgs</depend>` — it must be in
  **both** places.

### 2.6 `.msg` syntax

```
# Constants — uppercase, type + name + = + value
uint8 MODE_IDLE    = 0
uint8 MODE_RUNNING = 1
uint8 MODE_FAULT   = 2
uint8 MODE_ESTOP   = 3

# Field — lowercase snake_case
uint8 mode
```

- **Constants** uppercase, assigned a value with `=`. Compile-time integer constants, accessed under a
  namespace (`irp_msgs::msg::RobotMode::MODE_RUNNING`).
- **Fields** snake_case, just type + name.
- **`#`** for comments.

Naming convention: **type names PascalCase, field names snake_case**. Writing `uint8 RobotMode` makes
`RobotMode` a field name — a reader wonders "is this a type?".

### 2.7 `.srv` syntax

```
# Request section
RobotMode mode
---
# Response section
bool success
string message
```

- Single separator: **`---`** (three dashes, at the start of a line, alone).
- Request on top (client → server), response below (server → client).
- **Reference to a type from the same package:** just `RobotMode mode`. The full path
  (`irp_msgs/RobotMode`) is **not required**.
- **Reference to a type from another package:** full path like `std_msgs/Header header`.

**Typical pattern:** `bool success + string message` in the response — almost universal convention in the
ROS2 ecosystem. A human-readable message instead of an opaque error code.

### 2.8 `.action` syntax

```
# Goal section
float64 max_velocity_scaling
---
# Result section
bool success
string message
float64 final_error
---
# Feedback section
float64 percent_complete
```

- **Two** separators, **three** sections.
- Order: **goal → result → feedback**.
- Any section may be completely empty (only a comment, or nothing).

A single `.action` file produces **three message types**: `<Name>_Goal`, `<Name>_Result`,
`<Name>_Feedback`. It also sets up **six channels** behind the scenes:
- 3 services: send_goal, cancel_goal, get_result
- 2 topics: feedback, status

One file, six endpoints — that's what's under the hood of the action protocol.

### 2.9 Type composition

Message types can be referenced inside `.srv` and `.action`:

```
RobotMode mode          # from the same package
---
geometry_msgs/Pose pose # from another package
```

Benefits:
- **Type safety** — the client sends a full `RobotMode` instance, not a bare `uint8`; the constants come
  packed with the type.
- **Wire format is identical** (`RobotMode` is just a single `uint8` → same bytes).
- **Self-documenting** — a reader understands "this is a mode".

`ros2 interface show` expands nested types with indentation:
```
RobotMode mode
        uint8 MODE_IDLE    = 0
        uint8 MODE_RUNNING = 1
        uint8 MODE_FAULT   = 2
        uint8 MODE_ESTOP   = 3
        uint8 mode
---
bool success
string message
```

---

## 3. Lifecycle node

### 3.1 Why does it exist?

The standard `rclcpp::Node` follows a "start in the constructor, end in the destructor" model — insufficient
for production:
- Can't wait for hardware boot time (publishing before the sensor is ready → garbage data)
- Hard to establish deterministic startup order (perception → planner → controller)
- No safe runtime pause/resume (on a fault, stop the controller but keep perception alive)
- Composable managers can't coordinate

Solution: **managed lifecycle** — an explicit state machine, with transitions triggered from the outside.

### 3.2 State machine

```
unconfigured  ──configure──▶  inactive  ──activate──▶  active
                              ▲                       │
                              └──────deactivate───────┘
                              │
                          cleanup ──▶ unconfigured
                              │
                          shutdown ──▶ finalized
```

Five primary transitions: **configure, activate, deactivate, cleanup, shutdown**.

Each transition's callback:

| Callback | Triggered by | What to do |
|---|---|---|
| `on_configure` | configure | read parameters, **create** publishers/subscribers (don't activate yet), connect to hardware |
| `on_activate` | activate | `activate()` the publishers, start the real work |
| `on_deactivate` | deactivate | `deactivate()` the publishers, **keep** resources in memory |
| `on_cleanup` | cleanup | **destroy** publishers/subscribers, disconnect hardware, give memory back |
| `on_shutdown` | shutdown | final teardown |

**Critical distinction:** `inactive ↔ active` is cheap (just "passive/active" switching), `unconfigured ↔
inactive` is expensive (alloc/dealloc). Keep frequently-triggered transitions on the cheap side.

**The constructor's role is narrowed:** only dependency-free init (default values of member variables).
Don't touch hardware, read parameters, or create publishers → all of that goes to `on_configure`.

### 3.3 Lifecycle publisher

`rclcpp_lifecycle::LifecyclePublisher` — even if you call `publish()` while `inactive`, the message is
**dropped**. It prevents publishing messages by bypassing the state machine. It only actually sends in the
`active` state.

### 3.4 Process ≠ State

An important conceptual point: lifecycle state and the OS process are **independent**.

The `shutdown` transition moves the state to `finalized` but does not exit the process. It doesn't close
automatically; your `on_shutdown` or `main()` has to say "stop spinning and call `rclcpp::shutdown()`".

Observation from the CLI:
```bash
$ ros2 lifecycle set /lifecycle_demo shutdown
Transitioning successful
$ ros2 lifecycle get /lifecycle_demo
finalized [4]
$ ros2 lifecycle set /lifecycle_demo configure
Unknown transition requested, available ones are:    # empty list — terminal state
```

`finalized` is a terminal state with no exit. To restart, kill the process and respawn it.

### 3.5 CLI exploration

```bash
ros2 lifecycle nodes                          # all lifecycle nodes
ros2 lifecycle get /<node>                    # current state
ros2 lifecycle list /<node>                   # valid transitions from the current state
ros2 lifecycle set /<node> <transition>       # trigger a transition
```

`ros2 lifecycle list` changes with the state — good for exploring the state machine live from the CLI.

---

## 4. C++ patterns (in the ROS2 context)

### 4.1 Inheritance + constructor + member initializer list

```cpp
class LifecycleDemo : public rclcpp_lifecycle::LifecycleNode
{
public:
  LifecycleDemo()
  : rclcpp_lifecycle::LifecycleNode("lifecycle_demo")   // ← member init list
  {
    // constructor body
  }
};
```

- `: public Y` — public inheritance (standard in ROS2).
- `public:` — access specifier (default is `private`, ROS2 callbacks must be public).
- **Member initializer list** (`: Base("name")`) — runs BEFORE the constructor body, passes arguments to
  the base class constructor. The node name is **always given here** (not in the constructor body, the base
  must be constructed first).

### 4.2 `override` keyword

```cpp
CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
```

- `override` = "I am overriding a virtual function from the base class."
- If you write it wrong (parameter type doesn't match, typo in the name, etc.) → compile error → catches
  bugs early.
- Optional but **always write it**.

### 4.3 `const Type &` parameters

```cpp
on_configure(const rclcpp_lifecycle::State &)
```

- `&` reference — no copying.
- `const` — the function promises not to modify this object.
- **No name** — if a parameter is unused, you don't have to name it.

### 4.4 Smart pointer + `make_shared`

```cpp
auto node = std::make_shared<LifecycleDemo>();
```

- `std::make_shared<T>(...)` — create `T` on the heap, return a `std::shared_ptr<T>`.
- Reference-counted; auto-deleted when the last reference disappears.
- Manual `new`/`delete` is almost never used in ROS2 — everything is shared_ptr (or unique_ptr).

### 4.5 `auto` keyword

Compile-time type deduction. **Not** Python's dynamic typing; the compiler deduces the expression's type,
but the type is still fixed.

### 4.6 `->` vs `.`

- `obj.member` — member access from an object or reference.
- `ptr->member` — member access through a pointer (raw or shared_ptr). Same as `(*ptr).member`, its shortcut.

### 4.7 `using` type alias

```cpp
using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
```

C++11 syntax — the modern form of the old `typedef`. To bind long namespace paths to short local names.

### 4.8 `rclcpp::spin` for lifecycle nodes

```cpp
rclcpp::spin(node->get_node_base_interface());  // ✓ lifecycle node
rclcpp::spin(node);                              // ✗ compile error — not for lifecycle
```

A lifecycle node is in a different hierarchy from a plain Node. The `spin()` overloads expect specific
interface types; `get_node_base_interface()` gives the correct handle.

---

## 5. CMake patterns (ROS2 / ament)

### 5.1 Executable target

```cmake
add_executable(<target_name> <source_files...>)
ament_target_dependencies(<target_name> <ros2_packages...>)
install(TARGETS <target_name> DESTINATION lib/${PROJECT_NAME})
```

- `add_executable` — define a binary.
- `ament_target_dependencies` — link ROS packages + add include paths. The shortcut for what you'd do
  manually in plain CMake with `target_include_directories` + `target_link_libraries`.
- `install(TARGETS ... DESTINATION lib/${PROJECT_NAME})` — the conventional path `ros2 run <pkg> <bin>` looks for.

### 5.2 `${PROJECT_NAME}` variable

A CMake variable, set by `project(<name>)`. Using a variable instead of hardcoding — change the package name
in one place and everything updates.

---

## 6. Mistakes made and lessons learned

| Mistake | Symptom | Fix | Lesson |
|---|---|---|---|
| Putting `<member_of_group>` inside `<export>` | CMake "must include `<member_of_group>...`" error | Move it to top level | Official metadata top-level, tool config inside `<export>` |
| Running `colcon build` from inside `src/` | `build/install/log` created under `src/` | Run from the workspace root | colcon treats CWD as the workspace |
| Field name in PascalCase (`uint8 RobotMode`) | Build passes but name collision/confusion | Use snake_case (`uint8 mode`) | Type = PascalCase, field = snake_case |
| Writing the pkgname as a dep in `ros2 pkg create --dependencies ... <pkgname>` | `<depend>irp_examples</depend>` + `find_package(irp_examples REQUIRED)` self-dep | Remove manually | Read command arguments carefully |
| Source changed, no rebuild | `ros2 interface show` shows the old state | `colcon build` again | Source ↔ install distinction |
| `ros2 pkg create` opened `include/`+`src/` for an interface-only package | Empty dirs, broken intent signal | Delete | The default boilerplate isn't always appropriate |
| New package created at the repo root, not under `src/` | colcon can't find the package | `mv <pkg> src/` | The directory you call `ros2 pkg create` from matters |

---

## 7. Frequently used commands

```bash
# Build
colcon build --packages-select <pkg>
colcon build                              # all packages
colcon build --symlink-install            # symlink source changes into install (useful for Python)

# Introduce the workspace
source install/setup.bash

# Interface discovery
ros2 interface list
ros2 interface list | grep irp_msgs
ros2 interface show <pkg>/<type>/<Name>
ros2 interface proto <pkg>/<type>/<Name>  # JSON template

# Node + lifecycle
ros2 node list
ros2 run <pkg> <executable>
ros2 lifecycle nodes
ros2 lifecycle get /<node>
ros2 lifecycle list /<node>
ros2 lifecycle set /<node> <transition>

# Create a package (call from inside src/!)
ros2 pkg create --build-type ament_cmake --dependencies <deps...> <pkg_name>
```

---

## 8. Completed steps (P0)

- [x] `irp_msgs` package
  - `msg/RobotMode.msg` — `uint8 mode` + 4 constants (IDLE/RUNNING/FAULT/ESTOP)
  - `srv/SetRobotMode.srv` — `RobotMode mode` request, `bool success + string message` response
  - `action/MoveToHome.action` — `float64 max_velocity_scaling` goal, `bool success + string message + float64 final_error` result, `float64 percent_complete` feedback
- [x] Lifecycle node example (`irp_examples/lifecycle_demo`)
  - 5 transition callbacks implemented (configure/activate/deactivate/cleanup/shutdown)
  - All transitions tested from the CLI
  - Verified the `unconfigured → inactive → active → inactive → unconfigured → finalized` flow

Next (continuing P0):
- [ ] Layered launch system (YAML config + Python composition)
- [ ] 3 different QoS profiles demo
- [ ] UR5e xacro
- [ ] joint hierarchy in rviz
