// Static TF Broadcaster — programmatic static transform example
// ----------------------------------------------------------------------------
// The *producer* side of a STATIC transform, done programmatically instead of
// via URDF/robot_state_publisher. Publishes base_link -> lidar_link once, with a
// fixed translation + rotation — exactly how an AV declares a sensor mount.
//
// Static transforms are latched (transient_local QoS) under the hood, so a node
// that starts LATER still receives this transform (transient_local durability).
//
// Try it (with the TF listener):
//   ros2 run irp_examples static_mount_broadcaster
//   ros2 run irp_examples tf2_listener_demo --ros-args -p target_frame:=base_link -p source_frame:=lidar_link
//   ros2 run tf2_ros tf2_echo base_link lidar_link
// ----------------------------------------------------------------------------

#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2/LinearMath/Quaternion.hpp>
#include <tf2_ros/static_transform_broadcaster.hpp>

class StaticMountBroadcaster : public rclcpp::Node
{
public:
  StaticMountBroadcaster()
  : rclcpp::Node("static_mount_broadcaster")
  {
    // The broadcaster owns a latched publisher on /tf_static.
    broadcaster_ = std::make_shared<tf2_ros::StaticTransformBroadcaster>(this);
    publish_mount();
  }

private:
  void publish_mount()
  {
    geometry_msgs::msg::TransformStamped t;

    // Header: when + which parent frame.
    t.header.stamp = get_clock()->now();
    t.header.frame_id = "base_link";     // parent
    t.child_frame_id = "lidar_link";     // child

    // Translation: 1.5 m forward, 1.8 m up (typical roof-LIDAR offset).
    t.transform.translation.x = 1.5;
    t.transform.translation.y = 0.0;
    t.transform.translation.z = 1.8;

    // Rotation: build a quaternion from roll/pitch/yaw, then copy into the msg.
    // (TF always stores orientation as a quaternion — no gimbal lock, clean slerp.)
    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, 0.0);             // no rotation in this example
    t.transform.rotation.x = q.x();
    t.transform.rotation.y = q.y();
    t.transform.rotation.z = q.z();
    t.transform.rotation.w = q.w();

    broadcaster_->sendTransform(t);
    RCLCPP_INFO(get_logger(), "Published static base_link -> lidar_link");
  }

  std::shared_ptr<tf2_ros::StaticTransformBroadcaster> broadcaster_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  // Spin so the latched transform stays available for late-joining listeners.
  rclcpp::spin(std::make_shared<StaticMountBroadcaster>());
  rclcpp::shutdown();
  return 0;
}
