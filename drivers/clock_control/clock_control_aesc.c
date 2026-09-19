/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_clock_controller

#include <errno.h>
#include <ip_identification.h>
#include <aesc_syscon.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/init.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_clock, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define CLOCK_AESC_DOMAINS		0x00
#define CLOCK_AESC_DOMAIN_BASE		0x08
#define CLOCK_AESC_DOMAIN_STRIDE	0x08
#define CLOCK_AESC_DOMAIN_CONTROL(n)	(CLOCK_AESC_DOMAIN_BASE +	       \
					 ((n) * CLOCK_AESC_DOMAIN_STRIDE))
#define CLOCK_AESC_DOMAIN_RATIO(n)	(CLOCK_AESC_DOMAIN_CONTROL(n) + 0x04)

#define CLOCK_CTRL_ENABLE	BIT(31)
#define CLOCK_CTRL_LOCK		BIT(30)

struct clock_control_aesc_config {
	DEVICE_MMIO_ROM;
	const struct device *syscon;
};

struct clock_control_aesc_data {
	DEVICE_MMIO_RAM;
	uintptr_t reg_base;
};

#define DEV_CFG(dev) ((const struct clock_control_aesc_config *)(dev)->config)
#define DEV_DATA(dev) ((struct clock_control_aesc_data *)(dev)->data)

static uint32_t clock_control_aesc_domain_count(const struct device *dev)
{
	struct clock_control_aesc_data *data = DEV_DATA(dev);

	return sys_read32(data->reg_base + CLOCK_AESC_DOMAINS) & 0xFF;
}

static int clock_control_aesc_on(const struct device *dev,
				 clock_control_subsys_t sys)
{
	struct clock_control_aesc_data *data = DEV_DATA(dev);
	uint32_t domain = (uint32_t)(uintptr_t)sys;
	uintptr_t reg;

	if (domain >= clock_control_aesc_domain_count(dev)) {
		return -EINVAL;
	}

	reg = data->reg_base + CLOCK_AESC_DOMAIN_CONTROL(domain);
	sys_write32(CLOCK_CTRL_ENABLE, reg);
	if ((sys_read32(reg) & CLOCK_CTRL_ENABLE) == 0) {
		return -EIO;
	}

	return 0;
}

static int clock_control_aesc_off(const struct device *dev,
				  clock_control_subsys_t sys)
{
	struct clock_control_aesc_data *data = DEV_DATA(dev);
	uint32_t domain = (uint32_t)(uintptr_t)sys;
	uintptr_t reg;

	if (domain >= clock_control_aesc_domain_count(dev)) {
		return -EINVAL;
	}

	reg = data->reg_base + CLOCK_AESC_DOMAIN_CONTROL(domain);
	sys_write32(0, reg);

	if ((sys_read32(reg) & CLOCK_CTRL_ENABLE) != 0) {
		return -ENOTSUP;
	}

	return 0;
}

static int clock_control_aesc_get_rate(const struct device *dev,
				       clock_control_subsys_t sys,
				       uint32_t *rate)
{
	const struct clock_control_aesc_config *cfg = DEV_CFG(dev);
	struct clock_control_aesc_data *data = DEV_DATA(dev);
	uint32_t domain = (uint32_t)(uintptr_t)sys;
	uint32_t reference, ratio, mult, divider;
	int ret;

	if (domain >= clock_control_aesc_domain_count(dev)) {
		return -EINVAL;
	}

	if (!device_is_ready(cfg->syscon)) {
		return -ENODEV;
	}

	ret = syscon_read_reg(cfg->syscon, SYSCON_REF_CLOCK, &reference);
	if (ret < 0) {
		return ret;
	}

	ratio = sys_read32(data->reg_base + CLOCK_AESC_DOMAIN_RATIO(domain));
	mult = (ratio >> 16) & 0xFFFF;
	divider = ratio & 0xFFFF;
	if (divider == 0) {
		return -EINVAL;
	}

	*rate = (uint32_t)(((uint64_t)reference * mult) / divider);

	return 0;
}

static enum clock_control_status clock_control_aesc_get_status(
				const struct device *dev,
				clock_control_subsys_t sys)
{
	struct clock_control_aesc_data *data = DEV_DATA(dev);
	uint32_t domain = (uint32_t)(uintptr_t)sys;
	uint32_t control;

	if (domain >= clock_control_aesc_domain_count(dev)) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	control = sys_read32(data->reg_base + CLOCK_AESC_DOMAIN_CONTROL(domain));
	if ((control & CLOCK_CTRL_ENABLE) == 0) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	return CLOCK_CONTROL_STATUS_ON;
}

static int clock_control_aesc_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	struct clock_control_aesc_data *data = DEV_DATA(dev);
	volatile uintptr_t *base_addr = (volatile uintptr_t *)DEVICE_MMIO_GET(dev);

	LOG_DBG("IP core version: %i.%i.%i.",
		ip_id_get_major_version(base_addr),
		ip_id_get_minor_version(base_addr),
		ip_id_get_patchlevel(base_addr));

	data->reg_base = ip_id_relocate_driver(base_addr);

	return 0;
}

static DEVICE_API(clock_control, clock_control_aesc_api) = {
	.on = clock_control_aesc_on,
	.off = clock_control_aesc_off,
	.get_rate = clock_control_aesc_get_rate,
	.get_status = clock_control_aesc_get_status,
};

#define CLOCK_CONTROL_AESC_INIT(n)						\
	static struct clock_control_aesc_data clock_control_aesc_data_##n;	\
	static const struct clock_control_aesc_config clock_control_aesc_cfg_##n = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),				\
		.syscon = DEVICE_DT_GET(DT_INST_PHANDLE(n, syscon)),		\
	};									\
	DEVICE_DT_INST_DEFINE(n,						\
			      clock_control_aesc_init,				\
			      NULL,						\
			      &clock_control_aesc_data_##n,			\
			      &clock_control_aesc_cfg_##n,			\
			      PRE_KERNEL_1,					\
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,		\
			      &clock_control_aesc_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_CONTROL_AESC_INIT)
