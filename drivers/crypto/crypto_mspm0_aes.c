/*
 * Copyright 2026 Linumiz
 * Copyright (c) 2026 Texas Instruments Inc.
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

#define AES_MSPM0_PWREN_MASK				BIT(0)
#define AES_MSPM0_PWREN_KEY_MASK			GENMASK(31, 24)
#define AES_MSPM0_PWREN_KEY				FIELD_PREP(AES_MSPM0_PWREN_KEY_MASK, 0x26)

#define AES_MSPM0_IMASK_AESRDY_MASK			BIT(0)
#define AES_MSPM0_ICLR_AESRDY_MASK			BIT(0)

#define AES_MSPM0_AESACTL0_SWRST			BIT(7)
#define AES_MSPM0_AESACTL0_CMEN				BIT(15)
#define AES_MSPM0_AESACTL0_CMX_MASK			GENMASK(6, 5) /* AES cipher mode select */
#define AES_MSPM0_AESACTL0_KLX_MASK			GENMASK(3, 2) /* AES key length */
#define AES_MSPM0_AESACTL0_OPX_MASK			GENMASK(1, 0) /* AES operation */

#define AES_MSPM0_AESACTL0_CMX_ECB			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x0)
#define AES_MSPM0_AESACTL0_CMX_CBC			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x1)
#define AES_MSPM0_AESACTL0_CMX_OFB			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x2)
#define AES_MSPM0_AESACTL0_CMX_CFB			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x3)

#define AES_MSPM0_AESACTL0_KLX_128			FIELD_PREP(AES_MSPM0_AESACTL0_KLX_MASK, 0x0)
#define AES_MSPM0_AESACTL0_KLX_256			FIELD_PREP(AES_MSPM0_AESACTL0_KLX_MASK, 0x2)

#define AES_MSPM0_AESACTL0_OPX_ENCRYPT			FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x0)
#define AES_MSPM0_AESACTL0_OPX_DECRYPT			FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x1)
#define AES_MSPM0_AESACTL0_OPX_GEN_FIRST_KEY		FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x2)
#define AES_MSPM0_AESACTL0_OPX_DECRYPT_FIRST_KEY	FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x3)

#define AES_MSPM0_AESASTAT_KEYWR			BIT(1)

#define AES_HW_CAPS	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)
#define AES_BLOCK_SIZE	16

/*
 * The block cycle for AES module is 300 cycles (MSPM0_AES_BLOCK_CYC)
 * AES_BLOCK_TIMEOUT applies a safety margin i.e. 300 << 1 = 600 cycles
 * K_CYC(AES_BLOCK_TIMEOUT) converts this cycle count to a timeout
 * period in system ticks AES_SEM_TIMEOUT.
 */
#define MSPM0_AES_BLOCK_CYC	300
#define AES_BLOCK_TIMEOUT	(MSPM0_AES_BLOCK_CYC << 1)
#define AES_WAIT_TIMEOUT	K_USEC(10)
#define AES_SEM_TIMEOUT		K_CYC(AES_BLOCK_TIMEOUT)

LOG_MODULE_REGISTER(aes, CONFIG_CRYPTO_LOG_LEVEL);

typedef struct {
	uint32_t reserved0[0x200];
	volatile uint32_t pwren;	/* Power Enable				@0x800h */
	volatile uint32_t rstctl;	/* Reset Control			@0x804h */
	uint32_t reserved1[3];
	volatile uint32_t stat;		/* Status Register			@0x814h */
	uint32_t reserved2[0x200];
	volatile uint32_t pdbgctl;	/* Peripheral Debug Control		@0x1018h */
	uint32_t reserved3[1];
	volatile uint32_t iidx;		/* Interrupt Index Register		@0x1020h */
	uint32_t reserved4[1];
	volatile uint32_t imask;	/* Interrupt Mask			@0x1028h */
	uint32_t reserved5[1];
	volatile uint32_t ris;		/* Raw Interrupt Status			@0x1030h */
	uint32_t reserved6[1];
	volatile uint32_t mis;		/* Masked Interrupt Status		@0x1038h */
	uint32_t reserved7[1];
	volatile uint32_t iset;		/* Interrupt Set			@0x1040h */
	uint32_t reserved8[1];
	volatile uint32_t iclr;		/* Interrupt Clear			@0x1048h */
	uint32_t reserved9[1];
	volatile uint32_t iidx1;	/* Interrupt Index Register DMA0	@0x1050h */
	uint32_t reserved10[1];
	volatile uint32_t imask1;	/* Interrupt Mask DMA0			@0x1058h */
	uint32_t reserved11[1];
	volatile uint32_t ris1;		/* Raw Interrupt Status DMA0		@0x1060h */
	uint32_t reserved12[1];
	volatile uint32_t mis1;		/* Masked Interrupt Status DMA0		@0x1068h */
	uint32_t reserved13[1];
	volatile uint32_t iset1;	/* Interrupt Set DMA0			@0x1070h */
	uint32_t reserved14[1];
	volatile uint32_t iclr1;	/* Interrupt Clear DMA0			@0x1078h */
	uint32_t reserved15[1];
	volatile uint32_t iidx2;	/* Interrupt Index Register DMA1	@0x1080h */
	uint32_t reserved16[1];
	volatile uint32_t imask2;	/* Interrupt Mask DMA1			@0x1088h */
	uint32_t reserved17[1];
	volatile uint32_t ris2;		/* Raw Interrupt Status DMA1		@0x1090h */
	uint32_t reserved18[1];
	volatile uint32_t mis2;		/* Masked Interrupt Status DMA1		@0x1098h */
	uint32_t reserved19[1];
	volatile uint32_t iset2;	/* Interrupt Set DMA1			@0x10A0h */
	uint32_t reserved20[1];
	volatile uint32_t iclr2;	/* Interrupt Clear DMA1			@0x10A8h */
	uint32_t reserved21[1];
	volatile uint32_t iidx3;	/* Interrupt Index Register DMA2	@0x10B0h */
	uint32_t reserved22[1];
	volatile uint32_t imask3;	/* Interrupt Mask DMA2			@0x10B8h */
	uint32_t reserved23[1];
	volatile uint32_t ris3;		/* Raw Interrupt Status DMA2		@0x10C0h */
	uint32_t reserved24[1];
	volatile uint32_t mis3;		/* Masked Interrupt Status DMA2		@0x10C8h */
	uint32_t reserved25[1];
	volatile uint32_t iset3;	/* Interrupt Set DMA2			@0x10D0h */
	uint32_t reserved26[1];
	volatile uint32_t iclr3;	/* Interrupt Clear DMA2			@0x10D8h */
	uint32_t reserved27[1];
	volatile uint32_t evt_mode;	/* Event Mode				@0x10E0h */
	uint32_t reserved28[7];
	volatile uint32_t aesactl0;	/* AES Control Register 0		@0x1100h */
	volatile uint32_t aesactl1;	/* AES Control Register 1		@0x1104h */
	volatile uint32_t aesastat;	/* AES Status Register			@0x1108h */
	volatile uint32_t aesakey;	/* AES Key Register			@0x110Ch */
	volatile uint32_t aesadin;	/* AES Data In Register			@0x1110h */
	volatile uint32_t aesadout;	/* AES Data Out Register		@0x1114h */
	volatile uint32_t aesaxdin;	/* AES XORed Data In Register		@0x1118h */
	volatile uint32_t aesaxin;	/* AES XORed Data In (no trigger)	@0x111Ch */
} aes_ti_mspm0_reg_t;

struct crypto_mspm0_aes_config {
	aes_ti_mspm0_reg_t *regs;
	void (*irq_config_func)(const struct device *dev);
};

struct mspm0_aes_session {
	uint32_t keylen;
	uint32_t aesconfig;
	enum cipher_op op;
	bool in_use;
};

struct crypto_mspm0_aes_data {
	struct mspm0_aes_session sessions[CONFIG_CRYPTO_MSPM0_MAX_SESSION];
	struct k_mutex device_mutex;
	struct k_sem aes_done;
};

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
	uint8_t kl = FIELD_GET(AES_MSPM0_AESACTL0_KLX_MASK, keylen);

	switch (kl) {
	case AES_MSPM0_AESACTL0_KLX_128:
		num_words = 4U;
		break;
	case AES_MSPM0_AESACTL0_KLX_256:
		num_words = 8U;
		break;
	default:
		LOG_ERR("Invalid key length");
		return -EINVAL;
	}

	return aes_load_data_word(&regs->aesakey, key, num_words);
}

static int aes_load_data_in(aes_ti_mspm0_reg_t *regs, const uint8_t *data)
{
	return aes_load_data_word(&regs->aesadin, data, 4U);
}

static int aes_get_data_out(aes_ti_mspm0_reg_t *regs, uint8_t *data)
{
	for (uint8_t i = 0; i < 4U; i++) {
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
	return aes_load_data_word(&regs->aesaxdin, data, 4U);
}

static int aes_load_xor_data_in_without_trigger(aes_ti_mspm0_reg_t *regs, const uint8_t *data)
{
	return aes_load_data_word(&regs->aesaxin, data, 4U);
}

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

	return 0;
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

	k_sem_reset(&data->aes_done);

	ret = aes_hw_init(ctx);
	if (ret != 0) {
		goto cleanup;
	}

	do {
		/* load the block */
		ret = aes_load_data_in(config->regs, &pkt->in_buf[bytes_processed]);
		if (ret != 0) {
			break;
		}

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

		/* read the dataout */
		ret = aes_get_data_out(config->regs, &pkt->out_buf[bytes_processed]);
		if (ret != 0) {
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

	do {
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

		/* wait for AES operation completion */
		ret = k_sem_take(&data->aes_done, AES_SEM_TIMEOUT);
		if (ret != 0) {
			break;
		}

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

static void crypto_mspm0_aes_isr(const struct device *dev)
{
	const struct crypto_mspm0_aes_config *config = dev->config;
	struct crypto_mspm0_aes_data *data = dev->data;
	/* Current interrupt is cleared by the hardware on reading IIDX
	 * register and corresponding interrupt flag in RIS and MIS are
	 * cleared as well.
	 */
	if (!(config->regs->iidx)) {
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
		keylen = AES_MSPM0_AESACTL0_KLX_128;
		break;
	case 32U:
		keylen = AES_MSPM0_AESACTL0_KLX_256;
		break;
	default:
		LOG_ERR("key size is not supported");
		return -EINVAL;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		aesconfig = AES_MSPM0_AESACTL0_CMX_ECB |
			    ((op == CRYPTO_CIPHER_OP_ENCRYPT)
			    ? AES_MSPM0_AESACTL0_OPX_ENCRYPT
			    : AES_MSPM0_AESACTL0_OPX_DECRYPT);
		ctx->ops.block_crypt_hndlr = crypto_aes_ecb_op;
		break;

	case CRYPTO_CIPHER_MODE_CBC:
		aesconfig = AES_MSPM0_AESACTL0_CMX_CBC |
			    ((op == CRYPTO_CIPHER_OP_ENCRYPT)
			    ? AES_MSPM0_AESACTL0_OPX_ENCRYPT
			    : AES_MSPM0_AESACTL0_OPX_GEN_FIRST_KEY);
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
		LOG_ERR("All %d session(s) in use", CONFIG_CRYPTO_MSPM0_MAX_SESSION);
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
	const struct crypto_mspm0_aes_config *config = dev->config;

	if (!(config->regs->pwren & AES_MSPM0_PWREN_MASK)) {
		config->regs->pwren = AES_MSPM0_PWREN_KEY | AES_MSPM0_PWREN_MASK;
	}

	k_busy_wait(k_cyc_to_us_ceil32(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));

	/* disable interrupt */
	config->regs->imask &= ~(AES_MSPM0_IMASK_AESRDY_MASK);

	/* clear interrupt status regs */
	config->regs->iclr |= AES_MSPM0_ICLR_AESRDY_MASK;

	config->irq_config_func(dev);

	/* enable interrupt */
	config->regs->imask |= AES_MSPM0_IMASK_AESRDY_MASK;

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
		.regs = (aes_ti_mspm0_reg_t *)DT_INST_REG_ADDR(n),				\
		.irq_config_func = crypto_mspm0_irq_config_##n,					\
	};											\
												\
	static struct crypto_mspm0_aes_data crypto_aes_data_##n = {				\
		.device_mutex = Z_MUTEX_INITIALIZER(crypto_aes_data_##n.device_mutex),		\
		.aes_done = Z_SEM_INITIALIZER(crypto_aes_data_##n.aes_done, 0, 1),		\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, crypto_aes_init, NULL, &crypto_aes_data_##n,			\
			&crypto_aes_config_##n, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,	\
			(void *)&crypto_enc_funcs);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_AES_DEFINE)
