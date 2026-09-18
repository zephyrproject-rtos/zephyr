/*
 * Copyright (c) 2025 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_sha

#include <errno.h>
#include <it51xxx/chip_chipregs.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_it51xxx_sha, CONFIG_CRYPTO_LOG_LEVEL);

/* 0x00: SHA Control Register (SHACR) */
#define IT51XXX_SHACR           0x00
#define IT51XXX_SHAWB           BIT(2)
#define IT51XXX_SHAINI          BIT(1)
#define IT51XXX_SHAEXE          BIT(0)
/* 0x01: SHA Status Register (SHASR)*/
#define IT51XXX_SHASR           0x01
#define IT51XXX_SHAIE           BIT(3)
#define IT51XXX_SHAIS           BIT(2)
#define IT51XXX_SHABUSY         BIT(0)
/* 0x02: SHA Execution Counter Register (SHAECR) */
#define IT51XXX_SHAECR          0x02
#define IT51XXX_SHAEXEC_64_BYTE 0x00
#define IT51XXX_SHAEXEC_1K_BYTE 0x0F
/* 0x03: SHA DLM Base Address 0 Register (SHADBA0R) */
#define IT51XXX_SHADBA0R        0x03
/* 0x04: SHA DLM Base Address 1 Register (SHADBA1R) */
#define IT51XXX_SHADBA1R        0x04
/* 0x05: SHA Setting Register (SHASETR) */
#define IT51XXX_SHASETR         0x05
#define IT51XXX_SHA256          0x00
/* 0x06: SHA DLM Base Address 2 Register (SHADBA2R) */
#define IT51XXX_SHADBA2R        0x06

#define SHA_SHA256_HASH_LEN        32
#define SHA_SHA256_BLOCK_LEN       64
#define SHA_SHA256_HASH_LEN_WORDS  (SHA_SHA256_HASH_LEN / sizeof(uint32_t))
#define SHA_SHA256_BLOCK_LEN_WORDS (SHA_SHA256_BLOCK_LEN / sizeof(uint32_t))

/*
 * If the input message is more than 1K bytes, taking 10K bytes for example,
 * it should run 10 times SHA hardwired loading and execution, and process 1K bytes each time.
 */
#define SHA_HW_MAX_INPUT_LEN       1024
#define SHA_HW_MAX_INPUT_LEN_WORDS (SHA_HW_MAX_INPUT_LEN / sizeof(uint32_t))

#define IT51XXX_SHA_INIT 1U

/*
 * This struct is used by the hardware and must be stored in RAM first 4k-byte
 * and aligned on a 256-byte boundary.
 */
struct chip_sha256_ctx {
	union {
		/* SHA data buffer */
		uint32_t w_sha[SHA_HW_MAX_INPUT_LEN_WORDS];
		uint8_t w_input[SHA_HW_MAX_INPUT_LEN];
	};
	/* H[0] ~ H[7] */
	uint32_t h[SHA_SHA256_HASH_LEN_WORDS];
	uint32_t sha_init;
	uint32_t w_input_index;
	uint32_t total_len;
} __aligned(256);

Z_GENERIC_SECTION(.__hwcrypto_dlm_block) struct chip_sha256_ctx chip_ctx;

struct it51xxx_sha_config {
	mm_reg_t base;
};

struct it51xxx_sha_data {
	/* semaphore for sha execution or writing back hash complete */
	struct k_sem sha_done;
};

static void it51xxx_sha256_init(const struct device *dev)
{
	const struct it51xxx_sha_config *config = dev->config;

	chip_ctx.sha_init = IT51XXX_SHA_INIT;
	chip_ctx.total_len = 0;
	chip_ctx.w_input_index = 0;

	/* Set DLM address for input data */
	sys_write8(((uint32_t)&chip_ctx) & 0xC0, config->base + IT51XXX_SHADBA0R);
	sys_write8(((uint32_t)&chip_ctx) >> 8, config->base + IT51XXX_SHADBA1R);
	sys_write8(((uint32_t)&chip_ctx) >> 16, config->base + IT51XXX_SHADBA2R);
}

static void it51xxx_sha256_module_calculation(const struct device *dev)
{
	const struct it51xxx_sha_config *config = dev->config;
	struct it51xxx_sha_data *data = dev->data;

	if (chip_ctx.sha_init) {
		chip_ctx.sha_init = 0;
		sys_write8(IT51XXX_SHAINI | IT51XXX_SHAEXE, config->base + IT51XXX_SHACR);
	} else {
		sys_write8(IT51XXX_SHAEXE, config->base + IT51XXX_SHACR);
	}

	k_sem_take(&data->sha_done, K_FOREVER);

	chip_ctx.w_input_index = 0;
}

static int it51xxx_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct it51xxx_sha_config *config = ctx->device->config;
	struct it51xxx_sha_data *data = ctx->device->data;
	uint32_t in_buf_idx = 0;
	uint32_t rem_len = pkt->in_len;

	/* data length >= 1KiB */
	while (rem_len >= SHA_HW_MAX_INPUT_LEN) {
		rem_len = rem_len - SHA_HW_MAX_INPUT_LEN;

		memcpy(&chip_ctx.w_input[chip_ctx.w_input_index], &pkt->in_buf[in_buf_idx],
		       SHA_HW_MAX_INPUT_LEN);
		chip_ctx.w_input_index += SHA_HW_MAX_INPUT_LEN;
		in_buf_idx += SHA_HW_MAX_INPUT_LEN;

		/* HW automatically load 1KB data from DLM */
		sys_write8(IT51XXX_SHAEXEC_1K_BYTE, config->base + IT51XXX_SHAECR);
		while (sys_read8(config->base + IT51XXX_SHASR) & IT51XXX_SHABUSY) {
		};

		it51xxx_sha256_module_calculation(ctx->device);
	}

	/* 0 <= data length < 1KiB */
	while (rem_len) {
		rem_len--;
		chip_ctx.w_input[chip_ctx.w_input_index++] = pkt->in_buf[in_buf_idx++];

		/*
		 * If fill full 64 bytes then execute HW calculation.
		 * If not, will execute in later finish block.
		 */
		if (chip_ctx.w_input_index >= SHA_SHA256_BLOCK_LEN) {
			/* HW automatically load 64 bytes data from DLM */
			sys_write8(IT51XXX_SHAEXEC_64_BYTE, config->base + IT51XXX_SHAECR);
			while (sys_read8(config->base + IT51XXX_SHASR) & IT51XXX_SHABUSY) {
			};

			it51xxx_sha256_module_calculation(ctx->device);
		}
	}

	chip_ctx.total_len += pkt->in_len;

	if (finish) {
		uint32_t *out_buf_ptr = (uint32_t *)pkt->out_buf;

		/* Pre-processing (Padding) */
		chip_ctx.w_input[chip_ctx.w_input_index++] = 0x80;
		memset(&chip_ctx.w_input[chip_ctx.w_input_index], 0,
		       SHA_SHA256_BLOCK_LEN - chip_ctx.w_input_index);

		/*
		 * Handles the boundary case of rest data:
		 * Because the last eight bytes are bit length field of SHA256 rule.
		 * If the data index >= 56, it needs to trigger HW to calculate,
		 * then fill 0 data and the last eight bytes bit length, and calculate
		 * again.
		 */
		if (chip_ctx.w_input_index >= 56) {
			/* HW automatically load 64 bytes data from DLM */
			sys_write8(IT51XXX_SHAEXEC_64_BYTE, config->base + IT51XXX_SHAECR);
			while (sys_read8(config->base + IT51XXX_SHASR) & IT51XXX_SHABUSY) {
			};

			it51xxx_sha256_module_calculation(ctx->device);

			memset(&chip_ctx.w_input[chip_ctx.w_input_index], 0,
			       SHA_SHA256_BLOCK_LEN - chip_ctx.w_input_index);
		}

		/*
		 * Since input data (big-endian) are copied 1 byte by 1 byte to
		 * it51xxx memory (little-endian), so the bit length needs to
		 * be transformed into big-endian format and then write to memory.
		 */
		chip_ctx.w_sha[15] = sys_cpu_to_be32(chip_ctx.total_len * 8);

		/* HW automatically load 64 bytes data from DLM */
		sys_write8(IT51XXX_SHAEXEC_64_BYTE, config->base + IT51XXX_SHAECR);
		while (sys_read8(config->base + IT51XXX_SHASR) & IT51XXX_SHABUSY) {
		};

		it51xxx_sha256_module_calculation(ctx->device);

		/* HW write back the hash result to DLM */
		/* Set DLM address for input data */
		sys_write8(((uint32_t)&chip_ctx.h) & 0xC0, config->base + IT51XXX_SHADBA0R);
		sys_write8(((uint32_t)&chip_ctx.h) >> 8, config->base + IT51XXX_SHADBA1R);

		sys_write8(IT51XXX_SHAWB, config->base + IT51XXX_SHACR);

		k_sem_take(&data->sha_done, K_FOREVER);

		memcpy(out_buf_ptr, chip_ctx.h, sizeof(chip_ctx.h));

		it51xxx_sha256_init(ctx->device);
	}

	return 0;
}

static int it51xxx_hash_session_free(const struct device *dev, struct hash_ctx *ctx)
{
	it51xxx_sha256_init(dev);

	return 0;
}

static inline int it51xxx_query_hw_caps(const struct device *dev)
{
	return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int it51xxx_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
				      enum hash_algo algo)
{
	if (algo != CRYPTO_HASH_ALGO_SHA256) {
		LOG_ERR("Unsupported algorithm");
		return -ENOTSUP;
	}

	if (ctx->flags & ~(it51xxx_query_hw_caps(dev))) {
		LOG_ERR("Unsupported flag");
		return -ENOTSUP;
	}

	it51xxx_sha256_init(dev);
	ctx->hash_hndlr = it51xxx_hash_handler;
	ctx->device = dev;

	return 0;
}

static void it51xxx_sha_isr(const struct device *dev)
{
	const struct it51xxx_sha_config *config = dev->config;
	struct it51xxx_sha_data *data = dev->data;
	uint8_t sha_sts = sys_read8(config->base + IT51XXX_SHASR);

	if (sha_sts & IT51XXX_SHAIS) {
		k_sem_give(&data->sha_done);
		sys_write8(sha_sts | IT51XXX_SHAIS, config->base + IT51XXX_SHASR);
	}
}

static int it51xxx_sha_init(const struct device *dev)
{
	const struct it51xxx_sha_config *config = dev->config;
	struct it51xxx_sha_data *data = dev->data;

	it51xxx_sha256_init(dev);

	k_sem_init(&data->sha_done, 0, 1);

	/* Select SHA-2 Family, SHA-256 */
	sys_write8(IT51XXX_SHA256, config->base + IT51XXX_SHASETR);

	/* Enable SHA interrupt */
	sys_write8(sys_read8(config->base + IT51XXX_SHASR) | IT51XXX_SHAIE,
		   config->base + IT51XXX_SHASR);

	IRQ_CONNECT(DT_INST_IRQN(0), 0, it51xxx_sha_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static DEVICE_API(crypto, it51xxx_crypto_api) = {
	.hash_begin_session = it51xxx_hash_begin_session,
	.hash_free_session = it51xxx_hash_session_free,
	.query_hw_caps = it51xxx_query_hw_caps,
};

static const struct it51xxx_sha_config it51xxx_sha_config_0 = {
	.base = DT_INST_REG_ADDR(0),
};

static struct it51xxx_sha_data it51xxx_sha_data_0 = {};

DEVICE_DT_INST_DEFINE(0, &it51xxx_sha_init, NULL, &it51xxx_sha_data_0, &it51xxx_sha_config_0,
		      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, &it51xxx_crypto_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "support only one sha compatible node");
