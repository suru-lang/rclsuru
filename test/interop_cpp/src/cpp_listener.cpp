// chatter に届いた文を「C++ が聞きました: 文」と表示する
#include <cstdio>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cpp_listener");
  auto sub = node->create_subscription<std_msgs::msg::String>(
    "chatter", 10, [](const std_msgs::msg::String & msg) {
      std::printf("C++ が聞きました: %s\n", msg.data.c_str());
      std::fflush(stdout);
    });
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
