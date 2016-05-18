/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "qm_pic_timer.h"

/*
 * PIC timer access layer. Supports both Local APIC timer and MVIC timer.
 *
 * The MVIC timer differs from the LAPIC specs in that:
 * - it does not support TSC deadline mode
 * - vector table lvttimer[3:0] holds the IRQ (not the vector) of the timer
 */

#define LVTTIMER_MODE_PERIODIC_OFFS (17)
#define LVTTIMER_INT_MASK_OFFS (16)

static void (*callback)(void *data);
static void *callback_data;

#if (HAS_APIC)
#define PIC_TIMER (QM_LAPIC)
#else
#define PIC_TIMER (QM_PIC_TIMER)
#endif

QM_ISR_DECLARE(qm_pic_timer_isr)
{
	if (callback) {
		callback(callback_data);
	}

#if (HAS_APIC)
	/* Use an invalid vector to avoid acknowledging a valid IRQ */
	QM_ISR_EOI(0);
#else
	QM_ISR_EOI(QM_IRQ_PIC_TIMER_VECTOR);
#endif
}

int qm_pic_timer_set_config(const qm_pic_timer_config_t *const cfg)
{
	QM_CHECK(cfg != NULL, -EINVAL);
	QM_CHECK(cfg->mode <= QM_PIC_TIMER_MODE_PERIODIC, -EINVAL);

	/* Stop timer, mask interrupt and program interrupt vector */
	PIC_TIMER->timer_icr.reg = 0;
	PIC_TIMER->lvttimer.reg = BIT(LVTTIMER_INT_MASK_OFFS) |
#if (HAS_APIC)
				  QM_INT_VECTOR_PIC_TIMER;
#else
				  QM_IRQ_PIC_TIMER;
#endif

#if (HAS_APIC)
	/* LAPIC has a timer clock divisor, POR default: 2. Set it to 1. */
	QM_LAPIC->timer_dcr.reg = 0xB;
#endif

	PIC_TIMER->lvttimer.reg |= cfg->mode << LVTTIMER_MODE_PERIODIC_OFFS;
	callback = cfg->callback;
	callback_data = cfg->callback_data;
	if (cfg->int_en) {
		PIC_TIMER->lvttimer.reg &= ~BIT(LVTTIMER_INT_MASK_OFFS);
	}
	return 0;
}

int qm_pic_timer_set(const uint32_t count)
{
	PIC_TIMER->timer_icr.reg = count;

	return 0;
}

int qm_pic_timer_get(uint32_t *const count)
{
	QM_CHECK(count != NULL, -EINVAL);

	*count = PIC_TIMER->timer_ccr.reg;

	return 0;
}
