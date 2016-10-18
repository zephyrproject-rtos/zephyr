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

#include "qm_pwm.h"

static void (*callback[QM_PWM_NUM])(void *data, uint32_t int_status);

static void *callback_data[QM_PWM_NUM];

QM_ISR_DECLARE(qm_pwm_0_isr)
{
	/*  Which timers fired. */
	uint32_t int_status = QM_PWM[QM_PWM_0].timersintstatus;
	/* Clear timers interrupt flag. */
	QM_PWM[QM_PWM_0].timerseoi;

	if (callback[QM_PWM_0]) {
		(*callback[QM_PWM_0])(callback_data[QM_PWM_0], int_status);
	}
	QM_ISR_EOI(QM_IRQ_PWM_0_INT_VECTOR);
}

int qm_pwm_start(const qm_pwm_t pwm, const qm_pwm_id_t id)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);

	QM_PWM[pwm].timer[id].controlreg |= PWM_START;

	return 0;
}

int qm_pwm_stop(const qm_pwm_t pwm, const qm_pwm_id_t id)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);

	QM_PWM[pwm].timer[id].controlreg &= ~PWM_START;

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

	QM_PWM[pwm].timer[id].loadcount = cfg->lo_count - 1;
	QM_PWM[pwm].timer[id].controlreg =
	    (cfg->mode | (cfg->mask_interrupt << QM_PWM_INTERRUPT_MASK_OFFSET));
	QM_PWM[pwm].timer_loadcount2[id] = cfg->hi_count - 1;

	/* Assign user callback function. */
	callback[pwm] = cfg->callback;
	callback_data[pwm] = cfg->callback_data;

	return 0;
}

int qm_pwm_set(const qm_pwm_t pwm, const qm_pwm_id_t id,
	       const uint32_t lo_count, const uint32_t hi_count)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);
	QM_CHECK(0 < lo_count, -EINVAL);
	/* If mode is PWM, hi_count must be > 0, otherwise don't care. */
	QM_CHECK(((QM_PWM[pwm].timer[id].controlreg & QM_PWM_CONF_MODE_MASK) ==
			  QM_PWM_MODE_PWM
		      ? 0 < hi_count
		      : 1),
		 -EINVAL);

	QM_PWM[pwm].timer[id].loadcount = lo_count - 1;
	QM_PWM[pwm].timer_loadcount2[id] = hi_count - 1;

	return 0;
}

int qm_pwm_get(const qm_pwm_t pwm, const qm_pwm_id_t id,
	       uint32_t *const lo_count, uint32_t *const hi_count)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(id < QM_PWM_ID_NUM, -EINVAL);
	QM_CHECK(lo_count != NULL, -EINVAL);
	QM_CHECK(hi_count != NULL, -EINVAL);

	*lo_count = QM_PWM[pwm].timer[id].loadcount;
	*hi_count = QM_PWM[pwm].timer_loadcount2[id];

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_pwm_save_context(const qm_pwm_t pwm, qm_pwm_context_t *const ctx)
{
	QM_CHECK(pwm < QM_PWM_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	qm_pwm_reg_t *const controller = &QM_PWM[pwm];
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

	qm_pwm_reg_t *const controller = &QM_PWM[pwm];
	uint8_t i;

	for (i = 0; i < QM_PWM_ID_NUM; i++) {
		controller->timer[i].loadcount = ctx->channel[i].loadcount;
		controller->timer[i].controlreg = ctx->channel[i].controlreg;
		controller->timer_loadcount2[i] = ctx->channel[i].loadcount2;
	}

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
