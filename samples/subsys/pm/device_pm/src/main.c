/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include "dummy_driver.h"

/* Application main Thread */
int main(void)
{
	const struct device *dev;
	struct dummy_driver_api *api;
	int ret, val;

	printk("Device PM sample app start\n");
	dev = device_get_binding(DUMMY_DRIVER_NAME);
	api = (struct dummy_driver_api *)dev->api;
	ret = api->open(dev);
	val = 10;
	ret = api->write(dev, val);
	ret = api->read(dev, &val);
	ret = api->close(dev);
	printk("Device PM sample app complete\n");
	return 0;
}
