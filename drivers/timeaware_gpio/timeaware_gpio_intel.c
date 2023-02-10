/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_pmc_tagpio

#include <errno.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/timeaware_gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/syscall_handler.h>

/* TGPIO Register offsets */
#define ART_L           0x00
#define ART_H           0x04
#define CTL             0x10
#define COMPV31_0       0x20
#define COMPV63_32      0x24
#define PIV31_0         0x28
#define PIV63_32        0x2c
#define TCV31_0         0x30
#define TCV63_32        0x34
#define ECCV31_0        0x38
#define ECCV63_32       0x3c
#define EC31_0          0x40
#define EC63_32         0x44
#define REGSET_SIZE     0x100
#define UINT32_MASK     0xFFFFFFFF
#define UINT32_SIZE     32

/* Control Register */
#define CTL_EN                  BIT(0)
#define CTL_DIR                 BIT(1)
#define CTL_EP                  GENMASK(3, 2)
#define CTL_EP_RISING_EDGE      (0 << 2)
#define CTL_EP_FALLING_EDGE     (1 << 2)
#define CTL_EP_TOGGLE_EDGE      (2 << 2)
#define CTL_PM                  BIT(4)

/* Macro to get configuration data, required by DEVICE_MMIO_NAMED_* in init */
#define DEV_CFG(_dev) \
	((const struct tagpio_config *)(_dev)->config)
/* Macro to get runtime data, required by DEVICE_MMIO_NAMED_* in init */
#define DEV_DATA(_dev) ((struct tagpio_runtime *)(_dev)->data)
/* Macro to individual pin regbase */
#define pin_regs(addr, pin) (addr + (pin * REGSET_SIZE))

struct tagpio_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint32_t max_pins;
	uint32_t art_clock_freq;
};

struct tagpio_runtime {
	DEVICE_MMIO_NAMED_RAM(reg_base);
};

static mm_reg_t regs(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

static int tagpio_intel_get_time(const struct device *dev,
					uint64_t *time)
{
	*time = sys_read32(regs(dev) + ART_L);
	*time = *time + ((uint64_t)sys_read32(regs(dev) + ART_H) << UINT32_SIZE);

	return 0;
}

static int tagpio_intel_cyc_per_sec(const struct device *dev,
					   uint32_t *cycles)
{
	*cycles = DEV_CFG(dev)->art_clock_freq;

	return 0;
}

static int tagpio_intel_pin_disable(const struct device *dev,
					   uint32_t pin)
{
	mm_reg_t addr = regs(dev);

	if (pin >= DEV_CFG(dev)->max_pins) {
		return -EINVAL;
	}

	addr = pin_regs(addr, pin);
	sys_write32(sys_read32(addr + CTL) & ~CTL_EN, addr + CTL);

	return 0;
}

static int tagpio_intel_periodic_output(const struct device *dev,
					       uint32_t pin,
					       uint64_t start_time,
					       uint64_t repeat_interval,
					       bool periodic_enable)
{
	mm_reg_t addr = regs(dev);
	uint32_t val;

	if (pin >= DEV_CFG(dev)->max_pins) {
		return -EINVAL;
	}

	addr = pin_regs(addr, pin);
	tagpio_intel_pin_disable(dev, pin);

	/* Configure PIV */
	val = (repeat_interval >> UINT32_SIZE) & UINT32_MASK;
	sys_write32(val, addr + PIV63_32);
	val = repeat_interval & UINT32_MASK;
	sys_write32(val, addr + PIV31_0);

	/* Configure COMPV */
	val = (start_time >> UINT32_SIZE) & UINT32_MASK;
	sys_write32(val, addr + COMPV63_32);
	val = start_time & UINT32_MASK;
	sys_write32(val, addr + COMPV31_0);

	val = 0;

	/* Configure Periodic Mode */
	if (periodic_enable) {
		val = val | CTL_PM;
	}

	/* Enable the pin */
	val = val | CTL_EN;
	sys_write32(val, addr + CTL);

	return 0;
}

static int tagpio_intel_config_external_timestamp(const struct device *dev,
							 uint32_t pin,
							 uint32_t event_polarity)
{
	mm_reg_t addr = regs(dev);
	uint32_t val = 0;

	if (pin >= DEV_CFG(dev)->max_pins) {
		return -EINVAL;
	}

	addr = pin_regs(addr, pin);
	tagpio_intel_pin_disable(dev, pin);

	/* Configure interrupt polarity */
	if (event_polarity == 0) {
		val = CTL_EP_RISING_EDGE;
	} else if (event_polarity == 1) {
		val = CTL_EP_FALLING_EDGE;
	} else {
		val = CTL_EP_TOGGLE_EDGE;
	}

	/* Configure direction = input */
	val = val | CTL_DIR;
	sys_write32(val, addr + CTL);

	/* Enable the pin */
	val = val | CTL_EN;
	sys_write32(sys_read32(addr + CTL) | CTL_EN, addr + CTL);

	return 0;
}

static int tagpio_intel_read_ts_ec(const struct device *dev,
					  uint32_t pin,
					  uint64_t *timestamp,
					  uint64_t *event_count)
{
	if (pin >= DEV_CFG(dev)->max_pins) {
		return -EINVAL;
	}

	*timestamp = sys_read32(regs(dev) + TCV31_0);
	*timestamp = *timestamp + ((uint64_t)sys_read32(regs(dev) + TCV63_32) << UINT32_SIZE);
	*event_count = sys_read32(regs(dev) + ECCV31_0);
	*event_count = *event_count + ((uint64_t)sys_read32(regs(dev) + ECCV63_32) << UINT32_SIZE);

	return 0;
}

static const struct tagpio_driver_api api_funcs = {
	.pin_disable = tagpio_intel_pin_disable,
	.get_time = tagpio_intel_get_time,
	.set_perout = tagpio_intel_periodic_output,
	.config_ext_ts = tagpio_intel_config_external_timestamp,
	.read_ts_ec = tagpio_intel_read_ts_ec,
	.cyc_per_sec = tagpio_intel_cyc_per_sec,
};

static int tagpio_init(const struct device *dev)
{
	const struct tagpio_config *cfg = DEV_CFG(dev);
	struct tagpio_runtime *rt = DEV_DATA(dev);

	device_map(&rt->reg_base,
		   cfg->reg_base.phys_addr & ~0xFFU,
		   cfg->reg_base.size,
		   K_MEM_CACHE_NONE);

	return 0;
}

#define TAGPIO_INTEL_DEV_CFG_DATA(n)					       \
	static const struct tagpio_config				       \
		tagpio_##n##_cfg = {					       \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),	       \
		.max_pins = DT_INST_PROP(n, max_pins),			       \
		.art_clock_freq = DT_INST_PROP(n, timer_clock),		       \
	};								       \
									       \
	static struct tagpio_runtime tagpio_##n##_runtime;		       \
									       \
	DEVICE_DT_INST_DEFINE(n,					       \
			      &tagpio_init,				       \
			      NULL,					       \
			      &tagpio_##n##_runtime,			       \
			      &tagpio_##n##_cfg,			       \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &api_funcs);				       \

DT_INST_FOREACH_STATUS_OKAY(TAGPIO_INTEL_DEV_CFG_DATA)
