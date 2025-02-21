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

/**
 * @brief Defines a type for specific clock configuration structure.
 *
 * @param type suffix added clock_config_ to form the type name.
 * @param _onoff_cnt number of clock configuration options to be handled;
 *                   for each one a separate onoff manager instance is used.
 */
#define STRUCT_CLOCK_CONFIG(type, _onoff_cnt) \
	struct clock_config_##type { \
		atomic_t flags; \
		uint32_t flags_snapshot; \
		struct k_work work; \
		uint8_t onoff_cnt; \
		struct clock_onoff onoff[_onoff_cnt]; \
	}

/**
 * @brief Obtain LFOSC accuracy in ppm.
 *
 * @param[out] accuracy Accuracy in ppm.
 *
 * @retval 0 On success
 * @retval -EINVAL If accuracy is not configured.
 */
int lfosc_get_accuracy(uint16_t *accuracy);

/**
 * @brief Initializes a clock configuration structure.
 *
 * @param clk_cfg pointer to the structure to be initialized.
 * @param onoff_cnt number of clock configuration options handled
 *                  handled by the structure.
 * @param update_work_handler function that performs configuration update,
 *                            called from the system work queue.
 *
 * @return 0 on success, negative value when onoff initialization fails.
 */
int clock_config_init(void *clk_cfg, uint8_t onoff_cnt, k_work_handler_t update_work_handler);

/**
 * @brief Starts a clock configuration update.
 *
 * This function is supposed to be called by a specific clock control driver
 * from its update work handler.
 *
 * @param work pointer to the work item received by the update work handler.
 *
 * @return index of the clock configuration onoff option to be activated.
 */
uint8_t clock_config_update_begin(struct k_work *work);

/**
 * @brief Finalizes a clock configuration update.
 *
 * Notifies all relevant onoff managers about the update result.
 * Only the first call after each clock_config_update_begin() performs
 * the actual operation. Any further calls are simply no-ops.
 *
 * @param clk_cfg pointer to the clock configuration structure.
 * @param status result to be passed to onoff managers.
 */
void clock_config_update_end(void *clk_cfg, int status);

int api_nosys_on_off(const struct device *dev, clock_control_subsys_t sys);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_NRF2_COMMON_H_ */
