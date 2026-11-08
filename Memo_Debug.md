# Debug環境

## 開発環境

### 言語注入の背景
言語注入の背景書が見づらいので消す。
Settings → Editor → Color Scheme → General → Code → Injected language fragment

## Build関係

### 概要
JLINKノ

### 実行方法
make call, あるいはmake ansでプローブに繋がるJlinkGDBサーバを立ち上げる。
次にCall, Ansのリモートデバッグ環境を実行する。
起動前(Before launch)に、cmakeによるターゲットのBuildと、`make flash-call` / `make flash-ans` による書き込みを行う。
書き込みは JLinkExe を使わず、常駐GDBサーバ(Call :2331 / Ans :2341)経由で arm-none-eabi-gdb の `load` で行う。
```text
monitor clrbp → monitor reset → monitor halt → load → compare-sections
→ monitor reset → DEMCRのベクタキャッチ解除 → disconnect
```
- 書込み後はリセット直後で停止したまま切断するので、CLion接続後に Resume すれば起動直後からデバッグできる。
- GDBサーバが動いていないと make が非0で終わり、Before launch が止まる。
- 前回のデバッグセッションが残っているとGDBサーバが接続を受け付けないので、止めてから実行する。
- JLinkExe 方式をやめた理由: Ans の J-Link Compact PLUS は、GDBサーバ常駐中に JLinkExe から同時接続できない。
  また JLinkExe は切断時にCPUを再開させる(SetRestartOnClose)ため、起動直後のBPに間に合わなかった。
