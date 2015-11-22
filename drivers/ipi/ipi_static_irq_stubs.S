/*
 * Copyright (c) 2015, Intel Corporation
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

#define _ASMLANGUAGE

#ifdef CONFIG_X86_32
#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#endif

#if defined(CONFIG_IPI_QUARK_SE)
#if defined(CONFIG_IOAPIC)
        ioapic_mkstub quark_se_ipi quark_se_ipi_isr 0
#endif
#endif
