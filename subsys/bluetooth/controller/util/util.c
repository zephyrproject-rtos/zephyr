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

#include <stdint.h>
#include "util.h"

uint8_t util_ones_count_get(uint8_t *octets, uint8_t octets_len)
{
	uint8_t one_count = 0;

	while (octets_len--) {
		uint8_t bite;

		bite = *octets;
		while (bite) {
			bite &= (bite - 1);
			one_count++;
		}
		octets++;
	}

	return one_count;
}
