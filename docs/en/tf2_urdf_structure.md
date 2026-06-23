# TF2 + URDF: Repo Structure and Explanation

> This document walks through the **URDF/xacro** in the `irp_description` package and
> the **TF2** code in `irp_examples`: what each file does, why it is written that way,
> and how to run it.

---

## 1. Files — bird's-eye view

```
src/
├── irp_description/                    ← robot description package
│   ├── package.xml                     ament_cmake + xacro/rviz/rsp exec_depends
│   ├── CMakeLists.txt                  no build; installs urdf/launch/rviz only
│   ├── urdf/
│   │   ├── common.xacro                reusable macros
│   │   └── ur5e.urdf.xacro             UR5e kinematic chain (main file)
│   ├── launch/
│   │   └── display.launch.py           xacro→rsp→jsp_gui→rviz pipeline
│   └── rviz/
│       └── ur5e.rviz                   ready-made RViz view (RobotModel + TF + Grid)
│
└── irp_examples/
    └── src/tf2_listener_demo_node.cpp  TF2 lookup (consumer) side
```

**Division of labor between the two packages:**
- `irp_description` = **broadcaster side** (URDF → `robot_state_publisher` → `/tf`, `/tf_static`).
- `irp_examples/tf2_listener_demo` = **consumer side** (listens to `/tf`, queries via `lookupTransform`).

This split mirrors how URDF and TF2 connect in a real system.

---

## 2. URDF / xacro side

### 2.1 Why a separate `irp_description` package?

ROS2 convention: the robot **description** lives in its own package, separate from code.
Reason — **reuse + swap**: the same description is used by simulation, MoveIt, and real hardware.
P8 in the ROADMAP is designed so the UR5e ↔ Franka swap touches "only `irp_description` + `irp_hardware`".
Keeping the description isolated is the groundwork for that goal.

### 2.2 `common.xacro` — macros (the reason xacro exists)

Two macros:

| Macro | What it does | Why |
|---|---|---|
| `cylinder_link` | Takes name/radius/length/mass/material and produces a full `<link>` (visual+collision+inertial) | One template instead of writing all 6 arm links by hand — **DRY** |
| `solid_cylinder_inertial` | Computes a cylinder's inertia tensor (rigid-body formula) | Needed for Gazebo dynamics; writing inertia by hand is error-prone |

The subtlety in `cylinder_link`: the cylinder extends along **+Z** with its base at the link origin
(`origin z = length/2`). So the next joint sits exactly at `z = length` and links **stack cleanly**
along the chain. This keeps the joint-origin math simple.

> **xacro concepts:** `property` = variable, `macro` = parameterized template, `include` = file split,
> `${...}` = expression (math included). At build/launch time the `xacro` tool **expands these into
> plain URDF**; ROS only ever sees plain URDF.

### 2.3 `ur5e.urdf.xacro` — the kinematic chain

**Design decisions (also documented in the file's header comment):**
- **Segment lengths** are UR5e datasheet values: `d1=0.1625, a2=0.425, a3=0.3922, d5=0.0997, d6=0.0996` m.
- **Joint axes** follow the real UR5e pattern: **Z, Y, Y, Y, Z, Y**.
- **Visuals are primitive** (cylinder/box) — no vendor meshes. The aim is a kinematically honest,
  recognizable arm, not photorealism.
- Links stack along +Z so the `joint_state_publisher_gui` sliders move them intuitively.

**Resulting tree** (validated with `check_urdf`):
```
base_link
└─ shoulder_link        (shoulder_pan_joint,  revolute, axis Z)
   └─ upper_arm_link    (shoulder_lift_joint, revolute, axis Y)
      └─ forearm_link   (elbow_joint,         revolute, axis Y)
         └─ wrist_1_link(wrist_1_joint,       revolute, axis Y)
            └─ wrist_2_link (wrist_2_joint,   revolute, axis Z)
               └─ wrist_3_link (wrist_3_joint,revolute, axis Y)
                  └─ tool0          (tool0_fixed_joint,  FIXED)
                     └─ camera_link (camera_mount_joint, FIXED)
```
9 links, 8 joints (6 revolute + 2 fixed).

**`tool0`** — a zero-size link (`<link name="tool0"/>`). The tool flange frame in UR convention.
It has no geometry because it is purely a **frame** (coordinate system); a gripper/sensor attaches here.

**`camera_link` + `camera_mount_joint` (type=fixed)** — a **static transform** example. The camera is
attached to `tool0` with a **fixed** offset (`xyz="0.05 0 0.02" rpy="0 -π/2 0"`). Because it is a fixed
joint, `robot_state_publisher` publishes it on **`/tf_static`**, not `/tf` — the exact analog of a
sensor mount ("sensor mount = static TF").

> **URDF → TF bridge (the most important concept):** Each `<joint>` is an edge in the TF tree
> (`parent_link → child_link`). `robot_state_publisher` reads the URDF + `/joint_states`, computes the
> per-angle transform for **revolute** joints onto `/tf`, and publishes the constant transform for
> **fixed** joints onto `/tf_static`. So writing URDF = defining the static skeleton of the TF tree.

### 2.4 `display.launch.py` — the visualization pipeline

Four steps:
1. **Process `xacro`:** `Command(["xacro ", model])` turns the xacro into a URDF string at launch time.
   `ParameterValue(..., value_type=str)` is required so the result is passed as a plain string parameter.
2. **`robot_state_publisher`:** publishes the URDF on `/robot_description` and transforms on `/tf` + `/tf_static`.
3. **`joint_state_publisher_gui`:** slider window; publishes joint angles on `/joint_states` (can be disabled with `gui:=false`).
4. **`rviz2`:** opens with the ready-made `ur5e.rviz` config (RobotModel + TF + Grid, fixed frame `base_link`).

---

## 3. TF2 side — `tf2_listener_demo_node.cpp`

URDF/`robot_state_publisher` **publishes** the transforms; this node **reads** them.

**Skeleton:**
```cpp
tf_buffer_   = std::make_shared<tf2_ros::Buffer>(get_clock());     // cache + interpolation
tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);  // listens to /tf + /tf_static
// ...
t = tf_buffer_->lookupTransform(target_frame_, source_frame_, tf2::TimePointZero);
```

**Key points (also in the code comments):**
- **Order matters:** `Buffer` first, then `TransformListener` (the listener writes into the buffer).
- **`lookupTransform(TARGET, SOURCE, TIME)`** — "the transform that expresses a point in SOURCE in
  TARGET". The default query: `camera_link` in `base_link` = the wrist camera's pose relative to the base.
- **`tf2::TimePointZero`** = "the latest available transform" — no time-sync issues, most robust.
- **`try/catch (tf2::TransformException&)`** — every lookup is wrapped. It throws when the frame doesn't
  exist yet / the tree is disconnected / there's no data for that time. In a real system you **fail safe**
  here (never act on a stale/guessed pose).
- **Parameterized:** `target_frame` / `source_frame` are ROS parameters, changeable at runtime.

---

## 4. Build & run

```bash
cd ~/projects/industrial-robotics
colcon build --packages-select irp_description irp_examples
source install/setup.bash

# 1) Show the robot in RViz (move joints with the sliders)
ros2 launch irp_description display.launch.py

# 2) In another terminal: TF lookup demo (while transforms are being published)
source install/setup.bash
ros2 run irp_examples tf2_listener_demo
#   -> "camera_link in base_link | pos=[...] quat=[...]" every second

# Query a different transform:
ros2 run irp_examples tf2_listener_demo --ros-args -p source_frame:=tool0

# 3) Inspect the TF tree:
ros2 run tf2_tools view_frames        # generates frames.pdf
ros2 topic echo /tf_static            # see the camera + tool0 static transforms
ros2 run tf2_ros tf2_echo base_link camera_link
```

**Verification status:**
- `colcon build` — both packages built without warnings. ✅
- `xacro ... | check_urdf` — XML valid, tree is the expected serial chain. ✅
- RViz visual verification — RobotModel + TF Status: Ok, verified with the 6 joint sliders. ✅

---

## 5. RViz visual verification checklist

After `ros2 launch irp_description display.launch.py`:
- [ ] RobotModel is visible, no red errors.
- [ ] Fixed Frame `base_link`; TF axes visible on each link.
- [ ] `joint_state_publisher_gui` sliders → the relevant link rotates about the correct axis
      (shoulder_pan sweeps horizontally, shoulder_lift raises the arm, etc.).
- [ ] `camera_link` (red box) is at the wrist tip, fixed to tool0; it moves with the arm.
- [ ] The chain in the TF tree is unbroken (base_link → ... → camera_link, a single branch).

> **Note (QoS):** `robot_state_publisher` publishes `/robot_description` as **transient_local** (latched).
> The RViz RobotModel display's "Description Topic" durability must also be **Transient Local**, otherwise
> RViz (joining after the publish, i.e. late-join) won't receive the latched message and the model won't show.

---

## 6. Next steps

- This model uses **primitive geometry**; real UR5e meshes could be added later (optional).
- In P1, `ros2_control` tags will be added to this xacro (deliberately absent for now).
- Joint origins are a learning model; the exact DH-consistent UR5e pose will be settled in P2 (kinematics).
- **Technical debt:** `base_link` has an inertia; KDL does not support inertia on a root link
  ("KDL does not support a root link with an inertia" warning). Before P1/P2, making `base_link`
  inertia-free and adding a `base_link_inertia` child cleans this up.
