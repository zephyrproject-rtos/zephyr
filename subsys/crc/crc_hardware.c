/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc, CONFIG_CRC_LOG_LEVEL);

#include <zephyr/drivers/crc.h>
#include <zephyr/device.h>

/* This value needs to be XORed with the final crc value once crc for
 * the entire stream is calculated. This is a requirement of crc32c algo.
 */
#define CRC32C_XOR_OUT 0xFFFFFFFFUL

static const struct device *const crc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

static int crc_operation(const struct device *const dev, struct crc_ctx *ctx, const uint8_t *src,
			 size_t len)
{
	int ret;

	if (!device_is_ready((crc_dev))) {
		return -ENODEV;
	}

	ret = crc_begin(crc_dev, ctx);
	if (ret != 0) {
		return ret;
	}

	ret = crc_update(crc_dev, ctx, src, len);
	if (ret != 0) {
		return ret;
	}

	ret = crc_finish(crc_dev, ctx);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

#ifdef CONFIG_CRC4
uint8_t crc4(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed)
{
	uint8_t flag_reversed;
	int ret;

	if (reversed) {
		flag_reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT;
	}

	struct crc_ctx ctx = {
		.type = CRC4,
		.polynomial = polynomial,
		.seed = initial_value,
		.reversed = flag_reversed,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result & 0x0F;
}
#endif

#ifdef CONFIG_CRC4_TI
uint8_t crc4_ti(uint8_t seed, const uint8_t *src, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC4,
		.polynomial = CRC4_POLY,
		.seed = seed,
		.reversed = 0,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result & 0x0F;
}
#endif

#ifdef CONFIG_CRC7_BE
uint8_t crc7_be(uint8_t seed, const uint8_t *src, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC7_BE,
		.polynomial = CRC7_BE_POLY,
		.seed = seed,
		.reversed = 0,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result & 0x7F;
}
#endif

#ifdef CONFIG_CRC8
uint8_t crc8(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed)
{
	uint8_t flag_reversed;
	int ret;

	if (reversed) {
		flag_reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT;
	}

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = polynomial,
		.seed = initial_value,
		.reversed = flag_reversed,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}
	return ctx.result;
}
#endif

#ifdef CONFIG_CRC8_ROHC
uint8_t crc8_rohc(uint8_t initial_value, const void *buf, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = CRC8_POLY,
		.seed = initial_value,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, buf, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}
#endif

#ifdef CONFIG_CRC8_CCITT
uint8_t crc8_ccitt(uint8_t initial_value, const void *buf, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = CRC8_POLY,
		.seed = initial_value,
		.reversed = 0,
	};

	ret = crc_operation(crc_dev, &ctx, buf, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}
#endif

#ifdef CONFIG_CRC16
uint16_t crc16(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = seed,
		.reversed = 0,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}
#endif

#ifdef CONFIG_CRC16_REFLECT
uint16_t crc16_reflect(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}
	return ctx.result;
}
#endif

#ifdef CONFIG_CRC16_CCITT
uint16_t crc16_ccitt(uint16_t seed, const uint8_t *src, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}
#endif

#ifdef CONFIG_CRC16_ITU_T
uint16_t crc16_itu_t(uint16_t seed, const uint8_t *src, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = seed,
		.reversed = 0,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}
#endif

#ifdef CONFIG_CRC24_PGP
uint32_t crc24_pgp_update(uint32_t crc, const uint8_t *data, size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC24_PGP,
		.polynomial = CRC24_PGP_POLY,
		.seed = CRC24_PGP_INITIAL_VALUE,
		.reversed = 0,
	};

	ret = crc_operation(crc_dev, &ctx, data, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}

uint32_t crc24_pgp(const uint8_t *data, size_t len)
{
	return crc24_pgp_update(CRC24_PGP_INITIAL_VALUE, data, len) & CRC24_FINAL_VALUE_MASK;
}
#endif

#ifdef CONFIG_CRC32_C
uint32_t crc32_c(uint32_t crc, const uint8_t *buf, size_t len, bool first_pkt, bool last_pkt)
{
	int ret;

	if (first_pkt) {
		crc = CRC32_C_INIT_VAL;
	}

	struct crc_ctx ctx = {
		.type = CRC32_C,
		.polynomial = CRC32C_POLY,
		.seed = crc,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, buf, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return last_pkt ? (ctx.result ^ CRC32C_XOR_OUT) : ctx.result;
}
#endif

#ifdef CONFIG_CRC32_IEEE
uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *buf, size_t len)
{
	int ret;

	crc = ~crc;

	struct crc_ctx ctx = {
		.type = CRC32_IEEE,
		.polynomial = CRC32_IEEE_POLY,
		.seed = crc,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, buf, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}

uint32_t crc32_ieee(const uint8_t *buf, size_t len)
{
	return crc32_ieee_update(0x0, buf, len);
}
#endif

#ifdef CONFIG_CRC32_K_4_2
uint32_t crc32_k_4_2_update(uint32_t crc, const uint8_t *const data, const size_t len)
{
	int ret;

	struct crc_ctx ctx = {
		.type = CRC32_K_4_2,
		.polynomial = CRC32K_4_2_POLY,
		.seed = crc,
		.reversed = 0,
	};

	ret = crc_operation(crc_dev, &ctx, data, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("CRC operation failed: %d", ret);
		return 0;
	}

	return ctx.result;
}
#endif
