/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include "rand.h"

#include <bluetooth/log.h>
#include "debug.h"

#define RAND_RESERVED (4)

struct rand {
	uint8_t count;
	uint8_t first;
	uint8_t last;
	uint8_t rand[1];
};

static struct rand *rng;

void rand_init(uint8_t *context, uint8_t context_len)
{
	LL_ASSERT(context_len > sizeof(struct rand));

	rng = (struct rand *)context;
	rng->count = context_len - sizeof(struct rand) + 1;
	rng->first = rng->last = 0;

	NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Msk;
	NRF_RNG->EVENTS_VALRDY = 0;
	NRF_RNG->INTENSET = RNG_INTENSET_VALRDY_Msk;

	NRF_RNG->TASKS_START = 1;
}

size_t rand_get(size_t octets, uint8_t *rand)
{
	uint8_t reserved;
	uint8_t first;

	while (octets) {
		if (rng->first == rng->last) {
			break;
		}

		rand[--octets] = rng->rand[rng->first];

		first = rng->first + 1;
		if (first == rng->count) {
			first = 0;
		}
		rng->first = first;
	}

	reserved = RAND_RESERVED;
	first = rng->first;
	while (reserved--) {
		if (first == rng->last) {
			NRF_RNG->TASKS_START = 1;

			break;
		}

		first++;
		if (first == rng->count) {
			first = 0;
		}
	}

	return octets;
}

void isr_rand(void *param)
{
	ARG_UNUSED(param);

	if (NRF_RNG->EVENTS_VALRDY) {
		uint8_t last;

		last = rng->last + 1;
		if (last == rng->count) {
			last = 0;
		}

		if (last == rng->first) {
			/* this condition should not happen
			 * , but due to probable bug in HW
			 * , new value could be generated
			 * before task is stopped.
			 */
			NRF_RNG->TASKS_STOP = 1;
			NRF_RNG->EVENTS_VALRDY = 0;

			return;
		}

		rng->rand[rng->last] = NRF_RNG->VALUE;
		rng->last = last;

		last = rng->last + 1;
		if (last == rng->count) {
			last = 0;
		}

		NRF_RNG->EVENTS_VALRDY = 0;

		if (last == rng->first) {
			NRF_RNG->TASKS_STOP = 1;
		}
	}
}
