/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include "lwm2m_object.h"
#include "lwm2m_util.h"

#define PRECISION64_LEN 17U
#define PRECISION64 100000000000000000ULL

/*
 * The binary formats are IEEE 754 binary32 and binary64 in network byte order, the
 * representation of float and double on every target, so the bits are copied: NaN, the
 * infinities, subnormals and values beyond the range of int64_t keep their value.
 */
BUILD_ASSERT(sizeof(float) == sizeof(uint32_t) && FLT_MANT_DIG == 24,
	     "float must be IEEE 754 binary32");
BUILD_ASSERT(sizeof(double) == sizeof(uint64_t) && DBL_MANT_DIG == 53,
	     "double must be IEEE 754 binary64");

/* convert from float to binary32 */
int lwm2m_float_to_b32(double *in, uint8_t *b32, size_t len)
{
	float value = (float)*in;
	uint32_t bits;

	if (len != 4) {
		return -EINVAL;
	}

	memcpy(&bits, &value, sizeof(bits));
	sys_put_be32(bits, b32);

	return 0;
}

/* convert from float to binary64 */
int lwm2m_float_to_b64(double *in, uint8_t *b64, size_t len)
{
	uint64_t bits;

	if (len != 8) {
		return -EINVAL;
	}

	memcpy(&bits, in, sizeof(bits));
	sys_put_be64(bits, b64);

	return 0;
}

/* convert from binary32 to float */
int lwm2m_b32_to_float(uint8_t *b32, size_t len, double *out)
{
	uint32_t bits;
	float value;

	if (len != 4) {
		return -EINVAL;
	}

	bits = sys_get_be32(b32);
	memcpy(&value, &bits, sizeof(value));
	*out = (double)value;

	return 0;
}

/* convert from binary64 to float */
int lwm2m_b64_to_float(uint8_t *b64, size_t len, double *out)
{
	uint64_t bits;

	if (len != 8) {
		return -EINVAL;
	}

	bits = sys_get_be64(b64);
	memcpy(out, &bits, sizeof(bits));

	return 0;
}

int lwm2m_atof(const char *input, double *out)
{
	char *pos, *end, buf[24];
	long val;
	int64_t base = PRECISION64, sign = 1;
	int64_t val1, val2;

	if (!input || !out) {
		return -EINVAL;
	}

	strncpy(buf, input, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	if (strchr(buf, '-')) {
		sign = -1;
	}

	pos = strchr(buf, '.');
	if (pos) {
		*pos = '\0';
	}

	errno = 0;
	val = strtol(buf, &end, 10);
	if (errno || *end) {
		return -EINVAL;
	}

	val1 = (int64_t)val;
	val2 = 0;

	if (!pos) {
		*out = (double)val1;
		return 0;
	}

	while (*(++pos) && base > 1 && isdigit((unsigned char)*pos) != 0) {
		val2 = val2 * 10 + (*pos - '0');
		base /= 10;
	}

	val2 *= sign * base;

	*out = (double)val1 + (double)val2 / PRECISION64;

	return !*pos || base == 1 ? 0 : -EINVAL;
}

int lwm2m_ftoa(double *input, char *out, size_t outlen, int8_t dec_limit)
{
	size_t len;
	char buf[PRECISION64_LEN + 1];
	int64_t val1 = (int64_t)*input;
	int64_t val2 = (*input - (int64_t)*input) * PRECISION64;

	len = snprintk(buf, sizeof(buf), "%0*lld", PRECISION64_LEN,
		       (long long)llabs(val2));
	if (len != PRECISION64_LEN) {
		strcpy(buf, "0");
	} else {
		/* Round the value to the specified decimal point. */
		if (dec_limit > 0 && dec_limit < sizeof(buf) &&
		    buf[dec_limit] != '\0') {
			bool round_up = buf[dec_limit] >= '5';

			buf[dec_limit] = '\0';
			len = dec_limit;

			while (round_up && dec_limit > 0) {
				dec_limit--;
				buf[dec_limit]++;

				if (buf[dec_limit] > '9') {
					buf[dec_limit] = '0';
				} else {
					round_up = false;
				}
			}

			if (round_up) {
				if (*input < 0) {
					val1--;
				} else {
					val1++;
				}
			}
		}

		/* clear ending zeroes, but leave 1 if needed */
		while (len > 1U && buf[len - 1] == '0') {
			buf[--len] = '\0';
		}
	}

	return snprintk(out, outlen, "%s%lld.%s",
			/* handle negative val2 when val1 is 0 */
			(val1 == 0 && val2 < 0) ? "-" : "", (long long)val1, buf);
}

uint16_t lwm2m_atou16(const uint8_t *buf, uint16_t buflen, uint16_t *len)
{
	uint16_t val = 0U;
	uint16_t pos = 0U;

	/* we should get a value first - consume all numbers */
	while (pos < buflen && isdigit(buf[pos]) != 0) {
		val = val * 10U + (buf[pos] - '0');
		pos++;
	}

	*len = pos;
	return val;
}

int lwm2m_string_to_path(const char *pathstr, struct lwm2m_obj_path *path,
			  char delim)
{
	uint16_t value, len;
	int i, tokstart = -1, toklen;
	int end_index = strlen(pathstr) - 1;

	(void)memset(path, 0, sizeof(*path));
	for (i = 0; i <= end_index; i++) {
		/* search for first numeric */
		if (tokstart == -1) {
			if (isdigit((unsigned char)pathstr[i]) == 0) {
				continue;
			}

			tokstart = i;
		}

		/* find delimiter char or end of string */
		if (pathstr[i] == delim || i == end_index) {
			toklen = i - tokstart + 1;

			/* don't process delimiter char */
			if (pathstr[i] == delim) {
				toklen--;
			}

			if (toklen <= 0) {
				continue;
			}

			value = lwm2m_atou16(&pathstr[tokstart], toklen, &len);
			/* increase the path level for each token found */
			path->level++;
			switch (path->level) {
			case LWM2M_PATH_LEVEL_OBJECT:
				path->obj_id = value;
				break;

			case LWM2M_PATH_LEVEL_OBJECT_INST:
				path->obj_inst_id = value;
				break;

			case LWM2M_PATH_LEVEL_RESOURCE:
				path->res_id = value;
				break;

			case LWM2M_PATH_LEVEL_RESOURCE_INST:
				path->res_inst_id = value;
				break;

			default:
				return -EINVAL;

			}

			tokstart = -1;
		}
	}

	return 0;
}

bool lwm2m_obj_path_equal(const struct lwm2m_obj_path *a, const struct lwm2m_obj_path *b)
{
	uint8_t level = a->level;

	if (a->level != b->level) {
		return false;
	}

	if (level >= LWM2M_PATH_LEVEL_OBJECT && (a->obj_id != b->obj_id)) {
		return false;
	}

	if (level >= LWM2M_PATH_LEVEL_OBJECT_INST && (a->obj_inst_id != b->obj_inst_id)) {
		return false;
	}

	if (level >= LWM2M_PATH_LEVEL_RESOURCE && (a->res_id != b->res_id)) {
		return false;
	}

	if (level >= LWM2M_PATH_LEVEL_RESOURCE_INST && (a->res_inst_id != b->res_inst_id)) {
		return false;
	}

	return true;
}

/* for debugging: to print IP addresses */
char *lwm2m_sprint_ip_addr(const struct net_sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

	if (addr->sa_family == NET_AF_INET6) {
		return net_addr_ntop(NET_AF_INET6, &net_sin6(addr)->sin6_addr, buf, sizeof(buf));
	}

	if (addr->sa_family == NET_AF_INET) {
		return net_addr_ntop(NET_AF_INET, &net_sin(addr)->sin_addr, buf, sizeof(buf));
	}

	return "::";
}

static uint8_t to_hex_digit(uint8_t digit)
{
	if (digit >= 10U) {
		return digit - 10U + 'a';
	}

	return digit + '0';
}

char *sprint_token(const uint8_t *token, uint8_t tkl)
{
	static char buf[32];
	char *ptr = buf;

	if (token && tkl != 0) {
		int i;

		tkl = MIN(tkl, sizeof(buf) / 2 - 1);

		for (i = 0; i < tkl; i++) {
			*ptr++ = to_hex_digit(token[i] >> 4);
			*ptr++ = to_hex_digit(token[i] & 0x0F);
		}

		*ptr = '\0';
	} else {
		strcpy(buf, "[no-token]");
	}

	return buf;
}
