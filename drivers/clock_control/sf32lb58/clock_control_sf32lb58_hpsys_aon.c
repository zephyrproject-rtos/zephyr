/*
 * Copyright (c) 2025 Qingdao IotPi Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb58_hpsys_aon

#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/drivers/clock_control/sf32lb58_clock_control.h>
#include <zephyr/dt-bindings/clock/sf32lb58_clock.h>

struct hpsys_aon_config {
	uintptr_t base;
};

static int hpsys_aon_init(const struct device *dev)
{
	return 0;
}

/* static int hpsys_aon_ */
static int hpsys_aon_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct hpsys_aon_config *config = dev->config;
	uint32_t acr = 0;

	ARG_UNUSED(dev);

	acr = sys_read32(config->base + HPSYS_AON_ACR);

	switch ((uintptr_t)sys) {
	case HPSYS_AON_SUBSYS_HXT48:
		sys_set_bit(config->base + HPSYS_AON_ACR, HPSYS_AON_ACR_HXT48_REQ_Pos);
		break;
	case HPSYS_AON_SUBSYS_HRC48:
		sys_set_bit(config->base + HPSYS_AON_ACR, HPSYS_AON_ACR_HRC48_REQ_Pos);
		break;
	default:
		return -ENOSYS;
	}
	return 0;
}

static int hpsys_aon_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct hpsys_aon_config *config = dev->config;
	uint32_t acr = 0;

	ARG_UNUSED(dev);

	acr = sys_read32(config->base + HPSYS_AON_ACR);

	switch ((uintptr_t)sys) {
	case HPSYS_AON_SUBSYS_HXT48:
		sys_set_bit(HPSYS_AON_ACR, HPSYS_AON_ACR_HXT48_REQ_Pos);
		break;
	case HPSYS_AON_SUBSYS_HRC48:
		sys_set_bit(HPSYS_AON_ACR, HPSYS_AON_ACR_HRC48_REQ_Pos);
		break;
	default:
		return -ENOSYS;
	}
	return 0;
}

static enum clock_control_status hpsys_aon_get_status(const struct device *dev,
						      clock_control_subsys_t sys)
{
	const struct hpsys_aon_config *config = dev->config;
	uint32_t acr = 0;
	ARG_UNUSED(dev);

	acr = sys_read32(config->base + HPSYS_AON_ACR);
	switch ((uintptr_t)sys) {
	case HPSYS_AON_SUBSYS_HXT48:
		if (HPSYS_AON_ACR_HXT48_REQ & acr) {
			if (HPSYS_AON_ACR_HXT48_RDY & acr) {
				return CLOCK_CONTROL_STATUS_ON;
			} else {
				return CLOCK_CONTROL_STATUS_STARTING;
			}
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	case HPSYS_AON_SUBSYS_HRC48:
		if (HPSYS_AON_ACR_HRC48_REQ & acr) {
			if (HPSYS_AON_ACR_HRC48_RDY & acr) {
				return CLOCK_CONTROL_STATUS_ON;
			} else {
				return CLOCK_CONTROL_STATUS_STARTING;
			}
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	default:
	}

	return CLOCK_CONTROL_STATUS_UNKNOWN;
}

static DEVICE_API(clock_control, hpsys_aon_api) = {
	.on = hpsys_aon_on,
	.off = hpsys_aon_off,
	.get_status = hpsys_aon_get_status,
};

static const struct hpsys_aon_config config = {
	.base = DT_REG_ADDR(DT_DRV_INST(0)),
};

DEVICE_DT_INST_DEFINE(0, hpsys_aon_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &hpsys_aon_api);
