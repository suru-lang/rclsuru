// set_flag(std_srvs/SetBool)の C++ のサーバー。success に data を入れて返す
#include <cstdio>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/set_bool.hpp"

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cpp_set_bool_server");
  auto srv = node->create_service<std_srvs::srv::SetBool>(
    "set_flag", [](const std::shared_ptr<std_srvs::srv::SetBool::Request> req,
                   std::shared_ptr<std_srvs::srv::SetBool::Response> res) {
      std::printf("C++ が要求を受けました: data=%d\n", req->data ? 1 : 0);
      std::fflush(stdout);
      res->success = req->data;
      res->message = "C++ のサーバーです";
    });
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
