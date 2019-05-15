/*
 * Copyright (c) 2018, NXP Semiconductors, Inc.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_SYSCTL_H_
#define _FSL_SYSCTL_H_

#include "fsl_common.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @addtogroup sysctl
 * @{
 */

/*! @file */
/*! @file fsl_sysctl.h */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief Group sysctl driver version for SDK */
#define FSL_SYSCTL_DRIVER_VERSION (MAKE_VERSION(2, 0, 1)) /*!< Version 2.0.1. */
/*@}*/

/*! @brief SYSCTL share set*/
enum _sysctl_share_set_index
{
    kSYSCTL_ShareSet0 = 0, /*!< share set 0 */
    kSYSCTL_ShareSet1 = 1, /*!< share set 1 */
};

/*! @brief SYSCTL flexcomm signal */
typedef enum _sysctl_fcctrlsel_signal
{
    kSYSCTL_FlexcommSignalSCK = SYSCTL_FCCTRLSEL_SCKINSEL_SHIFT,       /*!< SCK signal */
    kSYSCTL_FlexcommSignalWS = SYSCTL_FCCTRLSEL_WSINSEL_SHIFT,         /*!< WS signal */
    kSYSCTL_FlexcommSignalDataIn = SYSCTL_FCCTRLSEL_DATAINSEL_SHIFT,   /*!< Data in signal */
    kSYSCTL_FlexcommSignalDataOut = SYSCTL_FCCTRLSEL_DATAOUTSEL_SHIFT, /*!< Data out signal */
} sysctl_fcctrlsel_signal_t;

/*! @brief SYSCTL flexcomm index*/
enum _sysctl_share_src
{
    kSYSCTL_Flexcomm0 = 0, /*!< share set 0 */
    kSYSCTL_Flexcomm1 = 1, /*!< share set 1 */
    kSYSCTL_Flexcomm2 = 2, /*!< share set 2 */
    kSYSCTL_Flexcomm4 = 4, /*!< share set 4 */
    kSYSCTL_Flexcomm5 = 5, /*!< share set 5 */
    kSYSCTL_Flexcomm6 = 6, /*!< share set 6 */
    kSYSCTL_Flexcomm7 = 7, /*!< share set 7 */
};

/*! @brief SYSCTL shared data out mask */
enum _sysctl_dataout_mask
{
    kSYSCTL_Flexcomm0DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC0DATAOUTEN_MASK, /*!< share set 0 */
    kSYSCTL_Flexcomm1DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC1DATAOUTEN_MASK, /*!< share set 1 */
    kSYSCTL_Flexcomm2DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_F20DATAOUTEN_MASK, /*!< share set 2 */
    kSYSCTL_Flexcomm3DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC3DATAOUTEN_MASK, /*!< share set 3 */
    kSYSCTL_Flexcomm4DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC4DATAOUTEN_MASK, /*!< share set 4 */
    kSYSCTL_Flexcomm5DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC5DATAOUTEN_MASK, /*!< share set 5 */
    kSYSCTL_Flexcomm6DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC6DATAOUTEN_MASK, /*!< share set 6 */
    kSYSCTL_Flexcomm7DataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC7DATAOUTEN_MASK, /*!< share set 7 */
};

/*! @brief SYSCTL flexcomm signal */
typedef enum _sysctl_sharedctrlset_signal
{
    kSYSCTL_SharedCtrlSignalSCK = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDSCKSEL_SHIFT,     /*!< SCK signal */
    kSYSCTL_SharedCtrlSignalWS = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDWSSEL_SHIFT,       /*!< WS signal */
    kSYSCTL_SharedCtrlSignalDataIn = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_SHAREDDATASEL_SHIFT, /*!< Data in signal */
    kSYSCTL_SharedCtrlSignalDataOut = SYSCTL_SHARECTRLSET_SHAREDCTRLSET_FC0DATAOUTEN_SHIFT, /*!< Data out signal */
} sysctl_sharedctrlset_signal_t;
/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief SYSCTL initial
 *
 * @param base Base address of the SYSCTL peripheral.
 */
void SYSCTL_Init(SYSCTL_Type *base);

/*!
 * @brief SYSCTL deinit
 *
 * @param base Base address of the SYSCTL peripheral.
 */
void SYSCTL_Deinit(SYSCTL_Type *base);

/* @} */

/*!
 * @name SYSCTL share signal configure
 * @{
 */

/*!
 * @brief SYSCTL share set configure for flexcomm
 *
 * @param base Base address of the SYSCTL peripheral.
 * @param flexCommIndex index of flexcomm, reference _sysctl_share_src
 * @param sckSet share set for sck,reference _sysctl_share_set_index
 * @param wsSet share set for ws, reference _sysctl_share_set_index
 * @param dataInSet share set for data in, reference _sysctl_share_set_index
 * @param dataOutSet share set for data out, reference _sysctl_share_set_index
 *
 */
void SYSCTL_SetFlexcommShareSet(SYSCTL_Type *base,
                                uint32_t flexCommIndex,
                                uint32_t sckSet,
                                uint32_t wsSet,
                                uint32_t dataInSet,
                                uint32_t dataOutSet);

/*!
 * @brief SYSCTL share set configure for separate signal
 *
 * @param base Base address of the SYSCTL peripheral
 * @param flexCommIndex index of flexcomm,reference _sysctl_share_src
 * @param signal FCCTRLSEL signal shift
 * @param setIndex share set for sck, reference _sysctl_share_set_index
 *
 */
void SYSCTL_SetShareSet(SYSCTL_Type *base, uint32_t flexCommIndex, sysctl_fcctrlsel_signal_t signal, uint32_t set);

/*!
 * @brief SYSCTL share set source configure
 *
 * @param base Base address of the SYSCTL peripheral
 * @param setIndex index of share set, reference _sysctl_share_set_index
 * @param sckShareSrc sck source fro this share set,reference _sysctl_share_src
 * @param wsShareSrc ws source fro this share set,reference _sysctl_share_src
 * @param dataInShareSrc data in source fro this share set,reference _sysctl_share_src
 * @param dataOutShareSrc data out source fro this share set,reference _sysctl_share_src
 *
 */
void SYSCTL_SetShareSetSrc(SYSCTL_Type *base,
                           uint32_t setIndex,
                           uint32_t sckShareSrc,
                           uint32_t wsShareSrc,
                           uint32_t dataInShareSrc,
                           uint32_t dataOutShareSrc);

/*!
 * @brief SYSCTL sck source configure
 *
 * @param base Base address of the SYSCTL peripheral
 * @param setIndex index of share set, reference _sysctl_share_set_index
 * @param sckShareSrc sck source fro this share set,reference _sysctl_share_src
 *
 */
void SYSCTL_SetShareSignalSrc(SYSCTL_Type *base,
                              uint32_t setIndex,
                              sysctl_sharedctrlset_signal_t signal,
                              uint32_t shareSrc);

/* @} */

#ifdef __cplusplus
}
#endif

/*@}*/

#endif /* _FSL_SYSCTL_H_ */
