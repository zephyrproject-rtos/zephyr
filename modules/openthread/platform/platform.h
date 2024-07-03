/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes the platform-specific initializers.
 *
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <stdint.h>
#include <openthread/config.h>
#include <openthread/instance.h>
#include <openthread-system.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OT_PLAT_DBG_LVL 0

#define OT_PLAT_DBG_LEVEL_NONE 0
#define OT_PLAT_DBG_LEVEL_ERR 1
#define OT_PLAT_DBG_LEVEL_WARNING 2
#define OT_PLAT_DBG_LEVEL_INFO 3
#define OT_PLAT_DBG_LEVEL_DEBUG 4

#ifndef OT_PLAT_DBG_LVL
#define OT_PLAT_DBG_LVL OT_PLAT_DBG_LEVEL_NONE
#endif

#if (OT_PLAT_DBG_LVL > OT_PLAT_DBG_LEVEL_NONE)
/* if OT_STACK_ENABLE_LOG the log module will be already enabled */
#ifndef OT_STACK_ENABLE_LOG
#define LOG_ENABLE 1
#define LOG_ENABLE_TIMESTAMP 1
#include "fsl_component_log.h"
#ifndef FREERTOS_LOG
#define LOG_MODULE_NAME ot_plat_log
LOG_MODULE_DEFINE(LOG_MODULE_NAME, kLOG_LevelDebug);
#endif
#endif
#endif

#if (OT_PLAT_DBG_LVL >= OT_PLAT_DBG_LEVEL_DEBUG)
#define OT_PLAT_DBG(fmt, ...) LOG_DBG("%s " fmt, __func__, ##__VA_ARGS__)
#define OT_PLAT_DBG_NO_FUNC(fmt, ...) LOG_DBG("%s " fmt, "", ##__VA_ARGS__)
#else
#define OT_PLAT_DBG(...)
#define OT_PLAT_DBG_NO_FUNC(...)
#endif
#if (OT_PLAT_DBG_LVL >= OT_PLAT_DBG_LEVEL_INFO)
#define OT_PLAT_INFO(fmt, ...) LOG_INF("%s " fmt, __func__, ##__VA_ARGS__)
#else
#define OT_PLAT_INFO(...)
#endif
#if (OT_PLAT_DBG_LVL >= OT_PLAT_DBG_LEVEL_WARNING)
#define OT_PLAT_WARN(fmt, ...) LOG_WRN("%s " fmt, __func__, ##__VA_ARGS__)
#else
#define OT_PLAT_WARN(...)
#endif
#if (OT_PLAT_DBG_LVL >= OT_PLAT_DBG_LEVEL_ERR)
#define OT_PLAT_ERR(fmt, ...) LOG_ERR("%s " fmt, __func__, ##__VA_ARGS__)
#else
#define OT_PLAT_ERR(...)
#endif

/**
 * This structure represents different CCA mode configurations before Tx.
 * CCA Mode type [CCA1=0x01,
				  CCA2=0x02,
				  CCA3=0x03[CCA1 AND CCA2],
				  CCA3=4[CCA1 OR CCA2],
				  NoCCA=0xFF]
 */
typedef struct otCCAModeConfig_tag {
	uint8_t mCcaMode;			 /* CCA Mode type */
	uint8_t mCca1Threshold;		 /* Energy threshold for CCA Mode1 */
	uint8_t mCca2CorrThreshold;	 /* CCA Mode 2 Correlation Threshold */
	uint8_t mCca2MinNumOfCorrTh; /* CCA Mode 2 Threshold Number of Correlation Peaks */
} otCCAModeConfig;

/**
 * This function initializes the alarm service used by OpenThread.
 *
 */
void otPlatAlarmInit(void);

/**
 * This function deinitialized the alarm service used by OpenThread.
 *
 */
void otPlatAlarmDeinit(void);

/**
 * This function performs alarm driver processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void otPlatAlarmProcess(otInstance *aInstance);

/**
 * This function initializes the radio service used by OpenThread.
 *
 */
void otPlatRadioInit(void);

/**
 * Allows to initialize the platform spinel interface
 * Only used in HOST+RCP configurations
 *
 */
void otPlatRadioInitSpinelInterface(void);

/**
 * This function deinitializes the radio service used by OpenThread.
 *
 */
void otPlatRadioDeinit(void);

/**
 * This function reset the radio service used by OpenThread.
 *
 */
otError otPlatResetOt(void);

/**
 * This function process radio events.
 *
 */
void otPlatRadioProcess(const otInstance *aInstance);

/**
 * This function process cli event.
 */
void otPlatCliUartProcess(void);

/**
 * This function initializes the ramdom service used by OpenThread
 */
void otPlatRandomInit(void);

/**
 * This function deinitializes the ramdom service used by OpenThread
 */
void otPlatRandomDeinit(void);

/**
 * Init the logging component
 */
void otPlatLogInit(void);

/**
 * Save settings in flash while in Idle
 */
void otPlatSaveSettingsIdle(void);

/**
 * This function performs a software reset on the platform, if otPlatReset() function
 *  was previously called.
 */
void otPlatResetIdle(void);

/**
 * This function allows to send spinel set prop vendor cmd with uint8_t value to be set.
 */
otError otPlatRadioSendSetPropVendorUint8Cmd(uint32_t aKey, uint8_t value);

/**
 * This function allows to receive spinel get prop vendor cmd with uint8_t *value to be get.
 */
otError otPlatRadioSendGetPropVendorUint8Cmd(uint32_t aKey, uint8_t *Value);

/**
 * This function allows to send spinel set prop vendor cmd with uint16_t value to be set.
 */
otError otPlatRadioSendSetPropVendorUint16Cmd(uint32_t aKey, uint16_t value);

/**
 * This function allows to receive spinel get prop vendor cmd with uint16_t *value to be get.
 */
otError otPlatRadioSendGetPropVendorUint16Cmd(uint32_t aKey, uint16_t *value);

/**
 * This function allows to send spinel set prop vendor cmd with uint32_t value to be set.
 */
otError otPlatRadioSendSetPropVendorUint32Cmd(uint32_t aKey, uint32_t value);

/**
 * This function allows to receive spinel get prop vendor cmd with uint32_t *value to be get.
 */
otError otPlatRadioSendGetPropVendorUint32Cmd(uint32_t aKey, uint32_t *value);

/**
 * This function allows to send spinel set prop vendor cmd with uint64_t value to be set.
 */
otError otPlatRadioSendSetPropVendorUint64Cmd(uint32_t aKey, uint64_t value);

/**
 * This function allows to receive spinel get prop vendor cmd with uint64_t *value to be get.
 */
otError otPlatRadioSendGetPropVendorUint64Cmd(uint32_t aKey, uint64_t *value);

/**
 * This function allows to send spinel get prop vendor cmd.
 *
 */
otError otPlatRadioSendGetPropVendorCmd(uint32_t aKey, const char *fwVersion, uint8_t fwVersionLen);

/**
 * Allows to set the UART instance for the ot cli
 */
void otPlatUartSetInstance(uint8_t newInstance);

/**
 * This function sends Vendor specific Manufactring commands to configure transceiver
 *
 */
otError otPlatRadioMfgCommand(otInstance   *aInstance,
							  uint32_t      aKey,
							  uint8_t      *payload,
							  const uint8_t payloadLenIn,
							  uint8_t      *payloadLenOut);

/**
 * This function sends a Vendor specific command to configure the CCA
 *
 */
otError otPlatRadioCcaConfigValue(uint32_t aKey, otCCAModeConfig *aCcaConfig, uint8_t aSetValue);

/**
 * This function is called from idle hook, likely for system idle operation such as flash operations
 *
 */
void otSysRunIdleTask(void);

/**
 * This function displays SPI diagnostic statistics on OT CLI
 *
 */
void otPlatRadioSpiDiag(void);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* PLATFORM_H_ */
