/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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
 * @brief Stack frames for debugging purposes
 *
 * This file contains a routine useful for debugging that gets a pointer to
 * the current interrupt stack frame.
 */

#include <nanokernel.h>
#include <kernel_structs.h>

NANO_ISF *sys_debug_current_isf_get(void)
{
	return _kernel.isf;
}
