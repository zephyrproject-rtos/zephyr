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

#include <microkernel/k_types.h>

#define INVALID_VECTOR 0xFFFFFFFF

struct task_irq_info {
	ktask_t taskId;  /* task ID of task IRQ object's owner */
	uint32_t irq;    /* IRQ used by task IRQ object */
	kevent_t event;  /* event number assigned to task IRQ object */
	uint32_t vector; /* interrupt vector assigned to task IRQ object */
};

extern struct task_irq_info task_irq_object[];

extern uint32_t task_irq_alloc(kirq_t irq_obj,
				      uint32_t irq,
				      uint32_t priority);
extern int _task_irq_test(kirq_t irq_ob, int32_t time);
extern void task_irq_ack(kirq_t irq_obj);
extern void task_irq_free(kirq_t irq_obj);

#define task_irq_test(irq_obj) _task_irq_test(irq_obj, TICKS_NONE)
#define task_irq_test_wait(irq_obj) _task_irq_test(irq_obj, TICKS_UNLIMITED)

#ifndef CONFIG_TICKLESS_KERNEL
#define task_irq_test_wait_timeout(irq_obj, time) _task_irq_test(irq_obj, time)
#endif

#endif /* TASK_IRQ_H */
