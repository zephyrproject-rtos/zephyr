/*
 * Copyright (c) 2022 YuLong Yao <feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_GD32_FILTER_H_
#define ZEPHYR_DRIVERS_CAN_GD32_FILTER_H_

#include <kernel.h>

struct can_gd32_filter_data {
	struct k_mutex inst_mutex;
	bool initialed;
};

struct can_gd32_filter_cfg {
	uint32_t reg;
	uint32_t rcu_periph_clock;
	uint8_t size;
};

struct can_gd32_filter {
	struct can_gd32_filter_data *data;
	struct can_gd32_filter_cfg *cfg;
};

int can_gd32_filter_initial(const struct can_gd32_filter *filter);
int can_gd32_filter_getsize(const struct can_gd32_filter *filter, enum can_ide id_type);

#endif /* ZEPHYR_DRIVERS_CAN_GD32_FILTER_H_ */
