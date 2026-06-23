cmake_minimum_required(VERSION 3.8)
project(irp_description)

find_package(ament_cmake REQUIRED)

# This is a description-only package: no compiled targets.
# We just install the xacro/urdf, launch, and rviz config into the share space
# so robot_state_publisher / ros2 launch / rviz2 can find them at runtime.
install(DIRECTORY
  urdf
  launch
  rviz
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
