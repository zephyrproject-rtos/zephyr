/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RX_ICU_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RX_ICU_H_

#define IRQ_CFG_PCLK_DIV1  (0)
#define IRQ_CFG_PCLK_DIV8  (1)
#define IRQ_CFG_PCLK_DIV32 (2)
#define IRQ_CFG_PCLK_DIV64 (3)

enum icu_irq_mode {
	ICU_LOW_LEVEL,
	ICU_FALLING,
	ICU_RISING,
	ICU_BOTH_EDGE,
	ICU_MODE_NONE,
};

enum icu_dig_filt {
	DISENABLE_DIG_FILT,
	ENABLE_DIG_FILT,
};

typedef struct rx_irq_dig_filt_s {
	uint8_t filt_clk_div; /* PCLK divisor setting for the input pin digital filter. */
	uint8_t filt_enable;  /* Filter enable setting for the input pin digital filter. */
} rx_irq_dig_filt_t;

extern void rx_icu_clear_ir_flag(unsigned int irqn);
extern int rx_icu_get_ir_flag(unsigned int irqn);
extern int rx_icu_set_irq_control(unsigned int pin_irqn, enum icu_irq_mode mode);
extern void rx_icu_set_irq_dig_filt(unsigned int pin_irqn, rx_irq_dig_filt_t dig_filt);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RX_ICU_H_ */
