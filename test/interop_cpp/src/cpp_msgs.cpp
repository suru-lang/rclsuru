// 段階2(メッセージの型と QoS)の相互運用の試験に使う C++ のノード
//   cmd_vel(geometry_msgs/Twist)を聞いて表示する
//   joint_states(sensor_msgs/JointState)へ 0.2 秒ごとに送る
//   scan(sensor_msgs/LaserScan)へ sensor_data の QoS(best effort)で 0.2 秒ごとに送る
//   latched(std_msgs/String)へ transient_local の QoS で始めに1回だけ送る
#include <chrono>
#include <cstdio>
#include <memory>
#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "std_msgs/msg/string.hpp"

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cpp_msgs");
  auto twist_sub = node->create_subscription<geometry_msgs::msg::Twist>(
    "cmd_vel", 10, [](const geometry_msgs::msg::Twist & m) {
      std::printf("C++ が Twist を聞きました: linear.x=%g angular.z=%g\n", m.linear.x, m.angular.z);
      std::fflush(stdout);
    });
  auto joint_pub = node->create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
  auto scan_pub = node->create_publisher<sensor_msgs::msg::LaserScan>("scan", rclcpp::SensorDataQoS());
  auto latched_pub = node->create_publisher<std_msgs::msg::String>(
    "latched", rclcpp::QoS(1).transient_local());
  std_msgs::msg::String latched;
  latched.data = "最初の1回だけ";
  latched_pub->publish(latched);
  int count = 0;
  auto timer = node->create_wall_timer(std::chrono::milliseconds(200), [&]() {
    ++count;
    sensor_msgs::msg::JointState j;
    j.header.stamp = node->now();
    j.header.frame_id = "腕";
    j.name = {"肩", "肘"};
    j.position = {0.5, -1.25};
    j.velocity = {static_cast<double>(count)};
    joint_pub->publish(j);
    sensor_msgs::msg::LaserScan s;
    s.header.frame_id = "laser";
    s.angle_min = -1.5f;
    s.angle_max = 1.5f;
    s.ranges = {1.0f, 2.5f, 4.0f};
    s.intensities = {};
    scan_pub->publish(s);
  });
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
