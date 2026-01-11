/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_hxt48

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <register.h>

#define HPSYS_AON_ACR offsetof(HPSYS_AON_TypeDef, ACR)

struct clock_control_sf32lb_hxt48_config {
	uintptr_t aon;
	uint32_t freq_hz;
};

static int clock_control_sf32lb_hxt48_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_sf32lb_hxt48_config *config = dev->config;
	uint32_t val;

	ARG_UNUSED(sys);

	val = sys_read32(config->aon + HPSYS_AON_ACR);
	val |= HPSYS_AON_ACR_HXT48_REQ;
	sys_write32(val, config->aon + HPSYS_AON_ACR);

	while (sys_test_bit(config->aon + HPSYS_AON_ACR, HPSYS_AON_ACR_HXT48_RDY_Pos) == 0U) {
	}

	return 0;
}

static int clock_control_sf32lb_hxt48_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_sf32lb_hxt48_config *config = dev->config;
	uint32_t val;

	ARG_UNUSED(sys);

	val = sys_read32(config->aon + HPSYS_AON_ACR);
	val &= ~HPSYS_AON_ACR_HXT48_REQ;
	sys_write32(val, config->aon + HPSYS_AON_ACR);

	return 0;
}

static enum clock_control_status clock_control_sf32lb_hxt48_get_status(const struct device *dev,
								       clock_control_subsys_t sys)
{
	const struct clock_control_sf32lb_hxt48_config *config = dev->config;

	ARG_UNUSED(sys);

	if (sys_test_bit(config->aon + HPSYS_AON_ACR, HPSYS_AON_ACR_HXT48_RDY_Pos) != 0U) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_sf32lb_hxt48_get_rate(const struct device *dev, clock_control_subsys_t sys,
					       uint32_t *rate)
{
	const struct clock_control_sf32lb_hxt48_config *config = dev->config;

	ARG_UNUSED(sys);

	*rate = config->freq_hz;

	return 0;
}

static DEVICE_API(clock_control, clock_control_sf32lb_hxt48_api) = {
	.on = clock_control_sf32lb_hxt48_on,
	.off = clock_control_sf32lb_hxt48_off,
	.get_status = clock_control_sf32lb_hxt48_get_status,
	.get_rate = clock_control_sf32lb_hxt48_get_rate,
};

static const struct clock_control_sf32lb_hxt48_config config = {
	.aon = DT_REG_ADDR(DT_INST_PHANDLE(0, sifli_aon)),
	.freq_hz = DT_INST_PROP(0, clock_frequency),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_sf32lb_hxt48_api);
