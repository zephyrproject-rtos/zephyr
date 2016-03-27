/*
 * Copyright (c) 2015, Intel Corporation
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

#define ADDRESS_MASK_7_BIT (0x7F)
#define ADDRESS_MASK_LOW_BOUND (0x7F)
#define ADDRESS_MASK_UP_BOUND (0x1FC00)

static void (*callback)(void);

void qm_mpr_isr(void)
{
	(*callback)();
	QM_MPR->mpr_vsts = QM_MPR_VSTS_VALID;

	QM_ISR_EOI(QM_IRQ_SRAM_VECTOR);
}

qm_rc_t qm_mpr_set_config(const qm_mpr_id_t id,
			  const qm_mpr_config_t *const cfg)
{
	QM_CHECK(id < QM_MPR_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

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
	return QM_RC_OK;
}

qm_rc_t qm_mpr_get_config(const qm_mpr_id_t id, qm_mpr_config_t *const cfg)
{
	QM_CHECK(id < QM_MPR_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	cfg->low_bound = QM_MPR->mpr_cfg[id] & ADDRESS_MASK_LOW_BOUND;

	cfg->up_bound = (QM_MPR->mpr_cfg[id] & ADDRESS_MASK_UP_BOUND) >>
			QM_MPR_UP_BOUND_OFFSET;

	cfg->agent_read_en_mask =
	    (QM_MPR->mpr_cfg[id] & QM_MPR_RD_EN_MASK) >> QM_MPR_RD_EN_OFFSET;

	cfg->agent_write_en_mask =
	    (QM_MPR->mpr_cfg[id] & QM_MPR_WR_EN_MASK) >> QM_MPR_WR_EN_OFFSET;

	cfg->en_lock_mask = (QM_MPR->mpr_cfg[id] & QM_MPR_EN_LOCK_MASK) >>
			    QM_MPR_EN_LOCK_OFFSET;
	return QM_RC_OK;
}

qm_rc_t qm_mpr_set_violation_policy(const qm_mpr_viol_mode_t mode,
				    qm_mpr_callback_t callback_fn)
{
	QM_CHECK(mode <= MPR_VIOL_MODE_PROBE, QM_RC_EINVAL);
	/*  interrupt mode */
	if (MPR_VIOL_MODE_INTERRUPT == mode) {

		QM_CHECK(callback_fn != NULL, QM_RC_EINVAL);
		callback = callback_fn;

		/* host interrupt to Lakemont core */
		QM_SCSS_INT->int_sram_controller_mask |=
		    QM_INT_SRAM_CONTROLLER_HOST_HALT_MASK;
		QM_SCSS_INT->int_sram_controller_mask &=
		    ~QM_INT_SRAM_CONTROLLER_HOST_MASK;
	}

	/* probe or reset mode */
	else {
		/* host halt interrupt to Lakemont core */
		QM_SCSS_INT->int_sram_controller_mask |=
		    QM_INT_SRAM_CONTROLLER_HOST_MASK;
		QM_SCSS_INT->int_sram_controller_mask &=
		    ~QM_INT_SRAM_CONTROLLER_HOST_HALT_MASK;

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
		}

		else {
			QM_SCSS_PMU->p_sts &=
			    ~QM_P_STS_HALT_INTERRUPT_REDIRECTION;
		}
	}
	return QM_RC_OK;
}
