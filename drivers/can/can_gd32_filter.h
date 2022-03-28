/*
 * Copyright (c) 2022 YuLong Yao <feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_GD32_FILTER_H_
#define ZEPHYR_DRIVERS_CAN_GD32_FILTER_H_

#include <kernel.h>

enum can_fifo { CAN_FIFO_0, CAN_FIFO_1 };

enum can_filter_status {
	CAN_FILTER_EMPTY,
	CAN_FILTER_16BIT_1,
	CAN_FILTER_16BIT_2,
	CAN_FILTER_16BIT_3,
	CAN_FILTER_16BIT_4,
	CAN_FILTER_32BIT,
};

enum can_filter_type {
	CAN_FILTER_16BIT_LIST,
	CAN_FILTER_16BIT_MASK,
	CAN_FILTER_32BIT_LIST,
	CAN_FILTER_32BIT_MASK,
};

struct can_gd32_filter_unit {
	enum can_filter_status status;
	enum can_filter_type type;

	can_rx_callback_t callback;
	void *callback_arg;
};

struct can_gd32_filter_data {
	struct k_mutex mutex;
	bool initialed;
};

struct can_gd32_filter_cfg {
	uint32_t reg;
	uint32_t rcu_periph_clock;
	uint8_t size;
	struct can_gd32_filter_unit *unit;
};

struct can_gd32_filter {
	struct can_gd32_filter_data *data;
	struct can_gd32_filter_cfg *cfg;
};

int can_gd32_filter_initial(const struct can_gd32_filter *filter);
int can_gd32_filter_getmaxsize(const struct can_gd32_filter *filter, enum can_ide id_type,
			       bool is_main_controller);
int can_gd32_filter_add(const struct can_gd32_filter *filter, bool is_main_controller,
			enum can_fifo fifo, can_rx_callback_t cb, void *cb_arg,
			const struct zcan_filter *zfilter);
void can_gd32_filter_remove(const struct can_gd32_filter *filter, int filter_id);
can_rx_callback_t can_gd32_filter_getcb(const struct can_gd32_filter *filter,
					bool is_main_controller, enum can_fifo fifo,
					uint8_t filter_number);
void *can_gd32_filter_getcbarg(const struct can_gd32_filter *filter, bool is_main_controller,
			       enum can_fifo fifo, int filter_number);
#endif /* ZEPHYR_DRIVERS_CAN_GD32_FILTER_H_ */
