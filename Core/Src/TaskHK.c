//
// Created by kikuchi on 2026/10/21.
//

#include <sys/types.h>
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "SEGGER_RTT.h"
#include "TaskHK.h"
#include "TaskHDLC.h"
//
extern QueueHandle_t hTxQ, hRxQ;
extern osThreadId_t TaskHDLCHandle;

// printfはnewlibのstdoutロック(taskENTER_CRITICAL)を握ったままUART送信するので、
// 送信中はTIM2割込とタスク切替が止まり、HDLCが取りこぼす。
// ロックを持たず送信待ちもしない SEGGER_RTT_printf を使うこと
// (RTTはバッファ満杯時は捨てる NO_BLOCK_SKIP。ロックは短時間のBASEPRIのみ)

// タスクのエントリポイント。weakをここで上書き
void StartTaskHK(void *argument) {
    uint8_t tx_data[Q_DATA_SIZE];
    uint8_t rx_data[Q_DATA_SIZE];
    static uint8_t f_count = 0;
    static uint32_t okFrame = 0, ngFrame = 0, ngByte = 0;

    // 基準時刻の初期化と現在のティック数を取得
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // 処理開始
        for (int i = 0; i < Q_DATA_SIZE; i++) {
            tx_data[i] = i;
        }
        // 送信実行
        xQueueSend(hTxQ, tx_data, 0);
        osDelay(200);

        // 受信
        if (xQueueReceive(hRxQ, rx_data, 1500) == pdTRUE) {
            // テストパターン(0,1,2,...)と照合して良否を数える
            int ng = 0;
            for (int i = 0; i < Q_DATA_SIZE; i++) {
                if (rx_data[i] != i) ng++;
            }
            if (ng == 0) {
                okFrame++;
            } else {
                ngFrame++;
                ngByte += ng;
            }
            SEGGER_RTT_printf(0, "GSE %d : OK:%lu NG:%lu NGBYTE:%lu\r\n", Is_gse, okFrame, ngFrame, ngByte);
            for (int i = 0; i < Q_DATA_SIZE; i++) {
                SEGGER_RTT_printf(0, "%04d ", rx_data[i]);
            }
            SEGGER_RTT_printf(0, "\r\n");
        } else {
            SEGGER_RTT_printf(0, "Receive timeout\r\n");
        }
        // 受信状態
        SEGGER_RTT_printf(0, "RDY:%lu OVF:%lu UDF:%lu FER:%lu FCS:%lu LEN:%d\r\n",
                HdlcCnt.rxReady, HdlcCnt.rxOverflow, HdlcCnt.txUnderflow,
                HdlcCnt.frameError, HdlcCnt.fcs, HdlcCnt.lastLen);
        // 送信状態とスタック残量(word単位。0に近いと溢れかけ)
        SEGGER_RTT_printf(0, "SRV:%lu TXF:%lu STK HDLC:%lu HK:%lu\r\n",
                HdlcCnt.service, HdlcCnt.txFrame,
                (uint32_t) uxTaskGetStackHighWaterMark(TaskHDLCHandle),
                (uint32_t) uxTaskGetStackHighWaterMark(NULL));
        // フレーム終端時の内訳
        SEGGER_RTT_printf(0, "FCSERR:%lu SHORT:%lu QFULL:%lu\r\n",
                HdlcCnt.fcsWithErr, HdlcCnt.shortFrame, HdlcCnt.rxQFull);
        // 待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        f_count++;
    }
}
