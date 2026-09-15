/*
 * Copyright 2026 Linumiz
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_CRYPTO_MSPM0_AES
#define DT_DRV_COMPAT ti_mspm0_aes
#else
#define DT_DRV_COMPAT ti_msp_aes_adv
#endif

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include "crypto_msp_aes.h"

LOG_MODULE_REGISTER(msp_aes, CONFIG_CRYPTO_LOG_LEVEL);

#ifdef CONFIG_CRYPTO_MSPM0_AES
static int aes_load_data_word(volatile uint32_t *reg, const uint8_t *ptr, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) {
		/* Read in each byte to avoid possible unaligned 32bit access */
		*reg = ((uint32_t)ptr[0] <<  0) |
		       ((uint32_t)ptr[1] <<  8) |
		       ((uint32_t)ptr[2] << 16) |
		       ((uint32_t)ptr[3] << 24);
		ptr += 4;
	}

	return 0;
}

static int aes_set_key(aes_ti_mspm0_reg_t *regs, const uint8_t *key, uint32_t keylen)
{
	uint8_t num_words;

	switch (keylen) {
	case AES_MSPM0_AESACTL0_KLX_128:
		num_words = AES_128_KEY_WORDS;
		break;
	case AES_MSPM0_AESACTL0_KLX_256:
		num_words = AES_256_KEY_WORDS;
		break;
	default:
		LOG_ERR("Invalid key length");
		return -EINVAL;
	}

	return aes_load_data_word(&regs->aesakey, key, num_words);
}

static int aes_load_data_in(aes_ti_mspm0_reg_t *regs, const uint8_t *data)
{
	return aes_load_data_word(&regs->aesadin, data, AES_128_KEY_WORDS);
}

static int aes_get_data_out(aes_ti_mspm0_reg_t *regs, uint8_t *data)
{
	for (uint8_t i = 0; i < AES_128_KEY_WORDS; i++) {
		/* Read out each byte to avoid possible unaligned 32bit access */
		uint32_t value = regs->aesadout;

		data[i * 4 + 0] = (value >>  0) & 0xff;
		data[i * 4 + 1] = (value >>  8) & 0xff;
		data[i * 4 + 2] = (value >> 16) & 0xff;
		data[i * 4 + 3] = (value >> 24) & 0xff;
	}

	return 0;
}

static int aes_load_xor_data_in(aes_ti_mspm0_reg_t *regs, const uint8_t *data)
{
	return aes_load_data_word(&regs->aesaxdin, data, AES_128_KEY_WORDS);
}

static int aes_load_xor_data_in_without_trigger(aes_ti_mspm0_reg_t *regs, const uint8_t *data)
{
	return aes_load_data_word(&regs->aesaxin, data, AES_128_KEY_WORDS);
}
#else
static void aes_adv_load_words(volatile uint32_t *reg, const uint8_t *ptr, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) {
		reg[i] = ((uint32_t)ptr[0] <<  0) |
			 ((uint32_t)ptr[1] <<  8) |
			 ((uint32_t)ptr[2] << 16) |
			 ((uint32_t)ptr[3] << 24);
		ptr += 4;
	}
}

static void aes_adv_read_words(uint8_t *ptr, volatile uint32_t *reg, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) {
		uint32_t val = reg[i];

		ptr[0] = (val >>  0) & 0xff;
		ptr[1] = (val >>  8) & 0xff;
		ptr[2] = (val >> 16) & 0xff;
		ptr[3] = (val >> 24) & 0xff;
		ptr += 4;
	}
}

static int aes_adv_load_key(aes_adv_ti_msp_reg_t *regs, const uint8_t *key, uint32_t keylen)
{
	uint8_t num_words;

	switch (keylen) {
	case AESADV_MSP_CTRL_KEYSIZE_128:
		num_words = AES_128_KEY_WORDS;
		break;
	case AESADV_MSP_CTRL_KEYSIZE_256:
		num_words = AES_256_KEY_WORDS;
		break;
	default:
		LOG_ERR("Invalid key length");
		return -EINVAL;
	}

	aes_adv_load_words(regs->key, key, num_words);

	return 0;
}

static void aes_adv_load_iv(aes_adv_ti_msp_reg_t *regs, const uint8_t *iv)
{
	aes_adv_load_words(regs->iv, iv, 4);
}
#endif

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
	const struct crypto_msp_aes_config *config = dev->config;
	struct msp_aes_session *session = ctx->drv_sessn_state;
	int ret;

#ifdef CONFIG_CRYPTO_MSPM0_AES
	/* AES software reset */
	config->regs->aesactl0 |= AES_MSPM0_AESACTL0_SWRST;

	/* Write command, operation and key length */
	config->regs->aesactl0 = (config->regs->aesactl0 &
				      ~(AES_MSPM0_AESACTL0_CMX_MASK |
				      AES_MSPM0_AESACTL0_OPX_MASK |
				      AES_MSPM0_AESACTL0_KLX_MASK)) |
				      session->aesconfig | session->keylen;

	ret = aes_set_key(config->regs, ctx->key.bit_stream, session->keylen);
	if (ret != 0) {
		LOG_ERR("AES HW init setkey failed : %d", ret);
		return ret;
	}

	/* All bytes written to AESAKEY */
	config->regs->aesastat |= AES_MSPM0_AESASTAT_KEYWR;
#else
	ret = aes_adv_load_key(config->regs, ctx->key.bit_stream, session->keylen);
	if (ret != 0) {
		LOG_ERR("AES ADV HW init setkey failed : %d", ret);
		return ret;
	}
	config->regs->ctrl = session->aesconfig | session->keylen;
#endif

	return 0;
}

static int crypto_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct device *dev = ctx->device;
	const struct crypto_msp_aes_config *config = dev->config;
	struct msp_aes_session *session = ctx->drv_sessn_state;
	struct crypto_msp_aes_data *data = dev->data;
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

	k_sem_reset(&data->aes_done);

	ret = aes_hw_init(ctx);
	if (ret != 0) {
		goto cleanup;
	}

#ifndef CONFIG_CRYPTO_MSPM0_AES
	config->regs->c_length_0 = pkt->in_len;
#endif

	do {
#ifdef CONFIG_CRYPTO_MSPM0_AES
		/* load the block */
		ret = aes_load_data_in(config->regs, &pkt->in_buf[bytes_processed]);
		if (ret != 0) {
			break;
		}
#else
		/* enable interrupt before writing data for next block */
		config->regs->imask |= AESADV_MSP_IMASK_OUTPUTRDY_MASK;
		aes_adv_load_words(config->regs->data,
				   &pkt->in_buf[bytes_processed],
				   AES_BLOCK_WORDS);
#endif

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

#ifdef CONFIG_CRYPTO_MSPM0_AES
		/* read the dataout */
		ret = aes_get_data_out(config->regs, &pkt->out_buf[bytes_processed]);
		if (ret != 0) {
			break;
		}
#else
		aes_adv_read_words(&pkt->out_buf[bytes_processed],
				   config->regs->data,
				   AES_BLOCK_WORDS);
#endif

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
	const struct crypto_msp_aes_config *config = dev->config;
	struct msp_aes_session *session = ctx->drv_sessn_state;
	struct crypto_msp_aes_data *data = dev->data;
	int bytes_processed = 0;
	int ret;

	if (session == NULL || !session->in_use) {
		LOG_ERR("Invalid session");
		return -EINVAL;
	}

	if (iv == NULL) {
		LOG_ERR("Iv input is invalid");
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

	k_sem_reset(&data->aes_done);

	ret = aes_hw_init(ctx);
	if (ret != 0) {
		goto cleanup;
	}

#ifdef CONFIG_CRYPTO_MSPM0_AES
	/* Enable cipher mode for cbc */
	config->regs->aesactl0 |= AES_MSPM0_AESACTL0_CMEN;

	/* change the mode from pre-gen to use-pre-gen key mode for decrypt */
	if (session->op == CRYPTO_CIPHER_OP_DECRYPT) {
		uint32_t aesmode;

		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			goto cleanup;
		}

		aesmode = AES_MSPM0_AESACTL0_CMX_CBC | AES_MSPM0_AESACTL0_OPX_DECRYPT_FIRST_KEY;

		/* Write command, operation and key length */
		config->regs->aesactl0 = (config->regs->aesactl0 & ~(AES_MSPM0_AESACTL0_CMX_MASK |
								     AES_MSPM0_AESACTL0_OPX_MASK |
								     AES_MSPM0_AESACTL0_KLX_MASK)) |
					  aesmode | session->keylen;

		/* All bytes written to AESAKEY */
		config->regs->aesastat |= AES_MSPM0_AESASTAT_KEYWR;
	}

	/* load iv */
	ret = aes_load_xor_data_in_without_trigger(config->regs, iv);
	if (ret != 0) {
		goto cleanup;
	}
#else
	/* load iv */
	aes_adv_load_iv(config->regs, iv);

	config->regs->c_length_0 = pkt->in_len;
#endif

	do {
#ifdef CONFIG_CRYPTO_MSPM0_AES
		/* load the next block */
		if (session->op == CRYPTO_CIPHER_OP_DECRYPT) {
			ret = aes_load_data_in(config->regs, &pkt->in_buf[bytes_processed]);
			if (ret != 0) {
				break;
			}
		} else {
			ret = aes_load_xor_data_in(config->regs, &pkt->in_buf[bytes_processed]);
			if (ret != 0) {
				break;
			}
		}
#else
		/* enable interrupt before writing data for next block */
		config->regs->imask |= AESADV_MSP_IMASK_OUTPUTRDY_MASK;
		aes_adv_load_words(config->regs->data,
				   &pkt->in_buf[bytes_processed],
				   AES_BLOCK_WORDS);
#endif

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

#ifdef CONFIG_CRYPTO_MSPM0_AES
		/* xor the iv with internal state */
		if (session->op == CRYPTO_CIPHER_OP_DECRYPT) {
			ret = aes_load_xor_data_in_without_trigger(config->regs, iv);
			if (ret != 0) {
				break;
			}
			/* update iv to current ciphertext block for next block's XOR */
			iv = &pkt->in_buf[bytes_processed];
		}

		/* read the dataout */
		ret = aes_get_data_out(config->regs, &pkt->out_buf[bytes_processed]);
		if (ret != 0) {
			break;
		}
#else
		aes_adv_read_words(&pkt->out_buf[bytes_processed],
				   config->regs->data,
				   AES_BLOCK_WORDS);
#endif

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

static void crypto_msp_aes_isr(const struct device *dev)
{
	const struct crypto_msp_aes_config *config = dev->config;
	struct crypto_msp_aes_data *data = dev->data;
	/* Current interrupt is cleared by the hardware on reading IIDX
	 * register and corresponding interrupt flag in RIS and MIS are
	 * cleared as well.
	 */
#ifdef CONFIG_CRYPTO_MSPM0_AES
	if (!(config->regs->iidx)) {
		LOG_ERR("No pending Interrupts");
		return;
	}
	k_sem_give(&data->aes_done);
#else
	switch (config->regs->iidx) {
	case 1:
		config->regs->imask &= ~AESADV_MSP_IMASK_OUTPUTRDY_MASK;
		k_sem_give(&data->aes_done);
		break;
	case 3:
		k_sem_give(&data->aes_done);
		break;
	default:
		break;
	}
#endif
}

static int aes_session_setup(const struct device *dev, struct cipher_ctx *ctx,
			     enum cipher_algo algo, enum cipher_mode mode, enum cipher_op op)
{
	struct crypto_msp_aes_data *data = dev->data;
	struct msp_aes_session *session = NULL;
	uint32_t keylen;
	uint32_t aesconfig;
	int ret;

	if (algo != CRYPTO_CIPHER_ALGO_AES || ctx == NULL || ctx->key.bit_stream == NULL) {
		return -EINVAL;
	}

	if (ctx->flags & ~(AES_HW_CAPS)) {
		return -ENOTSUP;
	}

	switch (ctx->keylen) {
	case 16U:
#ifdef CONFIG_CRYPTO_MSPM0_AES
		keylen = AES_MSPM0_AESACTL0_KLX_128;
#else
		keylen = AESADV_MSP_CTRL_KEYSIZE_128;
#endif
		break;
	case 32U:
#ifdef CONFIG_CRYPTO_MSPM0_AES
		keylen = AES_MSPM0_AESACTL0_KLX_256;
#else
		keylen = AESADV_MSP_CTRL_KEYSIZE_256;
#endif
		break;
	default:
		LOG_ERR("key size is not supported");
		return -EINVAL;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
#ifdef CONFIG_CRYPTO_MSPM0_AES
		aesconfig = AES_MSPM0_AESACTL0_CMX_ECB |
			    ((op == CRYPTO_CIPHER_OP_ENCRYPT)
			    ? AES_MSPM0_AESACTL0_OPX_ENCRYPT
			    : AES_MSPM0_AESACTL0_OPX_DECRYPT);
#else
		aesconfig = (op == CRYPTO_CIPHER_OP_ENCRYPT) ? AESADV_MSP_CTRL_DIR : 0;
#endif
		ctx->ops.block_crypt_hndlr = crypto_aes_ecb_op;
		break;

	case CRYPTO_CIPHER_MODE_CBC:
#ifdef CONFIG_CRYPTO_MSPM0_AES
		aesconfig = AES_MSPM0_AESACTL0_CMX_CBC |
			    ((op == CRYPTO_CIPHER_OP_ENCRYPT)
			    ? AES_MSPM0_AESACTL0_OPX_ENCRYPT
			    : AES_MSPM0_AESACTL0_OPX_GEN_FIRST_KEY);
#else
		aesconfig = (op == CRYPTO_CIPHER_OP_ENCRYPT) ?
			    (AESADV_MSP_CTRL_DIR | AESADV_MSP_CTRL_CBC)
			    : AESADV_MSP_CTRL_CBC;
#endif
		ctx->ops.cbc_crypt_hndlr = crypto_aes_cbc_op;
		break;

	default:
		LOG_ERR("Mode Not Supported");
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
			session->in_use = true;
			break;
		}
	}

	if (session == NULL) {
		LOG_ERR("All session in use");
		ret = -EBUSY;
		goto out;
	}

	session->aesconfig = aesconfig;
	session->keylen = keylen;
	ctx->drv_sessn_state = session;
	ctx->ops.cipher_mode = mode;
	ctx->device = dev;
	session->op = op;
out:
	k_mutex_unlock(&data->device_mutex);
	return ret;
}

/*
 * AES registers are write-only registers and always read as zero.
 */
static int aes_session_free(const struct device *dev, struct cipher_ctx *ctx)
{
	struct crypto_msp_aes_data *data = dev->data;
	struct msp_aes_session *session;
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
		ret = -EINVAL;
		goto out;
	}

	session->in_use = false;
	ctx->drv_sessn_state = NULL;
	ctx->device = NULL;
out:
	k_mutex_unlock(&data->device_mutex);
	return ret;
}

static int aes_query_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return AES_HW_CAPS;
}

static int crypto_aes_init(const struct device *dev)
{
	const struct crypto_msp_aes_config *config = dev->config;

	if (!(config->regs->pwren & AES_MSP_PWREN_MASK)) {
		config->regs->pwren = AES_MSP_PWREN_KEY | AES_MSP_PWREN_MASK;
	}

	k_busy_wait(k_cyc_to_us_ceil32(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));

#ifdef CONFIG_CRYPTO_MSPM0_AES
	/* disable interrupt */
	config->regs->imask &= ~(AES_MSPM0_IMASK_AESRDY_MASK);

	/* clear interrupt status regs */
	config->regs->iclr |= AES_MSPM0_ICLR_AESRDY_MASK;
#else
	/* disable interrupt */
	config->regs->imask &= ~(AESADV_MSP_IMASK_INT_MASK);

	/* clear interrupt status regs */
	config->regs->iclr |= AESADV_MSP_ICLR_INT_MASK;
#endif
	config->irq_config_func(dev);

#ifdef CONFIG_CRYPTO_MSPM0_AES
	/* enable interrupt */
	config->regs->imask |= AES_MSPM0_IMASK_AESRDY_MASK;
#endif

	return 0;
}

static DEVICE_API(crypto, crypto_enc_funcs) = {
	.cipher_begin_session = aes_session_setup,
	.cipher_free_session = aes_session_free,
	.query_hw_caps = aes_query_caps,
};

#define MSP_AES_DEFINE(n)									\
												\
	static void crypto_msp_irq_config_##n(const struct device *dev)				\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), crypto_msp_aes_isr,	\
			    DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_INST_IRQN(n));							\
	}											\
												\
	static const struct crypto_msp_aes_config crypto_aes_config_##n = {			\
		.regs = COND_CODE_1(CONFIG_CRYPTO_MSPM0_AES,					\
			((aes_ti_mspm0_reg_t *)DT_INST_REG_ADDR(n)),				\
			((aes_adv_ti_msp_reg_t *)DT_INST_REG_ADDR(n))),				\
		.irq_config_func = crypto_msp_irq_config_##n,					\
	};											\
												\
	static struct crypto_msp_aes_data crypto_aes_data_##n = {				\
		.device_mutex = Z_MUTEX_INITIALIZER(crypto_aes_data_##n.device_mutex),		\
		.aes_done = Z_SEM_INITIALIZER(crypto_aes_data_##n.aes_done, 0, 1),		\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, crypto_aes_init, NULL, &crypto_aes_data_##n,			\
			&crypto_aes_config_##n, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,	\
			(void *)&crypto_enc_funcs);

DT_INST_FOREACH_STATUS_OKAY(MSP_AES_DEFINE)
