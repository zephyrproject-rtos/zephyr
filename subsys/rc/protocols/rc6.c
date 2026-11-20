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

#define RC6_UNIT_US           444U
#define RC6_LEADER_PULSE_US   (6U * RC6_UNIT_US)
#define RC6_LEADER_SPACE_US   (2U * RC6_UNIT_US)
#define RC6_MIN_TIMEOUT_US    (6U * RC6_UNIT_US)
/* Leader mark + space, split from the Sony header (3000 us) at the midpoint */
#define RC6_LEADER_PERIOD_MIN 3276U
#define RC6_HEADER_BITS       4U
#define RC6_TR_UNITS          2U
#define RC6_MODE_0            0U
#define RC6_MODE_6A           6U
#define RC6_0_BITS            16U
#define RC6_MCE_TOGGLE        BIT(15)
#define RC6_LCC_MASK          0xffff0000U

enum rc6_phase {
	RC6_IDLE,
	RC6_LEADER_SPACE,
	RC6_HEADER,
	RC6_BODY,
};

enum rc6_half {
	RC6_HALF_NONE,
	RC6_HALF_SPACE,
	RC6_HALF_PULSE,
};

struct rc6_state {
	uint32_t body;
	uint16_t leader_us;
	uint8_t header;
	uint8_t count;
	uint8_t phase;
	uint8_t half;
};

static const uint32_t rc6_mce_codes[] = {0x800f0000U, 0x80340000U, 0x80460000U};

/* Units of a duration, split at the midpoints between whole units; 0 if at a midpoint */
static uint32_t rc6_units(uint32_t us)
{
	if ((((2U * us) + RC6_UNIT_US) % (2U * RC6_UNIT_US)) == 0U) {
		return 0U;
	}

	return ((2U * us) + RC6_UNIT_US) / (2U * RC6_UNIT_US);
}

static bool rc6_is_mce(uint32_t body)
{
	ARRAY_FOR_EACH(rc6_mce_codes, i) {
		if ((body & RC6_LCC_MASK) == rc6_mce_codes[i]) {
			return true;
		}
	}

	return false;
}

static void rc6_reset(void *state)
{
	struct rc6_state *s = state;

	*s = (struct rc6_state){0};
}

static uint32_t rc6_mode(const struct rc6_state *s)
{
	return (s->header >> 1) & 0x7U;
}

/* Header byte: start bit, 3 mode bits, toggle bit */
static bool rc6_header_ok(const struct rc6_state *s)
{
	uint32_t mode = rc6_mode(s);
	bool toggle = (s->header & 1U) != 0U;

	if ((s->header & BIT(4)) == 0U) {
		return false;
	}

	return (mode == RC6_MODE_0) || ((mode == RC6_MODE_6A) && !toggle);
}

static bool rc6_result(const struct rc6_state *s, struct rc_decode_result *res)
{
	res->flags = 0U;
	res->scancode = s->body;

	if (rc6_mode(s) == RC6_MODE_0) {
		res->proto = RC_PROTO_RC6_0;
		if ((s->header & 1U) != 0U) {
			res->flags = RC_EVENT_FLAG_TOGGLE;
		}
		return s->count == RC6_0_BITS;
	}

	switch (s->count) {
	case 20U:
		res->proto = RC_PROTO_RC6_6A_20;
		return true;
	case 24U:
		res->proto = RC_PROTO_RC6_6A_24;
		return true;
	case 32U:
		if (rc6_is_mce(s->body)) {
			res->proto = RC_PROTO_RC6_MCE;
			res->scancode &= ~RC6_MCE_TOGGLE;
			if ((s->body & RC6_MCE_TOGGLE) != 0U) {
				res->flags = RC_EVENT_FLAG_TOGGLE;
			}
		} else {
			res->proto = RC_PROTO_RC6_6A_32;
		}
		return true;
	default:
		return false;
	}
}

/* Feed one half bit; a 1 is a pulse then a space */
static bool rc6_half(struct rc6_state *s, bool pulse)
{
	bool one;

	if (s->half == RC6_HALF_NONE) {
		s->half = pulse ? RC6_HALF_PULSE : RC6_HALF_SPACE;
		return true;
	}

	if ((s->half == RC6_HALF_PULSE) == pulse) {
		return false;
	}

	one = (s->half == RC6_HALF_PULSE);
	s->half = RC6_HALF_NONE;

	if (s->phase == RC6_HEADER) {
		s->header = (uint8_t)((s->header << 1) | (one ? 1U : 0U));
		if (++s->count == (RC6_HEADER_BITS + 1U)) {
			if (!rc6_header_ok(s)) {
				return false;
			}
			s->phase = RC6_BODY;
			s->count = 0U;
		}
		return true;
	}

	if (s->count == 32U) {
		return false;
	}
	s->body = (s->body << 1) | (one ? 1U : 0U);
	s->count++;

	return true;
}

static bool rc6_is_leader(bool pulse, uint32_t us)
{
	return pulse && (us > (RC6_LEADER_PULSE_US - RC6_UNIT_US)) &&
	       (us < (RC6_LEADER_PULSE_US + RC6_UNIT_US));
}

/* A word that breaks a frame may be the leader of the next one */
static void rc6_restart(struct rc6_state *s, bool pulse, uint32_t us)
{
	rc6_reset(s);
	if (rc6_is_leader(pulse, us)) {
		s->phase = RC6_LEADER_SPACE;
		s->leader_us = (uint16_t)us;
	}
}

static int rc6_end(struct rc6_state *s, struct rc_decode_result *res)
{
	bool done = (s->phase == RC6_BODY) &&
		    ((s->half == RC6_HALF_NONE) ||
		     ((s->half == RC6_HALF_PULSE) && rc6_half(s, false))) &&
		    (s->half == RC6_HALF_NONE) && rc6_result(s, res);

	rc6_reset(s);

	return done ? 1 : 0;
}

static int rc6_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	struct rc6_state *s = state;
	bool pulse = rc_raw_is_pulse(word);
	uint32_t us = rc_raw_duration_us(word);
	uint32_t n = rc6_units(us);

	if ((word & RC_RAW_TIMEOUT) != 0U) {
		return rc6_end(s, res);
	}

	switch (s->phase) {
	case RC6_IDLE:
		rc6_restart(s, pulse, us);
		return 0;
	case RC6_LEADER_SPACE:
		if (!pulse && (n == 2U) && ((s->leader_us + us) > RC6_LEADER_PERIOD_MIN)) {
			s->phase = RC6_HEADER;
		} else {
			rc6_restart(s, pulse, us);
		}
		return 0;
	default:
		break;
	}

	/* In the body a space of 3 units or more ends the frame */
	if ((s->phase == RC6_BODY) && !pulse && ((n == 0U) || (n >= 3U))) {
		return rc6_end(s, res);
	}

	if (n == 0U) {
		rc6_restart(s, pulse, us);
		return 0;
	}

	while (n > 0U) {
		uint32_t w = ((s->phase == RC6_HEADER) && (s->count == RC6_HEADER_BITS))
				     ? RC6_TR_UNITS
				     : 1U;

		if ((n < w) || !rc6_half(s, pulse)) {
			rc6_restart(s, pulse, us);
			return 0;
		}
		n -= w;

		if ((s->phase == RC6_BODY) && (rc6_mode(s) == RC6_MODE_0) &&
		    (s->count == RC6_0_BITS) && (s->half == RC6_HALF_NONE)) {
			int ret = rc6_result(s, res) ? 1 : 0;

			rc6_reset(s);
			return ret;
		}
	}

	return 0;
}

static int rc6_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max)
{
	bool toggle = (flags & RC_EVENT_FLAG_TOGGLE) != 0U;
	uint32_t mode = RC6_MODE_6A;
	uint32_t tr = 0U;
	uint8_t nbits;
	struct rc_raw_writer w;

	switch (proto) {
	case RC_PROTO_RC6_0:
		if (scancode > 0xffffU) {
			return -EINVAL;
		}
		mode = RC6_MODE_0;
		tr = toggle ? 1U : 0U;
		nbits = RC6_0_BITS;
		break;
	case RC_PROTO_RC6_6A_20:
		if (scancode > 0xfffffU) {
			return -EINVAL;
		}
		nbits = 20U;
		break;
	case RC_PROTO_RC6_6A_24:
		if (scancode > 0xffffffU) {
			return -EINVAL;
		}
		nbits = 24U;
		break;
	case RC_PROTO_RC6_6A_32:
		/* Such a customer code decodes as MCE */
		if (rc6_is_mce(scancode)) {
			return -EINVAL;
		}
		nbits = 32U;
		break;
	case RC_PROTO_RC6_MCE:
		if (!rc6_is_mce(scancode) || ((scancode & RC6_MCE_TOGGLE) != 0U)) {
			return -EINVAL;
		}
		if (toggle) {
			scancode |= RC6_MCE_TOGGLE;
		}
		nbits = 32U;
		break;
	default:
		return -EINVAL;
	}

	rc_raw_writer_init(&w, words, max);
	rc_raw_put(&w, true, RC6_LEADER_PULSE_US);
	rc_raw_put(&w, false, RC6_LEADER_SPACE_US);
	rc_gen_manchester(&w, BIT(3) | mode, RC6_HEADER_BITS, RC6_UNIT_US, true);
	rc_gen_manchester(&w, tr, 1U, RC6_TR_UNITS * RC6_UNIT_US, true);
	rc_gen_manchester(&w, scancode, nbits, RC6_UNIT_US, true);

	return rc_raw_end(&w);
}

RC_PROTOCOL_DEFINE(rc_protocol_rc6) = {
	.name = "rc6",
	.proto_first = RC_PROTO_RC6_0,
	.proto_last = RC_PROTO_RC6_MCE,
	.state_size = sizeof(struct rc6_state),
	.min_timeout_us = RC6_MIN_TIMEOUT_US,
	.carrier_hz = 36000U,
	.repeat_period_ms = 114U,
	.tx_period_ms = 114U,
	.reset = rc6_reset,
	.decode = rc6_decode,
	.encode = rc6_encode,
};
