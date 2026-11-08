/**
 * @file    rtt_term.h
 * @brief   Thread-safe formatted output to SEGGER RTT virtual terminals (channel 0).
 *
 * SEGGER_RTT_SetTerminal() keeps a global "current terminal" state, so switching
 * terminal and printing must not be interleaved between tasks. RTT_TermPrintf()
 * serializes both steps with an RTOS mutex.
 *
 * Formatting is done by SEGGER_RTT_vprintf (no malloc, small stack, no %f).
 * Must NOT be called from interrupt context (returns -1 in that case).
 */
#ifndef RTT_TERM_H
#define RTT_TERM_H

#ifdef __cplusplus
extern "C" {
#endif

/** Create the mutex. Call once after osKernelInitialize(), before tasks use RTT_TermPrintf. */
void RTT_TermInit(void);

/**
 * printf to RTT channel 0, virtual terminal @p term (0..15).
 * @return number of bytes written, or -1 when called from an ISR / invalid terminal.
 */
int RTT_TermPrintf(unsigned char term, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#ifdef __cplusplus
}
#endif

#endif /* RTT_TERM_H */
