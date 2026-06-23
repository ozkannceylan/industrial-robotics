# Modern C++ Patterns: Repo Structure and Explanation

> A map of the two integration nodes added to the `irp_examples` package: which file
> shows which C++ concept, how to run it, and what was verified.

---

## 1. Files

```
src/irp_examples/
├── src/command_safety_gate_node.cpp        topic+service+param+RAII integration
├── src/static_mount_broadcaster_node.cpp   programmatic static TF
├── CMakeLists.txt                          + 2 targets, + std_srvs / tf2 find_package
└── package.xml                             + <depend>std_srvs</depend>, <depend>tf2</depend>
```

Both were added to the existing `irp_examples` package (no new package needed).

---

## 2. `command_safety_gate_node.cpp` — integration node

**What it does:** A minimal "safety gate". It subscribes to `/cmd_vel_in` (Twist), **clamps** `linear.x`
to `±max_speed`, and republishes on `/cmd_vel_out`. The `/reset` (Trigger) service zeroes the processed
command counter. The `max_speed` parameter can change at runtime; a negative value is **rejected**.

**Which C++ concept lives where:**

| Concept | Code | Logic |
|---|---|---|
| **shared_ptr** | all `pub_/sub_/reset_srv_` members are `...::SharedPtr`; `make_shared` in `main` | every rclcpp handle is reference-counted |
| **RAII / lock_guard** | `std::lock_guard<std::mutex> lock(mtx_)` in `on_cmd`, `on_reset`, `on_param_update` | `mtx_` guards `processed_` + `max_speed_`; auto-unlock on scope exit |
| **const correctness** | `on_cmd(const Twist & in)`, `const double v` | no copy + immutable |
| **std::bind vs lambda** | sub = **lambda**, service + param cb = **std::bind** | short inline → lambda; named member (esp. 2-arg service) → bind |
| **template** | `create_publisher<Twist>`, `create_service<Trigger>` | compile-time type |
| **param validation** | `add_on_set_parameters_callback` → `result.successful=false` rejects | rejects a negative `max_speed` |
| **std::clamp** | `std::clamp(in.linear.x, -limit, limit)` | C++17 standard algorithm |

**Why a mutex?** Under a `MultiThreadedExecutor`, the topic callback and the service callback can run on
**different threads**, and both touch `processed_` → a **data race**. The `lock_guard` guards it. Not
strictly required under a SingleThreadedExecutor, but it is the correct habit and thread-safe by design.

**Run & try:**
```bash
ros2 run irp_examples command_safety_gate
# another terminal:
ros2 topic pub /cmd_vel_in geometry_msgs/msg/Twist "{linear: {x: 5.0}}"
ros2 topic echo /cmd_vel_out                       # linear.x -> clamped to max_speed
ros2 param set /command_safety_gate max_speed 2.0  # accepted
ros2 param set /command_safety_gate max_speed -1.0 # REJECTED
ros2 service call /reset std_srvs/srv/Trigger      # reset the counter
```

---

## 3. `static_mount_broadcaster_node.cpp` — programmatic static TF

**What it does:** Publishes the `base_link -> lidar_link` static transform **in code** (instead of via
URDF/`robot_state_publisher`). Translation (1.5, 0, 1.8) + rotation (RPY→quaternion). This is the
broadcaster side of TF; `tf2_listener_demo` is the consumer side — together they complete TF.

**Key points:**
- `tf2_ros::StaticTransformBroadcaster` — under the hood it holds a **latched** (`transient_local` QoS)
  publisher on `/tf_static` → a late-joining listener still receives it.
- `tf2::Quaternion q; q.setRPY(...)` — TF always stores orientation as a **quaternion** (no gimbal lock,
  clean interpolation). Convert from RPY to a quaternion and copy into `t.transform.rotation`.
- Fill `t.header.stamp = get_clock()->now()`, `frame_id` (parent) + `child_frame_id` (child).
- `spin` so the latched transform stays available for listeners.

**Run & try:**
```bash
ros2 run irp_examples static_mount_broadcaster
# another terminal:
ros2 run irp_examples tf2_listener_demo --ros-args -p target_frame:=base_link -p source_frame:=lidar_link
ros2 run tf2_ros tf2_echo base_link lidar_link     # Translation: [1.5, 0, 1.8]
ros2 topic echo /tf_static
```

---

## 4. Verification

- ✅ `colcon build --packages-select irp_examples` — built without warnings.
- ✅ `command_safety_gate` runtime: `max_speed=-1.0` rejected ("must be >= 0"), `2.0` accepted, parameter
  callback log correct.
- ✅ `static_mount_broadcaster` runtime: `tf2_echo base_link lidar_link` → `Translation: [1.500, 0.000,
  1.800]`, identity rotation — as coded.

---

## 5. All `irp_examples` nodes in the repo

| Node | Shows |
|---|---|
| `lifecycle_demo` | managed lifecycle, 5 transitions, lifecycle publisher |
| `qos_demo` | 3 QoS profiles + mismatch + late-join |
| `tf2_listener_demo` | Buffer+Listener, `lookupTransform`, try/catch |
| `command_safety_gate` | smart_ptr + RAII mutex + params + topic + service |
| `static_mount_broadcaster` | programmatic static TF, RPY→quaternion |
| (`irp_description`) | UR5e xacro → robot_state_publisher → /tf |

All run via `ros2 run irp_examples <node>` (after `colcon build` + `source install/setup.bash`).

---

## 6. Known technical debt

- `command_safety_gate`'s thread safety assumes a `MultiThreadedExecutor`; the default `spin` is
  single-threaded. A real RT scenario needs callback groups + executor tuning.
- The `base_link` inertia warning is still open — KDL does not support inertia on a root link
  (see `tf2_urdf_structure.md`).
