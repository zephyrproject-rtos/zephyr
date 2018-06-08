/**
 * @file cryptdev.c
 * Finds the crypto subsystem pseudodevice at boot
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <init.h>

static struct device *cryptdev;

static int find_cryptdev(struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_CRYPTO_NAME)
	cryptdev = device_get_binding(CONFIG_CRYPTO_NAME);
	if (cryptdev == NULL) {
		return -ENODEV;
	}

	return 0;
#else
	return -ENODEV;
#endif
}

struct device *bt_get_cryptdev(void)
{
	return cryptdev;
}

SYS_INIT(find_cryptdev, PRE_KERNEL_2,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
