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

#include "qm_mpr.h"
#include "qm_interrupt.h"
#include "qm_interrupt_router.h"

#define ADDRESS_MASK_7_BIT (0x7F)

static void (*callback)(void *data);
static void *callback_data;

QM_ISR_DECLARE(qm_sram_mpr_0_isr)
{
	if (callback) {
		(*callback)(callback_data);
	}
	QM_MPR->mpr_vsts = QM_MPR_VSTS_VALID;

	QM_ISR_EOI(QM_IRQ_SRAM_MPR_0_INT_VECTOR);
}

int qm_mpr_set_config(const qm_mpr_id_t id, const qm_mpr_config_t *const cfg)
{
	QM_CHECK(id < QM_MPR_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	QM_MPR->mpr_cfg[id] &= ~QM_MPR_EN_LOCK_MASK;

	QM_MPR->mpr_cfg[id] =
	    (cfg->agent_write_en_mask << QM_MPR_WR_EN_OFFSET) |
	    (cfg->agent_read_en_mask << QM_MPR_RD_EN_OFFSET) |
	    /*   MPR Upper bound 16:10 */
	    ((cfg->up_bound & ADDRESS_MASK_7_BIT) << QM_MPR_UP_BOUND_OFFSET)
	    /*   MPR Lower bound 6:0 */
	    |
	    cfg->low_bound;

	/* enable/lock */
	QM_MPR->mpr_cfg[id] |= (cfg->en_lock_mask << QM_MPR_EN_LOCK_OFFSET);
	return 0;
}
#if (QM_SENSOR)
int qm_mpr_set_violation_policy(const qm_mpr_viol_mode_t mode,
				qm_mpr_callback_t callback_fn, void *cb_data)
{
	QM_CHECK(mode <= MPR_VIOL_MODE_PROBE, -EINVAL);
	/*  interrupt mode */
	if (MPR_VIOL_MODE_INTERRUPT == mode) {
		callback = callback_fn;
		callback_data = cb_data;

		/* unmask interrupt */
		QM_IR_UNMASK_INTERRUPTS(
		    QM_INTERRUPT_ROUTER->sram_mpr_0_int_mask);

		QM_IR_MASK_HALTS(QM_INTERRUPT_ROUTER->sram_mpr_0_int_mask);

		QM_SCSS_SS->ss_cfg &= ~QM_SS_STS_HALT_INTERRUPT_REDIRECTION;
	}

	/* probe or reset mode */
	else {
		/* mask interrupt */
		QM_IR_MASK_INTERRUPTS(QM_INTERRUPT_ROUTER->sram_mpr_0_int_mask);

		QM_IR_UNMASK_HALTS(QM_INTERRUPT_ROUTER->sram_mpr_0_int_mask);

		if (MPR_VIOL_MODE_PROBE == mode) {

			/* When an enabled host halt interrupt occurs, this bit
			 * determines if the interrupt event triggers a warm
			 * reset
			 * or an entry into Probe Mode.
			 * 0b : Warm Reset
			 * 1b : Probe Mode Entry
			 */
			QM_SCSS_SS->ss_cfg |=
			    QM_SS_STS_HALT_INTERRUPT_REDIRECTION;
		} else {
			QM_SCSS_SS->ss_cfg &=
			    ~QM_SS_STS_HALT_INTERRUPT_REDIRECTION;
		}
	}
	return 0;
}
#else
int qm_mpr_set_violation_policy(const qm_mpr_viol_mode_t mode,
				qm_mpr_callback_t callback_fn, void *cb_data)
{
	QM_CHECK(mode <= MPR_VIOL_MODE_PROBE, -EINVAL);
	/*  interrupt mode */
	if (MPR_VIOL_MODE_INTERRUPT == mode) {
		callback = callback_fn;
		callback_data = cb_data;

		/* unmask interrupt */
		QM_IR_UNMASK_INT(QM_IRQ_SRAM_MPR_0_INT);

		QM_IR_MASK_HALTS(QM_INTERRUPT_ROUTER->sram_mpr_0_int_mask);
	}

	/* probe or reset mode */
	else {
		/* mask interrupt */
		QM_IR_MASK_INT(QM_IRQ_SRAM_MPR_0_INT);

		QM_IR_UNMASK_HALTS(QM_INTERRUPT_ROUTER->sram_mpr_0_int_mask);

		if (MPR_VIOL_MODE_PROBE == mode) {

			/* When an enabled host halt interrupt occurs, this bit
			* determines if the interrupt event triggers a warm
			* reset
			* or an entry into Probe Mode.
			* 0b : Warm Reset
			* 1b : Probe Mode Entry
			*/
			QM_SCSS_PMU->p_sts |=
			    QM_P_STS_HALT_INTERRUPT_REDIRECTION;
		} else {
			QM_SCSS_PMU->p_sts &=
			    ~QM_P_STS_HALT_INTERRUPT_REDIRECTION;
		}
	}
	return 0;
}
#endif /* QM_SENSOR */

#if (ENABLE_RESTORE_CONTEXT)
int qm_mpr_save_context(qm_mpr_context_t *const ctx)
{
	QM_CHECK(ctx != NULL, -EINVAL);
	int i;

	qm_mpr_reg_t *const controller = QM_MPR;

	for (i = 0; i < QM_MPR_NUM; i++) {
		ctx->mpr_cfg[i] = controller->mpr_cfg[i];
	}

	return 0;
}

int qm_mpr_restore_context(const qm_mpr_context_t *const ctx)
{
	QM_CHECK(ctx != NULL, -EINVAL);
	int i;

	qm_mpr_reg_t *const controller = QM_MPR;

	for (i = 0; i < QM_MPR_NUM; i++) {
		controller->mpr_cfg[i] = ctx->mpr_cfg[i];
	}

	return 0;
}
#else
int qm_mpr_save_context(qm_mpr_context_t *const ctx)
{
	(void)ctx;

	return 0;
}

int qm_mpr_restore_context(const qm_mpr_context_t *const ctx)
{
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
