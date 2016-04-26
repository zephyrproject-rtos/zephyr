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
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call _Cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <nanokernel.h>
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

static inline void relocate_vector_table(void) { /* do nothing */ }
#else
static void dataCopy(void)
{
}

static inline void relocate_vector_table(void)
{
	/* vector table is already in SRAM, just point to it */
	_scs_relocate_vector_table((void *)CONFIG_SRAM_BASE_ADDRESS);
}
#endif

#ifdef CONFIG_FLOAT
static inline void enable_floating_point(void)
{
	/*
	 * Upon reset, the Co-Processor Access Control Register is 0x00000000.
	 * Enable CP10 and CP11 coprocessors to enable floating point.
	 */
	__scs.cpacr.val = (_SCS_CPACR_CP10_FULL_ACCESS |
				_SCS_CPACR_CP11_FULL_ACCESS);

	/*
	 * Upon reset, the FPU Context Control Register is 0xC0000000
	 * (both Automatic and Lazy state preservation is enabled).
	 * Disable lazy state preservation so the volatile FP registers are
	 * always saved on exception.
	 */
	__scs.fpu.ccr.val = (_SCS_FPU_CCR_ASPEN_ENABLE |
				_SCS_FPU_CCR_LSPEN_DISABLE);

	/*
	 * Although automatic state preservation is enabled, the processor
	 * does not automatically save the volatile FP registers until they
	 * have first been touched. Perform a dummy move operation so that
	 * the stack frames are created as expected before any task or fiber
	 * context switching can occur.
	 */
	__asm__ volatile(
		"vmov s0, s0;\n\t"
		"dsb;\n\t"
		"isb;\n\t"
		);
}
#else
static inline void enable_floating_point(void)
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
	relocate_vector_table();
	enable_floating_point();
	bssZero();
	dataCopy();
	_Cstart();
	CODE_UNREACHABLE;
}
