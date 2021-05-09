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

#include <drivers/virtualization/ivshmem.h>
#include "virt_ivshmem.h"

#ifdef CONFIG_IVSHMEM_DOORBELL

static void ivshmem_doorbell(const void *arg)
{
	const struct ivshmem_param *param = arg;

	LOG_DBG("Interrupt received on vector %u", param->vector);

	if (param->signal != NULL) {
		k_poll_signal_raise(param->signal, param->vector);
	}
}

static bool ivshmem_configure_interrupts(const struct device *dev)
{
	struct ivshmem *data = dev->data;
	bool ret = false;
	uint8_t n_vectors;
	uint32_t key;
	int i;

	key = irq_lock();

	n_vectors = pcie_msi_vectors_allocate(data->bdf,
					      CONFIG_IVSHMEM_INT_PRIORITY,
					      data->vectors,
					      CONFIG_IVSHMEM_MSI_X_VECTORS);
	if (n_vectors == 0) {
		LOG_ERR("Could not allocate %u MSI-X vectors",
			CONFIG_IVSHMEM_MSI_X_VECTORS);
		goto out;
	}

	LOG_DBG("Allocated %u vectors", n_vectors);

	for (i = 0; i < n_vectors; i++) {
		data->params[i].dev = dev;
		data->params[i].vector = i;

		if (!pcie_msi_vector_connect(data->bdf,
					     &data->vectors[i],
					     ivshmem_doorbell,
					     &data->params[i], 0)) {
			LOG_ERR("Failed to connect MSI-X vector %u", i);
			goto out;
		}
	}

	LOG_DBG("%u MSI-X Vectors connected", n_vectors);

	if (!pcie_msi_enable(data->bdf, data->vectors, n_vectors)) {
		LOG_ERR("Could not enable MSI-X");
		goto out;
	}

	data->n_vectors = n_vectors;
	ret = true;

	LOG_DBG("MSI-X configured");
out:
	irq_unlock(key);

	return ret;
}

static void register_signal(const struct device *dev,
			    struct k_poll_signal *signal,
			    uint16_t vector)
{
	struct ivshmem *data = dev->data;

	data->params[vector].signal = signal;
}

#else

static const struct ivshmem_reg no_reg;

#define ivshmem_configure_interrupts(...) true
#define register_signal(...)

#endif /* CONFIG_IVSHMEM_DOORBELL */

static bool ivshmem_configure(const struct device *dev)
{
	struct ivshmem *data = dev->data;
	struct pcie_mbar mbar_regs, mbar_mem;

	if (!pcie_get_mbar(data->bdf, IVSHMEM_PCIE_REG_BAR_IDX, &mbar_regs)) {
#ifdef CONFIG_IVSHMEM_DOORBELL
		LOG_ERR("ivshmem regs bar not found");
		return false;
#else
		LOG_DBG("ivshmem regs bar not found");
		device_map(DEVICE_MMIO_RAM_PTR(dev), (uintptr_t)&no_reg,
			   sizeof(struct ivshmem_reg), K_MEM_CACHE_NONE);
#endif /* CONFIG_IVSHMEM_DOORBELL */
	} else {
		pcie_set_cmd(data->bdf, PCIE_CONF_CMDSTAT_MEM, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar_regs.phys_addr,
			   mbar_regs.size, K_MEM_CACHE_NONE);
	}

	if (!pcie_get_mbar(data->bdf, IVSHMEM_PCIE_SHMEM_BAR_IDX, &mbar_mem)) {
		LOG_ERR("ivshmem mem bar not found");
		return false;
	}

	data->size = mbar_mem.size;

	z_phys_map((uint8_t **)&data->shmem,
		   mbar_mem.phys_addr, data->size,
		   K_MEM_CACHE_WB | K_MEM_PERM_RW | K_MEM_PERM_USER);

	LOG_DBG("ivshmem configured:");
	LOG_DBG("- Registers at 0x%lx (mapped to 0x%lx)",
		mbar_regs.phys_addr, DEVICE_MMIO_GET(dev));
	LOG_DBG("- Shared memory of %lu bytes at 0x%lx (mapped to 0x%lx)",
		data->size, mbar_mem.phys_addr, data->shmem);

	return ivshmem_configure_interrupts(dev);
}

static size_t ivshmem_api_get_mem(const struct device *dev,
				  uintptr_t *memmap)
{
	struct ivshmem *data = dev->data;

	*memmap = data->shmem;

	return data->size;
}

static uint32_t ivshmem_api_get_id(const struct device *dev)
{
	struct ivshmem_reg *regs = (struct ivshmem_reg *)DEVICE_MMIO_GET(dev);

	return regs->iv_position;
}

static uint16_t ivshmem_api_get_vectors(const struct device *dev)
{
#if CONFIG_IVSHMEM_DOORBELL
	struct ivshmem *data = dev->data;

	return data->n_vectors;
#else
	return 0;
#endif
}

static int ivshmem_api_int_peer(const struct device *dev,
				uint32_t peer_id, uint16_t vector)
{
#if CONFIG_IVSHMEM_DOORBELL
	struct ivshmem_reg *regs = (struct ivshmem_reg *)DEVICE_MMIO_GET(dev);
	struct ivshmem *data = dev->data;
	uint32_t doorbell;

	if (vector >= data->n_vectors) {
		return -EINVAL;
	}

	doorbell = IVSHMEM_GEN_DOORBELL(peer_id, vector);
	regs->doorbell = doorbell;

	return 0;
#else
	return -ENOSYS;
#endif
}

static int ivshmem_api_register_handler(const struct device *dev,
					struct k_poll_signal *signal,
					uint16_t vector)
{
#if CONFIG_IVSHMEM_DOORBELL
	struct ivshmem *data = dev->data;

	if (vector >= data->n_vectors) {
		return -EINVAL;
	}

	register_signal(dev, signal, vector);

	return 0;
#else
	return -ENOSYS;
#endif
}

static const struct ivshmem_driver_api ivshmem_api = {
	.get_mem = ivshmem_api_get_mem,
	.get_id = ivshmem_api_get_id,
	.get_vectors = ivshmem_api_get_vectors,
	.int_peer = ivshmem_api_int_peer,
	.register_handler = ivshmem_api_register_handler
};

static int ivshmem_init(const struct device *dev)
{
	struct ivshmem *data = dev->data;

	data->bdf = pcie_bdf_lookup(PCIE_ID(IVSHMEM_VENDOR_ID,
					    IVSHMEM_DEVICE_ID));
	if (data->bdf == PCIE_BDF_NONE) {
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
