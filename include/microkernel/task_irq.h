/* taskDevInt.h - APIs for managing task IRQ objects */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

#ifndef TASK_IRQ_H
#define TASK_IRQ_H


/**
 * @brief Microkernel Interrupt Services
 * @defgroup microkernel_irq Microkernel Interrupt Services
 * @ingroup microkernel_services
 * @{
 */

#include <microkernel/base_api.h>

#define INVALID_VECTOR 0xFFFFFFFF

/**
 *
 * @brief Register a task IRQ object
 *
 * This routine connects a task IRQ object to a system interrupt based
 * upon the specified IRQ and priority values.
 *
 * IRQ allocation is done via the microkernel server fiber so that simultaneous
 * allocation requests are single-threaded.
 *
 * @param irq_obj IRQ object identifier
 * @param irq Request IRQ
 * @param priority Requested interrupt priority
 * @param flags IRQ flags
 *
 * @return assigned interrupt vector if successful, INVALID_VECTOR if not
 */
extern uint32_t task_irq_alloc(kirq_t irq_obj, uint32_t irq, uint32_t priority,
			       uint32_t flags);

/**
 *
 * @brief Re-enable a task IRQ object's interrupt
 *
 * This re-enables the interrupt for a task IRQ object.
 * @param irq_obj IRQ object identifier
 * @return N/A
 */
extern void task_irq_ack(kirq_t irq_obj);

/**
 *
 * @brief Free a task IRQ object
 *
 * The task IRQ object's interrupt is disabled, and the associated event
 * is flushed; the object's interrupt vector is then freed, and the object's
 * global array entry is marked as unused.
 * @param irq_obj IRQ object identifier
 * @return N/A
 */
extern void task_irq_free(kirq_t irq_obj);

/**
 * @brief Wait for task IRQ to signal an interrupt
 *
 * This routine waits up to @a timeout ticks for the IRQ object @a irq_obj
 * to signal an interrupt.
 *
 * @param irq_obj IRQ object identifier
 * @param timeout Affects the action taken should the task IRQ not yet be
 * signaled. If TICKS_NONE, the return immediately. If TICKS_UNLIMITED, then
 * wait as long as necessary. Otherwise wait up to the specified number of
 * ticks before timing out.
 *
 * @retval RC_OK Successfully detected signaled interrupt
 * @retval RC_TIME Timed out while waiting for interrupt
 * @retval RC_FAIL Failed to immediately detect signaled interrupt when
 * @a timeout = TICKS_NONE
 */
extern int task_irq_wait(kirq_t irq_obj, int32_t timeout);

/**
 * @}
 */
#endif /* TASK_IRQ_H */
