/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Non-random number generator based on x86 CPU timestamp
 *
 * This module provides a non-random implementation of sys_rand32_get(), which
 * is not meant to be used in a final product as a truly random number
 * generator. It was provided to allow testing on a platform that does not (yet)
 * provide a random number generator.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <drivers/rand32.h>

/**
 *
 * @brief Initialize the random number generator
 *
 * The non-random number generator does not require any initialization.
 * Routine is automatically invoked by the kernel during system startup.
 *
 * @return N/A
 */

void sys_rand32_init(void)
{
}

/**
 *
 * @brief Get a 32 bit random number
 *
 * The non-random number generator returns values that are based off the
 * CPU's timestamp counter, which means that successive calls will normally
 * display ever-increasing values.
 *
 * @return a 32-bit number
 */

uint32_t sys_rand32_get(void)
{
	return _do_read_cpu_timestamp32();
}
