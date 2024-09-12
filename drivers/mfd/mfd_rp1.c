/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_rp1

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/math/ilog2.h>
#include <zephyr/sys/device_mmio.h>

struct mfd_rp1_config {
	const struct device *pci_dev;
	uintptr_t cfg_addr;
	size_t cfg_size;
};

static int mfd_rp1_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* TODO: POST_KERNEL is set to use printk, revert this after the development is done */
#define MFD_RP1_INIT(n)                                                                            \
	static struct mfd_rp1_data mfd_rp1_data_##n;                                               \
                                                                                                   \
	static const struct mfd_rp1_config mfd_rp1_cfg_##n = {                                     \
		.pci_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                               \
		.cfg_addr = DT_INST_REG_ADDR(n),                                                   \
		.cfg_size = DT_INST_REG_SIZE(n),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mfd_rp1_init, NULL, &mfd_rp1_data_##n, &mfd_rp1_cfg_##n,          \
			      POST_KERNEL, 98, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_RP1_INIT)
