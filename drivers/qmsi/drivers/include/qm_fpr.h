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

#ifndef __QM_FPR_H__
#define __QM_FPR_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Flash Protection Region control for Quark Microcontrollers.
 *
 * @brief Flash Protection Region for QM.
 * @defgroup groupFPR FPR
 * @{
 */
typedef void (*qm_fpr_callback_t)(void);

/**
 * FPR Interrupt Service Routines
 */
void qm_fpr_isr_0(void);
void qm_fpr_isr_1(void);

/**
 * FPR register map.
 */
typedef enum { QM_FPR_0, QM_FPR_1, QM_FPR_2, QM_FPR_3, QM_FPR_NUM } qm_fpr_id_t;

typedef enum {
	QM_FPR_DISABLE,
	QM_FPR_ENABLE,
	QM_FPR_LOCK_DISABLE,
	QM_FPR_LOCK_ENABLE
} qm_fpr_en_t;

typedef enum {
	FPR_VIOL_MODE_INTERRUPT = 0,
	FPR_VIOL_MODE_RESET,
	FPR_VIOL_MODE_PROBE
} qm_fpr_viol_mode_t;

typedef enum {
	QM_MAIN_FLASH_SYSTEM = 0,
#if (QUARK_D2000)
	QM_MAIN_FLASH_OTP,
#endif
	QM_MAIN_FLASH_NUM,
} qm_flash_region_type_t;

typedef enum {
	QM_FPR_HOST_PROCESSOR = BIT(0),
#if (QUARK_SE)
	QM_FPR_SENSOR_SUBSYSTEM = BIT(1),
#endif
	QM_FPR_DMA = BIT(2),
#if (QUARK_SE)
	QM_FPR_OTHER_AGENTS = BIT(3)
#endif
} qm_fpr_read_allow_t;

/** Flash Protection Region configuration structure */
typedef struct {
	qm_fpr_en_t en_mask;		  /**< Enable/lock bitmask */
	qm_fpr_read_allow_t allow_agents; /**< Per-agent read enable bitmask */
	uint8_t up_bound;  /**< 1KB-aligned upper Flash phys addr */
	uint8_t low_bound; /**< 1KB-aligned lower Flash phys addr */
} qm_fpr_config_t;

#define QM_FPR_FPR0_REG_OFFSET (7)

#define QM_FPR_WRITE_LOCK_OFFSET (31)
#define QM_FPR_ENABLE_OFFSET (30)
#define QM_FPR_ENABLE_MASK BIT(QM_FPR_ENABLE_OFFSET)
#define QM_FPR_RD_ALLOW_OFFSET (20)
#define QM_FPR_RD_ALLOW_MASK (0xF00000)
#define QM_FPR_UPPER_BOUND_OFFSET (10)
#define QM_FPR_UPPER_BOUND_MASK (0x3FC00)
#define QM_FPR_LOW_BOUND_MASK (0xFF)
#define QM_FPR_MPR_VSTS_VALID BIT(31)

#define QM_FPR_LOCK BIT(31)

/** FPR enable mask */
#define QM_FPR_EN_MASK_ENABLE BIT(0)
/** FPR mask lock */
#define QM_FPR_EN_MASK_LOCK BIT(1)
/** FPR mask host */
#define QM_FPR_AGENT_MASK_HOST BIT(0)
/** FPR mask ss */
#define QM_FPR_AGENT_MASK_SS BIT(1)
/** FPR mask dma */
#define QM_FPR_AGENT_MASK_DMA BIT(2)
/** FPR mask other agents */
#define QM_FPR_AGENT_MASK_OTHER BIT(3)

/**
 * Configure a Flash controller's Flash Protection Region.
 *
 * @param [in] flash Which Flash controller to configure.
 * @param [in] id FPR identifier.
 * @param [in] cfg FPR configuration.
 * @param [in] region The region of Flash to be configured.
 * @return RC_OK on success, error code otherwise.
 */
qm_rc_t qm_fpr_set_config(const qm_flash_t flash, const qm_fpr_id_t id,
			  const qm_fpr_config_t *const cfg,
			  const qm_flash_region_type_t region);

/**
 * Retrieve Flash controller's Flash Protection Region configuration.
 * This will set the cfg parameter to match the current configuration
 * of the given Flash controller's FPR.
 *
 * @brief Get Flash FPR configuration.
 * @param [in] flash Which Flash to read the configuration of.
 * @param [in] id FPR identifier.
 * @param [out] cfg FPR configuration.
 * @param [in] region The region of Flash configured.
 * @return RC_OK on success, error code otherwise.
 */
qm_rc_t qm_fpr_get_config(const qm_flash_t flash, const qm_fpr_id_t id,
			  qm_fpr_config_t *const cfg,
			  const qm_flash_region_type_t region);

/**
 * Configure FPR violation behaviour
 *
 * @param [in] mode (generate interrupt, warm reset, enter probe mode).
 * @param [in] fpr_cb for interrupt mode (only). This cannot be null.
 * @param [in] flash controller.
 * @return RC_OK on success, error code otherwise.
 * */
qm_rc_t qm_fpr_set_violation_policy(const qm_fpr_viol_mode_t mode,
				    const qm_flash_t flash,
				    qm_fpr_callback_t fpr_cb);

/**
 * @}
 */

#endif /* __QM_FPR_H__ */
