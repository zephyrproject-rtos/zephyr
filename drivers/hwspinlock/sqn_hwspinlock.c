/*
 * Copyright (c) 2025 Sequans Communications
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sqn_hwspinlock

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/hwspinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sqn_hwspinlock, CONFIG_HWSPINLOCK_INIT_PRIORITY);

#include <soc.h>

/*
 * HWSPINLOCK mechanism:
 * When a hwspinlock is unlocked, its register value is 0.
 * To claim the spinlock, the user write its cluster id.
 * To release it, the user write its cluster id again.
 * Trying to write another value does not update the value of the register.
 * To check that the user holds the lock, it reads the register and compare the value with its
 * cluster id.
 */

struct sqn_hwspinlock_data {
	DEVICE_MMIO_RAM;
};

struct sqn_hwspinlock_config {
	DEVICE_MMIO_ROM;
	uint16_t num_locks;
	uint16_t reg_width;
};

#define DEV_DATA(dev) ((struct sqn_hwspinlock_data *)dev->data)
#define DEV_CFG(dev)  ((const struct sqn_hwspinlock_config *)dev->config)

/*
 * CURRENT_CLUSTER value starts at 0. Because 0 is a reserved value for the register, increment all
 * ids by 1.
 */
#define HWSPINLOCK_CLUSTER_ID (CURRENT_CLUSTER + 1)

static inline mem_addr_t get_lock_addr(const struct device *dev, uint32_t id)
{
	return (mem_addr_t)(DEVICE_MMIO_GET(dev) + id * DEV_CFG(dev)->reg_width);
}

static int sqn_hwspinlock_trylock(const struct device *dev, uint32_t id)
{
	mem_addr_t lock_addr = get_lock_addr(dev, id);

	__ASSERT(id < DEV_CFG(dev)->num_locks, "Invalid hwspinlock id");
	__ASSERT(sys_read8(lock_addr) != HWSPINLOCK_CLUSTER_ID,
		 "Tried to lock hwspinlock already locked by this cluster");

	sys_write8(HWSPINLOCK_CLUSTER_ID, lock_addr);
	if (sys_read8(lock_addr) == HWSPINLOCK_CLUSTER_ID) {
		return 0;
	}

	return -EBUSY;
}

static void sqn_hwspinlock_lock(const struct device *dev, uint32_t id)
{
	mem_addr_t lock_addr = get_lock_addr(dev, id);

	__ASSERT(id < DEV_CFG(dev)->num_locks, "Invalid hwspinlock id");
	__ASSERT(sys_read8(lock_addr) != HWSPINLOCK_CLUSTER_ID,
		 "Tried to lock hwspinlock already locked by this cluster");

	sys_write8(HWSPINLOCK_CLUSTER_ID, lock_addr);
	while (sys_read8(lock_addr) != HWSPINLOCK_CLUSTER_ID) {
		k_busy_wait(CONFIG_SQN_HWSPINLOCK_RELAX_TIME);
		sys_write8(HWSPINLOCK_CLUSTER_ID, lock_addr);
	}
}

static void sqn_hwspinlock_unlock(const struct device *dev, uint32_t id)
{
	mem_addr_t lock_addr = get_lock_addr(dev, id);

	__ASSERT(id < DEV_CFG(dev)->num_locks, "Invalid hwspinlock id");
	__ASSERT(sys_read8(lock_addr) == HWSPINLOCK_CLUSTER_ID,
		 "Tried to unlock hwspinlock not locked by this cluster");

	/* Unlock by writing our own cluster id again */
	sys_write8(HWSPINLOCK_CLUSTER_ID, lock_addr);
}

static uint32_t sqn_hwspinlock_get_max_id(const struct device *dev)
{
	return DEV_CFG(dev)->num_locks;
}

static DEVICE_API(hwspinlock, sqn_hwspinlock_api) = {
	.trylock = sqn_hwspinlock_trylock,
	.lock = sqn_hwspinlock_lock,
	.unlock = sqn_hwspinlock_unlock,
	.get_max_id = sqn_hwspinlock_get_max_id,
};

static int sqn_hwspinlock_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

#define SQN_HWSPINLOCK_INIT(idx)                                                                   \
	static struct sqn_hwspinlock_data sqn_hwspinlock##idx##_data;                              \
	static const struct sqn_hwspinlock_config sqn_hwspinlock##idx##_config = {                 \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(idx)),                                            \
		.reg_width = DT_INST_PROP_OR(idx, reg_width, 1),                                   \
		.num_locks = DT_INST_PROP(idx, num_locks),                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, sqn_hwspinlock_init, NULL, &sqn_hwspinlock##idx##_data,         \
			      &sqn_hwspinlock##idx##_config, PRE_KERNEL_1,                         \
			      CONFIG_HWSPINLOCK_INIT_PRIORITY, &sqn_hwspinlock_api)

DT_INST_FOREACH_STATUS_OKAY(SQN_HWSPINLOCK_INIT);
