#!/bin/bash
# examples/足し算サーバー・足し算クライアント の試験(CI で使う)。
# ROS 2 の環境と example_interfaces・demo_nodes_cpp が要る。
#   1. 生成器で example_interfaces の型を作りなおし、型/example_interfaces.suru と同じことを確かめる
#   2. ros2 service call・demo_nodes_cpp の add_two_ints_client・する のクライアントで する のサーバーを呼ぶ
#   3. する のクライアントで demo_nodes_cpp の add_two_ints_server を呼ぶ
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-44}
tmp=$(mktemp -d)
failed=0

check() {  # check 名前 ログ 含むはずの文
  if grep -q -- "$3" "$2"; then
    echo "合格: $1"
  else
    echo "不合格: $1(「$3」がありません)"; sed 's/^/    /' "$2"; failed=1
  fi
}

(cd "$root" && ${SURU:-suru} 生成器.suru "$tmp" example_interfaces) || exit 1
if diff -u "$root/型/example_interfaces.suru" "$tmp/example_interfaces.suru"; then
  echo "合格: 生成器で作った example_interfaces の型が同じ"
else
  echo "不合格: 型/example_interfaces.suru が生成器の結果と違います"; failed=1
fi

(cd "$root/examples/足し算サーバー" && kumu 作る) || exit 1
(cd "$root/examples/足し算クライアント" && kumu 作る) || exit 1
server="$root/examples/足し算サーバー/出力/足し算サーバー"
client="$root/examples/足し算クライアント/出力/足し算クライアント"

(timeout -s INT 40 stdbuf -o0 "$server" > "$tmp/server.log" 2>&1 &)
sleep 2
timeout 15 ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 1, b: 2}" > "$tmp/call.log" 2>&1
check "ros2 service call → する のサーバー" "$tmp/call.log" "sum=3"
timeout 15 ros2 run demo_nodes_cpp add_two_ints_client > "$tmp/cpp_client.log" 2>&1
check "C++ の add_two_ints_client → する のサーバー" "$tmp/cpp_client.log" "Result of add_two_ints: 5"
timeout 15 stdbuf -o0 "$client" 10 20 > "$tmp/client.log" 2>&1
check "する のクライアント → する のサーバー" "$tmp/client.log" "答え: 10 + 20 = 30"
pkill -INT -f "足し算サーバー" || true
sleep 1

(timeout -s INT 20 ros2 run demo_nodes_cpp add_two_ints_server > /dev/null 2>&1 &)
sleep 2
timeout 15 stdbuf -o0 "$client" 7 8 > "$tmp/client_cpp.log" 2>&1
check "する のクライアント → C++ の add_two_ints_server" "$tmp/client_cpp.log" "答え: 7 + 8 = 15"
pkill -INT -f add_two_ints_server || true

rm -rf "$tmp"
exit $failed
