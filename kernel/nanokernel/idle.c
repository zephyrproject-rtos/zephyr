/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
 * @brief Nanokernel idle support
 *
 * This module provides routines to set the idle field in the nanokernel
 * data structure.
 */

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT

#include <nanokernel.h>
#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>

/**
 *
 * @brief Indicate that nanokernel is idling in tickless mode
 *
 * Sets the nanokernel data structure idle field to a non-zero value.
 *
 * @param ticks the number of ticks to idle
 *
 * @return N/A
 */
void nano_cpu_set_idle(int32_t ticks)
{
	extern tNANO _nanokernel;

	_nanokernel.idle = ticks;
}

#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */
