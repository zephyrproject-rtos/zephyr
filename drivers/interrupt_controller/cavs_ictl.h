/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_CAVS_ICTL_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_CAVS_ICTL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cavs_ictl_config_irq_t)(struct device *port);

struct cavs_ictl_config {
	u32_t irq_num;
	u32_t isr_table_offset;
	cavs_ictl_config_irq_t config_func;
};

struct cavs_ictl_runtime {
	u32_t base_addr;
};

struct cavs_registers {
	u32_t disable_il;	/* il_msd - offset 0x00 */
	u32_t enable_il;	/* il_mcd - offset 0x04 */
	u32_t disable_state_il;	/* il_md  - offset 0x08 */
	u32_t status_il;	/* il_sd  - offset 0x0C */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_CAVS_ICTL_H_ */
