/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic_direct.h>
#include <zephyr/arch/riscv/icsr.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>

#include "sw_isr_common.h"
#include "intc_riscv_aplic_priv.h"

LOG_MODULE_REGISTER(intc_riscv_aplic_direct, CONFIG_LOG_DEFAULT_LEVEL);

/* APLIC registers are 32-bit memory-mapped */
#define APLIC_REG_SIZE 32
#define APLIC_REG_MASK BIT_MASK(LOG2(APLIC_REG_SIZE))

static uint32_t save_irq[CONFIG_MP_MAX_NUM_CPUS];
static const struct device *save_dev[CONFIG_MP_MAX_NUM_CPUS];

static inline uint32_t local_irq_to_reg_index(uint32_t local_irq)
{
	return local_irq >> LOG2(APLIC_REG_SIZE);
}

static inline uint32_t local_irq_to_reg_offset(uint32_t local_irq)
{
	return local_irq_to_reg_index(local_irq) * sizeof(uint32_t);
}

static inline uint32_t local_irq_to_reg_bitpos(uint32_t local_irq)
{
	return 1U << (local_irq & APLIC_REG_MASK);
}

static inline uint32_t hart_and_prio_to_target(uint32_t hart_id, uint32_t prio)
{
	return (hart_id << APLIC_TARGET_HART_SHIFT) | (prio & APLIC_IPRIO_MASK);
}

int riscv_aplic_direct_mode_enable(const struct device *dev, bool enable)
{
	const struct aplic_cfg *cfg = dev->config;
	struct aplic_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t v = rd32(cfg->base, APLIC_DOMAINCFG);

	if (enable) {
		/* Enable direct delivery mode (DM field = 0) */
		v &= ~APLIC_DOMAINCFG_DM;
	} else {
		v |= APLIC_DOMAINCFG_DM;
	}
	wr32(cfg->base, APLIC_DOMAINCFG, v);

	k_spin_unlock(&data->lock, key);

	return 0;
}

int riscv_aplic_is_enabled(uint32_t local_irq)
{
	const struct device *dev = riscv_aplic_get_dev();
	const struct aplic_cfg *cfg = dev->config;

	if (local_irq == 0 || local_irq > cfg->num_sources) {
		return 0;
	}

	const uint32_t setie_offset = APLIC_SETIE_BASE + local_irq_to_reg_offset(local_irq);
	const uint32_t setie_value = rd32(cfg->base, setie_offset);

	return !!(setie_value & local_irq_to_reg_bitpos(local_irq));
}

int riscv_aplic_set_priority(const struct device *dev, uint32_t local_irq, uint32_t prio)
{
	const struct aplic_cfg *cfg = dev->config;
	struct aplic_data *data = dev->data;

	if (local_irq == 0U || local_irq > cfg->num_sources) {
		return -EINVAL;
	}

	uint32_t target_offset = aplic_target_off(local_irq);

	if (prio > cfg->max_prio) {
		LOG_WRN("AIA-APLIC-Direct: Invalid priority specified (irq %u, prio %u, max_prio "
			"%u)",
			local_irq, prio, cfg->max_prio);
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t hart_id = rd32(cfg->base, target_offset) >> APLIC_TARGET_HART_SHIFT;

	if ((hart_id >= arch_num_cpus()) || !IS_ENABLED(CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY)) {
		/* Use Hart 0 if affinity configuration is not supported or improperly configured.
		 * Otherwise, respect affinity currently configured in target register
		 */
		hart_id = 0U;
	}

	wr32(cfg->base, target_offset, hart_and_prio_to_target(hart_id, prio));

	k_spin_unlock(&data->lock, key);

	return 0;
}

#if defined(CONFIG_RISCV_APLIC_DIRECT_IRQ_AFFINITY)
int riscv_aplic_irq_set_affinity(const struct device *dev, uint32_t local_irq, uint32_t hart_id)
{
	const struct aplic_cfg *cfg = dev->config;
	struct aplic_data *data = dev->data;

	if ((local_irq == 0) || (local_irq > cfg->num_sources)) {
		return -EINVAL;
	}

	if (hart_id >= arch_num_cpus()) {
		return -EINVAL;
	}

	uint32_t target_offset = aplic_target_off(local_irq);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	const uint32_t prio = rd32(cfg->base, target_offset) & APLIC_IPRIO_MASK;

	wr32(cfg->base, target_offset, hart_and_prio_to_target(hart_id, prio));

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif

unsigned int riscv_aplic_get_saved_irq(void)
{
	return save_irq[arch_curr_cpu()->id];
}

const struct device *riscv_aplic_get_saved_dev(void)
{
	return save_dev[arch_curr_cpu()->id];
}

void aplic_irq_handler(const struct device *dev)
{
	const struct aplic_cfg *cfg = dev->config;
	const struct _isr_table_entry *ite;

	/* Claim the interrupt - Clears pending bit if possible */
	const uint32_t claimi_offset = aplic_claimi_off(arch_proc_id());
	const uint32_t local_irq = rd32(cfg->base, claimi_offset) >> APLIC_INTERRUPT_IDENTITY_SHIFT;

	/*
	 * Save IRQ in save_irq. To be used, if need be, by
	 * subsequent handlers registered in the _sw_isr_table table,
	 * as IRQ number held by the claim_complete register is
	 * cleared upon read.
	 */
	save_irq[arch_curr_cpu()->id] = local_irq;
	save_dev[arch_curr_cpu()->id] = dev;

	/*
	 * If the IRQ is out of range, call z_irq_spurious.
	 * A call to z_irq_spurious will not return.
	 */
	if (local_irq == 0U || local_irq > cfg->num_sources) {
		/* A call to z_irq_spurious will not return. */
		z_irq_spurious(NULL);
	}

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = &cfg->isr_table[local_irq];
	ite->isr(ite->arg);
}

int aplic_direct_init(const struct device *dev)
{
	const struct aplic_cfg *cfg = dev->config;

	riscv_aplic_domain_enable(dev, true);

	/* Explicitly enable direct delivery mode */
	riscv_aplic_direct_mode_enable(dev, true);

	/* Sources and Targets, Set all interrupt source modes as Inactive
	 * Source[0] is invalid, first register corresponds to Source[1]
	 */
	for (uint32_t src_num = 1; src_num <= cfg->num_sources; src_num++) {
		/* Sources -- Delegated = 0, Source Mode = 0 (Inactive)
		 * Targets -- When source[i] is set as inactive, target[i] is RAZ/WI
		 * En/Pend -- When source[i] is inactive, interrupt pending and enable bits are also
		 *            RAZ/WI
		 */
		wr32(cfg->base, aplic_sourcecfg_off(src_num), APLIC_SM_INACTIVE);
	}

	for (uint32_t cpu = 0; cpu < arch_num_cpus(); cpu++) {
		/* idelivery = 0b1 -- Interrupt delivery is enabled */
		wr32(cfg->base, aplic_idelivery_off(cpu), APLIC_IDC_IDELIVERY_ENABLE);
		/* ithreshold = 0b0 -- All enabled sources can signal */
		wr32(cfg->base, aplic_ithreshold_off(cpu), APLIC_IDC_ITHRESHOLD);
	}

	/* Configure IRQ for APLIC driver */
	cfg->irq_config_func();

	return 0;
}
