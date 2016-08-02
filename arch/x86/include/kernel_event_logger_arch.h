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
 * @file
 * @brief Kernel event logger support for x86
 */

#ifndef __KERNEL_EVENT_LOGGER_ARCH_H__
#define __KERNEL_EVENT_LOGGER_ARCH_H__

#include <arch/x86/irq_controller.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the identification of the current interrupt.
 *
 * This routine obtain the key of the interrupt that is currently processed
 * if it is called from a ISR context.
 *
 * @return The key of the interrupt that is currently being processed.
 */
static inline int _sys_current_irq_key_get(void)
{
	return _irq_controller_isr_vector_get();
}

#ifdef __cplusplus
}
#endif

#endif /* __KERNEL_EVENT_LOGGER_ARCH_H__ */
