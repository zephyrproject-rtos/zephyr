/*
 * Copyright (c) 2015 Intel Corporation
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
 * @brief Quark D2000 Interrupt Controller (MVIC) public APIs
 *
 * MVIC interrupt controller driver exports the same APIs as the standard
 * local apic & IO Apic drivers with the exception that it replaces the init
 * routines with a single _mvic_init routine.
 */
#ifndef __INC_MVIC
#define __INC_MVIC

#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <init.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _ASMLANGUAGE
extern int _mvic_init(struct device *unused);
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INC_MVIC */
