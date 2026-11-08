/**
 * @file    rtt_term.c
 * @brief   Thread-safe formatted output to SEGGER RTT virtual terminals (channel 0).
 */
#include <stdarg.h>
#include "rtt_term.h"
#include "SEGGER_RTT.h"
#include "cmsis_os.h"
#include "main.h"   /* __get_IPSR() via CMSIS */

static osMutexId_t rttMutex;

static const osMutexAttr_t rttMutex_attr = {
  .name      = "rttMutex",
  .attr_bits = osMutexPrioInherit,
};

void RTT_TermInit(void)
{
  SEGGER_RTT_Init();
  if (rttMutex == NULL) {
    rttMutex = osMutexNew(&rttMutex_attr);
  }
}

int RTT_TermPrintf(unsigned char term, const char *fmt, ...)
{
  va_list ap;
  int r;
  int locked = 0;

  if (__get_IPSR() != 0U || term > 15U) {
    return -1;   /* not allowed in ISR (mutex) / RTT supports terminals 0..15 */
  }

  /* Before the scheduler starts there is only one thread, so no lock is needed. */
  if (rttMutex != NULL && osKernelGetState() == osKernelRunning) {
    osMutexAcquire(rttMutex, osWaitForever);
    locked = 1;
  }

  SEGGER_RTT_SetTerminal(term);
  va_start(ap, fmt);
  r = SEGGER_RTT_vprintf(0, fmt, &ap);
  va_end(ap);

  if (locked) {
    osMutexRelease(rttMutex);
  }
  return r;
}
