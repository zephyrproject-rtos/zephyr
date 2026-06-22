/*
 * Copyright 2021, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zephyr/sys/sys_io.h>
#include <soc_common.h>
#include <adsp/cache.h>

#ifndef __INC_IMX_SOC_H
#define __INC_IMX_SOC_H

/* Macros related to interrupt handling */
#define XTENSA_IRQ_NUM_SHIFT			0
#define XTENSA_IRQ_NUM_MASK			0xff

/*
 * IRQs are mapped on levels. 2nd, 3rd and 4th level are left as 0x00.
 *
 * 1. Peripheral Register bit offset.
 */
#define XTENSA_IRQ_NUMBER(_irq) \
	((_irq >> XTENSA_IRQ_NUM_SHIFT) & XTENSA_IRQ_NUM_MASK)

extern void z_soc_irq_enable(uint32_t irq);
extern void z_soc_irq_disable(uint32_t irq);
extern int z_soc_irq_is_enabled(unsigned int irq);

#endif /* __INC_IMX_SOC_H */
