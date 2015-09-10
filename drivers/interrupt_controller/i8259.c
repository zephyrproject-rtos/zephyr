/* i8259.c - Disable Intel 8259A PIC (Programmable Interrupt Controller) */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
This module disables the Intel 8259A PIC (Programmable Interrupt Controller)
to prevent it from generating spurious interrupts.
*/


#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>

#include <drivers/pic.h>
#include <board.h>

#define PIC_DISABLE 0xff /* Disable PIC command */

/**
 *
 * @brief Initialize the Intel 8259A PIC device driver
 *
 * This routine disables the 8259A PIC device to prevent it from
 * generating spurious interrupts.
 *
 * @return N/A
 */

void _i8259_init(void)
{
	sys_out8(PIC_DISABLE, PIC_PORT2(PIC_SLAVE_BASE_ADRS));
	sys_out8(PIC_DISABLE, PIC_PORT2(PIC_MASTER_BASE_ADRS));
}
