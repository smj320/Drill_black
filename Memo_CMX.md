# CMX869B系Memo

## ポーリング

1200bps　pre 39, cc 1999
14400/12000 = 1.2 　実際は4800のポーリングでもいける。
14000/9600 = 1.5
14400/7200 = 2
14400/4800 = 3
14400/2400 = 6
14400/1200 = 12

実際は　4800のポーリングでもいける。
14400　4800で1byteを3回
12000  4800
9600   4800
7200
4800
2400
1200   4800

4800Hzで回して、二回に一回BitBangを行う。



## HDLCパケットの定義

main.hでしておく。

```c++
#include "cmsis_os.h"
// コントロールバイト（フラグ）の定義
#define MSG_FLAG_SINGLE  0x00  // 単一データパケット
#define MSG_FLAG_START   0x01  // 開始パケット
#define MSG_FLAG_MIDDLE  0x02  // 中間パケット
#define MSG_FLAG_END     0x03  // 終了パケット
#define MSG_FLAG_OK     0x10  // ACK（確認応答）パケット
#define MSG_FLAG_NG     0x11  // 再送要求

typedef struct {
  uint8_t  ucDeviceID;        // 送信元／宛先のタスクID（1〜3）
  uint8_t  ucControlFlags;    // パケット種別（START/MIDDLE/END/SINGLE/ACK）
  uint8_t  ucPacketSeq;       // パケット連番（0〜255のローテーション）
  size_t   xCurrentLength;    // データ長（最大80バイト、OK,NGの時は0）
  uint8_t  ucData[80];        // ペイロード（実データ、ACKの時は不使用）
} xHdlcMessage_t;

//CubeMXでQueueのハンドルとして
//QueueTx, QueueRxHK, QueueRxImg, QueueRxCMDを定義しておく
extern osMessageQueueId_t QueueTxHandle;
extern osMessageQueueId_t QueueRxHKHandle;
extern osMessageQueueId_t QueueRxCMDHandle;
extern osMessageQueueId_t QueueRxImgHandle;
```


### HDLCパケット
コントロールビット、
position  head 0b00, body 0b01, tail 0b10 single b11 2bit
アドレス　3bit
アボート　1bit
シーケンスカウンタ
データ長
データバッファ80byte


## Txのシーケンス
一回書くと、一文字の送信に対して5回割込がでる。実際の信号とも一致。

```aiignore
HK:09
0000:1 0 0 0 開始（おそらくTX=0から１に変わった）
0000:1 1 0 0 送信完了
0000:1 1 1 0 受信レジスタが有効になった
____:1 1 0 0 FSCを受信開始
0000:1 1 1 0 受信レジスタが有効になった
____:1 1 0 0 CRCを受信開始
0000:1 1 1 0 受信レジスタが有効になった

Txの操作
バッファにデータを入れて、割込許可をするとただちにTxDataReady割込が出るので処理させる。
送信データを送りきったところで最後のTxDataReady割込が出るので、そこで割込禁止にする。

```

oops: 3 の中身
ログに出ていない回は、ステータスを読んだが何もしなかった回です。loops: 3 の部分を分けるとこうなります。
回 | ステータス |処理 | 状況
1  | TXD=1 |73 を書く（xprintf で約4ms） | この間に 70 を受信し、IRQN が Low になる
2 | RXD=1 | 70 を読む（xprintf）| この間に送信バッファが空き、TXD=1 → IRQN が Low になる
3 | TXD=1 | 書くデータが無いので何もしない ここで IRQ がクリアされ、PIN=1 になってループを抜ける

## 呼出方法
リセットの効果を知りたいので、ドリルのほうから発信するようにしたい。
すると、ドリルがAnswer側ということになる。
1) GSEのモデムを起動またはリセット、受信待機
2) ドリルを起動、またはリセット

## タイマーの設定
TIM2をCMX869bのポーリング用に使う。値の設定はプログラムで。
TIM3は1200bpsのBit Bankの周期だが16bitカウンタなので最大65535
現在96MHzなのでプリスケーラ999で96x1000/1200-1=79

に倣って追加する。

## タイマ割込の実行コードをかく場所
設置場所はmain.cの
```c++
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}
```
あたりでWakeup_TaskHDLC_FromISR()を呼んで貰う。中身はタスクの起床。

## クロック
ボーレート (N)おすすめの設定値例 (96MHz基準)実効周波数誤差
1,200 Hz PSC = 99, ARR = 799 （総分周 80,000） 1,200.0 Hz 0.00%
2,400 Hz　PSC = 99, ARR = 399 （総分周 40,000） 2,400.0 Hz0.00%
9,600 HzPSC = 24, ARR = 399 （総分周 10,000） 9,600.0 Hz0.00%
12,000 HzPSC = 19, ARR = 399 （総分周 8,000） 12,000.0 Hz0.00%
14,400 HzPSC = 23, ARR = 277 （総分周 6,672） 14,388.5 Hz-0.08%
