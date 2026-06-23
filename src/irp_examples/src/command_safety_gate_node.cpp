// Command Safety Gate — rclcpp integration example
// ----------------------------------------------------------------------------
// One realistic node that brings several modern-C++ / rclcpp patterns together.
// Think of it as a minimal AV "safety gate": it limits how fast a velocity
// command is allowed to be before forwarding it to the actuator side.
//
//   * subscribe  /cmd_vel_in   (geometry_msgs/Twist)
//   * clamp linear.x to ±max_speed, republish on /cmd_vel_out
//   * service    /reset        (std_srvs/Trigger) -> zero the processed counter
//   * parameter  max_speed     with an on-set validation callback (reject < 0)
//
// C++ concepts on display:
//   shared_ptr handles, RAII std::lock_guard for thread safety, const-correct
//   callbacks, std::bind vs lambda, std::clamp, templated Publisher<T>.
//
// Try it:
//   ros2 run irp_examples command_safety_gate
//   ros2 topic pub /cmd_vel_in geometry_msgs/msg/Twist "{linear: {x: 5.0}}"
//   ros2 topic echo /cmd_vel_out            # -> clamped to max_speed
//   ros2 param set /command_safety_gate max_speed 2.0
//   ros2 param set /command_safety_gate max_speed -1.0   # rejected
//   ros2 service call /reset std_srvs/srv/Trigger
// ----------------------------------------------------------------------------

#include <algorithm>   // std::clamp
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_srvs/srv/trigger.hpp>

using Twist = geometry_msgs::msg::Twist;
using Trigger = std_srvs::srv::Trigger;

class CommandSafetyGate : public rclcpp::Node
{
public:
  CommandSafetyGate()
  : rclcpp::Node("command_safety_gate")
  {
    // Declare + read the limit once into a cached member.
    max_speed_ = declare_parameter<double>("max_speed", 1.0);

    // Validate every runtime change BEFORE it is applied (reject negatives).
    param_cb_handle_ = add_on_set_parameters_callback(
      std::bind(&CommandSafetyGate::on_param_update, this, std::placeholders::_1));

    pub_ = create_publisher<Twist>("cmd_vel_out", 10);

    // Lambda callback: short and inline. Dereference the SharedPtr to a const ref.
    sub_ = create_subscription<Twist>("cmd_vel_in", 10,
      [this](const Twist::SharedPtr msg) { on_cmd(*msg); });

    // std::bind: the handler is a named member with the 2-arg service signature.
    reset_srv_ = create_service<Trigger>("reset",
      std::bind(&CommandSafetyGate::on_reset, this,
                std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(get_logger(), "Command safety gate up (max_speed=%.2f)", max_speed_);
  }

private:
  // Topic callback. `const Twist &` in => no copy of the caller's message.
  void on_cmd(const Twist & in)
  {
    double limit;
    {
      // RAII lock: acquired here, released automatically at end of scope —
      // even if an exception is thrown. Guards the shared state below.
      std::lock_guard<std::mutex> lock(mtx_);
      limit = max_speed_;
      ++processed_;
    }
    Twist out = in;
    out.linear.x = std::clamp(in.linear.x, -limit, limit);
    pub_->publish(out);
  }

  // Service callback: 2 args (request, response), no return value.
  // Request unused here, so it is left unnamed.
  void on_reset(const std::shared_ptr<Trigger::Request> /*req*/,
                std::shared_ptr<Trigger::Response> res)
  {
    size_t was;
    {
      std::lock_guard<std::mutex> lock(mtx_);
      was = processed_;
      processed_ = 0;
    }
    res->success = true;
    res->message = "counter reset (was " + std::to_string(was) + ")";
    RCLCPP_INFO(get_logger(), "%s", res->message.c_str());
  }

  // Parameter validation callback. Runs BEFORE the new value is committed; if we
  // return successful=false the change is rejected and the old value stays.
  rcl_interfaces::msg::SetParametersResult
  on_param_update(const std::vector<rclcpp::Parameter> & params)
  {
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    for (const auto & p : params) {
      if (p.get_name() == "max_speed") {
        const double v = p.as_double();
        if (v < 0.0) {
          result.successful = false;
          result.reason = "max_speed must be >= 0";
          continue;
        }
        std::lock_guard<std::mutex> lock(mtx_);
        max_speed_ = v;
        RCLCPP_INFO(get_logger(), "max_speed updated -> %.2f", v);
      }
    }
    return result;
  }

  // Every rclcpp handle is a SharedPtr — reference-counted, freed automatically.
  rclcpp::Publisher<Twist>::SharedPtr pub_;
  rclcpp::Subscription<Twist>::SharedPtr sub_;
  rclcpp::Service<Trigger>::SharedPtr reset_srv_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_cb_handle_;

  std::mutex mtx_;          // guards max_speed_ + processed_
  double max_speed_ = 1.0;
  size_t processed_ = 0;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CommandSafetyGate>());
  rclcpp::shutdown();
  return 0;
}
