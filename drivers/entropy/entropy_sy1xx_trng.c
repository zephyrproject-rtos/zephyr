/*
 * Copyright (c) 2025 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensry_sy1xx_trng

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sy1xx_entropy, CONFIG_ENTROPY_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#define SY1XX_TRNG_VAL_OFFS        0x00
#define SY1XX_TRNG_FIFO_COUNT_OFFS 0x04
#define SY1XX_TRNG_STATUS_OFFS     0x08
#define SY1XX_TRNG_ERROR_OFFS      0x0c

#define SY1XX_TRNG_FIFO_SIZE 64

enum sy1xx_trng_wait_method {
	TRNG_NO_WAIT,
	TRNG_BUSY_WAIT,
	TRNG_SLEEP,
};

struct sy1xx_trng_config {
	uint32_t base_addr;
};

struct sy1xx_trng_data {
	struct k_mutex mtx;
};

static int sy1xx_trng_driver_init(const struct device *dev)
{
	const struct sy1xx_trng_config *const cfg = dev->config;
	struct sy1xx_trng_data *const data = dev->data;

	k_mutex_init(&data->mtx);

	/* trng comes up fully initialized, so only check if all is fine */
	if (0 != sys_read32(cfg->base_addr + SY1XX_TRNG_ERROR_OFFS)) {
		LOG_ERR("failure mode active, internal init failed");
		return -EINVAL;
	}

	if (SY1XX_TRNG_FIFO_SIZE != sys_read32(cfg->base_addr + SY1XX_TRNG_FIFO_COUNT_OFFS)) {
		LOG_ERR("fifo not fully loaded");
		return -EINVAL;
	}

	return 0;
}

static int sy1xx_trng_driver_read(const struct device *dev, uint8_t *buffer, uint16_t length,
				  enum sy1xx_trng_wait_method wait_method)
{
	const struct sy1xx_trng_config *const cfg = dev->config;
	struct sy1xx_trng_data *const data = dev->data;

	uint32_t word_count = DIV_ROUND_UP(length, 4);
	bool fast_read = false;

	if (wait_method == TRNG_NO_WAIT) {
		if (k_mutex_lock(&data->mtx, K_NO_WAIT)) {
			LOG_ERR("failure to acquire mutex");
			return -EAGAIN;
		}
	} else {
		k_mutex_lock(&data->mtx, K_FOREVER);
	}

	/* pre-check current fifo size */
	if (word_count <= sys_read32(cfg->base_addr + SY1XX_TRNG_FIFO_COUNT_OFFS)) {
		/* fifo has enough random words, we can fast read, without waiting */
		fast_read = true;
	}

	for (uint32_t i = 0; i < word_count; i++) {
		if (!fast_read) {
			/* wait (poll) in case fifo is empty, typically < 50us */
			while (sys_read32(cfg->base_addr + SY1XX_TRNG_FIFO_COUNT_OFFS) == 0) {
				if (wait_method == TRNG_NO_WAIT) {
					/* no waiting allowed */
					k_mutex_unlock(&data->mtx);
					return -EAGAIN;
				}
				if (wait_method == TRNG_SLEEP) {
					/* wait by sleeping */
					k_yield();
				} else {
					/* busy wait */
				}
			}
		}

		/* get new random word and copy to byte buffer */
		uint32_t random_word = sys_read32(cfg->base_addr + SY1XX_TRNG_VAL_OFFS);

		memcpy(&buffer[i * 4], &random_word, MIN(length, 4));
		length -= 4;
	}

	/* always error check, to make sure that we received valid readings */
	if (0 != sys_read32(cfg->base_addr + SY1XX_TRNG_ERROR_OFFS)) {
		LOG_ERR("failure mode active, reading of values failed");
		k_mutex_unlock(&data->mtx);
		return -EINVAL;
	}

	k_mutex_unlock(&data->mtx);
	return 0;
}

static int sy1xx_trng_driver_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	return sy1xx_trng_driver_read(dev, buffer, length, TRNG_SLEEP);
}

static int sy1xx_trng_driver_get_entropy_within_isr(const struct device *dev, uint8_t *buffer,
						    uint16_t length, uint32_t flags)
{
	return sy1xx_trng_driver_read(dev, buffer, length,
				      (flags & ENTROPY_BUSYWAIT) ? TRNG_BUSY_WAIT : TRNG_NO_WAIT);
}

static DEVICE_API(entropy, sy1xx_entropy_api) = {
	.get_entropy = sy1xx_trng_driver_get_entropy,
	.get_entropy_isr = sy1xx_trng_driver_get_entropy_within_isr};

#define SY1XX_TRNG_INIT(n)                                                                         \
                                                                                                   \
	static const struct sy1xx_trng_config sy1xx_trng##n##_cfg = {                              \
		.base_addr = (uint32_t)DT_INST_REG_ADDR(0),                                        \
	};                                                                                         \
                                                                                                   \
	static const struct sy1xx_trng_data sy1xx_trng##n##_data = {};                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(0, sy1xx_trng_driver_init, NULL, &sy1xx_trng##n##_data,              \
			      &sy1xx_trng##n##_cfg, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,    \
			      &sy1xx_entropy_api);

DT_INST_FOREACH_STATUS_OKAY(SY1XX_TRNG_INIT)
