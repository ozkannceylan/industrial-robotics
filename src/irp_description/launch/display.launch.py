# display.launch.py
# -----------------------------------------------------------------------------
# Brings the UR5e description to life for visualization:
#   1. xacro  -> URDF string (processed at launch time)
#   2. robot_state_publisher: publishes the URDF on /robot_description and the
#      link transforms on /tf (movable) and /tf_static (fixed joints).
#   3. joint_state_publisher_gui: sliders to drive the joints (publishes /joint_states).
#   4. rviz2: visualize the RobotModel + TF tree.
#
# Run:  ros2 launch irp_description display.launch.py
#       ros2 launch irp_description display.launch.py gui:=false   # no sliders
# -----------------------------------------------------------------------------

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_share = FindPackageShare("irp_description")

    default_model = PathJoinSubstitution([pkg_share, "urdf", "ur5e.urdf.xacro"])
    default_rviz = PathJoinSubstitution([pkg_share, "rviz", "ur5e.rviz"])

    model_arg = DeclareLaunchArgument(
        "model", default_value=default_model,
        description="Absolute path to the robot xacro/urdf file.")
    gui_arg = DeclareLaunchArgument(
        "gui", default_value="true",
        description="Launch joint_state_publisher_gui sliders (true/false).")

    # Process the xacro at launch time. `value_type=str` is required so the
    # result is passed as a plain string parameter, not a yaml-parsed object.
    robot_description = ParameterValue(
        Command(["xacro ", LaunchConfiguration("model")]),
        value_type=str,
    )

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[{"robot_description": robot_description}],
    )

    joint_state_publisher_gui = Node(
        package="joint_state_publisher_gui",
        executable="joint_state_publisher_gui",
        condition=IfCondition(LaunchConfiguration("gui")),
    )

    rviz = Node(
        package="rviz2",
        executable="rviz2",
        arguments=["-d", default_rviz],
        output="screen",
    )

    return LaunchDescription([
        model_arg,
        gui_arg,
        robot_state_publisher,
        joint_state_publisher_gui,
        rviz,
    ])
