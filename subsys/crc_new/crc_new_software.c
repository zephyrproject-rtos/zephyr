/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crc_subsys, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/crc_new/crc_new.h>

int crc4_new(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed, uint8_t *result)
{
	*result = crc4(src, len, polynomial, initial_value, reversed);

	return 0;
}

int crc4_ti_new(uint8_t seed, const uint8_t *src, size_t len, uint8_t *result)
{
	*result = crc4_ti(seed, src, len);

	return 0;
}

int crc7_be_new(uint8_t seed, const uint8_t *src, size_t len, uint8_t *result)
{
	*result = crc7_be(seed, src, len);

	return 0;
}

int crc8_ccitt_new(uint8_t val, const void *buf, size_t cnt, uint8_t *result)
{
	*result = crc8_ccitt(val, buf, cnt);

	return 0;
}

int crc8_rohc_new(uint8_t val, const void *buf, size_t cnt, uint8_t *result)
{
	*result = crc8_rohc(val, buf, cnt);

	return 0;
}

int crc8_new(const uint8_t *src, size_t len, uint8_t polynomial, uint8_t initial_value,
	     bool reversed, uint8_t *result)
{

	*result = crc8(src, len, polynomial, initial_value, reversed);

	return 0;
}

int crc16_new(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len, uint16_t *result)
{
	*result = crc16(poly, seed, src, len);

	return 0;
}

int crc16_reflect_new(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len,
		      uint16_t *result)
{
	*result = crc16_reflect(poly, seed, src, len);

	return 0;
}

int crc16_ccitt_new(uint16_t seed, const uint8_t *src, size_t len, uint16_t *result)
{
	*result = crc16_ccitt(seed, src, len);

	return 0;
}

int crc16_itu_t_new(uint16_t seed, const uint8_t *src, size_t len, uint16_t *result)
{
	*result = crc16_itu_t(seed, src, len);

	return 0;
}

int crc24_pgp_new(const uint8_t *data, size_t len, uint32_t *result)
{
	return crc24_pgp_update_new(CRC24_PGP_INITIAL_VALUE, data, len, result) &
	       CRC24_FINAL_VALUE_MASK;
}

int crc24_pgp_update_new(uint32_t crc, const uint8_t *data, size_t len, uint32_t *result)
{
	*result = crc24_pgp_update(crc, data, len);

	return 0;
}

int crc32_ieee_update_new(uint32_t crc, const uint8_t *data, size_t len, uint32_t *result)
{

	*result = crc32_ieee_update(crc, data, len);

	return 0;
}

int crc32_ieee_new(const uint8_t *data, size_t len, uint32_t *result)
{
	return crc32_ieee_update_new(0x0, data, len, result);
}

int crc32_c_new(uint32_t crc, const uint8_t *data, size_t len, bool first_pkt, bool last_pkt,
		uint32_t *result)
{
	*result = crc32_c(crc, data, len, first_pkt, last_pkt);

	return 0;
}
