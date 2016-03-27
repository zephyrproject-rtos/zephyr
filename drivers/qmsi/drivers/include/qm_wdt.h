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

#ifndef __QM_WDT_H__
#define __QM_WDT_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Watchdog timer for Quark Microcontrollers.
 *
 * @defgroup groupWDT WDT
 * @{
 */

#define QM_WDT_ENABLE (BIT(0))
#define QM_WDT_MODE (BIT(1))
#define QM_WDT_MODE_OFFSET (1)

#define QM_WDT_TIMEOUT_MASK (0xF)

/**
 * WDT Mode type.
 */
typedef enum { QM_WDT_MODE_RESET, QM_WDT_MODE_INTERRUPT_RESET } qm_wdt_mode_t;

/**
 * WDT clock cycles for timeout type. This value is a power of 2.
 */
typedef enum {
	QM_WDT_2_POW_16_CYCLES,
	QM_WDT_2_POW_17_CYCLES,
	QM_WDT_2_POW_18_CYCLES,
	QM_WDT_2_POW_19_CYCLES,
	QM_WDT_2_POW_20_CYCLES,
	QM_WDT_2_POW_21_CYCLES,
	QM_WDT_2_POW_22_CYCLES,
	QM_WDT_2_POW_23_CYCLES,
	QM_WDT_2_POW_24_CYCLES,
	QM_WDT_2_POW_25_CYCLES,
	QM_WDT_2_POW_26_CYCLES,
	QM_WDT_2_POW_27_CYCLES,
	QM_WDT_2_POW_28_CYCLES,
	QM_WDT_2_POW_29_CYCLES,
	QM_WDT_2_POW_30_CYCLES,
	QM_WDT_2_POW_31_CYCLES
} qm_wdt_clock_timeout_cycles_t;

/**
 * QM WDT configuration type.
 */
typedef struct {
	qm_wdt_clock_timeout_cycles_t timeout;
	qm_wdt_mode_t mode;
	void (*callback)(void);
} qm_wdt_config_t;

/**
 * Start WDT. Once started, WDT can only be stopped by a SoC reset.
 *
 * @param [in] wdt WDT index.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_wdt_start(const qm_wdt_t wdt);

/**
 * Set configuration of WDT module. This includes the timeout period in PCLK
 * cycles, the WDT mode of operation. It also registers an ISR to the user
 * defined callback.
 *
 * @param [in] wdt WDT index.
 * @param[in] cfg New configuration for WDT.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_wdt_set_config(const qm_wdt_t wdt, const qm_wdt_config_t *const cfg);

/**
 * Get the current configuration of WDT module. This includes the
 * timeout period in PCLK cycles, the WDT mode of operation.
 *
 * @param [in] wdt WDT index.
 * @param[out] cfg Parameter to be set with the current WDT configuration.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_wdt_get_config(const qm_wdt_t wdt, qm_wdt_config_t *const cfg);

/**
 * Reload the WDT counter with safety value, i.e. service the watchdog
 *
 * @param [in] wdt WDT index.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_wdt_reload(const qm_wdt_t wdt);

/**
 * WDT Interrupt Service Routine
 */
void qm_wdt_isr_0(void);

/**
 * @}
 */

#endif /* __QM_WDT_H__ */
