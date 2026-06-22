/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT riscv_aplic

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#ifdef CONFIG_RISCV_APLIC_DIRECT
#include <zephyr/drivers/interrupt_controller/riscv_aplic_direct.h>
#endif
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/util.h>

#include "intc_riscv_aplic_priv.h"

LOG_MODULE_REGISTER(intc_riscv_aplic, CONFIG_LOG_DEFAULT_LEVEL);

int riscv_aplic_domain_enable(const struct device *dev, bool enable)
{
	const struct aplic_cfg *cfg = dev->config;
	struct aplic_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t v = rd32(cfg->base, APLIC_DOMAINCFG);

	if (enable) {
		/* Enable domain; keep delivery mode as configured by platform.
		 * Do not modify DM bit - respect hardware/props configuration.
		 */
		v |= APLIC_DOMAINCFG_IE;
	} else {
		v &= ~APLIC_DOMAINCFG_IE;
	}
	wr32(cfg->base, APLIC_DOMAINCFG, v);

	k_spin_unlock(&data->lock, key);

	return 0;
}

int riscv_aplic_config_src(const struct device *dev, unsigned int src, unsigned int sm)
{
	const struct aplic_cfg *cfg = dev->config;
	struct aplic_data *data = dev->data;

	if (src == 0 || src > cfg->num_sources) {
		return -EINVAL;
	}
	/* Validate sm parameter - 0x2 and 0x3 are reserved (RISC-V AIA spec, section 4.5.2) */
	if (sm == 0x2 || sm == 0x3) {
		return -EINVAL;
	}
	uintptr_t off = aplic_sourcecfg_off(src);
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t v = rd32(cfg->base, off);

	v &= ~APLIC_SOURCECFG_SM_MASK; /* Clear source mode field */
	v |= (sm & APLIC_SOURCECFG_SM_MASK);
	wr32(cfg->base, off, v);
	k_spin_unlock(&data->lock, key);
	return 0;
}

int riscv_aplic_enable_src(const struct device *dev, unsigned int src, bool enable)
{
	const struct aplic_cfg *cfg = dev->config;

	if (src == 0 || src > cfg->num_sources) {
		return -EINVAL;
	}

	wr32(cfg->base, enable ? APLIC_SETIENUM : APLIC_CLRIENUM, src);
	return 0;
}

static int aplic_init(const struct device *dev)
{
#ifdef CONFIG_RISCV_APLIC_MSI
	return aplic_msi_init(dev);
#else
	return aplic_direct_init(dev);
#endif
}

uint32_t riscv_aplic_get_num_sources(const struct device *dev)
{
	const struct aplic_cfg *cfg = dev->config;

	return cfg->num_sources;
}

#ifdef CONFIG_RISCV_APLIC_DIRECT
/* With direct delivery mode enabled the APLIC must register an IRQ handler
 * on initialization
 */
#define APLIC_INTC_IRQ_FUNC_DECLARE(inst) static void aplic_irq_config_func_##inst(void)
#define APLIC_INTC_IRQ_FUNC_DEFINE(inst)                                                           \
	static void aplic_irq_config_func_##inst(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), 0, aplic_irq_handler, DEVICE_DT_INST_GET(inst),    \
			    0);                                                                    \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define APLIC_INIT(inst)                                                                           \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, max_prio),                                        \
		     "max_prio is required for APLIC direct-delivery mode but is not present");    \
	IRQ_PARENT_ENTRY_DEFINE(aplic##inst, DEVICE_DT_INST_GET(inst), DT_INST_IRQN(inst),         \
				INTC_INST_ISR_TBL_OFFSET(inst),                                    \
				DT_INST_INTC_GET_AGGREGATOR_LEVEL(inst));                          \
	APLIC_INTC_IRQ_FUNC_DECLARE(inst);                                                         \
	static struct aplic_data aplic_data_##inst;                                                \
	static const struct aplic_cfg aplic_cfg_##inst = {                                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.num_sources = DT_INST_PROP(inst, riscv_num_sources),                              \
		.max_prio = DT_INST_PROP(inst, riscv_max_priority),                                \
		.irq_config_func = aplic_irq_config_func_##inst,                                   \
		.isr_table = &_sw_isr_table[INTC_INST_ISR_TBL_OFFSET(inst)],                       \
	};                                                                                         \
	APLIC_INTC_IRQ_FUNC_DEFINE(inst)                                                           \
	DEVICE_DT_INST_DEFINE(inst, aplic_init, NULL, &aplic_data_##inst, &aplic_cfg_##inst,       \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
#else
/* APLIC_MSI interrupts are serviced by separate IMSIC driver */
#define APLIC_INIT(inst)                                                                           \
	static struct aplic_data aplic_data_##inst;                                                \
	static const struct aplic_cfg aplic_cfg_##inst = {                                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.num_sources = DT_INST_PROP(inst, riscv_num_sources),                              \
		IF_ENABLED(CONFIG_RISCV_APLIC_MSI,                                                 \
			(.imsic_addr = DT_REG_ADDR(DT_INST_PHANDLE(inst, msi_parent)),)) };        \
	DEVICE_DT_INST_DEFINE(inst, aplic_init, NULL, &aplic_data_##inst, &aplic_cfg_##inst,       \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
#endif

DT_INST_FOREACH_STATUS_OKAY(APLIC_INIT)
