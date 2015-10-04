/* taskDevInt.h - APIs for managing task IRQ objects */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
 *
 * @return assigned interrupt vector if successful, INVALID_VECTOR if not
 */
extern uint32_t task_irq_alloc(kirq_t irq_obj, uint32_t irq, uint32_t priority);
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
