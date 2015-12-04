/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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
 * @brief ARM Cortex-M systick handler stub for GDB debugging
 *
 * GDB stub needed before the real systick handler runs to be able to display the
 * correct state of the thread that was interrupted.
 */

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_timer_int_handler)
GTEXT(_real_timer_int_handler)
SECTION_FUNC(TEXT, _timer_int_handler)

    _GDB_STUB_EXC_ENTRY

    b _real_timer_int_handler
