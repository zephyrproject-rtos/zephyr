/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwspinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hwspinlock_k3, CONFIG_HWSPINLOCK_LOG_LEVEL);

#define DT_DRV_COMPAT ti_k3_hwspinlock

/* TRM SPINLOCK_* offsets from module base */
#define K3_SPINLOCK_SYSTATUS    0x0014U
#define K3_SPINLOCK_LOCK_REG(i) (0x0800U + ((uint32_t)(i) * 4U))

/* SYSTATUS.NUM_LOCKS is encoded as (lock_count / 32) in bits 31:24 */
#define K3_SPINLOCK_SYSTATUS_NUM_SHIFT 24U
#define K3_SPINLOCK_SYSTATUS_NUM_MASK  0xffU

/* LOCK_REG read: 0 = this read acquired the lock; 1 = already taken */
#define K3_SPINLOCK_FREE  0U
#define K3_SPINLOCK_TAKEN 1U

struct k3_hwspinlock_config {
	struct hwspinlock_driver_config common;
	DEVICE_MMIO_NAMED_ROM(base);
};

struct k3_hwspinlock_data {
	DEVICE_MMIO_NAMED_RAM(base);
};

#define DEV_CFG(dev) ((const struct k3_hwspinlock_config *)(dev)->config)

static inline mem_addr_t k3_lock_reg(const struct device *dev, uint32_t id)
{
	return (mem_addr_t)(DEVICE_MMIO_NAMED_GET(dev, base) + K3_SPINLOCK_LOCK_REG(id));
}

static uint32_t k3_hw_lock_count(mem_addr_t base)
{
	uint32_t systatus = sys_read32(base + K3_SPINLOCK_SYSTATUS);
	uint32_t encoded =
		(systatus >> K3_SPINLOCK_SYSTATUS_NUM_SHIFT) & K3_SPINLOCK_SYSTATUS_NUM_MASK;

	return encoded * 32U;
}

static int k3_hwspinlock_trylock(const struct device *dev, uint32_t id)
{
	if (sys_read32(k3_lock_reg(dev, id)) == K3_SPINLOCK_FREE) {
		return 0;
	}

	return -EBUSY;
}

static void k3_hwspinlock_lock(const struct device *dev, uint32_t id)
{
	mem_addr_t reg = k3_lock_reg(dev, id);

	while (sys_read32(reg) != K3_SPINLOCK_FREE) {
		arch_spin_relax();
	}
}

static void k3_hwspinlock_unlock(const struct device *dev, uint32_t id)
{
	/* Writing 0 releases LOCK_REG_j */
	sys_write32(K3_SPINLOCK_FREE, k3_lock_reg(dev, id));
}

static int k3_hwspinlock_init(const struct device *dev)
{
	const struct k3_hwspinlock_config *cfg = DEV_CFG(dev);
	mem_addr_t base;
	uint32_t hw_locks;

	DEVICE_MMIO_NAMED_MAP(dev, base, K_MEM_CACHE_NONE);
	base = (mem_addr_t)DEVICE_MMIO_NAMED_GET(dev, base);

	/*
	 * Do not soft-reset the module, SYSCONFIG.SOFT_RESET clears every
	 * LOCK_REG and would disturb peers that already hold a lock.
	 */
	hw_locks = k3_hw_lock_count(base);
	if (hw_locks == 0U) {
		LOG_ERR("SYSTATUS reported 0 locks (check MMIO mapping/clocks)");
		return -ENODEV;
	}

	if (hw_locks != cfg->common.num_locks) {
		LOG_WRN("num-locks DT (%u) != SYSTATUS-derived (%u)", cfg->common.num_locks,
			hw_locks);
	}

	return 0;
}

static DEVICE_API(hwspinlock, k3_hwspinlock_api) = {
	.trylock = k3_hwspinlock_trylock,
	.lock = k3_hwspinlock_lock,
	.unlock = k3_hwspinlock_unlock,
};

#define K3_HWSPINLOCK_INIT(inst)							\
	HWSPINLOCK_SPINLOCK_ARRAY_DT_INST_DEFINE(inst);					\
	static struct k3_hwspinlock_data k3_hwspinlock_data_##inst;			\
	static const struct k3_hwspinlock_config k3_hwspinlock_config_##inst = {		\
		.common = HWSPINLOCK_COMMON_CONFIG_FROM_DT_INST(inst),			\
		DEVICE_MMIO_NAMED_ROM_INIT(base, DT_DRV_INST(inst)),			\
	};										\
	DEVICE_DT_INST_DEFINE(inst, k3_hwspinlock_init, NULL,				\
			      &k3_hwspinlock_data_##inst, &k3_hwspinlock_config_##inst,	\
			      PRE_KERNEL_1, CONFIG_HWSPINLOCK_INIT_PRIORITY,		\
			      &k3_hwspinlock_api)

DT_INST_FOREACH_STATUS_OKAY(K3_HWSPINLOCK_INIT);
