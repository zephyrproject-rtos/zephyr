/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INTC_GRP_RX_ARCH_H_
#define ZEPHYR_INCLUDE_INTC_GRP_RX_ARCH_H_

#include "platform.h"

int rx_grp_intc_set_callback(const struct device *dev, bsp_int_src_t vector, bsp_int_cb_t callback,
			     void *context);
int rx_grp_intc_set_grp_int(const struct device *dev, bsp_int_src_t vector, bool set);
int rx_grp_intc_set_gen(const struct device *dev, int is_number, bool set);

#endif /* ZEPHYR_INCLUDE_INTC_GRP_RX_ARCH_H_ */
