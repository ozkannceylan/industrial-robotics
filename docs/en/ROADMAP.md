# ROADMAP

> Phase-by-phase progress tracker for the `industrial-robotics` project.
> Full plan and architectural principles: project context / `docs/PLAN.md`.

---

## Status

- **Current phase:** P0 — ROS2 fluency
- **Started:** 2026-05-15
- **Target completion:** Q3 2026 (16–24 weeks, ~15–20 h/week)
- **Pace:** slow-and-sure. No deadline pressure.

**Status legend:** 🟢 done · 🟡 in-progress · ⚪ todo · 🔴 blocked

---

## Phase Overview

| #   | Phase                         | Stage       | Status         | Exit criteria (short)                                          |
| --- | ----------------------------- | ----------- | -------------- | -------------------------------------------------------------- |
| P0  | ROS2 fluency                  | Foundation  | 🟡 In progress | A from-scratch UR5e xacro renders correctly in RViz            |
| P1  | `ros2_control` deep dive      | Foundation  | ⚪ Todo        | Custom hardware_interface + controller plugin run in Gazebo    |
| P2  | Kinematics from scratch       | Foundation  | ⚪ Todo        | FK + Jacobian + IK; numerical match against Pinocchio          |
| P3  | Joint + Cartesian controllers | Control     | ⚪ Todo        | Cartesian impedance controller runs in RT loop under jitter    |
| P4  | MoveIt2 integration           | Control     | ⚪ Todo        | 3-planner benchmark + programmatic planning scene              |
| P5  | Perception                    | Integration | ⚪ Todo        | Hand-eye calibration + AprilTag visual servoing                |
| P6  | Force-controlled tasks        | Integration | ⚪ Todo        | Peg-in-hole solved with a hybrid force-position controller     |
| P7  | Task layer                    | Integration | ⚪ Todo        | BT + recovery scenarios via error injection                    |
| P8  | Multi-robot generalization    | Production  | ⚪ Todo        | UR5e → Franka swap: only description + hardware change          |
| P9  | Production hardening          | Production  | ⚪ Todo        | PREEMPT_RT + CI + tests + Foxglove dashboard                   |
| P10 | Capstone                      | Production  | ⚪ Todo        | Multi-step task runs on two robots via a single launch arg     |

---

## Stage 1: Foundation (4–6 weeks)

### P0 — ROS2 fluency 🟡

- [x] `irp_msgs` package: at least 1 custom message, 1 service, 1 action
- [x] Lifecycle node example (configure → activate → deactivate)
- [x] Layered launch system (YAML config + Python composition)
- [x] Mini demo showing the difference between at least 3 QoS profiles
- [x] UR5e xacro written from scratch (vendor URDF read, not copied)
- [x] Correct link/joint hierarchy visible in RViz — display.launch.py, RobotModel+TF Status: Ok, verified with the 6 joint sliders
- [ ] Anti-pattern check: no file copied verbatim from a tutorial

**Architectural decisions (to settle during P0):**
- Xacro decomposition granularity
- Injection path for vendor-specific parameters (so the Franka swap in P8 stays clean)
- ros2_control tags: inside the URDF or in a separate file

### P1 — `ros2_control` deep dive ⚪

- [ ] Mock `hardware_interface` plugin (state echo only)
- [ ] Gazebo connection via `gz_ros2_control`
- [ ] Custom controller plugin (simplest: pass-through forward controller)
- [ ] Controller chain configure and runtime switching
- [ ] **Reading:** `ResourceManager`, `ControllerManager`, `ControllerInterface` headers
- [ ] **Reading:** `ur_robot_driver` hardware plugin structure

### P2 — Kinematics from scratch ⚪

- [ ] UR5e DH parameters (confirmed from vendor datasheet)
- [ ] Forward Kinematics (C++ or Python)
- [ ] Geometric Jacobian
- [ ] Damped least-squares IK
- [ ] Numerical match test against Pinocchio (unit test)
- [ ] **Reading:** Modern Robotics (Lynch & Park) Ch 3–6

---

## Stage 2: Control (3–5 weeks)

### P3 — Joint and Cartesian controllers ⚪

- [ ] Custom `joint_trajectory_controller` (hand-written)
- [ ] Cartesian velocity controller (Jacobian-based, singularity-aware damping)
- [ ] **Cartesian impedance controller** (most critical)
- [ ] RT loop discipline: no alloc, no dynamic dispatch in the hot path
- [ ] Jitter measurement and report

### P4 — MoveIt2 integration ⚪

- [ ] All config files hand-written (SRDF, kinematics.yaml, OMPL planner config, controllers.yaml)
- [ ] Programmatic planning-scene populating interface
- [ ] Planner benchmark: RRTConnect vs RRTstar vs CHOMP (success rate, path length, time)
- [ ] MoveIt2 → clean handoff to the custom controller stack

---

## Stage 3: Integration (5–7 weeks)

### P5 — Perception ⚪

- [ ] Gazebo camera plugin integration
- [ ] Intrinsic calibration (manual OpenCV, chessboard or ChArUco)
- [ ] **Hand-eye calibration** (critical milestone)
- [ ] AprilTag-based pose estimation
- [ ] Simple visual servoing demo (eye → arm)
- [ ] tf2 frame naming discipline doc

### P6 — Force-controlled tasks ⚪

- [ ] Simulated F/T sensor (noise + drift modeling)
- [ ] Hybrid force-position controller (hand-written, not MoveIt's built-in)
- [ ] Peg-in-hole success: specific tolerance + repeatability
- [ ] A "clean", deadline-free version of the task

### P7 — Task layer ⚪

- [ ] Pick-place tree with BehaviorTree.CPP v4
- [ ] FSM alternative of the same task (for comparison)
- [ ] Error injection harness (sensor drop, action fail, collision detect)
- [ ] Recovery behaviors: retry, fallback, safe-abort

---

## Stage 4: Production (4–6 weeks)

### P8 — Multi-robot generalization ⚪

- [ ] Franka Panda xacro under `irp_description`
- [ ] Franka hardware adapter under `irp_hardware/franka_adapter/`
- [ ] Same task tree runs on two robots (swap via launch arg)
- [ ] **Audit:** end of P8 — list of changed files. Ideal: only `irp_description` + `irp_hardware`.

### P9 — Production hardening ⚪

- [ ] PREEMPT_RT kernel install + cyclictest baseline
- [ ] Integration test suite with launch_testing
- [ ] gtest (C++) + pytest (Python) unit test coverage
- [ ] GitHub Actions CI: build + test on push
- [ ] Foxglove Studio dashboard (RT plots + node graph)

### P10 — Capstone ⚪

- [ ] Multi-step task: lift → place → insert
- [ ] UR5e ↔ Franka swap works via a single launch arg
- [ ] README finalize (project purpose + quick start + architecture diagram)
- [ ] `docs/DESIGN.md` finalize
- [ ] Package dependency graph (auto-generated)
- [ ] Demo video

---

## Progress Log

| Date       | Phase | Milestone                          | Notes                                            |
| ---------- | ----- | ---------------------------------- | ------------------------------------------------ |
| 2026-05-15 | P0    | Environment + repo skeleton        | Ubuntu 24.04, ROS2 Jazzy, Gazebo Harmonic installed |
| 2026-05-15 | P0    | `irp_msgs` package                 | RobotMode.msg + SetRobotMode.srv + MoveToHome.action; understood the rosidl pipeline |
| 2026-05-18 | P0    | Lifecycle node demo                | `irp_examples/lifecycle_demo_node.cpp`; `LifecyclePublisher` + WallTimer; symmetric create/activate/deactivate/cleanup tear-up/down; manual test via the ros2 lifecycle CLI. |
| 2026-05-18 | P0    | Layered launch + YAML config       | `irp_examples/launch/lifecycle_demo.launch.py` + `config/lifecycle_demo.yaml`; automatic configure→activate via `LifecycleNode` action + `OnStateTransition` event handler; a single `ros2 launch` command starts the message flow. |
| 2026-06-09 | P0    | QoS demo (3 profiles + mismatch + late-join) | `irp_examples/src/qos_demo_node.cpp`: sensor (BestEffort/KL1/Volatile), command (Reliable/KL10/Volatile), config (Reliable/KL1/TransientLocal). DDS incompatibility warning `mismatch_rx=0`, late subscriber received `config#1` after 3s. Internalized lambda capture (value vs ref) + QoS compatibility rule (pub ≥ sub). |
| 2026-06-22 | P0    | UR5e xacro + TF2 listener demo | New package `irp_description` (`urdf/ur5e.urdf.xacro` + `common.xacro` macros + `display.launch.py` + `ur5e.rviz`): 9 links / 6 revolute + 2 fixed, UR5e datasheet segment lengths, axis pattern Z-Y-Y-Y-Z-Y, primitive geometry (no meshes), `camera_link` fixed joint → /tf_static. `irp_examples/tf2_listener_demo_node.cpp`: Buffer+TransformListener+lookupTransform+try/catch. Written from scratch, not copied from `ur_description`. `colcon build` + `check_urdf` passed; visually verified in RViz. Doc: `docs/en/tf2_urdf_structure.md`. |
| 2026-06-23 | P0    | C++ patterns: safety gate + static TF broadcaster | `irp_examples/command_safety_gate_node.cpp`: topic(/cmd_vel_in)+service(/reset Trigger)+param(max_speed, rejects negatives)+RAII `lock_guard`/mutex in one node; smart_ptr/const/bind-vs-lambda/`std::clamp`. `irp_examples/static_mount_broadcaster_node.cpp`: programmatic `StaticTransformBroadcaster` base_link→lidar_link, RPY→quaternion. `colcon build` clean; runtime smoke-test passed (param rejection + tf2_echo [1.5,0,1.8]). Doc: `docs/en/cpp_patterns.md`. |

---

## Notes for Future Self

- When a phase's "Exit criteria" are met, update this file: status 🟢, add a date and short note to the Progress Log.
- Architectural decisions are **not** kept here; they live in `docs/PLAN.md` or a separate `docs/decisions/` (ADR).
- If the plan changes: update the ROADMAP and write the **rationale for the change** into the Progress Log.
