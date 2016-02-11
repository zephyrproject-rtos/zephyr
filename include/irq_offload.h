/*
 * Copyright (c) 2015 Intel corporation
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
 * @brief IRQ Offload interface
 */
#ifndef _IRQ_OFFLOAD_H_
#define _IRQ_OFFLOAD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*irq_offload_routine_t)(void *parameter);

/**
 * @brief Run a function in interrupt context
 *
 * This function synchronously runs the provided function in interrupt
 * context, passing in the supplied parameter. Useful for test code
 * which needs to show that kernel objects work correctly in interrupt
 * context.
 *
 * @param routine The function to run
 * @param parameter Argument to pass to the function when it is run as an
 * interrupt
 */
void irq_offload(irq_offload_routine_t routine, void *parameter);

#ifdef __cplusplus
}
#endif

#endif /* _SW_IRQ_H_ */
