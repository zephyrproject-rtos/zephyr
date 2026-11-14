/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstddef>
#define DT_DRV_COMPAT qemu_ivshmem_flat

#define LOG_LEVEL CONFIG_IVSHMEM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ivshmem_flat);

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>

#include "virt_ivshmem_flat.h"

static size_t ivshmem_flat_get_mem(const struct device *dev, uintptr_t *memmap)
{
	*memmap = DEVICE_MMIO_NAMED_GET(dev, shmem);

	return DEV_CFG(dev)->shmem_size;
}

static uint32_t ivshmem_flat_get_id(const struct device *dev)
{
	return sys_le32_to_cpu(
		sys_read32(DEVICE_MMIO_NAMED_GET(dev, mmr) + IVSHMEM_FLAT_IVPOSITION));
}

static uint16_t ivshmem_flat_get_vectors(const struct device *dev)
{
	return DEV_CFG(dev)->irq_config != NULL ? 1 : 0;
}

static int ivshmem_flat_int_peer(const struct device *dev, uint32_t peer_id, uint16_t vector)
{
	if (!IS_ENABLED(CONFIG_IVSHMEM_DOORBELL)) {
		return -ENOSYS;
	}

	if ((vector != 0) || (peer_id > UINT16_MAX)) {
		return -EINVAL;
	}

	/* Publish shared memory writes before notifying the peer. */
	barrier_dmem_fence_full();
	sys_write32(sys_cpu_to_le32((peer_id << 16) | vector),
		    DEVICE_MMIO_NAMED_GET(dev, mmr) + IVSHMEM_FLAT_DOORBELL);

	return 0;
}

static int ivshmem_flat_register_handler(const struct device *dev, struct k_poll_signal *signal,
					 uint16_t vector)
{
	struct ivshmem_flat_data *data = dev->data;

	if (ivshmem_flat_get_vectors(dev) == 0) {
		return -ENOSYS;
	}
	if (vector >= ivshmem_flat_get_vectors(dev)) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		if (signal != NULL) {
			data->signal = signal;
		}
	}

	return 0;
}

#if defined(CONFIG_IVSHMEM_DOORBELL) && (DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts) ||           \
					 DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts_extended))
static void ivshmem_flat_isr(const struct device *dev)
{
	struct ivshmem_flat_data *data = dev->data;

	/* INTMASK and INTSTATUS are reserved. The single IRQ carries no vector ID. */
	K_SPINLOCK(&data->lock) {
		if (data->signal != NULL) {
			int ret = k_poll_signal_raise(data->signal, 0);

			if (ret != 0) {
				LOG_DBG("Signal raise failed: %d", ret);
			}
		}
	}
}
#endif

static int ivshmem_flat_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, mmr, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, shmem, K_MEM_CACHE_NONE | K_MEM_PERM_USER);

	if (DEV_CFG(dev)->irq_config != NULL) {
		DEV_CFG(dev)->irq_config(dev);
	}

	return 0;
}

static DEVICE_API(ivshmem, ivshmem_flat_api) = {
	.get_mem = ivshmem_flat_get_mem,
	.get_id = ivshmem_flat_get_id,
	.get_vectors = ivshmem_flat_get_vectors,
	.int_peer = ivshmem_flat_int_peer,
	.register_handler = ivshmem_flat_register_handler,
};

#define IVSHMEM_FLAT_DEFINE(n)                                                                     \
	BUILD_ASSERT(DT_NUM_REGS(DT_DRV_INST(n)) == 2, "Two memory regions are required");         \
	BUILD_ASSERT(DT_INST_REG_SIZE_BY_NAME(n, mmr) == IVSHMEM_FLAT_MMR_SIZE,                    \
		     "MMR size must be 0x10");                                                     \
	BUILD_ASSERT(DT_INST_REG_SIZE_BY_NAME(n, shmem) > 0, "Shared memory must not be empty");   \
	BUILD_ASSERT(DT_NUM_IRQS(DT_DRV_INST(n)) <= 1, "Only one IRQ is supported");               \
	IF_ENABLED(IVSHMEM_FLAT_IRQ_ENABLED(n), (                                               \
		static void ivshmem_flat_irq_config_##n(const struct device *dev)               \
		{                                                                               \
			IRQ_CONNECT(DT_INST_IRQN(n),                                             \
				    COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, priority),                \
						(DT_INST_IRQ(n, priority)), (0)),                \
				    ivshmem_flat_isr, DEVICE_DT_INST_GET(n), 0);                  \
			irq_enable(DT_INST_IRQN(n));                                            \
		}                                                                               \
	))  \
	static struct ivshmem_flat_data ivshmem_flat_data_##n;                                     \
	static const struct ivshmem_flat_config ivshmem_flat_config_##n = {                        \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(mmr, DT_DRV_INST(n)),                           \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(shmem, DT_DRV_INST(n)),                         \
		.shmem_size = DT_INST_REG_SIZE_BY_NAME(n, shmem),                                  \
		.irq_config = COND_CODE_1(IVSHMEM_FLAT_IRQ_ENABLED(n),                          \
					 (ivshmem_flat_irq_config_##n), (NULL)),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ivshmem_flat_init, NULL, &ivshmem_flat_data_##n,                  \
			      &ivshmem_flat_config_##n, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ivshmem_flat_api);

DT_INST_FOREACH_STATUS_OKAY(IVSHMEM_FLAT_DEFINE)
