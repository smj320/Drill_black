//
// Created by kikuchi on 2026/10/21.
//
#include <string.h>
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "cmx869b.h"
#include "TaskHDLC.h"

#include <sys/types.h>
// ----------------------------------------------------------------------------
// 各種宣言と確保
// ----------------------------------------------------------------------------
// タスク起床用に自分のハンドルを持ってくる
QueueHandle_t hTxQ, hRxQ;
extern osThreadId_t TaskHDLCHandle;
extern TIM_HandleTypeDef htim2;
// CubeMXのキュー要素型とバッファ長の一致を保証
_Static_assert(sizeof(HdlcFrame_t) == Q_DATA_SIZE, "HdlcFrame_t size != Q_DATA_SIZE");
//
int FrameError = 0;
// 診断用カウンタ(TaskHKで表示)
HdlcCntType HdlcCnt = {0};

// ISRから読んでもらう
void Wakeup_TaskHDLC_FromISR(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(TaskHDLCHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ----------------------------------------------------------------------------
// タスクのエントリポイント。weakをここで上書き
// ----------------------------------------------------------------------------
// -------------------------------------
// HDLC service
// -------------------------------------
static void HDLC_service() {
    StatusRegType st = {0};
    static uint8_t rxBuffer[Q_DATA_SIZE];
    static uint8_t txBuffer[Q_DATA_SIZE];
    static int rxPtr = 0;
    static int txPtr = Q_DATA_SIZE;
    //ステータス確認
    st = cbus_read_status();
    HdlcCnt.service++;
    // ------------------------
    // TXレジスタがReadyならデバイスに書く
    // ------------------------
    if (st.Bits.TxDataReady == 1) {
        //txPtrがxQTxの端まで言っていたらキューの有無をしらべて、txPtr=0にする
        if (txPtr == Q_DATA_SIZE) {
            if (xQueueReceive(hTxQ, txBuffer, 0) == pdPASS) {
                HdlcCnt.txFrame++;
                txPtr = 0;
            }
        }
        //端までいっていなければ送信
        if (txPtr < Q_DATA_SIZE) {
            cbus_write_data(txBuffer[txPtr++]);
        }
    }
    // ------------------------
    // RXレジスタがReadyなら読む
    // ------------------------
    if (st.Bits.RxDataReady == 1) {
        uint8_t rx_data = cbus_read_data();
        HdlcCnt.rxReady++;
        //ポインタが端までいっていなければバッファに書く
        if (rxPtr < Q_DATA_SIZE) {
            rxBuffer[rxPtr++] = rx_data;
        }
    }
    if (st.Bits.RxDataOverflow == 1) HdlcCnt.rxOverflow++;
    if (st.Bits.TxDataUnderflow == 1) HdlcCnt.txUnderflow++;
    //フレームが壊れていればそこまでの受信データを捨てる
    if (st.Bits.FrameError == 1) {
        HdlcCnt.frameError++;
        FrameError = 1;
        rxPtr = 0;
    }
    //パケットが終了ならキューに書き込んでrxPtr=0に戻す
    //終端フラグはRxDataReadyと別のタイミングで立つことがあるので独立して見る
    //FCSビットは立ちっぱなしのことがあるので、受信データがあるときだけ送る
    if (st.Bits.FCS == 1 && rxPtr > 0) {
        HdlcCnt.fcs++;
        HdlcCnt.lastLen = rxPtr;
        //終端時の状態を記録(FrameErrorが同時に立っていればCRC異常の可能性)
        if (st.Bits.FrameError == 1) HdlcCnt.fcsWithErr++;
        if (rxPtr != Q_DATA_SIZE) HdlcCnt.shortFrame++;
        if (xQueueSend(hRxQ, rxBuffer, 0) != pdPASS) HdlcCnt.rxQFull++;
        rxPtr = 0;
    }
}

// -------------------------------------
// タスクのエントリーポイントとメインループ
// -------------------------------------
void StartTaskHDLC(void *argument) {
    // CMX869Bの初期化
    cmx869b_init();
    // 初期化完了後に周期割込(TIM2)を開始。タスク生成前に割込が来るのを防ぐ
    HAL_TIM_Base_Start_IT(&htim2);

    for (;;) {
        // 周期割込待ち
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);;
        // 計測開始
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
        //割込処理の実体
        HDLC_service();
        // 計測終了
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}
