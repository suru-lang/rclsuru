#!/bin/bash
# examples/亀 を turtlesim と動かす試験(CI で使う)。ROS 2 の環境と turtlesim と xvfb-run が要る。
#   1. 生成器で turtlesim の型を作りなおし、examples/亀/型/turtlesim.suru と同じことを確かめる
#   2. turtlesim_node を動かし、亀 が /turtle1/cmd_vel で動かして /turtle1/pose を読めることを確かめる
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-43}
tmp=$(mktemp -d)
failed=0

(cd "$root" && ${SURU:-suru} 生成器.suru "$tmp" turtlesim) || exit 1
if diff -u "$root/examples/亀/型/turtlesim.suru" "$tmp/turtlesim.suru"; then
  echo "合格: 生成器で作った turtlesim の型が同じ"
else
  echo "不合格: examples/亀/型/turtlesim.suru が生成器の結果と違います"; failed=1
fi

(cd "$root/examples/亀" && kumu 作る) || exit 1
(timeout -s INT 20 xvfb-run -a ros2 run turtlesim turtlesim_node > "$tmp/sim.log" 2>&1 &)
sleep 4
timeout -s INT 8 stdbuf -o0 "$root/examples/亀/出力/亀" > "$tmp/turtle.log" 2>&1
pkill -INT -f turtlesim_node || true
cat "$tmp/turtle.log"
# 初めは x=5.544…。動いていれば最後の場所はそこから離れている
last=$(grep "亀の場所" "$tmp/turtle.log" | tail -1)
if [ -z "$last" ]; then
  echo "不合格: 亀の場所が届きません"; cat "$tmp/sim.log"; failed=1
elif echo "$last" | grep -q "x=5.5444"; then
  echo "不合格: 亀が動いていません($last)"; failed=1
else
  echo "合格: 亀が動き、場所を読めた($last)"
fi
rm -rf "$tmp"
exit $failed
