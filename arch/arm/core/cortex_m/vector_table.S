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
 * @brief Populated vector table in ROM
 *
 * Vector table at the beginning of the image for starting system. The reset
 * vector is the system entry point, ie. the first instruction executed.
 *
 * The table is populated with all the system exception handlers. The NMI vector
 * must be populated with a valid handler since it can happen at any time. The
 * rest should not be triggered until the kernel is ready to handle them.
 */

#define _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>
#include <drivers/system_timer.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)

/* in XIP kernels. the entry point is also the start of the vector table */
#if defined(CONFIG_XIP)
SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,__start)
#endif

    .word __CORTEXM_BOOT_MSP
    .word __reset
    .word __nmi

    .word __hard_fault
#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    .word __reserved
    .word __reserved
    .word __reserved
    .word __reserved
    .word __reserved
    .word __reserved
    .word __reserved
    .word __reserved	/* SVC not used for now (PendSV used instead) */
    .word __reserved
#else /* CONFIG_CPU_CORTEX_M3_M4 */
    .word __mpu_fault
    .word __bus_fault
    .word __usage_fault
    .word __reserved
    .word __reserved
    .word __reserved
    .word __reserved
    .word __svc
    .word __debug_monitor
#endif
    .word __reserved
    .word __pendsv
    .word _timer_int_handler
