/* linker.cmd - Linker command/script file */

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
This is the linker script for both standard images and XIP images.
*/

/* Flash base address and size */
#define FLASH_START 0x00000000
#define FLASH_SIZE  1M

/*
 * K64F Flash configuration fields
 * These are 16 bytes, which must be loaded to address 0x400, and include 
 * default protection and security settings.
 * They are loaded at reset to various Flash Memory module (FTFE) registers.
 */
#define SKIP_TO_SECURITY_FRDM_K64F . = 0x400;

/*
 * SRAM base address and size
 *
 * Although the K64F CPU has a 64 KB region of SRAM at 0x1FFF0000, it is not
 * used by this BSP.  Only the 192 KB region based at the standard ARMv7-M
 * SRAM base address of 0x20000000 is supported.
 */
#define SRAM_START  0x20000000  /* 192K in SRAM space */
#define SRAM_SIZE   192K

#include <arch/arm/CortexM/scripts/linker.cmd>
