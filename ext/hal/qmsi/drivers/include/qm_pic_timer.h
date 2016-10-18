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

#ifndef __QM_PIC_TIMER_H__
#define __QM_PIC_TIMER_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * PIC timer.
 *
 * @defgroup groupPICTimer PIC Timer
 * @{
 */

/**
 * PIC timer mode type.
 */
typedef enum {
	QM_PIC_TIMER_MODE_ONE_SHOT, /**< One shot mode. */
	QM_PIC_TIMER_MODE_PERIODIC  /**< Periodic mode. */
} qm_pic_timer_mode_t;

/**
 * PIC timer configuration type.
 */
typedef struct {
	qm_pic_timer_mode_t mode; /**< Operation mode. */
	bool int_en;		  /**< Interrupt enable. */

	/**
	 * User callback.
	 *
	 * @param[in] data User defined data.
	 */
	void (*callback)(void *data);
	void *callback_data; /**< Callback user data. */
} qm_pic_timer_config_t;

/**
 * Set the PIC timer configuration.
 *
 * Set the PIC timer configuration.
 * This includes timer mode and if interrupts are enabled. If interrupts are
 * enabled, it will configure the callback function.
 *
 * @param[in] cfg PIC timer configuration. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pic_timer_set_config(const qm_pic_timer_config_t *const cfg);

/**
 * Set the current count value of the PIC timer.
 *
 * Set the current count value of the PIC timer.
 * A value equal to 0 effectively stops the timer.
 *
 * @param[in] count Value to load the timer with.
 *
 * @return Standard errno return type for QMSI.
 * @retval Always returns 0.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pic_timer_set(const uint32_t count);

/**
 * Get the current count value of the PIC timer.
 *
 * @param[out] count Pointer to the store the timer count.
 *                   This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pic_timer_get(uint32_t *const count);

#if (ENABLE_RESTORE_CONTEXT)
/**
 * Save PIC Timer peripheral's context.
 *
 * Saves the configuration of the specified PIC Timer peripheral
 * before entering sleep.
 *
 * @param[out] ctx PIC Timer context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pic_timer_save_context(qm_pic_timer_context_t *const ctx);

/**
 * Restore PIC Timer peripheral's context.
 *
 * Restore the configuration of the specified PIC Timer peripheral
 * after exiting sleep.
 * The timer is restored to the count saved before sleep.
 *
 * @param[in] ctx PIC Timer context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_pic_timer_restore_context(const qm_pic_timer_context_t *const ctx);
#endif
/**
 * @}
 */

#endif /* __QM_PIC_TIMER_H__ */
