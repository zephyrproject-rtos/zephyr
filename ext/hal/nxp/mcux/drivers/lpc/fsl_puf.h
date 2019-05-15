/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PUF_H_
#define _PUF_H_

#include <stddef.h>
#include <stdint.h>

#include "fsl_common.h"

typedef enum _puf_key_index_register
{
    kPUF_KeyIndex_00 = 0x00U,
    kPUF_KeyIndex_01 = 0x01U,
    kPUF_KeyIndex_02 = 0x02U,
    kPUF_KeyIndex_03 = 0x03U,
    kPUF_KeyIndex_04 = 0x04U,
    kPUF_KeyIndex_05 = 0x05U,
    kPUF_KeyIndex_06 = 0x06U,
    kPUF_KeyIndex_07 = 0x07U,
    kPUF_KeyIndex_08 = 0x08U,
    kPUF_KeyIndex_09 = 0x09U,
    kPUF_KeyIndex_10 = 0x0AU,
    kPUF_KeyIndex_11 = 0x0BU,
    kPUF_KeyIndex_12 = 0x0CU,
    kPUF_KeyIndex_13 = 0x0DU,
    kPUF_KeyIndex_14 = 0x0EU,
    kPUF_KeyIndex_15 = 0x0FU,
} puf_key_index_register_t;

typedef enum _puf_min_max
{
    kPUF_KeySizeMin = 8u,
    kPUF_KeySizeMax = 512u,
    kPUF_KeyIndexMax = kPUF_KeyIndex_15,
} puf_min_max_t;

typedef enum _puf_key_slot
{
    kPUF_KeySlot0 = 0U, /*!< PUF key slot 0 */
    kPUF_KeySlot1 = 1U, /*!< PUF key slot 1 */
#if defined(FSL_FEATURE_PUF_HAS_KEYSLOTS) && (FSL_FEATURE_PUF_HAS_KEYSLOTS > 2)
    kPUF_KeySlot2 = 2U, /*!< PUF key slot 2 */
    kPUF_KeySlot3 = 3U, /*!< PUF key slot 3 */
#endif
} puf_key_slot_t;

/*! @brief Get Key Code size in bytes from key size in bytes at compile time. */
#define PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(x) ((160u + ((((x << 3) + 255u) >> 8) << 8)) >> 3)
#define PUF_MIN_KEY_CODE_SIZE PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(8)
#define PUF_ACTIVATION_CODE_SIZE 1192
/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @brief Initialize PUF
 *
 * This function enables power to PUF block and waits until the block initializes.
 *
 * @param base PUF peripheral base address
 * @param dischargeTimeMsec time in ms to wait for PUF SRAM to fully discharge
 * @param coreClockFrequencyHz core clock frequency in Hz
 * @return Status of the init operation
 */
status_t PUF_Init(PUF_Type *base, uint32_t dischargeTimeMsec, uint32_t coreClockFrequencyHz);

/*!
 * @brief Denitialize PUF
 *
 * This function disables power to PUF SRAM and peripheral clock.
 *
 * @param base PUF peripheral base address
 * @param dischargeTimeMsec time in ms to wait for PUF SRAM to fully discharge
 * @param coreClockFrequencyHz core clock frequency in Hz
 */
void PUF_Deinit(PUF_Type *base, uint32_t dischargeTimeMsec, uint32_t coreClockFrequencyHz);

/*!
 * @brief Enroll PUF
 *
 * This function derives a digital fingerprint, generates the corresponding Activation Code (AC)
 * and returns it to be stored in an NVM or a file. This step needs to be
 * performed only once for each device. This function may be permanently disallowed by a fuse.
 *
 * @param base PUF peripheral base address
 * @param[out] activationCode Word aligned address of the resulting activation code.
 * @param activationCodeSize Size of the activationCode buffer in bytes. Shall be 1192 bytes.
 * @return Status of enroll operation.
 */
status_t PUF_Enroll(PUF_Type *base, uint8_t *activationCode, size_t activationCodeSize);

/*!
 * @brief Start PUF
 *
 * The Activation Code generated during the Enroll operation is used to
 * reconstruct the digital fingerprint. This needs to be done after every power-up
 * and reset.
 *
 * @param base PUF peripheral base address
 * @param activationCode Word aligned address of the input activation code.
 * @param activationCodeSize Size of the activationCode buffer in bytes. Shall be 1192 bytes.
 * @return Status of start operation.
 */
status_t PUF_Start(PUF_Type *base, const uint8_t *activationCode, size_t activationCodeSize);

/*!
 * @brief Set intrinsic key
 *
 * The digital fingerprint generated during the Enroll/Start
 * operations is used to generate a Key Code (KC) that defines a unique intrinsic
 * key. This KC is returned to be stored in an NVM or a file. This operation
 * needs to be done only once for each intrinsic key.
 * Each time a Set Intrinsic Key operation is executed a new unique key is
 * generated.
 *
 * @param base PUF peripheral base address
 * @param keyIndex PUF key index register
 * @param keySize Size of the intrinsic key to generate in bytes.
 * @param[out] keyCode Word aligned address of the resulting key code.
 * @param keyCodeSize Size of the keyCode buffer in bytes. Shall be PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(keySize).
 * @return Status of set intrinsic key operation.
 */
status_t PUF_SetIntrinsicKey(
    PUF_Type *base, puf_key_index_register_t keyIndex, size_t keySize, uint8_t *keyCode, size_t keyCodeSize);

/*!
 * @brief Set user key
 *
 * The digital fingerprint generated during the Enroll/Start
 * operations and a user key (UK) provided as input are used to
 * generate a Key Code (KC). This KC is sent returned to be stored
 * in an NVM or a file. This operation needs to be done only once for each user key.
 *
 * @param base PUF peripheral base address
 * @param keyIndex PUF key index register
 * @param userKey Word aligned address of input user key.
 * @param userKeySize Size of the input user key in bytes.
 * @param[out] keyCode Word aligned address of the resulting key code.
 * @param keyCodeSize Size of the keyCode buffer in bytes. Shall be PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(userKeySize).
 * @return Status of set user key operation.
 */
status_t PUF_SetUserKey(PUF_Type *base,
                        puf_key_index_register_t keyIndex,
                        const uint8_t *userKey,
                        size_t userKeySize,
                        uint8_t *keyCode,
                        size_t keyCodeSize);

/*!
 * @brief Reconstruct key from a key code
 *
 * The digital fingerprint generated during the Start operation and the KC
 * generated during a Set Key operation (Set intrinsic key or Set user key) are used to retrieve a stored key. This
 * operation needs to be done every time a key is needed.
 * This function accepts only Key Codes created for PUF index registers kPUF_KeyIndex_01 to kPUF_KeyIndex_15.
 *
 * @param base PUF peripheral base address
 * @param keyCode Word aligned address of the input key code.
 * @param keyCodeSize Size of the keyCode buffer in bytes. Shall be PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(keySize).
 * @param[out] key Word aligned address of output key.
 * @param keySize Size of the output key in bytes.
 * @return Status of get key operation.
 */
status_t PUF_GetKey(PUF_Type *base, const uint8_t *keyCode, size_t keyCodeSize, uint8_t *key, size_t keySize);

/*!
 * @brief Reconstruct hw bus key from a key code
 *
 * The digital fingerprint generated during the Start operation and the KC
 * generated during a Set Key operation (Set intrinsic key or Set user key) are used to retrieve a stored key. This
 * operation needs to be done every time a key is needed.
 * This function accepts only Key Codes created for PUF index register kPUF_KeyIndex_00.
 * Such a key is output directly to a dedicated hardware bus. The reconstructed key is not exposed to system memory.
 *
 * @param base PUF peripheral base address
 * @param keyCode Word aligned address of the input key code.
 * @param keyCodeSize Size of the keyCode buffer in bytes. Shall be PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(keySize).
 * @param keySlot key slot to output on hw bus. Parameter is ignored on devices with less than two key slots.
 * @param keyMask key masking value. Shall be random for each POR/reset. Value does not have to be cryptographicaly
 * secure.
 * @return Status of get key operation.
 */
status_t PUF_GetHwKey(
    PUF_Type *base, const uint8_t *keyCode, size_t keyCodeSize, puf_key_slot_t keySlot, uint32_t keyMask);

/*!
 * @brief Zeroize PUF
 *
 * This function clears all PUF internal logic and puts the PUF to error state.
 *
 * @param base PUF peripheral base address
 * @return Status of the zeroize operation.
 */
status_t PUF_Zeroize(PUF_Type *base);

/*!
 * @brief Checks if Get Key operation is allowed.
 *
 * This function returns true if get key operation is allowed.
 *
 * @param base PUF peripheral base address
 * @return true if get key operation is allowed
 */
bool PUF_IsGetKeyAllowed(PUF_Type *base);

static inline void PUF_BlockSetKey(PUF_Type *base)
{
    base->CFG |= PUF_CFG_BLOCKKEYOUTPUT_MASK; /* block set key */
}

static inline void PUF_BlockEnroll(PUF_Type *base)
{
    base->CFG |= PUF_CFG_BLOCKENROLL_SETKEY_MASK; /* block enroll */
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _PUF_H_ */
