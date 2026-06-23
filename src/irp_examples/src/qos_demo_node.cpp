// QoS Demo Node
// ----------------------------------------------------------------------------
// 3 topic, 3 farkli QoS profili:
//   sensor   -> BestEffort + KeepLast(1)  + Volatile         (hizli, kayipli)
//   command  -> Reliable   + KeepLast(10) + Volatile         (kaybedilemez)
//   config   -> Reliable   + KeepLast(1)  + TransientLocal   (gec-katilan da alir)
//
// Bonus: sensor topic'ine kasten "Reliable" subscriber bagla -> QoS mismatch
// (rclcpp uyari basar, mesaj akmaz).
// ----------------------------------------------------------------------------

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

using namespace std::chrono_literals;
using StringMsg = std_msgs::msg::String;

class QosDemo : public rclcpp::Node
{
public:
  QosDemo()
  : rclcpp::Node("qos_demo")
  {
    RCLCPP_INFO(get_logger(), "QosDemo kuruluyor");

    // ---- 1) SENSOR topic --------------------------------------------------
    // QoS profilini olustur (builder pattern). Asagidaki satir referans:
    auto sensor_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                        .best_effort()
                        .durability_volatile();


    sensor_pub_ = create_publisher<StringMsg>("sensor", sensor_qos);



    sensor_sub_ = create_subscription<StringMsg>("sensor", sensor_qos,
            [this](const StringMsg::SharedPtr) { sensor_rx_count_++; });


    sensor_timer_ = create_wall_timer(10ms, [this]() {
    StringMsg m;
    m.data = "sensor#" + std::to_string(sensor_tx_count_++);
    sensor_pub_->publish(m);
    });

    auto command_qos = rclcpp::QoS(rclcpp::KeepLast(10))
                        .reliable()
                        .durability_volatile();
    // Publisher    
    command_pub_ = create_publisher<StringMsg>("command", command_qos);
    // Subscriber
    command_sub_ = create_subscription<StringMsg>("command", command_qos,
        [this](const StringMsg::SharedPtr) { command_rx_count_++; });
    // Timer
    command_timer_ = create_wall_timer(200ms, [this]() {
        StringMsg m;
        m.data = "command#" + std::to_string(command_tx_count_++);
        command_pub_->publish(m);
    });

    // ---- 3) CONFIG topic --------------------------------------------------
    // Publisher hemen olustur ve TEK BIR mesaj yayinla.
    // Subscriber'i 3 saniye sonra (wall_timer ile) kur — late-join testi.
    // TODO: config_qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();
    // TODO: config_pub_ create + publish (bir kere).
    // TODO: late_sub_setup_timer_ = create_wall_timer(3s, [this]() {
    //         // burada config_sub_ olustur, sonra timer'i reset et.
    //       });

    auto config_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                            .reliable()
                            .transient_local();
    // Publisher
    config_pub_ = create_publisher<StringMsg>("config", config_qos);

    StringMsg config_msg;

    config_msg.data = "config#1";
    config_pub_->publish(config_msg);

    // Timer
    late_sub_setup_timer_ = create_wall_timer(3s, [this, config_qos]() {
        // Subscriber
        config_sub_ = create_subscription<StringMsg>("config", config_qos,
            [this](const StringMsg::SharedPtr msg) { 
                RCLCPP_INFO(get_logger(), "Late subscriber received: '%s'", msg->data.c_str());
            });
        late_sub_setup_timer_->cancel();
    });

    // ---- 4) MISMATCH (opsiyonel ama egitici) ------------------------------
    // sensor topic'i BestEffort, biz Reliable isteyen bir sub kuruyoruz.
    // TODO: mismatch_qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable();
    // TODO: mismatch_sub_ = create_subscription<StringMsg>("sensor", mismatch_qos,
    //         [this](const StringMsg::SharedPtr) { mismatch_rx_count_++; });

    auto mismatch_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                            .reliable();


    mismatch_sub_ = create_subscription<StringMsg>("sensor", mismatch_qos,
        [this](const StringMsg::SharedPtr) { mismatch_rx_count_++; });
    // ---- Stats log her 2 saniyede ----------------------------------------
    stats_timer_ = create_wall_timer(2s, [this]() {
      RCLCPP_INFO(get_logger(),
        "tx[sensor=%zu cmd=%zu]  rx[sensor=%zu cmd=%zu mismatch=%zu]",
        sensor_tx_count_, command_tx_count_,
        sensor_rx_count_, command_rx_count_, mismatch_rx_count_);
    });
  }

private:
  // Publishers
    rclcpp::Publisher<StringMsg>::SharedPtr sensor_pub_;
    rclcpp::Publisher<StringMsg>::SharedPtr command_pub_;
    rclcpp::Publisher<StringMsg>::SharedPtr config_pub_;

  // Subscriptions
  rclcpp::Subscription<StringMsg>::SharedPtr sensor_sub_;
  rclcpp::Subscription<StringMsg>::SharedPtr command_sub_;
  rclcpp::Subscription<StringMsg>::SharedPtr config_sub_;
  rclcpp::Subscription<StringMsg>::SharedPtr mismatch_sub_;

  // Timers
    rclcpp::TimerBase::SharedPtr sensor_timer_;
    rclcpp::TimerBase::SharedPtr command_timer_;
    rclcpp::TimerBase::SharedPtr late_sub_setup_timer_;
    rclcpp::TimerBase::SharedPtr stats_timer_;

  // Counters
  size_t sensor_tx_count_ = 0;
  size_t sensor_rx_count_ = 0;
  size_t command_tx_count_ = 0;
  size_t command_rx_count_ = 0;
  size_t mismatch_rx_count_ = 0;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<QosDemo>());
  rclcpp::shutdown();
  return 0;
}
