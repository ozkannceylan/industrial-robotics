#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <std_msgs/msg/string.hpp>

using namespace std::chrono_literals;


// The lifecycle callback return type lives deep in a namespace.
// Alias it locally so callback signatures stay readable.
using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

// Inherit from LifecycleNode (NOT the plain rclcpp::Node) to opt into
// the managed lifecycle state machine.
class LifecycleDemo : public rclcpp_lifecycle::LifecycleNode
{
public:
  // Constructor: only dependency-free init here.
  // No hardware access, no parameter reads, no publisher creation yet.
  LifecycleDemo()
  : rclcpp_lifecycle::LifecycleNode("lifecycle_demo")
  {
    RCLCPP_INFO(get_logger(), "Constructor done (state: unconfigured)");
    declare_parameter<std::string>("greeting", "hello world");
  }

  // unconfigured -> inactive
  // Allocate resources, read parameters, create (but don't activate) publishers.
  CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_configure: allocating resources");
    pub_ = create_publisher<std_msgs::msg::String>("greeting", 10);
    return CallbackReturn::SUCCESS;
  }

  // inactive -> active
  // Switch publishers on; start doing real work.
  CallbackReturn on_activate(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_activate: now running");
    pub_->on_activate();
    timer_ = create_wall_timer(1s, [this]() {
      auto msg = std_msgs::msg::String();
      msg.data = get_parameter("greeting").as_string() + " " + std::to_string(count_++);
      pub_->publish(msg);
    });
    return CallbackReturn::SUCCESS;
  }

  // active -> inactive
  // Pause work but keep resources allocated (cheap to re-activate).
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_deactivate: paused");
    timer_.reset();
    pub_->on_deactivate();
    return CallbackReturn::SUCCESS;
  }

  // inactive -> unconfigured
  // Release everything that on_configure allocated.
  CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_cleanup: resources released");
    pub_.reset();
    return CallbackReturn::SUCCESS;
  }

  // any state -> finalized
  // Final teardown; node will not run again.
  CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_shutdown: terminating");
    timer_.reset();
    pub_.reset();
    return CallbackReturn::SUCCESS;
  }

private:
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  size_t count_ = 0;


};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<LifecycleDemo>();

  // Lifecycle nodes need the node-base interface, not the node itself.
  // rclcpp::spin(node) won't compile.
  rclcpp::spin(node->get_node_base_interface());

  rclcpp::shutdown();
  return 0;
}
