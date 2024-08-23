/**
 * clock_control_ra2_main_osc.c
 *
 * RA2 Main clock oscillator (MOSC) driver implementation
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include "clock_control_ra2_priv.h"

#define DT_DRV_COMPAT	renesas_ra2_main_osc
#define MOSC_NODE	DT_DRV_INST(0)

struct ra_main_osc_config {
	/* Must be first */
	struct ra_common_osc_config common;
};

static int mosc_driver_api_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct ra_root_osc_data *dat = dev->data;

	ARG_UNUSED(sys);

	if (sys_read8(CGC_MOSCCR) & CGC_MOSCCR_MOSTP) {
		K_SPINLOCK(&dat->lock) {
			uint16_t old_prcr = get_register_protection();

			set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

			sys_write8(0, CGC_MOSCCR);

			set_register_protection(old_prcr);

			while ((sys_read8(CGC_OSCSF) & CGC_OSCSF_MOSCSF) == 0)
				;
		}
	}
	return 0;
}

static int mosc_driver_api_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct ra_root_osc_data *dat = dev->data;

	ARG_UNUSED(sys);

	if ((sys_read8(CGC_MOSCCR) & CGC_MOSCCR_MOSTP) == 0) {
		K_SPINLOCK(&dat->lock) {
			uint16_t old_prcr = get_register_protection();

			set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

			sys_write8(CGC_MOSCCR_MOSTP, CGC_MOSCCR);

			set_register_protection(old_prcr);

			while ((sys_read8(CGC_OSCSF) & CGC_OSCSF_MOSCSF) != 0)
				;
		}
	}
	return 0;
}

static int mosc_driver_api_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	ARG_UNUSED(sys);

	if (rate) {
		*rate = DT_PROP(MOSC_NODE, clock_frequency);
		return 0;
	}

	return -EINVAL;
}

static enum clock_control_status mosc_driver_api_get_status(const struct device *dev,
		clock_control_subsys_t sys)
{
	enum clock_control_status ret;

	ARG_UNUSED(sys);

	if (sys_read8(CGC_MOSCCR) & CGC_MOSCCR_MOSTP) {
		ret = CLOCK_CONTROL_STATUS_OFF;
	} else {
		if (sys_read8(CGC_OSCSF) & CGC_OSCSF_MOSCSF) {
			ret = CLOCK_CONTROL_STATUS_ON;
		} else {
			ret = CLOCK_CONTROL_STATUS_STARTING;
		}
	}

	return ret;
}

static int mosc_init(const struct device *dev)
{
	uint16_t old_prcr;
	uint8_t reg;
	const uint32_t freq = DT_PROP(MOSC_NODE, clock_frequency);

	reg  = (freq < MHZ(10)) ? CGC_MOMCR_MODRV1 : 0;
	reg |= DT_ENUM_IDX(MOSC_NODE, clock_type) * CGC_MOMCR_MOSEL;

	old_prcr = get_register_protection();

	set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

	/* First we define the stabilisation time of the MOSC */
	sys_write8(
		CGC_MOSCWTCR_MSTS(DT_ENUM_IDX(MOSC_NODE, stabilisation_time)),
		CGC_MOSCWTCR);
	sys_write8(reg, CGC_MOMCR);

	set_register_protection(old_prcr);

	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
	static const struct clock_control_driver_api mosc_driver_api = {
		.on = mosc_driver_api_on,
		.off = mosc_driver_api_off,
		.get_rate = mosc_driver_api_get_rate,
		.get_status = mosc_driver_api_get_status,
	};

	static const struct ra_main_osc_config ra_main_osc_config = {
		.common.id = DT_REG_ADDR(MOSC_NODE),
	};

	static struct ra_root_osc_data ra_main_osc_data;

	DEVICE_DT_DEFINE(MOSC_NODE,
			&mosc_init,
			NULL, /* TODO */
			&ra_main_osc_data,
			&ra_main_osc_config,
			PRE_KERNEL_1,
			CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
			&mosc_driver_api);
#endif
