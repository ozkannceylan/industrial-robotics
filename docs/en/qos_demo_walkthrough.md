# `qos_demo_node.cpp` — Line-by-Line Walkthrough

> Explains [src/irp_examples/src/qos_demo_node.cpp](../../src/irp_examples/src/qos_demo_node.cpp) **line by line**.
> Byproduct: a C++ syntax crash course (only the concepts that appear in this file).
>
> Goal: when you close the file, be able to say "**why** each line is there".

---

## 0. What this node does — 30-second summary

One node, three publishers + three subscribers (+ one mismatch sub). Each publisher uses a different
**QoS profile**:

| Topic    | Reliability | History     | Durability        | Scenario                       |
| -------- | ----------- | ----------- | ----------------- | ------------------------------ |
| sensor   | BestEffort  | KeepLast(1) | Volatile          | High frequency, loss tolerated |
| command  | Reliable    | KeepLast(10)| Volatile          | Command, cannot be lost        |
| config   | Reliable    | KeepLast(1) | **TransientLocal**| A late-joining subscriber must also receive it |

Bonus: an **intentionally incompatible** (Reliable) subscriber connects to the `sensor` topic → a DDS
warning + no messages flow.

A stats timer prints tx/rx counters every 2 seconds.

---

## 1. C++ Crash Course (only what appears in this file)

I assume you're a senior programmer; I explain with a **Node.js/Python** reference. This is **not** a
self-sufficient C++ course — just enough to understand which oddity you ran into here.

### 1.1 `#include`

```cpp
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
```

The ancestor of JS's `require`/`import`, but **purely textual**. The preprocessor finds the file and pastes
its contents in place of this line. `<...>` for standard/system paths, `"..."` for in-project paths (a loose
convention).

- `.hpp` = header file. Class declarations, template definitions, inline functions live here.
- In ROS2, `rclcpp/rclcpp.hpp` is the **convenience header** that pulls in "all of rclcpp".

### 1.2 `namespace` + `using`

```cpp
using namespace std::chrono_literals;
using StringMsg = std_msgs::msg::String;
```

- **namespace** = a name bucket (like a module in Python / a package in Java). `std::chrono::milliseconds`
  → `std` the main namespace, `chrono` a sub-namespace, `milliseconds` the type.
- `using namespace X;` → you can use the things inside that namespace as if they belonged to the current
  scope. Dangerous — don't do it globally, it creates name clashes.
- `using A = B;` → **type alias** (like TypeScript `type`). Writing `StringMsg` means `std_msgs::msg::String`.
  Purely for readability.
- `chrono_literals` is a special namespace: it contains **user-defined literals** like `10ms`, `2s`. Without
  the `using`, `10ms` won't compile.

### 1.3 `class` and access specifiers

```cpp
class QosDemo : public rclcpp::Node
{
public:
  QosDemo() : rclcpp::Node("qos_demo") { ... }
private:
  rclcpp::Publisher<StringMsg>::SharedPtr sensor_pub_;
};
```

- `class X : public Y` → `X` **inherits** from `Y` (like `extends Y`). `public` here means "Y's public
  members are also public in X".
- `public:` / `private:` → set the access level of the members written **below** them. Once `public:` is
  written, all members are public until a new specifier appears.
- The default inside a class is `private:` (in a struct it's `public:`).
- C++ convention: private members get a trailing underscore (`sensor_pub_`). `_name` and `__name` are
  **reserved** for the C++ standard — don't use them.

### 1.4 Constructor + member initializer list

```cpp
QosDemo()
: rclcpp::Node("qos_demo")
{
  RCLCPP_INFO(get_logger(), "...");
}
```

- `QosDemo() : rclcpp::Node("qos_demo")` — the part after `:` is the **member initializer list**. It runs
  before the curly braces `{}` open.
- Especially important here: we call the **constructor** of the parent class `rclcpp::Node`. Node has no
  default constructor; it must have a name.
- JS analogy: `class QosDemo extends Node { constructor() { super("qos_demo"); ... } }`. The `super()` call
  is the initializer list.
- Member fields are also **directly constructed** here (construction, not assignment). Critical for
  performance + support for `const`/reference members.

### 1.5 Templates

```cpp
rclcpp::Publisher<StringMsg>::SharedPtr
create_publisher<StringMsg>("sensor", sensor_qos);
```

- You pass a **type parameter** inside `<>`. Compile-time generics. Same mental model as TypeScript generics,
  but entirely compile-time; at runtime `Publisher<StringMsg>` and `Publisher<Int32>` are **different types**.
- `create_publisher<StringMsg>("sensor", qos)` — means "create a publisher that publishes StringMsg".
- `::SharedPtr` — a nested type alias defined inside the class (usually `using SharedPtr = std::shared_ptr<X>;`).

### 1.6 Smart pointers — `shared_ptr`

```cpp
rclcpp::Publisher<StringMsg>::SharedPtr sensor_pub_;
std::make_shared<QosDemo>();
```

- Managing an object's lifetime in C++ is **manual**. If you create on the heap with `new`, you must
  `delete` it; forget and you get a memory leak.
- `std::shared_ptr<T>` — a reference-counted smart pointer. It counts how many copies exist and `delete`s the
  object when the last copy is gone. Feels close to Python/JS GC but is deterministic.
- `std::make_shared<T>(args...)` — creates `T` on the heap and returns a `shared_ptr<T>`. Prefer it over using
  `new T(...)` directly (allocation optimization + exception safety).
- In this file **all** ROS2 publishers/subscribers/timers are held as `shared_ptr` — because the ROS2 executor
  also holds references to them; your variable isn't the owner, it's a **co-owner**.

### 1.7 `auto` keyword

```cpp
auto sensor_qos = rclcpp::QoS(rclcpp::KeepLast(1)).best_effort();
```

- `auto` — the compiler deduces the type of the right-hand expression and assigns it to the variable (like
  TypeScript `let x = ...`, Go `x := ...`).
- Useful for hiding long type names. Here `sensor_qos`'s type is `rclcpp::QoS` — you could write it, but
  `auto` is cleaner.

### 1.8 `chrono` literals

```cpp
using namespace std::chrono_literals;
create_wall_timer(10ms, [this]() { ... });
create_wall_timer(3s,  [this]() { ... });
```

- `10ms`, `200ms`, `3s` → user-defined literals that turn into types like `std::chrono::milliseconds(10)`.
- Won't compile without `using namespace std::chrono_literals;`.
- The `chrono` library = C++'s type-safe duration/time library. Pass a `seconds` to something expecting
  `milliseconds` and the compiler converts automatically; but there is no direct conversion from `int` to
  `seconds` (a deliberate check).

### 1.9 Lambdas and the capture list — **the most critical part of this file**

```cpp
sensor_sub_ = create_subscription<StringMsg>("sensor", sensor_qos,
    [this](const StringMsg::SharedPtr) { sensor_rx_count_++; });
```

- `[capture](params) -> ReturnType { body }` → defines a **closure**. Like a JS arrow function but the
  capture is explicit.
- The `[capture]` list says which outer variables the lambda can access, and **how**:
  - `[]` — capture nothing.
  - `[x]` — capture `x` by value (copy).
  - `[&x]` — capture `x` by reference (linked to the original outside).
  - `[this]` — capture the `this` pointer so you can access class members.
  - `[=]` — capture all variables by value (murky; prefer an explicit list).
  - `[&]` — capture all variables by reference (dangerous).

**JS analogy + the difference:** In JS closures are automatic. In C++ it's **explicit** — what you capture,
by copy or by reference, you write all of it. Because a variable created on the stack disappears when its
stack frame dies — if you captured by reference and the lambda runs after the frame is gone, that's a
**dangling reference** (UB). That's why **when** the lambda runs is critical.

The examples in this file:

| Place | Capture | Why |
|---|---|---|
| sensor_sub_ callback | `[this]` | `sensor_rx_count_++` is a member, needs `this` |
| sensor_timer_ callback | `[this]` | `sensor_pub_`, `sensor_tx_count_` are members; doesn't need `sensor_qos` |
| late_sub_setup_timer_ | `[this, config_qos]` | `config_qos` is a local; by the time the timer fires the constructor scope is closed — value capture is required |

> Had you written `[this, &config_qos]`: when the timer fires 3 seconds later, `config_qos` would already be
> erased from the stack → **undefined behavior**. Like a crash or reading random bytes.

### 1.10 Various oddities

- `const StringMsg::SharedPtr` parameter → the incoming message in a callback. `SharedPtr` is already
  copyable; `const` says "I won't modify this pointer" (not its contents — that would need
  `SharedPtr<const StringMsg>`).
- `m.data = "sensor#" + std::to_string(...)` — `std::string` has an overloaded `+` operator for
  concatenation. `to_string` turns int → `std::string`.
- Create the node with `std::make_shared<QosDemo>()`, drop it into the event loop with `rclcpp::spin`.
  `spin` blocks; it runs callbacks until SIGINT arrives.

---

## 2. ROS2 QoS — minimal theory

It has three axes (3 of DDS's 20+ policies that ROS2 uses often):

### 2.1 Reliability
- **BestEffort:** "Send it; if it's lost, it's lost, no retry." Low latency, loss tolerated.
- **Reliable:** "Retry until ACK, deliver it." Latency + bandwidth cost.

### 2.2 History (Depth + Kind)
- **KeepLast(N):** Keep the last N messages in memory. If a slow consumer can't keep up, the oldest is dropped.
- **KeepAll:** Drop nothing (up to the RMW limit). Rare in practice.

### 2.3 Durability
- **Volatile:** The publisher doesn't keep history. Messages published before you subscribe are **lost**.
- **TransientLocal:** The publisher keeps the last N (= history depth) messages; sends them to a
  **late-joining** subscriber.

### 2.4 Compatibility rule

`pub ≥ sub` (on each axis). That is, the publisher's offer must be able to **satisfy** the subscriber's request.

- Reliable pub + BestEffort sub → matches
- BestEffort pub + Reliable sub → **no match**
- TransientLocal pub + Volatile sub → matches
- Volatile pub + TransientLocal sub → **no match**

When DDS discovery notices the incompatibility, it prints `requested QoS profile is incompatible` and
**no messages flow**.

### 2.5 Builder pattern

```cpp
auto qos = rclcpp::QoS(rclcpp::KeepLast(1))
            .best_effort()
            .durability_volatile();
```

- The `rclcpp::QoS` constructor takes History (depth + kind).
- Other settings come from **chained setter** calls: each setter returns `*this` → chaining.
- The pattern's name: **fluent interface / builder pattern**. Like Java `StringBuilder`,
  JS `array.map(...).filter(...)`.

---

## 3. The file, line by line

### 3.1 Header comment block (lines 1–10)

```cpp
// QoS Demo Node
// ----------------------------------------------------------------------------
// 3 topics, 3 different QoS profiles:
//   sensor   -> BestEffort + KeepLast(1)  + Volatile         (fast, lossy)
//   command  -> Reliable   + KeepLast(10) + Volatile         (cannot be lost)
//   config   -> Reliable   + KeepLast(1)  + TransientLocal   (late-joiner also receives)
//
// Bonus: intentionally attach a "Reliable" subscriber to the sensor topic -> QoS mismatch
// (rclcpp prints a warning, no messages flow).
// ----------------------------------------------------------------------------
```

File-top documentation. The Doxygen format (`///`, `/** */`) is also common in C++, but this is a teaching
node, so simple `//` comments are enough.

### 3.2 Includes (12–13)

```cpp
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
```

- `rclcpp/rclcpp.hpp` — `rclcpp::Node`, `rclcpp::QoS`, `rclcpp::Publisher`, `rclcpp::spin`, `rclcpp::init`,
  etc. The convenience header that pulls them all in at once.
- `std_msgs/msg/string.hpp` — a generated header. ROS2's standard `std_msgs/String` message type. `rosidl`
  produces it during the build. If you include a message type that hasn't been built, you get `file not found`.

### 3.3 Namespace and type alias (15–16)

```cpp
using namespace std::chrono_literals;
using StringMsg = std_msgs::msg::String;
```

- `chrono_literals` → the `10ms`, `200ms`, `2s`, `3s` literals.
- `StringMsg` → a shorthand to avoid writing `std_msgs::msg::String` everywhere. Purely cosmetic.

> **Why is `using namespace` dangerous at global scope?** Throughout the whole file, all names inside
> `std::chrono` enter "without knocking". If a name clashes, you get a compile or semantic error. There's no
> problem here because `chrono_literals` is a very narrow namespace (only the `_ms`, `_s`, `_ns`, etc.
> operators). As a general rule, use it inside **function scope** or not at all.

### 3.4 Class declaration (18–20)

```cpp
class QosDemo : public rclcpp::Node
{
public:
```

- Inheriting from `rclcpp::Node` → we get the ROS2 node infrastructure (parameters, logging, executor
  handoff) for free.
- `public:` → the constructor comes next.

### 3.5 Constructor head (21–23)

```cpp
QosDemo()
: rclcpp::Node("qos_demo")
{
  RCLCPP_INFO(get_logger(), "QosDemo kuruluyor");
```

- The constructor takes no parameters. The node name is given to the parent constructor in the initializer
  list — this name will appear in `ros2 node list`.
- `RCLCPP_INFO(logger, fmt, ...)` → ROS2's printf-style logger macro. `get_logger()` is a helper from the
  parent Node.

### 3.6 Sensor QoS + publisher + subscriber + timer (28–50)

```cpp
auto sensor_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                    .best_effort()
                    .durability_volatile();

sensor_pub_ = create_publisher<StringMsg>("sensor", sensor_qos);

sensor_sub_ = create_subscription<StringMsg>("sensor", sensor_qos,
        [this](const StringMsg::SharedPtr) { sensor_rx_count_++; });

sensor_timer_ = create_wall_timer(10ms, [this]() {
  StringMsg m;
  m.data = "sensor#" + std::to_string(sensor_tx_count_++);
  sensor_pub_->publish(m);
});
```

Line by line:

1. `rclcpp::QoS(rclcpp::KeepLast(1))` — construct a QoS object with History=KeepLast(1).
2. `.best_effort()` — Reliability=BestEffort. The setter returns `*this` → chaining.
3. `.durability_volatile()` — Durability=Volatile. (Default is already Volatile; writing it explicitly is documentation.)
4. `create_publisher<StringMsg>("sensor", sensor_qos)` — a method from the base class Node. "Publish this
   type of message to this topic, with this QoS." Returns a `shared_ptr<Publisher<StringMsg>>` → assigned to
   the member `sensor_pub_`.
5. `create_subscription<StringMsg>("sensor", sensor_qos, callback)` — subscribe to the same topic. The
   **callback** is called every time a message arrives.
6. The lambda parameter `const StringMsg::SharedPtr` — the incoming message. We didn't name it (didn't write
   `msg`) because we don't use it; we just increment a counter. With C++17+ the parameter name is optional.
7. `create_wall_timer(10ms, lambda)` — runs the lambda every 10ms by the wall clock (wall clock, *not*
   simulation time).
8. Lambda body: create a new `StringMsg`, write a string into the `data` field, publish with the publisher.
   `sensor_tx_count_++` is a **post-increment** — use the current value first, then increment. The first
   message is `"sensor#0"`.

> Why self-publish + self-subscribe? The same node is both publisher and subscriber — so we can see the tx/rx
> difference as a mismatch-free baseline. With the same QoS it always matches.

### 3.7 Command (55–68)

Exactly the same pattern as `sensor`, only the QoS profile differs:

```cpp
auto command_qos = rclcpp::QoS(rclcpp::KeepLast(10))
                    .reliable()
                    .durability_volatile();
```

- `KeepLast(10)` — buffers 10 messages for a slow consumer; typical for keeping messages awaiting a Reliable
  retry in the buffer.
- 200ms period → 5Hz, a "command" cadence.

### 3.8 Config + late-join test (79–98)

```cpp
auto config_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                        .reliable()
                        .transient_local();
config_pub_ = create_publisher<StringMsg>("config", config_qos);

StringMsg config_msg;
config_msg.data = "config#1";
config_pub_->publish(config_msg);

late_sub_setup_timer_ = create_wall_timer(3s, [this, config_qos]() {
    config_sub_ = create_subscription<StringMsg>("config", config_qos,
        [this](const StringMsg::SharedPtr msg) {
            RCLCPP_INFO(get_logger(), "Late subscriber received: '%s'", msg->data.c_str());
        });
    late_sub_setup_timer_->cancel();
});
```

The **heart** of it. The flow:

1. `config_qos` = Reliable + KeepLast(1) + **TransientLocal**.
2. The publisher is created immediately.
3. **`config#1` is published immediately** — there are no subscribers yet.
4. A 3-second timer is set up. The lambda captures `this` + `config_qos` **by value**.
5. When the timer fires, **a subscriber is created**. Because the pub is TransientLocal, the last published
   message (`config#1`) is in memory → as soon as the sub matches, DDS delivers the copy to it.
6. The inner callback prints "Late subscriber received: 'config#1'" via `RCLCPP_INFO`.
7. `late_sub_setup_timer_->cancel()` → the timer stops, never fires again.

> **Why value capture in `[this, config_qos]`?** `config_qos` is a local created in the constructor's scope.
> When the constructor `{...}` block closes (i.e. after the node is fully constructed), that stack variable
> dies. The lambda fires 3 seconds later — at which point `config_qos` is already gone. Had we captured by
> reference, that would be a dangling reference → UB.
>
> Why is `this` captured? Inside the lambda we assign to the member `config_sub_` and call
> `late_sub_setup_timer_->cancel()`. These are class members; they can't be accessed without `this`.

> **`cancel()` or `reset()`?** `cancel()` stops the timer; the object stays alive but never triggers again.
> `reset()` (a shared_ptr method) empties the shared_ptr → the timer object goes down the destruction path.
> Here `cancel` is correct — a simple "don't call again" intent.

### 3.9 Mismatch subscriber (106–111)

```cpp
auto mismatch_qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable();

mismatch_sub_ = create_subscription<StringMsg>("sensor", mismatch_qos,
    [this](const StringMsg::SharedPtr) { mismatch_rx_count_++; });
```

- The `sensor` topic publisher is BestEffort. This sub wants Reliable. The compatibility rule is broken
  (`BestEffort < Reliable`).
- DDS discovery warns both sides:
  - "New publisher discovered on topic '/sensor', offering incompatible QoS." (from the Reliable sub side)
  - "New subscription discovered on topic '/sensor', requesting incompatible QoS." (from the BestEffort pub side)
- Result: `mismatch_rx_count_` stays 0 forever.

### 3.10 Stats timer (113–118)

```cpp
stats_timer_ = create_wall_timer(2s, [this]() {
  RCLCPP_INFO(get_logger(),
    "tx[sensor=%zu cmd=%zu]  rx[sensor=%zu cmd=%zu mismatch=%zu]",
    sensor_tx_count_, command_tx_count_,
    sensor_rx_count_, command_rx_count_, mismatch_rx_count_);
});
```

- A summary report every 2 seconds. `%zu` = the `size_t` printf format (32/64-bit depending on platform).
- The `[this]` capture is enough — all variables are members.

### 3.11 Private members (121–144)

```cpp
private:
  rclcpp::Publisher<StringMsg>::SharedPtr sensor_pub_;
  // ...
  size_t sensor_tx_count_ = 0;
```

- Publisher/Subscription/Timer are all `SharedPtr` — the ROS2 executor also holds references to them, shared
  ownership.
- `size_t` — like `unsigned int`, 32 or 64 bits depending on platform. Idiomatic for counters.
- `= 0` — an **in-class member initializer**. No need to assign again in the constructor.

### 3.12 `main` (147–153)

```cpp
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<QosDemo>());
  rclcpp::shutdown();
  return 0;
}
```

- `rclcpp::init` — set up the ROS2 client library (initialize RMW, install signal handlers, etc.). It also
  processes command-line arguments (`__node:=...`, `--ros-args ...`).
- `std::make_shared<QosDemo>()` — create `QosDemo` on the heap, return a `shared_ptr<QosDemo>`. (If you create
  it on the stack, `spin` wants a shared_ptr argument; the types won't match.)
- `rclcpp::spin(node)` — a **blocking call**. It hands the node to the default single-threaded executor and
  starts calling callbacks. It returns when SIGINT (Ctrl-C) arrives.
- `rclcpp::shutdown` — cleanup; `init` can be called again.

> `int main(int argc, char ** argv)` — inherited from C, the standard `main` signature. `argv` is a C-string
> array; `char *` is a pointer to a pointer.

---

## 4. Running it + expected output

```bash
colcon build --packages-select irp_examples
source install/setup.bash
ros2 launch irp_examples qos_demo.launch.py
```

What to expect:

- **Two** warnings at startup: incompatible QoS, both RELIABILITY_QOS_POLICY.
- Around ~3 seconds: `Late subscriber received: 'config#1'`.
- Stats every 2 seconds: `tx[sensor=…] rx[sensor=…sensor_tx, cmd=cmd_tx, mismatch=0]`.

Seeing the mismatch sub receive **no messages** is the proof of the QoS incompatibility concept. BestEffort
arriving lossless on loopback = the DDS shared-memory transport works within the same process; on a real
network you'd see drops.

---

## 5. Frequently asked / confusing points

- **Could you hold a `rclcpp::Node` instance instead of inheriting from it?** You could, but it's not
  idiomatic. Inheritance is cleaner; it gives direct access to helpers (`get_logger`, `create_publisher`...).
- **`make_shared` vs `new`?** Always prefer `make_shared`. Single allocation, exception safety, less boilerplate.
- **Should I take the `SharedPtr` by `const &` as a parameter?** The subscription callback signature already
  wants `const SharedPtr`. In your own functions, `const std::shared_ptr<T>&` is more efficient (no copy, no
  ref-count bump).
- **What if the timer were a local instead of a member?** It wouldn't work — if created on the stack, the
  `shared_ptr` reference is lost when the scope ends, the ref-count drops, and the timer is destroyed. We keep
  it as a member so it lives for the duration of the executor.
- **Is self-pub/self-sub in the same node meaningful?** Rarely in production. Here it's for teaching — we can
  see the whole flow in a single process.

---

## 6. Checklist for the next step

When you close this document you should be able to say the following **without looking at the file**:

- [ ] **Why** the difference between `[this]` and `[this, config_qos]` is there the way it is.
- [ ] I recognize the **builder pattern** in `auto sensor_qos = rclcpp::QoS(...).best_effort().durability_volatile()`,
      and I know how to look up which setters exist in rclcpp.
- [ ] I can correctly answer which BestEffort + Reliable combinations **match**.
- [ ] I can explain TransientLocal's late-join behavior **in two sentences**.
- [ ] What `std::make_shared<T>(...)` does, and why we use it instead of `new T()`.
- [ ] Why `using namespace std::chrono_literals;` is needed, and what breaks if I remove it.
- [ ] What `rclcpp::spin(node)` does, and when it returns.

If anything is missing, go back to that line's section.
