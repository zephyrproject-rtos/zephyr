/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_iproc_rng200

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(iproc_rng200_entropy, CONFIG_ENTROPY_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#define IPROC_RNG200_CTRL_OFFS       0x00
#define IPROC_RNG200_RNG_RESET_OFFS  0x04
#define IPROC_RNG200_RBG_RESET_OFFS  0x08
#define IPROC_RNG200_RESERVED1_OFFS  0x0c
#define IPROC_RNG200_RESERVED2_OFFS  0x10
#define IPROC_RNG200_RESERVED3_OFFS  0x14
#define IPROC_RNG200_INT_STATUS_OFFS 0x18
#define IPROC_RNG200_RESERVED4_OFFS  0x1c
#define IPROC_RNG200_FIFO_DATA_OFFS  0x20
#define IPROC_RNG200_FIFO_COUNT_OFFS 0x24

#define IPROC_RNG200_CTRL_RBG_EN                    BIT(0)
#define IPROC_RNG200_RESET_EN                       BIT(0)
#define IPROC_RNG200_INT_STATUS_NIST_FAIL           BIT(5)
#define IPROC_RNG200_INT_STATUS_MASTER_FAIL_LOCKOUT BIT(31)

#define IPROC_RNG200_CTRL_RBG_EN_MASK BIT_MASK(13)
#define IPROC_RNG200_FIFO_COUNT_MASK  BIT_MASK(8)

/* time needed to fill fifo when empty */
#define IPROC_RNG200_FIFO_REFILL_TIME_USEC   40
#define IPROC_RNG200_FIFO_REFILL_MAX_RETRIES 5

#define DEV_CFG(dev)  ((const struct iproc_rng200_config *)(dev)->config)
#define DEV_DATA(dev) ((struct iproc_rng200_data *)(dev)->data)

struct iproc_rng200_config {
	DEVICE_MMIO_NAMED_ROM(base_addr);
};

struct iproc_rng200_data {
	DEVICE_MMIO_NAMED_RAM(base_addr);
	struct k_mutex mutex;
};

static int iproc_rng200_driver_init(const struct device *dev)
{
	struct iproc_rng200_data *const data = dev->data;

	k_mutex_init(&data->mutex);

	DEVICE_MMIO_NAMED_MAP(dev, base_addr, K_MEM_CACHE_NONE);

	const mem_addr_t base = DEVICE_MMIO_NAMED_GET(dev, base_addr);
	const uint32_t val =
		sys_read32(base + IPROC_RNG200_CTRL_OFFS) & IPROC_RNG200_CTRL_RBG_EN_MASK;

	sys_write32(val & ~IPROC_RNG200_CTRL_RBG_EN, base + IPROC_RNG200_CTRL_OFFS);

	return 0;
}

static int iproc_rng200_driver_get_entropy(const struct device *dev, uint8_t *buffer,
					   uint16_t length)
{
	const mem_addr_t base = DEVICE_MMIO_NAMED_GET(dev, base_addr);
	const uint32_t word_count = DIV_ROUND_UP(length, 4);
	struct iproc_rng200_data *const data = dev->data;
	uint32_t retries_left;
	uint32_t random_word;

	for (uint32_t i = 0; i < word_count; i++) {
		retries_left = IPROC_RNG200_FIFO_REFILL_MAX_RETRIES;
		k_mutex_lock(&data->mutex, K_FOREVER);

		while (true) {
			const uint32_t status = sys_read32(base + IPROC_RNG200_INT_STATUS_OFFS);
			uint32_t val;

			if (status & (IPROC_RNG200_INT_STATUS_MASTER_FAIL_LOCKOUT |
				      IPROC_RNG200_INT_STATUS_NIST_FAIL)) {

				sys_write32(0xFFFFFFFF, base + IPROC_RNG200_INT_STATUS_OFFS);

				val = sys_read32(base + IPROC_RNG200_RNG_RESET_OFFS);
				sys_write32(val | IPROC_RNG200_RESET_EN,
					    base + IPROC_RNG200_RNG_RESET_OFFS);

				val = sys_read32(base + IPROC_RNG200_RBG_RESET_OFFS);
				sys_write32(val | IPROC_RNG200_RESET_EN,
					    base + IPROC_RNG200_RBG_RESET_OFFS);

				val = sys_read32(base + IPROC_RNG200_RNG_RESET_OFFS);
				sys_write32(val & ~IPROC_RNG200_RESET_EN,
					    base + IPROC_RNG200_RNG_RESET_OFFS);

				val = sys_read32(base + IPROC_RNG200_RBG_RESET_OFFS);
				sys_write32(val & ~IPROC_RNG200_RESET_EN,
					    base + IPROC_RNG200_RBG_RESET_OFFS);
			}

			/* make sure fifo has at least one random word */
			const uint32_t fcnt = sys_read32(base + IPROC_RNG200_FIFO_COUNT_OFFS);

			if ((fcnt & IPROC_RNG200_FIFO_COUNT_MASK) > 0) {
				/* get new random word */
				random_word = sys_read32(base + IPROC_RNG200_FIFO_DATA_OFFS);
				break;
			}

			/* currently no random values available, thus wait */
			retries_left--;
			if (!retries_left) {
				/* number of retries exhausted, give up */
				k_mutex_unlock(&data->mutex);
				return -ETIMEDOUT;
			}

			k_sleep(K_USEC(IPROC_RNG200_FIFO_REFILL_TIME_USEC));
		}

		k_mutex_unlock(&data->mutex);

		memcpy(&buffer[i * 4], &random_word, MIN(length, 4));
		length -= 4;
	}

	return 0;
}

static DEVICE_API(entropy, iproc_rng200_entropy_api) = {
	.get_entropy = iproc_rng200_driver_get_entropy,
};

#define IPROC_RNG200_INIT(n)                                                                       \
	static const struct iproc_rng200_config iproc_rng200_##n##_cfg = {                         \
		DEVICE_MMIO_NAMED_ROM_INIT(base_addr, DT_DRV_INST(n)),                             \
	};                                                                                         \
	static struct iproc_rng200_data iproc_rng200_##n##_data;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, iproc_rng200_driver_init, NULL, &iproc_rng200_##n##_data,         \
			      &iproc_rng200_##n##_cfg, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, \
			      &iproc_rng200_entropy_api);

DT_INST_FOREACH_STATUS_OKAY(IPROC_RNG200_INIT)
