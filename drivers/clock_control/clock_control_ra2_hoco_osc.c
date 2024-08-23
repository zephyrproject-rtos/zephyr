/**
 * clock_control_ra2_hoco_osc.c
 *
 * RA2 High-speed on-chip oscillator (HOCO) driver implementation
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

#define DT_DRV_COMPAT	renesas_ra2_hoco_osc
#define HOCO_NODE	DT_DRV_INST(0)

struct ra_hoco_osc_config {
	/* Must be first */
	struct ra_common_osc_config common;
};

static int hoco_driver_api_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct ra_root_osc_data *dat = dev->data;

	ARG_UNUSED(sys);

	if (sys_read8(CGC_HOCOCR) & CGC_HOCOCR_HCSTP) {
		K_SPINLOCK(&dat->lock) {
			uint16_t old_prcr = get_register_protection();

			set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

			sys_write8(0, CGC_HOCOCR);

			set_register_protection(old_prcr);

			while ((sys_read8(CGC_OSCSF) & CGC_OSCSF_HOCOSF) == 0)
				;
		}
	}

	return 0;
}

static int hoco_driver_api_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct ra_root_osc_data *dat = dev->data;

	ARG_UNUSED(sys);

	if ((sys_read8(CGC_HOCOCR) & CGC_HOCOCR_HCSTP) == 0) {
		K_SPINLOCK(&dat->lock) {
			uint16_t old_prcr = get_register_protection();

			set_register_protection(old_prcr | SYSC_PRCR_CLK_PROT);

			sys_write8(CGC_HOCOCR_HCSTP, CGC_HOCOCR);

			set_register_protection(old_prcr);

			while ((sys_read8(CGC_OSCSF) & CGC_OSCSF_HOCOSF) != 0)
				;
		}
	}
	return 0;
}

static int hoco_driver_api_get_rate(const struct device *dev,
				 clock_control_subsys_t sys,
				 uint32_t *rate)
{
	ARG_UNUSED(sys);

	if (rate) {
		*rate = DT_PROP(HOCO_NODE, clock_frequency);
		return 0;
	}
	return -EINVAL;
}

static enum clock_control_status hoco_driver_api_get_status(const struct device *dev,
		clock_control_subsys_t sys)
{
	enum clock_control_status ret;

	ARG_UNUSED(sys);

	if (sys_read8(CGC_HOCOCR) & CGC_HOCOCR_HCSTP) {
		ret = CLOCK_CONTROL_STATUS_OFF;
	} else {
		if (sys_read8(CGC_OSCSF) & CGC_OSCSF_HOCOSF) {
			ret = CLOCK_CONTROL_STATUS_ON;
		} else {
			ret = CLOCK_CONTROL_STATUS_STARTING;
		}
	}

	return ret;
}

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
	static const struct clock_control_driver_api hoco_driver_api = {
		.on = hoco_driver_api_on,
		.off = hoco_driver_api_off,
		.get_rate = hoco_driver_api_get_rate,
		.get_status = hoco_driver_api_get_status,
	};

	static const struct ra_hoco_osc_config ra_hoco_osc_config = {
		.common.id = DT_REG_ADDR(HOCO_NODE),
	};

	static struct ra_root_osc_data ra_hoco_osc_data;

	DEVICE_DT_DEFINE(HOCO_NODE,
		NULL,
		NULL, /* TODO */
		&ra_hoco_osc_data,
		&ra_hoco_osc_config,
		PRE_KERNEL_1,
		CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		&hoco_driver_api);
#endif
