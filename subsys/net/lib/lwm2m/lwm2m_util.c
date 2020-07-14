/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdlib.h>
#include "lwm2m_util.h"

#define SHIFT_LEFT(v, o, m) (((v) << (o)) & (m))
#define SHIFT_RIGHT(v, o, m) (((v) >> (o)) & (m))

/* convert from float32 to binary32 */
int lwm2m_f32_to_b32(float32_value_t *f32, uint8_t *b32, size_t len)
{
	int32_t e = -1, v, f = 0;
	int i;

	if (len != 4) {
		return -EINVAL;
	}

	/* handle zero value special case */
	if (f32->val1 == 0 && f32->val2 == 0) {
		memset(b32, 0, len);
		return 0;
	}

	/* sign handled later */
	v = abs(f32->val1);

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
	v = abs(f32->val2);

	/* add decimal to fraction */
	i = e;
	while (v > 0 && i < 23) {
		v *= 2;
		if (!f && e < 0 && v < LWM2M_FLOAT32_DEC_MAX) {
			/* handle -e */
			e--;
			continue;
		} else if (v >= LWM2M_FLOAT32_DEC_MAX) {
			v -= LWM2M_FLOAT32_DEC_MAX;
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
	if (f32->val1 == 0) {
		b32[0] = f32->val2 < 0 ? 0x80 : 0;
	} else {
		b32[0] = f32->val1 < 0 ? 0x80 : 0;
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

/* convert from float64 to binary64 */
int lwm2m_f64_to_b64(float64_value_t *f64, uint8_t *b64, size_t len)
{
	int64_t v, f = 0;
	int32_t e = -1;
	int i;

	if (len != 8) {
		return -EINVAL;
	}

	/* handle zero value special case */
	if (f64->val1 == 0LL && f64->val2 == 0LL) {
		memset(b64, 0, len);
		return 0;
	}

	/* sign handled later */
	v = abs(f64->val1);

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
	v = abs(f64->val2);

	/* add decimal to fraction */
	i = e;
	while (v > 0 && i < 52) {
		v *= 2;
		if (!f && e < 0 && v < LWM2M_FLOAT64_DEC_MAX) {
			/* handle -e */
			e--;
			continue;
		} else if (v >= LWM2M_FLOAT64_DEC_MAX) {
			v -= LWM2M_FLOAT64_DEC_MAX;
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
	if (f64->val1 == 0) {
		b64[0] = f64->val2 < 0 ? 0x80 : 0;
	} else {
		b64[0] = f64->val1 < 0 ? 0x80 : 0;
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

/* convert from binary32 to float32 */
int lwm2m_b32_to_f32(uint8_t *b32, size_t len, float32_value_t *f32)
{
	int32_t f, k, i, e;
	bool sign = false;

	if (len != 4) {
		return -EINVAL;
	}

	f32->val1 = 0;
	f32->val2 = 0;

	/* calc sign: bit 31 */
	sign = SHIFT_RIGHT(b32[0], 7, 0x1);

	/* calc exponent: bits 30-23 */
	e  = SHIFT_LEFT(b32[0], 1, 0xFF);
	e += SHIFT_RIGHT(b32[1], 7, 0x1);
	/* remove bias */
	e -= 127;

	/* enable "hidden" fraction bit 23 which is always 1 */
	f  = ((int32_t)1 << 22);
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

		f32->val1 = (f >> (23 - e)) * (sign ? -1 : 1);
	}

	/* calculate the rest of the decimal */
	k = LWM2M_FLOAT32_DEC_MAX;

	/* account for -e */
	while (e < -1) {
		k /= 2;
		e++;
	}

	for (i = 22 - e; i >= 0; i--) {
		k /= 2;
		if (f & (1 << i)) {
			f32->val2 += k;

		}
	}

	return 0;
}

/* convert from binary64 to float64 */
int lwm2m_b64_to_f64(uint8_t *b64, size_t len, float64_value_t *f64)
{
	int64_t f, k;
	int i, e;
	bool sign = false;

	if (len != 8) {
		return -EINVAL;
	}

	f64->val1 = 0LL;
	f64->val2 = 0LL;

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

		f64->val1 = (f >> (52 - e)) * (sign ? -1 : 1);
	}

	/* calculate the rest of the decimal */
	k = LWM2M_FLOAT64_DEC_MAX;

	/* account for -e */
	while (e < -1) {
		k /= 2;
		e++;
	}

	for (i = 51 - e; i >= 0; i--) {
		k /= 2;
		if (f & ((int64_t)1 << i)) {
			f64->val2 += k;

		}
	}

	return 0;
}
