// lookup(tf2_msgs/action/LookupTransform)の C++ のアクションサーバー。
// feedback を3回送り、child_frame_id が「target_frame + _cpp」の結果を返す。source_frame が slow なら10回(取り消せる)
#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "tf2_msgs/action/lookup_transform.hpp"

using Lookup = tf2_msgs::action::LookupTransform;
using Handle = rclcpp_action::ServerGoalHandle<Lookup>;

static void execute(std::shared_ptr<Handle> h) {
  auto goal = h->get_goal();
  int n = goal->source_frame == "slow" ? 10 : 3;
  auto result = std::make_shared<Lookup::Result>();
  result->transform.child_frame_id = goal->target_frame + "_cpp";
  for (int i = 0; i < n; i++) {
    if (h->is_canceling()) {
      std::printf("C++: 取り消しを受けました\n");
      h->canceled(result);
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    h->publish_feedback(std::make_shared<Lookup::Feedback>());
  }
  h->succeed(result);
}

int main(int argc, char ** argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cpp_lookup_server");
  auto server = rclcpp_action::create_server<Lookup>(
    node, "lookup",
    [](const rclcpp_action::GoalUUID &, std::shared_ptr<const Lookup::Goal> goal) {
      std::printf("C++ が目標を受けました: %s\n", goal->target_frame.c_str());
      return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
    },
    [](std::shared_ptr<Handle>) { return rclcpp_action::CancelResponse::ACCEPT; },
    [](std::shared_ptr<Handle> h) { std::thread(execute, h).detach(); });
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
