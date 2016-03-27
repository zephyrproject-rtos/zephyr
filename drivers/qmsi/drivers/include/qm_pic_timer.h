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

#ifndef __QM_PIC_TIMER_H__
#define __QM_PIC_TIMER_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
* PIC timer for Quark Microcontrollers.
*
* @defgroup groupPICTimer PIC Timer
* @{
*/

/**
 * PIC timer mode type.
 */
typedef enum {
	QM_PIC_TIMER_MODE_ONE_SHOT,
	QM_PIC_TIMER_MODE_PERIODIC
} qm_pic_timer_mode_t;

/**
* PIC timer configuration type.
*/
typedef struct {
	qm_pic_timer_mode_t mode; /**< Operation mode */
	bool int_en;		  /**< Interrupt enable */
	void (*callback)(void);   /**< Callback function */
} qm_pic_timer_config_t;

/**
 * PIC timer Interrupt Service Routine
 */
void qm_pic_timer_isr(void);

/**
* Set the PIC timer configuration.
* This includes timer mode and if interrupts are enabled. If interrupts are
* enabled, it will configure the callback function.
*
* @brief Set the PIC timer configuration.
* @param [in] cfg PIC timer configuration.
* @return qm_rc_t QM_RC_OK on success, error code otherwise.
*/
qm_rc_t qm_pic_timer_set_config(const qm_pic_timer_config_t *const cfg);

/**
* Get PIC timer configuration.
* Populate the cfg parameter to match the current configuration of the PIC
* timer.
*
* @brief Get PIC timer configuration.
* @param[out] cfg PIC timer configuration.
* @return qm_rc_t QM_RC_OK on success, error code otherwise.
*/
qm_rc_t qm_pic_timer_get_config(qm_pic_timer_config_t *const cfg);

/**
* Set the current count value of the PIC timer.
* A value equal to 0 effectively stops the timer.
*
* @param [in] count Value to load the timer with.
* @return qm_rc_t QM_RC_OK on success, error code otherwise.
*/
qm_rc_t qm_pic_timer_set(const uint32_t count);

/**
* Get the current count value of the PIC timer.
*
* @return uint32_t Returns current PIC timer count value.
*/
uint32_t qm_pic_timer_get(void);

/**
* @}
*/

#endif /* __QM_PIC_TIMER_H__ */
