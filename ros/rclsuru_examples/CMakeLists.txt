cmake_minimum_required(VERSION 3.8)
project(rclsuru_examples NONE)

find_package(ament_cmake REQUIRED)

# kumu(する のプロジェクト管理)で する のノードを作り、lib/<パッケージ>/ に置く。
# ros2 run rclsuru_examples talker・ros2 launch で起動できる。
find_program(KUMU kumu REQUIRED)
get_filename_component(PACKAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR} REALPATH)
get_filename_component(RCLSURU_EXAMPLES_DIR ${PACKAGE_DIR}/../../examples ABSOLUTE)

foreach(node talker listener)
  add_custom_target(${node} ALL
    COMMAND ${KUMU} 作る
    WORKING_DIRECTORY ${RCLSURU_EXAMPLES_DIR}/${node}
    COMMENT "kumu で ${node} を作ります"
    VERBATIM)
  install(PROGRAMS ${RCLSURU_EXAMPLES_DIR}/${node}/出力/${node}
    DESTINATION lib/${PROJECT_NAME})
endforeach()

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
