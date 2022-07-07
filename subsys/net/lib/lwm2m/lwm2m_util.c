/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "lwm2m_object.h"
#include "lwm2m_util.h"

#define SHIFT_LEFT(v, o, m) (((v) << (o)) & (m))
#define SHIFT_RIGHT(v, o, m) (((v) >> (o)) & (m))

#define PRECISION64_LEN 17U
#define PRECISION64 100000000000000000ULL

#define PRECISION32 1000000000UL

/* convert from float to binary32 */
int lwm2m_float_to_b32(double *in, uint8_t *b32, size_t len)
{
	int32_t e = -1, v, f = 0;
	int32_t val1 = (int32_t)*in;
	int32_t val2 = (*in - (int32_t)*in) * PRECISION32;
	int i;

	if (len != 4) {
		return -EINVAL;
	}

	/* handle zero value special case */
	if (val1 == 0 && val2 == 0) {
		memset(b32, 0, len);
		return 0;
	}

	/* sign handled later */
	v = abs(val1);

	/* add whole value to fraction */
	while (v > 0) {
		f >>= 1;

		if (v & 1) {
			f |= (1 << 23);
		}

		v >>= 1;
		e++;
	}

	/* sign handled later */
	v = abs(val2);

	/* add decimal to fraction */
	i = e;
	while (v > 0 && i < 23) {
		v *= 2;
		if (!f && e < 0 && v < PRECISION32) {
			/* handle -e */
			e--;
			continue;
		} else if (v >= PRECISION32) {
			v -= PRECISION32;
			f |= 1 << (22 - i);
		}

		if (v == 0) {
			break;
		}

		i++;
	}

	/* adjust exponent for bias */
	e += 127;

	memset(b32, 0, len);

	/* sign: bit 31 */
	if (val1 == 0) {
		b32[0] = val2 < 0 ? 0x80 : 0;
	} else {
		b32[0] = val1 < 0 ? 0x80 : 0;
	}

	/* exponent: bits 30-23 */
	b32[0] |= e >> 1;
	b32[1] = (e & 1) << 7;

	/* fraction: bits 22-0 */
	/* NOTE: ignore the "hidden" bit 23 in fraction */
	b32[1] |= (f >> 16) & 0x7F;
	b32[2] = (f >> 8) & 0xFF;
	b32[3] = f & 0xFF;

	return 0;
}

/* convert from float to binary64 */
int lwm2m_float_to_b64(double *in, uint8_t *b64, size_t len)
{
	int64_t v, f = 0;
	int32_t e = -1;
	int64_t val1 = (int64_t)*in;
	int64_t val2 = (*in - (int64_t)*in) * PRECISION64;
	int i;

	if (len != 8) {
		return -EINVAL;
	}

	/* handle zero value special case */
	if (val1 == 0 && val2 == 0) {
		memset(b64, 0, len);
		return 0;
	}

	/* sign handled later */
	v = llabs(val1);

	/* add whole value to fraction */
	while (v > 0) {
		f >>= 1;

		if (v & 1) {
			f |= ((int64_t)1 << 52);
		}

		v >>= 1;
		e++;
	}

	/* sign handled later */
	v = llabs(val2);

	/* add decimal to fraction */
	i = e;
	while (v > 0 && i < 52) {
		v *= 2;
		if (!f && e < 0 && v < PRECISION64) {
			/* handle -e */
			e--;
			continue;
		} else if (v >= PRECISION64) {
			v -= PRECISION64;
			f |= (int64_t)1 << (51 - i);
		}

		if (v == 0) {
			break;
		}

		i++;
	}

	/* adjust exponent for bias */
	e += 1023;

	memset(b64, 0, len);

	/* sign: bit 63 */
	if (val1 == 0) {
		b64[0] = val2 < 0 ? 0x80 : 0;
	} else {
		b64[0] = val1 < 0 ? 0x80 : 0;
	}

	/* exponent: bits 62-52 */
	b64[0] |= (e >> 4);
	b64[1] = ((e & 0xF) << 4);

	/* fraction: bits 51-0 */
	/* NOTE: ignore the "hidden" bit 52 in fraction */
	b64[1] |= ((f >> 48) & 0xF);
	b64[2] = (f >> 40) & 0xFF;
	b64[3] = (f >> 32) & 0xFF;
	b64[4] = (f >> 24) & 0xFF;
	b64[5] = (f >> 16) & 0xFF;
	b64[6] = (f >> 8) & 0xFF;
	b64[7] = f & 0xFF;

	return 0;
}

/* convert from binary32 to float */
int lwm2m_b32_to_float(uint8_t *b32, size_t len, double *out)
{
	int32_t f, k, i, e;
	bool sign = false;
	int32_t val1, val2;

	if (len != 4) {
		return -EINVAL;
	}

	val1 = 0;
	val2 = 0;

	/* calc sign: bit 31 */
	sign = SHIFT_RIGHT(b32[0], 7, 0x1);

	/* calc exponent: bits 30-23 */
	e  = SHIFT_LEFT(b32[0], 1, 0xFF);
	e += SHIFT_RIGHT(b32[1], 7, 0x1);
	/* remove bias */
	e -= 127;

	/* enable "hidden" fraction bit 24 which is always 1 */
	f  = ((int32_t)1 << 23);
	/* calc fraction: bits 22-0 */
	f += ((int32_t)(b32[1] & 0x7F) << 16);
	f += ((int32_t)b32[2] << 8);
	f += b32[3];

	/* handle whole number */
	if (e > -1) {
		/* precision overflow */
		if (e > 23) {
			e = 23;
		}

		val1 = (f >> (23 - e)) * (sign ? -1 : 1);
	}

	/* calculate the rest of the decimal */
	k = PRECISION32;

	/* account for -e */
	while (e < -1) {
		k /= 2;
		e++;
	}

	for (i = 22 - e; i >= 0; i--) {
		k /= 2;
		if (f & (1 << i)) {
			val2 += k;

		}
	}

	if (sign) {
		*out = (double)val1 - (double)val2 / PRECISION32;
	} else {
		*out = (double)val1 + (double)val2 / PRECISION32;
	}

	return 0;
}

/* convert from binary64 to float */
int lwm2m_b64_to_float(uint8_t *b64, size_t len, double *out)
{
	int64_t f, k;
	int i, e;
	bool sign = false;
	int64_t val1, val2;

	if (len != 8) {
		return -EINVAL;
	}

	val1 = 0LL;
	val2 = 0LL;

	/* calc sign: bit 63 */
	sign = SHIFT_RIGHT(b64[0], 7, 0x1);

	/* get exponent: bits 62-52 */
	e  = SHIFT_LEFT((uint16_t)b64[0], 4, 0x7F0);
	e += SHIFT_RIGHT(b64[1], 4, 0xF);
	/* remove bias */
	e -= 1023;

	/* enable "hidden" fraction bit 53 which is always 1 */
	f  = ((int64_t)1 << 52);
	/* get fraction: bits 51-0 */
	f += ((int64_t)(b64[1] & 0xF) << 48);
	f += ((int64_t)b64[2] << 40);
	f += ((int64_t)b64[3] << 32);
	f += ((int64_t)b64[4] << 24);
	f += ((int64_t)b64[5] << 16);
	f += ((int64_t)b64[6] << 8);
	f += b64[7];

	/* handle whole number */
	if (e > -1) {
		/* precision overflow */
		if (e > 52) {
			e = 52;
		}

		val1 = (f >> (52 - e)) * (sign ? -1 : 1);
	}

	/* calculate the rest of the decimal */
	k = PRECISION64;

	/* account for -e */
	while (e < -1) {
		k /= 2;
		e++;
	}

	for (i = 51 - e; i >= 0; i--) {
		k /= 2;
		if (f & ((int64_t)1 << i)) {
			val2 += k;

		}
	}

	if (sign) {
		*out = (double)val1 - (double)val2 / PRECISION64;
	} else {
		*out = (double)val1 + (double)val2 / PRECISION64;
	}

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

	while (*(++pos) && base > 1 && isdigit((unsigned char)*pos)) {
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

int lwm2m_path_to_string(char *buf, size_t buf_size, struct lwm2m_obj_path *input, int level_max)
{
	size_t fpl = 0; /* Length of the formed path */
	int level;
	int w;

	if (!buf || buf_size < sizeof("/") || !input) {
		return -EINVAL;
	}

	memset(buf, '\0', buf_size);

	level = MIN(input->level, level_max);

	/* Write path element at a time and leave space for the terminating NULL */
	for (int idx = LWM2M_PATH_LEVEL_NONE; idx <= level; idx++) {
		switch (idx) {
		case LWM2M_PATH_LEVEL_NONE:
			w = snprintk(&(buf[fpl]), buf_size - fpl, "/");
			break;
		case LWM2M_PATH_LEVEL_OBJECT:
			w = snprintk(&(buf[fpl]), buf_size - fpl, "%" PRIu16 "/", input->obj_id);
			break;
		case LWM2M_PATH_LEVEL_OBJECT_INST:
			w = snprintk(&(buf[fpl]), buf_size - fpl, "%" PRIu16 "/",
				     input->obj_inst_id);
			break;
		case LWM2M_PATH_LEVEL_RESOURCE:
			w = snprintk(&(buf[fpl]), buf_size - fpl, "%" PRIu16 "", input->res_id);
			break;
		case LWM2M_PATH_LEVEL_RESOURCE_INST:
			w = snprintk(&(buf[fpl]), buf_size - fpl, "/%" PRIu16 "",
				     input->res_inst_id);
			break;
		default:
			__ASSERT_NO_MSG(false);
			return -EINVAL;
		}

		if (w < 0 || w >= buf_size - fpl) {
			return -ENOBUFS;
		}

		/* Next path element, overwrites terminating NULL */
		fpl += w;
	}

	return fpl;
}

uint16_t lwm2m_atou16(const uint8_t *buf, uint16_t buflen, uint16_t *len)
{
	uint16_t val = 0U;
	uint16_t pos = 0U;

	/* we should get a value first - consume all numbers */
	while (pos < buflen && isdigit(buf[pos])) {
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
			if (!isdigit((unsigned char)pathstr[i])) {
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
