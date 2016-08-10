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

#ifndef __QM_MPR_H__
#define __QM_MPR_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Memory Protection Region control for Quark Microcontrollers.
 *
 * @defgroup groupMPR MPR
 * @{
 */

/** MPR mask enable */
#define QM_SRAM_MPR_EN_MASK_ENABLE BIT(0)
/** MPR mask lock */
#define QM_SRAM_MPR_EN_MASK_LOCK BIT(1)
/** MPR mask host */
#define QM_SRAM_MPR_AGENT_MASK_HOST BIT(0)
/** MPR mask ss */
#if (QUARK_SE)
#define QM_SRAM_MPR_AGENT_MASK_SS BIT(1)
#endif
/** MPR mask dma */
#define QM_SRAM_MPR_AGENT_MASK_DMA BIT(2)

typedef void (*qm_mpr_callback_t)(void *);

/* MPR identifier */
typedef enum {
	QM_MPR_0 = 0,
	QM_MPR_1,
	QM_MPR_2,
	QM_MPR_3,
	QM_MPR_NUM
} qm_mpr_id_t;

/** SRAM Memory Protection Region configuration type. */
typedef struct {
	uint8_t en_lock_mask;	/**< Enable/lock bitmask */
	uint8_t agent_read_en_mask;  /**< Per-agent read enable bitmask */
	uint8_t agent_write_en_mask; /**< Per-agent write enable bitmask */
	uint8_t up_bound;	    /**< 1KB-aligned upper addr */
	uint8_t low_bound;	   /**< 1KB-aligned lower addr */
} qm_mpr_config_t;

typedef enum {
	MPR_VIOL_MODE_INTERRUPT = 0,
	MPR_VIOL_MODE_RESET,
	MPR_VIOL_MODE_PROBE
} qm_mpr_viol_mode_t;

/**
 * Configure SRAM controller's Memory Protection Region.
 *
 * @param [in] id Which MPR to configure.
 * @param [in] cfg MPR configuration.
 * @return int 0 on success, error code otherwise.
 */
int qm_mpr_set_config(const qm_mpr_id_t id, const qm_mpr_config_t *const cfg);

/**
 * Configure MPR violation behaviour
 *
 * @param [in] mode (generate interrupt, warm reset, enter probe mode).
 * @param [in] callback_fn for interrupt mode (only).
 * @param [in] callback_data user data for interrupt mode (only).
 * @return int 0 on success, error code otherwise.
 * */
int qm_mpr_set_violation_policy(const qm_mpr_viol_mode_t mode,
				qm_mpr_callback_t callback_fn, void *data);

/**
 * @}
 */

#endif /* __QM_MPR_H__ */
