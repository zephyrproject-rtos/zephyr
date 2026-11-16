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

#define SONY_UNIT_US        600U
#define SONY_HEADER_US      (4U * SONY_UNIT_US)
#define SONY_ONE_US         (2U * SONY_UNIT_US)
#define SONY_MIN_TIMEOUT_US 6000U
#define SONY_MAX_BITS       20U

/* Classes split at the midpoints: unit (300, 900), one (900, 1500), header (1800, 3000) */
#define SONY_UNIT_MIN          (SONY_UNIT_US / 2U)
#define SONY_UNIT_MAX          ((SONY_UNIT_US + SONY_ONE_US) / 2U)
#define SONY_ONE_MAX           (SONY_ONE_US + (SONY_UNIT_US / 2U))
#define SONY_HEADER_MIN        ((SONY_ONE_US + SONY_HEADER_US) / 2U)
#define SONY_HEADER_MAX        (SONY_HEADER_US + SONY_UNIT_US)
/* Header mark + space, split at the midpoints from RC5 (2667 us) and the RC6 leader (3552 us) */
#define SONY_HEADER_PERIOD_MIN 2833U
#define SONY_HEADER_PERIOD_MAX 3276U

#define SONY12_MASK 0x1f007fU
#define SONY15_MASK 0xff007fU
#define SONY20_MASK 0x1fff7fU

enum sony_phase {
	SONY_IDLE,
	SONY_HEADER_SPACE,
	SONY_BIT_PULSE,
	SONY_BIT_SPACE,
};

struct sony_state {
	uint32_t bits;
	uint16_t header_us;
	uint8_t count;
	uint8_t phase;
};

static bool sony_in(uint32_t us, uint32_t min, uint32_t max)
{
	return (us > min) && (us < max);
}

static void sony_reset(void *state)
{
	struct sony_state *s = state;

	*s = (struct sony_state){0};
}

static void sony_restart(struct sony_state *s, bool pulse, uint32_t us)
{
	sony_reset(s);
	if (pulse && sony_in(us, SONY_HEADER_MIN, SONY_HEADER_MAX)) {
		s->phase = SONY_HEADER_SPACE;
		s->header_us = (uint16_t)us;
	}
}

static int sony_end(struct sony_state *s, struct rc_decode_result *res)
{
	uint32_t b = s->bits;
	uint32_t device = (b >> 7) & 0x1fU;
	uint32_t subdevice = 0U;
	int ret = 1;

	switch (s->count) {
	case 12U:
		res->proto = RC_PROTO_SONY12;
		break;
	case 15U:
		res->proto = RC_PROTO_SONY15;
		device = (b >> 7) & 0xffU;
		break;
	case 20U:
		res->proto = RC_PROTO_SONY20;
		subdevice = (b >> 12) & 0xffU;
		break;
	default:
		ret = 0;
		break;
	}

	res->flags = 0U;
	res->scancode = (device << 16) | (subdevice << 8) | (b & 0x7fU);
	sony_reset(s);

	return ret;
}

static int sony_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	struct sony_state *s = state;
	bool pulse = rc_raw_is_pulse(word);
	uint32_t us = rc_raw_duration_us(word);

	if ((word & RC_RAW_TIMEOUT) != 0U) {
		if (s->phase == SONY_BIT_SPACE) {
			return sony_end(s, res);
		}
		sony_reset(s);
		return 0;
	}

	switch (s->phase) {
	case SONY_HEADER_SPACE:
		if (!pulse && sony_in(us, SONY_UNIT_MIN, SONY_UNIT_MAX) &&
		    sony_in(s->header_us + us, SONY_HEADER_PERIOD_MIN, SONY_HEADER_PERIOD_MAX)) {
			s->phase = SONY_BIT_PULSE;
			return 0;
		}
		break;
	case SONY_BIT_PULSE:
		if (pulse && sony_in(us, SONY_UNIT_MIN, SONY_ONE_MAX) && (us != SONY_UNIT_MAX) &&
		    (s->count < SONY_MAX_BITS)) {
			if (us > SONY_UNIT_MAX) {
				s->bits |= BIT(s->count);
			}
			s->count++;
			s->phase = SONY_BIT_SPACE;
			return 0;
		}
		break;
	case SONY_BIT_SPACE:
		if (!pulse && sony_in(us, SONY_UNIT_MIN, SONY_UNIT_MAX)) {
			s->phase = SONY_BIT_PULSE;
			return 0;
		}
		/* A space longer than a bit space ends the frame */
		if (!pulse && (us >= SONY_UNIT_MAX)) {
			return sony_end(s, res);
		}
		break;
	default:
		break;
	}

	sony_restart(s, pulse, us);

	return 0;
}

static int sony_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		       size_t max)
{
	static const struct rc_gen_pl_timings timings = {
		.header_pulse = SONY_HEADER_US,
		.bit_space = SONY_UNIT_US,
		.bit_pulse = {SONY_UNIT_US, SONY_ONE_US},
	};
	uint32_t data = (scancode & 0x7fU) | (((scancode >> 16) & 0xffU) << 7);
	struct rc_raw_writer w;
	uint8_t nbits;

	ARG_UNUSED(flags);

	switch (proto) {
	case RC_PROTO_SONY12:
		if ((scancode & ~SONY12_MASK) != 0U) {
			return -EINVAL;
		}
		nbits = 12U;
		break;
	case RC_PROTO_SONY15:
		if ((scancode & ~SONY15_MASK) != 0U) {
			return -EINVAL;
		}
		nbits = 15U;
		break;
	case RC_PROTO_SONY20:
		if ((scancode & ~SONY20_MASK) != 0U) {
			return -EINVAL;
		}
		data |= ((scancode >> 8) & 0xffU) << 12;
		nbits = 20U;
		break;
	default:
		return -EINVAL;
	}

	rc_raw_writer_init(&w, words, max);
	rc_gen_pl(&w, &timings, data, nbits);

	return rc_raw_end(&w);
}

RC_PROTOCOL_DEFINE(rc_protocol_sony) = {
	.name = "sony",
	.proto_first = RC_PROTO_SONY12,
	.proto_last = RC_PROTO_SONY20,
	.state_size = sizeof(struct sony_state),
	.min_timeout_us = SONY_MIN_TIMEOUT_US,
	.carrier_hz = 40000U,
	.repeat_period_ms = 100U,
	.tx_period_ms = 45U,
	.reset = sony_reset,
	.decode = sony_decode,
	.encode = sony_encode,
};
