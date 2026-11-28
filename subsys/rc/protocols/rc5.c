/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>

#define RC5_UNIT_US           889U
#define RC5_NBITS             14U
#define RC5X_NBITS            20U
#define RC5X_GAP_AT           8U
#define RC5X_GAP_UNITS        4U
#define RC5_MAX_UNITS         6U
#define RC5_MIN_TIMEOUT_US    (6U * RC5_UNIT_US)
/* Shortest trailer space, the lower bound of the 6 unit class, alone or after a half bit */
#define RC5_TRAILER_MIN_US    ((11U * RC5_UNIT_US / 2U) + 1U)
#define RC5_LAST_SPACE_MIN_US (RC5_UNIT_US + RC5_TRAILER_MIN_US)

#define RC5_SCANCODE_MASK  0x1f7fU
#define RC5X_SCANCODE_MASK 0x1f7f3fU

enum rc5_half {
	RC5_HALF_NONE,
	RC5_HALF_SPACE,
	RC5_HALF_PULSE,
};

struct rc5_state {
	uint32_t bits;
	uint8_t count;
	uint8_t nbits;
	uint8_t half;
	bool active;
	bool busy;
};

/* Units of a duration; n units span ((2n - 1) * u / 2, (2n + 1) * u / 2) */
static uint32_t rc5_units(uint32_t us)
{
	uint32_t n = ((2U * us) + RC5_UNIT_US) / (2U * RC5_UNIT_US);

	return (n <= RC5_MAX_UNITS) ? n : 0U;
}

static void rc5_reset(void *state)
{
	struct rc5_state *s = state;

	*s = (struct rc5_state){0};
}

static void rc5_result(const struct rc5_state *s, struct rc_decode_result *res)
{
	uint32_t b = s->bits;
	uint32_t cmd;

	res->flags = 0U;
	if (s->nbits == RC5X_NBITS) {
		cmd = ((b >> 6) & 0x3fU) | (((b & BIT(18)) != 0U) ? 0U : 0x40U);
		res->proto = RC_PROTO_RC5X_20;
		res->scancode = (((b >> 12) & 0x1fU) << 16) | (cmd << 8) | (b & 0x3fU);
		if ((b & BIT(17)) != 0U) {
			res->flags = RC_EVENT_FLAG_TOGGLE;
		}
	} else {
		cmd = (b & 0x3fU) | (((b & BIT(12)) != 0U) ? 0U : 0x40U);
		res->proto = RC_PROTO_RC5;
		res->scancode = (((b >> 6) & 0x1fU) << 8) | cmd;
		if ((b & BIT(11)) != 0U) {
			res->flags = RC_EVENT_FLAG_TOGGLE;
		}
	}
}

/* A 1 is a space then a pulse */
static bool rc5_half(struct rc5_state *s, bool pulse)
{
	if (s->half == RC5_HALF_NONE) {
		s->half = pulse ? RC5_HALF_PULSE : RC5_HALF_SPACE;
		return true;
	}

	if ((s->half == RC5_HALF_PULSE) == pulse) {
		return false;
	}

	s->bits = (s->bits << 1) | ((s->half == RC5_HALF_SPACE) ? 1U : 0U);
	s->count++;
	s->half = RC5_HALF_NONE;

	return true;
}

/* S1 is a 1: its space half is the idle line, its pulse half starts the frame */
static bool rc5_start(struct rc5_state *s, bool pulse, uint32_t n)
{
	if (!pulse || (n == 0U) || (n > 2U)) {
		return false;
	}

	s->active = true;
	s->nbits = RC5_NBITS;
	s->half = RC5_HALF_SPACE;

	return true;
}

/* Drop the frame; the next one may start only after a trailer space or a timeout */
static void rc5_abort(struct rc5_state *s)
{
	rc5_reset(s);
	s->busy = true;
}

static int rc5_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	struct rc5_state *s = state;
	bool timeout = (word & RC_RAW_TIMEOUT) != 0U;
	bool pulse = rc_raw_is_pulse(word);
	uint32_t us = rc_raw_duration_us(word);
	uint32_t n = rc5_units(us);
	uint32_t done = s->count + ((s->half == RC5_HALF_PULSE) ? 1U : 0U);
	bool gap = s->active && (s->nbits == RC5_NBITS) && (done == RC5X_GAP_AT);

	/* A trailer space or a timeout ends a frame, merged with the last half bit if pending */
	if (!pulse && (timeout || ((us >= RC5_TRAILER_MIN_US) && !gap))) {
		bool ended = s->active &&
			     ((s->count == s->nbits) ||
			      ((s->half == RC5_HALF_PULSE) && ((s->count + 1U) == s->nbits) &&
			       (timeout || (us >= RC5_LAST_SPACE_MIN_US)) && rc5_half(s, false)));

		if (ended) {
			rc5_result(s, res);
		}
		rc5_reset(s);
		return ended ? 1 : 0;
	}

	if (s->busy || (n == 0U) || (s->active && (s->count == s->nbits)) ||
	    (!s->active && !rc5_start(s, pulse, n))) {
		rc5_abort(s);
		return 0;
	}

	while (n > 0U) {
		if ((s->count == RC5X_GAP_AT) && (s->half == RC5_HALF_NONE) && !pulse &&
		    (s->nbits == RC5_NBITS) && (n >= RC5X_GAP_UNITS)) {
			s->nbits = RC5X_NBITS;
			n -= RC5X_GAP_UNITS;
			continue;
		}

		if ((s->count == s->nbits) || !rc5_half(s, pulse)) {
			rc5_abort(s);
			return 0;
		}
		n--;
	}

	return 0;
}

static int rc5_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max)
{
	uint32_t toggle = ((flags & RC_EVENT_FLAG_TOGGLE) != 0U) ? 1U : 0U;
	struct rc_raw_writer w;
	uint32_t head;
	uint32_t cmd;

	rc_raw_writer_init(&w, words, max);

	switch (proto) {
	case RC_PROTO_RC5:
		if ((scancode & ~RC5_SCANCODE_MASK) != 0U) {
			return -EINVAL;
		}
		cmd = scancode & 0x7fU;
		head = BIT(2) | ((cmd < 0x40U) ? BIT(1) : 0U) | toggle;
		rc_gen_manchester(&w,
				  (head << 11) | (((scancode >> 8) & 0x1fU) << 6) | (cmd & 0x3fU),
				  RC5_NBITS, RC5_UNIT_US, false);
		break;
	case RC_PROTO_RC5X_20:
		if ((scancode & ~RC5X_SCANCODE_MASK) != 0U) {
			return -EINVAL;
		}
		cmd = (scancode >> 8) & 0x7fU;
		head = BIT(2) | ((cmd < 0x40U) ? BIT(1) : 0U) | toggle;
		rc_gen_manchester(&w, (head << 5) | ((scancode >> 16) & 0x1fU), RC5X_GAP_AT,
				  RC5_UNIT_US, false);
		rc_raw_put(&w, false, RC5X_GAP_UNITS * RC5_UNIT_US);
		rc_gen_manchester(&w, ((cmd & 0x3fU) << 6) | (scancode & 0x3fU),
				  RC5X_NBITS - RC5X_GAP_AT, RC5_UNIT_US, false);
		break;
	default:
		return -EINVAL;
	}

	return rc_raw_end(&w);
}

RC_PROTOCOL_DEFINE(rc_protocol_rc5) = {
	.name = "rc5",
	.proto_first = RC_PROTO_RC5,
	.proto_last = RC_PROTO_RC5X_20,
	.state_size = sizeof(struct rc5_state),
	.min_timeout_us = RC5_MIN_TIMEOUT_US,
	.carrier_hz = 36000U,
	.repeat_period_ms = 114U,
	.tx_period_ms = 114U,
	.reset = rc5_reset,
	.decode = rc5_decode,
	.encode = rc5_encode,
};
