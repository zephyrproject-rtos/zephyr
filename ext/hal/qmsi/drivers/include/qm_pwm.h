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

#ifndef __QM_PWM_H__
#define __QM_PWM_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Pulse width modulation and Timer driver.
 *
 * @defgroup groupPWM PWM / Timer
 * @{
 */

/**
 * QM PWM mode type.
 */
typedef enum {
	/**< Timer: Free running mode. */
	QM_PWM_MODE_TIMER_FREE_RUNNING = QM_PWM_MODE_TIMER_FREE_RUNNING_VALUE,
	/**< Timer: Counter mode. */
	QM_PWM_MODE_TIMER_COUNT = QM_PWM_MODE_TIMER_COUNT_VALUE,
	/**< PWM mode. */
	QM_PWM_MODE_PWM = QM_PWM_MODE_PWM_VALUE
} qm_pwm_mode_t;

/**
 * QM PWM / Timer configuration type.
 */
typedef struct {
	/**
	 * Number of cycles the PWM output is driven low.
	 * In timer mode, this is the timer load count. Must be > 0.
	 */
	uint32_t lo_count;
	/**
	 * Number of cycles the PWM output is driven high.
	 * Not applicable in timer mode. Must be > 0.
	 */
	uint32_t hi_count;
	bool mask_interrupt; /**< Mask interrupt. */
	qm_pwm_mode_t mode;  /**< Pwm mode. */

	/**
	 * User callback.
	 *
	 * @param[in] data The callback user data.
	 * @param[in] int_status The timer status.
	 */
	void (*callback)(void *data, uint32_t int_status);
	void *callback_data; /**< Callback user data. */
} qm_pwm_config_t;

/**
 * Change the configuration of a PWM channel.
 *
 * This includes low period load value, high period load value, interrupt
 * enable/disable. If interrupts are enabled, registers an ISR with the given
 * user callback function. When operating in PWM mode, 0% and 100% duty cycle
 * is not available on Quark SE or Quark D2000. When setting the mode to PWM
 * mode, hi_count must be > 0. In timer mode, the value of high count is
 * ignored.
 *
 * @brief Set PWM channel configuration.
 * @param[in] pwm Which PWM module to configure.
 * @param[in] id PWM channel id to configure.
 * @param[in] cfg New configuration for PWM. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pwm_set_config(const qm_pwm_t pwm, const qm_pwm_id_t id,
		      const qm_pwm_config_t *const cfg);

/**
 * Set the next period values of a PWM channel.
 *
 * This includes low period count and high period count. When operating in PWM
 * mode, 0% and 100% duty cycle is not available on Quark SE or Quark D2000.
 * When operating in PWM mode, hi_count must be > 0. In timer mode, the value of
 * high count is ignored.
 *
 * @brief Set PWM period counts.
 * @param[in] pwm Which PWM module to set the counts of.
 * @param[in] id PWM channel id to set.
 * @param[in] lo_count Num of cycles the output is driven low.
 * @param[in] hi_count Num of cycles the output is driven high.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pwm_set(const qm_pwm_t pwm, const qm_pwm_id_t id,
	       const uint32_t lo_count, const uint32_t hi_count);

/**
 * Get the current period values of a PWM channel.
 *
 * @param[in] pwm Which PWM module to get the count of.
 * @param[in] id PWM channel id to read the values of.
 * @param[out] lo_count Num of cycles the output is driven low. This must not be
 * NULL.
 * @param[out] hi_count Num of cycles the output is driven high. This must not
 * be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pwm_get(const qm_pwm_t pwm, const qm_pwm_id_t id,
	       uint32_t *const lo_count, uint32_t *const hi_count);

/**
 * Start a PWM/timer channel.
 *
 * @param[in] pwm Which PWM block the PWM is in.
 * @param[in] id PWM channel id to start.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pwm_start(const qm_pwm_t pwm, const qm_pwm_id_t id);

/**
 * Stop a PWM/timer channel.
 *
 * @param[in] pwm Which PWM block the PWM is in.
 * @param[in] id PWM channel id to stop.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pwm_stop(const qm_pwm_t pwm, const qm_pwm_id_t id);

#if (ENABLE_RESTORE_CONTEXT)
/**
 * Save PWM peripheral's context.
 *
 * Saves the configuration of the specified PWM peripheral
 * before entering sleep.
 *
 * @param[in] pwm PWM device.
 * @param[out] ctx PWM context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pwm_save_context(const qm_pwm_t pwm, qm_pwm_context_t *const ctx);

/**
 * Restore PWM peripheral's context.
 *
 * Restore the configuration of the specified PWM peripheral
 * after exiting sleep.
 *
 * @param[in] pwm PWM device.
 * @param[in] ctx PWM context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pwm_restore_context(const qm_pwm_t pwm,
			   const qm_pwm_context_t *const ctx);
#endif /* ENABLE_RESTORE_CONTEXT */

/**
 * @}
 */

#endif /* __QM_PWM_H__ */
