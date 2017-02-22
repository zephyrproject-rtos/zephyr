/*
 * Copyright (c) 2017, Intel Corporation
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

#include "qm_pwm.h"

/* Store callback information for each PWM channel. */
static void (*callback[QM_PWM_NUM][QM_PWM_ID_NUM])(void *data,
						   uint32_t int_status);
static void *callback_data[QM_PWM_NUM][QM_PWM_ID_NUM];

#if (NUM_PWM_CONTROLLER_INTERRUPTS > 1)
const uint32_t pwm_isr_vectors[QM_PWM_ID_NUM] = {
    QM_IRQ_PWM_0_INT_0_PRIORITY, QM_IRQ_PWM_0_INT_1_PRIORITY,
    QM_IRQ_PWM_0_INT_2_PRIORITY, QM_IRQ_PWM_0_INT_3_PRIORITY};
#endif /* NUM_PWM_CONTROLLER_INTERRUPTS > 1 */

#ifndef UNIT_TEST
qm_pwm_reg_t *qm_pwm[QM_PWM_NUM] = {(qm_pwm_reg_t *)(QM_PWM_BASE)};
#endif /* UNIT_TEST */

#if (NUM_PWM_CONTROLLER_INTERRUPTS > 1)
/*
 * If there is more than one interrupt line for PWM, use a common handler
 * and only clear the corresponding interrupt IRQ.
 */
/* Interrupt service routine handler for PWM channels. */
static void pwm_isr_handler(const qm_pwm_t pwm, const qm_pwm_id_t id)
{
	qm_pwm_reg_t *const controller = QM_PWM[pwm];
	/* Check for callback function. */
	if (callback[pwm][id]) {
		(callback[pwm][id])(callback_data[pwm][id], BIT(id));
	}

	/* Clear interrupt on read. */
	controller->timer[id].eoi;
	QM_ISR_EOI(pwm_isr_vectors[id]);
}

QM_ISR_DECLARE(qm_pwm_0_isr_0)
{
	pwm_isr_handler(QM_PWM_0, QM_PWM_ID_0);
}

QM_ISR_DECLARE(qm_pwm_0_isr_1)
{
	pwm_isr_handler(QM_PWM_0, QM_PWM_ID_1);
}

QM_ISR_DECLARE(qm_pwm_0_isr_2)
{
	pwm_isr_handler(QM_PWM_0, QM_PWM_ID_2);
}

QM_ISR_DECLARE(qm_pwm_0_isr_3)
{
	pwm_isr_handler(QM_PWM_0, QM_PWM_ID_3);
}

#else /* NUM_PWM_CONTROLLER_INTERRUPTS > 1 */
QM_ISR_DECLARE(qm_pwm_0_isr_0)
{
	qm_pwm_reg_t *const controller = QM_PWM[QM_PWM_0];
	uint32_t int_status = controller->timersintstatus;
	uint8_t pwm_id = 0;

	for (; pwm_id < QM_PWM_ID_NUM; pwm_id++) {
		if (int_status & BIT(pwm_id)) {
			if (callback[QM_PWM_0][pwm_id]) {
				(*callback[QM_PWM_0][pwm_id])(
				    callback_data[QM_PWM_0][pwm_id],
				    BIT(pwm_id));
				controller->timer[pwm_id].eoi;
			}
		}
	}
	QM_ISR_EOI(QM_IRQ_PWM_0_INT_VECTOR);
}

#endif /* NUM_PWM_CONTROLLER_INTERRUPTS > 1 */

int qm_pwm_start(const qm_pwm_t pwm, const qm_pwm_id_t id)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);

	qm_pwm_reg_t *const controller = QM_PWM[pwm];
	controller->timer[id].controlreg |= PWM_START;

	return 0;
}

int qm_pwm_stop(const qm_pwm_t pwm, const qm_pwm_id_t id)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);

	qm_pwm_reg_t *const controller = QM_PWM[pwm];

	controller->timer[id].controlreg &= ~PWM_START;

	return 0;
}

int qm_pwm_set_config(const qm_pwm_t pwm, const qm_pwm_id_t id,
		      const qm_pwm_config_t *const cfg)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);
	QM_CHECK(cfg->mode <= QM_PWM_MODE_PWM, -EINVAL);
	QM_CHECK(0 < cfg->lo_count, -EINVAL);
	/* If mode is PWM, hi_count must be > 0, otherwise don't care. */
	QM_CHECK(cfg->mode == QM_PWM_MODE_PWM ? 0 != cfg->hi_count : 1,
		 -EINVAL);

	qm_pwm_reg_t *const controller = QM_PWM[pwm];
	controller->timer[id].loadcount = cfg->lo_count - 1;
	controller->timer[id].controlreg =
	    (cfg->mode | (cfg->mask_interrupt << QM_PWM_INTERRUPT_MASK_OFFSET));
	controller->timer_loadcount2[id] = cfg->hi_count - 1;

	/* Assign user callback function. */
	callback[pwm][id] = cfg->callback;
	callback_data[pwm][id] = cfg->callback_data;

	return 0;
}

int qm_pwm_set(const qm_pwm_t pwm, const qm_pwm_id_t id,
	       const uint32_t lo_count, const uint32_t hi_count)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);
	QM_CHECK(0 < lo_count, -EINVAL);
	/* If mode is PWM, hi_count must be > 0, otherwise don't care. */
	QM_CHECK(((QM_PWM[pwm]->timer[id].controlreg & QM_PWM_CONF_MODE_MASK) ==
			  QM_PWM_MODE_PWM
		      ? 0 < hi_count
		      : 1),
		 -EINVAL);

	qm_pwm_reg_t *const controller = QM_PWM[pwm];
	controller->timer[id].loadcount = lo_count - 1;
	controller->timer_loadcount2[id] = hi_count - 1;

	return 0;
}

int qm_pwm_get(const qm_pwm_t pwm, const qm_pwm_id_t id,
	       uint32_t *const lo_count, uint32_t *const hi_count)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);
	QM_CHECK(lo_count != NULL, -EINVAL);
	QM_CHECK(hi_count != NULL, -EINVAL);

	qm_pwm_reg_t *const controller = QM_PWM[pwm];
	*lo_count = controller->timer[id].loadcount;
	*hi_count = controller->timer_loadcount2[id];

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_pwm_save_context(const qm_pwm_t pwm, qm_pwm_context_t *const ctx)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_pwm_reg_t *const controller = QM_PWM[pwm];
	uint8_t i;

	for (i = 0; i < QM_PWM_ID_NUM; i++) {
		ctx->channel[i].loadcount = controller->timer[i].loadcount;
		ctx->channel[i].controlreg = controller->timer[i].controlreg;
		ctx->channel[i].loadcount2 = controller->timer_loadcount2[i];
	}

	return 0;
}

int qm_pwm_restore_context(const qm_pwm_t pwm,
			   const qm_pwm_context_t *const ctx)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_pwm_reg_t *const controller = QM_PWM[pwm];
	uint8_t i;

	for (i = 0; i < QM_PWM_ID_NUM; i++) {
		controller->timer[i].loadcount = ctx->channel[i].loadcount;
		controller->timer[i].controlreg = ctx->channel[i].controlreg;
		controller->timer_loadcount2[i] = ctx->channel[i].loadcount2;
	}

	return 0;
}
#else
int qm_pwm_save_context(const qm_pwm_t pwm, qm_pwm_context_t *const ctx)
{
	(void)pwm;
	(void)ctx;

	return 0;
}

int qm_pwm_restore_context(const qm_pwm_t pwm,
			   const qm_pwm_context_t *const ctx)
{
	(void)pwm;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
