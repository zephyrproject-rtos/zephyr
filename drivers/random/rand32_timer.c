/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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
 * @brief Non-random number generator based on system timer
 *
 * This module provides a non-random implementation of sys_rand32_get(), which
 * is not meant to be used in a final product as a truly random number
 * generator. It was provided to allow testing on a platform that does not (yet)
 * provide a random number generator.
 */

#include <drivers/rand32.h>
#include <drivers/system_timer.h>
#include <nanokernel.h>
#include <atomic.h>

#if defined(__GNUC__)

/*
 * Symbols used to ensure a rapid series of calls to random number generator
 * return different values.
 */
static atomic_val_t _rand32_counter;

#define _RAND32_INC 1000000013

/**
 *
 * @brief Initialize the random number generator
 *
 * The non-random number generator does not require any initialization.
 * This routine is automatically invoked by the kernel during system
 * initialization.
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
 * target's clock counter, which means that successive calls will return
 * different values.
 *
 * @return a 32-bit number
 */

uint32_t sys_rand32_get(void)
{
	return sys_cycle_get_32() + atomic_add(&_rand32_counter, _RAND32_INC);
}

#endif /* __GNUC__ */
