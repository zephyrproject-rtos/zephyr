/*
 * Copyright 2026 Linumiz
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

#define AES_HW_CAPS	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)
#define AES_BLOCK_SIZE	16

/*
 * The block cycle for aes module max is 300 cycles
 * safety margin for this module timeout 300 << 1 cycles
 * K_CYC calculates the time period for respective clock freq
 * AES_WAIT_TIMEOUT is calculated for worst case time
 */
#define AES_SEM_TIMEOUT		K_CYC(AES_BLOCK_TIMEOUT)
#define AES_WAIT_TIMEOUT	K_USEC(10)
#define AES_BLOCK_TIMEOUT	(MSPM0_AES_BLOCK_CYC << 1)
#define MSPM0_AES_BLOCK_CYC	300

LOG_MODULE_REGISTER(aes, CONFIG_CRYPTO_LOG_LEVEL);

struct crypto_mspm0_aes_config {
	AES_Regs *regs;
	void (*irq_config_func)(const struct device *dev);
};

struct mspm0_aes_session {
	DL_AES_KEY_LENGTH keylen;
	DL_AES_MODE aesconfig;
	enum cipher_op op;
	bool in_use;
};

struct crypto_mspm0_aes_data {
	struct mspm0_aes_session sessions[CONFIG_CRYPTO_MSPM0_MAX_SESSION];
	struct k_mutex device_mutex;
	struct k_sem aes_done;
};

static int validate_pkt(struct cipher_pkt *pkt)
{
	if (pkt == NULL || pkt->in_buf == NULL || pkt->out_buf == NULL) {
		LOG_ERR("Invalid packet or NULL buffers");
		return -EINVAL;
	}

	if (pkt->in_len == 0 || (pkt->in_len % AES_BLOCK_SIZE) != 0) {
		LOG_ERR("Invalid input length");
		return -EINVAL;
	}

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("Output buffer too small");
		return -EINVAL;
	}

	return 0;
}

static int aes_hw_init(struct cipher_ctx *ctx)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct mspm0_aes_session *session = ctx->drv_sessn_state;
	int ret;

	DL_AES_softwareReset(config->regs);

	DL_AES_init(config->regs, session->aesconfig, session->keylen);

	ret = DL_AES_setKey(config->regs, ctx->key.bit_stream, session->keylen);
	if (ret != DL_AES_STATUS_SUCCESS) {
		LOG_ERR("AES HW init setkey failed : %d", ret);
		return ret;
	}

	DL_AES_setAllKeyWritten(config->regs);

	return ret;
}

static int crypto_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct mspm0_aes_session *session = ctx->drv_sessn_state;
	struct crypto_mspm0_aes_data *data = dev->data;
	int bytes_processed = 0;
	int ret;

	if (session == NULL || !session->in_use) {
		LOG_ERR("No session data");
		return -EINVAL;
	}

	ret = validate_pkt(pkt);
	if (ret != 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->device_mutex, AES_WAIT_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	ret = aes_hw_init(ctx);
	if (ret != 0) {
		goto cleanup;
	}

	do {
		/* load the block */
		ret = DL_AES_loadDataIn(config->regs, &pkt->in_buf[bytes_processed]);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

		/* read the dataout */
		ret = DL_AES_getDataOut(config->regs, &pkt->out_buf[bytes_processed]);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}

		bytes_processed += AES_BLOCK_SIZE;

	} while (bytes_processed < pkt->in_len);

cleanup:
	if (ret != 0 && bytes_processed < pkt->in_len) {
		LOG_ERR("aborted after %d/%d bytes (ret : %d)", bytes_processed, pkt->in_len, ret);
	}
	pkt->out_len = bytes_processed;
	k_mutex_unlock(&data->device_mutex);

	return ret;
}

static int crypto_aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct mspm0_aes_session *session = ctx->drv_sessn_state;
	struct crypto_mspm0_aes_data *data = dev->data;
	bool pregen_key = false;
	int bytes_processed = 0;
	int ret;

	if (session == NULL || !session->in_use || iv == NULL) {
		LOG_ERR("No session data");
		return -EINVAL;
	}

	ret = validate_pkt(pkt);
	if (ret != 0) {
		return ret;
	}

	if (session->op == CRYPTO_CIPHER_OP_DECRYPT) {
		pregen_key = true;
	}

	ret = k_mutex_lock(&data->device_mutex, AES_WAIT_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	ret = aes_hw_init(ctx);
	if (ret != 0) {
		goto cleanup;
	}

	/* Enable cipher mode for cbc */
	DL_AES_enableCipherMode(config->regs);

	/* change the mode from pre-gen to use-pre-gen key mode */
	if (pregen_key) {
		DL_AES_MODE aesmode;

		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			goto cleanup;
		}

		aesmode = DL_AES_MODE_DECRYPT_KEY_IS_FIRST_ROUND_KEY_CBC_MODE;

		DL_AES_init(config->regs, aesmode, session->keylen);
		DL_AES_setAllKeyWritten(config->regs);
	}

	/* load iv */
	ret = DL_AES_loadXORDataInWithoutTrigger(config->regs, iv);
	if (ret != DL_AES_STATUS_SUCCESS) {
		goto cleanup;
	}

	do {
		/* load the next block */
		if (session->op == CRYPTO_CIPHER_OP_DECRYPT) {
			ret = DL_AES_loadDataIn(config->regs,
						&pkt->in_buf[bytes_processed]);
			if (ret != DL_AES_STATUS_SUCCESS) {
				break;
			}
		} else {
			ret = DL_AES_loadXORDataIn(config->regs,
						   &pkt->in_buf[bytes_processed]);
			if (ret != DL_AES_STATUS_SUCCESS) {
				break;
			}
		}

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

		/* xor the iv with internal state */
		if (session->op == CRYPTO_CIPHER_OP_DECRYPT) {
			ret = DL_AES_loadXORDataInWithoutTrigger(config->regs, iv);
			if (ret != DL_AES_STATUS_SUCCESS) {
				break;
			}
			iv = &pkt->in_buf[bytes_processed];
		}

		/* read the dataout */
		ret = DL_AES_getDataOut(config->regs, &pkt->out_buf[bytes_processed]);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}
		bytes_processed += AES_BLOCK_SIZE;

	} while (bytes_processed < pkt->in_len);

cleanup:
	if (ret != 0 && bytes_processed < pkt->in_len) {
		LOG_ERR("aborted after %d/%d bytes (ret : %d)", bytes_processed, pkt->in_len, ret);
	}
	pkt->out_len = bytes_processed;
	k_mutex_unlock(&data->device_mutex);

	return ret;
}

static int crypto_aes_cfb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct mspm0_aes_session *session = ctx->drv_sessn_state;
	struct crypto_mspm0_aes_data *data = dev->data;
	int bytes_processed = 0;
	int ret;

	if (session == NULL || !session->in_use || iv == NULL) {
		LOG_ERR("No session data");
		return -EINVAL;
	}

	ret = validate_pkt(pkt);
	if (ret != 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->device_mutex, AES_WAIT_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	ret = aes_hw_init(ctx);
	if (ret != 0) {
		goto cleanup;
	}

	/* Enable cipher mode for cfb */
	DL_AES_enableCipherMode(config->regs);

	do {
		/* load the next block */
		ret = DL_AES_loadDataIn(config->regs, iv);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

		/* xor the intput block with internal state */
		ret = DL_AES_loadXORDataInWithoutTrigger(config->regs,
							 &pkt->in_buf[bytes_processed]);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}

		/* load next block */
		if (session->op == CRYPTO_CIPHER_OP_DECRYPT) {
			iv = &pkt->in_buf[bytes_processed];
		} else {
			iv = &pkt->out_buf[bytes_processed];
		}

		/* read the dataout */
		ret = DL_AES_getDataOut(config->regs, &pkt->out_buf[bytes_processed]);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}
		bytes_processed += AES_BLOCK_SIZE;

	} while (bytes_processed < pkt->in_len);

cleanup:
	pkt->out_len = bytes_processed;
	k_mutex_unlock(&data->device_mutex);

	return ret;
}

static int crypto_aes_ofb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct device *dev = ctx->device;
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct mspm0_aes_session *session = ctx->drv_sessn_state;
	struct crypto_mspm0_aes_data *data = dev->data;
	uint8_t block[AES_BLOCK_SIZE] __aligned(4);
	int bytes_processed = 0;
	int ret;

	if (session == NULL || !session->in_use || iv == NULL) {
		LOG_ERR("No session data");
		return -EINVAL;
	}

	ret = validate_pkt(pkt);
	if (ret != 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->device_mutex, AES_WAIT_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	ret = aes_hw_init(ctx);
	if (ret != 0) {
		goto cleanup;
	}

	/* Enable cipher mode for ofb */
	DL_AES_enableCipherMode(config->regs);

	do {
		/* load the next block */
		ret = DL_AES_loadDataIn(config->regs, iv);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

		/* read the dataout for next feedback */
		ret = DL_AES_getDataOut(config->regs, (uint8_t *)&block);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}
		iv = (uint8_t *)&block;

		/* xor the input text with internal state */
		ret = DL_AES_loadXORDataInWithoutTrigger(config->regs,
							 &pkt->in_buf[bytes_processed]);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}

		/* read the dataout */
		ret = DL_AES_getDataOut(config->regs, &pkt->out_buf[bytes_processed]);
		if (ret != DL_AES_STATUS_SUCCESS) {
			break;
		}
		bytes_processed += AES_BLOCK_SIZE;

	} while (bytes_processed < pkt->in_len);

cleanup:
	pkt->out_len = bytes_processed;
	k_mutex_unlock(&data->device_mutex);

	return ret;
}

static void crypto_mspm0_aes_isr(const struct device *dev)
{
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct crypto_mspm0_aes_data *data = dev->data;

	if (!DL_AES_getPendingInterrupt(config->regs)) {
		LOG_ERR("No pending Interrupts");
		return;
	}
	k_sem_give(&data->aes_done);
}

static int aes_session_setup(const struct device *dev, struct cipher_ctx *ctx,
			     enum cipher_algo algo, enum cipher_mode mode, enum cipher_op op)
{
	struct crypto_mspm0_aes_data *data = dev->data;
	struct mspm0_aes_session *session = NULL;
	DL_AES_KEY_LENGTH keylen;
	int ret;

	if (algo != CRYPTO_CIPHER_ALGO_AES || ctx == NULL || ctx->key.bit_stream == NULL) {
		return -EINVAL;
	}

	if (ctx->flags & ~(AES_HW_CAPS)) {
		return -ENOTSUP;
	}

	switch (ctx->keylen) {
	case 16U:
		keylen = DL_AES_KEY_LENGTH_128;
		break;
	case 32U:
		keylen = DL_AES_KEY_LENGTH_256;
		break;
	default:
		LOG_ERR("key size is not supported");
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->device_mutex, AES_WAIT_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t session_num = 0; session_num < ARRAY_SIZE(data->sessions); session_num++) {
		if (!data->sessions[session_num].in_use) {
			LOG_INF("Claiming session %d", session_num);
			session = &data->sessions[session_num];
			break;
		}
	}

	if (session == NULL) {
		LOG_ERR("All %d session(s) in use", CONFIG_CRYPTO_MSPM0_MAX_SESSION);
		ret = -EINVAL;
		goto out;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			session->aesconfig = DL_AES_MODE_ENCRYPT_ECB_MODE;
		} else {
			session->aesconfig = DL_AES_MODE_DECRYPT_SAME_KEY_ECB_MODE;
		}
		ctx->ops.block_crypt_hndlr = crypto_aes_ecb_op;
		break;

	case CRYPTO_CIPHER_MODE_CBC:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			session->aesconfig = DL_AES_MODE_ENCRYPT_CBC_MODE;
		} else {
			session->aesconfig = DL_AES_MODE_GEN_FIRST_ROUND_KEY_CBC_MODE;
		}
		ctx->ops.cbc_crypt_hndlr = crypto_aes_cbc_op;
		break;

	case CRYPTO_CIPHER_MODE_CFB:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			session->aesconfig = DL_AES_MODE_ENCRYPT_CFB_MODE;
		} else {
			session->aesconfig = DL_AES_MODE_DECRYPT_SAME_KEY_CFB_MODE;
		}
		ctx->ops.cfb_crypt_hndlr = crypto_aes_cfb_op;
		break;

	case CRYPTO_CIPHER_MODE_OFB:
		if (op == CRYPTO_CIPHER_OP_ENCRYPT) {
			session->aesconfig = DL_AES_MODE_ENCRYPT_OFB_MODE;
		} else {
			session->aesconfig = DL_AES_MODE_DECRYPT_SAME_KEY_OFB_MODE;
		}
		ctx->ops.ofb_crypt_hndlr = crypto_aes_ofb_op;
		break;

	default:
		LOG_ERR("MODE NOT SUPPORTED");
		ret = -EINVAL;
		goto out;
	}

	session->keylen = keylen;
	session->in_use = true;
	ctx->drv_sessn_state = session;
	ctx->ops.cipher_mode = mode;
	ctx->device = dev;
	session->op = op;
out:
	k_mutex_unlock(&data->device_mutex);
	return ret;
}

static int aes_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	struct crypto_mspm0_aes_data *data = dev->data;
	struct mspm0_aes_session *session;
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	session = ctx->drv_sessn_state;

	ret = k_mutex_lock(&data->device_mutex, AES_WAIT_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (session == NULL || !session->in_use) {
		LOG_ERR("Session already free!");
		k_mutex_unlock(&data->device_mutex);
		return -EINVAL;
	}

	session->in_use = false;
	ctx->drv_sessn_state = NULL;
	ctx->device = NULL;

	k_mutex_unlock(&data->device_mutex);

	return 0;
}

static int aes_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return AES_HW_CAPS;
}

int crypto_aes_init(const struct device *dev)
{
	const struct crypto_mspm0_aes_config *config = dev->config;

	DL_AES_enablePower(config->regs);

	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	/* disable interrupt */
	DL_AES_disableInterrupt(config->regs);

	/* clear interrupt status regs */
	DL_AES_clearInterruptStatus(config->regs);

	config->irq_config_func(dev);

	/* enable interrupt */
	DL_AES_enableInterrupt(config->regs);

	return 0;
}

static DEVICE_API(crypto, crypto_enc_funcs) = {
	.cipher_begin_session = aes_session_setup,
	.cipher_free_session = aes_session_free,
	.query_hw_caps = aes_query_caps,
};

#define MSPM0_AES_DEFINE(n)									\
												\
	static void crypto_mspm0_irq_config_##n(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), crypto_mspm0_aes_isr,	\
			    DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_INST_IRQN(n));							\
	}											\
												\
	static const struct crypto_mspm0_aes_config crypto_aes_config_##n = {			\
		.regs = (AES_Regs *)DT_INST_REG_ADDR(n),					\
		.irq_config_func = crypto_mspm0_irq_config_##n,					\
	};											\
												\
	struct crypto_mspm0_aes_data crypto_aes_data_##n = {					\
		.device_mutex = Z_MUTEX_INITIALIZER(crypto_aes_data_##n.device_mutex),		\
		.aes_done = Z_SEM_INITIALIZER(crypto_aes_data_##n.aes_done, 0, 1),		\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, crypto_aes_init, NULL, &crypto_aes_data_##n,			\
			&crypto_aes_config_##n, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,	\
			(void *)&crypto_enc_funcs);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_AES_DEFINE)
