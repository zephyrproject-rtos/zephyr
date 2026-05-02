/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BROADCOM_AFBR_S50_PLATFORM_H_
#define ZEPHYR_DRIVERS_SENSOR_BROADCOM_AFBR_S50_PLATFORM_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/slist.h>

#include <platform/argus_s2pi.h>

struct afbr_s50_platform_data {
	struct {
		argus_hnd_t *handle;
		const uint8_t id;
	} argus;
	struct {
		struct k_timer timer;
		uint32_t interval_us;
		void *param;
	} timer;
	struct {
		atomic_t mode;
		const struct pinctrl_dev_config *pincfg;
		struct {
			struct rtio_iodev *iodev;
			struct rtio *ctx;
			atomic_t state;
			struct {
				s2pi_callback_t handler;
				void *data;
			} callback;
		} rtio;
		struct {
			struct gpio_callback cb;
			s2pi_irq_callback_t handler;
			void *data;
		} irq;
		struct {
			struct {
				const struct gpio_dt_spec *cs;
				const struct gpio_dt_spec *clk;
				const struct gpio_dt_spec *mosi;
				const struct gpio_dt_spec *miso;
			} spi;
			const struct gpio_dt_spec * const irq;
		} gpio;
	} s2pi;
};

struct afbr_s50_platform_init_node {
	sys_snode_t node;
	int (*init_fn)(struct afbr_s50_platform_data *data);
};

void afbr_s50_platform_init_hooks_add(struct afbr_s50_platform_init_node *node);

int afbr_s50_platform_get_by_id(s2pi_slave_t slave,
				struct afbr_s50_platform_data **data);
int afbr_s50_platform_get_by_hnd(argus_hnd_t *hnd,
				 struct afbr_s50_platform_data **data);

#endif /* ZEPHYR_DRIVERS_SENSOR_BROADCOM_AFBR_S50_PLATFORM_H_ */
