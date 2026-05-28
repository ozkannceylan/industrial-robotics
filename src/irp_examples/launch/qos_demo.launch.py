from launch import LaunchDescription
from launch_ros.actions import Node               # LifecycleNode DEGIL
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    config_file = os.path.join(
        get_package_share_directory('irp_examples'),
        'config',
        'qos_demo.yaml',
    )

    qos_demo_node = Node(
        package='irp_examples',
        executable='qos_demo',
        name='qos_demo',
        output='screen',
        parameters=[config_file],
    )

    return LaunchDescription([
        qos_demo_node,
    ])
