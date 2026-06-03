/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pulse_io.h>

int pulse_io_encode_bytes(const struct pulse_io_bit_template *tmpl, const uint8_t *bytes,
			  size_t nbytes, struct pulse_symbol *out, size_t out_cap, size_t *produced)
{
	size_t need = nbytes * 8U * 2U;
	size_t idx = 0;

	if (tmpl == NULL || bytes == NULL || out == NULL || produced == NULL) {
		return -EINVAL;
	}
	if (out_cap < need) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < nbytes; i++) {
		uint8_t b = bytes[i];

		for (int bit = 0; bit < 8; bit++) {
			int pos = tmpl->msb_first ? (7 - bit) : bit;
			bool one = (b >> pos) & 0x1;
			const struct pulse_symbol *src = one ? tmpl->one : tmpl->zero;

			out[idx++] = src[0];
			out[idx++] = src[1];
		}
	}

	*produced = idx;
	return 0;
}

static bool within(uint32_t value, uint32_t target, uint32_t tol)
{
	uint32_t lo = (target > tol) ? (target - tol) : 0U;

	return value >= lo && value <= target + tol;
}

int pulse_io_decode_bytes(const struct pulse_io_bit_template *tmpl, uint32_t tolerance_ticks,
			  const struct pulse_symbol *in, size_t nsyms, uint8_t *out, size_t out_cap,
			  size_t *produced)
{
	size_t nbits = nsyms / 2U;
	size_t nbytes = nbits / 8U;
	size_t idx = 0;

	if (tmpl == NULL || in == NULL || out == NULL || produced == NULL) {
		return -EINVAL;
	}
	if (out_cap < nbytes) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < nbytes; i++) {
		uint8_t b = 0;

		for (int bit = 0; bit < 8; bit++) {
			const struct pulse_symbol *sym = &in[(i * 8U + bit) * 2U];
			int pos = tmpl->msb_first ? (7 - bit) : bit;

			if (within(sym->duration, tmpl->one[0].duration, tolerance_ticks)) {
				b |= (uint8_t)(1U << pos);
			} else if (within(sym->duration, tmpl->zero[0].duration,
					  tolerance_ticks)) {
				/* zero bit: leave the bit clear */
			} else {
				return -EILSEQ;
			}
		}

		out[idx++] = b;
	}

	*produced = idx;
	return 0;
}
