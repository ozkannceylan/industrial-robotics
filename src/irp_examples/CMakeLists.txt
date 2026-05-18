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

# Declare a C++ executable target.
# Source file: src/lifecycle_demo_node.cpp (path relative to this CMakeLists).
add_executable(lifecycle_demo src/lifecycle_demo_node.cpp)

# Link the executable against ROS2 packages we depend on.
# ament_target_dependencies handles include dirs + linker flags transitively.
ament_target_dependencies(lifecycle_demo
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
)

# Install the binary so `ros2 run irp_examples lifecycle_demo` can find it.
# Convention: package executables go under lib/<package_name>/.
install(TARGETS lifecycle_demo
  DESTINATION lib/${PROJECT_NAME}
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
