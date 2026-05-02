/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_icu

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/interrupt_controller/intc_rx_icu.h>
#include <zephyr/sw_isr_table.h>
#include <errno.h>

#define IR_BASE_ADDRESS       DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IR)
#define IRQCR_BASE_ADDRESS    DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IRQCR)
#define IRQFLTE_BASE_ADDRESS  DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IRQFLTE)
#define IRQFLTC0_BASE_ADDRESS DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IRQFLTC0)

#define IRi_REG(i)    (IR_BASE_ADDRESS + (i))
#define IRQCRi_REG(i) (IRQCR_BASE_ADDRESS + (i))

static struct k_spinlock lock;

void rx_icu_clear_ir_flag(unsigned int irqn)
{
	volatile uint8_t *icu_ir = (uint8_t *)IRi_REG(irqn);

	/* Clear IR Register */
	*icu_ir = 0x0;
}

int rx_icu_get_ir_flag(unsigned int irqn)
{
	volatile uint8_t *icu_ir = (uint8_t *)IRi_REG(irqn);

	/* Return IR Register */
	return *icu_ir;
}

int rx_icu_set_irq_control(unsigned int pin_irqn, enum icu_irq_mode mode)
{

	volatile uint8_t *icu_irqcr = (uint8_t *)IRQCRi_REG(pin_irqn);

	if (mode >= ICU_MODE_NONE) {
		return -EINVAL;
	}

	/* Set IRQ Control Register */
	*icu_irqcr = (uint8_t)(mode << 2);

	return 0;
}

void rx_icu_set_irq_dig_filt(unsigned int pin_irqn, rx_irq_dig_filt_t dig_filt)
{
	volatile uint8_t *icu_irqflte = (uint8_t *)IRQFLTE_BASE_ADDRESS;
	volatile uint16_t *icu_irqfltc0 = (uint16_t *)IRQFLTC0_BASE_ADDRESS;
	uint8_t temp_8bit;
	uint16_t temp_16bit;

	k_spinlock_key_t key = k_spin_lock(&lock);
	/* Set IRQ Pin Digital Filter Setting Register 0 (IRQFLTC0) */
	temp_16bit = *icu_irqfltc0;
	temp_16bit &= (uint16_t) ~(0x0003 << (pin_irqn * 2));
	temp_16bit |= ((uint16_t)(dig_filt.filt_clk_div) << (pin_irqn * 2));
	*icu_irqfltc0 = temp_16bit;
	k_spin_unlock(&lock, key);

	key = k_spin_lock(&lock);
	/* Set IRQ Pin Digital Filter Enable Register 0 (IRQFLTE0) */
	temp_8bit = *icu_irqflte;
	temp_8bit &= (uint8_t) ~(1 << pin_irqn);
	temp_8bit |= (uint8_t)(dig_filt.filt_enable << pin_irqn);
	*icu_irqflte = temp_8bit;
	k_spin_unlock(&lock, key);
}

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
