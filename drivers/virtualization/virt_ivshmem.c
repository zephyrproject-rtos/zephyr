/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_IVSHMEM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ivshmem);

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <soc.h>
#include <device.h>
#include <init.h>
#include <drivers/pcie/pcie.h>

#include <drivers/virtualization/ivshmem.h>
#include "virt_ivshmem.h"

static bool ivshmem_check_on_bdf(pcie_bdf_t bdf)
{
	uint32_t data;

	data = pcie_conf_read(bdf, PCIE_CONF_ID);
	if ((data != PCIE_ID_NONE) &&
	    (PCIE_ID_TO_VEND(data) == IVSHMEM_VENDOR_ID) &&
	    (PCIE_ID_TO_DEV(data) == IVSHMEM_DEVICE_ID)) {
		return true;
	}

	return false;
}

/* Ivshmem's BDF is not a static value that we could get from DTS,
 * since the same image could run on qemu or ACRN which could set
 * a different one. So instead, let's find it at runtime.
 */
static pcie_bdf_t ivshmem_bdf_lookup(void)
{
	int bus, dev, func;

	for (bus = 0; bus <= MAX_BUS; bus++) {
		for (dev = 0; dev <= MAX_DEV; ++dev) {
			for (func = 0; func <= MAX_FUNC; ++func) {
				pcie_bdf_t bdf = PCIE_BDF(bus, dev, func);

				if (ivshmem_check_on_bdf(bdf)) {
					return bdf;
				}
			}
		}
	}

	return 0;
}

static bool ivshmem_configure(const struct device *dev)
{
	struct ivshmem *data = dev->data;
	struct pcie_mbar mbar_regs, mbar_mem;

	if (!pcie_get_mbar(data->bdf, IVSHMEM_PCIE_REG_BAR_IDX, &mbar_regs)) {
		LOG_ERR("ivshmem regs bar not found");
		return false;
	}

	pcie_set_cmd(data->bdf, PCIE_CONF_CMDSTAT_MEM, true);

	device_map(DEVICE_MMIO_RAM_PTR(dev), mbar_regs.phys_addr,
		   mbar_regs.size, K_MEM_CACHE_NONE);

	if (!pcie_get_mbar(data->bdf, IVSHMEM_PCIE_SHMEM_BAR_IDX, &mbar_mem)) {
		LOG_ERR("ivshmem mem bar not found");
		return false;
	}

	data->size = mbar_mem.size;

	z_phys_map((uint8_t **)&data->shmem,
		   mbar_mem.phys_addr, data->size,
		   K_MEM_CACHE_WB | K_MEM_PERM_RW);

	LOG_DBG("ivshmem configured:");
	LOG_DBG("- Registers at 0x%lx (mapped to 0x%lx)",
		mbar_regs.phys_addr, DEVICE_MMIO_GET(dev));
	LOG_DBG("- Shared memory of %lu bytes at 0x%lx (mapped to 0x%lx)",
		data->size, mbar_mem.phys_addr, data->shmem);

	return true;
}

static size_t ivshmem_api_get_mem(const struct device *dev,
				  uintptr_t *memmap)
{
	struct ivshmem *data = dev->data;

	*memmap = data->shmem;

	return data->size;
}

static const struct ivshmem_driver_api ivshmem_api = {
	.get_mem = ivshmem_api_get_mem,
};

static int ivshmem_init(const struct device *dev)
{
	struct ivshmem *data = dev->data;

	data->bdf = ivshmem_bdf_lookup();
	if (data->bdf == 0) {
		LOG_WRN("ivshmem device not found");
		return -ENOTSUP;
	}

	LOG_DBG("ivshmem found at bdf 0x%x", data->bdf);

	if (!ivshmem_configure(dev)) {
		return -EIO;
	}

	return 0;
}

static struct ivshmem ivshmem_data;

DEVICE_DEFINE(ivshmem, CONFIG_IVSHMEM_DEV_NAME,
	      ivshmem_init, device_pm_control_nop, &ivshmem_data, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ivshmem_api);
