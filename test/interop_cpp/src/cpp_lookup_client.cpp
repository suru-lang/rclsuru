// lookup(tf2_msgs/action/LookupTransform)に目標を送る C++ のアクションクライアント。
// 1つ目は終わるまで待ち、2つ目(source_frame が slow)は 1 秒後に取り消す
#include <chrono>
#include <cstdio>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "tf2_msgs/action/lookup_transform.hpp"

using Lookup = tf2_msgs::action::LookupTransform;
using Client = rclcpp_action::Client<Lookup>;

static const char * code_name(rclcpp_action::ResultCode c) {
  switch (c) {
    case rclcpp_action::ResultCode::SUCCEEDED: return "SUCCEEDED";
    case rclcpp_action::ResultCode::CANCELED: return "CANCELED";
    case rclcpp_action::ResultCode::ABORTED: return "ABORTED";
    default: return "UNKNOWN";
  }
}

static int run(rclcpp::Node::SharedPtr node, Client::SharedPtr client, const char * source, bool cancel) {
  Lookup::Goal goal;
  goal.target_frame = "map";
  goal.source_frame = source;
  Client::SendGoalOptions opt;
  opt.feedback_callback = [](Client::GoalHandle::SharedPtr, std::shared_ptr<const Lookup::Feedback>) {
    std::printf("C++ が feedback を受けました\n");
  };
  auto gf = client->async_send_goal(goal, opt);
  if (rclcpp::spin_until_future_complete(node, gf, std::chrono::seconds(5)) !=
      rclcpp::FutureReturnCode::SUCCESS || !gf.get()) {
    std::printf("C++: 目標が受け付けられません\n");
    return 1;
  }
  auto h = gf.get();
  if (cancel) {
    rclcpp::sleep_for(std::chrono::seconds(1));
    auto cf = client->async_cancel_goal(h);
    if (rclcpp::spin_until_future_complete(node, cf, std::chrono::seconds(5)) ==
        rclcpp::FutureReturnCode::SUCCESS) {
      std::printf("C++ が取り消しの答えを受けました: %d\n", cf.get()->return_code);
    }
  }
  auto rf = client->async_get_result(h);
  if (rclcpp::spin_until_future_complete(node, rf, std::chrono::seconds(10)) !=
      rclcpp::FutureReturnCode::SUCCESS) {
    std::printf("C++: 結果が届きません\n");
    return 1;
  }
  auto r = rf.get();
  std::printf("C++ が結果を受けました: %s child_frame_id=%s\n", code_name(r.code),
              r.result->transform.child_frame_id.c_str());
  return 0;
}

int main(int argc, char ** argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cpp_lookup_client");
  auto client = rclcpp_action::create_client<Lookup>(node, "lookup");
  if (!client->wait_for_action_server(std::chrono::seconds(10))) {
    std::printf("C++: アクションサーバーが見つかりません\n");
    return 1;
  }
  int e = run(node, client, "base", false);
  e |= run(node, client, "slow", true);
  rclcpp::shutdown();
  return e;
}
