/**
 * clock_control_ra2_root_osc.c
 *
 * RA2 fixed frequency clocks driver implementation
 *
 * Copyright (c) 2023-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include "clock_control_ra2_priv.h"

#define DT_DRV_COMPAT renesas_ra2_root_osc

/* Common for all root oscillators */
#define CGC_CCR_STP	BIT(0)

struct ra_root_osc_cfg {
	/* Must be first */
	struct ra_common_osc_config common;
	mm_reg_t	base;
	uint32_t	freq;
	uint32_t	stab_delay;
};

static int ra_root_osc_driver_api_on(const struct device *dev,
				clock_control_subsys_t sys)
{
	int ret = 0;
	const struct ra_root_osc_cfg *const cfg = dev->config;
	struct ra_root_osc_data *dat = dev->data;

	ARG_UNUSED(sys);

	if (sys_read8(cfg->base)) {
		K_SPINLOCK(&dat->lock) {
			uint16_t old_prcr = get_register_protection();

			set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

			sys_write8(0, cfg->base);

			set_register_protection(old_prcr);
		}

		k_busy_wait(cfg->stab_delay);
	}

	return ret;
}

static int ra_root_osc_driver_api_off(const struct device *dev,
				clock_control_subsys_t sys)
{
	const struct ra_root_osc_cfg * const cfg = dev->config;
	struct ra_root_osc_data *dat = dev->data;

	ARG_UNUSED(sys);

	if (!sys_read8(cfg->base)) {
		K_SPINLOCK(&dat->lock) {
			uint16_t old_prcr = get_register_protection();

			set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

			sys_write8(CGC_CCR_STP, cfg->base);

			set_register_protection(old_prcr);
		}
	}

	return 0;
}

static int ra_root_osc_driver_api_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	ARG_UNUSED(sys);

	if (rate) {
		const struct ra_root_osc_cfg *cfg = dev->config;

		*rate = cfg->freq;
		return 0;
	}

	return -EINVAL;
}

static enum clock_control_status ra_root_osc_driver_api_get_status(
		const struct device *dev, clock_control_subsys_t sys)
{
	const struct ra_root_osc_cfg *cfg = dev->config;

	ARG_UNUSED(sys);

	return sys_read8(cfg->base) ? CLOCK_CONTROL_STATUS_OFF :
					CLOCK_CONTROL_STATUS_ON;
}

/* Switch off unused clocks (ie with 'disabled' status)
 * Since the device objects for the disabled nodes don't exist,
 * disable them explicitly here.
 */
void disable_unused_root_osc(void)
{
	uint16_t old_prcr = get_register_protection();

	set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

	if (!DT_NODE_HAS_STATUS(DT_NODELABEL(hoco), okay)) {
		sys_write8(CGC_HOCOCR_HCSTP, CGC_HOCOCR);
	}

	if (!DT_NODE_HAS_STATUS(DT_NODELABEL(loco), okay)) {
		sys_write8(CGC_LOCOCR_LCSTP, CGC_LOCOCR);
	}

	if (!DT_NODE_HAS_STATUS(DT_NODELABEL(moco), okay)) {
		sys_write8(CGC_MOCOCR_MCSTP, CGC_MOCOCR);
	}

	if (!DT_NODE_HAS_STATUS(DT_NODELABEL(mosc), okay)) {
		sys_write8(CGC_MOSCCR_MOSTP, CGC_MOSCCR);
	}

	if (!DT_NODE_HAS_STATUS(DT_NODELABEL(sosc), okay)) {
		sys_write8(CGC_SOSCCR_SOSTP, CGC_SOSCCR);
	}

	set_register_protection(old_prcr);
}

static const struct clock_control_driver_api ra_root_osc_driver_api = {
	.on = ra_root_osc_driver_api_on,
	.off = ra_root_osc_driver_api_off,
	.get_rate = ra_root_osc_driver_api_get_rate,
	.get_status = ra_root_osc_driver_api_get_status,
};

#define RA_ROOT_OSC_INIT(inst)						       \
	static const struct ra_root_osc_cfg ra_root_osc_cfg##inst = {	       \
		.base = DT_INST_REG_ADDR_BY_NAME(inst, cr),		       \
		.freq = DT_INST_PROP(inst, clock_frequency),		       \
		.stab_delay = DT_INST_PROP(inst, stabilisation_time),	       \
		.common.id = DT_INST_REG_ADDR_BY_NAME(inst, id),	       \
	};								       \
									       \
	static struct ra_root_osc_data ra_root_osc_data##inst;		       \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
		NULL,							       \
		NULL, /* TODO */					       \
		&ra_root_osc_data##inst,				       \
		&ra_root_osc_cfg##inst,					       \
		PRE_KERNEL_1,						       \
		CONFIG_CLOCK_CONTROL_INIT_PRIORITY,			       \
		&ra_root_osc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RA_ROOT_OSC_INIT)
