// 0.2 秒ごとに chatter へ「C++ から N」を送る
#include <chrono>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cpp_talker");
  auto pub = node->create_publisher<std_msgs::msg::String>("chatter", 10);
  int count = 0;
  auto timer = node->create_wall_timer(std::chrono::milliseconds(200), [&]() {
    std_msgs::msg::String msg;
    msg.data = "C++ から " + std::to_string(++count);
    pub->publish(msg);
  });
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
