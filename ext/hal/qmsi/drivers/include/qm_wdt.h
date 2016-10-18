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

#ifndef __QM_WDT_H__
#define __QM_WDT_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Watchdog timer.
 *
 * @defgroup groupWDT WDT
 * @{
 */

/**
 * WDT Mode type.
 */
typedef enum {
	/** Watchdog Reset Response Mode.
	 *
	 * The watchdog will request a SoC Warm Reset on a timeout.
	 */
	QM_WDT_MODE_RESET = 0,
	/** Watchdog Interrupt Reset Response Mode.
	 *
	 * The watchdog will generate an interrupt on first timeout.
	 * If interrupt has not been cleared by the second timeout
	 * the watchdog will then request a SoC Warm Reset.
	 */
	QM_WDT_MODE_INTERRUPT_RESET
} qm_wdt_mode_t;

/**
 * QM WDT configuration type.
 */
typedef struct {
	/**
	 * Index for the WDT timeout table.
	 * For each instantiation of WDT there are multiple timeout
	 * values pre-programmed in hardware.
	 * Reference the SoC datasheet or register file for the table
	 * associated to the WDT being configured.
	 */
	uint32_t timeout;
	qm_wdt_mode_t mode; /**< Watchdog response mode. */
#if (HAS_WDT_PAUSE)
	/**
	 * Pause enable in LMT power state C2 and C2 Plus.
	 *
	 * When equal to 1, the WDT is paused when LMT enters the C2
	 * state. When equal to 0, the WDT is not paused when LMT
	 * enters the C2 state.
	 *
	 * This field applies only to Watchdogs on AON power island.
	 *
	 */
	bool pause_en;
#endif /* HAS_WDT_PAUSE */

	/**
	 * User callback.
	 *
	 * param[in] data Callback user data.
	 */
	void (*callback)(void *data);
	void *callback_data; /**< Callback user data. */
} qm_wdt_config_t;

/**
 * Start WDT.
 *
 * Once started, WDT can only be stopped by a SoC reset.
 *
 * @param[in] wdt WDT index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_wdt_start(const qm_wdt_t wdt);

/**
 * Set configuration of WDT module.
 *
 * This includes the timeout period in PCLK cycles, the WDT mode of operation.
 * It also registers an ISR to the user defined callback.
 *
 * @param[in] wdt WDT index.
 * @param[in] cfg New configuration for WDT.
 *                This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_wdt_set_config(const qm_wdt_t wdt, const qm_wdt_config_t *const cfg);

/**
 * Reload the WDT counter.
 *
 * Reload the WDT counter with safety value, i.e. service the watchdog.
 *
 * @param[in] wdt WDT index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_wdt_reload(const qm_wdt_t wdt);

#if (ENABLE_RESTORE_CONTEXT)
/**
 * Save watchdog context.
 *
 * Save the configuration of the watchdog before entering sleep.
 *
 * @param[in] wdt WDT index.
 * @param[out] ctx WDT context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_wdt_save_context(const qm_wdt_t wdt, qm_wdt_context_t *const ctx);

/**
 * Restore watchdog context.
 *
 * Restore the configuration of the watchdog after exiting sleep.
 *
 * @param[in] wdt WDT index.
 * @param[in] ctx WDT context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_wdt_restore_context(const qm_wdt_t wdt,
			   const qm_wdt_context_t *const ctx);
#endif /* ENABLE_RESTORE_CONTEXT */

/**
 * @}
 */

#endif /* __QM_WDT_H__ */
