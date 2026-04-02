/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwspinlock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hwspinlock_nxp_sema42, CONFIG_HWSPINLOCK_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_sema42

struct nxp_sema42_config {
	DEVICE_MMIO_ROM;
	uint8_t domain_id;
	uint8_t num_locks;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

/* SEMA42 gate n register address.
 *
 * The SEMA42 gates are sorted in the order 3, 2, 1, 0, 7, 6, 5, 4, ... not in the order
 * 0, 1, 2, 3, 4, 5, 6, 7, ... The macro SEMA42_GATEn gets the SEMA42 gate based on the
 * gate index.
 *
 * The input gate index is XOR'ed with 3U:
 * 0 ^ 3 = 3
 * 1 ^ 3 = 2
 * 2 ^ 3 = 1
 * 3 ^ 3 = 0
 * 4 ^ 3 = 7
 * 5 ^ 3 = 6
 * 6 ^ 3 = 5
 * 7 ^ 3 = 4
 * ...
 */
static inline mem_addr_t nxp_sema42_gate_addr(const struct device *dev, uint32_t id)
{
	return (mem_addr_t)(DEVICE_MMIO_GET(dev) + ((id ^ 3U) & 0x0FU));
}

static inline uint8_t nxp_sema42_lock_value(const struct device *dev)
{
	/* HW expects (domain_id + 1), where domain_id is 0..14 */
	const struct nxp_sema42_config *cfg = (const struct nxp_sema42_config *)dev->config;

	return (uint8_t)(cfg->domain_id + 1U);
}

static int nxp_sema42_trylock(const struct device *dev, uint32_t id)
{
	const uint8_t lock_val = nxp_sema42_lock_value(dev);
	mem_addr_t gate_addr;
	uint8_t gate_val;

	gate_addr = nxp_sema42_gate_addr(dev, id);

	/* Attempt to lock: write lock_val then read back to verify */
	sys_write8(lock_val, gate_addr);
	gate_val = sys_read8(gate_addr) & 0x0FU;

	return (gate_val == lock_val) ? 0 : -EBUSY;
}

static void nxp_sema42_lock(const struct device *dev, uint32_t id)
{
	const uint8_t lock_val = nxp_sema42_lock_value(dev);
	mem_addr_t gate_addr;
	uint8_t gate_val;

	gate_addr = nxp_sema42_gate_addr(dev, id);

	while (true) {
		sys_write8(lock_val, gate_addr);
		gate_val = sys_read8(gate_addr) & 0x0FU;

		if (gate_val == lock_val) {
			return;
		}

		arch_spin_relax();
	}
}

static void nxp_sema42_unlock(const struct device *dev, uint32_t id)
{
	mem_addr_t gate_addr;

	gate_addr = nxp_sema42_gate_addr(dev, id);

	/* Unlock by writing 0 */
	sys_write8(0U, gate_addr);
}

static uint32_t nxp_sema42_get_max_id(const struct device *dev)
{
	const struct nxp_sema42_config *cfg = (const struct nxp_sema42_config *)dev->config;

	return (uint32_t)(cfg->num_locks - 1U);
}

static int nxp_sema42_init(const struct device *dev)
{
	const struct nxp_sema42_config *cfg = (const struct nxp_sema42_config *)dev->config;

	if (cfg->clock_dev != NULL) {
		if (!device_is_ready(cfg->clock_dev)) {
			return -ENODEV;
		}

		int ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);

		if (ret) {
			LOG_ERR("Device clock turn on failed");
			return ret;
		}
	}

	/* Do not reset/clear gates here.
	 *
	 * In multi-core (multi-domain) systems, this hwspinlock device can be used to
	 * synchronize between cores/domains. Unconditionally clearing all gates during
	 * init on every core can break another core's active locks.
	 *
	 * Hardware reset (POR/system reset) is expected to initialize gates to 0.
	 */

	return 0;
}

static DEVICE_API(hwspinlock, nxp_sema42_api) = {
	.trylock = nxp_sema42_trylock,
	.lock = nxp_sema42_lock,
	.unlock = nxp_sema42_unlock,
	.get_max_id = nxp_sema42_get_max_id,
};

#define NXP_SEMA42_HWSPINLOCK_INIT(inst)							\
												\
	static const struct nxp_sema42_config _CONCAT(nxp_sema42_config, inst) = {		\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),					\
		.domain_id = DT_INST_PROP(inst, domain_id),					\
		.num_locks = DT_INST_PROP_OR(inst, num_locks, 16),				\
		.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks),			\
				(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))), (NULL)),		\
		.clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks),		\
				((clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name)),	\
				((clock_control_subsys_t)0U)),					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, nxp_sema42_init, NULL, NULL,				\
			      &_CONCAT(nxp_sema42_config, inst), PRE_KERNEL_1,			\
			      CONFIG_HWSPINLOCK_INIT_PRIORITY, &nxp_sema42_api)

DT_INST_FOREACH_STATUS_OKAY(NXP_SEMA42_HWSPINLOCK_INIT);
