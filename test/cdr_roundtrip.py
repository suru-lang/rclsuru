#!/usr/bin/env python3
"""rclsuru の CDR の試験: 同梱した型(型/*.suru)を全部、rclpy と往復させる。

  python3 test/cdr_roundtrip.py

型ごとに乱数で値を作り、rclpy で直列化したバイト列を する のプログラムに渡す。
する は生成した型で読み取って(メッセージ復元する)、また直列化する(メッセージ直列化する)。
その結果を rclpy で読み戻し、もとの値と同じになることを確かめる。
ROS 2 の環境(source /opt/ros/$ROS_DISTRO/setup.bash 済み)と kumu が要る。
"""
import importlib
import os
import random
import re
import subprocess
import sys
import tempfile

from rclpy.serialization import deserialize_message, serialize_message
from rosidl_parser import definition as d

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CASES_PER_TYPE = int(os.environ.get("CASES", "3"))
random.seed(int(os.environ.get("SEED", "1")))

INT_RANGES = {
    "int8": (-128, 127), "uint8": (0, 255), "char": (0, 255), "octet": (0, 255),
    "int16": (-32768, 32767), "uint16": (0, 65535),
    "int32": (-2**31, 2**31 - 1), "uint32": (0, 2**32 - 1),
    # する の数は ±2^62 まで(コンパイルしたもの)
    "int64": (-2**62, 2**62 - 1), "uint64": (0, 2**62 - 1),
}
TEXT = "abcxyz する ROS 2 日本語 ✓"


def bundled_types():
    """型/*.suru が作った型の (パッケージ, 種類, 名前) と、作らなかった型の名前"""
    types, skipped = [], set()
    for f in sorted(os.listdir(os.path.join(ROOT, "型"))):
        if not f.endswith(".suru"):
            continue
        text = open(os.path.join(ROOT, "型", f), encoding="utf-8").read()
        for m in re.finditer(r"^(\w+)型 とは 型名 は 「(\w+)/(msg|srv)/(\w+)」 で 寸法", text, re.M):
            types.append((m.group(2), m.group(3), m.group(4)))
        for m in re.finditer(r"^# (\w+)/(msg|srv)/(\w+) は .*作りません", text, re.M):
            skipped.add(m.group(3))
    return types, skipped


def python_class(pkg, kind, name):
    mod = importlib.import_module(f"{pkg}.{kind}")
    if kind == "srv":
        srv, part = name.rsplit("_", 1)
        return getattr(getattr(mod, srv), part)
    return getattr(mod, name)


def value_of(t):
    if isinstance(t, d.BasicType):
        n = t.typename
        if n == "boolean":
            return random.random() < 0.5
        if n == "float":
            return random.randint(-4000, 4000) / 8.0  # float32 で丸めずに表せる値
        if n in ("double", "long double"):
            return random.uniform(-1e6, 1e6)
        lo, hi = INT_RANGES.get(n, (0, 0))
        v = random.randint(lo, hi)
        return bytes([v]) if n == "octet" else v
    if isinstance(t, (d.UnboundedString, d.BoundedString)):
        n = random.randint(0, 12)
        if isinstance(t, d.BoundedString):
            n = min(n, t.maximum_size)
        s = "".join(random.choice(TEXT) for _ in range(n))
        if isinstance(t, d.BoundedString):
            while len(s.encode()) > t.maximum_size:
                s = s[:-1]
        return s
    if isinstance(t, d.NamespacedType):
        mod = importlib.import_module(".".join(t.namespaces))
        return fill(getattr(mod, t.name)())
    if isinstance(t, d.Array):
        return [value_of(t.value_type) for _ in range(t.size)]
    if isinstance(t, (d.UnboundedSequence, d.BoundedSequence)):
        n = random.randint(0, 3)
        if isinstance(t, d.BoundedSequence):
            n = min(n, t.maximum_size)
        return [value_of(t.value_type) for _ in range(n)]
    raise ValueError(f"知らない型 {t}")


def fill(msg):
    for field, t in zip(msg.get_fields_and_field_types().keys(), msg.SLOT_TYPES):
        setattr(msg, field, value_of(t))
    return msg


def main():
    types, skipped = bundled_types()
    work = tempfile.mkdtemp(prefix="rclsuru_cdr_")
    data = os.path.join(work, "data")
    os.makedirs(data)
    cases = []
    for pkg, kind, name in types:
        cls = python_class(pkg, kind, name)
        for k in range(CASES_PER_TYPE):
            msg = cls() if k == 0 else fill(cls())  # 1 つ目は既定の値
            i = len(cases)
            with open(os.path.join(data, f"{i}.in"), "wb") as f:
                f.write(serialize_message(msg))
            cases.append((i, f"{pkg}/{kind}/{name}", cls, msg))
    with open(os.path.join(data, "一覧.txt"), "w", encoding="utf-8") as f:
        for i, full, _, _ in cases:
            f.write(f"{i} {full}\n")

    # 型ごとに 往復する の場合を並べた する のプログラム
    proj = os.path.join(work, "往復")
    os.makedirs(proj)
    with open(os.path.join(proj, "組.suru"), "w", encoding="utf-8") as f:
        f.write(f"名前 とは 「往復」\n依存 とは 「{ROOT}」\n")
    pkgs = sorted({p for p, _, _ in types})
    lines = ["「標準」 を 読み込んで 使う", "「ファイル」 を 読み込む", "「環境」 を 読み込む",
             "「rclsuru/CDR」 を 読み込む"]
    lines += [f"「rclsuru/型/{p}」 を 読み込む" for p in pkgs]
    lines += ["", "文字列 を バイト列 で 往復する は バイト列 の 動作"]
    for pkg, kind, name in types:
        t = f"{pkg}内の{name}型"
        lines += [f"「{pkg}/{kind}/{name}」 を b で 往復する とは 手順",
                  f"  x とは b を {t} で CDR内のメッセージ復元した 結果",
                  f"  x を {t} で CDR内のメッセージ直列化する"]
    lines += [
        "名 を b で 往復する とは （「知らない型: 」 + 名） を 失敗する",
        "",
        "文字列 を 文字列 で 一件往復する は 数 の 動作",
        "行 を 場所 で 一件往復する とは 手順",
        "  部分 とは 行 を 「 」 で 分割する",
        "  道 とは 場所 + 「/」 + （部分 の 先頭）",
        "  b とは （道 + 「.in」） を ファイル内のバイト列読込した 結果",
        "  c とは （部分 の 最後） を b で 往復した 結果",
        "  c を （道 + 「.out」） へ ファイル内のバイト列書出する",
        "",
        "始めする とは 手順",
        "  引数 とは 環境内の引数取得した 結果",
        "  場所 とは 引数 の 先頭",
        "  行群 とは （場所 + 「/一覧.txt」） を ファイル内の行分割読込した 結果",
        "  作業 とは 行群 の それぞれ を 場所 で 一件往復する",
        "  作業 を 実行する",
        "",
        "始めする",
    ]
    with open(os.path.join(proj, "本体.suru"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    r = subprocess.run(["kumu", "実行", "--", data], cwd=proj)
    if r.returncode != 0:
        print("する のプログラムが失敗しました")
        return 1

    failed = 0
    for i, full, cls, msg in cases:
        out = open(os.path.join(data, f"{i}.out"), "rb").read()
        back = deserialize_message(out, cls)
        # バイト列は比べない(Fast CDR は揃えの隙間に前のゴミを残すことがある)
        if repr(back) != repr(msg):
            failed += 1
            a, b = serialize_message(msg), serialize_message(back)
            k = next((j for j in range(min(len(a), len(b))) if a[j] != b[j]), min(len(a), len(b)))
            print(f"不合格: {full}(長さ {len(a)} と {len(b)}、{k} バイト目から違う)")
            print(f"  もと: {a[max(0, k - 8):k + 16].hex(' ')}\n  往復: {b[max(0, k - 8):k + 16].hex(' ')}")
    print(f"{len(types)} 個の型、{len(cases)} 個の値を往復しました。不合格 {failed}")
    if skipped:
        print("作らなかった型: " + "・".join(sorted(skipped)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
