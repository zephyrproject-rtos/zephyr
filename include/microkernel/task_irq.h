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
 * @cond internal
 */
extern int _task_irq_test(kirq_t irq_ob, int32_t time);

/**
 * @endcond
 */

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
 * @brief Determine if a task IRQ object has had an interrupt
 *
 * This tests a task IRQ object to see if it has signaled an interrupt.
 * It fails immediately if no interrupt has occurred.
 *
 * @param irq_obj IRQ object identifier
 *
 * @return RC_OK, RC_FAIL
 */
#define task_irq_test(irq_obj) _task_irq_test(irq_obj, TICKS_NONE)

/**
 * @brief Determine if a task IRQ object has had an interrupt and wait
 *
 * This tests a task IRQ object to see if it has signaled an interrupt.
 * It waits forever for a interrupt to happen.
 *
 * @param irq_obj IRQ object identifier
 *
 * @return RC_OK
 */
#define task_irq_test_wait(irq_obj) _task_irq_test(irq_obj, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS
/**
 * @brief Determine if a task IRQ object has had an interrupt and timeout
 *
 * This tests a task IRQ object to see if it has signaled an interrupt
 * with a timeout option.
 *
 * @param irq_obj IRQ object identifier
 * @param time  Time to wait (in ticks)
 *
 * @return RC_OK, RC_FAIL, or RC_TIME
 */
#define task_irq_test_wait_timeout(irq_obj, time) _task_irq_test(irq_obj, time)
#endif

/**
 * @}
 */
#endif /* TASK_IRQ_H */
