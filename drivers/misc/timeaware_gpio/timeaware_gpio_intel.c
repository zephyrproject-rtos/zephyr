/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_timeaware_gpio

#include <errno.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/misc/timeaware_gpio/timeaware_gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/internal/syscall_handler.h>

/* TGPIO Register offsets */
#define ART_L           0x00 /* ART lower 32 bit reg */
#define ART_H           0x04 /* ART higher 32 bit reg */
#define CTL             0x10 /* TGPIO control reg */
#define COMPV31_0       0x20 /* Comparator lower 32 bit reg */
#define COMPV63_32      0x24 /* Comparator higher 32 bit reg */
#define PIV31_0         0x28 /* Periodic Interval lower 32 bit reg */
#define PIV63_32        0x2c /* Periodic Interval higher 32 bit reg */
#define TCV31_0         0x30 /* Time Capture lower 32 bit reg */
#define TCV63_32        0x34 /* Time Capture higher 32 bit reg */
#define ECCV31_0        0x38 /* Event Counter Capture lower 32 bit reg */
#define ECCV63_32       0x3c /* Event Counter Capture higher 32 bit reg */
#define EC31_0          0x40 /* Event Counter lower 32 bit reg */
#define EC63_32         0x44 /* Event Counter higher 32 bit reg */
#define REGSET_SIZE     0x100 /* Difference between 0 and 1 */
#define UINT32_MASK     0xFFFFFFFF /* 32 bit Mask */
#define UINT32_SIZE     32

/* Control Register */
#define CTL_EN                  BIT(0) /* Control enable */
#define CTL_DIR                 BIT(1) /* Control disable */
#define CTL_EP                  GENMASK(3, 2) /* Recerved polarity */
#define CTL_EP_RISING_EDGE      (0 << 2) /* Rising edge */
#define CTL_EP_FALLING_EDGE     (1 << 2) /* Falling edge */
#define CTL_EP_TOGGLE_EDGE      (2 << 2) /* Toggle edge */
#define CTL_PM                  BIT(4) /* Periodic mode */

/* Macro to get configuration data, required by DEVICE_MMIO_NAMED_* in init */
#define DEV_CFG(_dev) \
	((const struct tgpio_config *)(_dev)->config)
/* Macro to get runtime data, required by DEVICE_MMIO_NAMED_* in init */
#define DEV_DATA(_dev) ((struct tgpio_runtime *)(_dev)->data)
/* Macro to individual pin regbase */
#define pin_regs(addr, pin) (addr + (pin * REGSET_SIZE))

struct tgpio_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint32_t max_pins;
	uint32_t art_clock_freq;
};

struct tgpio_runtime {
	DEVICE_MMIO_NAMED_RAM(reg_base);
};

static mm_reg_t regs(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

static int tgpio_intel_get_time(const struct device *dev,
					uint64_t *current_time)
{
	*current_time = sys_read32(regs(dev) + ART_L);
	*current_time += ((uint64_t)sys_read32(regs(dev) + ART_H) << UINT32_SIZE);

	return 0;
}

static int tgpio_intel_cyc_per_sec(const struct device *dev,
					   uint32_t *cycles)
{
	*cycles = DEV_CFG(dev)->art_clock_freq;

	return 0;
}

static int tgpio_intel_pin_disable(const struct device *dev,
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

static int tgpio_intel_periodic_output(const struct device *dev,
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
	tgpio_intel_pin_disable(dev, pin);

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
		val |= CTL_PM;
	}

	/* Enable the pin */
	val |= CTL_EN;
	sys_write32(val, addr + CTL);

	return 0;
}

static int tgpio_intel_config_external_timestamp(const struct device *dev,
							 uint32_t pin,
							 uint32_t event_polarity)
{
	mm_reg_t addr = regs(dev);
	uint32_t val;

	if (pin >= DEV_CFG(dev)->max_pins) {
		return -EINVAL;
	}

	addr = pin_regs(addr, pin);
	tgpio_intel_pin_disable(dev, pin);

	/* Configure interrupt polarity */
	if (event_polarity == 0) {
		val = CTL_EP_RISING_EDGE;
	} else if (event_polarity == 1) {
		val = CTL_EP_FALLING_EDGE;
	} else {
		val = CTL_EP_TOGGLE_EDGE;
	}

	/* Configure direction = input */
	val |= CTL_DIR;
	sys_write32(val, addr + CTL);

	/* Enable the pin */
	sys_write32(sys_read32(addr + CTL) | CTL_EN, addr + CTL);

	return 0;
}

static int tgpio_intel_read_ts_ec(const struct device *dev,
					  uint32_t pin,
					  uint64_t *timestamp,
					  uint64_t *event_count)
{
	if (pin >= DEV_CFG(dev)->max_pins) {
		return -EINVAL;
	}

	*timestamp = sys_read32(regs(dev) + TCV31_0);
	*timestamp += ((uint64_t)sys_read32(regs(dev) + TCV63_32) << UINT32_SIZE);
	*event_count = sys_read32(regs(dev) + ECCV31_0);
	*event_count += ((uint64_t)sys_read32(regs(dev) + ECCV63_32) << UINT32_SIZE);

	return 0;
}

static const struct tgpio_driver_api api_funcs = {
	.pin_disable = tgpio_intel_pin_disable,
	.get_time = tgpio_intel_get_time,
	.set_perout = tgpio_intel_periodic_output,
	.config_ext_ts = tgpio_intel_config_external_timestamp,
	.read_ts_ec = tgpio_intel_read_ts_ec,
	.cyc_per_sec = tgpio_intel_cyc_per_sec,
};

static int tgpio_init(const struct device *dev)
{
	const struct tgpio_config *cfg = DEV_CFG(dev);
	struct tgpio_runtime *rt = DEV_DATA(dev);

	device_map(&rt->reg_base,
		   cfg->reg_base.phys_addr & ~0xFFU,
		   cfg->reg_base.size,
		   K_MEM_CACHE_NONE);

	return 0;
}

#define TGPIO_INTEL_DEV_CFG_DATA(n)					       \
	static const struct tgpio_config				       \
		tgpio_##n##_cfg = {					       \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),	       \
		.max_pins = DT_INST_PROP(n, max_pins),			       \
		.art_clock_freq = DT_INST_PROP(n, timer_clock),		       \
	};								       \
									       \
	static struct tgpio_runtime tgpio_##n##_runtime;		       \
									       \
	DEVICE_DT_INST_DEFINE(n,					       \
			      &tgpio_init,				       \
			      NULL,					       \
			      &tgpio_##n##_runtime,			       \
			      &tgpio_##n##_cfg,			       \
			      POST_KERNEL, CONFIG_TIMEAWARE_GPIO_INIT_PRIORITY,\
			      &api_funcs);				       \

DT_INST_FOREACH_STATUS_OKAY(TGPIO_INTEL_DEV_CFG_DATA)
