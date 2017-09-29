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
	u8_t first, last, remaining;

	LL_ASSERT(rng);

	first = rng->first;
	last = rng->last;

	if (first <= last) {
		u8_t *d, *s;
		u8_t avail;

		d = &rand[octets];
		s = &rng->rand[first];

		avail = last - first;
		if (octets < avail) {
			remaining = avail - octets;
			avail = octets;
		} else {
			remaining = 0;
		}

		first += avail;
		octets -= avail;

		while (avail--) {
			*(--d) = *s++;
		}

		rng->first = first;
	} else {
		u8_t *d, *s;
		u8_t avail;

		d = &rand[octets];
		s = &rng->rand[first];

		avail = rng->count - first;
		if (octets < avail) {
			remaining = avail + last - octets;
			avail = octets;
			first += avail;
		} else {
			remaining = last;
			first = 0;
		}

		octets -= avail;

		while (avail--) {
			*(--d) = *s++;
		}

		if (octets && last) {
			s = &rng->rand[0];

			if (octets < last) {
				remaining = last - octets;
				last = octets;
			} else {
				remaining = 0;
			}

			first = last;
			octets -= last;

			while (last--) {
				*(--d) = *s++;
			}
		}

		rng->first = first;
	}

	if (remaining < rng->threshold) {
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
		int ret;

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
