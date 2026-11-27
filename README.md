# rclsuru

する言語(suru)で ROS 2 のノードを書くためのライブラリです。トピック・サービス・アクション・パラメータ・ログ・ROS の時刻が使え、rclcpp・rclpy のノードや `ros2` の道具とそのまま話せます。計画と進み具合は [#1](https://github.com/suru-lang/rclsuru/issues/1) にあります。

```
「rclsuru」 を 読み込んで 使う

始めする とは 以下の入出力手順です
  n とは 「listener」 を ノード作成した 結果
  文字列型 を 「chatter」 から n で 聞いたこと表示すること に 購読する
  n を スピンする
  以上です

文 を 聞いたこと表示する とは （「聞きました: 」 + 文） を 表示する

始めする
```

## 入れ方

1. **ROS 2 Jazzy。** Ubuntu 24.04 に [ROS 2 の手順](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)で `ros-jazzy-ros-base`(か `ros-jazzy-desktop`)を入れます。使うときは毎回 `source /opt/ros/jazzy/setup.bash` します。
2. **suru 0.10.0 以上と clang。** `kumu`・`suruc`・`suru` が要ります。入れ方は [suru の README](https://github.com/suru-lang/suru#入れ方) にあります(Homebrew なら `brew install suru-lang/suru/suru`、バイナリなら GitHub Releases)。clang は `sudo apt install clang` で入ります。
3. **rclsuru。** 自分のプロジェクトで `kumu 追加 github.com/suru-lang/rclsuru` とします(次の「はじめてのノード」)。

`docker/Dockerfile` に、ROS 2 Jazzy・suru・clang を全部入れた環境があります。試験には、ほかに colcon と g++(C++ の試験用ノード)が要ります。

## はじめてのノード

`chatter` に1秒ごとに「こんにちは」を送るノードを作ります。

```sh
source /opt/ros/jazzy/setup.bash
kumu 新規 hello
cd hello
kumu 追加 github.com/suru-lang/rclsuru
```

`本体.suru` を次のように書きます。

```
「rclsuru」 を 読み込んで 使う

始めする とは 以下の入出力手順です
  n とは 「hello」 を ノード作成した 結果
  p とは 文字列型 を 「chatter」 へ n で 発行者作成した 結果
  （「こんにちは」 を p へ 発行する） を 1.0 で n に 周期登録する
  「始めました」 を n で 情報記録する
  n を スピンする
  以上です

始めする
```

`kumu 実行` で動きます(`kumu 作る` なら `出力/hello` の実行ファイルができます)。別の端末で聞くと届いています。

```sh
ros2 topic echo chatter std_msgs/msg/String   # data: こんにちは
ros2 run demo_nodes_cpp listener              # C++ の listener でも聞ける
```

ノードの名前は英数字と `_` だけです(ROS の決まり)。Ctrl-C で後始末して終わります。ほかの例は `examples/` にあります。

| 例 | 中身 |
|---|---|
| `examples/talker`・`examples/listener` | demo_nodes_cpp の talker・listener と同じ `chatter` |
| `examples/亀` | turtlesim の亀を Twist で動かし、Pose を聞く |
| `examples/足し算サーバー`・`examples/足し算クライアント` | example_interfaces/srv/AddTwoInts のサービス |
| `examples/フィボナッチサーバー`・`examples/フィボナッチクライアント` | example_interfaces/action/Fibonacci のアクション |

## 使い方

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
| `型 を 名前 で n に 関数 へ サービス作成する` | サービスのサーバー。要求ごとに `要求 を 関数する`(応答の値を返す動作)を行い、その応答を返す |
| `型 を 名前 で n に クライアント作成する` | サービスのクライアント |
| `c を 秒 で サービス待機する` | サーバーが出るまで 秒 まで待つ。出れば 真 |
| `要求 を c へ 秒 で 呼出する` | 要求を送り、応答を 秒 まで待つ。結果は `応答 の 有` か `無`。待つ間もほかの購読・タイマーは動く |
| `要求 を c へ 関数 で 要求送信する` | 要求を送るだけ。応答が届いたら(スピンする の中で)`応答 を 関数する` を行う |
| `文字列型` | `std_msgs/msg/String`。する の値は文字列そのもの(`型/std_msgs` の `String型` は `data` の組) |

### メッセージの型

よく使うパッケージの型を `型/` に同梱しています。`「rclsuru/型/パッケージ」` を読み込んで使います。

```
「rclsuru」 を 読み込んで 使う
「rclsuru/型/geometry_msgs」 を 読み込んで 使う

始めする とは 以下の入出力手順です
  n とは 「turtle_driver」 を ノード作成した 結果
  p とは Twist型 を 「turtle1/cmd_vel」 へ n で 発行者作成した 結果
  t とは linear は （x は 2.0 の Vector3既定） で angular は （z は 1.0 の Vector3既定） の Twist既定
  （t を p へ 発行する） を 0.5 で n に 周期登録する
  n を スピンする
  以上です

始めする
```

同梱したパッケージ: builtin_interfaces・std_msgs・example_interfaces・geometry_msgs・sensor_msgs・nav_msgs・std_srvs・shape_msgs・trajectory_msgs・visualization_msgs・diagnostic_msgs・stereo_msgs・tf2_msgs・rosgraph_msgs・rcl_interfaces・action_msgs・unique_identifier_msgs・service_msgs

1つのメッセージ(例: `geometry_msgs/msg/Twist`)から、次のものができます。

| 名前 | 意味 |
|---|---|
| `Twist` | 組の型。フィールドの名前は .msg のままで、型も書いてあります(`linear は Vector3 で angular は Vector3 の 組`)。違う型の値を入れると型の検査でエラー |
| `Twist既定` | 既定の値。数は 0、文字列は「」、配列は 空、.msg に既定の値があればその値。`x は 1.5 の Vector3既定` のように一部だけ変えて使う |
| `Twist型` | `発行者作成する`・`購読する` に渡す メッセージ型 |
| `GoalStatus_STATUS_ACCEPTED` | .msg の定数(`メッセージ名_定数名`) |

.srv(例: `std_srvs/srv/SetBool`)からは `SetBool_Request`・`SetBool_Response` の2つのメッセージと `SetBool型`(サービス型。`サービス作成する`・`クライアント作成する` に渡す)ができます。.action(例: `example_interfaces/action/Fibonacci`)からは `Fibonacci_Goal`・`Fibonacci_Result`・`Fibonacci_Feedback` と `Fibonacci型`(アクション型。`アクションサーバー作成する`・`アクションクライアント作成する` に渡す)ができます(ROS 2 が中で使う `Fibonacci_SendGoal` などもできます)。ほかのパッケージの型は `std_msgs内のHeader既定` のように「パッケージ内の」を付けて書きます(`読み込んで 使う` にしたパッケージは付けなくてよい)。2つのパッケージに同じ名前の型があるとき(turtlesim と geometry_msgs の `Pose` など)は、片方を `読み込む` にします。

| ROS の型 | する の値 |
|---|---|
| `bool` | 真偽 |
| `int8`〜`int64`・`uint8`〜`uint64`・`byte`・`char` | 数(64 ビットの数は ±2^62 まで) |
| `float32`・`float64` | 小数 |
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

### サービス

```
「rclsuru」 を 読み込んで 使う
「rclsuru/型/example_interfaces」 を 読み込んで 使う

始めする とは 以下の入出力手順です
  n とは 「add_two_ints_server」 を ノード作成した 結果
  AddTwoInts型 を 「add_two_ints」 で n に 足し算すること へ サービス作成する
  n を スピンする
  以上です

要求 を 足し算する とは （sum は （要求 の a） + （要求 の b） の AddTwoInts_Response既定） を 結果にする

始めする
```

クライアントは `examples/足し算クライアント` にあります。`QoS付きサービス作成する`・`QoS付きクライアント作成する` で QoS も決められます(既定は `サービスQoS`)。

rcl はサービスの要求と応答を直列化したまま渡せないので、rclsuru は rosidl_generator_c の `…__create` で C の構造体を作り、`rmw_serialize`・`rmw_deserialize` で CDR と行き来します。そのため、使う型のパッケージの `lib<パッケージ>__rosidl_generator_c.so` も要ります(ふつうは入っています)。

### ログ

```
「始めました」 を n で 情報記録する
```

`詳細記録する`(DEBUG)・`情報記録する`(INFO)・`警告記録する`(WARN)・`エラー記録する`(ERROR)・`致命記録する`(FATAL)の5つです。ノードの名前のロガーで出すので、rclcpp のノードと同じく、コンソール・`/rosout`(rqt_console)・ログファイルに出ます。`--ros-args --log-level debug` や `--log-level ノード名:=debug` も効きます。ログに付く場所(ファイル・関数・行)は空です(する の呼んだ場所は分かりません)。

### 時刻

| 関数 | 中身 |
|---|---|
| `n を ROS時刻取得する` | いまの ROS の時刻(秒。小数) |
| `n を 時刻印作成する` | いまの ROS の時刻の `builtin_interfaces/Time`(`header.stamp` に入れる) |

ノードはパラメータ `use_sim_time` を宣言します。起動時に 真 なら(`--ros-args -p use_sim_time:=true`、launch の `use_sim_time`)、`/clock` を購読して、その時刻が ROS の時刻になります(Gazebo や `ros2 bag play --clock`)。`周期登録する` のタイマーも ROS の時刻で動きます。起動した後で `use_sim_time` を変えても時刻の元は変わりません。

### アクション

```
Fibonacci型 を 「fibonacci」 で n に 計算すること へ アクションサーバー作成する

Fibonacci_Goal を 目標扱い で 計算する は Fibonacci_Result を 返す 入出力動作
目標 を 扱い で 計算する とは 以下の入出力手順です
  …
  （sequence は 途中の列 の Fibonacci_Feedback既定） を 扱い へ 途中送信する
  …
  （sequence は 列 の Fibonacci_Result既定） を 結果にする
  以上です
```

目標が届くたびに、別のスレッドで `目標 を 扱い で 関数する` を行い、その結果がアクションの結果になります。例は `examples/フィボナッチサーバー` と `examples/フィボナッチクライアント` です。

| 関数 | 中身 |
|---|---|
| `型 を 名前 で n に 関数 へ アクションサーバー作成する` | アクションサーバー。目標はみな受け付ける |
| `型 を 名前 で n に 関数 へ 判断 より 判断付きアクションサーバー作成する` | `目標 を 判断する` が 真 の目標だけ受け付ける |
| `途中 を 扱い へ 途中送信する` | feedback を送る |
| `扱い を 取消確認する` | 取り消しを頼まれていれば 真。早めに終えて `扱い を 取消済みにする` |
| `扱い を 取消済みにする`・`扱い を 中止にする` | 結果の状態を CANCELED・ABORTED にする(しなければ SUCCEEDED) |
| `型 を 名前 で n に アクションクライアント作成する` | アクションクライアント |
| `ac を 秒 で アクションサーバー待機する` | サーバーが出るまで 秒 まで待つ。出れば 真 |
| `目標 を ac へ 途中関数 で 秒 に 目標呼出する` | 目標を送り、結果を 秒 まで待つ。feedback ごとに `途中 を 途中関数する`。結果は `アクション結果 の 有`(受け付けられないか、待ちきれなければ `無`) |
| `目標 を ac へ 途中関数 で 結果関数 に 目標送信する` | 送るだけ。結果が届いたら(スピンする の中で)`アクション結果 を 結果関数する`。結果は目標の UUID |
| `u を ac で 目標取消する` | 取り消しを頼む。結果は CancelGoal の return_code(0 なら受け付けた) |

`アクション結果` は `終状態`(action_msgs/GoalStatus の 4 成功・5 取り消し・6 中止。受け付けられなかったときは 0)と `結果値` の組です。

アクションは、`名前/_action/` の下の3つのサービス(send_goal・cancel_goal・get_result)と2つのトピック(feedback・status)でできています。rclsuru は rcl_action を使わず、これを rclsuru のサービスと発行者の上に する で書いています。feedback・状態の発行と結果の返信は、実行のスレッドからスピンする のスレッドに頼んで行います。取り消しの要求は、まだ終わっていない目標ならみな受け付けます。終わった目標の結果は 900 秒(rcl_action の既定)覚えています。

### パラメータ

`--ros-args -p 名前:=値` と `--params-file 〜.yaml` の値は、ノードを作ったときに読みます。パラメータは宣言してから使います。宣言の結果は、起動時に値が渡されていればその値、なければ既定の値です。

```
rate とは n から 「rate」 を 1.0 で 小数パラメータ宣言した 結果
名 とは n から 「name」 を 「suru」 で 文字列パラメータ宣言した 結果
count とは n から 「count」 を 整数パラメータ取得した 結果      # いまの値(ros2 param set で変わる)
```

| 関数 | 中身 |
|---|---|
| `n から 名前 を 既定 で 〜パラメータ宣言する` | 〜は 真偽・整数・小数・文字列・バイト列・真偽列・整数列・小数列・文字列列。2回宣言すると失敗する |
| `n から 名前 を 〜パラメータ取得する` | いまの値 |
| `n から 名前 を 値 で 説明 に パラメータ値宣言する` | `rcl_interfaces/msg/ParameterValue` のまま宣言する(説明 は describe に出る) |
| `n へ 関数 で パラメータ変更検査登録する` | 値が変わる前に `変更のリスト を 関数する` を行う。結果が 「」 なら受け入れ、ほかの文字列ならその理由で断る |

変更は `変更名` と `変更値`(ParameterValue)の組です。小数のパラメータに整数(`rate:=5`)を渡したときは小数にします。それ以外の型の違う値は断ります。

ノードは rclcpp と同じ6つのサービス(`~/list_parameters`・`~/get_parameters`・`~/get_parameter_types`・`~/describe_parameters`・`~/set_parameters`・`~/set_parameters_atomically`)に答え、宣言と変更を `/parameter_events` に知らせます。そのため `ros2 param list・get・set・describe・dump` がそのまま使えます。`set_parameters` は1つずつ、`set_parameters_atomically` は全部まとめて受け入れるか断ります。

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

自分の ament のパッケージに入れるときは、`ros/rclsuru_examples/CMakeLists.txt` と同じように、CMake から `kumu 作る` を呼んで、できた実行ファイルを `lib/${PROJECT_NAME}` に置きます。

```cmake
find_program(KUMU kumu REQUIRED)
add_custom_target(hello ALL
  COMMAND ${KUMU} 作る
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/hello
  VERBATIM)
install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/hello/出力/hello DESTINATION lib/${PROJECT_NAME})
```

する のプロジェクトをいくつも作るときは、`add_dependencies` で1つずつ作ります(同じ rclsuru の C の包みを同時に作らないため)。

## rclcpp・rclpy との対応

| rclcpp | rclpy | する(rclsuru) |
|---|---|---|
| `rclcpp::Node::make_shared("talker")` | `Node("talker")` | `「talker」 を ノード作成する` |
| `create_publisher<String>("chatter", 10)` | `create_publisher(String, "chatter", 10)` | `文字列型 を 「chatter」 へ n で 発行者作成する` |
| `pub->publish(msg)` | `pub.publish(msg)` | `値 を p へ 発行する` |
| `create_subscription<T>("t", 10, cb)` | `create_subscription(T, "t", cb, 10)` | `T型 を 「t」 から n で 関数 に 購読する` |
| QoS を渡す(`rclcpp::SensorDataQoS()` など) | `qos_profile_sensor_data` など | `… に センサーQoS より QoS付き購読する` |
| `create_wall_timer(1s, cb)` | `create_timer(1.0, cb)` | `動作 を 1.0 で n に 周期登録する`(ROS の時刻) |
| `rclcpp::spin(node)` | `rclpy.spin(node)` | `n を スピンする` |
| `create_service<T>("s", cb)` | `create_service(T, "s", cb)` | `T型 を 「s」 で n に 関数 へ サービス作成する` |
| `create_client<T>("s")`・`wait_for_service` | `create_client(T, "s")`・`wait_for_service` | `T型 を 「s」 で n に クライアント作成する`・`c を 秒 で サービス待機する` |
| `async_send_request` | `call_async` | `要求 を c へ 関数 で 要求送信する`(待つなら `要求 を c へ 秒 で 呼出する`) |
| `rclcpp_action::create_server<T>` | `ActionServer(node, T, "a", cb)` | `T型 を 「a」 で n に 関数 へ アクションサーバー作成する` |
| `goal_handle->publish_feedback(f)` | `goal_handle.publish_feedback(f)` | `f を 扱い へ 途中送信する` |
| `goal_handle->is_canceling()` | `goal_handle.is_cancel_requested` | `扱い を 取消確認する` |
| `rclcpp_action::create_client<T>` | `ActionClient(node, T, "a")` | `T型 を 「a」 で n に アクションクライアント作成する` |
| `async_send_goal` | `send_goal_async` | `目標 を ac へ 途中関数 で 結果関数 に 目標送信する`(待つなら `目標呼出する`) |
| `declare_parameter("rate", 1.0)` | `declare_parameter("rate", 1.0)` | `n から 「rate」 を 1.0 で 小数パラメータ宣言する` |
| `get_parameter("rate")` | `get_parameter("rate").value` | `n から 「rate」 を 小数パラメータ取得する` |
| `add_on_set_parameters_callback` | `add_on_set_parameters_callback` | `n へ 関数 で パラメータ変更検査登録する` |
| `RCLCPP_INFO(get_logger(), "…")` | `get_logger().info("…")` | `「…」 を n で 情報記録する` |
| `now()` | `get_clock().now()` | `n を 時刻印作成する`(Time)・`n を ROS時刻取得する`(秒) |
| `msg.data = "…"` / `String()` | `String(data="…")` | `data は 「…」 の String既定`(組の値。一部だけ変えて作る) |

## 作り

- **C の包みは小さく。** suru の外部の関数は構造体を値で受け渡せないので、値で受け渡す rcl の関数(`rcl_get_zero_initialized_*`・`*_get_default_options` など)だけを `c/rclsuru.c` で包み、番地にして渡します。番地を受け取る rcl の関数(`rcl_init`・`rcl_node_init`・`rcl_publish_serialized_message` など)は する から直接呼びます。C の関数は `rclsuru.suru` の頭で、`以下は外部の入出力動作です` から `以上です` までに `番地 を 低水準節点破棄する は 「rcl_node_fini」 で 整数 を 返します` のように1行に1つずつ書いています。番地で受け取った C の関数(type support の関数など)も、`p を f で 破棄呼出する とは （f） で 番地 を 番地 で 受け取って 無 を 返します` のように同じまとまりに書きます(suru 0.10.0)。
- **する の書き方。** 入出力をする関数は `… は 発行者 を 返す 入出力動作` と宣言し、本体を `以下の入出力手順です` から `以上です` までの文で書きます。
- **メッセージは直列化した CDR。** `rcl_publish_serialized_message`・`rcl_take_serialized_message` でバイト列をやりとりし、CDR の組み立てと読み取りは する で書きます(`CDR.suru`)。生成した型ごとに、書くのに要る大きさを数える関数・C のメモリに書く関数・読む関数ができ、rmw の領域に直接書いて読みます。型の情報(type support)は、型の名前(`std_msgs/msg/String`)から「動的リンク」で `lib<パッケージ>__rosidl_typesupport_c.so` を開いて引きます。型ごとの C は要りません。
- **待ち方は通路。** wait set は使わず、`rcl_subscription_set_on_new_message_callback` に `通路送り番地作成する` の番地を渡し、DDS のスレッドから届いた知らせを する の通路で受け取ります(rclcpp の EventsExecutor と同じ考え)。番地は1つだけ作り、購読者ごとの番号を user_data に入れて見分けます。タイマーと Ctrl-C も、する のスレッドから同じ番地を呼んで同じ通路に知らせます。

## 試験

```sh
source /opt/ros/jazzy/setup.bash
kumu 試験                       # する で書いた部分の試験(ソースのファイルの中の試験のまとまりと例)
kumu 点検
python3 test/cdr_roundtrip.py   # 同梱した型の CDR を rclpy と往復させる
SURU=suru test/interop.sh       # C++(rclcpp)・ros2 topic・ros2 launch・QoS・インタプリタとの相互運用
SURU=suru test/turtle.sh        # examples/亀 と turtlesim(turtlesim と xvfb-run が要る)
SURU=suru test/add_two_ints.sh  # examples/足し算サーバー・足し算クライアント(example_interfaces と demo_nodes_cpp が要る)
test/fibonacci.sh               # examples/フィボナッチサーバー・フィボナッチクライアント(example_interfaces が要る)
```

`kumu 試験` は、`rclsuru.suru`・`CDR.suru`・`生成器.suru` の中の `以下は「…」の試験です … 以上です` と `例えば … は … です` を動かします(suru 0.10.0 の同じファイルの試験と例)。`python3 test/cdr_roundtrip.py` は、同梱した型を全部(203 個)、乱数で作った値で rclpy と往復させ、同じ値に戻ることを確かめます。`test/interop.sh` は colcon で `ros/rclsuru_examples` と `test/interop_cpp`(C++ の talker と listener)を作り、両方向で届くこと、`--ros-args` の付け替え、`ros2 launch` と Ctrl-C での終わり、Twist・JointState・LaserScan のやりとり、QoS(sensor_data・transient_local)、サービス、パラメータ(`-p`・YAML・`ros2 param`)、アクション(`ros2 action send_goal`・C++ の rclcpp_action と両方向。型は tf2_msgs/action/LookupTransform)を確かめます。GitHub Actions(`.github/workflows/ci.yml`)でも同じものを動かします。

## まだできないこと

- 起動した後で `use_sim_time` を変えても、時刻の元は変わりません
- アクションの取り消しを断る関数は登録できません(まだ終わっていない目標の取り消しはみな受け付けます)
- パラメータの範囲(`integer_range`・`floating_point_range`)は確かめません
- `wstring` を使う型は作りません
- `スピンする` は1つのスレッドで回します。1つの発行者に、いくつものスレッドから同時に `発行する` しないでください(直列化の領域を共有しています)

## ライセンス

MIT
