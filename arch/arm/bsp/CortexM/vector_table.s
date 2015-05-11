/* vector_table.s - populated vector table in ROM */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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
Vector table in ROM for starting system. The reset vector is the system entry
point, ie. the first instruction executed.

The table is populated with all the system exception handlers. The NMI vector
must be populated with a valid handler since it can happen at any time. The
rest should not be triggered until the kernel is ready to handle them.
*/

#define _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>
#include <drivers/system_timer.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

/* Diab requires a __start symbol */
SECTION_SUBSEC_FUNC(exc_vector_table,_Start,__start)
SECTION_SUBSEC_FUNC(exc_vector_table,_Start,_VectorTableROM)

    .word __CORTEXM_BOOT_MSP
    .word __reset
    .word __nmi

    .word __hard_fault
    .word __mpu_fault
    .word __bus_fault
    .word __usage_fault
    .word __reserved
    .word __reserved
    .word __reserved
    .word __reserved
    .word __svc
    .word __debug_monitor
    .word __reserved
    .word __pendsv
    .word _timer_int_handler
