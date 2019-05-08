/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MCHP_XEC_ICTL_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MCHP_XEC_ICTL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 0
typedef void (*cavs_ictl_config_irq_t)(struct device *port);

struct cavs_ictl_config {
	u32_t irq_num;
	u32_t isr_table_offset;
	cavs_ictl_config_irq_t config_func;
};

struct cavs_ictl_runtime {
	u32_t base_addr;
};
#endif

typedef void (*xec_ictl_config_irq_t)(struct device *port);

struct xec_ictl_config {
	u32_t irq_num;
	u32_t girq_num;
	u32_t isr_table_offset;
	xec_ictl_config_irq_t config_func;
};

struct xec_ictl_runtime {
	u32_t girq_addr;
	u32_t girq_ctrl_addr;
};

struct xec_ctrl_registers {
	u32_t girq_aggr_out_set_en;
	u32_t girq_aggr_out_clr_en;
	u32_t girq_active;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MCHP_XEC_ICTL_H_ */
