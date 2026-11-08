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

        // 受信状態
        SEGGER_RTT_printf(0, "TI:%05d SNR:%02X MODE:%02X ERR:%03d LEN: %03d\r\n",
            f_count, HdlcCnt.SNR, HdlcCnt.MODE, HdlcCnt.frameError, HdlcCnt.lastLen);
        // フレーム終端時の内訳
        if (xQueueReceive(hRxQ, rx_data, 1500) == pdTRUE) {
            for (int i = 0; i < Q_DATA_SIZE; i++) {
                SEGGER_RTT_printf(0, "%04d ", rx_data[i]);
            }
            SEGGER_RTT_printf(0, "\r\n");
        } else {
            SEGGER_RTT_printf(0, "Receive timeout\r\n");
        }
        // 待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        f_count++;
    }
}
