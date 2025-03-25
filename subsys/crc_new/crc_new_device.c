/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_subsys, CONFIG_CRC_LOG_LEVEL);

#include <zephyr/crc_new/crc_new.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/device.h>

static const struct device *const crc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

static int crc_operation(const struct device *const dev, struct crc_ctx *ctx, const uint8_t *src,
			 size_t len)
{
	int ret;

	ret = crc_begin(crc_dev, ctx);
	if (ret != 0) {
		LOG_ERR("Failed to begin CRC: %d", ret);
		return ret;
	}

	ret = crc_update(crc_dev, ctx, src, len);
	if (ret != 0) {
		LOG_ERR("Failed to update CRC: %d", ret);
		return ret;
	}

	ret = crc_finish(crc_dev, ctx);
	if (ret != 0) {
		LOG_ERR("Failed to finish CRC: %d", ret);
		return ret;
	}

	return 0;
}

int crc4_new(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed, uint8_t *result)
{
	int ret;
	uint8_t flag_reversed;

	if ((polynomial != CRC4_POLY) && (polynomial != CRC4_REFLECT_POLY)) {
		return -ENOTSUP;
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

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		return ret;
	}
	*result = ctx.result;

	return 0;
}

int crc4_ti_new(uint8_t seed, const uint8_t *src, size_t len, uint8_t *result)
{

	int ret;

	ret = crc4_new(src, len, CRC4_POLY, seed, false, result);
	if (ret != 0) {
		return ret;
	}

	*result = *result & 0x0F;

	return 0;
}

int crc7_be_new(uint8_t seed, const uint8_t *src, size_t len, uint8_t *result)
{
	int ret;

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC7_BE,
		.polynomial = CRC7_BE_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		return ret;
	}

	*result = ctx.result & 0x7F;

	return 0;
}

int crc8_new(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed, uint8_t *result)
{
	int ret;
	uint8_t flag_reversed;

	if ((polynomial != CRC8_POLY) && (polynomial != CRC8_REFLECT_POLY)) {
		return -ENOTSUP;
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

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		return ret;
	}
	*result = ctx.result;

	return 0;
}

int crc8_rohc_new(uint8_t initial_value, const void *buf, size_t len, uint8_t *result)
{
	int ret;

	ret = crc8_new(buf, len, CRC8_REFLECT_POLY, initial_value, true, result);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int crc8_ccitt_new(uint8_t initial_value, const void *buf, size_t len, uint8_t *result)
{
	int ret;

	ret = crc8_new(buf, len, CRC8_POLY, initial_value, false, result);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int crc16_new(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len, uint16_t *result)
{
	int ret;

	if (poly != CRC16_POLY) {
		return -ENOTSUP;
	}

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		return ret;
	}
	*result = ctx.result;

	return 0;
}

int crc16_itu_t_new(uint16_t seed, const uint8_t *src, size_t len, uint16_t *result)
{
	int ret;

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		return ret;
	}
	*result = ctx.result;

	return 0;
}

int crc16_ccitt_new(uint16_t seed, const uint8_t *src, size_t len, uint16_t *result)
{
	int ret;

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		return ret;
	}
	*result = ctx.result;

	return 0;
}

int crc16_reflect_new(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len,
		      uint16_t *result)
{
	int ret;

	if (poly != CRC16_REFLECT_POLY) {
		return -ENOTSUP;
	}

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = seed,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, src, len);
	if (ret != 0) {
		return ret;
	}
	*result = ctx.result;

	return 0;
}

int crc24_pgp_update_new(uint32_t crc, const uint8_t *data, size_t len, uint32_t *result)
{
	int ret;

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	struct crc_ctx ctx = {
		.type = CRC24_PGP,
		.polynomial = CRC24_PGP_POLY,
		.seed = CRC24_PGP_INITIAL_VALUE,
		.reversed = CRC_FLAG_NO_REVERSE_OUTPUT | CRC_FLAG_NO_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, data, len);
	if (ret != 0) {
		return ret;
	}

	*result = ctx.result;

	return 0;
}

int crc24_pgp_new(const uint8_t *data, size_t len, uint32_t *result)
{
	int ret;

	ret = crc24_pgp_update_new(CRC24_PGP_INITIAL_VALUE, data, len, result) &
	      CRC24_FINAL_VALUE_MASK;
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int crc32_c_new(uint32_t crc, const uint8_t *buf, size_t len, bool first_pkt, bool last_pkt,
		uint32_t *result)
{
	int ret;

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

	ret = crc_operation(crc_dev, &ctx, buf, len);
	if (ret != 0) {
		return ret;
	}

	*result = last_pkt ? (ctx.result ^ CRC32C_XOR_OUT) : ctx.result;

	return 0;
}

int crc32_ieee_update_new(uint32_t crc, const uint8_t *buf, size_t len, uint32_t *result)
{
	int ret;

	__ASSERT(device_is_ready(crc_dev), "CRC device is not ready");

	crc = ~crc;

	struct crc_ctx ctx = {
		.type = CRC32_IEEE,
		.polynomial = CRC32_IEEE_POLY,
		.seed = crc,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	ret = crc_operation(crc_dev, &ctx, buf, len);
	if (ret != 0) {
		return ret;
	}
	*result = ctx.result;

	return 0;
}

int crc32_ieee_new(const uint8_t *buf, size_t len, uint32_t *result)
{
	return crc32_ieee_update_new(0x0, buf, len, result);
}
