/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <sys/off.h>

void main(void)
{
	struct device *clock = device_get_binding("CLOCK");
	struct device *spi = device_get_binding("SPI");
	struct onoff_client client;
	struct onoff_service *srv;
	int err;

	err = device_subsys_get_onoff_service(clock, subsys, &srv);
	if (err < 0) {
		LOG_ERR("Failed to get device onoff service (err: %d)", err);
	}

	err = onoff_request(srv, &client);

	/* hypothetical another api supported by the device */
	err = device_get_op_mngr(spi, &op_mngr);
	if (err < 0) {

	}

	err = op_mngr_schedule(op_mngr, transaction);
}
