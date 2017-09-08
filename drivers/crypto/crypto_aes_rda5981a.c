/*
* Copyright (c) 2017 RDA
*
* SPDX-License-Identifier: Apache-2.0
*/

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG

#include <logging/sys_log.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <string.h>
#include <device.h>
#include <i2c.h>
#include <assert.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <crypto/cipher.h>
#include <drivers/rda5981a_dma.h>
#include <misc/byteorder.h>

#include "crypto_aes_rda5981a.h"

struct tc_rda5981a_drv_state aes_data;

volatile struct dma_cfg_rda5981a *dma_cfg = (volatile struct dma_cfg_rda5981a *)RDA_DMACFG_BASE;
volatile struct scu_ctrl *scu_cfg = (volatile struct scu_ctrl *)RDA_SCU_BASE;

#define SCU_CLK_GATE0_ENABLE()           do { \
			scu_cfg->clkgate0 |= BIT(18); \
			__DSB(); \
		} while (0)

#define SCU_CLK_GATE0_DISABLE()          do { \
			scu_cfg->clkgate0 &= ~BIT(18); \
			__DSB(); \
		} while (0)

#define RDA_AES_CTRL_SRC_ADDR          (DMA_CTL_SRC_ADDR_INC << 0)
#define RDA_AES_CTRL_DST_ADDR          (DMA_CTL_DST_ADDR_INC << 1)
#define RDA_AES_CTRL_HSIZEM            (DMA_CTL_HSM_4BYTES << 2)

typedef enum _aes_mode {
	ECB_MODE  = 0,
	CBC_MODE  = 1
} aes_mode;

#define _AES_ENG_CLK_ENABLE()             do { \
			dma_cfg->dma_func_ctrl |= BIT(24); \
		} while (0)
#define AES_ENG_CLK_DISABLE()            do { \
			dma_cfg->dma_func_ctrl &= ~BIT(24);  \
		} while (0)

#define AES_KEY_GEN_CLK_ENABLE()         do { \
			dma_cfg->dma_func_ctrl |= BIT(25); \
		} while (0)

#define AES_KEY_GEN_CLK_DISABLE()         do { \
			dma_cfg->dma_func_ctrl &= ~BIT(25); \
		} while (0)

#define AESMEM_SECTION __attribute__((section(".AHB1SMEM0"), aligned))
#define AES_SHARED_MEMORY_MAX_SIZE  (1024)

static uint8_t AES_SHARE_MEM_ADDR_START AESMEM_SECTION;
static uint8_t *p_left    = &AES_SHARE_MEM_ADDR_START;
static uint32_t  size_left = AES_SHARED_MEMORY_MAX_SIZE;

/* The last malloc must be freed first ! */
static void aes_free_s(void *p, uint32_t size)
{
	if (p_left == (uint8_t *)p + size) {
		size_left += size;
		p_left -= size;
	}
}
static void *aes_malloc_s(uint32_t size)
{
	void *p = p_left;

	if (size > size_left) {
		return (void *)0;
	}

	size_left -= size;
	p_left    += size;

	return (void *)p;
}

static void rda_aes_setkey(uint32_t *rk_buf, uint32_t aes_mode)
{
	/* mode is the save as before, could be CBC_MODE or ECB_MODE */
	dma_cfg->aes_key0 = rk_buf[0];
	dma_cfg->aes_key1 = rk_buf[1];
	dma_cfg->aes_key2 = rk_buf[2];
	dma_cfg->aes_key3 = rk_buf[3];

	dma_cfg->aes_mode = aes_mode | RDA_AES_KEY_TART;
	dma_cfg->aes_mode = aes_mode;
}
/* set aes IV, this is used by CBC mode */
static void rda_aes_setiv(const uint8_t *iv)
{
	int len = RDA_AES_IV_LENGTH/sizeof(uint32_t);
	uint32_t iv_buf[len];

	for (int i = 0; i < len; i++) {
		iv_buf[len - 1 - i] = sys_get_be32(&iv[i << 2]);
	}

	/* iv 128 bits */
	dma_cfg->aes_iv0 = iv_buf[0];
	dma_cfg->aes_iv1 = iv_buf[1];
	dma_cfg->aes_iv2 = iv_buf[2];
	dma_cfg->aes_iv3 = iv_buf[3];
}

/* start aes, mode(enc / dec) */
static void rda_aes_start(dma_mode mode)
{
	uint32_t dma_ctrl_val  = 0;

	dma_ctrl_val = RDA_AES_CTRL_SRC_ADDR | RDA_AES_CTRL_DST_ADDR | \
				   RDA_AES_CTRL_HSIZEM | (mode << 28);

	dma_cfg->dma_ctrl = dma_ctrl_val;
	__DSB();
}

int rda_aes_crypt(struct device *dev, unsigned int *rk_buf,
			dma_mode mode, /* DEC/ENC*/
			unsigned int length, /* bytes */
			unsigned char *iv, /*CBC only*/
			const unsigned char *input,
			unsigned char *output)
{
	int32_t i, int_status = 0;
	struct tc_rda5981a_drv_state *data = dev->driver_data;

	uint32_t *a0 = (uint32_t *)aes_malloc_s(length);
	uint32_t *a1 = (uint32_t *)aes_malloc_s(length);
	memset(a0, 0, length);
	memset(a1, 0, length);

	k_sem_take(&data->device_sem, K_FOREVER);

	/* Input data */
	uint32_t *temp = (uint32_t *)input;
	/* A aes block = (32-bit words key * 32-bit words data)*/
	for (i = 0; i < length / 4; i++) {
		a0[i] = __REV(temp[i]);
	}

	SCU_CLK_GATE0_ENABLE();
	AES_KEY_GEN_CLK_ENABLE();
	if (NULL != iv) {
		rda_aes_setkey(rk_buf, CBC_MODE);
		rda_aes_setiv(iv);
	} else {
		rda_aes_setkey(rk_buf, ECB_MODE);
	}

	dma_cfg->dma_src = (uint32_t)a0;
	dma_cfg->dma_dst = (uint32_t)a1;
	dma_cfg->dma_len = length / 4;

	rda_aes_start(mode);
	AES_KEY_GEN_CLK_DISABLE();

	while (1) {
		int_status = dma_cfg->dma_int_out;
		if (int_status & AHB_DMA_DONE)
			break;
	}
	dma_cfg->dma_int_out |= AHB_DMA_DONE;

	SCU_CLK_GATE0_DISABLE();

	/* Output data */
	temp = (uint32_t *)output;
	for (i = 0; i < length / 4; i++) {
		temp[i] = __REV(a1[i]);
	}

	aes_free_s(a1, length);
	aes_free_s(a0, length);

	k_sem_give(&data->device_sem);

	return 0;
}

static int do_block(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	struct device *dev = ctx->device;

	return rda_aes_crypt(dev, (unsigned int *)ctx->key.bit_stream,
		ECB_MODE, pkt->in_len, NULL, pkt->in_buf, pkt->out_buf);
}

static int do_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *op,
			  uint8_t *iv)
{
	struct device *dev = ctx->device;

	return rda_aes_crypt(dev, (unsigned int *)ctx->key.bit_stream,
		RDA_AES_ENC_MODE, op->in_len, iv, op->in_buf, op->out_buf);
}

static int do_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *op,
			  uint8_t *iv)
{
	struct device *dev = ctx->device;

	/* TinyCrypt expects the IV and cipher text to be in a contiguous
	 * buffer for efficieny
	 */
	if (iv != op->in_buf) {
		return -EIO;
	}

	return rda_aes_crypt(dev, (unsigned int *)ctx->key.bit_stream,
		RDA_AES_DEC_MODE, op->in_len, iv, op->in_buf, op->out_buf);
}

static int rdaes_session_setup(struct device *dev, struct cipher_ctx *ctx,
		     enum cipher_algo algo, enum cipher_mode mode,
		     enum cipher_op op_type)
{
	struct tc_rda5981a_drv_state *data = dev->driver_data;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		return -EINVAL;
	}

	/* TinyCrypt being a software library, only synchronous operations
	 * make sense.
	 */
	if (!(ctx->flags & CAP_SYNC_OPS)) {
		return -EINVAL;
	}

	if (ctx->keylen != RDA_AES_KEY_SIZE) {
		/* TinyCrypt supports only 128 bits */
		return -EINVAL;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = do_block;
			break;

		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = do_cbc_encrypt;
			break;

		default:
			return -EINVAL;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = do_block;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = do_cbc_decrypt;
			break;
		default:
			return -EINVAL;
		}
	}

	ctx->ops.cipher_mode = mode;

	data->in_use = 1;

	return 0;
}

static int rdaes_query_caps(struct device *dev)
{
	return (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}


static int rdaes_session_free(struct device *dev, struct cipher_ctx *sessn)
{
	struct tc_rda5981a_drv_state *data = dev->driver_data;

	data->in_use = 0;

	return 0;
}

static int aes_rda5981a_init(struct device *dev)
{
	struct tc_rda5981a_drv_state *data = dev->driver_data;

	data->in_use = 0;

	k_sem_init(&data->device_sem, 0, UINT_MAX);
	k_sem_give(&data->device_sem);

	return 0;
}

static struct crypto_driver_api rdaes_enc_funcs = {
	.begin_session = rdaes_session_setup,
	.free_session = rdaes_session_free,
	.crypto_async_callback_set = NULL,
	.query_hw_caps = rdaes_query_caps,
};

DEVICE_AND_API_INIT(aes_rda5981a, CONFIG_CRYPTO_RDA_NAME,
		    &aes_rda5981a_init, &aes_data, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		   &rdaes_enc_funcs);
