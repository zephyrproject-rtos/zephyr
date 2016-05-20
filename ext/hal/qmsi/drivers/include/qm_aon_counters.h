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

#ifndef __QM_AON_COUNTERS_H__
#define __QM_AON_COUNTERS_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Always-on Counters.
 *
 * @defgroup groupAONC Always-on Counters
 * @{
 */

/**
 * Always-on Periodic Timer configuration type.
 */
typedef struct {
	uint32_t count; /**< Time to count down from in clock cycles.*/
	bool int_en;    /**< Enable/disable the interrupts. */

	/**
	* User callback.
	*
	* @param[in] data User defined data.
	*/
	void (*callback)(void *data);
	void *callback_data; /**< Callback data. */
} qm_aonpt_config_t;

/**
 * Enable the Always-on Counter.
 *
 * @param[in] aonc Always-on counter to read.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
*/
int qm_aonc_enable(const qm_scss_aon_t aonc);

/**
 * Disable the Always-on Counter.
 *
 * @param[in] aonc Always-on counter to read.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_aonc_disable(const qm_scss_aon_t aonc);

/**
 * Get the current value of the Always-on Counter.
 *
 * Returns a 32-bit value which represents the number of clock cycles
 * since the counter was first enabled.
 *
 * @param[in] aonc Always-on counter to read.
 * @param[out] val Value of the counter. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_aonc_get_value(const qm_scss_aon_t aonc, uint32_t *const val);

/**
 * Set the Always-on Periodic Timer configuration.
 *
 * This includes the initial value of the Always-on Periodic Timer,
 * the interrupt enable and the callback function that will be run
 * when the timer expiers and an interrupt is triggered.
 * The Periodic Timer is disabled if the counter is set to 0.
 *
 * @param[in] aonc Always-on counter to read.
 * @param[in] cfg New configuration for the Always-on Periodic Timer.
 * This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_aonpt_set_config(const qm_scss_aon_t aonc,
			const qm_aonpt_config_t *const cfg);

/**
 * Get the current value of the Always-on Periodic Timer.
 *
 * Returns a 32-bit value which represents the number of clock cycles
 * remaining before the timer fires.
 * This is the initial configured number minus the number of cycles that have
 * passed.
 *
 * @param[in] aonc Always-on counter to read.
 * @param[out] val Value of the Always-on Periodic Timer.
 * This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_aonpt_get_value(const qm_scss_aon_t aonc, uint32_t *const val);

/**
 * Get the current status of the Always-on Periodic Timer.
 *
 *  Returns true if the timer has expired. This will continue to return true
 *  until it is cleared with qm_aonpt_clear().
 *
 * @param[in] aonc Always-on counter to read.
 * @param[out] status Status of the Always-on Periodic Timer.
 * This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_aonpt_get_status(const qm_scss_aon_t aonc, bool *const status);

/**
 * Clear the status of the Always-on Periodic Timer.
 *
 * The status must be clear before the Always-on Periodic Timer can trigger
 * another interrupt.
 *
 * @param[in] aonc Always-on counter to read.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_aonpt_clear(const qm_scss_aon_t aonc);

/**
 * Reset the Always-on Periodic Timer back to the configured value.
 *
 * @param[in] aonc Always-on counter to read.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_aonpt_reset(const qm_scss_aon_t aonc);

/**
 * @}
 */
#endif /* __QM_AON_COUNTERS_H__ */
