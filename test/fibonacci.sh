#!/bin/bash
# examples/フィボナッチサーバー・フィボナッチクライアント の試験(CI で使う)。
# ROS 2 の環境と example_interfaces が要る。
#   1. ros2 action send_goal で する のサーバーに目標を送り、feedback と結果を確かめる
#   2. する のクライアントで する のサーバーに目標を送る
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-46}
tmp=$(mktemp -d)
failed=0

check() {  # check 名前 ログ 含むはずの文
  if grep -q -- "$3" "$2"; then
    echo "合格: $1"
  else
    echo "不合格: $1(「$3」がありません)"; sed 's/^/    /' "$2"; failed=1
  fi
}

(cd "$root/examples/フィボナッチサーバー" && kumu 作る) || exit 1
(cd "$root/examples/フィボナッチクライアント" && kumu 作る) || exit 1
server="$root/examples/フィボナッチサーバー/出力/フィボナッチサーバー"
client="$root/examples/フィボナッチクライアント/出力/フィボナッチクライアント"

(timeout -s INT 40 stdbuf -o0 "$server" > "$tmp/server.log" 2>&1 &)
sleep 2
timeout 20 ros2 action send_goal /fibonacci example_interfaces/action/Fibonacci "{order: 5}" --feedback > "$tmp/cli.log" 2>&1
check "ros2 action send_goal → する のサーバー(feedback)" "$tmp/cli.log" "Feedback:"
check "ros2 action send_goal → する のサーバー(結果)" "$tmp/cli.log" "Goal finished with status: SUCCEEDED"
check "ros2 action send_goal → する のサーバー(数列)" "$tmp/cli.log" "^- 3$"
timeout 20 stdbuf -o0 "$client" 6 > "$tmp/client.log" 2>&1
check "する のクライアント → する のサーバー(feedback)" "$tmp/client.log" "途中: 0 1 1 2"
check "する のクライアント → する のサーバー(結果)" "$tmp/client.log" "結果(状態 4): 0 1 1 2 3 5"
pkill -INT -f "フィボナッチサーバー" || true

rm -rf "$tmp"
exit $failed
