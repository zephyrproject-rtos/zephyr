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

#include "qm_fpr.h"

static void (*callback[QM_FLASH_NUM])(void);

void qm_fpr_isr_0(void)
{
	(*callback[QM_FLASH_0])();
	QM_FLASH[QM_FLASH_0].mpr_vsts = QM_FPR_MPR_VSTS_VALID;

	QM_ISR_EOI(QM_IRQ_FLASH_0_VECTOR);
}

#if (QUARK_SE)
void qm_fpr_isr_1(void)
{
	(*callback[QM_FLASH_1])();
	QM_FLASH[QM_FLASH_1].mpr_vsts = QM_FPR_MPR_VSTS_VALID;

	QM_ISR_EOI(QM_IRQ_FLASH_1_VECTOR);
}
#endif

qm_rc_t qm_fpr_set_config(const qm_flash_t flash, const qm_fpr_id_t id,
			  const qm_fpr_config_t *const cfg,
			  const qm_flash_region_type_t region)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(id < QM_FPR_NUM, QM_RC_EINVAL);
	QM_CHECK(region < QM_MAIN_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);
	QM_CHECK(cfg->low_bound <= cfg->up_bound, QM_RC_EINVAL);

	QM_FLASH[flash].fpr_rd_cfg[id] &= ~QM_FPR_LOCK;

	if (region == QM_MAIN_FLASH_SYSTEM) {
		QM_FLASH[flash].fpr_rd_cfg[id] =
		    (cfg->allow_agents << QM_FPR_RD_ALLOW_OFFSET) |
		    ((cfg->up_bound + QM_FLASH_REGION_DATA_BASE_OFFSET)
		     << QM_FPR_UPPER_BOUND_OFFSET) |
		    (cfg->low_bound + QM_FLASH_REGION_DATA_BASE_OFFSET);
	}
#if (QUARK_D2000)
	else if (region == QM_MAIN_FLASH_OTP) {
		QM_FLASH[flash].fpr_rd_cfg[id] =
		    (cfg->allow_agents << QM_FPR_RD_ALLOW_OFFSET) |
		    (cfg->up_bound << QM_FPR_UPPER_BOUND_OFFSET) |
		    cfg->low_bound;
	}
#endif
	/* qm_fpr_id_t enable/lock */
	QM_FLASH[flash].fpr_rd_cfg[id] |=
	    (cfg->en_mask << QM_FPR_ENABLE_OFFSET);

	return QM_RC_OK;
}

qm_rc_t qm_fpr_get_config(const qm_flash_t flash, const qm_fpr_id_t id,
			  qm_fpr_config_t *const cfg,
			  const qm_flash_region_type_t region)
{
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(id < QM_FPR_NUM, QM_RC_EINVAL);
	QM_CHECK(region < QM_MAIN_FLASH_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	cfg->en_mask = (QM_FLASH[flash].fpr_rd_cfg[id] & QM_FPR_ENABLE_MASK) >>
		       QM_FPR_ENABLE_OFFSET;

	cfg->allow_agents =
	    (QM_FLASH[flash].fpr_rd_cfg[id] & QM_FPR_RD_ALLOW_MASK) >>
	    QM_FPR_RD_ALLOW_OFFSET;
	if (region == QM_MAIN_FLASH_SYSTEM) {
		cfg->up_bound = ((QM_FLASH[flash].fpr_rd_cfg[id] &
				  QM_FPR_UPPER_BOUND_MASK) >>
				 QM_FPR_UPPER_BOUND_OFFSET) -
				QM_FLASH_REGION_DATA_BASE_OFFSET;
		cfg->low_bound =
		    ((QM_FLASH[flash].fpr_rd_cfg[id] & QM_FPR_LOW_BOUND_MASK) -
		     QM_FLASH_REGION_DATA_BASE_OFFSET);
	}
#if (QUARK_D2000)
	else if (region == QM_MAIN_FLASH_OTP) {
		cfg->up_bound = (QM_FLASH[flash].fpr_rd_cfg[id] &
				 QM_FPR_UPPER_BOUND_MASK) >>
				QM_FPR_UPPER_BOUND_OFFSET;
		cfg->low_bound =
		    QM_FLASH[flash].fpr_rd_cfg[id] & QM_FPR_LOW_BOUND_MASK;
	}
#endif

	return QM_RC_OK;
}

qm_rc_t qm_fpr_set_violation_policy(const qm_fpr_viol_mode_t mode,
				    const qm_flash_t flash,
				    qm_fpr_callback_t callback_fn)
{
	QM_CHECK(mode <= FPR_VIOL_MODE_PROBE, QM_RC_EINVAL);
	QM_CHECK(flash < QM_FLASH_NUM, QM_RC_EINVAL);
	volatile uint32_t *int_flash_controller_mask =
	    &QM_SCSS_INT->int_flash_controller_0_mask;

	/* interrupt mode */
	if (FPR_VIOL_MODE_INTERRUPT == mode) {

		QM_CHECK(callback_fn != NULL, QM_RC_EINVAL);
		callback[flash] = callback_fn;

		/* host interrupt to Lakemont core */
		int_flash_controller_mask[flash] |=
		    QM_INT_FLASH_CONTROLLER_HOST_HALT_MASK;
		int_flash_controller_mask[flash] &=
		    ~QM_INT_FLASH_CONTROLLER_HOST_MASK;

		QM_SCSS_PMU->p_sts &= ~QM_P_STS_HALT_INTERRUPT_REDIRECTION;
	}

	/* probe or reset mode */
	else {
		int_flash_controller_mask[flash] |=
		    QM_INT_FLASH_CONTROLLER_HOST_MASK;
		int_flash_controller_mask[flash] &=
		    ~QM_INT_FLASH_CONTROLLER_HOST_HALT_MASK;

		if (FPR_VIOL_MODE_PROBE == mode) {

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
	return QM_RC_OK;
}
