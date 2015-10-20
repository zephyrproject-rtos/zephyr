/* prep_c.c - full C support initialization */

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

/*
 * DESCRIPTION
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

/**
 *
 * @brief Clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 *
 * @return N/A
 */

static void bssZero(void)
{
	volatile uint32_t *pBSS = (uint32_t *)&__bss_start;
	unsigned int n;

	for (n = 0; n < (unsigned int)&__bss_num_words; n++) {
		pBSS[n] = 0;
	}
}

/**
 *
 * @brief Copy the data section from ROM to RAM
 *
 * This routine copies the data section from ROM to RAM.
 *
 * @return N/A
 */

#ifdef CONFIG_XIP
static void dataCopy(void)
{
	volatile uint32_t *pROM = (uint32_t *)&__data_rom_start;
	volatile uint32_t *pRAM = (uint32_t *)&__data_ram_start;
	unsigned int n;

	for (n = 0; n < (unsigned int)&__data_num_words; n++) {
		pRAM[n] = pROM[n];
	}
}
#else
static void dataCopy(void)
{
}
#endif

extern FUNC_NORETURN void _Cstart(void);
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
	bssZero();
	dataCopy();
	_Cstart();
	CODE_UNREACHABLE;
}
