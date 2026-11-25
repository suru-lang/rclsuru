#!/bin/bash
# rclsuru の相互運用の試験。ROS 2 の環境(source /opt/ros/$ROS_DISTRO/setup.bash 済み)で動かす。
#   test/interop.sh            … colcon で作って、試験を全部動かす
# 要るもの: kumu(PATH にあること)、clang、colcon。インタプリタの試験は SURU が suru のコマンドのとき。

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
ws=${RCLSURU_TEST_WS:-/tmp/rclsuru_ws}
export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-42}
export RCUTILS_COLORIZED_OUTPUT=0
logs=$(mktemp -d)
failed=0

rm -rf "$ws" && mkdir -p "$ws/src"
cp -r "$root" "$ws/src/rclsuru"
rm -rf "$ws/src/rclsuru/.git"
(cd "$ws" && colcon build --event-handlers console_cohesion+ > "$logs/build.log" 2>&1) || {
  cat "$logs/build.log"; echo "作るのに失敗しました"; exit 1; }
# shellcheck disable=SC1091
source "$ws/install/setup.bash"

check() {  # check 名前 ログ 含むはずの文
  if grep -q -- "$3" "$2"; then
    echo "合格: $1"
  else
    echo "不合格: $1(「$3」がありません)"; sed 's/^/    /' "$2"; failed=1
  fi
}

# 送る側と受け取る側を動かし、受け取る側の出力を返す
pair() {  # pair 名前 送る命令 受け取る命令
  local out="$logs/$1.log"
  (timeout -s INT 10 bash -c "$3" > "$out" 2>&1 &)
  sleep 1
  timeout -s INT 7 bash -c "$2" > "$logs/$1.sender.log" 2>&1
  sleep 3
  echo "$out"
}

out=$(pair suru_to_cpp "ros2 run rclsuru_examples talker" "ros2 run rclsuru_interop_cpp cpp_listener")
check "する の talker → C++ の listener" "$out" "C++ が聞きました: こんにちは する"

out=$(pair cpp_to_suru "ros2 run rclsuru_interop_cpp cpp_talker" "stdbuf -o0 ros2 run rclsuru_examples listener")
check "C++ の talker → する の listener" "$out" "聞きました: C++ から"

out=$(pair suru_to_suru "ros2 run rclsuru_examples talker" "stdbuf -o0 ros2 run rclsuru_examples listener")
check "する の talker → する の listener" "$out" "聞きました: こんにちは する"

out=$(pair suru_to_py "ros2 run rclsuru_examples talker" "ros2 topic echo chatter std_msgs/msg/String")
check "する の talker → ros2 topic echo" "$out" "data: こんにちは する"

out=$(pair py_to_suru "ros2 topic pub -r 5 chatter std_msgs/msg/String '{data: パイソン}'" "stdbuf -o0 ros2 run rclsuru_examples listener")
check "ros2 topic pub → する の listener" "$out" "聞きました: パイソン"

# 名前と名前空間と話題の付け替え(--ros-args)
log="$logs/remap.log"
(timeout -s INT 15 ros2 run rclsuru_examples talker --ros-args -r __node:=alias -r __ns:=/ns -r chatter:=other > /dev/null 2>&1 &)
# 見つかるまでの時間は機械によって違う(ros2 のデーモンの始まりと DDS の発見)ので、10 秒まで待つ
for _ in $(seq 20); do
  { ros2 node list; ros2 topic list; } > "$log" 2>&1
  grep -q "^/ns/alias$" "$log" && grep -q "^/ns/other$" "$log" && break
  sleep 0.5
done
pkill -INT -f "__node:=alias" || true
sleep 1
check "ノードの名前の付け替え" "$log" "^/ns/alias$"
check "話題の名前の付け替え" "$log" "^/ns/other$"

# ros2 launch と Ctrl-C での終わり
log="$logs/launch.log"
timeout -s INT 6 ros2 launch rclsuru_examples talker_listener.launch.py > "$log" 2>&1
check "ros2 launch で talker と listener" "$log" "聞きました: こんにちは する"
check "Ctrl-C できれいに終わる(talker)" "$log" "talker-1\]: process has finished cleanly"
check "Ctrl-C できれいに終わる(listener)" "$log" "listener-2\]: process has finished cleanly"

# 段階2: メッセージの型と QoS(test/interop_suru と C++ の cpp_msgs)
(cd "$root/test/interop_suru" && kumu 作る > "$logs/interop_suru.log" 2>&1) || {
  cat "$logs/interop_suru.log"; echo "test/interop_suru を作るのに失敗しました"; exit 1; }
suru_node="$root/test/interop_suru/出力/interop_suru"

out=$(pair twist_to_cpp "$suru_node twist" "stdbuf -o0 ros2 run rclsuru_interop_cpp cpp_msgs")
check "する の Twist → C++" "$out" "C++ が Twist を聞きました: linear.x=1.5 angular.z=-0.5"

out=$(pair twist_to_py "$suru_node twist" "ros2 topic echo cmd_vel geometry_msgs/msg/Twist")
check "する の Twist → ros2 topic echo" "$out" "x: 1.5"

out=$(pair joint_from_cpp "ros2 run rclsuru_interop_cpp cpp_msgs" "stdbuf -o0 $suru_node joint")
check "C++ の JointState → する" "$out" "JointState を聞きました: 肩・肘 = 0.5 -1.25 frame=腕"

out=$(pair scan_from_cpp "ros2 run rclsuru_interop_cpp cpp_msgs" "stdbuf -o0 $suru_node scan")
check "C++ の LaserScan(sensor_data)→ する の sensor_data の購読" "$out" "LaserScan を聞きました: 1 2.5 4 angle_max=1.5"

out=$(pair scan_reliable "ros2 run rclsuru_interop_cpp cpp_msgs" "stdbuf -o0 $suru_node scan_reliable")
if grep -q "LaserScan を聞きました" "$out"; then
  echo "不合格: reliable の購読に best effort の送り手から届きました(QoS が効いていません)"; failed=1
else
  echo "合格: reliable の購読には best effort の送り手から届かない"
fi

# transient_local: 送り手が先に1回だけ送り、後から来た購読者にも届く
log="$logs/latched.log"
(timeout -s INT 12 ros2 run rclsuru_interop_cpp cpp_msgs > /dev/null 2>&1 &)
sleep 2
timeout -s INT 6 stdbuf -o0 "$suru_node" latched > "$log" 2>&1
check "C++ の transient_local → 後から来た する の購読" "$log" "String を聞きました: 最初の1回だけ"
pkill -INT -f cpp_msgs || true

log="$logs/latch_pub.log"
(timeout -s INT 20 "$suru_node" latch_pub > /dev/null 2>&1 &)
sleep 2
timeout -s INT 15 ros2 topic echo --once --qos-durability transient_local --qos-reliability reliable --qos-depth 1 /latched_suru std_msgs/msg/String > "$log" 2>&1
check "する の transient_local → 後から来た ros2 topic echo" "$log" "data: する から1回だけ"
pkill -INT -f "interop_suru latch_pub" || true
sleep 1

# 段階3: サービス(std_srvs/SetBool の set_flag)
out=$(pair srv_suru_suru "$suru_node client" "stdbuf -o0 $suru_node server")
check "する のクライアント → する のサーバー(要求)" "$out" "要求を受けました: data=真"
check "する のクライアント → する のサーバー(待たない書き方)" "$out" "要求を受けました: data=偽"
out=$(pair srv_suru_suru_res "stdbuf -o0 $suru_node server" "stdbuf -o0 $suru_node client")
check "する のサーバー → する のクライアント(待つ書き方の応答)" "$out" "応答: success=偽 message=する のサーバーです"
check "する のサーバー → する のクライアント(待たない書き方の応答)" "$out" "応答: success=真 message=する のサーバーです"

out=$(pair srv_cpp_client "stdbuf -o0 $suru_node server" "ros2 run rclsuru_interop_cpp cpp_set_bool_client")
check "C++ のクライアント → する のサーバー" "$out" "C++ が応答を受けました: success=0 message=する のサーバーです"

out=$(pair srv_cpp_server "ros2 run rclsuru_interop_cpp cpp_set_bool_server" "stdbuf -o0 $suru_node client")
check "する のクライアント → C++ のサーバー" "$out" "応答: success=真 message=C++ のサーバーです"

log="$logs/srv_cli.log"
(timeout -s INT 20 "$suru_node" server > /dev/null 2>&1 &)
sleep 1
timeout 15 ros2 service call /set_flag std_srvs/srv/SetBool "{data: true}" > "$log" 2>&1
check "ros2 service call → する のサーバー" "$log" "success=False, message='する のサーバーです'"
pkill -INT -f "interop_suru server" || true
sleep 1

# インタプリタ
if [ -n "${SURU:-}" ]; then
  out=$(pair interp_listener "ros2 run rclsuru_interop_cpp cpp_talker" "cd $root/examples/listener && stdbuf -o0 $SURU 本体.suru")
  check "インタプリタの listener ← C++ の talker" "$out" "聞きました: C++ から"
  out=$(pair interp_talker "cd $root/examples/talker && $SURU 本体.suru" "ros2 run rclsuru_interop_cpp cpp_listener")
  check "インタプリタの talker → C++ の listener" "$out" "C++ が聞きました: こんにちは する"
fi

rm -rf "$logs"
if [ "$failed" = 0 ]; then echo "全部合格"; else echo "不合格があります"; exit 1; fi
