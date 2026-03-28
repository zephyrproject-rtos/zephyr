/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CAVS_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CAVS_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cavs_ictl_config_irq_t)(const struct device *port);

struct cavs_ictl_config {
	uint32_t irq_num;
	uint32_t isr_table_offset;
	cavs_ictl_config_irq_t config_func;
};

struct cavs_ictl_runtime {
	uint32_t base_addr;
};

struct cavs_registers {
	uint32_t disable_il;	/* il_msd - offset 0x00 */
	uint32_t enable_il;	/* il_mcd - offset 0x04 */
	uint32_t disable_state_il;	/* il_md  - offset 0x08 */
	uint32_t status_il;	/* il_sd  - offset 0x0C */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CAVS_H_ */
