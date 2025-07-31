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

LOG_MODULE_REGISTER(aes_lpc54s018, CONFIG_CRYPTO_LOG_LEVEL);

/* AES peripheral base address from baremetal */
#define AES_BASE 0x400A0000U
#define AES ((AES_Type *)AES_BASE)

/* AES IRQ */
#define AES_IRQ 49
#define AES_IRQ_PRIORITY 3

/* AES register structure based on LPC54S018M.h */
typedef struct {
	union {
		__IO uint32_t CFG;
		struct {
			__IO uint16_t CFG0_15;
			__IO uint16_t CFG16_31;
		} CFG0_32;
	};
	__IO uint32_t CMD;
	__IO uint32_t STAT;
	__IO uint32_t CTR_INCR;
	uint8_t RESERVED_0[16];
	__O  uint32_t KEY[8];
	__O  uint32_t INTEXT[4];
	__O  uint32_t HOLDING[4];
	__I  uint32_t OUTTEXT[4];
	__O  uint32_t IV[4];
	__I  uint32_t TAG[4];
	__I  uint32_t GF128_Y[4];
	__I  uint32_t GF128_Z[4];
	__I  uint32_t GCM_TAG[4];
} AES_Type;

/* AES Configuration bits */
#define AES_CFG_PROC_EN_MASK         0x00000003U
#define AES_CFG_PROC_EN_ENCRYPT      0x00000001U
#define AES_CFG_PROC_EN_DECRYPT      0x00000002U
#define AES_CFG_KEY_CFG_MASK         0x00000300U
#define AES_CFG_KEY_CFG_128          0x00000000U
#define AES_CFG_KEY_CFG_192          0x00000100U
#define AES_CFG_KEY_CFG_256          0x00000200U

/* AES Command bits */
#define AES_CMD_START                0x00000001U

/* AES Status bits */
#define AES_STAT_DONE                0x00000001U
#define AES_STAT_ERROR               0x00000002U

struct aes_lpc54s018_data {
	struct k_sem sync_sem;
	bool busy;
	int error;
};

struct aes_lpc54s018_config {
	uint32_t base;
	void (*irq_config_func)(const struct device *dev);
};

static void aes_lpc54s018_isr(const struct device *dev)
{
	struct aes_lpc54s018_data *data = dev->data;
	uint32_t status = AES->STAT;

	if (status & AES_STAT_DONE) {
		data->error = 0;
		data->busy = false;
		k_sem_give(&data->sync_sem);
	} else if (status & AES_STAT_ERROR) {
		data->error = -EIO;
		data->busy = false;
		k_sem_give(&data->sync_sem);
	}
}

static int aes_lpc54s018_ecb_encrypt(const struct device *dev,
				      const uint8_t *key, size_t key_len,
				      const uint8_t *input, uint8_t *output)
{
	struct aes_lpc54s018_data *data = dev->data;
	uint32_t key_cfg;
	int ret;

	/* Select key size */
	switch (key_len) {
	case 16:
		key_cfg = AES_CFG_KEY_CFG_128;
		break;
	case 24:
		key_cfg = AES_CFG_KEY_CFG_192;
		break;
	case 32:
		key_cfg = AES_CFG_KEY_CFG_256;
		break;
	default:
		return -EINVAL;
	}

	/* Configure AES for ECB encryption */
	AES->CFG = AES_CFG_PROC_EN_ENCRYPT | key_cfg;

	/* Load key */
	const uint32_t *key32 = (const uint32_t *)key;
	for (int i = 0; i < key_len / 4; i++) {
		AES->KEY[i] = key32[i];
	}

	/* Load input data */
	const uint32_t *in32 = (const uint32_t *)input;
	for (int i = 0; i < 4; i++) {
		AES->INTEXT[i] = in32[i];
	}

	data->busy = true;
	k_sem_reset(&data->sync_sem);

	/* Start encryption */
	AES->CMD = AES_CMD_START;

	/* Wait for completion */
	ret = k_sem_take(&data->sync_sem, K_MSEC(100));
	if (ret != 0) {
		LOG_ERR("AES operation timeout");
		return -ETIMEDOUT;
	}

	if (data->error) {
		return data->error;
	}

	/* Read output */
	uint32_t *out32 = (uint32_t *)output;
	for (int i = 0; i < 4; i++) {
		out32[i] = AES->OUTTEXT[i];
	}

	return 0;
}

/* CMAC subkey generation */
static void cmac_generate_subkeys(const struct device *dev,
				  const uint8_t *key, size_t key_len,
				  uint8_t *k1, uint8_t *k2)
{
	uint8_t zero_block[16] = {0};
	uint8_t l[16];
	int carry;

	/* Generate L = AES(K, 0) */
	aes_lpc54s018_ecb_encrypt(dev, key, key_len, zero_block, l);

	/* Generate K1 = L << 1 */
	carry = (l[0] & 0x80) ? 1 : 0;
	for (int i = 0; i < 15; i++) {
		k1[i] = (l[i] << 1) | (l[i + 1] >> 7);
	}
	k1[15] = l[15] << 1;
	if (carry) {
		k1[15] ^= 0x87; /* Rb for AES */
	}

	/* Generate K2 = K1 << 1 */
	carry = (k1[0] & 0x80) ? 1 : 0;
	for (int i = 0; i < 15; i++) {
		k2[i] = (k1[i] << 1) | (k1[i + 1] >> 7);
	}
	k2[15] = k1[15] << 1;
	if (carry) {
		k2[15] ^= 0x87;
	}
}

static int aes_lpc54s018_cmac(const struct device *dev,
			      const uint8_t *key, size_t key_len,
			      const uint8_t *msg, size_t msg_len,
			      uint8_t *mac)
{
	uint8_t k1[16], k2[16];
	uint8_t m_last[16];
	uint8_t x[16] = {0};
	uint8_t y[16];
	size_t n_blocks;
	bool complete_block;

	/* Generate subkeys */
	cmac_generate_subkeys(dev, key, key_len, k1, k2);

	/* Calculate number of blocks */
	n_blocks = (msg_len + 15) / 16;
	if (n_blocks == 0) {
		n_blocks = 1;
	}
	complete_block = (msg_len > 0) && ((msg_len % 16) == 0);

	/* Process all blocks except the last */
	for (size_t i = 0; i < n_blocks - 1; i++) {
		/* XOR with previous result */
		for (int j = 0; j < 16; j++) {
			x[j] ^= msg[i * 16 + j];
		}
		/* Encrypt */
		aes_lpc54s018_ecb_encrypt(dev, key, key_len, x, y);
		memcpy(x, y, 16);
	}

	/* Prepare last block */
	size_t last_block_len = msg_len - (n_blocks - 1) * 16;
	if (complete_block) {
		/* Complete block: XOR with K1 */
		for (int j = 0; j < 16; j++) {
			m_last[j] = msg[(n_blocks - 1) * 16 + j] ^ k1[j];
		}
	} else {
		/* Incomplete block: pad and XOR with K2 */
		memset(m_last, 0, 16);
		memcpy(m_last, &msg[(n_blocks - 1) * 16], last_block_len);
		m_last[last_block_len] = 0x80; /* Padding */
		for (int j = 0; j < 16; j++) {
			m_last[j] ^= k2[j];
		}
	}

	/* XOR with previous result */
	for (int j = 0; j < 16; j++) {
		x[j] ^= m_last[j];
	}

	/* Final encryption */
	aes_lpc54s018_ecb_encrypt(dev, key, key_len, x, mac);

	return 0;
}

static int aes_lpc54s018_init(const struct device *dev)
{
	const struct aes_lpc54s018_config *config = dev->config;
	struct aes_lpc54s018_data *data = dev->data;

	LOG_INF("Initializing AES hardware accelerator");

	k_sem_init(&data->sync_sem, 0, 1);

	/* Enable AES clock */
	/* TODO: Enable via clock control driver */

	/* Configure interrupts */
	config->irq_config_func(dev);

	LOG_INF("AES initialized");

	return 0;
}

static void aes_lpc54s018_irq_config(const struct device *dev)
{
	IRQ_CONNECT(AES_IRQ, AES_IRQ_PRIORITY,
		    aes_lpc54s018_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(AES_IRQ);
}

/* Exported API for secure boot */
int lpc_aes_cmac_authenticate(const uint8_t *key, size_t key_len,
			      const uint8_t *data, size_t data_len,
			      uint8_t *mac)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return aes_lpc54s018_cmac(dev, key, key_len, data, data_len, mac);
}

static struct aes_lpc54s018_data aes_lpc54s018_data_0;

static const struct aes_lpc54s018_config aes_lpc54s018_config_0 = {
	.base = AES_BASE,
	.irq_config_func = aes_lpc54s018_irq_config,
};

DEVICE_DT_INST_DEFINE(0, aes_lpc54s018_init, NULL,
		      &aes_lpc54s018_data_0, &aes_lpc54s018_config_0,
		      PRE_KERNEL_1, CONFIG_CRYPTO_INIT_PRIORITY,
		      NULL);