/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_CONSOLE_H__
#define __XEN_CONSOLE_H__

#include <init.h>
#include <device.h>
#include <drivers/uart.h>
#include <sys/device_mmio.h>

struct hvc_xen_data {
	DEVICE_MMIO_RAM;	/* should be first */
	const struct device *dev;
	struct xencons_interface *intf;
	uint64_t evtchn;
};

int xen_console_init(const struct device *dev);

#endif /* __XEN_CONSOLE_H__ */
