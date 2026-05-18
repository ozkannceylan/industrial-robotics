cmake_minimum_required(VERSION 3.8)
project(irp_msgs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# Loads the rosidl_generate_interfaces macro into this scope.
find_package(rosidl_default_generators REQUIRED)

# Run the rosidl pipeline:
#   1) read each listed schema file
#   2) generate C++ headers, Python module, type-support, introspection
#   3) compile + install everything as library targets
# Must be called BEFORE ament_package().
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotMode.msg"
  "srv/SetRobotMode.srv"
  "action/MoveToHome.action"
  # DEPENDENCIES <pkg>  # add if a .msg references another package's type
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
