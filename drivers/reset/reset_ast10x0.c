/*
 * Copyright (c) 2022 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ast10x0_reset
#include <errno.h>
#include <zephyr/dt-bindings/reset/ast10x0_reset.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>


/*
 * RESET_CTRL0/1_ASSERT registers:
 *   - Each bit in these registers controls a reset line
 *   - Write '1' to a bit: assert the corresponding reset line
 *   - Write '0' to a bit: no effect
 * RESET_CTRL0/1_DEASSERT register:
 *   - Write '1' to a bit: clear the corresponding bit in RESET_CTRL0/1_ASSERT.
 *                         (deassert the corresponding reset line)
 */
#define RESET_CTRL0_ASSERT		0x40
#define RESET_CTRL0_DEASSERT		0x44
#define RESET_CTRL1_ASSERT		0x50
#define RESET_CTRL1_DEASSERT		0x54

struct reset_aspeed_config {
	const struct device *syscon;
};

static int aspeed_reset_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_aspeed_config *config = dev->config;
	const struct device *syscon = config->syscon;
	uint32_t addr = RESET_CTRL0_ASSERT;

	if (id >= ASPEED_RESET_GRP_1_OFFSET) {
		id -= ASPEED_RESET_GRP_1_OFFSET;
		addr = RESET_CTRL1_ASSERT;
	}

	return syscon_write_reg(syscon, addr, BIT(id));
}

static int aspeed_reset_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_aspeed_config *config = dev->config;
	const struct device *syscon = config->syscon;
	uint32_t addr = RESET_CTRL0_DEASSERT;

	if (id >= ASPEED_RESET_GRP_1_OFFSET) {
		id -= ASPEED_RESET_GRP_1_OFFSET;
		addr = RESET_CTRL1_DEASSERT;
	}

	return syscon_write_reg(syscon, addr, BIT(id));
}

static int aspeed_reset_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_aspeed_config *config = dev->config;
	const struct device *syscon = config->syscon;
	uint32_t addr = RESET_CTRL0_ASSERT;
	uint32_t reg_value;
	int ret;

	if (id >= ASPEED_RESET_GRP_1_OFFSET) {
		id -= ASPEED_RESET_GRP_1_OFFSET;
		addr = RESET_CTRL1_ASSERT;
	}

	ret = syscon_read_reg(syscon, addr, &reg_value);
	if (ret == 0) {
		*status = !!(reg_value & BIT(id));
	}

	return ret;
}

static int aspeed_reset_line_toggle(const struct device *dev, uint32_t id)
{
	int ret;

	ret = aspeed_reset_line_assert(dev, id);
	if (ret == 0) {
		ret = aspeed_reset_line_deassert(dev, id);
	}

	return ret;
}

static int aspeed_reset_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct reset_driver_api aspeed_reset_api = {
	.status = aspeed_reset_status,
	.line_assert = aspeed_reset_line_assert,
	.line_deassert = aspeed_reset_line_deassert,
	.line_toggle = aspeed_reset_line_toggle
};

#define ASPEED_RESET_INIT(n)                                                                       \
	static const struct reset_aspeed_config reset_aspeed_cfg_##n = {                           \
		.syscon = DEVICE_DT_GET(DT_NODELABEL(syscon)),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &aspeed_reset_control_init, NULL, NULL, &reset_aspeed_cfg_##n,    \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                    \
			      &aspeed_reset_api);

DT_INST_FOREACH_STATUS_OKAY(ASPEED_RESET_INIT)
