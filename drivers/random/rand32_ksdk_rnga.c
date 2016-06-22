/*
 * Copyright (c) 2016 ARM Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <drivers/rand32.h>
#include <drivers/system_timer.h>
#include <misc/sys_log.h>

#include "fsl_rnga.h"


void sys_rand32_init(void)
{
	uint32_t seed = sys_cycle_get_32();

	RNGA_Init(RNG);

	/* The range of seed values acquired by this method is likely
	 * to be relatively small.  The RNGA hardware uses two free
	 * running oscillators to add entropy to the seed value, we
	 * take care below to ensure the read rate is lower than the
	 * rate at which the hardware can add entropy.
	 */
	RNGA_Seed(RNG, seed);
	RNGA_SetMode(RNG, kRNGA_ModeSleep);
}

uint32_t sys_rand32_get(void)
{
	uint32_t random;
	uint32_t output = 0;
	int i;

	RNGA_SetMode(RNG, kRNGA_ModeNormal);
	/* The Reference manual states that back to back reads from
	 * the RNGA deliver one or two bits of entropy per 32-bit
	 * word, therefore we deliberately only use 1 bit per 32 bit
	 * word read.
	 */
	for (i = 0; i < 32; i++) {
		status_t status;

		status = RNGA_GetRandomData(RNG, &random, sizeof(random));
		__ASSERT(!status, "RNGA_GetRandomData failed");
		if (status) {
			SYS_LOG_ERR("RNGA_GetRandomData failed with %d",
				    status);
		}
		output <<= 1;
		output |= random & 1;
	}
	RNGA_SetMode(RNG, kRNGA_ModeSleep);

	return output;
}
