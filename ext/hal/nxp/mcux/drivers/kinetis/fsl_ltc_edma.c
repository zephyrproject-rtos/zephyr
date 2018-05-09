/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
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

#include "fsl_ltc_edma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*<! Structure definition for ltc_edma_private_handle_t. The structure is private. */
typedef struct _ltc_edma_private_handle
{
    LTC_Type *base;
    ltc_edma_handle_t *handle;
} ltc_edma_private_handle_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*<! Private handle only used for internally. */
static ltc_edma_private_handle_t s_edmaPrivateHandle[FSL_FEATURE_SOC_LTC_COUNT];

/* Array of LTC peripheral base address. */
static LTC_Type *const s_ltcBase[] = LTC_BASE_PTRS;

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* State machine state.*/
#define LTC_SM_STATE_START 0x0000u
#define LTC_SM_STATE_FINISH 0xFFFFu

/*! Full word representing the actual bit values for the LTC mode register. */
typedef uint32_t ltc_mode_t;

#define LTC_FIFO_SZ_MAX_DOWN_ALGN (0xff0u)
#define LTC_MD_ALG_AES (0x10U)        /*!< Bit field value for LTC_MD_ALG: AES */
#define LTC_MD_ALG_DES (0x20U)        /*!< Bit field value for LTC_MD_ALG: DES */
#define LTC_MD_ALG_TRIPLE_DES (0x21U) /*!< Bit field value for LTC_MD_ALG: 3DES */
#define LTC_MDPK_ALG_PKHA (0x80U)     /*!< Bit field value for LTC_MDPK_ALG: PKHA */
#define LTC_MD_ENC_DECRYPT (0U)       /*!< Bit field value for LTC_MD_ENC: Decrypt. */
#define LTC_MD_ENC_ENCRYPT (0x1U)     /*!< Bit field value for LTC_MD_ENC: Encrypt. */
#define LTC_MD_AS_UPDATE (0U)         /*!< Bit field value for LTC_MD_AS: Update */
#define LTC_MD_AS_INITIALIZE (0x1U)   /*!< Bit field value for LTC_MD_AS: Initialize */
#define LTC_MD_AS_FINALIZE (0x2U)     /*!< Bit field value for LTC_MD_AS: Finalize */
#define LTC_MD_AS_INIT_FINAL (0x3U)   /*!< Bit field value for LTC_MD_AS: Initialize/Finalize */

enum _ltc_md_dk_bit_shift
{
    kLTC_ModeRegBitShiftDK = 12U,
};

typedef enum _ltc_algorithm
{
    kLTC_AlgorithmAES = LTC_MD_ALG_AES << LTC_MD_ALG_SHIFT,
#if defined(FSL_FEATURE_LTC_HAS_DES) && FSL_FEATURE_LTC_HAS_DES
    kLTC_AlgorithmDES = LTC_MD_ALG_DES << LTC_MD_ALG_SHIFT,
    kLTC_Algorithm3DES = LTC_MD_ALG_TRIPLE_DES << LTC_MD_ALG_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_DES */
} ltc_algorithm_t;

typedef enum _ltc_mode_symmetric_alg
{
    kLTC_ModeCTR = 0x00U << LTC_MD_AAI_SHIFT,
    kLTC_ModeCBC = 0x10U << LTC_MD_AAI_SHIFT,
    kLTC_ModeECB = 0x20U << LTC_MD_AAI_SHIFT,
    kLTC_ModeCFB = 0x30U << LTC_MD_AAI_SHIFT,
    kLTC_ModeOFB = 0x40U << LTC_MD_AAI_SHIFT,
    kLTC_ModeCMAC = 0x60U << LTC_MD_AAI_SHIFT,
    kLTC_ModeXCBCMAC = 0x70U << LTC_MD_AAI_SHIFT,
    kLTC_ModeCCM = 0x80U << LTC_MD_AAI_SHIFT,
    kLTC_ModeGCM = 0x90U << LTC_MD_AAI_SHIFT,
} ltc_mode_symmetric_alg_t;

typedef enum _ltc_mode_encrypt
{
    kLTC_ModeDecrypt = LTC_MD_ENC_DECRYPT << LTC_MD_ENC_SHIFT,
    kLTC_ModeEncrypt = LTC_MD_ENC_ENCRYPT << LTC_MD_ENC_SHIFT,
} ltc_mode_encrypt_t;

typedef enum _ltc_mode_algorithm_state
{
    kLTC_ModeUpdate = LTC_MD_AS_UPDATE << LTC_MD_AS_SHIFT,
    kLTC_ModeInit = LTC_MD_AS_INITIALIZE << LTC_MD_AS_SHIFT,
    kLTC_ModeFinalize = LTC_MD_AS_FINALIZE << LTC_MD_AS_SHIFT,
    kLTC_ModeInitFinal = LTC_MD_AS_INIT_FINAL << LTC_MD_AS_SHIFT
} ltc_mode_algorithm_state_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

extern status_t ltc_get_context(LTC_Type *base, uint8_t *dest, uint8_t dataSize, uint8_t startIndex);
extern status_t ltc_set_context(LTC_Type *base, const uint8_t *data, uint8_t dataSize, uint8_t startIndex);
extern status_t ltc_symmetric_update(LTC_Type *base,
                                     const uint8_t *key,
                                     uint8_t keySize,
                                     ltc_algorithm_t alg,
                                     ltc_mode_symmetric_alg_t mode,
                                     ltc_mode_encrypt_t enc);
extern void ltc_memcpy(void *dst, const void *src, size_t size);
extern bool ltc_check_key_size(const uint32_t keySize);
extern status_t ltc_wait(LTC_Type *base);
extern void ltc_clear_all(LTC_Type *base, bool addPKHA);
extern status_t ltc_3des_check_input_args(ltc_mode_symmetric_alg_t modeAs,
                                          uint32_t size,
                                          const uint8_t *key1,
                                          const uint8_t *key2);
extern void ltc_symmetric_process(LTC_Type *base, uint32_t inSize, const uint8_t **inData, uint8_t **outData);
extern status_t ltc_symmetric_process_data(LTC_Type *base, const uint8_t *inData, uint32_t inSize, uint8_t *outData);

static uint32_t LTC_GetInstance(LTC_Type *base);
static void ltc_symmetric_process_EDMA(LTC_Type *base, uint32_t inSize, const uint8_t **inData, uint8_t **outData);
static status_t ltc_process_message_in_sessions_EDMA(LTC_Type *base, ltc_edma_handle_t *handle);

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * LTC Common code static
 ******************************************************************************/

/*!
 * @brief Splits the LTC job into sessions. Used for CBC, CTR, CFB, OFB cipher block modes.
 *
 * @param base LTC peripheral base address
 * @param inData Input data to process.
 * @param inSize Input size of the input buffer.
 * @param outData Output data buffer.
 */
static status_t ltc_process_message_in_sessions_EDMA(LTC_Type *base, ltc_edma_handle_t *handle)
{
    status_t retval;
    bool exit_sm = false;

    handle->modeReg = base->MD;
    retval = kStatus_Success;

    if ((!handle->inData) || (!handle->outData))
    {
        handle->state = LTC_SM_STATE_FINISH; /* END */
        retval = kStatus_InvalidArgument;
    }

    while (exit_sm == false)
    {
        switch (handle->state)
        {
            case LTC_SM_STATE_START:
                if (handle->size)
                {
                    uint32_t sz;

                    if (handle->size <= LTC_FIFO_SZ_MAX_DOWN_ALGN)
                    {
                        sz = handle->size;
                    }
                    else
                    {
                        sz = LTC_FIFO_SZ_MAX_DOWN_ALGN;
                    }

                    /* retval = ltc_symmetric_process_data_EDMA(base, handle->inData, sz, handle->outData); */
                    {
                        uint32_t lastSize;
                        uint32_t inSize = sz;

                        /* Write the data size. */
                        base->DS = inSize;

                        /* Split the inSize into full 16-byte chunks and last incomplete block due to LTC AES OFIFO
                         * errata */
                        if (inSize <= 16u)
                        {
                            lastSize = inSize;
                            inSize = 0;
                        }
                        else
                        {
                            /* Process all 16-byte data chunks. */
                            lastSize = inSize % 16u;
                            if (lastSize == 0)
                            {
                                lastSize = 16;
                                inSize -= 16;
                            }
                            else
                            {
                                inSize -=
                                    lastSize; /* inSize will be rounded down to 16 byte boundary. remaining bytes in
                                                 lastSize */
                            }
                        }

                        if (inSize)
                        {
                            handle->size -= inSize;
                            ltc_symmetric_process_EDMA(base, inSize, &handle->inData, &handle->outData);
                            exit_sm = true;
                        }
                        else if (lastSize)
                        {
                            ltc_symmetric_process(base, lastSize, &handle->inData, &handle->outData);
                            retval = ltc_wait(base);
                            handle->size -= lastSize;
                        }
                        else
                        {
                        }
                    }
                }
                else
                {
                    handle->state = LTC_SM_STATE_FINISH;
                }
                break;
            case LTC_SM_STATE_FINISH:
            default:
                base->MD = handle->modeReg;

                ltc_clear_all(base, false);

                if (handle->callback)
                {
                    handle->callback(base, handle, retval, handle->userData);
                }
                exit_sm = true;
                break;
        }
    }

    return retval;
}

/*!
 * @brief Splits the LTC job into sessions. Used for CBC, CTR, CFB, OFB cipher block modes.
 *
 * @param base LTC peripheral base address
 * @param inData Input data to process.
 * @param inSize Input size of the input buffer.
 * @param outData Output data buffer.
 */
static status_t ltc_process_message_in_sessions_ctr_EDMA(LTC_Type *base, ltc_edma_handle_t *handle)
{
    status_t retval;
    bool exit_sm = false;

    handle->modeReg = base->MD;
    retval = kStatus_Success;

    if ((!handle->inData) || (!handle->outData))
    {
        handle->state = LTC_SM_STATE_FINISH;
        retval = kStatus_InvalidArgument;
    }

    while (exit_sm == false)
    {
        switch (handle->state)
        {
            case LTC_SM_STATE_START:
                if (handle->size)
                {
                    uint32_t sz;

                    if (handle->size <= LTC_FIFO_SZ_MAX_DOWN_ALGN)
                    {
                        sz = handle->size;
                    }
                    else
                    {
                        sz = LTC_FIFO_SZ_MAX_DOWN_ALGN;
                    }

                    /* retval = ltc_symmetric_process_data_EDMA(base, handle->inData, sz, handle->outData); */
                    {
                        uint32_t lastSize;
                        uint32_t inSize = sz;

                        /* Write the data size. */
                        base->DS = inSize;

                        /* Split the inSize into full 16-byte chunks and last incomplete block due to LTC AES OFIFO
                         * errata */
                        if (inSize <= 16u)
                        {
                            lastSize = inSize;
                            inSize = 0;
                        }
                        else
                        {
                            /* Process all 16-byte data chunks. */
                            lastSize = inSize % 16u;
                            if (lastSize == 0)
                            {
                                lastSize = 16;
                                inSize -= 16;
                            }
                            else
                            {
                                inSize -=
                                    lastSize; /* inSize will be rounded down to 16 byte boundary. remaining bytes in
                                                 lastSize */
                            }
                        }

                        if (inSize)
                        {
                            handle->size -= inSize;
                            ltc_symmetric_process_EDMA(base, inSize, &handle->inData, &handle->outData);
                            exit_sm = true;
                        }
                        else if (lastSize)
                        {
                            ltc_symmetric_process(base, lastSize, &handle->inData, &handle->outData);
                            retval = ltc_wait(base);
                            handle->size -= lastSize;
                        }
                        else
                        {
                        }
                    }
                }
                else
                {
                    handle->state = LTC_SM_STATE_FINISH;
                }
                break;
            case LTC_SM_STATE_FINISH:
            default:
                base->MD = handle->modeReg;

                /* CTR final phase.*/
                if (kStatus_Success == retval)
                {
                    const uint8_t *input = handle->inData;
                    uint8_t *output = handle->outData;

                    if ((handle->counterlast != NULL) && (handle->lastSize))
                    {
                        uint8_t zeroes[16] = {0};
                        ltc_mode_t modeReg;

                        modeReg = (uint32_t)kLTC_AlgorithmAES | (uint32_t)kLTC_ModeCTR | (uint32_t)kLTC_ModeEncrypt;
                        /* Write the mode register to the hardware. */
                        base->MD = modeReg | (uint32_t)kLTC_ModeFinalize;

                        /* context is re-used (CTRi) */

                        /* Process data and return status. */
                        retval = ltc_symmetric_process_data(base, input, handle->lastSize, output);
                        if (kStatus_Success == retval)
                        {
                            if (handle->szLeft)
                            {
                                *handle->szLeft = 16U - handle->lastSize;
                            }

                            /* Initialize algorithm state. */
                            base->MD = modeReg | (uint32_t)kLTC_ModeUpdate;

                            /* context is re-used (CTRi) */

                            /* Process data and return status. */
                            retval = ltc_symmetric_process_data(base, zeroes, 16U, handle->counterlast);
                        }
                    }
                    if (kStatus_Success == retval)
                    {
                        ltc_get_context(base, &handle->counter[0], 16U, 4U);

                        ltc_clear_all(base, false);
                    }
                }

                if (handle->callback)
                {
                    handle->callback(base, handle, retval, handle->userData);
                }

                exit_sm = true;
                break;
        }
    }

    return retval;
}

/*******************************************************************************
 * AES Code public
 ******************************************************************************/

status_t LTC_AES_EncryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t *key,
                                uint32_t keySize)
{
    status_t retval;

    if ((ltc_check_key_size(keySize) == 0) || (size < 16u) ||
        (size % 16u)) /* ECB mode, size must be 16-byte multiple */
    {
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_InvalidArgument, handle->userData);
        }

        return kStatus_InvalidArgument;
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeECB, kLTC_ModeEncrypt);

    /* Process data and return status. */
    handle->inData = &plaintext[0];
    handle->outData = &ciphertext[0];
    handle->size = size;
    handle->state = LTC_SM_STATE_START;
    handle->state_machine = ltc_process_message_in_sessions_EDMA;
    retval = handle->state_machine(base, handle);
    return retval;
}

status_t LTC_AES_DecryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t *key,
                                uint32_t keySize,
                                ltc_aes_key_t keyType)
{
    status_t status;

    if ((ltc_check_key_size(keySize) == 0) || (size < 16u) ||
        (size % 16u)) /* ECB mode, size must be 16-byte multiple */
    {
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_InvalidArgument, handle->userData);
        }

        return kStatus_InvalidArgument;
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeECB, kLTC_ModeDecrypt);

    /* set DK bit in the LTC Mode Register AAI field for directly loaded decrypt keys */
    if (keyType == kLTC_DecryptKey)
    {
        base->MD |= (1U << kLTC_ModeRegBitShiftDK);
    }

    /* Process data and return status. */
    handle->inData = &ciphertext[0];
    handle->outData = &plaintext[0];
    handle->size = size;
    handle->state = LTC_SM_STATE_START;
    handle->state_machine = ltc_process_message_in_sessions_EDMA;
    status = handle->state_machine(base, handle);

    return status;
}

status_t LTC_AES_EncryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_AES_IV_SIZE],
                                const uint8_t *key,
                                uint32_t keySize)
{
    status_t retval;

    if ((ltc_check_key_size(keySize) == 0) || (size < 16u) ||
        (size % 16u)) /* CBC mode, size must be 16-byte multiple */
    {
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_InvalidArgument, handle->userData);
        }

        return kStatus_InvalidArgument;
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeCBC, kLTC_ModeEncrypt);

    /* Write IV data to the context register. */
    ltc_set_context(base, &iv[0], LTC_AES_IV_SIZE, 0);

    /* Process data and return status. */
    handle->inData = &plaintext[0];
    handle->outData = &ciphertext[0];
    handle->size = size;
    handle->state = LTC_SM_STATE_START;
    handle->state_machine = ltc_process_message_in_sessions_EDMA;
    retval = handle->state_machine(base, handle);
    return retval;
}

status_t LTC_AES_DecryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_AES_IV_SIZE],
                                const uint8_t *key,
                                uint32_t keySize,
                                ltc_aes_key_t keyType)
{
    status_t retval;

    if ((ltc_check_key_size(keySize) == 0) || (size < 16u) ||
        (size % 16u)) /* CBC mode, size must be 16-byte multiple */
    {
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_InvalidArgument, handle->userData);
        }

        return kStatus_InvalidArgument;
    }

    /* set DK bit in the LTC Mode Register AAI field for directly loaded decrypt keys */
    if (keyType == kLTC_DecryptKey)
    {
        base->MD |= (1U << kLTC_ModeRegBitShiftDK);
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeCBC, kLTC_ModeDecrypt);

    /* Write IV data to the context register. */
    ltc_set_context(base, &iv[0], LTC_AES_IV_SIZE, 0);

    /* Process data and return status. */
    handle->inData = &ciphertext[0];
    handle->outData = &plaintext[0];
    handle->size = size;
    handle->state = LTC_SM_STATE_START;
    handle->state_machine = ltc_process_message_in_sessions_EDMA;
    retval = handle->state_machine(base, handle);
    return retval;
}

status_t LTC_AES_CryptCtrEDMA(LTC_Type *base,
                              ltc_edma_handle_t *handle,
                              const uint8_t *input,
                              uint8_t *output,
                              uint32_t size,
                              uint8_t counter[LTC_AES_BLOCK_SIZE],
                              const uint8_t *key,
                              uint32_t keySize,
                              uint8_t counterlast[LTC_AES_BLOCK_SIZE],
                              uint32_t *szLeft)
{
    status_t retval;
    uint32_t lastSize;

    if (!ltc_check_key_size(keySize))
    {
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_InvalidArgument, handle->userData);
        }
        return kStatus_InvalidArgument;
    }

    lastSize = 0U;
    if (counterlast != NULL)
    {
        /* Split the size into full 16-byte chunks and last incomplete block due to LTC AES OFIFO errata */
        if (size <= 16U)
        {
            lastSize = size;
            size = 0U;
        }
        else
        {
            /* Process all 16-byte data chunks. */
            lastSize = size % 16U;
            if (lastSize == 0U)
            {
                lastSize = 16U;
                size -= 16U;
            }
            else
            {
                size -= lastSize; /* size will be rounded down to 16 byte boundary. remaining bytes in lastSize */
            }
        }
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeCTR, kLTC_ModeEncrypt);

    /* Write initial counter data to the context register.
     * NOTE the counter values start at 4-bytes offset into the context. */
    ltc_set_context(base, &counter[0], 16U, 4U);

    /* Process data and return status. */
    handle->inData = &input[0];
    handle->outData = &output[0];
    handle->size = size;
    handle->state = LTC_SM_STATE_START;
    handle->state_machine = ltc_process_message_in_sessions_ctr_EDMA;

    handle->counter = counter;
    handle->key = key;
    handle->keySize = keySize;
    handle->counterlast = counterlast;
    handle->szLeft = szLeft;
    handle->lastSize = lastSize;
    retval = handle->state_machine(base, handle);

    return retval;
}

#if defined(FSL_FEATURE_LTC_HAS_DES) && FSL_FEATURE_LTC_HAS_DES
/*******************************************************************************
 * DES / 3DES Code static
 ******************************************************************************/
static status_t ltc_des_process_EDMA(LTC_Type *base,
                                     ltc_edma_handle_t *handle,
                                     const uint8_t *input,
                                     uint8_t *output,
                                     uint32_t size,
                                     const uint8_t iv[LTC_DES_IV_SIZE],
                                     const uint8_t key[LTC_DES_KEY_SIZE],
                                     ltc_mode_symmetric_alg_t modeAs,
                                     ltc_mode_encrypt_t modeEnc)
{
    status_t retval;

    /* all but OFB, size must be 8-byte multiple */
    if ((modeAs != kLTC_ModeOFB) && ((size < 8u) || (size % 8u)))
    {
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_InvalidArgument, handle->userData);
        }
        return kStatus_InvalidArgument;
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, &key[0], LTC_DES_KEY_SIZE, kLTC_AlgorithmDES, modeAs, modeEnc);

    if ((modeAs != kLTC_ModeECB))
    {
        ltc_set_context(base, iv, LTC_DES_IV_SIZE, 0);
    }

    /* Process data and return status. */
    handle->inData = input;
    handle->outData = output;
    handle->size = size;
    handle->state = LTC_SM_STATE_START;
    handle->state_machine = ltc_process_message_in_sessions_EDMA;
    retval = handle->state_machine(base, handle);

    return retval;
}

static status_t ltc_3des_process_EDMA(LTC_Type *base,
                                      ltc_edma_handle_t *handle,
                                      const uint8_t *input,
                                      uint8_t *output,
                                      uint32_t size,
                                      const uint8_t iv[LTC_DES_IV_SIZE],
                                      const uint8_t key1[LTC_DES_KEY_SIZE],
                                      const uint8_t key2[LTC_DES_KEY_SIZE],
                                      const uint8_t key3[LTC_DES_KEY_SIZE],
                                      ltc_mode_symmetric_alg_t modeAs,
                                      ltc_mode_encrypt_t modeEnc)
{
    status_t retval;
    uint8_t key[LTC_DES_KEY_SIZE * 3];
    uint8_t keySize = LTC_DES_KEY_SIZE * 2;

    retval = ltc_3des_check_input_args(modeAs, size, key1, key2);
    if (kStatus_Success != retval)
    {
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_InvalidArgument, handle->userData);
        }
        return retval;
    }

    ltc_memcpy(&key[0], &key1[0], LTC_DES_KEY_SIZE);
    ltc_memcpy(&key[LTC_DES_KEY_SIZE], &key2[0], LTC_DES_KEY_SIZE);
    if (key3)
    {
        ltc_memcpy(&key[LTC_DES_KEY_SIZE * 2], &key3[0], LTC_DES_KEY_SIZE);
        keySize = sizeof(key);
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, &key[0], keySize, kLTC_Algorithm3DES, modeAs, modeEnc);

    if ((modeAs != kLTC_ModeECB))
    {
        ltc_set_context(base, iv, LTC_DES_IV_SIZE, 0);
    }

    /* Process data and return status. */
    handle->inData = input;
    handle->outData = output;
    handle->size = size;
    handle->state = LTC_SM_STATE_START;
    handle->state_machine = ltc_process_message_in_sessions_EDMA;
    retval = handle->state_machine(base, handle);

    return retval;
}
/*******************************************************************************
 * DES / 3DES Code public
 ******************************************************************************/
status_t LTC_DES_EncryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, plaintext, ciphertext, size, NULL, key, kLTC_ModeECB, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, ciphertext, plaintext, size, NULL, key, kLTC_ModeECB, kLTC_ModeDecrypt);
}

status_t LTC_DES_EncryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key, kLTC_ModeCBC, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key, kLTC_ModeCBC, kLTC_ModeDecrypt);
}

status_t LTC_DES_EncryptCfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key, kLTC_ModeCFB, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptCfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key, kLTC_ModeCFB, kLTC_ModeDecrypt);
}

status_t LTC_DES_EncryptOfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key, kLTC_ModeOFB, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptOfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key, kLTC_ModeOFB, kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, NULL, key1, key2, NULL, kLTC_ModeECB,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, NULL, key1, key2, key3, kLTC_ModeECB,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, NULL, key1, key2, NULL, kLTC_ModeECB,
                                 kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, NULL, key1, key2, key3, kLTC_ModeECB,
                                 kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key1, key2, NULL, kLTC_ModeCBC,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key1, key2, key3, kLTC_ModeCBC,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key1, key2, NULL, kLTC_ModeCBC,
                                 kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key1, key2, key3, kLTC_ModeCBC,
                                 kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key1, key2, NULL, kLTC_ModeCFB,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key1, key2, key3, kLTC_ModeCFB,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key1, key2, NULL, kLTC_ModeCFB,
                                 kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key1, key2, key3, kLTC_ModeCFB,
                                 kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key1, key2, NULL, kLTC_ModeOFB,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, plaintext, ciphertext, size, iv, key1, key2, key3, kLTC_ModeOFB,
                                 kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key1, key2, NULL, kLTC_ModeOFB,
                                 kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process_EDMA(base, handle, ciphertext, plaintext, size, iv, key1, key2, key3, kLTC_ModeOFB,
                                 kLTC_ModeDecrypt);
}
#endif /* FSL_FEATURE_LTC_HAS_DES */

/*********************** LTC EDMA tools ***************************************/

static uint32_t LTC_GetInstance(LTC_Type *base)
{
    uint32_t instance = 0;
    uint32_t i;

    for (i = 0; i < FSL_FEATURE_SOC_LTC_COUNT; i++)
    {
        if (s_ltcBase[instance] == base)
        {
            instance = i;
            break;
        }
    }
    return instance;
}

/*!
 * @brief Enable or disable LTC Input FIFO DMA request.
 *
 * This function enables or disables DMA request and done signals for Input FIFO.
 *
 * @param base LTC peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void LTC_EnableInputFifoDMA(LTC_Type *base, bool enable)
{
    if (enable)
    {
        base->CTL |= LTC_CTL_IFE_MASK;
    }
    else
    {
        base->CTL &= ~LTC_CTL_IFE_MASK;
    }
}

/*!
 * @brief Enable or disable LTC Output FIFO DMA request.
 *
 * This function enables or disables DMA request and done signals for Output FIFO.
 *
 * @param base LTC peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void LTC_EnableOutputFifoDMA(LTC_Type *base, bool enable)
{
    if (enable)
    {
        base->CTL |= LTC_CTL_OFE_MASK;
    }
    else
    {
        base->CTL &= ~LTC_CTL_OFE_MASK;
    }
}

static void LTC_InputFifoEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    ltc_edma_private_handle_t *ltcPrivateHandle = (ltc_edma_private_handle_t *)param;

    /* Avoid the warning for unused variables. */
    handle = handle;
    tcds = tcds;

    if (transferDone)
    {
        /* Stop DMA channel. */
        EDMA_StopTransfer(ltcPrivateHandle->handle->inputFifoEdmaHandle);

        /* Disable Input Fifo DMA */
        LTC_EnableInputFifoDMA(ltcPrivateHandle->base, false);
    }
}

static void LTC_OutputFifoEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    ltc_edma_private_handle_t *ltcPrivateHandle = (ltc_edma_private_handle_t *)param;

    /* Avoid the warning for unused variables. */
    handle = handle;
    tcds = tcds;

    if (transferDone)
    {
        /* Stop DMA channel. */
        EDMA_StopTransfer(ltcPrivateHandle->handle->outputFifoEdmaHandle);

        /* Disable Output Fifo DMA */
        LTC_EnableOutputFifoDMA(ltcPrivateHandle->base, false);

        if (ltcPrivateHandle->handle->state_machine)
        {
            ltcPrivateHandle->handle->state_machine(ltcPrivateHandle->base, ltcPrivateHandle->handle);
        }
    }
}

/* @brief Copy data to Input FIFO and reading from Ouput FIFO using eDMA. */
static void ltc_symmetric_process_EDMA(LTC_Type *base, uint32_t inSize, const uint8_t **inData, uint8_t **outData)
{
    const uint8_t *in = *inData;
    uint8_t *out = *outData;
    uint32_t instance = LTC_GetInstance(base);
    uint32_t entry_number = inSize / sizeof(uint32_t);
    const uint8_t *inputBuffer = *inData;
    uint8_t *outputBuffer = *outData;
    edma_transfer_config_t config;

    if (entry_number)
    {
        /* =========== Init Input FIFO DMA ======================*/
        memset(&config, 0, sizeof(config));

        /* Prepare transfer. */
        EDMA_PrepareTransfer(&config, (void *)inputBuffer, 1, (void *)(&((base)->IFIFO)), 4U, 4U, entry_number * 4,
                             kEDMA_MemoryToPeripheral);
        /* Submit transfer. */
        EDMA_SubmitTransfer(s_edmaPrivateHandle[instance].handle->inputFifoEdmaHandle, &config);

        /* Set request size.*/
        base->CTL &= ~LTC_CTL_IFR_MASK; /* 1 entry */
        /* Enable Input Fifo DMA */
        LTC_EnableInputFifoDMA(base, true);

        /* Start the DMA channel */
        EDMA_StartTransfer(s_edmaPrivateHandle[instance].handle->inputFifoEdmaHandle);

        /* =========== Init Output FIFO DMA ======================*/
        memset(&config, 0, sizeof(config));

        /* Prepare transfer. */
        EDMA_PrepareTransfer(&config, (void *)(&((base)->OFIFO)), 4U, (void *)outputBuffer, 1U, 4U, entry_number * 4,
                             kEDMA_PeripheralToMemory);
        /* Submit transfer. */
        EDMA_SubmitTransfer(s_edmaPrivateHandle[instance].handle->outputFifoEdmaHandle, &config);

        /* Set request size.*/
        base->CTL &= ~LTC_CTL_OFR_MASK; /* 1 entry */

        /* Enable Output Fifo DMA */
        LTC_EnableOutputFifoDMA(base, true);

        /* Start the DMA channel */
        EDMA_StartTransfer(s_edmaPrivateHandle[instance].handle->outputFifoEdmaHandle);

        { /* Dummy read of LTC register. Do not delete.*/
            volatile uint32_t status_reg;

            status_reg = (base)->STA;

            (void)status_reg;
        }

        out += entry_number * sizeof(uint32_t);
        in += entry_number * sizeof(uint32_t);

        *inData = in;
        *outData = out;
    }
}

void LTC_CreateHandleEDMA(LTC_Type *base,
                          ltc_edma_handle_t *handle,
                          ltc_edma_callback_t callback,
                          void *userData,
                          edma_handle_t *inputFifoEdmaHandle,
                          edma_handle_t *outputFifoEdmaHandle)
{
    assert(handle);
    assert(inputFifoEdmaHandle);
    assert(outputFifoEdmaHandle);

    uint32_t instance = LTC_GetInstance(base);

    s_edmaPrivateHandle[instance].base = base;
    s_edmaPrivateHandle[instance].handle = handle;

    memset(handle, 0, sizeof(*handle));

    handle->inputFifoEdmaHandle = inputFifoEdmaHandle;
    handle->outputFifoEdmaHandle = outputFifoEdmaHandle;

    handle->callback = callback;
    handle->userData = userData;

    /* Register DMA callback functions */
    EDMA_SetCallback(handle->inputFifoEdmaHandle, LTC_InputFifoEDMACallback, &s_edmaPrivateHandle[instance]);
    EDMA_SetCallback(handle->outputFifoEdmaHandle, LTC_OutputFifoEDMACallback, &s_edmaPrivateHandle[instance]);

    /* Set request size. DMA request size is 1 entry.*/
    base->CTL &= ~LTC_CTL_IFR_MASK;
    base->CTL &= ~LTC_CTL_OFR_MASK;
}
