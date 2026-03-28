/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_socfpga_reset

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>

/** regwidth 4 for 32 bit register */
#define RESET_REG_WIDTH 4

struct reset_intel_config {
	DEVICE_MMIO_ROM;
	/* check peripheral active low / active high */
	bool active_low;
};

struct reset_intel_soc_data {
	DEVICE_MMIO_RAM;
};

static int32_t reset_intel_soc_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_intel_config *config = (const struct reset_intel_config *)dev->config;
	uintptr_t base_address = DEVICE_MMIO_GET(dev);
	uint32_t value;
	uint16_t offset;
	uint8_t regbit;

	regbit = (id & ((RESET_REG_WIDTH << (RESET_REG_WIDTH - 1)) - 1));
	offset = (id >> (RESET_REG_WIDTH + 1)) << (RESET_REG_WIDTH >> 1);
	value = sys_read32(base_address + offset);
	*status = !(value & BIT(regbit)) ^ config->active_low;
	return 0;
}

static void reset_intel_soc_update(const struct device *dev, uint32_t id, bool assert)
{
	const struct reset_intel_config *config = (const struct reset_intel_config *)dev->config;
	uintptr_t base_address = DEVICE_MMIO_GET(dev);
	uint16_t offset;
	uint8_t regbit;

	regbit = (id & ((RESET_REG_WIDTH << (RESET_REG_WIDTH - 1)) - 1));
	offset = (id >> (RESET_REG_WIDTH + 1)) << (RESET_REG_WIDTH >> 1);

	if (assert ^ !config->active_low) {
		if (sys_test_bit(base_address + offset, regbit) == 0) {
			sys_set_bit(base_address + offset, regbit);
		}
	} else {
		if (sys_test_bit(base_address + offset, regbit) != 0) {
			sys_clear_bit(base_address + offset, regbit);
		}
	}
}

static int32_t reset_intel_soc_line_assert(const struct device *dev, uint32_t id)
{
	reset_intel_soc_update(dev, id, true);

	return 0;
}

static int32_t reset_intel_soc_line_deassert(const struct device *dev, uint32_t id)
{
	reset_intel_soc_update(dev, id, false);

	return 0;
}

static int32_t reset_intel_soc_line_toggle(const struct device *dev, uint32_t id)
{
	(void)reset_intel_soc_line_assert(dev, id);
	/* TODO: Add required delay once tested on Emulator/Hardware platform. */
	(void)reset_intel_soc_line_deassert(dev, id);

	return 0;
}

static int32_t reset_intel_soc_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(reset, reset_intel_soc_driver_api) = {
	.status = reset_intel_soc_status,
	.line_assert = reset_intel_soc_line_assert,
	.line_deassert = reset_intel_soc_line_deassert,
	.line_toggle = reset_intel_soc_line_toggle,
};

#define INTEL_SOC_RESET_INIT(_inst)						\
	static struct reset_intel_soc_data reset_intel_soc_data_##_inst;	\
	static const struct reset_intel_config reset_intel_config_##_inst = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(_inst)),			\
		.active_low = DT_INST_PROP(_inst, active_low),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(_inst,						\
			reset_intel_soc_init,					\
			NULL,							\
			&reset_intel_soc_data_##_inst,				\
			&reset_intel_config_##_inst,				\
			PRE_KERNEL_1,						\
			CONFIG_RESET_INIT_PRIORITY,				\
			&reset_intel_soc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INTEL_SOC_RESET_INIT);
