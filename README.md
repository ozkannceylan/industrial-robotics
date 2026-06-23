# industrial-robotics

A from-scratch ROS2 project building toward an industrial manipulation stack
(UR5e, with a path to multi-robot generalization). Everything is written by hand —
messages, nodes, descriptions, and launch — to understand the framework end to end
rather than wiring together prebuilt examples.

**Stack:** ROS2 Jazzy · C++ (rclcpp) · Gazebo Harmonic · Ubuntu 24.04

## Packages

| Package | What it is |
|---|---|
| `irp_msgs` | Interface package: a custom message, service, and action (`RobotMode`, `SetRobotMode`, `MoveToHome`). |
| `irp_examples` | Standalone example nodes — lifecycle, QoS, TF2 listener/broadcaster, and an rclcpp integration node. |
| `irp_description` | UR5e robot description in xacro + RViz/TF visualization. |

## Quick start

```bash
# From the workspace root
colcon build
source install/setup.bash

# Visualize the UR5e (RViz + joint sliders)
ros2 launch irp_description display.launch.py

# Run an example node
ros2 run irp_examples lifecycle_demo
ros2 run irp_examples qos_demo
ros2 run irp_examples tf2_listener_demo
```

Available `irp_examples` nodes: `lifecycle_demo`, `qos_demo`, `tf2_listener_demo`,
`command_safety_gate`, `static_mount_broadcaster`.

## Documentation

See [docs/](docs/) — available in English ([docs/en](docs/en)) and Turkish ([docs/tr](docs/tr)):

- **ROADMAP** — phases, exit criteria, progress log
- **notes** — workspace, interfaces, lifecycle, C++/CMake patterns
- **qos_demo_walkthrough** — line-by-line walkthrough of the QoS demo
- **tf2_urdf_structure** — the UR5e description + TF code
- **cpp_patterns** — modern C++ patterns in the integration nodes

## Status

Phase **P0 — ROS2 fluency** (in progress). See [docs/en/ROADMAP.md](docs/en/ROADMAP.md)
for the full phase plan (P0–P10).
