/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <soc.h>

#include "rand.h"

#include "debug.h"

#define RAND_RESERVED (4)

struct rand {
	uint8_t count;
	uint8_t first;
	uint8_t last;
	uint8_t rand[1];
};

static struct rand *_rand;

void rand_init(uint8_t *context, uint8_t context_len)
{
	LL_ASSERT(context_len > sizeof(struct rand));

	_rand = (struct rand *)context;
	_rand->count = context_len - sizeof(struct rand) + 1;
	_rand->first = _rand->last = 0;

	NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Msk;
	NRF_RNG->EVENTS_VALRDY = 0;
	NRF_RNG->INTENSET = RNG_INTENSET_VALRDY_Msk;
	NRF_RNG->TASKS_START = 1;
}

uint32_t rand_get(uint8_t octets, uint8_t *rand)
{
	uint8_t reserved;
	uint8_t first;

	while (octets) {
		if (_rand->first == _rand->last) {
			break;
		}

		rand[--octets] = _rand->rand[_rand->first];

		first = _rand->first + 1;
		if (first == _rand->count) {
			first = 0;
		}
		_rand->first = first;
	}

	reserved = RAND_RESERVED;
	first = _rand->first;
	while (reserved--) {
		if (first == _rand->last) {
			NRF_RNG->TASKS_START = 1;

			break;
		}

		first++;
		if (first == _rand->count) {
			first = 0;
		}
	}

	return octets;
}

void rng_isr(void)
{
	if (NRF_RNG->EVENTS_VALRDY) {
		uint8_t last;

		last = _rand->last + 1;
		if (last == _rand->count) {
			last = 0;
		}

		if (last == _rand->first) {
			/* this condition should not happen
			 * , but due to probable bug in HW
			 * , new value could be generated
			 * before task is stopped.
			 */
			NRF_RNG->TASKS_STOP = 1;
			NRF_RNG->EVENTS_VALRDY = 0;

			return;
		}

		_rand->rand[_rand->last] = NRF_RNG->VALUE;
		_rand->last = last;

		last = _rand->last + 1;
		if (last == _rand->count) {
			last = 0;
		}

		NRF_RNG->EVENTS_VALRDY = 0;

		if (last == _rand->first) {
			NRF_RNG->TASKS_STOP = 1;
		}
	}
}
