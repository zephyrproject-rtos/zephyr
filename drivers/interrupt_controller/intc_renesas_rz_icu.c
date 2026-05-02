/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_icu_v2

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/intc_rz_icu.h>
#include <zephyr/drivers/interrupt_controller/gic.h>

#define RZ_ICU_BASE        DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), base)
#define RZ_ICU_INTSEL_BASE (RZ_ICU_BASE + DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), intsel))
#define SELECTABLE_INT_MIN 384
#define SELECTABLE_INT_MAX 543

#define OFFSET(i)                 ((i) - SELECTABLE_INT_MIN - \
					COND_CODE_1(CONFIG_GIC, (GIC_SPI_INT_BASE), (0)))
#define REG_INTSEL_READ(i)        sys_read32(RZ_ICU_INTSEL_BASE + (OFFSET(i) / 2) * 4)
#define REG_INTSEL_WRITE(i, v)    sys_write32((v), RZ_ICU_INTSEL_BASE + (OFFSET(i) / 2) * 4)
#define REG_INTSEL_INTSEL_MASK(i) (BIT_MASK(10) << ((OFFSET(i) % 2) * 16))

int icu_connect_irq_event(int irq, elc_event_t event)
{

	uint32_t reg_val;
	unsigned int key;

	if ((OFFSET(irq) < 0) || (OFFSET(irq) > (SELECTABLE_INT_MAX - SELECTABLE_INT_MIN))) {
		return -EINVAL;
	}

	key = irq_lock();
	reg_val = REG_INTSEL_READ(irq);
	reg_val &= ~REG_INTSEL_INTSEL_MASK(irq);
	reg_val |= FIELD_PREP(REG_INTSEL_INTSEL_MASK(irq), event);
	REG_INTSEL_WRITE(irq, reg_val);
	irq_unlock(key);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
