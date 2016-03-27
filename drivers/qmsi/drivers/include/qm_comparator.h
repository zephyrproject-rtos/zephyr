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

#ifndef __QM_COMPARATOR_H__
#define __QM_COMPARATOR_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Analog comparator for Quark Microcontrollers.
 *
 * @defgroup groupAC Analog Comparator
 * @{
 */

/**
 * Analog comparator configuration type. Each bit in the
 * registers controls an Analog Comparator pin.
 */
typedef struct {
	uint32_t int_en;    /* Enable/disable comparator interrupt */
	uint32_t reference; /* 1b: VREF; 0b: AR_PIN */
	uint32_t polarity;  /* 0b: input>ref; 1b: input<ref */
	uint32_t power;     /* 1b: Normal mode; 0b:Power-down/Shutdown mode */
	void (*callback)(uint32_t int_status); /* Callback function */
} qm_ac_config_t;

/**
 * Analog Comparator Interrupt Service Routine
 */
void qm_ac_isr(void);

/**
 * Get Analog Comparator configuration.
 *
 * @param [in] config Analog Comparator configuration.
 * @return qm_rc_t QM_RC_OK on success, QM_RC_ERR otherwise.
 */
qm_rc_t qm_ac_get_config(qm_ac_config_t *const config);

/**
 * Set Analog Comparator configuration.
 *
 * @param [in] config Analog Comparator configuration.
 * @return qm_rc_t QM_RC_OK on success, QM_RC_ERR otherwise.
 */
qm_rc_t qm_ac_set_config(const qm_ac_config_t *const config);

/**
 * @}
 */

#endif /* __QM_COMPARATOR_H__ */
