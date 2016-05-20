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

#ifndef __QM_RTC_H__
#define __QM_RTC_H__

#include "qm_common.h"
#include "qm_soc_regs.h"
#include "clk.h"

/**
 * Real Time clock.
 *
 * @defgroup groupRTC RTC
 * @{
 */

#define QM_RTC_DIVIDER CLK_RTC_DIV_1

#define QM_RTC_CCR_INTERRUPT_ENABLE BIT(0)
#define QM_RTC_CCR_INTERRUPT_MASK BIT(1)
#define QM_RTC_CCR_ENABLE BIT(2)

#define QM_RTC_ALARM_SECOND (32768 / BIT(QM_RTC_DIVIDER))
#define QM_RTC_ALARM_MINUTE (QM_RTC_ALARM_SECOND * 60)
#define QM_RTC_ALARM_HOUR (QM_RTC_ALARM_MINUTE * 60)
#define QM_RTC_ALARM_DAY (QM_RTC_ALARM_HOUR * 24)

/**
 * RTC configuration type.
 */
typedef struct {
	uint32_t init_val;  /**< Initial value in RTC clocks. */
	bool alarm_en;      /**< Alarm enable. */
	uint32_t alarm_val; /**< Alarm value in RTC clocks. */
	/**
	 * User callback.
	 *
	 * @param[in] data User defined data.
	 */
	void (*callback)(void *data);
	void *callback_data; /**< Callback user data. */
} qm_rtc_config_t;

/**
 * Set RTC configuration.
 *
 * This includes the initial value in RTC clock periods, and the alarm value if
 * an alarm is required. If the alarm is enabled, register an ISR with the user
 * defined callback function.
 *
 * @param[in] rtc RTC index.
 * @param[in] cfg New RTC configuration. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_rtc_set_config(const qm_rtc_t rtc, const qm_rtc_config_t *const cfg);

/**
 * Set Alarm value.
 *
 * Set a new RTC alarm value after an alarm, that has been set using the
 * qm_rtc_set_config function, has expired and a new alarm value is required.
 *
 * @param[in] rtc RTC index.
 * @param[in] alarm_val Value to set alarm to.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_rtc_set_alarm(const qm_rtc_t rtc, const uint32_t alarm_val);

/**
 * @}
 */

#endif /* __QM_RTC_H__ */
