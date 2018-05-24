/*
 * The Clear BSD License
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSL_BEE_H_
#define _FSL_BEE_H_

#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief BEE driver version. Version 2.0.0.
 *
 * Current version: 2.0.0
 *
 * Change log:
 * - Version 2.0.0
 *   - Initial version
 */
#define FSL_BEE_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

typedef enum _bee_aes_mode
{
    kBEE_AesEcbMode = 0U, /*!< AES ECB Mode */
    kBEE_AesCtrMode = 1U  /*!< AES CTR Mode */
} bee_aes_mode_t;

typedef enum _bee_region
{
    kBEE_Region0 = 0U, /*!< BEE region 0 */
    kBEE_Region1 = 1U  /*!< BEE region 1 */
} bee_region_t;

typedef enum _bee_region_enable
{
    kBEE_RegionDisabled = 0U, /*!< BEE region disabled */
    kBEE_RegionEnabled = 1U   /*!< BEE region enabled */
} bee_region_enable_t;

typedef enum _bee_status_flags
{
    kBEE_DisableAbortFlag = 1U,              /*!< Disable abort flag. */
    kBEE_Reg0ReadSecViolation = 2U,          /*!< Region-0 read channel security violation */
    kBEE_ReadIllegalAccess = 4U,             /*!< Read channel illegal access detected */
    kBEE_Reg1ReadSecViolation = 8U,          /*!< Region-1 read channel security violation */
    kBEE_Reg0AccessViolation = 16U,          /*!< Protected region-0 access violation */
    kBEE_Reg1AccessViolation = 32U,          /*!< Protected region-1 access violation */
    kBEE_IdleFlag = BEE_STATUS_BEE_IDLE_MASK /*!< Idle flag */
} bee_status_flags_t;

/*! @brief BEE region configuration structure. */
typedef struct _bee_region_config
{
    bee_aes_mode_t mode;          /*!< AES mode used for encryption/decryption */
    uint32_t regionBot;           /*!< Region bottom address */
    uint32_t regionTop;           /*!< Region top address */
    uint32_t addrOffset;          /*!< Region address offset */
    bee_region_enable_t regionEn; /*!< Region enable/disable */
} bee_region_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Resets BEE module to factory default values.
 *
 * This function performs hardware reset of BEE module. Attributes and keys from software for both regions are cleared.
 *
 * @param base BEE peripheral address.
 */
void BEE_Init(BEE_Type *base);

/*!
 * @brief Resets BEE module, clears keys for both regions and disables clock to the BEE.
 *
 * This function performs hardware reset of BEE module and disables clocks. Attributes and keys from software for both
 * regions are cleared.
 *
 * @param base BEE peripheral address.
 */
void BEE_Deinit(BEE_Type *base);

/*!
 * @brief Enables BEE decryption.
 *
 * This function enables decryption using BEE.
 *
 * @param base BEE peripheral address.
 */
static inline void BEE_Enable(BEE_Type *base)
{
    base->CTRL |= BEE_CTRL_BEE_ENABLE_MASK | BEE_CTRL_KEY_VALID_MASK;
}

/*!
 * @brief Disables BEE decryption.
 *
 * This function disables decryption using BEE.
 *
 * @param base BEE peripheral address.
 */
static inline void BEE_Disable(BEE_Type *base)
{
    base->CTRL &= ~BEE_CTRL_BEE_ENABLE_MASK | BEE_CTRL_KEY_VALID_MASK;
}

/*!
 * @brief Loads default values to the BEE region configuration structure.
 *
 * Loads default values to the BEE region configuration structure. The default values are as follows:
 * @code
 *   config->mode = kBEE_AesCbcMode;
 *   config->regionBot = 0U;
 *   config->regionTop = 0U;
 *   config->addrOffset = 0xF0000000U;
 *   config->regionEn = kBEE_RegionDisabled;
 * @endcode
 *
 * @param config Configuration structure for BEE region.
 */
void BEE_GetDefaultConfig(bee_region_config_t *config);

/*!
 * @brief Sets BEE region configuration.
 *
 * This function sets BEE region settings accorging to given configuration structure.
 *
 * @param base BEE peripheral address.
 * @param region Selection of the BEE region to be configured.
 * @param config Configuration structure for BEE region.
 */
status_t BEE_SetRegionConfig(BEE_Type *base, bee_region_t region, const bee_region_config_t *config);

/*!
 * @brief Loads the AES key and nonce for selected region into BEE key registers.
 *
 * This function loads given AES key and nonce(only AES CTR mode) to BEE register for the given region.
 *
 * Please note, that eFuse BEE_KEYx_SEL must be set accordingly to be able to load and use key loaded in BEE registers.
 * Otherwise, key cannot loaded and BEE will use key from OTPMK or SW_GP2.
 *
 * @param base BEE peripheral address.
 * @param region Selection of the BEE region to be configured.
 * @param key AES key.
 * @param keySize Size of AES key.
 * @param nonce AES nonce.
 * @param nonceSize Size of AES nonce.
 */
status_t BEE_SetRegionKey(
    BEE_Type *base, bee_region_t region, const uint8_t *key, size_t keySize, const uint8_t *nonce, size_t nonceSize);

/*!
 * @brief Gets the BEE status flags.
 *
 * This function returns status of BEE peripheral.
 *
 * @param base BEE peripheral address.
 *
 * @return The status flags. This is the logical OR of members of the
 *         enumeration ::bee_status_flags_t
 */
uint32_t BEE_GetStatusFlags(BEE_Type *base);

/*!
 * @brief Clears the BEE status flags.
 *
 * @param base BEE peripheral base address.
 * @param mask The status flags to clear. This is a logical OR of members of the
 *             enumeration ::bee_status_flags_t
 */
void BEE_ClearStatusFlags(BEE_Type *base, uint32_t mask);

/*!
 * @brief Computes offset to be set for specifed memory location.
 *
 * This function calculates offset that must be set for BEE region to access physical memory location.
 *
 * @param addressMemory Address of physical memory location.
 */
static inline uint32_t BEE_GetOffset(uint32_t addressMemory)
{
    return (addressMemory >> 16);
}

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_BEE_H_ */
