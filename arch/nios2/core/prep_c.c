/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call _Cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <stdint.h>
#include <toolchain.h>
#include <linker-defs.h>
#include <kernel_structs.h>
#include <nano_internal.h>

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

void _PrepC(void)
{
	_bss_zero();
#ifdef CONFIG_XIP
	_data_copy();
	/* In most XIP scenarios we copy the exception code into RAM, so need
	 * to flush instruction cache.
	 */
	_nios2_icache_flush_all();
#if ALT_CPU_ICACHE_SIZE > 0
	/* Only need to flush the data cache here if there actually is an
	 * instruction cache, so that the cached instruction data written is
	 * actually committed.
	 */
	_nios2_dcache_flush_all();
#endif
#endif
	_Cstart();
	CODE_UNREACHABLE;
}
