//
// Created by kikuchi on 2026/10/21.
//

#include <sys/types.h>
#include "cmsis_os.h"
#include "main.h"
#include "task.h"
#include "queue.h"
#include "TaskHK.h"
#include "TaskHDLC.h"
#include "RTT_Term.h"
//
//extern QueueHandle_t hTxQ, hRxQ;
extern osThreadId_t TaskHDLCHandle;

// タスクのエントリポイント。weakをここで上書き
void StartTaskHK(void *argument) {
    uint8_t tx_data[Q_DATA_SIZE];
    uint8_t rx_data[Q_DATA_SIZE];
    static uint8_t f_count = 0;

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

        // フレーム終端時の内訳
        int err = 0;
        if (xQueueReceive(hRxQ, rx_data, 1500) == pdTRUE) {
            for (int i = 0; i < Q_DATA_SIZE; i++) {
                if (rx_data[i] != i) err++;
            }
            RTT_TermPrintf(0, "GSE:%d TI:%05d SNR:%02X MODE:%02X ERR:%03ld LEN: %03d ERR:%03d\r\n",
                           Is_gse,
                           f_count,
                           HdlcCnt.SNR,
                           HdlcCnt.MODE,
                           HdlcCnt.frameError,
                           HdlcCnt.lastLen,
                           err);
        } else {
            RTT_TermPrintf(0, "Receive timeout\r\n");
        }
        // 受信状態

        // 待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        f_count++;
    }
}
