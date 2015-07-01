/* prep_c.c - full C support initialization */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION

Initialization of full C support: zero the .bss, copy the .data if XIP,
call _Cstart().

Stack is available in this module, but not the global data/bss until their
initialization is performed.
 */

#include <stdint.h>
#include <toolchain.h>
#include <linker-defs.h>

/**
 *
 * bssZero - clear BSS
 *
 * This routine clears the BSS region, so all bytes are 0.
 *
 * RETURNS: N/A
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
 * dataCopy - copy the data section from ROM to RAM
 *
 * This routine copies the data section from ROM to RAM.
 *
 * RETURNS: N/A
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
 * _PrepC - prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * RETURNS: N/A
 */

void _PrepC(void)
{
	bssZero();
	dataCopy();
	_Cstart();
	CODE_UNREACHABLE;
}
