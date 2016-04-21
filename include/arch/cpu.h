/* cpu.h - automatically selects the correct arch.h file to include */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

#ifndef __ARCHCPU_H__
#define __ARCHCPU_H__

#if defined(CONFIG_X86)
#include <arch/x86/arch.h>
#elif defined(CONFIG_ARM)
#include <arch/arm/arch.h>
#elif defined(CONFIG_ARC)
#include <arch/arc/arch.h>
#elif defined(CONFIG_NIOS2)
#include <arch/nios2/arch.h>
#else
#error "Unknown Architecture"
#endif

#endif /* __ARCHCPU_H__ */
