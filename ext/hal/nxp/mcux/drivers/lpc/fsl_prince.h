/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_PRINCE_H_
#define _FSL_PRINCE_H_

#include "fsl_common.h"
#include "fsl_iap_ffr.h"
#include "fsl_puf.h"

/*!
 * @addtogroup prince
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief PRINCE driver version 2.0.0.
 *
 * Current version: 2.0.0
 *
 * Change log:
 * - Version 2.0.0
 *   - Initial version.
 */
#define FSL_PRINCE_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

#define FSL_PRINCE_DRIVER_SUBREGION_SIZE_IN_KB (8)

#if !defined(ALIGN_DOWN)
#define ALIGN_DOWN(x, a) ((x) & (uint32_t)(-((int32_t)(a))))
#endif

typedef enum _prince_region
{
    kPRINCE_Region0 = 0U, /*!< PRINCE region 0 */
    kPRINCE_Region1 = 1U, /*!< PRINCE region 1 */
    kPRINCE_Region2 = 2U, /*!< PRINCE region 2 */
} prince_region_t;

typedef enum _prince_lock
{
    kPRINCE_Region0Lock = 1U, /*!< PRINCE region 0 lock */
    kPRINCE_Region1Lock = 2U, /*!< PRINCE region 1 lock */
    kPRINCE_Region2Lock = 4U, /*!< PRINCE region 2 lock */
    kPRINCE_MaskLock = 256U,  /*!< PRINCE mask register lock */
} prince_lock_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Enable data encryption.
 *
 * This function enables PRINCE on-the-fly data encryption.
 *
 * @param base PRINCE peripheral address.
 */
static inline void PRINCE_EncryptEnable(PRINCE_Type *base)
{
    base->ENC_ENABLE = 1;
}

/*!
 * @brief Disable data encryption.
 *
 * This function disables PRINCE on-the-fly data encryption.
 *
 * @param base PRINCE peripheral address.
 */
static inline void PRINCE_EncryptDisable(PRINCE_Type *base)
{
    base->ENC_ENABLE = 0;
}

/*!
 * @brief Sets PRINCE data mask.
 *
 * This function sets the PRINCE mask that is used to mask decrypted data.
 *
 * @param base PRINCE peripheral address.
 * @param mask 64-bit data mask value.
 */
static inline void PRINCE_SetMask(PRINCE_Type *base, uint64_t mask)
{
    base->MASK_LSB = mask & 0xffffffffu;
    base->MASK_MSB = mask >> 32u;
}

/*!
 * @brief Locks access for specified region registers or data mask register.
 *
 * This function sets lock on specified region registers or mask register.
 *
 * @param base PRINCE peripheral address.
 * @param lock registers to lock. This is a logical OR of members of the
 *             enumeration ::prince_lock_t
 */
static inline void PRINCE_SetLock(PRINCE_Type *base, uint32_t lock)
{
    base->LOCK = lock & 0x1ffu;
}

/*!
 * @brief Generate new IV code.
 *
 * This function generates new IV code and stores it into the persistent memory.
 * This function is implemented as a wrapper of the exported ROM bootloader API.
 * Ensure about 800 bytes free space on the stack when calling this routine with the store parameter set to true!
 *
 * @param region PRINCE region index.
 * @param iv_code IV code pointer used for storing the newly generated 52 bytes long IV code.
 * @param store flag to allow storing the newly generated IV code into the persistent memory (FFR).
 * param flash_context pointer to the flash driver context structure.
 *
 * @return kStatus_Success upon success
 * @return kStatus_Fail    otherwise, kStatus_Fail is also returned if the key code for the particular
 *                         PRINCE region is not present in the keystore (though new IV code has been provided)
 */
status_t PRINCE_GenNewIV(prince_region_t region, uint8_t *iv_code, bool store, flash_config_t *flash_context);

/*!
 * @brief Load IV code.
 *
 * This function enables IV code loading into the PRINCE bus encryption engine.
 * This function is implemented as a wrapper of the exported ROM bootloader API.
 *
 * @param region PRINCE region index.
 * @param iv_code IV code pointer used for passing the IV code.
 *
 * @return kStatus_Success upon success
 * @return kStatus_Fail    otherwise
 */
status_t PRINCE_LoadIV(prince_region_t region, uint8_t *iv_code);

/*!
 * @brief Allow encryption/decryption for specified address range.
 *
 * This function sets the encryption/decryption for specified address range.
 * This function is implemented as a wrapper of the exported ROM bootloader API.
 * Ensure about 800 bytes free space on the stack when calling this routine!
 *
 * @param region PRINCE region index.
 * @param start_address start address of the area to be encrypted/decrypted.
 * @param length length of the area to be encrypted/decrypted.
 * param flash_context pointer to the flash driver context structure.
 *
 * @return kStatus_Success upon success
 * @return kStatus_Fail    otherwise
 */
status_t PRINCE_SetEncryptForAddressRange(prince_region_t region,
                                          uint32_t start_address,
                                          uint32_t length,
                                          flash_config_t *flash_context);

/*!
 * @brief Gets the PRINCE Sub-Region Enable register.
 *
 * This function gets PRINCE SR_ENABLE register.
 *
 * @param base PRINCE peripheral address.
 * @param region PRINCE region index.
 * @param sr_enable Sub-Region Enable register pointer.
 *
 * @return kStatus_Success upon success
 * @return kStatus_InvalidArgument
 */
status_t PRINCE_GetRegionSREnable(PRINCE_Type *base, prince_region_t region, uint32_t *sr_enable);

/*!
 * @brief Gets the PRINCE region base address register.
 *
 * This function gets PRINCE BASE_ADDR register.
 *
 * @param base PRINCE peripheral address.
 * @param region PRINCE region index.
 * @param region_base_addr Region base address pointer.
 *
 * @return kStatus_Success upon success
 * @return kStatus_InvalidArgument
 */
status_t PRINCE_GetRegionBaseAddress(PRINCE_Type *base, prince_region_t region, uint32_t *region_base_addr);

/*!
 * @brief Sets the PRINCE region IV.
 *
 * This function sets specified AES IV for the given region.
 *
 * @param base PRINCE peripheral address.
 * @param region Selection of the PRINCE region to be configured.
 * @param iv 64-bit AES IV in little-endian byte order.
 */
status_t PRINCE_SetRegionIV(PRINCE_Type *base, prince_region_t region, const uint8_t iv[8]);

/*!
 * @brief Sets the PRINCE region base address.
 *
 * This function configures PRINCE region base address.
 *
 * @param base PRINCE peripheral address.
 * @param region Selection of the PRINCE region to be configured.
 * @param region_base_addr Base Address for region.
 */
status_t PRINCE_SetRegionBaseAddress(PRINCE_Type *base, prince_region_t region, uint32_t region_base_addr);

/*!
 * @brief Sets the PRINCE Sub-Region Enable register.
 *
 * This function configures PRINCE SR_ENABLE register.
 *
 * @param base PRINCE peripheral address.
 * @param region Selection of the PRINCE region to be configured.
 * @param sr_enable Sub-Region Enable register value.
 */
status_t PRINCE_SetRegionSREnable(PRINCE_Type *base, prince_region_t region, uint32_t sr_enable);

#if defined(__cplusplus)
}
#endif

/*!
 *@}
 */

#endif /* _FSL_PRINCE_H_ */
