# 「rclsuru」

rclsuru: a library for writing ROS 2 nodes in Suru.
Load it with `「rclsuru」 を 読み込んで 使う`.

| Usage | What it does |
|---|---|
| `名前 を ノード作成する` | Creates a node (also reads `--ros-args` from the command line) |
| `型 を 話題 へ n で 発行者作成する` | Creates a publisher |
| `値 を p へ 発行する` | Publishes a value |
| `型 を 話題 から n で 関数 に 購読する` | Runs 「値 を 関数する」 for each value that arrives |
| `動作 を 秒 で n に 周期登録する` | Runs the action every `秒` seconds |
| `n を スピンする` | Processes messages and timers until Ctrl-C, then cleans up |
| `n を ノード終了する` | Cleans up the node (also called when `スピンする` ends with Ctrl-C) |
| `QoS付き発行者作成する`, `QoS付き購読する` | The same with a QoS (`既定QoS`, `センサーQoS` and so on) |
| `文字列型` | std_msgs/msg/String (a Suru string) |
| `型 を 名前 で n に 関数 へ サービス作成する` | A service server (returns the response of 「要求 を 関数する」 for each request) |
| `型 を 名前 で n に クライアント作成する` | A service client: `c を 秒 で サービス待機する`, `要求 を c へ 秒 で 呼出する` (the response `有` or `無`), `要求 を c へ 関数 で 要求送信する` |
| `型 を 名前 で n に 関数 へ アクションサーバー作成する` | An action server (runs 「目標 を 扱い で 関数する」 in its own thread for each goal) |
| `型 を 名前 で n に アクションクライアント作成する` | An action client: `目標 を ac へ 途中関数 で 秒 に 目標呼出する` and so on |
| `n から 名前 を 既定 で 小数パラメータ宣言する` and so on | Parameters (answers `ros2 param`) |
| `文 を n で 情報記録する` and so on | Logging (console and /rosout) |
| `n を ROS時刻取得する`, `n を 時刻印作成する` | ROS time (/clock with use_sim_time) |

Other message types are loaded from 「rclsuru/型/geometry_msgs」 and so on (made by 生成器.suru).

rcl functions are called through a small C wrapper (c/rclsuru.c). Messages are exchanged as serialized CDR bytes
(read and written by CDR.suru), and the type support is looked up by the type name through dynamic linking.
Instead of an rcl wait set, the address made by `通路送り番地作成する` is given to rcl's "call when something arrives"
functions, and the notices are received on a Suru channel (the same idea as rclcpp's EventsExecutor). Timers and Ctrl-C
call the same address from Suru threads and notify the same channel.

## A first node

Install ROS 2 Jazzy, clang and Suru 0.11.0 or later, run `kumu 追加 github.com/suru-lang/rclsuru` in your project, and write:

```suru
「rclsuru」 を 読み込んで 使う

始めする とは 以下の入出力手順です
  n とは 「listener」 を ノード作成した 結果
  文字列型 を 「chatter」 から n で 聞いたこと表示すること に 購読する
  n を スピンする
  以上です

文 を 聞いたこと表示する とは （「聞きました: 」 + 文） を 表示する

始めする
```

The full guide (installing, messages, services, actions, parameters, QoS, `ros2 run` and `ros2 launch`) is in the
[README](https://github.com/suru-lang/rclsuru#readme) (in Japanese).

## Bundled message types

Load them with 「rclsuru/型/<package>」 (for example `「rclsuru/型/geometry_msgs」 を 読み込んで 使う`):
action_msgs, builtin_interfaces, diagnostic_msgs, example_interfaces, geometry_msgs, nav_msgs, rcl_interfaces,
rosgraph_msgs, sensor_msgs, service_msgs, shape_msgs, std_msgs, std_srvs, stereo_msgs, tf2_msgs, trajectory_msgs,
unique_identifier_msgs and visualization_msgs. Other packages can be generated with 生成器.suru.


```{toctree}
:maxdepth: 2
:caption: API
:glob:

api/*
```
