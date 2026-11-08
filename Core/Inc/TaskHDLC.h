//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASK_HDLC_H
#define CMX869B_TASK_HDLC_H

#define Q_DATA_SIZE 80

void StartTaskHDLC(void *argument);
void Wakeup_TaskHDLC_FromISR(void);

extern int FrameError;

// フレームキュー (CubeMX生成の QueueTXHandle / QueueRXHandle を main.c で代入)
#include "FreeRTOS.h"
#include "queue.h"
extern QueueHandle_t hTxQ, hRxQ;

// 診断用カウンタ
typedef struct {
    uint32_t frameError;
    uint32_t HDLCAbort;
    uint8_t SNR;
    uint8_t MODE;
    int lastLen;
} HdlcCntType;
extern HdlcCntType HdlcCnt;

#endif //CMX869B_TASK_HDLC_H
