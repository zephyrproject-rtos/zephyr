/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/sys/util.h>

static bool rc_bit(uint32_t data, uint8_t nbits, uint8_t i, bool msb_first)
{
	uint8_t pos = msb_first ? (nbits - 1U - i) : i;

	return ((data >> pos) & 1U) != 0U;
}

void rc_raw_put(struct rc_raw_writer *w, bool pulse, uint32_t us)
{
	if ((us == 0U) || ((w->count == 0U) && !pulse)) {
		return;
	}

	if ((w->count > 0U) && (rc_raw_is_pulse(w->words[w->count - 1U]) == pulse)) {
		uint32_t *last = &w->words[w->count - 1U];
		uint32_t sum = rc_raw_duration_us(*last) + MIN(us, RC_RAW_DURATION_MASK);

		*last = pulse ? RC_RAW_PULSE_US(sum) : RC_RAW_SPACE_US(sum);
		return;
	}

	if (w->count == w->max) {
		w->overflow = true;
		return;
	}

	w->words[w->count++] = pulse ? RC_RAW_PULSE_US(us) : RC_RAW_SPACE_US(us);
}

void rc_gen_manchester(struct rc_raw_writer *w, uint32_t data, uint8_t nbits, uint32_t half_us,
		       bool one_pulse_first)
{
	for (uint8_t i = 0U; i < nbits; i++) {
		bool first = (rc_bit(data, nbits, i, true) == one_pulse_first);

		rc_raw_put(w, first, half_us);
		rc_raw_put(w, !first, half_us);
	}
}

void rc_gen_pd(struct rc_raw_writer *w, const struct rc_gen_pd_timings *t, uint32_t data,
	       uint8_t nbits)
{
	rc_raw_put(w, true, t->header_pulse);
	rc_raw_put(w, false, t->header_space);

	for (uint8_t i = 0U; i < nbits; i++) {
		rc_raw_put(w, true, t->bit_pulse);
		rc_raw_put(w, false, t->bit_space[rc_bit(data, nbits, i, t->msb_first) ? 1 : 0]);
	}

	rc_raw_put(w, true, t->trailer_pulse);
}

void rc_gen_pl(struct rc_raw_writer *w, const struct rc_gen_pl_timings *t, uint32_t data,
	       uint8_t nbits)
{
	rc_raw_put(w, true, t->header_pulse);

	for (uint8_t i = 0U; i < nbits; i++) {
		rc_raw_put(w, false, t->bit_space);
		rc_raw_put(w, true, t->bit_pulse[rc_bit(data, nbits, i, t->msb_first) ? 1 : 0]);
	}
}

int rc_raw_end(struct rc_raw_writer *w)
{
	if (w->overflow) {
		return -ENOBUFS;
	}

	if ((w->count > 0U) && !rc_raw_is_pulse(w->words[w->count - 1U])) {
		w->count--;
	}

	return (int)w->count;
}
