/*
 * Copyright (c) 2024 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_sha_v2

#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/byteorder.h>
#include <chip_chipregs.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sha_it8xxx2, CONFIG_CRYPTO_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "support only one sha compatible node");

#define IT8XXX2_SHA_REGS_BASE DT_REG_ADDR(DT_NODELABEL(sha0))

/* 0x00: SHA Control Register */
#define IT8XXX2_REG_SHACR        (0x00)
#define IT8XXX2_SEL1SHA1         BIT(6)
#define IT8XXX2_SELSHA2ALL       (BIT(5) | BIT(4))
#define IT8XXX2_SHAWB            BIT(2)
#define IT8XXX2_SHAINI           BIT(1)
#define IT8XXX2_SHAEXE           BIT(0)
/* 0x01: SHA Status Register */
#define IT8XXX2_REG_SHASR        (0x01)
#define IT8XXX2_SHAIE            BIT(3)
#define IT8XXX2_SHAIS            BIT(2)
#define IT8XXX2_SHABUSY          BIT(0)
/* 0x02: SHA Execution Counter Register */
#define IT8XXX2_REG_SHAECR       (0x02)
#define IT8XXX2_SHAEXEC_64Byte   0x0
#define IT8XXX2_SHAEXEC_512Byte  0x7
#define IT8XXX2_SHAEXEC_1KByte   0xf
/* 0x03: SHA DLM Base Address 0 Register */
#define IT8XXX2_REG_SHADBA0R     (0x03)
/* 0x04: SHA DLM Base Address 1 Register */
#define IT8XXX2_REG_SHADBA1R     (0x04)

#define SHA_SHA256_HASH_LEN                32
#define SHA_SHA256_BLOCK_LEN               64
#define SHA_SHA256_SRAM_BUF                1024
#define SHA_SHA256_HASH_LEN_WORDS          (SHA_SHA256_HASH_LEN / sizeof(uint32_t))
#define SHA_SHA256_BLOCK_LEN_WORDS         (SHA_SHA256_BLOCK_LEN / sizeof(uint32_t))
#define SHA_SHA256_SRAM_BUF_WORDS          (SHA_SHA256_SRAM_BUF / sizeof(uint32_t))
#define SHA_SHA256_CALCULATE_TIMEOUT_US    150
#define SHA_SHA256_WRITE_BACK_TIMEOUT_US   45
#define SHA_SHA256_WAIT_NEXT_CLOCK_TIME_US 15

/*
 * This struct is used by the hardware and must be stored in RAM first 4k-byte
 * and aligned on a 256-byte boundary.
 */
struct chip_sha256_ctx {
	union {
		/* SHA data buffer */
		uint32_t w_sha[SHA_SHA256_SRAM_BUF_WORDS];
		uint8_t w_input[SHA_SHA256_SRAM_BUF];
	};
	/* H[0] ~ H[7] */
	uint32_t h[SHA_SHA256_HASH_LEN_WORDS];
	uint32_t sha_init;
	uint32_t w_input_index;
	uint32_t total_len;
} __aligned(256);

Z_GENERIC_SECTION(.__sha256_ram_block) struct chip_sha256_ctx chip_ctx;

static void it8xxx2_sha256_init(bool init_k)
{
	chip_ctx.sha_init = init_k;
	chip_ctx.w_input_index = 0;
	chip_ctx.total_len = 0;

	/* Set DLM address for input data */
	sys_write8(((uint32_t)&chip_ctx) & 0xc0,
			IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHADBA0R);
	sys_write8(((uint32_t)&chip_ctx) >> 8,
			IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHADBA1R);
}

static int it8xxx2_sha256_module_calculation(void)
{
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;
	uint32_t key, count;
	uint8_t sha_ctrl;
	bool timeout = true;

	sha_ctrl = sys_read8(IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHACR);

	if (chip_ctx.sha_init) {
		sha_ctrl |= (IT8XXX2_SHAINI | IT8XXX2_SHAEXE);
		chip_ctx.sha_init = 0;
	} else {
		sha_ctrl |= IT8XXX2_SHAEXE;
	}

	/*
	 * Global interrupt is disabled because the CPU cannot access memory
	 * via the DLM (Data Local Memory) bus while HW module is computing
	 * hash.
	 */
	key = irq_lock();
	/* Crypto use SRAM */
	gctrl_regs->GCTRL_PMER3 |= IT8XXX2_GCTRL_SRAM_CRYPTO_USED;
	sys_write8(sha_ctrl, IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHACR);

	/*
	 * HW 64 bytes data calculation ~= 4us;
	 * HW 1024 bytes data calculation ~= 66us.
	 */
	for (count = 0; count <= (SHA_SHA256_CALCULATE_TIMEOUT_US /
	     SHA_SHA256_WAIT_NEXT_CLOCK_TIME_US); count++) {
		/* Delay 15us */
		gctrl_regs->GCTRL_WNCKR = IT8XXX2_GCTRL_WN65K;

		if ((sys_read8(IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHASR) & IT8XXX2_SHAIS)) {
			timeout = 0;
			break;
		}
	}

	sys_write8(IT8XXX2_SHAIS, IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHASR);
	/* CPU use SRAM */
	gctrl_regs->GCTRL_PMER3 &= ~IT8XXX2_GCTRL_SRAM_CRYPTO_USED;
	gctrl_regs->GCTRL_PMER3;
	irq_unlock(key);

	if (timeout) {
		LOG_ERR("HW execute sha256 calculation timeout");
		it8xxx2_sha256_init(true);

		return -ETIMEDOUT;
	}

	chip_ctx.w_input_index = 0;

	return 0;
}

static int it8xxx2_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt,
				bool finish)
{
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;
	uint32_t rem_len = pkt->in_len;
	uint32_t in_buf_idx = 0;
	uint32_t i, key, count;
	uint8_t sha_ctrl;
	bool timeout = true;
	int ret;

	while (rem_len) {
		/* Data length >= 1KB */
		if (rem_len >= SHA_SHA256_SRAM_BUF) {
			rem_len = rem_len - SHA_SHA256_SRAM_BUF;

			for (i = 0; i < SHA_SHA256_SRAM_BUF; i++) {
				chip_ctx.w_input[chip_ctx.w_input_index++] =
								pkt->in_buf[in_buf_idx++];
			}

			/* HW automatically load 1KB data from DLM */
			sys_write8(IT8XXX2_SHAEXEC_1KByte,
					IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHAECR);
			ret = it8xxx2_sha256_module_calculation();

			if (ret) {
				return ret;
			}
		} else {
			/* 0 <= Data length < 1KB */
			while (rem_len) {
				rem_len--;
				chip_ctx.w_input[chip_ctx.w_input_index++] =
								pkt->in_buf[in_buf_idx++];

				/*
				 * If fill full 64byte then execute HW calculation.
				 * If not, will execute in later finish block.
				 */
				if (chip_ctx.w_input_index >= SHA_SHA256_BLOCK_LEN) {
					/* HW automatically load 64Bytes data from DLM */
					sys_write8(IT8XXX2_SHAEXEC_64Byte,
							IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHAECR);
					ret = it8xxx2_sha256_module_calculation();

					if (ret) {
						return ret;
					}
				}
			}
		}
	}

	chip_ctx.total_len += pkt->in_len;

	if (finish) {
		uint32_t *ob_ptr = (uint32_t *)pkt->out_buf;

		/* Pre-processing (Padding) */
		memset(&chip_ctx.w_input[chip_ctx.w_input_index],
			0, SHA_SHA256_BLOCK_LEN - chip_ctx.w_input_index);
		chip_ctx.w_input[chip_ctx.w_input_index] = 0x80;

		/*
		 * Handles the boundary case of rest data:
		 * Because the last eight bytes are bit length field of sha256 rule.
		 * If the data index >= 56, it needs to trigger HW to calculate,
		 * then fill 0 data and the last eight bytes bit length, and calculate again.
		 */
		if (chip_ctx.w_input_index >= 56) {
			/* HW automatically load 64Bytes data from DLM */
			sys_write8(IT8XXX2_SHAEXEC_64Byte,
					IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHAECR);
			ret = it8xxx2_sha256_module_calculation();

			if (ret) {
				return ret;
			}
			memset(&chip_ctx.w_input[chip_ctx.w_input_index],
				0, SHA_SHA256_BLOCK_LEN - chip_ctx.w_input_index);
		}

		/*
		 * Since input data (big-endian) are copied 1byte by 1byte to
		 * it8xxx2 memory (little-endian), so the bit length needs to
		 * be transformed into big-endian format and then write to memory.
		 */
		chip_ctx.w_sha[15] = sys_cpu_to_be32(chip_ctx.total_len * 8);

		/* HW automatically load 64Bytes data from DLM */
		sys_write8(IT8XXX2_SHAEXEC_64Byte, IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHAECR);
		ret = it8xxx2_sha256_module_calculation();

		if (ret) {
			return ret;
		}

		/* HW write back the hash result to DLM */
		/* Set DLM address for input data */
		sys_write8(((uint32_t)&chip_ctx.h) & 0xc0,
				IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHADBA0R);
		sys_write8(((uint32_t)&chip_ctx.h) >> 8,
				IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHADBA1R);

		key = irq_lock();
		/* Crypto use SRAM */
		gctrl_regs->GCTRL_PMER3 |= IT8XXX2_GCTRL_SRAM_CRYPTO_USED;
		sha_ctrl = sys_read8(IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHACR);
		sys_write8(sha_ctrl | IT8XXX2_SHAWB, IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHACR);

		/* HW write back the hash result to DLM ~= 1us */
		for (count = 0; count <= (SHA_SHA256_WRITE_BACK_TIMEOUT_US /
		     SHA_SHA256_WAIT_NEXT_CLOCK_TIME_US); count++) {
			/* Delay 15us */
			gctrl_regs->GCTRL_WNCKR = IT8XXX2_GCTRL_WN65K;

			if ((sys_read8(IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHASR)
			      & IT8XXX2_SHAIS)) {
				timeout = 0;
				break;
			}
		}

		sys_write8(IT8XXX2_SHAIS, IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHASR);
		/* CPU use SRAM */
		gctrl_regs->GCTRL_PMER3 &= ~IT8XXX2_GCTRL_SRAM_CRYPTO_USED;
		gctrl_regs->GCTRL_PMER3;
		irq_unlock(key);

		if (timeout) {
			LOG_ERR("HW write back hash timeout");
			it8xxx2_sha256_init(true);

			return -ETIMEDOUT;
		}

		for (i = 0; i < SHA_SHA256_HASH_LEN_WORDS; i++) {
			ob_ptr[i] = chip_ctx.h[i];
		}

		it8xxx2_sha256_init(true);
	}

	return 0;
}

static int it8xxx2_hash_session_free(const struct device *dev,
				struct hash_ctx *ctx)
{
	it8xxx2_sha256_init(true);

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
		LOG_ERR("Unsupported algorithm");
		return -EINVAL;
	}

	if (ctx->flags & ~(it8xxx2_query_hw_caps(dev))) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	it8xxx2_sha256_init(true);
	ctx->hash_hndlr = it8xxx2_hash_handler;

	return 0;
}

static int it8xxx2_sha_init(const struct device *dev)
{
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;

	/* CPU use SRAM */
	gctrl_regs->GCTRL_PMER3 &= ~IT8XXX2_GCTRL_SRAM_CRYPTO_USED;
	gctrl_regs->GCTRL_PMER3;

	it8xxx2_sha256_init(true);

	/* Select SHA-2 Family, SHA-256 */
	sys_write8(0, IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHACR);
	/* SHA interrupt disable */
	sys_write8(0, IT8XXX2_SHA_REGS_BASE + IT8XXX2_REG_SHASR);

	return 0;
}

static const struct crypto_driver_api it8xxx2_crypto_api = {
	.hash_begin_session = it8xxx2_hash_begin_session,
	.hash_free_session = it8xxx2_hash_session_free,
	.query_hw_caps = it8xxx2_query_hw_caps,
};

DEVICE_DT_INST_DEFINE(0, &it8xxx2_sha_init, NULL, NULL, NULL, POST_KERNEL,
			CONFIG_CRYPTO_INIT_PRIORITY, &it8xxx2_crypto_api);
