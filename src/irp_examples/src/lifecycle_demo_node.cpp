#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

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
    return CallbackReturn::SUCCESS;
  }

  // inactive -> active
  // Switch publishers on; start doing real work.
  CallbackReturn on_activate(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_activate: now running");
    return CallbackReturn::SUCCESS;
  }

  // active -> inactive
  // Pause work but keep resources allocated (cheap to re-activate).
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_deactivate: paused");
    return CallbackReturn::SUCCESS;
  }

  // inactive -> unconfigured
  // Release everything that on_configure allocated.
  CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_cleanup: resources released");
    return CallbackReturn::SUCCESS;
  }

  // any state -> finalized
  // Final teardown; node will not run again.
  CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "on_shutdown: terminating");
    return CallbackReturn::SUCCESS;
  }
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
