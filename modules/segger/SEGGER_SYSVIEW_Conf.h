/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include <zephyr/irq.h>

#define SEGGER_SYSVIEW_GET_TIMESTAMP sysview_get_timestamp
#define SEGGER_SYSVIEW_GET_INTERRUPT_ID sysview_get_interrupt

uint32_t sysview_get_timestamp(void);
uint32_t sysview_get_interrupt(void);

#define SEGGER_SYSVIEW_RTT_BUFFER_SIZE CONFIG_SEGGER_SYSVIEW_RTT_BUFFER_SIZE
#define SEGGER_SYSVIEW_POST_MORTEM_MODE CONFIG_SEGGER_SYSVIEW_POST_MORTEM_MODE

#if defined(CONFIG_SEGGER_SYSVIEW_SECTION_DTCM)
#define SEGGER_SYSVIEW_SECTION	".dtcm_data"
#endif

/* Lock SystemView (nestable) */
#define SEGGER_SYSVIEW_LOCK()	{					       \
					unsigned int __sysview_irq_key =       \
						irq_lock()

/* Unlock SystemView (nestable) */
#define SEGGER_SYSVIEW_UNLOCK()		irq_unlock(__sysview_irq_key);         \
				}
