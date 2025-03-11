/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_subsys, CONFIG_CRC_LOG_LEVEL);

#include <zephyr/drivers/crc.h>
#include <zephyr/device.h>

static const struct device *const crc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

static void crc_operation(const struct device *const dev, struct crc_ctx *ctx, const uint8_t *src,
			  size_t len)
{
	int ret;

	ret = crc_begin(crc_dev, ctx);
	if (ret != 0) {
		__ASSERT_MSG_INFO("Failed to begin CRC: %d", ret);
	}

	ret = crc_update(crc_dev, ctx, src, len);
	if (ret != 0) {
		__ASSERT_MSG_INFO("Failed to update CRC: %d", ret);
	}

	ret = crc_finish(crc_dev, ctx);
	if (ret != 0) {
		__ASSERT_MSG_INFO("Failed to finish CRC: %d", ret);
	}
}

#ifdef CONFIG_CRC4
uint8_t crc4(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed)
{
	uint8_t flag_reversed;

	if ((polynomial != CRC4_POLY) && (polynomial != CRC4_REFLECT_POLY)) {
		__ASSERT_MSG_INFO("Invalid polynomial");
	}

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	if (reversed) {
		flag_reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT;
	} else {
		flag_reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT;
	}

	struct crc_ctx ctx = {
		.type = CRC4,
		.polynomial = polynomial,
		.seed = initial_value,
		.reversed = flag_reversed,
	};

	crc_operation(crc_dev, &ctx, src, len);

	return ctx.result;
}

uint8_t crc4_ti(uint8_t seed, const uint8_t *src, size_t len)
{
	return crc4(src, len, CRC4_POLY, seed, false) & 0x0F;
}
#endif

#ifdef CONFIG_CRC7
uint8_t crc7_be(uint8_t seed, const uint8_t *src, size_t len)
{
	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC7_BE,
		.polynomial = CRC7_BE_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, src, len);

	return ctx.result & 0x7F;
}
#endif

#ifdef CONFIG_CRC8
uint8_t crc8(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed)
{
	uint8_t flag_reversed;

	if ((polynomial != CRC8_POLY) && (polynomial != CRC8_REFLECT_POLY)) {
		__ASSERT_MSG_INFO("Invalid polynomial");
	}

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	if (reversed) {
		flag_reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT;
	} else {
		flag_reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT;
	}

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = polynomial,
		.seed = initial_value,
		.reversed = flag_reversed,
	};

	crc_operation(crc_dev, &ctx, src, len);

	return ctx.result;
}

uint8_t crc8_rohc(uint8_t initial_value, const void *buf, size_t len)
{
	return crc8(buf, len, CRC8_REFLECT_POLY, initial_value, true);
}

uint8_t crc8_ccitt(uint8_t initial_value, const void *buf, size_t len)
{
	return crc8(buf, len, CRC8_POLY, initial_value, false);
}
#endif

#ifdef CONFIG_CRC16
uint16_t crc16(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len)
{
	if (poly != CRC16_POLY) {
		__ASSERT_MSG_INFO("Invalid polynomial");
	}

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, src, len);

	return ctx.result;
}

uint16_t crc16_reflect(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len)
{
	if (poly != CRC16_REFLECT_POLY) {
		__ASSERT_MSG_INFO("Invalid polynomial");
	}

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, src, len);

	return ctx.result;
}
#endif

#ifdef CONFIG_CRC_CCITT
uint16_t crc16_ccitt(uint16_t seed, const uint8_t *src, size_t len)
{
	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, src, len);

	return ctx.result;
}
uint16_t crc16_itu_t(uint16_t seed, const uint8_t *src, size_t len)
{
	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, src, len);

	return ctx.result;
}
#endif

#ifdef CONFIG_CRC24
uint32_t crc24_pgp_update(uint32_t crc, const uint8_t *data, size_t len)
{
	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC24_PGP,
		.polynomial = CRC24_PGP_POLY,
		.seed = CRC24_PGP_INITIAL_VALUE,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, data, len);

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
	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	if (first_pkt) {
		crc = CRC32_C_INIT_VAL;
	}

	struct crc_ctx ctx = {
		.type = CRC32_C,
		.polynomial = CRC32C_POLY,
		.seed = crc,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, buf, len);

	return last_pkt ? (ctx.result ^ CRC32C_XOR_OUT) : ctx.result;
}
#endif

#ifdef CONFIG_CRC32_IEEE
uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *buf, size_t len)
{
	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	crc = ~crc;

	struct crc_ctx ctx = {
		.type = CRC32_IEEE,
		.polynomial = CRC32_IEEE_POLY,
		.seed = crc,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	crc_operation(crc_dev, &ctx, buf, len);

	return ctx.result;
}

uint32_t crc32_ieee(const uint8_t *buf, size_t len)
{
	return crc32_ieee_update(0x0, buf, len);
}
#endif
