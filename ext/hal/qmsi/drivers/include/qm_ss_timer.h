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

#ifndef __QM_SS_TIMER_H__
#define __QM_SS_TIMER_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"

/**
 * Timer driver for Sensor Subsystem.
 *
 * @defgroup groupSSTimer SS Timer
 * @{
 */

/**
 * Sensor Subsystem Timer Configuration Type.
 */
typedef struct {
	bool watchdog_mode; /**< Watchdog mode. */

	/**
	 * Increments in run state only.
	 *
	 * If this field is set to 0, the timer will count
	 * in both halt state and running state.
	 * When set to 1, this will only increment in
	 * running state.
	 */
	bool inc_run_only;
	bool int_en;    /**< Interrupt enable. */
	uint32_t count; /**< Final count value. */

	/**
	 * User callback.
	 *
	 * Called for any interrupt on the Sensor Subsystem Timer.
	 *
	 * @param[in] data The callback user data.
	 */
	void (*callback)(void *data);
	void *callback_data; /**< Callback user data. */
} qm_ss_timer_config_t;

/**
 * Set the SS timer configuration.
 *
 * This includes final count value, timer mode and if interrupts are enabled.
 * If interrupts are enabled, it will configure the callback function.
 *
 * @param[in] timer Which SS timer to configure.
 * @param[in] cfg SS timer configuration. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_timer_set_config(const qm_ss_timer_t timer,
			   const qm_ss_timer_config_t *const cfg);

/**
 * Set SS timer count value.
 *
 * Set the current count value of the SS timer.
 *
 * @param[in] timer Which SS timer to set the count of.
 * @param[in] count Value to load the timer with.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_timer_set(const qm_ss_timer_t timer, const uint32_t count);

/**
 * Get SS timer count value.
 *
 * Get the current count value of the SS timer.
 *
 * @param[in] timer Which SS timer to get the count of.
 * @param[out] count Current value of timer. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_timer_get(const qm_ss_timer_t timer, uint32_t *const count);

/*
 * Save SS TIMER context.
 *
 * Save the configuration of the specified TIMER peripheral
 * before entering sleep.
 *
 * @param[in] timer SS TIMER index.
 * @param[out] ctx SS TIMER context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_timer_save_context(const qm_ss_timer_t timer,
			     qm_ss_timer_context_t *const ctx);

/*
 * Restore SS TIMER context.
 *
 * Restore the configuration of the specified TIMER peripheral
 * after exiting sleep.
 *
 * @param[in] timer SS TIMER index.
 * @param[in] ctx SS TIMER context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_timer_restore_context(const qm_ss_timer_t timer,
				const qm_ss_timer_context_t *const ctx);

/**
 * @}
 */

#endif /* __QM_SS_TIMER_H__ */
