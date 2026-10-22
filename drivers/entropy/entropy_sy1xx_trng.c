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

/* time needed to fill fifo when empty */
#define SY1XX_TRNG_FIFO_REFILL_TIME_USEC   80
#define SY1XX_TRNG_FIFO_REFILL_MAX_RETRIES 5

struct sy1xx_trng_config {
	uint32_t base_addr;
};

struct sy1xx_trng_data {
	struct k_mutex mutex;
};

static int sy1xx_trng_driver_init(const struct device *dev)
{
	const struct sy1xx_trng_config *const cfg = dev->config;
	struct sy1xx_trng_data *const data = dev->data;

	k_mutex_init(&data->mutex);

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

static int sy1xx_trng_driver_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct sy1xx_trng_config *const cfg = dev->config;
	struct sy1xx_trng_data *const data = dev->data;
	uint32_t retries_left;
	uint32_t random_word;

	uint32_t word_count = DIV_ROUND_UP(length, 4);

	for (uint32_t i = 0; i < word_count; i++) {
		retries_left = SY1XX_TRNG_FIFO_REFILL_MAX_RETRIES;

		k_mutex_lock(&data->mutex, K_FOREVER);
		while (true) {
			/* make sure fifo has at least one random word */
			if (sys_read32(cfg->base_addr + SY1XX_TRNG_FIFO_COUNT_OFFS) > 0) {
				/* get new random word */
				random_word = sys_read32(cfg->base_addr + SY1XX_TRNG_VAL_OFFS);
				break;
			}

			/* currently no random values available, thus wait */
			retries_left--;
			if (!retries_left) {
				/* number of retries exhausted, give up */
				k_mutex_unlock(&data->mutex);
				return -ETIMEDOUT;
			}
			k_sleep(K_USEC(SY1XX_TRNG_FIFO_REFILL_TIME_USEC));
		};
		k_mutex_unlock(&data->mutex);

		memcpy(&buffer[i * 4], &random_word, MIN(length, 4));
		length -= 4;
	}

	/* always error check, to make sure that we received valid readings */
	if (0 != sys_read32(cfg->base_addr + SY1XX_TRNG_ERROR_OFFS)) {
		LOG_ERR("failure mode active, reading of values failed");
		return -EINVAL;
	}

	return 0;
}

static int sy1xx_trng_driver_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					     uint16_t length, uint32_t flags)
{

	const struct sy1xx_trng_config *const cfg = dev->config;
	unsigned int key;
	uint32_t random_word;
	int ret;

	uint32_t word_count = DIV_ROUND_UP(length, 4);

	for (uint32_t i = 0; i < word_count; i++) {

		do {
			key = irq_lock();
			/* make sure fifo has at least one random word */
			if (sys_read32(cfg->base_addr + SY1XX_TRNG_FIFO_COUNT_OFFS) > 0) {
				/* get new random word */
				random_word = sys_read32(cfg->base_addr + SY1XX_TRNG_VAL_OFFS);
				ret = 0;
			} else {
				ret = -EAGAIN;
			}
			irq_unlock(key);

			if (ret && !(flags & ENTROPY_BUSYWAIT)) {
				/* no waiting allowed */
				return ret;
			}

		} while (ret);

		memcpy(&buffer[i * 4], &random_word, MIN(length, 4));
		length -= 4;
	}

	/* always error check, to make sure that we received valid readings */
	if (0 != sys_read32(cfg->base_addr + SY1XX_TRNG_ERROR_OFFS)) {
		LOG_ERR("failure mode active, reading of values failed");
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(entropy,
		  sy1xx_entropy_api) = {.get_entropy = sy1xx_trng_driver_get_entropy,
					.get_entropy_isr = sy1xx_trng_driver_get_entropy_isr};

#define SY1XX_TRNG_INIT(n)                                                                         \
                                                                                                   \
	static const struct sy1xx_trng_config sy1xx_trng##n##_cfg = {                              \
		.base_addr = (uint32_t)DT_INST_REG_ADDR(n),                                        \
	};                                                                                         \
                                                                                                   \
	static struct sy1xx_trng_data sy1xx_trng##n##_data = {};                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, sy1xx_trng_driver_init, NULL, &sy1xx_trng##n##_data,              \
			      &sy1xx_trng##n##_cfg, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,    \
			      &sy1xx_entropy_api);

DT_INST_FOREACH_STATUS_OKAY(SY1XX_TRNG_INIT)
