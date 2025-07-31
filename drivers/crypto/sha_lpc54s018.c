/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/crypto.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <fsl_device_registers.h>

LOG_MODULE_REGISTER(sha_lpc54s018, CONFIG_CRYPTO_LOG_LEVEL);

/* SHA peripheral base address */
#define SHA_BASE 0x400A4000U
#define SHA ((SHA_Type *)SHA_BASE)

/* SHA IRQ */
#define SHA_IRQ SHA_IRQn
#define SHA_IRQ_PRIORITY 3

/* SHA register structure */
typedef struct {
	__IO uint32_t CTRL;          /* Control register */
	__IO uint32_t STATUS;        /* Status register */
	__IO uint32_t INTENSET;      /* Interrupt enable set */
	__IO uint32_t INTENCLR;      /* Interrupt enable clear */
	__IO uint32_t MEMCTRL;       /* Memory control */
	__IO uint32_t MEMADDR;       /* Memory address */
	uint8_t RESERVED_0[8];
	__O  uint32_t INDATA;        /* Input data */
	__O  uint32_t ALIAS[7];      /* Alias for burst write */
	__I  uint32_t DIGEST[8];     /* Digest output */
} SHA_Type;

/* SHA Control bits */
#define SHA_CTRL_MODE_MASK           0x00000003U
#define SHA_CTRL_MODE_SHA1           0x00000000U
#define SHA_CTRL_MODE_SHA224         0x00000001U
#define SHA_CTRL_MODE_SHA256         0x00000002U
#define SHA_CTRL_NEW                 0x00000010U
#define SHA_CTRL_DMA_EN              0x00000100U

/* SHA Status bits */
#define SHA_STATUS_WAITING           0x00000001U
#define SHA_STATUS_DIGEST            0x00000002U
#define SHA_STATUS_ERROR             0x00000004U

/* SHA memory control */
#define SHA_MEMCTRL_MASTER           0x00000001U
#define SHA_MEMCTRL_COUNT_MASK       0xFFFF0000U
#define SHA_MEMCTRL_COUNT_SHIFT      16

enum sha_mode {
	SHA_MODE_SHA1 = 0,
	SHA_MODE_SHA224 = 1,
	SHA_MODE_SHA256 = 2
};

struct sha_lpc54s018_data {
	struct k_sem sync_sem;
	enum sha_mode mode;
	bool busy;
	int error;
};

struct sha_lpc54s018_config {
	uint32_t base;
	void (*irq_config_func)(const struct device *dev);
};

static void sha_lpc54s018_isr(const struct device *dev)
{
	struct sha_lpc54s018_data *data = dev->data;
	uint32_t status = SHA->STATUS;

	if (status & SHA_STATUS_DIGEST) {
		/* Digest ready */
		data->error = 0;
		data->busy = false;
		k_sem_give(&data->sync_sem);
	} else if (status & SHA_STATUS_ERROR) {
		LOG_ERR("SHA error");
		data->error = -EIO;
		data->busy = false;
		k_sem_give(&data->sync_sem);
	}
}

static int sha_lpc54s018_hash_internal(const struct device *dev,
				       enum sha_mode mode,
				       const uint8_t *data, size_t data_len,
				       uint8_t *digest, size_t digest_len)
{
	struct sha_lpc54s018_data *dev_data = dev->data;
	int ret;
	size_t blocks;
	size_t remaining;
	const uint32_t *data32;

	/* Set mode and start new hash */
	SHA->CTRL = (mode & SHA_CTRL_MODE_MASK) | SHA_CTRL_NEW;

	/* Process complete 512-bit blocks */
	blocks = data_len / 64;
	data32 = (const uint32_t *)data;

	for (size_t i = 0; i < blocks; i++) {
		/* Feed 64 bytes (16 words) */
		for (int j = 0; j < 16; j++) {
			SHA->INDATA = *data32++;
		}

		/* Wait for block to be processed */
		while (SHA->STATUS & SHA_STATUS_WAITING) {
			k_yield();
		}
	}

	/* Handle remaining data with padding */
	remaining = data_len % 64;
	uint8_t padding_block[64] = {0};
	
	if (remaining > 0) {
		memcpy(padding_block, data + blocks * 64, remaining);
	}

	/* Add padding */
	padding_block[remaining] = 0x80;

	/* If there's not enough room for the length, we need another block */
	if (remaining >= 56) {
		/* Submit this block */
		data32 = (const uint32_t *)padding_block;
		for (int j = 0; j < 16; j++) {
			SHA->INDATA = *data32++;
		}
		while (SHA->STATUS & SHA_STATUS_WAITING) {
			k_yield();
		}

		/* Clear padding block for next submission */
		memset(padding_block, 0, 64);
	}

	/* Add length in bits to last 8 bytes (big-endian) */
	uint64_t bit_len = data_len * 8;
	padding_block[56] = (bit_len >> 56) & 0xFF;
	padding_block[57] = (bit_len >> 48) & 0xFF;
	padding_block[58] = (bit_len >> 40) & 0xFF;
	padding_block[59] = (bit_len >> 32) & 0xFF;
	padding_block[60] = (bit_len >> 24) & 0xFF;
	padding_block[61] = (bit_len >> 16) & 0xFF;
	padding_block[62] = (bit_len >> 8) & 0xFF;
	padding_block[63] = bit_len & 0xFF;

	/* Submit final block */
	data32 = (const uint32_t *)padding_block;
	for (int j = 0; j < 16; j++) {
		SHA->INDATA = *data32++;
	}

	dev_data->busy = true;
	k_sem_reset(&dev_data->sync_sem);

	/* Wait for digest */
	ret = k_sem_take(&dev_data->sync_sem, K_MSEC(1000));
	if (ret != 0) {
		LOG_ERR("SHA operation timeout");
		return -ETIMEDOUT;
	}

	if (dev_data->error) {
		return dev_data->error;
	}

	/* Read digest */
	uint32_t *digest32 = (uint32_t *)digest;
	size_t words = digest_len / 4;
	for (size_t i = 0; i < words; i++) {
		uint32_t word = SHA->DIGEST[i];
		/* Convert from big-endian */
		digest32[i] = __builtin_bswap32(word);
	}

	return 0;
}

int lpc_sha256_hash(const uint8_t *data, size_t data_len, uint8_t *digest)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return sha_lpc54s018_hash_internal(dev, SHA_MODE_SHA256, 
					   data, data_len, digest, 32);
}

int lpc_sha1_hash(const uint8_t *data, size_t data_len, uint8_t *digest)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return sha_lpc54s018_hash_internal(dev, SHA_MODE_SHA1,
					   data, data_len, digest, 20);
}

static int sha_lpc54s018_init(const struct device *dev)
{
	const struct sha_lpc54s018_config *config = dev->config;
	struct sha_lpc54s018_data *data = dev->data;

	LOG_INF("Initializing SHA hardware accelerator");

	k_sem_init(&data->sync_sem, 0, 1);

	/* Enable SHA clock */
	/* TODO: Enable via clock control driver */

	/* Configure interrupts */
	config->irq_config_func(dev);

	/* Enable digest interrupt */
	SHA->INTENSET = SHA_STATUS_DIGEST | SHA_STATUS_ERROR;

	LOG_INF("SHA initialized");

	return 0;
}

static void sha_lpc54s018_irq_config(const struct device *dev)
{
	IRQ_CONNECT(SHA_IRQ, SHA_IRQ_PRIORITY,
		    sha_lpc54s018_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(SHA_IRQ);
}

static struct sha_lpc54s018_data sha_lpc54s018_data_0;

static const struct sha_lpc54s018_config sha_lpc54s018_config_0 = {
	.base = SHA_BASE,
	.irq_config_func = sha_lpc54s018_irq_config,
};

DEVICE_DT_INST_DEFINE(0, sha_lpc54s018_init, NULL,
		      &sha_lpc54s018_data_0, &sha_lpc54s018_config_0,
		      PRE_KERNEL_1, CONFIG_CRYPTO_INIT_PRIORITY,
		      NULL);