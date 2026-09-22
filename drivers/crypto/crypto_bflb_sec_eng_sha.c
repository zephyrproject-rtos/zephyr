/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bouffalo Lab SEC Engine SHA hash driver
 */

#define DT_DRV_COMPAT bflb_sec_eng_sha

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/crypto/hash.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <bouffalolab/common/sec_eng_reg.h>

LOG_MODULE_REGISTER(crypto_bflb_sha, CONFIG_CRYPTO_LOG_LEVEL);

#define SHA_TIMEOUT_US          100000
#define SHA256_BLOCK_SIZE       (512 / BITS_PER_BYTE)
#define SHA512_BLOCK_SIZE       (1024 / BITS_PER_BYTE)
#define SHA_FINAL_PAD_BUF_SIZE  (SHA512_BLOCK_SIZE * 2)
#define BFLB_SHA_PAD_START_BYTE 0x80U

/* SHA mode values (for MODE field) */
#define SHA_MODE_SHA256 0
#define SHA_MODE_SHA224 1
#define SHA_MODE_SHA1   2
#define SHA_MODE_SHA512 4
#define SHA_MODE_SHA384 5

/*
 * HAL register offsets are relative to SEC_ENG_BASE. The DT node provides a
 * named "sec_eng" register region with that base address.
 */
#define SEC_ENG_SHA_BASE(inst) DT_INST_REG_ADDR_BY_NAME(inst, sec_eng)

struct bflb_sha_session {
	bool in_use;
	enum hash_algo algo;
	uint8_t sha_buf[SHA512_BLOCK_SIZE]; /* partial block buffer */
	uint32_t buf_pos;
	uint64_t total_len;
	bool started;
};

struct crypto_bflb_sha_data {
	struct k_mutex lock;
	struct bflb_sha_session sessions[CONFIG_CRYPTO_BFLB_SEC_ENG_SHA_MAX_SESSION];
};

struct crypto_bflb_sha_config {
	uintptr_t base;
};

/* Helpers */

static int bflb_sha_wait_not_busy(uintptr_t base)
{
	uint32_t timeout = SHA_TIMEOUT_US;

	while (timeout--) {
		if (!(sys_read32(base + SEC_ENG_SE_SHA_0_CTRL_OFFSET) & SEC_ENG_SE_SHA_0_BUSY)) {
			return 0;
		}
		k_busy_wait(1);
	}
	return -ETIMEDOUT;
}

static int bflb_sha_block_size(enum hash_algo algo)
{
	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA384:
	case CRYPTO_HASH_ALGO_SHA512:
		return SHA512_BLOCK_SIZE;
	default:
		return SHA256_BLOCK_SIZE;
	}
}

static int bflb_sha_digest_size(enum hash_algo algo)
{
	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		return 28;
	case CRYPTO_HASH_ALGO_SHA256:
		return 32;
	case CRYPTO_HASH_ALGO_SHA384:
		return 48;
	case CRYPTO_HASH_ALGO_SHA512:
		return 64;
	default:
		return -ENOTSUP;
	}
}

static uint8_t bflb_sha_hw_mode(enum hash_algo algo)
{
	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA256:
		return SHA_MODE_SHA256;
	case CRYPTO_HASH_ALGO_SHA224:
		return SHA_MODE_SHA224;
	case CRYPTO_HASH_ALGO_SHA384:
		return SHA_MODE_SHA384;
	case CRYPTO_HASH_ALGO_SHA512:
		return SHA_MODE_SHA512;
	default:
		return 0;
	}
}

static bool bflb_sha_algo_supported(enum hash_algo algo)
{
#if defined(CONFIG_SOC_SERIES_BL61X)
	/* BL61x SEC_ENG supports SHA-224/256/384/512. */
	return (algo == CRYPTO_HASH_ALGO_SHA224 || algo == CRYPTO_HASH_ALGO_SHA256 ||
		algo == CRYPTO_HASH_ALGO_SHA384 || algo == CRYPTO_HASH_ALGO_SHA512);
#else
	/* BL60x/BL70x SEC_ENG supports SHA-224/256 only. */
	return (algo == CRYPTO_HASH_ALGO_SHA224 || algo == CRYPTO_HASH_ALGO_SHA256);
#endif
}

/* SHA implementation */

static int bflb_sha_process_blocks(uintptr_t base, const uint8_t *data, uint32_t nblocks,
				   size_t len, bool is_first, uint8_t hw_mode)
{
	uint32_t regval;

	regval = sys_read32(base + SEC_ENG_SE_SHA_0_CTRL_OFFSET);

	/* Set mode */
	regval &= ~SEC_ENG_SE_SHA_0_MODE_MASK;
	regval &= ~SEC_ENG_SE_SHA_0_MODE_EXT_MASK;
	regval |= (hw_mode << SEC_ENG_SE_SHA_0_MODE_SHIFT);

	/* Enable SHA */
	regval |= SEC_ENG_SE_SHA_0_EN;

	/* Set HASH_SEL for continuation (not first block) */
	if (is_first) {
		regval &= ~SEC_ENG_SE_SHA_0_HASH_SEL;
	} else {
		regval |= SEC_ENG_SE_SHA_0_HASH_SEL;
	}

	/* Set source address */
	sys_write32((uint32_t)(uintptr_t)data, base + SEC_ENG_SE_SHA_0_MSA_OFFSET);

	/* Set block count */
	regval &= ~SEC_ENG_SE_SHA_0_MSG_LEN_MASK;
	regval |= (nblocks << SEC_ENG_SE_SHA_0_MSG_LEN_SHIFT);
	sys_write32(regval, base + SEC_ENG_SE_SHA_0_CTRL_OFFSET);

	/* The SHA engine reads autonomously from RAM, which is cached.
	 * This is handled automatically on L1C SoCs (BL60x, BL70x/L)
	 * But on XTHeadCMO SoCs (BL61x) this requires manual cache handling
	 */
	sys_cache_data_flush_range((void *)data, len);

	/* Trigger */
	regval |= SEC_ENG_SE_SHA_0_TRIG_1T;
	sys_write32(regval, base + SEC_ENG_SE_SHA_0_CTRL_OFFSET);

	return bflb_sha_wait_not_busy(base);
}

static void bflb_sha_read_digest(uintptr_t base, uint8_t *output, enum hash_algo algo)
{
	int digest_len = bflb_sha_digest_size(algo);
	bool is_sha512_family =
		(algo == CRYPTO_HASH_ALGO_SHA384 || algo == CRYPTO_HASH_ALGO_SHA512);

	if (is_sha512_family) {
		int nwords = digest_len / sizeof(uint32_t);

		for (int i = 0; i < nwords / 2; i++) {
			sys_put_le32(sys_read32(base + SEC_ENG_SE_SHA_0_HASH_H_0_OFFSET +
						(i * sizeof(uint32_t))),
				     output);
			output += sizeof(uint32_t);
			sys_put_le32(sys_read32(base + SEC_ENG_SE_SHA_0_HASH_L_0_OFFSET +
						(i * sizeof(uint32_t))),
				     output);
			output += sizeof(uint32_t);
		}
	} else {
		int nwords = digest_len / sizeof(uint32_t);

		for (int i = 0; i < nwords; i++) {
			sys_put_le32(sys_read32(base + SEC_ENG_SE_SHA_0_HASH_L_0_OFFSET +
						(i * sizeof(uint32_t))),
				     output);
			output += sizeof(uint32_t);
		}
	}
}

static int bflb_sha_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct device *dev = ctx->device;
	struct crypto_bflb_sha_data *data = dev->data;
	const struct crypto_bflb_sha_config *cfg = dev->config;
	struct bflb_sha_session *sess = ctx->drv_sessn_state;
	uintptr_t base = cfg->base;
	int block_sz = bflb_sha_block_size(sess->algo);
	const uint8_t *input = pkt->in_buf;
	const uint32_t input_len = pkt->in_len;
	uint32_t len = pkt->in_len;
	int ret = 0;
	uint8_t hw_mode = bflb_sha_hw_mode(sess->algo);

	k_mutex_lock(&data->lock, K_FOREVER);

	uint32_t fill = block_sz - sess->buf_pos;

	/* If we have buffered data and new input fills a block */
	if (sess->buf_pos > 0 && len >= fill) {
		memcpy(sess->sha_buf + sess->buf_pos, input, fill);

		ret = bflb_sha_process_blocks(base, sess->sha_buf, 1, block_sz, !sess->started,
					      hw_mode);
		if (ret) {
			goto out;
		}
		sess->started = true;
		input += fill;
		len -= fill;
		sess->buf_pos = 0;
	}

	/* Process full blocks from input */
	uint32_t full_blocks = len / block_sz;

	if (full_blocks > 0) {
		ret = bflb_sha_process_blocks(base, input, full_blocks, full_blocks * block_sz,
					      !sess->started, hw_mode);
		if (ret) {
			goto out;
		}
		sess->started = true;
		input += full_blocks * block_sz;
		len -= full_blocks * block_sz;
	}

	/* Buffer remaining bytes */
	if (len > 0) {
		memcpy(sess->sha_buf + sess->buf_pos, input, len);
		sess->buf_pos += len;
	}

	/* Finalize if requested */
	if (finish) {
		uint8_t pad_buf[SHA_FINAL_PAD_BUF_SIZE];
		uint32_t pad_len = sess->buf_pos;
		uint64_t bit_len = (sess->total_len + input_len) * BITS_PER_BYTE;

		memcpy(pad_buf, sess->sha_buf, sess->buf_pos);
		pad_buf[pad_len++] = BFLB_SHA_PAD_START_BYTE;

		if (block_sz == SHA256_BLOCK_SIZE) {
			while (pad_len % SHA256_BLOCK_SIZE !=
			       (SHA256_BLOCK_SIZE - sizeof(uint64_t))) {
				pad_buf[pad_len++] = 0;
			}
			sys_put_be64(bit_len, pad_buf + pad_len);
			pad_len += sizeof(uint64_t);
		} else {
			while (pad_len % SHA512_BLOCK_SIZE !=
			       (SHA512_BLOCK_SIZE - (2 * sizeof(uint64_t)))) {
				pad_buf[pad_len++] = 0;
			}
			/* 128-bit length (upper 64 bits = 0) */
			memset(pad_buf + pad_len, 0, sizeof(uint64_t));
			pad_len += sizeof(uint64_t);
			sys_put_be64(bit_len, pad_buf + pad_len);
			pad_len += sizeof(uint64_t);
		}

		ret = bflb_sha_process_blocks(base, pad_buf, pad_len / block_sz, pad_len,
					      !sess->started, hw_mode);
		if (ret) {
			goto out;
		}

		bflb_sha_read_digest(base, pkt->out_buf, sess->algo);

		/* Disable SHA */
		uint32_t regval = sys_read32(base + SEC_ENG_SE_SHA_0_CTRL_OFFSET);

		regval &= ~SEC_ENG_SE_SHA_0_HASH_SEL;
		regval &= ~SEC_ENG_SE_SHA_0_EN;
		sys_write32(regval, base + SEC_ENG_SE_SHA_0_CTRL_OFFSET);

		/* Reset session for potential reuse */
		sess->buf_pos = 0;
		sess->total_len = 0;
		sess->started = false;
	} else {
		/* Commit message length only when non-final update succeeds. */
		sess->total_len += input_len;
	}

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

/* Crypto API */

static int crypto_bflb_sha_query_caps(const struct device *dev)
{
	return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int crypto_bflb_sha_begin_session(const struct device *dev, struct hash_ctx *ctx,
					 enum hash_algo algo)
{
	struct crypto_bflb_sha_data *data = dev->data;
	struct bflb_sha_session *sess = NULL;

	if (!bflb_sha_algo_supported(algo)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (int i = 0; i < CONFIG_CRYPTO_BFLB_SEC_ENG_SHA_MAX_SESSION; i++) {
		if (!data->sessions[i].in_use) {
			sess = &data->sessions[i];
			break;
		}
	}

	if (!sess) {
		k_mutex_unlock(&data->lock);
		return -ENOSPC;
	}

	memset(sess, 0, sizeof(*sess));
	sess->in_use = true;

	k_mutex_unlock(&data->lock);

	sess->algo = algo;

	ctx->drv_sessn_state = sess;
	ctx->device = dev;
	ctx->hash_hndlr = bflb_sha_hash_handler;
	ctx->started = false;

	return 0;
}

static int crypto_bflb_sha_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	struct crypto_bflb_sha_data *data = dev->data;
	struct bflb_sha_session *sess = ctx->drv_sessn_state;

	if (sess) {
		k_mutex_lock(&data->lock, K_FOREVER);
		sess->in_use = false;
		k_mutex_unlock(&data->lock);
	}

	return 0;
}

static int crypto_bflb_sha_init(const struct device *dev)
{
	struct crypto_bflb_sha_data *data = dev->data;

	k_mutex_init(&data->lock);

	return 0;
}

static DEVICE_API(crypto, crypto_bflb_sha_api) = {
	.query_hw_caps = crypto_bflb_sha_query_caps,
	.hash_begin_session = crypto_bflb_sha_begin_session,
	.hash_free_session = crypto_bflb_sha_free_session,
};

#define BFLB_SHA_INIT(inst)                                                                        \
	static struct crypto_bflb_sha_data crypto_bflb_sha_data_##inst;                            \
	static const struct crypto_bflb_sha_config crypto_bflb_sha_config_##inst = {               \
		.base = SEC_ENG_SHA_BASE(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, crypto_bflb_sha_init, NULL, &crypto_bflb_sha_data_##inst,      \
			      &crypto_bflb_sha_config_##inst, POST_KERNEL,                         \
			      CONFIG_CRYPTO_INIT_PRIORITY, &crypto_bflb_sha_api);

DT_INST_FOREACH_STATUS_OKAY(BFLB_SHA_INIT)
