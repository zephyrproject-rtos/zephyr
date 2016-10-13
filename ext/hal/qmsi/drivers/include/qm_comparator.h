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

#ifndef __QM_COMPARATOR_H__
#define __QM_COMPARATOR_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Analog Comparator.
 *
 * @defgroup groupAC Analog Comparator
 * @{
 */

/**
 * Analog Comparator configuration type.
 *
 * Each bit in the registers controls a single Analog Comparator pin.
 */
typedef struct {
	uint32_t int_en;    /**< Interrupt enable. */
	uint32_t reference; /**< Reference voltage, 1b: VREF; 0b: AR_PIN. */
	uint32_t polarity;  /**< 0b: input>ref; 1b: input<ref */
	uint32_t power;     /**< 1b: Normal mode; 0b:Power-down/Shutdown mode */
#if HAS_COMPARATOR_VREF2
	/**< 0b: VREF_1; 1b: VREF_2; When reference is external */
	uint32_t ar_pad;
#endif /* HAS_COMPARATOR_VREF2 */

	/**
	 * Transfer callback.
	 *
	 * @param[in] data Callback user data.
	 * @param[in] status Comparator interrupt status.
	 */
	void (*callback)(void *data, uint32_t int_status);
	void *callback_data; /**< Callback user data. */
} qm_ac_config_t;

/**
 * Set Analog Comparator configuration.
 *
 * @param[in] config Analog Comparator configuration. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ac_set_config(const qm_ac_config_t *const config);

/**
 * @}
 */

#endif /* __QM_COMPARATOR_H__ */
