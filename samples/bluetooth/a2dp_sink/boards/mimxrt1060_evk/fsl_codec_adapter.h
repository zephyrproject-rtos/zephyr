/*
 * Copyright  2017- 2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_CODEC_ADAPTER_H_
#define _FSL_CODEC_ADAPTER_H_
#include "fsl_common.h"

#define CODEC_WM8960_ENABLE 1
#ifdef CODEC_WM8904_ENABLE
#include "fsl_wm8904_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_WM8904_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_WM8904_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_WM8904_HANDLER_SIZE
#endif
#endif /* CODEC_WM8904_ENABLE */

#ifdef CODEC_WM8960_ENABLE
#include "fsl_wm8960_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_WM8960_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_WM8960_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_WM8960_HANDLER_SIZE
#endif

#endif /* CODEC_WM8960_ENABLE */

#ifdef CODEC_WM8524_ENABLE
#include "fsl_wm8524_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_WM8524_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_WM8524_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_WM8524_HANDLER_SIZE
#endif

#endif /* CODEC_WM8524_ENABLE */

#ifdef CODEC_SGTL5000_ENABLE
#include "fsl_sgtl_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_SGTL_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_SGTL_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_SGTL_HANDLER_SIZE
#endif

#endif /* CODEC_SGTL5000_ENABLE */

#ifdef CODEC_DA7212_ENABLE
#include "fsl_da7212_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_DA7212_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_DA7212_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_DA7212_HANDLER_SIZE
#endif

#endif /* CODEC_DA7212_ENABLE */

#ifdef CODEC_CS42888_ENABLE
#include "fsl_cs42888_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_CS42888_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_CS42888_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_CS42888_HANDLER_SIZE
#endif

#endif /* CODEC_CS42888_ENABLE */

#ifdef CODEC_AK4497_ENABLE
#include "fsl_ak4497_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_AK4497_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_AK4497_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_AK4497_HANDLER_SIZE
#endif

#endif /* CODEC_AK4497_ENABLE */

#ifdef CODEC_AK4458_ENABLE
#include "fsl_ak4458_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_AK4458_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_AK4458_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_AK4458_HANDLER_SIZE
#endif

#endif /* CODEC_AK4458_ENABLE */

#ifdef CODEC_TFA9XXX_ENABLE
#include "fsl_tfa9xxx_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_TFA98XX_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_TFA98XX_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_TFA98XX_HANDLER_SIZE
#endif

#endif /* CODEC_TFA9XXX_ENABLE */

#ifdef CODEC_TFA9896_ENABLE
#include "fsl_tfa9896_adapter.h"

#if ((defined HAL_CODEC_HANDLER_SIZE) && (HAL_CODEC_HANDLER_SIZE < HAL_CODEC_TFA9896_HANDLER_SIZE))
#undef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_TFA9896_HANDLER_SIZE
#endif

#if (!(defined HAL_CODEC_HANDLER_SIZE))
#define HAL_CODEC_HANDLER_SIZE HAL_CODEC_TFA9896_HANDLER_SIZE
#endif

#endif /* CODEC_TFA9896_ENABLE */

#ifndef HAL_CODEC_HANDLER_SIZE
#define HAL_CODEC_HANDLER_SIZE 128U
#endif
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief codec type
 * @anchor _codec_type
 */
enum
{
    kCODEC_WM8904,   /*!< wm8904 */
    kCODEC_WM8960,   /*!< wm8960 */
    kCODEC_WM8524,   /*!< wm8524 */
    kCODEC_SGTL5000, /*!< sgtl5000 */
    kCODEC_DA7212,   /*!< da7212 */
    kCODEC_CS42888,  /*!< CS42888 */
    kCODEC_AK4497,   /*!< AK4497 */
    kCODEC_AK4458,   /*!< ak4458 */
    kCODEC_TFA9XXX,  /*!< tfa9xxx */
    kCODEC_TFA9896,  /*!< tfa9896 */
};
/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif
/*!
 * @brief Codec initilization.
 *
 * @param handle codec handle.
 * @param config codec configuration.
 * @return kStatus_Success is success, else initial failed.
 */
status_t HAL_CODEC_Init(void *handle, void *config);

/*!
 * @brief Codec de-initilization.
 *
 * @param handle codec handle.
 * @return kStatus_Success is success, else de-initial failed.
 */
status_t HAL_CODEC_Deinit(void *handle);

/*!
 * @brief set audio data format.
 *
 * @param handle codec handle.
 * @param mclk master clock frequency in HZ.
 * @param sampleRate sample rate in HZ.
 * @param bitWidth bit width.
 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_SetFormat(void *handle, uint32_t mclk, uint32_t sampleRate, uint32_t bitWidth);

/*!
 * @brief set audio codec module volume.
 *
 * @param handle codec handle.
 * @param playChannel audio codec play channel, can be a value or combine value of _codec_play_channel.
 * @param volume volume value, support 0 ~ 100, 0 is mute, 100 is the maximum volume value.
 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_SetVolume(void *handle, uint32_t playChannel, uint32_t volume);

/*!
 * @brief set audio codec module mute.
 *
 * @param handle codec handle.
 * @param playChannel audio codec play channel, can be a value or combine value of _codec_play_channel.
 * @param isMute true is mute, false is unmute.
 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_SetMute(void *handle, uint32_t playChannel, bool isMute);

/*!
 * @brief set audio codec module power.
 *
 * @param handle codec handle.
 * @param module audio codec module.
 * @param powerOn true is power on, false is power down.
 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_SetPower(void *handle, uint32_t module, bool powerOn);

/*!
 * @brief codec set record source.
 *
 * @param handle codec handle.
 * @param recordSource audio codec record source, can be a value or combine value of _codec_record_source.
 *
 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_SetRecord(void *handle, uint32_t recordSource);

/*!
 * @brief codec set record channel.
 *
 * @param handle codec handle.
 * @param leftRecordChannel audio codec record channel, reference _codec_record_channel, can be a value or combine value
 of member in _codec_record_channel.
 * @param rightRecordChannel audio codec record channel, reference _codec_record_channel, can be a value combine of
 member in _codec_record_channel.

 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_SetRecordChannel(void *handle, uint32_t leftRecordChannel, uint32_t rightRecordChannel);

/*!
 * @brief codec set play source.
 *
 * @param handle codec handle.
 * @param playSource audio codec play source, can be a value or combine value of _codec_play_source.
 *
 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_SetPlay(void *handle, uint32_t playSource);

/*!
 * @brief codec module control.
 *
 * This function is used for codec module control, support switch digital interface cmd, can be expand to support codec
 * module specific feature
 *
 * @param handle codec handle.
 * @param cmd module control cmd, reference _codec_module_ctrl_cmd.
 * @param data value to write, when cmd is kCODEC_ModuleRecordSourceChannel, the data should be a value combine
 *  of channel and source, please reference macro CODEC_MODULE_RECORD_SOURCE_CHANNEL(source, LP, LN, RP, RN), reference
 *  codec specific driver for detail configurations.
 * @return kStatus_Success is success, else configure failed.
 */
status_t HAL_CODEC_ModuleControl(void *handle, uint32_t cmd, uint32_t data);

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_CODEC_ADAPTER_H_ */
