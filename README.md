# rclsuru

する言語(suru)で ROS 2 のノードを書くためのライブラリです。今は段階1(talker と listener)まで動きます。計画は [#1](https://github.com/suru-lang/rclsuru/issues/1) にあります。

```
「rclsuru」 を 読み込んで 使う

始めする とは 手順
  n とは 「listener」 を ノード作成した 結果
  文字列型 を 「chatter」 から n で 聞いたこと表示すること に 購読する
  n を スピンする

文 を 聞いたこと表示する とは （「聞きました: 」 + 文） を 表示する

始めする
```

## 要るもの

- ROS 2 Jazzy(Ubuntu 24.04)。`source /opt/ros/jazzy/setup.bash` してから使います
- suru 0.5.0 以上(kumu・suruc・インタプリタの `suru`)と clang
- 試験には colcon と、C++ の試験用ノードを作るための g++

`docker/Dockerfile` に、全部を入れた環境があります。

## 使い方

自分のプロジェクトの依存に足します。

```sh
source /opt/ros/jazzy/setup.bash
kumu 追加 github.com/suru-lang/rclsuru
kumu 実行
```

C の包み(`c/rclsuru.c`)は kumu が一緒にコンパイルします。ROS 2 のヘッダーとライブラリは `/opt/ros/${ROS_DISTRO}` から探すので、`setup.bash` を読まずに作ると「環境変数「ROS_DISTRO」がありません」のエラーになります。インタプリタ(`suru 本体.suru`)でも同じように動きます。

| 書き方 | 意味 |
|---|---|
| `名前 を ノード作成する` | ノードを作る動作。コマンドラインの `--ros-args`(`-r __node:=…`・`-r __ns:=…`・`-r chatter:=…` など)も読む |
| `型 を 話題 へ n で 発行者作成する` | 発行者(publisher)。QoS は既定(深さ 10) |
| `値 を p へ 発行する` | publish |
| `型 を 話題 から n で 関数 に 購読する` | 購読(subscription)。届いた値ごとに `値 を 関数する` を行う |
| `動作 を 秒 で n に 周期登録する` | 秒ごとに動作を行う(タイマー) |
| `n を スピンする` | 届いたものとタイマーを処理し続ける。Ctrl-C(INT・TERM)で後始末して終わる |
| `n を ノード終了する` | 後始末(発行者・購読者・ノード・context を片付ける) |
| `文字列型` | `std_msgs/msg/String`。する の値は文字列 |

例は `examples/talker` と `examples/listener` です(demo_nodes_cpp の talker・listener と同じ `chatter` を使います)。

### ros2 run と ros2 launch

する のノードは `kumu 作る` で普通の実行ファイルになるので、ament のパッケージの `lib/<パッケージ>/` に置けば ROS の道具から起動できます。`ros/rclsuru_examples` がその例で、CMake から `kumu 作る` を呼んで例を置きます。

```sh
mkdir -p ws/src && cd ws/src && git clone https://github.com/suru-lang/rclsuru && cd ..
colcon build
source install/setup.bash
ros2 run rclsuru_examples talker
ros2 launch rclsuru_examples talker_listener.launch.py
```

## 作り

- **C の包みは小さく。** suru の外部の関数は構造体を値で受け渡せないので、値で受け渡す rcl の関数(`rcl_get_zero_initialized_*`・`*_get_default_options` など)だけを `c/rclsuru.c` で包み、番地にして渡します。番地を受け取る rcl の関数(`rcl_init`・`rcl_node_init`・`rcl_publish_serialized_message` など)は する から直接呼びます。
- **メッセージは直列化した CDR。** `rcl_publish_serialized_message`・`rcl_take_serialized_message` でバイト列をやりとりし、CDR の組み立てと読み取りは する で書きます。型の情報(type support)は、型の名前(`std_msgs/msg/String`)から「動的リンク」で `lib<パッケージ>__rosidl_typesupport_c.so` を開いて引きます。型ごとの C は要りません。
- **待ち方は通路。** wait set は使わず、`rcl_subscription_set_on_new_message_callback` に `通路送り番地作成する` の番地を渡し、DDS のスレッドから届いた知らせを する の通路で受け取ります(rclcpp の EventsExecutor と同じ考え)。番地は1つだけ作り、購読者ごとの番号を user_data に入れて見分けます。タイマーと Ctrl-C も、する のスレッドから同じ番地を呼んで同じ通路に知らせます。

## 試験

```sh
source /opt/ros/jazzy/setup.bash
kumu 試験                       # する で書いた部分の試験
kumu 点検
SURU=suru test/interop.sh       # C++(rclcpp)・ros2 topic・ros2 launch・インタプリタとの相互運用
```

`test/interop.sh` は colcon で `ros/rclsuru_examples` と `test/interop_cpp`(C++ の talker と listener)を作り、両方向で届くこと、`--ros-args` の付け替え、`ros2 launch` と Ctrl-C での終わりを確かめます。GitHub Actions(`.github/workflows/ci.yml`)でも同じものを動かします。

## まだできないこと

- 型は `std_msgs/msg/String` だけ(段階2 で CDR と生成器を作ります)
- QoS は既定だけ、サービス・パラメータ・アクション・ログの関数はまだありません
- `スピンする` は1つのスレッドで回します。1つの発行者に、いくつものスレッドから同時に `発行する` しないでください(直列化の領域を共有しています)

## ライセンス

MIT
