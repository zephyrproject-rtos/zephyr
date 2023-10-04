/*
 * Copyright (c) 2023 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_symcr

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include "zephyr/sys/util.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(xec_symcr, CONFIG_CRYPTO_LOG_LEVEL);

#include <soc.h>

/* ROM API for Hash without using external files */

enum mchp_rom_hash_alg_id {
	MCHP_ROM_HASH_ALG_NONE = 0,
	MCHP_ROM_HASH_ALG_SHA1,
	MCHP_ROM_HASH_ALG_SHA224,
	MCHP_ROM_HASH_ALG_SHA256,
	MCHP_ROM_HASH_ALG_SHA384,
	MCHP_ROM_HASH_ALG_SHA512,
	MCHP_ROM_HASH_ALG_SM3,
	MCHP_ROM_HASH_ALG_MAX
};

#define MCHP_XEC_STRUCT_HASH_STATE_STRUCT_SIZE	8
#define MCHP_XEC_STRUCT_HASH_STRUCT_SIZE	240

struct mchphashstate {
	uint32_t v[MCHP_XEC_STRUCT_HASH_STATE_STRUCT_SIZE / 4];
};

struct mchphash {
	uint32_t v[MCHP_XEC_STRUCT_HASH_STRUCT_SIZE / 4];
};

#define MCHP_XEC_ROM_API_BASE DT_REG_ADDR(DT_NODELABEL(rom_api))
#define MCHP_XEC_ROM_API_ADDR(n)						\
	(((uint32_t)(MCHP_XEC_ROM_API_BASE) + ((uint32_t)(n) * 4u)) | BIT(0))

#define MCHP_XEC_ROM_HASH_CREATE_SHA224_ID 95
#define mchp_xec_rom_hash_create_sha224						\
	((int (*)(struct mchphash *)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_CREATE_SHA224_ID))

#define MCHP_XEC_ROM_HASH_CREATE_SHA256_ID 96
#define mchp_xec_rom_hash_create_sha256						\
	((int (*)(struct mchphash *)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_CREATE_SHA256_ID))

#define MCHP_XEC_ROM_HASH_CREATE_SHA384_ID 97
#define mchp_xec_rom_hash_create_sha384						\
	((int (*)(struct mchphash *)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_CREATE_SHA384_ID))

#define MCHP_XEC_ROM_HASH_CREATE_SHA512_ID 98
#define mchp_xec_rom_hash_create_sha512						\
	((int (*)(struct mchphash *)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_CREATE_SHA512_ID))


#define MCHP_XEC_ROM_HASH_INIT_STATE_ID 100
#define mec172x_rom_hash_init_state						\
	((void (*)(struct mchphash *, struct mchphashstate *, char *))		\
		MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_INIT_STATE_ID))

#define MCHP_XEC_ROM_HASH_RESUME_STATE_ID 101
#define mchp_xec_rom_hash_resume_state						\
	((void (*)(struct mchphash *, struct mchphashstate *))			\
		MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_RESUME_STATE_ID))

#define MCHP_XEC_ROM_HASH_SAVE_STATE_ID 102
#define mchp_xec_rom_hash_save_state						\
	((int (*)(struct mchphash *)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_SAVE_STATE_ID))

#define MCHP_XEC_ROM_HASH_FEED_ID 103
#define mchp_xec_rom_hash_feed							\
	((int (*)(struct mchphash *, const uint8_t *, size_t))			\
		MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_FEED_ID))

#define MCHP_XEC_ROM_HASH_DIGEST_ID 104
#define mchp_xec_rom_hash_digest						\
	((int (*)(struct mchphash *, char *)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_DIGEST_ID))

#define MCHP_XEC_ROM_HASH_WAIT_ID 105
#define mec172x_rom_hash_wait							\
	((int (*)(struct mchphash *)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_HASH_WAIT_ID))

#define MCHP_XEC_ROM_AH_DMA_INIT_ID 144
#define mchp_xec_rom_ah_dma_init						\
	((int (*)(uint8_t)) MCHP_XEC_ROM_API_ADDR(MCHP_XEC_ROM_AH_DMA_INIT_ID))

#define MCHP_ROM_AH_DMA_INIT_NO_RESET 0
#define MCHP_ROM_AH_DMA_INIT_WITH_RESET 1

#define MCHP_XEC_SYMCR_CAPS_SUPPORT	\
	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)
#define MCHP_XEC_SYMCR_MAX_SESSION	1
#define MCHP_XEC_STATE_BUF_SIZE 256
#define MCHP_XEC_BLOCK_BUF_SIZE 128

struct xec_symcr_hash_session {
	struct mchphash mhctx;
	struct mchphashstate mhstate;
	enum hash_algo algo;
	enum mchp_rom_hash_alg_id rom_algo;
	bool open;
	size_t blksz;
	size_t blklen;
	uint8_t blockbuf[MCHP_XEC_BLOCK_BUF_SIZE] __aligned(4);
	uint8_t statebuf[MCHP_XEC_STATE_BUF_SIZE] __aligned(4);
};

struct xec_symcr_config {
	uint32_t regbase;
	const struct device *clk_dev;
	struct mchp_xec_pcr_clk_ctrl clk_ctrl;
	uint8_t irq_num;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t rsvd1;
};

struct xec_symcr_data {
	struct xec_symcr_hash_session hash_sessions[MCHP_XEC_SYMCR_MAX_SESSION];
};

static int mchp_xec_get_unused_session_index(struct xec_symcr_data *data)
{
	int i;

	for (i = 0; i < MCHP_XEC_SYMCR_MAX_SESSION; i++) {
		if (!data->hash_sessions[i].open) {
			data->hash_sessions[i].open = true;
			return i;
		}
	}

	return -EPERM;
}

struct hash_alg_to_rom {
	enum hash_algo algo;
	enum mchp_rom_hash_alg_id rom_algo;
};

const struct hash_alg_to_rom hash_alg_tbl[] = {
	{ CRYPTO_HASH_ALGO_SHA224, MCHP_ROM_HASH_ALG_SHA224 },
	{ CRYPTO_HASH_ALGO_SHA256, MCHP_ROM_HASH_ALG_SHA256 },
	{ CRYPTO_HASH_ALGO_SHA384, MCHP_ROM_HASH_ALG_SHA384 },
	{ CRYPTO_HASH_ALGO_SHA512, MCHP_ROM_HASH_ALG_SHA512 },
};

static enum mchp_rom_hash_alg_id lookup_hash_alg(enum hash_algo algo)
{
	for (size_t n = 0; n < ARRAY_SIZE(hash_alg_tbl); n++) {
		if (hash_alg_tbl[n].algo == algo) {
			return hash_alg_tbl[n].rom_algo;
		}
	}

	return MCHP_ROM_HASH_ALG_NONE;
}

/* SHA-1, 224, and 256 use block size of 64 bytes
 * SHA-384 and 512 use 128 bytes.
 */
static size_t hash_block_size(enum hash_algo algo)
{
	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA384:
	case CRYPTO_HASH_ALGO_SHA512:
		return 128u;
	default:
		return 64u;
	}
}

static int init_rom_hash_context(enum mchp_rom_hash_alg_id rom_algo, struct mchphash *c)
{
	int ret = 0;

	if (!c) {
		return -EINVAL;
	}

	switch (rom_algo) {
	case MCHP_ROM_HASH_ALG_SHA224:
		ret = mchp_xec_rom_hash_create_sha224(c);
		break;
	case MCHP_ROM_HASH_ALG_SHA256:
		ret = mchp_xec_rom_hash_create_sha256(c);
		break;
	case MCHP_ROM_HASH_ALG_SHA384:
		ret = mchp_xec_rom_hash_create_sha384(c);
		break;
	case MCHP_ROM_HASH_ALG_SHA512:
		ret = mchp_xec_rom_hash_create_sha512(c);
		break;
	default:
		return -EINVAL;
	}

	if (ret) { /* use zephyr return value */
		ret = -EIO;
	}

	return ret;
}

/* use zephyr return values */
int mchp_xec_rom_hash_init_state_wrapper(struct mchphash *c, struct mchphashstate *h,
					 uint8_t *dmamem)
{
	if (!c || !h || !dmamem) {
		return -EINVAL;
	}

	mec172x_rom_hash_init_state(c, h, (char *)dmamem);

	return 0;
}

int mchp_xec_rom_hash_resume_state_wrapper(struct mchphash *c, struct mchphashstate *h)
{
	if (!c || !h) {
		return -EINVAL;
	}

	mchp_xec_rom_hash_resume_state(c, h);
	return 0;
}

int mchp_xec_rom_hash_save_state_wrapper(struct mchphash *c)
{
	if (!c) {
		return -EINVAL;
	}

	if (mchp_xec_rom_hash_save_state(c) != 0) {
		return -EIO;
	}

	return 0;
}

int mchp_xec_rom_hash_feed_wrapper(struct mchphash *c, const uint8_t *msg, size_t sz)
{
	if ((!c) || (!msg && sz)) {
		return -EINVAL;
	}

	if (mchp_xec_rom_hash_feed(c, (const char *)msg, sz) != 0) {
		return -EIO;
	}

	return 0;
}

int mchp_xec_rom_hash_digest_wrapper(struct mchphash *c, uint8_t *digest)
{
	if (!c || !digest) {
		return -EINVAL;
	}

	if (mchp_xec_rom_hash_digest(c, (char *)digest)) {
		return -EIO;
	}

	return 0;
}

/* Wait for hardware to finish.
 * returns 0 if hardware finished with no errors
 * returns -EIO if hardware stopped due to error
 * returns -EINVAL if parameter is bad, hardware may still be running!
 */
int mchp_xec_rom_hash_wait_wrapper(struct mchphash *c)
{
	if (!c) {
		return -EINVAL;
	}

	if (mec172x_rom_hash_wait(c) != 0) {
		return -EIO;
	}

	return 0;
}

/* Called by application for update(finish==false)
 * and compute final hash digest(finish==true)
 */
static int xec_symcr_do_hash(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	struct xec_symcr_hash_session *hs = NULL;
	struct mchphash *c = NULL;
	struct mchphashstate *cstate = NULL;
	size_t fill_len = 0, rem_len = 0;
	int ret = 0;

	if (!ctx || !pkt) {
		return -EINVAL;
	}

	hs = (struct xec_symcr_hash_session *)ctx->drv_sessn_state;
	c = &hs->mhctx;
	cstate = &hs->mhstate;

	if (!hs->open) {
		LOG_ERR("Session not open");
		return -EIO;
	}

	if (!finish && !pkt->in_len) {
		return 0; /* nothing to do */
	}

	/* Not final digest computation and not enough data to run engine */
	if (!finish && ((hs->blklen + pkt->in_len) < hs->blksz)) {
		memcpy(&hs->blockbuf[hs->blklen], pkt->in_buf, pkt->in_len);
		hs->blklen += pkt->in_len;
		return 0;
	}

	ret = init_rom_hash_context(hs->rom_algo, c);
	if (ret) {
		LOG_ERR("ROM context init error %d", ret);
		return ret;
	}
	ret = mchp_xec_rom_hash_resume_state_wrapper(c, cstate);
	if (ret) {
		LOG_ERR("Resume state error %d", ret);
		return ret;
	}

	fill_len = pkt->in_len;
	rem_len = 0;
	if (!finish) {
		rem_len = pkt->in_len & (hs->blksz - 1u);
		fill_len = pkt->in_len & ~(hs->blksz - 1u);
		if (hs->blklen) {
			fill_len = ((hs->blklen + pkt->in_len) & ~(hs->blksz - 1u)) - hs->blklen;
			rem_len = pkt->in_len - fill_len;
		}
	}

	if (hs->blklen) {
		ret = mchp_xec_rom_hash_feed_wrapper(c, (const uint8_t *)hs->blockbuf, hs->blklen);
		if (ret) {
			LOG_ERR("ROM hash feed error %d", ret);
			return ret;
		}
		hs->blklen = 0; /* consumed */
	}

	ret = mchp_xec_rom_hash_feed_wrapper(c, (const uint8_t *)pkt->in_buf, fill_len);
	if (ret) {
		LOG_ERR("ROM hash feed error %d", ret);
		return ret;
	}

	if (finish) {
		ret = mchp_xec_rom_hash_digest_wrapper(c, pkt->out_buf);
		if (ret) {
			LOG_ERR("ROM Hash final error %d", ret);
			return ret;
		}
	} else {
		ret = mchp_xec_rom_hash_save_state(c);
		if (ret) {
			LOG_ERR("ROM hash save state error %d", ret);
			return ret;
		}
	}
	ret = mchp_xec_rom_hash_wait_wrapper(c);
	if (ret) {
		LOG_ERR("ROM hash wait error %d", ret);
		return ret;
	}
	if (finish) {
		hs->blklen = 0;
	} else {
		memcpy(hs->blockbuf, &pkt->in_buf[fill_len], rem_len);
		hs->blklen = rem_len;
	}

	return 0;
}

static int xec_symcr_hash_session_begin(const struct device *dev, struct hash_ctx *ctx,
					enum hash_algo algo)
{
	struct xec_symcr_data *data = dev->data;
	struct xec_symcr_hash_session *hs = NULL;
	struct mchphash *c = NULL;
	struct mchphashstate *cstate = NULL;
	enum mchp_rom_hash_alg_id rom_algo = MCHP_ROM_HASH_ALG_NONE;
	int session_idx = 0;
	int ret = 0;

	if (ctx->flags & ~(MCHP_XEC_SYMCR_CAPS_SUPPORT)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	rom_algo = lookup_hash_alg(algo);
	if (rom_algo == MCHP_ROM_HASH_ALG_NONE) {
		LOG_ERR("Unsupported algo %d", algo);
		return -EINVAL;
	}

	session_idx = mchp_xec_get_unused_session_index(data);
	if (session_idx < 0) {
		LOG_ERR("No session available");
		return -ENOSPC;
	}

	hs = &data->hash_sessions[session_idx];

	hs->algo = algo;
	hs->rom_algo = rom_algo;
	hs->open = false;
	hs->blklen = 0;
	hs->blksz = hash_block_size(algo);

	ctx->drv_sessn_state = hs;
	ctx->started = false;
	ctx->hash_hndlr = xec_symcr_do_hash;

	/* reset HW at beginning of session */
	ret = mchp_xec_rom_ah_dma_init(MCHP_ROM_AH_DMA_INIT_WITH_RESET);
	if (ret) {
		LOG_ERR("ROM HW init error %d", ret);
		return -EIO;
	}

	c = &hs->mhctx;
	cstate = &hs->mhstate;

	ret = init_rom_hash_context(hs->rom_algo, c);
	if (ret) {
		LOG_ERR("ROM HW context init error %d", ret);
		return ret;
	}

	ret = mchp_xec_rom_hash_init_state_wrapper(c, cstate, hs->statebuf);
	if (ret) {
		LOG_ERR("ROM HW init state error %d", ret);
	}

	hs->open = true;

	return ret;
}

/*
 * struct hash_ctx {
 *	const struct device *device; this device driver's instance structure
 *	void *drv_sessn_state; pointer to driver instance struct session state. Defined by driver
 *	hash_op hash_hndlr; pointer to this driver function. App calls via pointer to do operations
 *	bool started; true if multipart hash has been started
 *	uint16_t flags; app populates this before calling hash_begin_session
 * }
 */
static int xec_symcr_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	struct xec_symcr_hash_session *hs = NULL;
	int ret = 0;

	ret = mchp_xec_rom_ah_dma_init(MCHP_ROM_AH_DMA_INIT_WITH_RESET);
	if (ret) {
		ret = -EIO;
		LOG_ERR("ROM HW reset error %d", ret);
	}

	hs = (struct xec_symcr_hash_session *)ctx->drv_sessn_state;

	memset(hs, 0, sizeof(struct xec_symcr_hash_session));

	return ret;
}

static int xec_symcr_query_hw_caps(const struct device *dev)
{
	return MCHP_XEC_SYMCR_CAPS_SUPPORT;
}

static int xec_symcr_init(const struct device *dev)
{
	const struct xec_symcr_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->clk_dev)) {
		LOG_ERR("clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t *)&cfg->clk_ctrl);
	if (ret < 0) {
		LOG_ERR("clock on error %d", ret);
		return ret;
	}

	ret = mchp_xec_rom_ah_dma_init(MCHP_ROM_AH_DMA_INIT_WITH_RESET);
	if (ret) {
		ret = -EIO;
	}

	return ret;
}

static struct crypto_driver_api xec_symcr_api = {
	.query_hw_caps = xec_symcr_query_hw_caps,
	.hash_begin_session = xec_symcr_hash_session_begin,
	.hash_free_session = xec_symcr_hash_session_free,
};

#define XEC_SYMCR_PCR_INFO(i)						\
	MCHP_XEC_PCR_SCR_ENCODE(DT_INST_CLOCKS_CELL(i, regidx),		\
				DT_INST_CLOCKS_CELL(i, bitpos),		\
				DT_INST_CLOCKS_CELL(i, domain))

#define XEC_SYMCR_INIT(inst)								\
											\
	static struct xec_symcr_data xec_symcr_data_##inst;				\
											\
	static const struct xec_symcr_config xec_symcr_cfg_##inst = {			\
		.regbase = DT_INST_REG_ADDR(inst),					\
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),			\
		.clk_ctrl = {								\
			.pcr_info = XEC_SYMCR_PCR_INFO(inst),				\
		},									\
		.irq_num = DT_INST_IRQN(inst),						\
		.girq = DT_INST_PROP_BY_IDX(inst, girqs, 0),				\
		.girq_pos = DT_INST_PROP_BY_IDX(inst, girqs, 1),			\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, &xec_symcr_init, NULL,				\
			      &xec_symcr_data_##inst, &xec_symcr_cfg_##inst,		\
			      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,			\
			      &xec_symcr_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_SYMCR_INIT)
