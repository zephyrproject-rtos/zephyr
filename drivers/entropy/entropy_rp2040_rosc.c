/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_rp2040_rosc_rng

#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>

/* RP2040 datasheet, sections 2.15 and 2.17. */
#define ROSC_STATUS_OFFSET      0x18U
#define ROSC_RANDOMBIT_OFFSET   0x1cU
#define ROSC_STATUS_ENABLED     BIT(12)
#define ROSC_STATUS_STABLE      BIT(31)
#define CLK_REF_CTRL_OFFSET     0x30U
#define CLK_REF_SELECTED_OFFSET 0x38U
#define CLK_SYS_CTRL_OFFSET     0x3cU
#define CLK_SYS_SELECTED_OFFSET 0x44U
#define CLK_CTRL_AUXSRC_MASK    GENMASK(7, 5)
#define CLK_SYS_AUXSRC_PLL_SYS  0U
#define CLK_SYS_AUXSRC_PLL_USB  1U
#define CLK_SYS_AUXSRC_XOSC     3U
#define CLK_REF_AUXSRC_PLL_USB  0U
#define ROSC_SAMPLE_INTERVAL_US 10U
#define ROSC_MAX_PAIRS_PER_BIT  32U

struct entropy_rp2040_rosc_config {
	uintptr_t rosc_base;
	uintptr_t clocks_base;
};

struct entropy_rp2040_rosc_data {
	struct k_spinlock lock;
};

static bool entropy_rp2040_rosc_clock_valid(const struct entropy_rp2040_rosc_config *config)
{
	uint32_t selected = sys_read32(config->clocks_base + CLK_SYS_SELECTED_OFFSET);
	uint32_t ctrl;
	uint32_t auxsrc;

	if (selected == BIT(1)) {
		ctrl = sys_read32(config->clocks_base + CLK_SYS_CTRL_OFFSET);
		auxsrc = FIELD_GET(CLK_CTRL_AUXSRC_MASK, ctrl);
		return auxsrc == CLK_SYS_AUXSRC_PLL_SYS || auxsrc == CLK_SYS_AUXSRC_PLL_USB ||
		       auxsrc == CLK_SYS_AUXSRC_XOSC;
	}

	if (selected != BIT(0)) {
		return false;
	}

	selected = sys_read32(config->clocks_base + CLK_REF_SELECTED_OFFSET);
	if (selected == BIT(2)) {
		return true;
	}
	if (selected != BIT(1)) {
		return false;
	}

	ctrl = sys_read32(config->clocks_base + CLK_REF_CTRL_OFFSET);
	return FIELD_GET(CLK_CTRL_AUXSRC_MASK, ctrl) == CLK_REF_AUXSRC_PLL_USB;
}

static int entropy_rp2040_rosc_check(const struct entropy_rp2040_rosc_config *config)
{
	uint32_t status = sys_read32(config->rosc_base + ROSC_STATUS_OFFSET);
	uint32_t required = ROSC_STATUS_ENABLED | ROSC_STATUS_STABLE;

	if ((status & required) != required) {
		return -EIO;
	}
	if (!entropy_rp2040_rosc_clock_valid(config)) {
		return -ENOTSUP;
	}

	return 0;
}

static int entropy_rp2040_rosc_get_bit(const struct device *dev, uint8_t *bit)
{
	const struct entropy_rp2040_rosc_config *config = dev->config;
	struct entropy_rp2040_rosc_data *data = dev->data;

	for (uint32_t pair = 0U; pair < ROSC_MAX_PAIRS_PER_BIT; pair++) {
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		uint8_t first = 0U;
		uint8_t second = 0U;
		int ret = entropy_rp2040_rosc_check(config);

		if (ret == 0) {
			/* Serialize each pair and space every read, including across calls. */
			k_busy_wait(ROSC_SAMPLE_INTERVAL_US);
			first = sys_read32(config->rosc_base + ROSC_RANDOMBIT_OFFSET) & 1U;
			k_busy_wait(ROSC_SAMPLE_INTERVAL_US);
			second = sys_read32(config->rosc_base + ROSC_RANDOMBIT_OFFSET) & 1U;
			ret = entropy_rp2040_rosc_check(config);
		}
		k_spin_unlock(&data->lock, key);

		if (ret != 0) {
			return ret;
		}
		/* Von Neumann extraction: discard 00/11, map 01 to 0 and 10 to 1. */
		if (first != second) {
			*bit = first;
			return 0;
		}
	}

	return -EIO;
}

static int entropy_rp2040_rosc_get_entropy(const struct device *dev, uint8_t *buffer,
					   uint16_t length)
{
	if (buffer == NULL && length != 0U) {
		return -EINVAL;
	}

	for (uint16_t byte = 0U; byte < length; byte++) {
		uint8_t value = 0U;

		for (uint8_t shift = 0U; shift < 8U; shift++) {
			uint8_t bit;
			int ret = entropy_rp2040_rosc_get_bit(dev, &bit);

			if (ret != 0) {
				return ret;
			}
			value |= bit << shift;
		}
		buffer[byte] = value;
	}

	return 0;
}

static int entropy_rp2040_rosc_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					       uint16_t length, uint32_t flags)
{
	if ((flags & ~ENTROPY_BUSYWAIT) != 0U) {
		return -EINVAL;
	}
	if (length == 0U) {
		return 0;
	}
	if ((flags & ENTROPY_BUSYWAIT) == 0U) {
		return -EAGAIN;
	}

	int ret = entropy_rp2040_rosc_get_entropy(dev, buffer, length);

	return ret == 0 ? length : ret;
}

static int entropy_rp2040_rosc_init(const struct device *dev)
{
	return entropy_rp2040_rosc_check(dev->config);
}

static DEVICE_API(entropy, entropy_rp2040_rosc_api) = {
	.get_entropy = entropy_rp2040_rosc_get_entropy,
	.get_entropy_isr = entropy_rp2040_rosc_get_entropy_isr,
};

#define ENTROPY_RP2040_ROSC_DEFINE(inst)                                                           \
	static const struct entropy_rp2040_rosc_config entropy_rp2040_rosc_config_##inst = {       \
		.rosc_base = DT_INST_REG_ADDR(inst),                                               \
		.clocks_base = DT_REG_ADDR_BY_NAME(                                                \
			DT_INST_PHANDLE(inst, raspberrypi_clock_controller), clocks),              \
	};                                                                                         \
	static struct entropy_rp2040_rosc_data entropy_rp2040_rosc_data_##inst;                    \
	DEVICE_DT_INST_DEFINE(inst, entropy_rp2040_rosc_init, NULL,                                \
			      &entropy_rp2040_rosc_data_##inst,                                    \
			      &entropy_rp2040_rosc_config_##inst, POST_KERNEL,                     \
			      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_rp2040_rosc_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_RP2040_ROSC_DEFINE)
