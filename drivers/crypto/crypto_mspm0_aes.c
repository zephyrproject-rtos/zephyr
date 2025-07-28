/*
 * Copyright 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_aes

#include <zephyr/crypto/cipher.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ti/driverlib/dl_aes.h>

#define BLOCK_SIZE      16
#define AES		DT_NODELABEL(aes)

#define LOG_LEVEL	CONFIG_CRYPTO_LOG_LEVEL
LOG_MODULE_REGISTER(aes);

#define AES_HW_CAPS	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | \
			CAP_SYNC_OPS | CAP_NO_IV_PREFIX)

struct crypto_mspm0_aes_config {
	AES_Regs *regs;
};

struct crypto_mspm0_aes_data {
	struct cipher_pkt *pkt;
	uint8_t block[BLOCK_SIZE] __aligned(4);
	uint8_t processed;
	DL_AES_KEY_LENGTH keylen;
	enum cipher_mode mode;
	enum cipher_op op;
	bool pregen_key;
};

static void crypto_aes_setKey(struct cipher_ctx *ctx, DL_AES_KEY_LENGTH keylen)
{
	const struct device *dev = (struct device *)ctx->device;
	const struct crypto_mspm0_aes_config *config = (struct crypto_mspm0_aes_config *)dev->config;
	AES_Regs *regs = (AES_Regs *)config->regs;

	if (DL_AES_setKey(regs, ctx->key.bit_stream, keylen)
			!= DL_AES_STATUS_SUCCESS) {
		LOG_ERR("Writing AESAKEY reg Failed");
		return;
	}
	DL_AES_setAllKeyWritten(regs);
}

static void crypto_aes_loadDataIn(AES_Regs *regs, uint8_t *data)
{
	if (DL_AES_loadDataIn(regs, data)
			!= DL_AES_STATUS_SUCCESS){
		LOG_ERR("Writing AESADIN reg Failed");
		return;
	}
}

static void crypto_aes_loadXorDataIn(AES_Regs *regs, uint8_t *data)
{
	if (DL_AES_loadXORDataIn(regs, data)
			!= DL_AES_STATUS_SUCCESS){
		LOG_ERR("Writing AESAXDIN reg Failed");
		return;
	}
}

static void crypto_aes_loadXorDataInWithoutTrigger(AES_Regs *regs, uint8_t *data)
{
	if (DL_AES_loadXORDataInWithoutTrigger(regs, data)
			!= DL_AES_STATUS_SUCCESS){
		LOG_ERR("Writing AESAXIN reg Failed");
		return;
	}
}

static void crypto_aes_xorData(uint8_t *data, uint8_t *xorData, uint8_t *output)
{
	if (DL_AES_xorData(data, xorData, output) != DL_AES_STATUS_SUCCESS) {
		LOG_ERR("Unaligned access");
		return;
	}
}

static int validate_pkt(struct cipher_pkt *pkt)
{
	if (pkt->out_buf_max < 16) {
		LOG_ERR("Output buf too small");
		return -ENOMEM;
	}

	if (!pkt || !pkt->out_buf) {
		LOG_WRN("Missing packet");
		return -EINVAL;
	}

	if (pkt->in_len % 16 || pkt->in_len == 0) {
		LOG_ERR("Can't Work on partial blocks");
		return -EINVAL;
	}

	return 0;
}

static int crypto_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct crypto_mspm0_aes_data *data = (struct crypto_mspm0_aes_data *)dev->data;
	AES_Regs *regs = (AES_Regs *)config->regs;
	int ret;

	data->pkt = pkt;

	ret = validate_pkt(pkt);
	if (ret) {
		return ret;
	}

	crypto_aes_setKey(ctx, data->keylen);

	crypto_aes_loadDataIn(regs, pkt->in_buf);

	return 0;
}

static int crypto_aes_cbc_op(struct cipher_ctx *ctx,
			     struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct crypto_mspm0_aes_data *data = (struct crypto_mspm0_aes_data *)dev->data;
	AES_Regs *regs = (AES_Regs *)config->regs;
	int ret;

	data->pkt = pkt;

	ret = validate_pkt(pkt);
	if (ret) {
		return ret;
	}

	switch (data->op) {
	case CRYPTO_CIPHER_OP_DECRYPT:
		data->pregen_key = true;
		iv = pkt->in_buf;
		break;
	case CRYPTO_CIPHER_OP_ENCRYPT:
		memcpy(pkt->out_buf, iv, 16);
		pkt->out_len = 16;
		break;
	default:
		return -EINVAL;
	};

	crypto_aes_setKey(ctx, data->keylen);

	crypto_aes_loadXorDataInWithoutTrigger(regs, iv);

	if (data->op == CRYPTO_CIPHER_OP_DECRYPT) {
		crypto_aes_loadDataIn(regs, pkt->in_buf + BLOCK_SIZE);
	} else {
		crypto_aes_loadXorDataIn(regs, pkt->in_buf);
	}

	return 0;
}

static int crypto_aes_cfb_op(struct cipher_ctx *ctx,
			     struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct crypto_mspm0_aes_data *data = (struct crypto_mspm0_aes_data *)dev->data;
	AES_Regs *regs = (AES_Regs *)config->regs;
	int ret;

	data->pkt = pkt;

	ret = validate_pkt(pkt);
	if (ret) {
		return ret;
	}

	switch (data->op) {
	case CRYPTO_CIPHER_OP_DECRYPT:
		pkt->in_buf += 16;
		pkt->in_len -= 16;
		break;
	case CRYPTO_CIPHER_OP_ENCRYPT:
		memcpy(pkt->out_buf, iv, 16);
		pkt->out_len = 16;
		break;
	default:
		return -EINVAL;
	};

	crypto_aes_setKey(ctx, data->keylen);

	crypto_aes_loadDataIn(regs, iv);
	return 0;
}

static int crypto_aes_ofb_op(struct cipher_ctx *ctx,
			     struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct crypto_mspm0_aes_data *data = (struct crypto_mspm0_aes_data *)dev->data;
	AES_Regs *regs = (AES_Regs *)config->regs;
	int ret;

	data->pkt = pkt;

	ret = validate_pkt(pkt);
	if (ret) {
		return ret;
	}

	switch (data->op) {
	case CRYPTO_CIPHER_OP_DECRYPT:
		pkt->in_buf += 16;
		pkt->in_len -= 16;
		break;
	case CRYPTO_CIPHER_OP_ENCRYPT:
		memcpy(pkt->out_buf, iv, 16);
		pkt->out_len = 16;
		break;
	default:
		return -EINVAL;
	};

	crypto_aes_setKey(ctx, data->keylen);

	crypto_aes_loadDataIn(regs, iv);
	return 0;
}

static void crypto_mspm0_aes_isr(const struct device *dev)
{
	const struct crypto_mspm0_aes_config *config = (struct crypto_mspm0_aes_config *)dev->config;
	struct crypto_mspm0_aes_data *data = (struct crypto_mspm0_aes_data *)dev->data;
	struct cipher_pkt *pkt = data->pkt;
	AES_Regs *regs = config->regs;

	if (data->processed >= pkt->in_len) {
		LOG_ERR("No more block to process");
		return;
	}

	if (!DL_AES_getPendingInterrupt(regs)) {
		LOG_ERR("Module is Busy");
		return;
	}

	if (DL_AES_getDataOut(regs, data->block)
			!= DL_AES_STATUS_SUCCESS){
		LOG_ERR("Reading DataOut Reg Failed");
		return;
	}

	DL_AES_clearInterruptStatus(regs);
	/* Check int is key generated interrupt */
	if (data->pregen_key) {
		DL_AES_MODE aesconf;

		data->pregen_key = false;
		switch (data->mode) {
		case CRYPTO_CIPHER_MODE_CBC:
			/* select the decryption using generated key mode */
			aesconf = DL_AES_MODE_DECRYPT_KEY_IS_FIRST_ROUND_KEY_CBC_MODE;
			break;
		default:
			return;
		}
		DL_AES_init(regs, aesconf, data->keylen);
		DL_AES_setAllKeyWritten(regs);
		return;
	}

	if (data->processed < pkt->in_len) {
		switch (data->mode) {
		case CRYPTO_CIPHER_MODE_CBC:
			if (!(data->op == CRYPTO_CIPHER_OP_ENCRYPT)) {
				/* xor the output read from dataout to get the plaintext */
				crypto_aes_xorData((pkt->in_buf + data->processed),
						   data->block, (pkt->out_buf + data->processed));
				data->processed += BLOCK_SIZE;
				pkt->out_len += data->processed;
				/* load next block of input to start the engine */
				crypto_aes_loadDataIn(regs, (pkt->in_buf + data->processed
							+ BLOCK_SIZE));
			} else {
				/* copy the dataout to output buf */
				memcpy((pkt->out_buf + data->processed + BLOCK_SIZE),
				       data->block, BLOCK_SIZE);
				data->processed += BLOCK_SIZE;
				pkt->out_len += data->processed;
				/* load next block of input to start the engine */
				crypto_aes_loadXorDataIn(regs, (pkt->in_buf + data->processed));
			}
			break;
		case CRYPTO_CIPHER_MODE_CFB:
			if (!(data->op == CRYPTO_CIPHER_OP_ENCRYPT)) {
				/* xor the output read from dataout to get the plaintext */
				crypto_aes_xorData(data->block, (pkt->in_buf + data->processed),
						   (pkt->out_buf + data->processed));
				/* load prev block of input to start the engine */
				crypto_aes_loadDataIn(regs, (pkt->in_buf + data->processed));
				data->processed += BLOCK_SIZE;
				pkt->out_len += data->processed;
			} else {
				/* xor the output read from dataout to get the ciphertext */
				crypto_aes_xorData(data->block, (pkt->in_buf + data->processed),
						   (pkt->out_buf + data->processed + BLOCK_SIZE));
				data->processed += BLOCK_SIZE;
				pkt->out_len += data->processed;
				/* load next block of input to start the engine */
				crypto_aes_loadDataIn(regs, (pkt->out_buf + data->processed));
			}
			break;
		case CRYPTO_CIPHER_MODE_OFB:
			if (!(data->op == CRYPTO_CIPHER_OP_ENCRYPT)) {
				/* xor the output read from dataout to get the plaintext */
				crypto_aes_xorData(data->block, (pkt->in_buf + data->processed),
						   (pkt->out_buf + data->processed));
				data->processed += BLOCK_SIZE;
				pkt->out_len += data->processed;
				/* start the next block operation with feedback data */
				DL_AES_setAllDataWritten(regs);
			} else {
				/* xor the output read from dataout to get the ciphertext */
				crypto_aes_xorData(data->block, (pkt->in_buf + data->processed),
						   (pkt->out_buf + data->processed + BLOCK_SIZE));
				data->processed += BLOCK_SIZE;
				pkt->out_len += data->processed;
				/* start the next block operation with feedback data */
				DL_AES_setAllDataWritten(regs);
			}
			break;
		case CRYPTO_CIPHER_MODE_ECB:
			/* copy the dataout to output buf */
			memcpy((pkt->out_buf + data->processed), data->block, BLOCK_SIZE);
			data->processed += BLOCK_SIZE;
			pkt->out_len = data->processed;
		default:
			return;
		};
	}
}

static int aes_session_setup(const struct device *dev, struct cipher_ctx *ctx,
			     enum cipher_algo algo, enum cipher_mode mode, enum cipher_op op)
{
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct crypto_mspm0_aes_data *data = dev->data;
	AES_Regs *regs = config->regs;
	DL_AES_MODE aesconfig;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algo");
		return -EINVAL;
	}

	if (!ctx) {
		LOG_ERR("Missing context");
		return -EINVAL;
	}

	if (!ctx->key.bit_stream) {
		LOG_ERR("No key provided");
		return -EINVAL;
	}

	switch (ctx->keylen) {
	case 16U:
		data->keylen = DL_AES_KEY_LENGTH_128;
		break;
	case 32U:
		data->keylen = DL_AES_KEY_LENGTH_256;
		break;
	default:
		LOG_ERR("%d : %u key size is not supported", __LINE__, ctx->keylen);
		return -EINVAL;
	}

	DL_AES_softwareReset(regs);

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			aesconfig = DL_AES_MODE_ENCRYPT_ECB_MODE;
		} else {
			aesconfig = DL_AES_MODE_DECRYPT_SAME_KEY_ECB_MODE;
		}
		ctx->ops.block_crypt_hndlr = crypto_aes_ecb_op;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			aesconfig = DL_AES_MODE_ENCRYPT_CBC_MODE;
		} else {
			aesconfig = DL_AES_MODE_GEN_FIRST_ROUND_KEY_CBC_MODE;
		}
		/* enable cipher block mode */
		DL_AES_enableCipherMode(regs);
		ctx->ops.cbc_crypt_hndlr = crypto_aes_cbc_op;
		break;

	case CRYPTO_CIPHER_MODE_CFB:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			aesconfig = DL_AES_MODE_ENCRYPT_CFB_MODE;
		} else {
			aesconfig = DL_AES_MODE_DECRYPT_SAME_KEY_CFB_MODE;
		}
		/* enable cipher block mode */
		DL_AES_enableCipherMode(regs);
		ctx->ops.cfb_crypt_hndlr = crypto_aes_cfb_op;
		break;
	case CRYPTO_CIPHER_MODE_OFB:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			aesconfig = DL_AES_MODE_ENCRYPT_OFB_MODE;
		} else {
			aesconfig = DL_AES_MODE_DECRYPT_SAME_KEY_OFB_MODE;
		}
		/* enable cipher block mode */
		DL_AES_enableCipherMode(regs);
		ctx->ops.ofb_crypt_hndlr = crypto_aes_ofb_op;
		break;
	default:
		return -EINVAL;
	}

	DL_AES_init(regs, aesconfig, data->keylen);

	ctx->ops.cipher_mode = mode;
	ctx->device = dev;

	data->pregen_key = false;
	data->mode = mode;
	data->op = op;

	return 0;
}

static int aes_session_free(const struct device *dev,
			    struct cipher_ctx *ctx)
{
	ARG_UNUSED(dev);
	struct crypto_mspm0_aes_data *data = dev->data;

	data->processed = 0;
	data->keylen = 0;
	return 0;
}

static int aes_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return AES_HW_CAPS;
}

int crypto_aes_init(const struct device *dev)
{
	const struct crypto_mspm0_aes_config *config = (struct crypto_mspm0_aes_config *)dev->config;
	struct crypto_mspm0_aes_data *data = (struct crypto_mspm0_aes_data *)dev->data;
	AES_Regs *regs = config->regs;

	DL_AES_enablePower(regs);

	if (!DL_AES_isPowerEnabled(regs)) {
		LOG_ERR("AES Power is not enabled !!");
		return -1;
	}
	/* disable interrupt */
	DL_AES_disableInterrupt(regs);

	/* clear interrupt status regs */
	DL_AES_clearInterruptStatus(regs);

	/* enable interrupt */
	DL_AES_enableInterrupt(regs);

	if (!DL_AES_getEnabledInterrupts(regs)) {
		LOG_ERR("AES READY INT is not enabled !!");
		return -1;
	}

	IRQ_CONNECT(DT_IRQN(AES),
		    DT_IRQ(AES, priority),
			crypto_mspm0_aes_isr, DEVICE_DT_GET(AES), 0);
	irq_enable(DT_IRQN(AES));

	data->pregen_key = false;
	data->processed = 0;

	return 0;
}

static DEVICE_API(crypto, crypto_enc_funcs) = {
	.cipher_begin_session = aes_session_setup,
	.cipher_free_session = aes_session_free,
	.query_hw_caps = aes_query_caps,
};

static const struct crypto_mspm0_aes_config crypto_aes_config = {
	.regs = (AES_Regs *)DT_INST_REG_ADDR(0),
};

struct crypto_mspm0_aes_data crypto_aes_data;

DEVICE_DT_INST_DEFINE(0, crypto_aes_init, NULL, &crypto_aes_data, &crypto_aes_config,
		      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, (void *)&crypto_enc_funcs);
