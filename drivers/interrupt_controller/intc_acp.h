/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_ACP_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_ACP_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*acp_intc_ictl_config_irq_t)(const struct device *port);

/* IRQ numbers - wrt Tensilica DSP */
#define IRQ_NUM_SOFTWARE0  1 /* level 1 */
#define IRQ_NUM_TIMER0     0 /* level 1 */
#define IRQ_NUM_EXT_LEVEL3 3 /* level 1 */
#define IRQ_NUM_TIMER1     6 /* level 2 */
#define IRQ_NUM_EXT_LEVEL4 4 /* level 2 */
#define IRQ_NUM_EXT_LEVEL5 5 /* level 3 */

typedef void (*irq_enable_t)(const struct device *dev, uint32_t irq);
typedef void (*irq_disable_t)(const struct device *dev, uint32_t irq);
typedef int (*irq_is_enabled_t)(const struct device *dev, unsigned int irq);
typedef int (*irq_connect_dynamic_t)(const struct device *dev, unsigned int irq,
				     unsigned int priority, void (*routine)(const void *parameter),
				     const void *parameter, uint32_t flags);

struct acp_intc_driver_api {
	irq_enable_t intr_enable;
	irq_disable_t intr_disable;
	irq_is_enabled_t intr_is_enabled;
#ifdef CONFIG_DYNAMIC_INTERRUPTS
	irq_connect_dynamic_t intr_connect_dynamic;
#endif
};

struct acp_intc_reg {
	uint32_t acp_intc_ctrl;
	uint32_t acp_intc_ctrl1;
	uint32_t acp_intc_status;
	uint32_t acp_intc_status1;
	uint32_t acp_external_intr_enb;
	uint32_t sw_intr_ctrl_reg;
};

/* Tensilica DSP interrupt levels */
typedef enum {
	ACP_INTC_LEVEL_3 = 0,
	ACP_INTC_LEVEL_4,
	ACP_INTC_LEVEL_5,
	ACP_INTC_LEVEL_NMI,
	ACP_INTC_LEVEL_MAX
} acp_intc_interrupt_level_t;

/* Tensilica DSP timer control modes */
typedef enum {
	ACP_INTC_TIMER_CNTL_DISABLE = 0,
	ACP_INTC_TIMER_CNTL_ONESHOT,
	ACP_INTC_TIMER_CNTL_PERIODIC,
	ACP_INTC_TIMER_CNTL_MAX
} acp_intc_timer_control_t;

struct acp_intc_cfg {
	uint32_t irq_num;
	uint32_t isr_table_offset;
	acp_intc_ictl_config_irq_t config_func;
};

struct acp_intc_reg_base {
	uint32_t base;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_ACP_H_ */
