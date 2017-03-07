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

#include "ss_init.h"
#include "qm_mpr.h"

/* Sensor Subsystem application's pointer to the entry point (Flash0) */
#define SS_APP_PTR_ADDR (0x40000000)

#if (UNIT_TEST)
uint32_t __sensor_reset_vector[1];
#else
extern uint32_t __sensor_reset_vector[];
#endif

/* Set-up an MPR to protect ARC IVT (only ARC will have access to it). */
static void sensor_vectors_protect(void)
{
	/*
	 * MPR config:
	 * - Enable and lock MPR
	 * - Allow R/W access to ARC only
	 * - Protect first kB of SRAM (i.e., where ARC IVT is located)
	 */
	static const qm_mpr_config_t cfg = {
	    .en_lock_mask =
		QM_SRAM_MPR_EN_MASK_ENABLE | QM_SRAM_MPR_EN_MASK_LOCK,
	    .agent_read_en_mask = QM_SRAM_MPR_AGENT_MASK_SS,
	    .agent_write_en_mask = QM_SRAM_MPR_AGENT_MASK_SS,
	    .up_bound = 0,
	    .low_bound = 0,
	};
	qm_mpr_set_config(QM_MPR_1, &cfg);
}

void sensor_activation(void)
{
	/* Write the ARC reset vector.
	 *
	 * The ARC reset vector is in SRAM. The first 4 bytes of the Sensor
	 * Subsystem Flash partition point to the application entry point
	 * (pointer located at SS_APP_PTR_ADDR).
	 * Write the pointer to the application entry point into the reset
	 * vector.
	 */
	volatile uint32_t *ss_reset_vector = __sensor_reset_vector;
	volatile uint32_t *sensor_startup = (uint32_t *)SS_APP_PTR_ADDR;

	*ss_reset_vector = *sensor_startup;

	/* Set up an MPR to protect ARC IVT from other agents. */
	sensor_vectors_protect();

	/* Request ARC Run */
	QM_SCSS_SS->ss_cfg |= QM_SS_CFG_ARC_RUN_REQ_A;
}
