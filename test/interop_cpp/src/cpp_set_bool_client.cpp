// set_flag(std_srvs/SetBool)を data=true で呼び、応答を表示する C++ のクライアント
#include <chrono>
#include <cstdio>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/set_bool.hpp"

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cpp_set_bool_client");
  auto client = node->create_client<std_srvs::srv::SetBool>("set_flag");
  if (!client->wait_for_service(std::chrono::seconds(10))) {
    std::printf("C++: サーバーが見つかりません\n");
    return 1;
  }
  auto req = std::make_shared<std_srvs::srv::SetBool::Request>();
  req->data = true;
  auto future = client->async_send_request(req);
  if (rclcpp::spin_until_future_complete(node, future, std::chrono::seconds(5)) !=
      rclcpp::FutureReturnCode::SUCCESS) {
    std::printf("C++: 応答が届きません\n");
    return 1;
  }
  auto res = future.get();
  std::printf("C++ が応答を受けました: success=%d message=%s\n", res->success ? 1 : 0,
              res->message.c_str());
  rclcpp::shutdown();
  return 0;
}
