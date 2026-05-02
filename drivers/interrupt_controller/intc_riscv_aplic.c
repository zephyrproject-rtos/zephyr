/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT riscv_aplic

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/logging/log.h>
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

	LOG_DBG("APLIC DOMAINCFG: wrote 0x%08x", v);

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
	/* Direct delivery mode - just enable the domain */
	riscv_aplic_domain_enable(dev, true);
	return 0;
#endif
}

#define APLIC_INIT(inst)                                                                           \
	static struct aplic_data aplic_data_##inst;                                                \
	static const struct aplic_cfg aplic_cfg_##inst = {                                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.num_sources = DT_INST_PROP(inst, riscv_num_sources),                              \
		IF_ENABLED(CONFIG_RISCV_APLIC_MSI,                                                 \
			   (.imsic_addr = DT_REG_ADDR(DT_INST_PHANDLE(inst, msi_parent)),))        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, aplic_init, NULL, &aplic_data_##inst, &aplic_cfg_##inst,       \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(APLIC_INIT)

/* Global device pointer for easy access */
static const struct device *aplic_device;

/* Zephyr integration functions */
const struct device *riscv_aplic_get_dev(void)
{
	if (!aplic_device) {
		aplic_device = DEVICE_DT_GET_ANY(riscv_aplic);
	}
	return aplic_device;
}

uint32_t riscv_aplic_get_num_sources(const struct device *dev)
{
	if (!dev) {
		return 0;
	}
	const struct aplic_cfg *cfg = dev->config;

	return cfg->num_sources;
}
