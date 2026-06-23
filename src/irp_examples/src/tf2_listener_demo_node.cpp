// TF2 Listener Demo
// ----------------------------------------------------------------------------
// The "consumer" side of TF2. robot_state_publisher (from irp_description)
// broadcasts the UR5e transforms; this node *looks them up*.
//
// It periodically answers: "where is <source_frame> expressed in <target_frame>?"
// e.g. "where is the wrist camera in the robot base frame?".
//
// Core pattern:
//   Buffer + TransformListener, lookupTransform(target, source, time), try/catch.
//
// Run (after launching display.launch.py so transforms exist):
//   ros2 run irp_examples tf2_listener_demo
//   ros2 run irp_examples tf2_listener_demo --ros-args -p target_frame:=base_link -p source_frame:=tool0
// ----------------------------------------------------------------------------

#include <chrono>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

using namespace std::chrono_literals;

class Tf2ListenerDemo : public rclcpp::Node
{
public:
  Tf2ListenerDemo()
  : rclcpp::Node("tf2_listener_demo")
  {
    // Which transform to query. Defaults: camera (sensor) in base frame.
    target_frame_ = declare_parameter<std::string>("target_frame", "base_link");
    source_frame_ = declare_parameter<std::string>("source_frame", "camera_link");

    // Buffer caches incoming transforms (~10 s by default) and interpolates.
    // TransformListener subscribes to /tf + /tf_static and fills the buffer.
    // Order matters: the buffer must exist before the listener.
    tf_buffer_   = std::make_shared<tf2_ros::Buffer>(get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    timer_ = create_wall_timer(1s, std::bind(&Tf2ListenerDemo::on_timer, this));

    RCLCPP_INFO(get_logger(), "Looking up '%s' in frame '%s' every 1 s",
                source_frame_.c_str(), target_frame_.c_str());
  }

private:
  void on_timer()
  {
    geometry_msgs::msg::TransformStamped t;
    try {
      // lookupTransform(TARGET, SOURCE, TIME):
      //   "give me the transform that expresses a point in SOURCE in TARGET".
      // TimePointZero = "the latest available transform" — robust, no time sync.
      t = tf_buffer_->lookupTransform(
            target_frame_, source_frame_, tf2::TimePointZero);
    } catch (const tf2::TransformException & ex) {
      // Frame not published yet, tree disconnected, or no data for that time.
      // In a real system: fail-safe here, never act on a stale/guessed pose.
      RCLCPP_WARN(get_logger(), "Could not transform '%s' -> '%s': %s",
                  source_frame_.c_str(), target_frame_.c_str(), ex.what());
      return;
    }

    const auto & p = t.transform.translation;
    const auto & q = t.transform.rotation;
    RCLCPP_INFO(get_logger(),
      "%s in %s | pos=[% .3f % .3f % .3f] quat=[% .3f % .3f % .3f % .3f]",
      source_frame_.c_str(), target_frame_.c_str(),
      p.x, p.y, p.z, q.x, q.y, q.z, q.w);
  }

  std::string target_frame_;
  std::string source_frame_;
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Tf2ListenerDemo>());
  rclcpp::shutdown();
  return 0;
}
