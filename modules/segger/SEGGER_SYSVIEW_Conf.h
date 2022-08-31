/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SEGGER_SEGGER_SYSVIEW_CONF_H_
#define SEGGER_SEGGER_SYSVIEW_CONF_H_

#include <stdint.h>

#define SEGGER_SYSVIEW_GET_TIMESTAMP sysview_get_timestamp
#define SEGGER_SYSVIEW_GET_INTERRUPT_ID sysview_get_interrupt

uint32_t sysview_get_timestamp(void);
uint32_t sysview_get_interrupt(void);

#define SEGGER_SYSVIEW_RTT_BUFFER_SIZE CONFIG_SEGGER_SYSVIEW_RTT_BUFFER_SIZE
#define SEGGER_SYSVIEW_POST_MORTEM_MODE CONFIG_SEGGER_SYSVIEW_POST_MORTEM_MODE

#if defined(CONFIG_SEGGER_SYSVIEW_SECTION_DTCM)
#define SEGGER_SYSVIEW_SECTION	".dtcm_data"
#endif

extern unsigned int zephyr_rtt_irq_lock(void);
extern void zephyr_rtt_irq_unlock(unsigned int key);

/* Lock SystemView (nestable) */
#define SEGGER_SYSVIEW_LOCK()	{					       \
					unsigned int __sysview_irq_key =       \
						zephyr_rtt_irq_lock()

/* Unlock SystemView (nestable) */
#define SEGGER_SYSVIEW_UNLOCK()		zephyr_rtt_irq_unlock(__sysview_irq_key);         \
				}

#endif  /* SEGGER_SEGGER_SYSVIEW_CONF_H_ */
