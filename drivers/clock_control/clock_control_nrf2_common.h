/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_NRF2_COMMON_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_NRF2_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/onoff.h>

#define FLAGS_COMMON_BITS 10

struct clock_onoff {
	struct onoff_manager mgr;
	onoff_notify_fn notify;
	uint8_t idx;
};

#define STRUCT_CLOCK_CONFIG(type, _onoff_cnt) \
	struct clock_config_##type { \
		atomic_t flags; \
		uint32_t flags_snapshot; \
		struct k_work work; \
		uint8_t onoff_cnt; \
		struct clock_onoff onoff[_onoff_cnt]; \
	}

int clock_config_init(void *clk_cfg, uint8_t onoff_cnt,
		      k_work_handler_t handler);

uint32_t clock_config_update_begin(struct k_work *work);

void clock_config_update_end(void *clk_cfg, int status);

int api_nosys_on_off(const struct device *dev, clock_control_subsys_t sys);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_NRF2_COMMON_H_ */
