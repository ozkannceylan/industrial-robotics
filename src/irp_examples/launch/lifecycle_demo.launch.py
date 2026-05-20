from launch import LaunchDescription
from launch_ros.actions import LifecycleNode
import os
from ament_index_python.packages import get_package_share_directory
from launch.actions import EmitEvent, RegisterEventHandler
from launch_ros.events.lifecycle import ChangeState
from launch_ros.event_handlers import OnStateTransition
from lifecycle_msgs.msg import Transition
import launch.events

def generate_launch_description():

    config_file = os.path.join(
    get_package_share_directory('irp_examples'),
    'config',
    'lifecycle_demo.yaml',
)

    # Define node actions
    lifecycle_demo_node = LifecycleNode(
        package='irp_examples',
        executable='lifecycle_demo',
        name='lifecycle_demo',
        namespace='',
        output='screen',
        parameters=[config_file]
    )
    configure_event = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=launch.events.matches_action(lifecycle_demo_node),
            transition_id=Transition.TRANSITION_CONFIGURE,
        )
    )
    activate_on_inactive = RegisterEventHandler(
    OnStateTransition(
        target_lifecycle_node=lifecycle_demo_node,
        goal_state='inactive',
        entities=[
            EmitEvent(event=ChangeState(
                lifecycle_node_matcher=launch.events.matches_action(lifecycle_demo_node),
                transition_id=Transition.TRANSITION_ACTIVATE,
            )),
        ],
    )
)


    return LaunchDescription([
        lifecycle_demo_node,
        configure_event,
        activate_on_inactive
    ])
