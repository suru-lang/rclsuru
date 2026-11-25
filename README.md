# rclsuru

する言語(suru)で ROS 2 のノードを書くためのライブラリです。今は段階2(メッセージの型と QoS)まで動きます。計画は [#1](https://github.com/suru-lang/rclsuru/issues/1) にあります。

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
| `名前 を ノード作成する` | ノードを作る動作。名前は英数字と `_` だけ(ROS の決まり)。コマンドラインの `--ros-args`(`-r __node:=…`・`-r __ns:=…`・`-r chatter:=…` など)も読む |
| `型 を 話題 へ n で 発行者作成する` | 発行者(publisher)。QoS は既定(深さ 10) |
| `値 を p へ 発行する` | publish |
| `型 を 話題 から n で 関数 に 購読する` | 購読(subscription)。届いた値ごとに `値 を 関数する` を行う |
| `動作 を 秒 で n に 周期登録する` | 秒ごとに動作を行う(タイマー) |
| `n を スピンする` | 届いたものとタイマーを処理し続ける。Ctrl-C(INT・TERM)で後始末して終わる |
| `n を ノード終了する` | 後始末(発行者・購読者・ノード・context を片付ける) |
| `型 を 話題 へ n で q より QoS付き発行者作成する` | QoS を決めた発行者 |
| `型 を 話題 から n で 関数 に q より QoS付き購読する` | QoS を決めた購読 |
| `文字列型` | `std_msgs/msg/String`。する の値は文字列そのもの(`型/std_msgs` の `String型` は `data` の組) |

### メッセージの型

よく使うパッケージの型を `型/` に同梱しています。`「rclsuru/型/パッケージ」` を読み込んで使います。

```
「rclsuru」 を 読み込んで 使う
「rclsuru/型/geometry_msgs」 を 読み込んで 使う

始めする とは 手順
  n とは 「turtle_driver」 を ノード作成した 結果
  p とは Twist型 を 「turtle1/cmd_vel」 へ n で 発行者作成した 結果
  t とは linear は （x は 2.0 の Vector3既定） で angular は （z は 1.0 の Vector3既定） の Twist既定
  （t を p へ 発行する） を 0.5 で n に 周期登録する
  n を スピンする

始めする
```

同梱したパッケージ: builtin_interfaces・std_msgs・geometry_msgs・sensor_msgs・nav_msgs・std_srvs・shape_msgs・trajectory_msgs・visualization_msgs・diagnostic_msgs・stereo_msgs・tf2_msgs・rosgraph_msgs・rcl_interfaces・action_msgs・unique_identifier_msgs・service_msgs

1つのメッセージ(例: `geometry_msgs/msg/Twist`)から、次のものができます。

| 名前 | 意味 |
|---|---|
| `Twist` | 組の型。フィールドの名前は .msg のまま(`linear と angular の 組`) |
| `Twist既定` | 既定の値。数は 0、文字列は「」、配列は 空、.msg に既定の値があればその値。`x は 1.5 の Vector3既定` のように一部だけ変えて使う |
| `Twist型` | `発行者作成する`・`購読する` に渡す メッセージ型 |
| `GoalStatus_STATUS_ACCEPTED` | .msg の定数(`メッセージ名_定数名`) |

.srv(例: `std_srvs/srv/SetBool`)からは `SetBool_Request`・`SetBool_Response` の2つのメッセージと `SetBool型`(サービス型。段階3 で使います)ができます。ほかのパッケージの型は `std_msgs内のHeader既定` のように「パッケージ内の」を付けて書きます(`読み込んで 使う` にしたパッケージは付けなくてよい)。2つのパッケージに同じ名前の型があるとき(turtlesim と geometry_msgs の `Pose` など)は、片方を `読み込む` にします。

| ROS の型 | する の値 |
|---|---|
| `bool` | 真偽 |
| `int8`〜`int64`・`uint8`〜`uint64`・`byte`・`char` | 数(64 ビットの数は ±2^62 まで) |
| `float32`・`float64` | 数(小数) |
| `string`・`string<=N` | 文字列(UTF-8) |
| `uint8[]`・`byte[]`・`char[]`(固定長・上限つきも) | バイト列(`sensor_msgs/Image` の `data` など) |
| ほかの `T[]`・`T[N]`・`T[<=N]` | T の リスト。固定長は長さが違うと、上限つきは超えると発行するときにエラー |
| `wstring` | まだ使えません(その型は作りません) |

#### 自分のパッケージの型を作る

`生成器.suru` で、自分の ROS 2 のパッケージ(.msg・.srv)から する の型のファイルを作れます。パッケージを colcon で作って `install/setup.bash` を読んだ後に、

```sh
suru <rclsuru のフォルダ>/生成器.suru 型 my_msgs      # 型/my_msgs.suru を書く
```

と実行し、`「型/my_msgs」 を 読み込む` で使います。`AMENT_PREFIX_PATH` からパッケージを探します。同梱したパッケージの型(std_msgs など)を使う型は、`「rclsuru/型/std_msgs」` を読み込むファイルになります。例は `examples/亀`(turtlesim の `turtlesim/msg/Pose`)です。

メッセージを CDR のバイト列にしたいとき(ファイルに残すなど)は、`「rclsuru/CDR」 を 読み込む` で `x を 型 で CDR内のメッセージ直列化する` と `b を 型 で CDR内のメッセージ復元する` が使えます。

### QoS

`QoS付き発行者作成する`・`QoS付き購読する` に QoS を渡します。決まったプロファイルの `既定QoS`(深さ 10、reliable、volatile)・`センサーQoS`(sensor_data。best effort)・`サービスQoS`・`パラメータQoS`・`パラメータ事象QoS`・`システム既定QoS` から選び、変えたいところだけ変えます。

```
保持QoS とは 深さ は 1 で 永続性 は 「transient_local」 の 既定QoS     # /tf_static のような使い方
LaserScan型 を 「scan」 から n で 表示すること に センサーQoS より QoS付き購読する
```

| フィールド | 書けるもの(「」 ならプロファイルのまま) |
|---|---|
| `履歴` | 「keep_last」・「keep_all」・「system_default」 |
| `深さ` | 数(0 ならプロファイルのまま) |
| `信頼性` | 「reliable」・「best_effort」・「system_default」 |
| `永続性` | 「transient_local」・「volatile」・「system_default」 |

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
- **メッセージは直列化した CDR。** `rcl_publish_serialized_message`・`rcl_take_serialized_message` でバイト列をやりとりし、CDR の組み立てと読み取りは する で書きます(`CDR.suru`)。生成した型ごとに、書くのに要る大きさを数える関数・C のメモリに書く関数・読む関数ができ、rmw の領域に直接書いて読みます。型の情報(type support)は、型の名前(`std_msgs/msg/String`)から「動的リンク」で `lib<パッケージ>__rosidl_typesupport_c.so` を開いて引きます。型ごとの C は要りません。
- **待ち方は通路。** wait set は使わず、`rcl_subscription_set_on_new_message_callback` に `通路送り番地作成する` の番地を渡し、DDS のスレッドから届いた知らせを する の通路で受け取ります(rclcpp の EventsExecutor と同じ考え)。番地は1つだけ作り、購読者ごとの番号を user_data に入れて見分けます。タイマーと Ctrl-C も、する のスレッドから同じ番地を呼んで同じ通路に知らせます。

## 試験

```sh
source /opt/ros/jazzy/setup.bash
kumu 試験                       # する で書いた部分の試験
kumu 点検
python3 test/cdr_roundtrip.py   # 同梱した型の CDR を rclpy と往復させる
SURU=suru test/interop.sh       # C++(rclcpp)・ros2 topic・ros2 launch・QoS・インタプリタとの相互運用
SURU=suru test/turtle.sh        # examples/亀 と turtlesim(turtlesim と xvfb-run が要る)
```

`python3 test/cdr_roundtrip.py` は、同梱した型を全部(195 個)、乱数で作った値で rclpy と往復させ、同じ値に戻ることを確かめます。`test/interop.sh` は colcon で `ros/rclsuru_examples` と `test/interop_cpp`(C++ の talker と listener)を作り、両方向で届くこと、`--ros-args` の付け替え、`ros2 launch` と Ctrl-C での終わり、Twist・JointState・LaserScan のやりとり、QoS(sensor_data・transient_local)を確かめます。GitHub Actions(`.github/workflows/ci.yml`)でも同じものを動かします。

## まだできないこと

- サービス・パラメータ・アクション・ログの関数はまだありません(.action の型も、まだ作りません)
- `wstring` を使う型は作りません
- `スピンする` は1つのスレッドで回します。1つの発行者に、いくつものスレッドから同時に `発行する` しないでください(直列化の領域を共有しています)

## ライセンス

MIT
