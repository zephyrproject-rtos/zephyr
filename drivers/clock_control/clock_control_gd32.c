/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_cctl

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>

#include <gd32_regs.h>

/** RCU offset (from id cell) */
#define GD32_CLOCK_ID_OFFSET(id) (((id) >> 6U) & 0xFFU)
/** RCU configuration bit (from id cell) */
#define GD32_CLOCK_ID_BIT(id)	 ((id)&0x1FU)

/** AHB prescaler exponents */
static const uint8_t ahb_exp[16] = {
	0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U,
};
/** APB1 prescaler exponents */
static const uint8_t apb1_exp[8] = {
	0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U,
};
/** APB2 prescaler exponents */
static const uint8_t apb2_exp[8] = {
	0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U,
};

struct clock_control_gd32_config {
	uint32_t base;
};

static int clock_control_gd32_on(const struct device *dev,
				 clock_control_subsys_t sys)
{
	const struct clock_control_gd32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	sys_set_bit(config->base + GD32_CLOCK_ID_OFFSET(id),
		    GD32_CLOCK_ID_BIT(id));

	return 0;
}

static int clock_control_gd32_off(const struct device *dev,
				  clock_control_subsys_t sys)
{
	const struct clock_control_gd32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	sys_clear_bit(config->base + GD32_CLOCK_ID_OFFSET(id),
		      GD32_CLOCK_ID_BIT(id));

	return 0;
}

static int clock_control_gd32_get_rate(const struct device *dev,
				       clock_control_subsys_t sys,
				       uint32_t *rate)
{
	const struct clock_control_gd32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;
	uint32_t cfg;
	uint8_t psc;

	cfg = sys_read32(config->base + RCU_CFG0_OFFSET);

	switch (GD32_CLOCK_ID_OFFSET(id)) {
#if defined(CONFIG_SOC_SERIES_GD32F4XX)
	case RCU_AHB1EN_OFFSET:
	case RCU_AHB2EN_OFFSET:
	case RCU_AHB3EN_OFFSET:
#else
	case RCU_AHBEN_OFFSET:
#endif
		psc = (cfg & RCU_CFG0_AHBPSC_MSK) >> RCU_CFG0_AHBPSC_POS;
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >> ahb_exp[psc];
		break;
	case RCU_APB1EN_OFFSET:
#if !defined(CONFIG_SOC_SERIES_GD32VF103)
	case RCU_ADDAPB1EN_OFFSET:
#endif
		psc = (cfg & RCU_CFG0_APB1PSC_MSK) >> RCU_CFG0_APB1PSC_POS;
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >> apb1_exp[psc];
		break;
	case RCU_APB2EN_OFFSET:
		psc = (cfg & RCU_CFG0_APB2PSC_MSK) >> RCU_CFG0_APB2PSC_POS;
		*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >> apb2_exp[psc];
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static enum clock_control_status
clock_control_gd32_get_status(const struct device *dev,
			      clock_control_subsys_t sys)
{
	const struct clock_control_gd32_config *config = dev->config;
	uint16_t id = *(uint16_t *)sys;

	if (sys_test_bit(config->base + GD32_CLOCK_ID_OFFSET(id),
			 GD32_CLOCK_ID_BIT(id)) != 0) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static struct clock_control_driver_api clock_control_gd32_api = {
	.on = clock_control_gd32_on,
	.off = clock_control_gd32_off,
	.get_rate = clock_control_gd32_get_rate,
	.get_status = clock_control_gd32_get_status,
};

static int clock_control_gd32_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct clock_control_gd32_config config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, clock_control_gd32_init, NULL, NULL, &config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &clock_control_gd32_api);
