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

#define NEC_BITS           32U
#define NEC_FRAME_WORDS    (2U + (2U * NEC_BITS) + 1U)
#define NEC_REPEAT_WORDS   3U
#define NEC_MIN_TIMEOUT_US 5625U

#define NEC_LEADER_PULSE_US 9000U
#define NEC_LEADER_SPACE_US 4500U
#define NEC_REPEAT_SPACE_US 2250U
#define NEC_UNIT_US         563U
#define NEC_ONE_SPACE_US    1688U

#define NEC_LEADER_PULSE_MIN 7875U
#define NEC_LEADER_PULSE_MAX 10125U
#define NEC_LEADER_SPACE_MIN 3938U
#define NEC_LEADER_SPACE_MAX 5063U
#define NEC_REPEAT_SPACE_MIN 1688U
#define NEC_REPEAT_SPACE_MAX 2813U
#define NEC_UNIT_MIN         282U
#define NEC_UNIT_MAX         844U
#define NEC_ONE_SPACE_MIN    1407U
#define NEC_ONE_SPACE_MAX    1969U

enum nec_phase {
	NEC_IDLE,
	NEC_LEADER_SPACE,
	NEC_BIT_PULSE,
	NEC_BIT_SPACE,
	NEC_REPEAT_PULSE,
};

struct nec_state {
	uint32_t bits;
	uint8_t phase;
	uint8_t count;
};

static bool nec_in(uint32_t us, uint32_t min, uint32_t max)
{
	return (us >= min) && (us <= max);
}

static void nec_reset(void *state)
{
	struct nec_state *s = state;

	s->bits = 0U;
	s->phase = NEC_IDLE;
	s->count = 0U;
}

static void nec_scancode(uint32_t bits, struct rc_decode_result *res)
{
	uint8_t b0 = bits & 0xffU;
	uint8_t b1 = (bits >> 8) & 0xffU;
	uint8_t b2 = (bits >> 16) & 0xffU;
	uint8_t b3 = (bits >> 24) & 0xffU;

	res->flags = 0U;
	if ((b2 ^ b3) != 0xffU) {
		res->proto = RC_PROTO_NEC32;
		res->scancode =
			((uint32_t)b1 << 24) | ((uint32_t)b0 << 16) | ((uint32_t)b3 << 8) | b2;
	} else if ((b0 ^ b1) != 0xffU) {
		res->proto = RC_PROTO_NECX;
		res->scancode = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | b2;
	} else {
		res->proto = RC_PROTO_NEC;
		res->scancode = ((uint32_t)b0 << 8) | b2;
	}
}

static int nec_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	struct nec_state *s = state;
	uint32_t us = rc_raw_duration_us(word);
	bool pulse = rc_raw_is_pulse(word);

	if ((word & RC_RAW_TIMEOUT) != 0U) {
		nec_reset(s);
		return 0;
	}

	switch (s->phase) {
	case NEC_LEADER_SPACE:
		if (!pulse && nec_in(us, NEC_LEADER_SPACE_MIN, NEC_LEADER_SPACE_MAX)) {
			s->bits = 0U;
			s->count = 0U;
			s->phase = NEC_BIT_PULSE;
			return 0;
		}
		if (!pulse && nec_in(us, NEC_REPEAT_SPACE_MIN, NEC_REPEAT_SPACE_MAX)) {
			s->phase = NEC_REPEAT_PULSE;
			return 0;
		}
		break;
	case NEC_BIT_PULSE:
		if (pulse && nec_in(us, NEC_UNIT_MIN, NEC_UNIT_MAX)) {
			if (s->count == NEC_BITS) {
				nec_scancode(s->bits, res);
				nec_reset(s);
				return 1;
			}
			s->phase = NEC_BIT_SPACE;
			return 0;
		}
		break;
	case NEC_BIT_SPACE:
		if (!pulse && nec_in(us, NEC_UNIT_MIN, NEC_UNIT_MAX)) {
			s->count++;
			s->phase = NEC_BIT_PULSE;
			return 0;
		}
		if (!pulse && nec_in(us, NEC_ONE_SPACE_MIN, NEC_ONE_SPACE_MAX)) {
			s->bits |= BIT(s->count);
			s->count++;
			s->phase = NEC_BIT_PULSE;
			return 0;
		}
		break;
	case NEC_REPEAT_PULSE:
		if (pulse && nec_in(us, NEC_UNIT_MIN, NEC_UNIT_MAX)) {
			res->proto = RC_PROTO_NEC;
			res->flags = RC_EVENT_FLAG_REPEAT;
			res->scancode = 0U;
			nec_reset(s);
			return 1;
		}
		break;
	default:
		break;
	}

	/* An unexpected word may start the next frame */
	nec_reset(s);
	if (pulse && nec_in(us, NEC_LEADER_PULSE_MIN, NEC_LEADER_PULSE_MAX)) {
		s->phase = NEC_LEADER_SPACE;
	}

	return 0;
}

static int nec_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max)
{
	uint8_t b[4];
	size_t n = 0;

	ARG_UNUSED(flags);

	switch (proto) {
	case RC_PROTO_NEC:
		if (scancode > 0xffffU) {
			return -EINVAL;
		}
		b[0] = (scancode >> 8) & 0xffU;
		b[1] = ~b[0];
		b[2] = scancode & 0xffU;
		b[3] = ~b[2];
		break;
	case RC_PROTO_NECX:
		if (scancode > 0xffffffU) {
			return -EINVAL;
		}
		b[0] = (scancode >> 16) & 0xffU;
		b[1] = (scancode >> 8) & 0xffU;
		b[2] = scancode & 0xffU;
		b[3] = ~b[2];
		break;
	case RC_PROTO_NEC32:
		b[0] = (scancode >> 16) & 0xffU;
		b[1] = (scancode >> 24) & 0xffU;
		b[2] = scancode & 0xffU;
		b[3] = (scancode >> 8) & 0xffU;
		break;
	default:
		return -EINVAL;
	}

	if (max < NEC_FRAME_WORDS) {
		return -ENOBUFS;
	}

	words[n++] = RC_RAW_PULSE_US(NEC_LEADER_PULSE_US);
	words[n++] = RC_RAW_SPACE_US(NEC_LEADER_SPACE_US);
	for (unsigned int i = 0; i < NEC_BITS; i++) {
		bool one = ((b[i / 8U] >> (i % 8U)) & 1U) != 0U;

		words[n++] = RC_RAW_PULSE_US(NEC_UNIT_US);
		words[n++] = RC_RAW_SPACE_US(one ? NEC_ONE_SPACE_US : NEC_UNIT_US);
	}
	words[n++] = RC_RAW_PULSE_US(NEC_UNIT_US);

	return (int)n;
}

static int nec_encode_repeat(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
			     size_t max)
{
	ARG_UNUSED(proto);
	ARG_UNUSED(scancode);
	ARG_UNUSED(flags);

	if (max < NEC_REPEAT_WORDS) {
		return -ENOBUFS;
	}

	words[0] = RC_RAW_PULSE_US(NEC_LEADER_PULSE_US);
	words[1] = RC_RAW_SPACE_US(NEC_REPEAT_SPACE_US);
	words[2] = RC_RAW_PULSE_US(NEC_UNIT_US);

	return (int)NEC_REPEAT_WORDS;
}

RC_PROTOCOL_DEFINE(rc_protocol_nec) = {
	.name = "nec",
	.proto_first = RC_PROTO_NEC,
	.proto_last = RC_PROTO_NEC32,
	.state_size = sizeof(struct nec_state),
	.min_timeout_us = NEC_MIN_TIMEOUT_US,
	.carrier_hz = 38000U,
	.repeat_period_ms = 110U,
	.tx_period_ms = 108U,
	.reset = nec_reset,
	.decode = nec_decode,
	.encode = nec_encode,
	.encode_repeat = nec_encode_repeat,
};
