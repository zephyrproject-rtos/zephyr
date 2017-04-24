#pragma once

#include <stdint.h>

#define SEGGER_SYSVIEW_GET_TIMESTAMP sysview_get_timestamp
#define SEGGER_SYSVIEW_GET_INTERRUPT_ID sysview_get_interrupt

uint32_t sysview_get_timestamp(void);
uint32_t sysview_get_interrupt(void);

#define SEGGER_SYSVIEW_RTT_BUFFER_SIZE 4096
