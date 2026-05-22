/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Broadcom BCM2835 simple hardware RNG block, present on
 * BCM2835/BCM2836/BCM2837/BCM2710 SoCs (Raspberry Pi 1/2/3/Zero family).
 *
 * NOT to be confused with the iProc RNG200 used on BCM2711 (Pi 4).
 * Register layout (4 registers, 16 bytes):
 *   0x00 RNG_CTRL     bit 0 = RBGEN, set to enable
 *   0x04 RNG_STATUS   bits [31:24] = words available in FIFO;
 *                     write warm-up count here before enabling
 *   0x08 RNG_DATA     read pops one 32-bit random word
 *   0x10 RNG_INT_MASK bit 0 = mask interrupt
 */

#define DT_DRV_COMPAT brcm_bcm2835_rng

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bcm2835_rng_entropy, CONFIG_ENTROPY_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <errno.h>

#define BCM2835_RNG_CTRL_OFFS     0x00
#define BCM2835_RNG_STATUS_OFFS   0x04
#define BCM2835_RNG_DATA_OFFS     0x08
#define BCM2835_RNG_INT_MASK_OFFS 0x10

#define BCM2835_RNG_RBGEN     BIT(0)
#define BCM2835_RNG_INT_OFF   BIT(0)
#define BCM2835_RNG_WARMUP    0x40000

#define BCM2835_RNG_POLL_MAX_RETRIES 100
#define BCM2835_RNG_POLL_INTERVAL_USEC 34

struct bcm2835_rng_config {
	DEVICE_MMIO_NAMED_ROM(base_addr);
};

struct bcm2835_rng_data {
	DEVICE_MMIO_NAMED_RAM(base_addr);
	struct k_mutex mutex;
};

/* Required by the DEVICE_MMIO_NAMED_* accessor macros. */
#define DEV_CFG(dev)  ((const struct bcm2835_rng_config *)(dev)->config)
#define DEV_DATA(dev) ((struct bcm2835_rng_data *)(dev)->data)

static int bcm2835_rng_init(const struct device *dev)
{
	struct bcm2835_rng_data *const data = dev->data;

	k_mutex_init(&data->mutex);

	DEVICE_MMIO_NAMED_MAP(dev, base_addr, K_MEM_CACHE_NONE);

	const mem_addr_t base = DEVICE_MMIO_NAMED_GET(dev, base_addr);

	/* Mask the interrupt -- the driver polls RNG_STATUS instead. */
	sys_write32(BCM2835_RNG_INT_OFF, base + BCM2835_RNG_INT_MASK_OFFS);

	/* Program the warm-up count and enable only if not already running. */
	if (!(sys_read32(base + BCM2835_RNG_CTRL_OFFS) & BCM2835_RNG_RBGEN)) {
		sys_write32(BCM2835_RNG_WARMUP, base + BCM2835_RNG_STATUS_OFFS);
		sys_write32(BCM2835_RNG_RBGEN, base + BCM2835_RNG_CTRL_OFFS);
	}

	return 0;
}

static int bcm2835_rng_get_entropy(const struct device *dev, uint8_t *buffer,
				   uint16_t length)
{
	const mem_addr_t base = DEVICE_MMIO_NAMED_GET(dev, base_addr);
	struct bcm2835_rng_data *const data = dev->data;

	while (length > 0) {
		uint32_t retries = BCM2835_RNG_POLL_MAX_RETRIES;
		uint32_t avail;

		k_mutex_lock(&data->mutex, K_FOREVER);

		while (true) {
			avail = sys_read32(base + BCM2835_RNG_STATUS_OFFS) >> 24;
			if (avail > 0) {
				break;
			}
			if (--retries == 0) {
				k_mutex_unlock(&data->mutex);
				return -ETIMEDOUT;
			}
			k_sleep(K_USEC(BCM2835_RNG_POLL_INTERVAL_USEC));
		}

		while (avail > 0 && length > 0) {
			uint32_t word = sys_read32(base + BCM2835_RNG_DATA_OFFS);
			size_t copylen = MIN(length, sizeof(word));

			memcpy(buffer, &word, copylen);
			buffer += copylen;
			length -= copylen;
			avail--;
		}

		k_mutex_unlock(&data->mutex);
	}

	return 0;
}

static DEVICE_API(entropy, bcm2835_rng_api) = {
	.get_entropy = bcm2835_rng_get_entropy,
};

#define BCM2835_RNG_INIT(n)                                                                        \
	static const struct bcm2835_rng_config bcm2835_rng_##n##_cfg = {                           \
		DEVICE_MMIO_NAMED_ROM_INIT(base_addr, DT_DRV_INST(n)),                             \
	};                                                                                         \
	static struct bcm2835_rng_data bcm2835_rng_##n##_data;                                     \
	DEVICE_DT_INST_DEFINE(n, bcm2835_rng_init, NULL,                                           \
			      &bcm2835_rng_##n##_data,                                             \
			      &bcm2835_rng_##n##_cfg, PRE_KERNEL_1,                                \
			      CONFIG_ENTROPY_INIT_PRIORITY, &bcm2835_rng_api);

DT_INST_FOREACH_STATUS_OKAY(BCM2835_RNG_INIT)
