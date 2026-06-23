cmake_minimum_required(VERSION 3.8)
project(irp_examples)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(lifecycle_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)

# Declare a C++ executable target.
# Source file: src/lifecycle_demo_node.cpp (path relative to this CMakeLists).
add_executable(lifecycle_demo src/lifecycle_demo_node.cpp)
add_executable(qos_demo src/qos_demo_node.cpp)
add_executable(tf2_listener_demo src/tf2_listener_demo_node.cpp)
add_executable(command_safety_gate src/command_safety_gate_node.cpp)
add_executable(static_mount_broadcaster src/static_mount_broadcaster_node.cpp)

# Link the executable against ROS2 packages we depend on.
# ament_target_dependencies handles include dirs + linker flags transitively.
ament_target_dependencies(lifecycle_demo
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  std_msgs
)

ament_target_dependencies(qos_demo
  rclcpp
  std_msgs
)

ament_target_dependencies(tf2_listener_demo
  rclcpp
  geometry_msgs
  tf2_ros
)

ament_target_dependencies(command_safety_gate
  rclcpp
  geometry_msgs
  std_srvs
)

ament_target_dependencies(static_mount_broadcaster
  rclcpp
  geometry_msgs
  tf2
  tf2_ros
)

# Install the binary so `ros2 run irp_examples lifecycle_demo` can find it.
# Convention: package executables go under lib/<package_name>/.
install(TARGETS
  lifecycle_demo
  qos_demo
  tf2_listener_demo
  command_safety_gate
  static_mount_broadcaster
  DESTINATION lib/${PROJECT_NAME}
)
# Install launch files and config so `ros2 launch` and parameter loading can find them.
install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
