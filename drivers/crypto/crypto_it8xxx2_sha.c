/*
 * Copyright (c) 2023 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_sha

#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/byteorder.h>
#include <chip_chipregs.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sha_it8xxx2, CONFIG_CRYPTO_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "unsupported sha instance");

#define IT8XXX2_SHA_REGS_BASE DT_REG_ADDR(DT_NODELABEL(sha0))

/* 0x00: Hash Control Register */
#define IT8XXX2_REG_HASHCTRLR    (0)
/* 0x01: SHA256 Hash Base Address 1 Register */
#define IT8XXX2_REG_SHA_HBADDR   (1)
/* 0x02: SHA256 Hash Base Address 2 Register */
#define IT8XXX2_REG_SHA_HBADDR2  (2)

#define IT8XXX2_SHA_START_SHA256 BIT(1)

#define SHA_SHA256_HASH_LEN        32
#define SHA_SHA256_BLOCK_LEN       64
#define SHA_SHA256_K_LEN           256
#define SHA_SHA256_HASH_LEN_WORDS  (SHA_SHA256_HASH_LEN / sizeof(uint32_t))
#define SHA_SHA256_BLOCK_LEN_WORDS (SHA_SHA256_BLOCK_LEN / sizeof(uint32_t))
#define SHA_SHA256_K_LEN_WORDS     (SHA_SHA256_K_LEN / sizeof(uint32_t))

/*
 * This struct is used by the hardware and must be stored in RAM first 4k-byte
 * and aligned on a 256-byte boundary.
 */
struct chip_sha256_ctx {
	union {
		/* W[0] ~ W[15] */
		uint32_t w_sha[SHA_SHA256_BLOCK_LEN_WORDS];
		uint8_t w_input[SHA_SHA256_BLOCK_LEN];
	};
	/* reserved */
	uint32_t reserved1[8];
	/* H[0] ~ H[7] */
	uint32_t h[SHA_SHA256_HASH_LEN_WORDS];
	/* reserved */
	uint32_t reserved2[30];
	uint32_t w_input_index;
	uint32_t total_len;
	/* K[0] ~ K[63] */
	uint32_t k[SHA_SHA256_K_LEN_WORDS];
} __aligned(256);

Z_GENERIC_SECTION(.__sha256_ram_block) struct chip_sha256_ctx chip_ctx;

static const uint32_t sha256_h0[SHA_SHA256_HASH_LEN_WORDS] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c,
	0x1f83d9ab, 0x5be0cd19
};

/*
 * References of K of SHA-256:
 * https://en.wikipedia.org/wiki/SHA-2#Pseudocode
 */
static const uint32_t sha256_k[SHA_SHA256_K_LEN_WORDS] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
	0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
	0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
	0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
	0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
	0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void it8xxx2_sha256_init(bool init_k)
{
	int i;

	chip_ctx.total_len = 0;
	chip_ctx.w_input_index = 0;

	/* Initialize hash values */
	for (i = 0; i < ARRAY_SIZE(sha256_h0); i++) {
		chip_ctx.h[i] = sha256_h0[i];
	}
	/* Initialize array of round constants */
	if (init_k) {
		for (i = 0; i < ARRAY_SIZE(sha256_k); i++) {
			chip_ctx.k[i] = sha256_k[i];
		}
	}
}

static void it8xxx2_sha256_module_calculation(void)
{
	uint32_t key;
	uint8_t hash_ctrl;

	/*
	 * Since W field on it8xxx2 requires big-endian format, change byte
	 * order before computing hash.
	 */
	for (int i = 0; i < SHA_SHA256_BLOCK_LEN_WORDS; i++) {
		chip_ctx.w_sha[i] = sys_cpu_to_be32(chip_ctx.w_sha[i]);
	}
	/*
	 * Global interrupt is disabled because the CPU cannot access memory
	 * via the DLM (Data Local Memory) bus while HW module is computing
	 * hash.
	 */
	key = irq_lock();
	hash_ctrl = sys_read8(IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_HASHCTRLR);
	sys_write8(hash_ctrl | IT8XXX2_SHA_START_SHA256,
			IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_HASHCTRLR);
	hash_ctrl = sys_read8(IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_HASHCTRLR);
	irq_unlock(key);

	chip_ctx.w_input_index = 0;
}

static int it8xxx2_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt,
				bool finish)
{
	uint32_t rem_len = pkt->in_len;
	uint32_t in_buf_idx = 0;

	while (rem_len--) {
		chip_ctx.w_input[chip_ctx.w_input_index++] =
						pkt->in_buf[in_buf_idx++];
		if (chip_ctx.w_input_index >= SHA_SHA256_BLOCK_LEN) {
			it8xxx2_sha256_module_calculation();
		}
	}
	chip_ctx.total_len += pkt->in_len;

	if (finish) {
		uint32_t *ob_ptr = (uint32_t *)pkt->out_buf;

		/* Pre-processing (Padding) */
		memset(&chip_ctx.w_input[chip_ctx.w_input_index],
			0, SHA_SHA256_BLOCK_LEN - chip_ctx.w_input_index);
		chip_ctx.w_input[chip_ctx.w_input_index] = 0x80;

		if (chip_ctx.w_input_index >= 56) {
			it8xxx2_sha256_module_calculation();
			memset(&chip_ctx.w_input[chip_ctx.w_input_index],
			0, SHA_SHA256_BLOCK_LEN - chip_ctx.w_input_index);
		}
		chip_ctx.w_sha[15] = sys_cpu_to_be32(chip_ctx.total_len * 8);
		it8xxx2_sha256_module_calculation();

		for (int i = 0; i < SHA_SHA256_HASH_LEN_WORDS; i++) {
			ob_ptr[i] = sys_be32_to_cpu(chip_ctx.h[i]);
		}

		it8xxx2_sha256_init(false);
	}

	return 0;
}

static int it8xxx2_hash_session_free(const struct device *dev,
				struct hash_ctx *ctx)
{
	it8xxx2_sha256_init(false);

	return 0;
}

static inline int it8xxx2_query_hw_caps(const struct device *dev)
{
	return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int it8xxx2_hash_begin_session(const struct device *dev,
				struct hash_ctx *ctx, enum hash_algo algo)
{
	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_ERR("Unsupported algo");
		return -EINVAL;
	}

	if (ctx->flags & ~(it8xxx2_query_hw_caps(dev))) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	it8xxx2_sha256_init(false);
	ctx->hash_hndlr = it8xxx2_hash_handler;

	return 0;
}

static int it8xxx2_sha_init(const struct device *dev)
{
	it8xxx2_sha256_init(true);
	/* Configure base address register for W and H */
	sys_write8(((uint32_t)&chip_ctx >> 6) & 0xfc,
			IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHA_HBADDR);
	/* Configure base address register for K */
	sys_write8(((uint32_t)&chip_ctx.k >> 6) & 0xfc,
			IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHA_HBADDR2);

	return 0;
}

static DEVICE_API(crypto, it8xxx2_crypto_api) = {
	.hash_begin_session = it8xxx2_hash_begin_session,
	.hash_free_session = it8xxx2_hash_session_free,
	.query_hw_caps = it8xxx2_query_hw_caps,
};

DEVICE_DT_INST_DEFINE(0, &it8xxx2_sha_init, NULL, NULL, NULL, POST_KERNEL,
			CONFIG_CRYPTO_INIT_PRIORITY, &it8xxx2_crypto_api);
