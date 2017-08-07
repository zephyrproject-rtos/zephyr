/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include "hal/rand.h"

#include "common/log.h"
#include "hal/debug.h"

struct rand {
	u8_t count;
	u8_t threshold;
	u8_t first;
	u8_t last;
	u8_t rand[1];
};

static struct rand *rng_isr;
static struct rand *rng_thr;

static void init(struct rand **rng, u8_t *context, u8_t len, u8_t threshold)
{
	struct rand *p;

	LL_ASSERT(len > (offsetof(struct rand, rand) + threshold));

	*rng = (struct rand *)context;

	p = *rng;
	p->count = len - offsetof(struct rand, rand);
	p->threshold = threshold;
	p->first = p->last = 0;

	if (!rng_isr || !rng_thr) {
		NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Msk;
		NRF_RNG->EVENTS_VALRDY = 0;
		NRF_RNG->INTENSET = RNG_INTENSET_VALRDY_Msk;

		NRF_RNG->TASKS_START = 1;
	}
}

void rand_init(u8_t *context, u8_t context_len, u8_t threshold)
{
	init(&rng_thr, context, context_len, threshold);
}

void rand_isr_init(u8_t *context, u8_t context_len, u8_t threshold)
{
	init(&rng_isr, context, context_len, threshold);
}

static size_t get(struct rand *rng, size_t octets, u8_t *rand)
{
	u8_t first;
	u8_t avail;

	LL_ASSERT(rng);

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

	if (rng->first <= rng->last) {
		avail = rng->last - rng->first;
	} else {
		avail = rng->count - rng->first + rng->last;
	}

	if (avail < rng->threshold) {
		NRF_RNG->TASKS_START = 1;
	}

	return octets;
}

size_t rand_get(size_t octets, u8_t *rand)
{
	return get(rng_thr, octets, rand);
}

size_t rand_isr_get(size_t octets, u8_t *rand)
{
	return get(rng_isr, octets, rand);
}

static int isr(struct rand *rng, bool store)
{
	u8_t last;

	if (!rng) {
		return -ENOBUFS;
	}

	last = rng->last + 1;
	if (last == rng->count) {
		last = 0;
	}

	if (last == rng->first) {
		/* this condition should not happen, but due to probable race,
		 * new value could be generated before NRF_RNG task is stopped.
		 */
		return -ENOBUFS;
	}

	if (!store) {
		return -EBUSY;
	}

	rng->rand[rng->last] = NRF_RNG->VALUE;
	rng->last = last;

	last = rng->last + 1;
	if (last == rng->count) {
		last = 0;
	}

	if (last == rng->first) {
		return 0;
	}

	return -EBUSY;
}

void isr_rand(void *param)
{
	ARG_UNUSED(param);

	if (NRF_RNG->EVENTS_VALRDY) {
		u8_t ret;

		ret = isr(rng_isr, true);
		if (ret != -EBUSY) {
			ret = isr(rng_thr, (ret == -ENOBUFS));
		}

		NRF_RNG->EVENTS_VALRDY = 0;

		if (ret != -EBUSY) {
			NRF_RNG->TASKS_STOP = 1;
		}
	}
}
