/*
 * Copyright (c) 2026 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_hwrng

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define HWRNG_CTRL            0x0U
#define HWRNG_CTRL_DISABLE    BIT(0)
#define HWRNG_CTRL_MODE       GENMASK(5, 1)
#define HWRNG_CTRL_DATA_VALID BIT(31)
#define HWRNG_DATA            0x4U

#define HWRNG_MODE 0x18U

#define HWRNG_TIMEOUT_US    1000U
#define HWRNG_DISCARD_WORDS 32U

struct aspeed_hwrng_config {
	uintptr_t base;
};

static int aspeed_hwrng_try_read(const struct device *dev, uint32_t *word)
{
	const struct aspeed_hwrng_config *config = dev->config;
	unsigned int key;
	int ret = 0;

	/* Keep another caller from consuming data between the valid check and read. */
	key = irq_lock();
	if ((sys_read32(config->base + HWRNG_CTRL) & HWRNG_CTRL_DATA_VALID) == 0U) {
		ret = -EAGAIN;
	} else {
		*word = sys_read32(config->base + HWRNG_DATA);
	}
	irq_unlock(key);

	return ret;
}

static int aspeed_hwrng_read_word(const struct device *dev, uint32_t *word, bool busywait)
{
	bool ready;
	int ret;

	if (!busywait) {
		return aspeed_hwrng_try_read(dev, word);
	}

	ready = WAIT_FOR((ret = aspeed_hwrng_try_read(dev, word)) != -EAGAIN, HWRNG_TIMEOUT_US,
			 k_busy_wait(1U));
	if (!ready) {
		return -ETIMEDOUT;
	}

	return ret;
}

static int aspeed_hwrng_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					uint16_t length, uint32_t flags)
{
	bool busywait = (flags & ENTROPY_BUSYWAIT) != 0U;
	size_t count = 0U;
	size_t to_copy;
	uint32_t word;
	int ret;

	while (count < length) {
		ret = aspeed_hwrng_read_word(dev, &word, busywait);
		if (ret == -EAGAIN) {
			break;
		}
		if (ret != 0) {
			return ret;
		}

		to_copy = MIN(length - count, sizeof(word));
		memcpy(buffer + count, &word, to_copy);
		count += to_copy;
	}

	return (int)count;
}

static int aspeed_hwrng_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	int ret = aspeed_hwrng_get_entropy_isr(dev, buffer, length, ENTROPY_BUSYWAIT);

	return ret < 0 ? ret : 0;
}

static int aspeed_hwrng_init(const struct device *dev)
{
	const struct aspeed_hwrng_config *config = dev->config;
	uint32_t ctrl;
	uint32_t word;
	int ret;

	ctrl = sys_read32(config->base + HWRNG_CTRL);
	ctrl &= ~(HWRNG_CTRL_DISABLE | HWRNG_CTRL_MODE);
	ctrl |= FIELD_PREP(HWRNG_CTRL_MODE, HWRNG_MODE);
	sys_write32(ctrl, config->base + HWRNG_CTRL);

	/* The first word read after power-on repeats across power cycles. */
	for (uint32_t i = 0U; i < HWRNG_DISCARD_WORDS; i++) {
		ret = aspeed_hwrng_read_word(dev, &word, true);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(entropy, aspeed_hwrng_api) = {
	.get_entropy = aspeed_hwrng_get_entropy,
	.get_entropy_isr = aspeed_hwrng_get_entropy_isr,
};

#define ASPEED_HWRNG_INIT(n)                                                                       \
	static const struct aspeed_hwrng_config aspeed_hwrng_config_##n = {                        \
		.base = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, aspeed_hwrng_init, NULL, NULL, &aspeed_hwrng_config_##n,          \
			      PRE_KERNEL_2, CONFIG_ENTROPY_INIT_PRIORITY, &aspeed_hwrng_api);

DT_INST_FOREACH_STATUS_OKAY(ASPEED_HWRNG_INIT)
