/* random.c - Random number API */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <nanokernel.h>
#include <drivers/system_timer.h>

#include "lib/random.h"

void random_init(unsigned short seed)
{

}

unsigned short random_rand(void)
{
	/* FIXME: At the moment there is no random number generator
	 *        implemented so fix this func when random numbers
	 *        are available.
	 */
	return (unsigned short)sys_cycle_get_32();
}
