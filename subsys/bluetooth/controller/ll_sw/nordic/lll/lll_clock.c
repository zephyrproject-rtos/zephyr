/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <soc.h>
#include <device.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_clock
#include "common/log.h"
#include "hal/debug.h"

/* Clock setup timeouts are unlikely, below values are experimental */
#define LFCLOCK_TIMEOUT_MS 500
#define HFCLOCK_TIMEOUT_MS 2

static void clock_ready(struct device *dev, clock_control_subsys_t subsys,
			void *user_data);

static struct device *dev;

int lll_clock_init(void)
{
	int err;

	dev = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	if (!dev) {
		return -ENODEV;
	}

	err = clock_control_on(dev, CLOCK_CONTROL_NRF_SUBSYS_LF);

	return err;
}

int lll_clock_wait(void)
{
	static bool done;

	if (done) {
		return 0;
	}
	done = true;

	struct k_sem sem_clock_wait;
	struct clock_control_async_data async_data = {
		.cb = clock_ready,
		.user_data = &sem_clock_wait,
	};
	int err;

	k_sem_init(&sem_clock_wait, 0, 1);

	err = clock_control_async_on(dev, CLOCK_CONTROL_NRF_SUBSYS_LF,
				     &async_data);
	if (err) {
		return err;
	}

	err = k_sem_take(&sem_clock_wait, K_MSEC(LFCLOCK_TIMEOUT_MS));

	return err;
}

int lll_hfclock_on(void)
{
	int err;

	/* turn on radio clock in non-blocking mode. */
	err = clock_control_on(dev, CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!err || err == -EINPROGRESS) {
		DEBUG_RADIO_XTAL(1);
	}

	return err;
}

int lll_hfclock_on_wait(void)
{
	struct k_sem sem_clock_wait;
	struct clock_control_async_data async_data = {
		.cb = clock_ready,
		.user_data = &sem_clock_wait,
	};
	int err;

	k_sem_init(&sem_clock_wait, 0, 1);

	err = clock_control_async_on(dev, CLOCK_CONTROL_NRF_SUBSYS_HF,
				     &async_data);
	LL_ASSERT(!err);

	err = k_sem_take(&sem_clock_wait, K_MSEC(HFCLOCK_TIMEOUT_MS));

	DEBUG_RADIO_XTAL(1);

	return err;
}

int lll_hfclock_off(void)
{
	int err;

	/* turn off radio clock in non-blocking mode. */
	err = clock_control_off(dev, CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!err) {
		DEBUG_RADIO_XTAL(0);
	} else if (err == -EBUSY) {
		DEBUG_RADIO_XTAL(1);
	}

	return err;
}

static void clock_ready(struct device *dev, clock_control_subsys_t subsys,
			void *user_data)
{
	k_sem_give(user_data);
}
