# する の talker と listener を一緒に起動する
#   ros2 launch rclsuru_examples talker_listener.launch.py
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(package='rclsuru_examples', executable='talker', name='suru_talker', output='screen'),
        Node(package='rclsuru_examples', executable='listener', name='suru_listener', output='screen'),
    ])
