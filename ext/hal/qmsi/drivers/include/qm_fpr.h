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

#ifndef __QM_FPR_H__
#define __QM_FPR_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Flash Protection Region control.
 *
 * @defgroup groupFPR FPR
 * @{
 */
typedef void (*qm_fpr_callback_t)(void *data);

/**
 * FPR register map.
 */
typedef enum {
	QM_FPR_0, /**< FPR 0. */
	QM_FPR_1, /**< FPR 1. */
	QM_FPR_2, /**< FPR 2. */
	QM_FPR_3, /**< FPR 3. */
	QM_FPR_NUM
} qm_fpr_id_t;

/**
 * FPR enable type.
 */
typedef enum {
	QM_FPR_DISABLE,      /**< Disable FPR. */
	QM_FPR_ENABLE,       /**< Enable FPR. */
	QM_FPR_LOCK_DISABLE, /**< Disable FPR lock. */
	QM_FPR_LOCK_ENABLE   /**< Enable FPR lock. */
} qm_fpr_en_t;

/**
 * FPR vilation mode type.
 */
typedef enum {
	FPR_VIOL_MODE_INTERRUPT = 0, /**< Generate interrupt on violation. */
	FPR_VIOL_MODE_RESET,	 /**< Reset SoC on violation. */
	FPR_VIOL_MODE_PROBE	  /**< Enter probe mode on violation. */
} qm_fpr_viol_mode_t;

/**
 * FPR region type.
 */
typedef enum {
	QM_MAIN_FLASH_SYSTEM = 0, /**< System flash region. */
#if (QUARK_D2000)
	QM_MAIN_FLASH_DATA, /**< Data flash region. */
#endif
	QM_MAIN_FLASH_NUM, /**< Number of flash regions. */
} qm_flash_region_type_t;

/**
 * FPR read allow type.
 */
typedef enum {
	QM_FPR_HOST_PROCESSOR =
	    BIT(0), /**< Allow host processor to access flash region. */
#if (QUARK_SE)
	QM_FPR_SENSOR_SUBSYSTEM =
	    BIT(1), /**< Allow sensor subsystem to access flash region. */
#endif
	QM_FPR_DMA = BIT(2), /**< Allow DMA to access flash region. */
#if (QUARK_SE)
	QM_FPR_OTHER_AGENTS =
	    BIT(3) /**< Allow other agents to access flash region. */
#endif
} qm_fpr_read_allow_t;

/**
 * Flash Protection Region configuration structure.
 */
typedef struct {
	qm_fpr_en_t en_mask;		  /**< Enable/lock bitmask. */
	qm_fpr_read_allow_t allow_agents; /**< Per-agent read enable bitmask. */
	uint8_t up_bound;  /**< 1KB-aligned upper Flash phys addr. */
	uint8_t low_bound; /**< 1KB-aligned lower Flash phys addr. */
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

#define QM_FPR_EN_MASK_ENABLE BIT(0)
#define QM_FPR_EN_MASK_LOCK BIT(1)
#define QM_FPR_AGENT_MASK_HOST BIT(0)
#define QM_FPR_AGENT_MASK_SS BIT(1)
#define QM_FPR_AGENT_MASK_DMA BIT(2)
#define QM_FPR_AGENT_MASK_OTHER BIT(3)

/**
 * Configure a Flash controller's Flash Protection Region.
 *
 * @param[in] flash Which Flash controller to configure.
 * @param[in] id FPR identifier.
 * @param[in] cfg FPR configuration.
 * @param[in] region The region of Flash to be configured.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_fpr_set_config(const qm_flash_t flash, const qm_fpr_id_t id,
		      const qm_fpr_config_t *const cfg,
		      const qm_flash_region_type_t region);

/**
 * Configure FPR violation behaviour.
 *
 * @param[in] mode (generate interrupt, warm reset, enter probe mode).
 * @param[in] flash controller.
 * @param[in] fpr_cb for interrupt mode (only).
 * @param[in] data user callback data for interrupt mode (only).
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_fpr_set_violation_policy(const qm_fpr_viol_mode_t mode,
				const qm_flash_t flash,
				qm_fpr_callback_t fpr_cb, void *data);

/**
 * @}
 */

#endif /* __QM_FPR_H__ */
