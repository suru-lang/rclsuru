#!/usr/bin/env python3
"""試験用: /clock に 1000 秒から始まる時刻を、10 倍の速さで送る(0.01 秒ごとに 0.1 秒進める)。"""
import time

import rclpy
from rclpy.node import Node
from rosgraph_msgs.msg import Clock

rclpy.init()
node = Node("test_clock")
pub = node.create_publisher(Clock, "/clock", 10)
t = 1000.0
while rclpy.ok():
    m = Clock()
    m.clock.sec = int(t)
    m.clock.nanosec = int((t - int(t)) * 1e9)
    pub.publish(m)
    t += 0.1
    time.sleep(0.01)
