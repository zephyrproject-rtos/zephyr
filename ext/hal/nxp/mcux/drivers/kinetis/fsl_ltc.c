/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
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

#include "fsl_ltc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! Full word representing the actual bit values for the LTC mode register. */
typedef uint32_t ltc_mode_t;

#define LTC_FIFO_SZ_MAX_DOWN_ALGN (0xff0u)
#define LTC_MD_ALG_AES (0x10U)        /*!< Bit field value for LTC_MD_ALG: AES */
#define LTC_MD_ALG_DES (0x20U)        /*!< Bit field value for LTC_MD_ALG: DES */
#define LTC_MD_ALG_TRIPLE_DES (0x21U) /*!< Bit field value for LTC_MD_ALG: 3DES */
#define LTC_MD_ALG_SHA1 (0x41U)       /*!< Bit field value for LTC_MD_ALG: SHA-1 */
#define LTC_MD_ALG_SHA224 (0x42U)     /*!< Bit field value for LTC_MD_ALG: SHA-224 */
#define LTC_MD_ALG_SHA256 (0x43U)     /*!< Bit field value for LTC_MD_ALG: SHA-256 */
#define LTC_MDPK_ALG_PKHA (0x80U)     /*!< Bit field value for LTC_MDPK_ALG: PKHA */
#define LTC_MD_ENC_DECRYPT (0U)       /*!< Bit field value for LTC_MD_ENC: Decrypt. */
#define LTC_MD_ENC_ENCRYPT (0x1U)     /*!< Bit field value for LTC_MD_ENC: Encrypt. */
#define LTC_MD_AS_UPDATE (0U)         /*!< Bit field value for LTC_MD_AS: Update */
#define LTC_MD_AS_INITIALIZE (0x1U)   /*!< Bit field value for LTC_MD_AS: Initialize */
#define LTC_MD_AS_FINALIZE (0x2U)     /*!< Bit field value for LTC_MD_AS: Finalize */
#define LTC_MD_AS_INIT_FINAL (0x3U)   /*!< Bit field value for LTC_MD_AS: Initialize/Finalize */

#define LTC_AES_GCM_TYPE_AAD 55
#define LTC_AES_GCM_TYPE_IV 0

#define LTC_CCM_TAG_IDX 8 /*! For CCM encryption, the encrypted final MAC is written to the context word 8-11 */
#define LTC_GCM_TAG_IDX 0 /*! For GCM encryption, the encrypted final MAC is written to the context word 0-3 */

enum _ltc_md_dk_bit_shift
{
    kLTC_ModeRegBitShiftDK = 12U,
};

typedef enum _ltc_algorithm
{
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
    kLTC_AlgorithmPKHA = LTC_MDPK_ALG_PKHA << LTC_MD_ALG_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_PKHA */
    kLTC_AlgorithmAES = LTC_MD_ALG_AES << LTC_MD_ALG_SHIFT,
#if defined(FSL_FEATURE_LTC_HAS_DES) && FSL_FEATURE_LTC_HAS_DES
    kLTC_AlgorithmDES = LTC_MD_ALG_DES << LTC_MD_ALG_SHIFT,
    kLTC_Algorithm3DES = LTC_MD_ALG_TRIPLE_DES << LTC_MD_ALG_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_DES */
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
    kLTC_AlgorithmSHA1 = LTC_MD_ALG_SHA1 << LTC_MD_ALG_SHIFT,
    kLTC_AlgorithmSHA224 = LTC_MD_ALG_SHA224 << LTC_MD_ALG_SHIFT,
    kLTC_AlgorithmSHA256 = LTC_MD_ALG_SHA256 << LTC_MD_ALG_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_SHA */
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

/*! @brief LTC status flags */
enum _ltc_status_flag
{
    kLTC_StatusAesBusy = 1U << LTC_STA_AB_SHIFT,
#if defined(FSL_FEATURE_LTC_HAS_DES) && FSL_FEATURE_LTC_HAS_DES
    kLTC_StatusDesBusy = 1U << LTC_STA_DB_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_DES */
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
    kLTC_StatusPkhaBusy = 1U << LTC_STA_PB_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_PKHA */
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
    kLTC_StatusMdhaBusy = 1U << LTC_STA_MB_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_SHA */
    kLTC_StatusDoneIsr = 1U << LTC_STA_DI_SHIFT,
    kLTC_StatusErrorIsr = 1U << LTC_STA_EI_SHIFT,
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
    kLTC_StatusPublicKeyPrime = 1U << LTC_STA_PKP_SHIFT,
    kLTC_StatusPublicKeyOpOne = 1U << LTC_STA_PKO_SHIFT,
    kLTC_StatusPublicKeyOpZero = 1U << LTC_STA_PKZ_SHIFT,
#endif /* FSL_FEATURE_LTC_HAS_PKHA */
    kLTC_StatusAll = LTC_STA_AB_MASK |
#if defined(FSL_FEATURE_LTC_HAS_DES) && FSL_FEATURE_LTC_HAS_DES
                     LTC_STA_DB_MASK |
#endif /* FSL_FEATURE_LTC_HAS_DES */
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
                     LTC_STA_MB_MASK |
#endif /* FSL_FEATURE_LTC_HAS_SHA */
                     LTC_STA_DI_MASK | LTC_STA_EI_MASK
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
                     |
                     LTC_STA_PB_MASK | LTC_STA_PKP_MASK | LTC_STA_PKO_MASK | LTC_STA_PKZ_MASK
#endif /* FSL_FEATURE_LTC_HAS_PKHA */
};

/*! @brief LTC clear register */
typedef enum _ltc_clear_written
{
    kLTC_ClearMode = 1U << LTC_CW_CM_SHIFT,
    kLTC_ClearDataSize = 1U << LTC_CW_CDS_SHIFT,
    kLTC_ClearIcvSize = 1U << LTC_CW_CICV_SHIFT,
    kLTC_ClearContext = 1U << LTC_CW_CCR_SHIFT,
    kLTC_ClearKey = 1U << LTC_CW_CKR_SHIFT,
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
    kLTC_ClearPkhaSizeA = 1U << LTC_CW_CPKA_SHIFT,
    kLTC_ClearPkhaSizeB = 1U << LTC_CW_CPKB_SHIFT,
    kLTC_ClearPkhaSizeN = 1U << LTC_CW_CPKN_SHIFT,
    kLTC_ClearPkhaSizeE = 1U << LTC_CW_CPKE_SHIFT,
    kLTC_ClearAllSize = (int)kLTC_ClearPkhaSizeA | kLTC_ClearPkhaSizeB | kLTC_ClearPkhaSizeN | kLTC_ClearPkhaSizeE,
#endif /* FSL_FEATURE_LTC_HAS_PKHA */
    kLTC_ClearOutputFifo = 1U << LTC_CW_COF_SHIFT,
    kLTC_ClearInputFifo = (int)(1U << LTC_CW_CIF_SHIFT),
    kLTC_ClearAll = (int)(LTC_CW_CM_MASK | LTC_CW_CDS_MASK | LTC_CW_CICV_MASK | LTC_CW_CCR_MASK | LTC_CW_CKR_MASK |
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
                          LTC_CW_CPKA_MASK | LTC_CW_CPKB_MASK | LTC_CW_CPKN_MASK | LTC_CW_CPKE_MASK |
#endif /* FSL_FEATURE_LTC_HAS_PKHA */
                          LTC_CW_COF_MASK | LTC_CW_CIF_MASK)
} ltc_clear_written_t;

enum _ltc_ctrl_swap
{
    kLTC_CtrlSwapAll =
        LTC_CTL_IFS_MASK | LTC_CTL_OFS_MASK | LTC_CTL_KIS_MASK | LTC_CTL_KOS_MASK | LTC_CTL_CIS_MASK | LTC_CTL_COS_MASK,
};

/*! @brief Type used in GCM and CCM modes.

    Content of a block is established via individual bytes and moved to LTC
   IFIFO by moving 32-bit words.
*/
typedef union _ltc_xcm_block_t
{
    uint32_t w[4]; /*!< LTC context register is 16 bytes written as four 32-bit words */
    uint8_t b[16]; /*!< 16 octets block for CCM B0 and CTR0 and for GCM */
} ltc_xcm_block_t;

#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA

/*! @brief PKHA functions - arithmetic, copy/clear memory. */
typedef enum _ltc_pkha_func_t
{
    kLTC_PKHA_ClearMem = 1U,
    kLTC_PKHA_ArithModAdd = 2U,         /*!< (A + B) mod N */
    kLTC_PKHA_ArithModSub1 = 3U,        /*!< (A - B) mod N */
    kLTC_PKHA_ArithModSub2 = 4U,        /*!< (B - A) mod N */
    kLTC_PKHA_ArithModMul = 5U,         /*!< (A x B) mod N */
    kLTC_PKHA_ArithModExp = 6U,         /*!< (A^E) mod N */
    kLTC_PKHA_ArithModRed = 7U,         /*!< (A) mod N */
    kLTC_PKHA_ArithModInv = 8U,         /*!< (A^-1) mod N */
    kLTC_PKHA_ArithEccAdd = 9U,         /*!< (P1 + P2) */
    kLTC_PKHA_ArithEccDouble = 10U,     /*!< (P2 + P2) */
    kLTC_PKHA_ArithEccMul = 11U,        /*!< (E x P1) */
    kLTC_PKHA_ArithModR2 = 12U,         /*!< (R^2 mod N) */
    kLTC_PKHA_ArithGcd = 14U,           /*!< GCD (A, N) */
    kLTC_PKHA_ArithPrimalityTest = 15U, /*!< Miller-Rabin */
    kLTC_PKHA_CopyMemSizeN = 16U,
    kLTC_PKHA_CopyMemSizeSrc = 17U,
} ltc_pkha_func_t;

/*! @brief Register areas for PKHA clear memory operations. */
typedef enum _ltc_pkha_reg_area
{
    kLTC_PKHA_RegA = 8U,
    kLTC_PKHA_RegB = 4U,
    kLTC_PKHA_RegE = 2U,
    kLTC_PKHA_RegN = 1U,
    kLTC_PKHA_RegAll = kLTC_PKHA_RegA | kLTC_PKHA_RegB | kLTC_PKHA_RegE | kLTC_PKHA_RegN,
} ltc_pkha_reg_area_t;

/*! @brief Quadrant areas for 2048-bit registers for PKHA copy memory
 * operations. */
typedef enum _ltc_pkha_quad_area_t
{
    kLTC_PKHA_Quad0 = 0U,
    kLTC_PKHA_Quad1 = 1U,
    kLTC_PKHA_Quad2 = 2U,
    kLTC_PKHA_Quad3 = 3U,
} ltc_pkha_quad_area_t;

/*! @brief User-supplied (R^2 mod N) input or LTC should calculate. */
typedef enum _ltc_pkha_r2_t
{
    kLTC_PKHA_CalcR2 = 0U, /*!< Calculate (R^2 mod N) */
    kLTC_PKHA_InputR2 = 1U /*!< (R^2 mod N) supplied as input */
} ltc_pkha_r2_t;

/*! @brief LTC PKHA parameters */
typedef struct _ltc_pkha_mode_params_t
{
    ltc_pkha_func_t func;
    ltc_pkha_f2m_t arithType;
    ltc_pkha_montgomery_form_t montFormIn;
    ltc_pkha_montgomery_form_t montFormOut;
    ltc_pkha_reg_area_t srcReg;
    ltc_pkha_quad_area_t srcQuad;
    ltc_pkha_reg_area_t dstReg;
    ltc_pkha_quad_area_t dstQuad;
    ltc_pkha_timing_t equalTime;
    ltc_pkha_r2_t r2modn;
} ltc_pkha_mode_params_t;

#endif /* FSL_FEATURE_LTC_HAS_PKHA */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
static status_t ltc_pkha_clear_regabne(LTC_Type *base, bool A, bool B, bool N, bool E);
#endif /* FSL_FEATURE_LTC_HAS_PKHA */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * LTC Common code static
 ******************************************************************************/
/*!
 * @brief Tests the correct key size.
 *
 * This function tests the correct key size.
 * @param keySize Input key length in bytes.
 * @return True if the key length is supported, false if not.
 */
bool ltc_check_key_size(const uint32_t keySize)
{
    return ((keySize == 16u)
#if defined(FSL_FEATURE_LTC_HAS_AES192) && FSL_FEATURE_LTC_HAS_AES192
            || ((keySize == 24u))
#endif /* FSL_FEATURE_LTC_HAS_AES192 */
#if defined(FSL_FEATURE_LTC_HAS_AES256) && FSL_FEATURE_LTC_HAS_AES256
            || ((keySize == 32u))
#endif /* FSL_FEATURE_LTC_HAS_AES256 */
                );
}

/*! @brief LTC driver wait mechanism. */
status_t ltc_wait(LTC_Type *base)
{
    status_t status;

    bool error = false;
    bool done = false;

    /* Wait for 'done' or 'error' flag. */
    while ((!error) && (!done))
    {
        uint32_t temp32 = base->STA;
        error = temp32 & LTC_STA_EI_MASK;
        done = temp32 & LTC_STA_DI_MASK;
    }

    if (error)
    {
        base->COM = LTC_COM_ALL_MASK; /* Reset all engine to clear the error flag */
        status = kStatus_Fail;
    }
    else /* 'done' */
    {
        status = kStatus_Success;

        base->CW = kLTC_ClearDataSize;
        /* Clear 'done' interrupt status.  This also clears the mode register. */
        base->STA = kLTC_StatusDoneIsr;
    }

    return status;
}

/*!
 * @brief Clears the LTC module.
 * This function can be used to clear all sensitive data from theLTC module, such as private keys. It is called
 * internally by the LTC driver in case of an error or operation complete.
 * @param base LTC peripheral base address
 * @param pkha Include LTC PKHA register clear. If there is no PKHA, the argument is ignored.
 */
void ltc_clear_all(LTC_Type *base, bool addPKHA)
{
    base->CW = (uint32_t)kLTC_ClearAll;
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
    if (addPKHA)
    {
        ltc_pkha_clear_regabne(base, true, true, true, true);
    }
#endif /* FSL_FEATURE_LTC_HAS_PKHA */
}

void ltc_memcpy(void *dst, const void *src, size_t size)
{
#if defined(__cplusplus)
    register uint8_t *to = (uint8_t *)dst;
    register const uint8_t *from = (const uint8_t *)src;
#else
    register uint8_t *to = dst;
    register const uint8_t *from = src;
#endif
    while (size)
    {
        *to = *from;
        size--;
        to++;
        from++;
    }
}

/*!
 * @brief Reads an unaligned word.
 *
 * This function creates a 32-bit word from an input array of four bytes.
 *
 * @param src Input array of four bytes. The array can start at any address in memory.
 * @return 32-bit unsigned int created from the input byte array.
 */
static inline uint32_t ltc_get_word_from_unaligned(const uint8_t *srcAddr)
{
#if (!(defined(__CORTEX_M)) || (defined(__CORTEX_M) && (__CORTEX_M == 0)))
    register const uint8_t *src = srcAddr;
    /* Cortex M0 does not support misaligned loads */
    if ((uint32_t)src & 0x3u)
    {
        union _align_bytes_t
        {
            uint32_t word;
            uint8_t byte[sizeof(uint32_t)];
        } my_bytes;

        my_bytes.byte[0] = *src;
        my_bytes.byte[1] = *(src + 1);
        my_bytes.byte[2] = *(src + 2);
        my_bytes.byte[3] = *(src + 3);
        return my_bytes.word;
    }
    else
    {
        /* addr aligned to 0-modulo-4 so it is safe to type cast */
        return *((const uint32_t *)src);
    }
#elif defined(__CC_ARM)
    /* -O3 optimization in Keil 5.15 and 5.16a uses LDM instruction here (LDM r4!, {r0})
     *    which is wrong, because srcAddr might be unaligned.
     *    LDM on unaligned address causes hard-fault. in contrary,
     *    LDR supports unaligned address on Cortex M4 */
    register uint32_t retVal;
    __asm
    {
        LDR retVal, [srcAddr]
    }
    return retVal;
#else
    return *((const uint32_t *)srcAddr);
#endif
}

/*!
 * @brief Converts a 32-bit word into a byte array.
 *
 * This function creates an output array of four bytes from an input 32-bit word.
 *
 * @param srcWord Input 32-bit unsigned integer.
 * @param dst Output array of four bytes. The array can start at any address in memory.
 */
static inline void ltc_set_unaligned_from_word(uint32_t srcWord, uint8_t *dstAddr)
{
#if (!(defined(__CORTEX_M)) || (defined(__CORTEX_M) && (__CORTEX_M == 0)))
    register uint8_t *dst = dstAddr;
    /* Cortex M0 does not support misaligned stores */
    if ((uint32_t)dst & 0x3u)
    {
        *dst++ = (srcWord & 0x000000FFU);
        *dst++ = (srcWord & 0x0000FF00U) >> 8;
        *dst++ = (srcWord & 0x00FF0000U) >> 16;
        *dst++ = (srcWord & 0xFF000000U) >> 24;
    }
    else
    {
        *((uint32_t *)dstAddr) = srcWord; /* addr aligned to 0-modulo-4 so it is safe to type cast */
    }
#elif defined(__CC_ARM)
    __asm
    {
        STR srcWord, [dstAddr]
    }
    return;
#else
    *((uint32_t *)dstAddr) = srcWord;
#endif
}

/*!
 * @brief Sets the LTC keys.
 *
 * This function writes the LTC keys into the key register.  The keys should
 * be written before the key size.
 *
 * @param base LTC peripheral base address
 * @param key Key
 * @param keySize Number of bytes for all keys to be loaded (maximum 32, must be a
 *                multiple of 4).
 * @returns Key set status
 */
static status_t ltc_set_key(LTC_Type *base, const uint8_t *key, uint8_t keySize)
{
    int32_t i;

    for (i = 0; i < (keySize / 4); i++)
    {
        base->KEY[i] = ltc_get_word_from_unaligned(key + i * sizeof(uint32_t));
    }

    return kStatus_Success;
}

/*!
 * @brief Gets the LTC keys.
 *
 * This function retrieves the LTC keys from the key register.
 *
 * @param base LTC peripheral base address
 * @param key Array of data to store keys
 * @param keySize Number of bytes of keys to retrieve
 * @returns Key set status
 */
static status_t ltc_get_key(LTC_Type *base, uint8_t *key, uint8_t keySize)
{
    int32_t i;

    for (i = 0; i < (keySize / 4); i++)
    {
        ltc_set_unaligned_from_word(base->KEY[i], key + i * sizeof(uint32_t));
    }

    return kStatus_Success;
}

/*!
 * @brief Writes the LTC context register;
 *
 * The LTC context register is a 512 bit (64 byte) register that holds
 * internal context for the crypto engine.  The meaning varies based on the
 * algorithm and operating state being used.  This register is written by the
 * driver/application to load state such as IV, counter, and so on. Then, it is
 * updated by the internal crypto engine as needed.
 *
 * @param base LTC peripheral base address
 * @param data Data to write
 * @param dataSize Size of data to write in bytes
 * @param startIndex Starting word (4-byte) index into the 16-word register.
 * @return Status of write
 */
status_t ltc_set_context(LTC_Type *base, const uint8_t *data, uint8_t dataSize, uint8_t startIndex)
{
    int32_t i;
    int32_t j;
    int32_t szLeft;

    /* Context register is 16 words in size (64 bytes).  Ensure we are only
     * writing a valid amount of data. */
    if (startIndex + (dataSize / 4) >= 16)
    {
        return kStatus_InvalidArgument;
    }

    j = 0;
    szLeft = dataSize % 4;
    for (i = startIndex; i < (startIndex + dataSize / 4); i++)
    {
        base->CTX[i] = ltc_get_word_from_unaligned(data + j);
        j += sizeof(uint32_t);
    }

    if (szLeft)
    {
        uint32_t context_data = {0};
        ltc_memcpy(&context_data, data + j, szLeft);
        base->CTX[i] = context_data;
    }
    return kStatus_Success;
}

/*!
 * @brief Reads the LTC context register.
 *
 * The LTC context register is a 512 bit (64 byte) register that holds
 * internal context for the crypto engine.  The meaning varies based on the
 * algorithm and operating state being used.  This register is written by the
 * driver/application to load state such as IV, counter, and so on. Then, it is
 * updated by the internal crypto engine as needed.
 *
 * @param base LTC peripheral base address
 * @param data Destination of read data
 * @param dataSize Size of data to read in bytes
 * @param startIndex Starting word (4-byte) index into the 16-word register.
 * @return Status of read
 */
status_t ltc_get_context(LTC_Type *base, uint8_t *dest, uint8_t dataSize, uint8_t startIndex)
{
    int32_t i;
    int32_t j;
    int32_t szLeft;
    uint32_t rdCtx;

    /* Context register is 16 words in size (64 bytes).  Ensure we are only
     * writing a valid amount of data. */
    if (startIndex + (dataSize / 4) >= 16)
    {
        return kStatus_InvalidArgument;
    }

    j = 0;
    szLeft = dataSize % 4;
    for (i = startIndex; i < (startIndex + dataSize / 4); i++)
    {
        ltc_set_unaligned_from_word(base->CTX[i], dest + j);
        j += sizeof(uint32_t);
    }

    if (szLeft)
    {
        rdCtx = 0;
        rdCtx = base->CTX[i];
        ltc_memcpy(dest + j, &rdCtx, szLeft);
    }
    return kStatus_Success;
}

static status_t ltc_symmetric_alg_state(LTC_Type *base,
                                        const uint8_t *key,
                                        uint8_t keySize,
                                        ltc_algorithm_t alg,
                                        ltc_mode_symmetric_alg_t mode,
                                        ltc_mode_encrypt_t enc,
                                        ltc_mode_algorithm_state_t as)
{
    ltc_mode_t modeReg;

    /* Clear internal register states. */
    base->CW = (uint32_t)kLTC_ClearAll;

    /* Set byte swap on for several registers we will be reading and writing
     * user data to/from. */
    base->CTL |= kLTC_CtrlSwapAll;

    /* Write the key in place. */
    ltc_set_key(base, key, keySize);

    /* Write the key size.  This must be done after writing the key, and this
     * action locks the ability to modify the key registers. */
    base->KS = keySize;

    /* Clear the 'done' interrupt. */
    base->STA = kLTC_StatusDoneIsr;

    /* Set the proper block and algorithm mode. */
    modeReg = (uint32_t)alg | (uint32_t)enc | (uint32_t)as | (uint32_t)mode;

    /* Write the mode register to the hardware. */
    base->MD = modeReg;

    return kStatus_Success;
}

/*!
 * @brief Initializes the LTC for symmetric encrypt/decrypt operation. Mode is set to UPDATE.
 *
 * @param base LTC peripheral base address
 * @param key Input key to use for encryption
 * @param keySize Size of the input key, in bytes. Must be 8, 16, 24, or 32.
 * @param alg Symmetric algorithm
 * @param mode Symmetric block mode
 * @param enc Encrypt/decrypt control
 * @return Status
 */
status_t ltc_symmetric_update(LTC_Type *base,
                              const uint8_t *key,
                              uint8_t keySize,
                              ltc_algorithm_t alg,
                              ltc_mode_symmetric_alg_t mode,
                              ltc_mode_encrypt_t enc)
{
    return ltc_symmetric_alg_state(base, key, keySize, alg, mode, enc, kLTC_ModeUpdate);
}

#if defined(FSL_FEATURE_LTC_HAS_GCM) && FSL_FEATURE_LTC_HAS_GCM
/*!
 * @brief Initializes the LTC for symmetric encrypt/decrypt operation. Mode is set to FINALIZE.
 *
 * @param base LTC peripheral base address
 * @param key Input key to use for encryption
 * @param keySize Size of the input key, in bytes. Must be 8, 16, 24, or 32.
 * @param alg Symmetric algorithm
 * @param mode Symmetric block mode
 * @param enc Encrypt/decrypt control
 * @return Status
 */
static status_t ltc_symmetric_final(LTC_Type *base,
                                    const uint8_t *key,
                                    uint8_t keySize,
                                    ltc_algorithm_t alg,
                                    ltc_mode_symmetric_alg_t mode,
                                    ltc_mode_encrypt_t enc)
{
    return ltc_symmetric_alg_state(base, key, keySize, alg, mode, enc, kLTC_ModeFinalize);
}
#endif /* FSL_FEATURE_LTC_HAS_GCM */

/*!
 * @brief Initializes the LTC for symmetric encrypt/decrypt operation. Mode is set to INITIALIZE.
 *
 * @param base LTC peripheral base address
 * @param key Input key to use for encryption
 * @param keySize Size of the input key, in bytes. Must be 8, 16, 24, or 32.
 * @param alg Symmetric algorithm
 * @param mode Symmetric block mode
 * @param enc Encrypt/decrypt control
 * @return Status
 */
static status_t ltc_symmetric_init(LTC_Type *base,
                                   const uint8_t *key,
                                   uint8_t keySize,
                                   ltc_algorithm_t alg,
                                   ltc_mode_symmetric_alg_t mode,
                                   ltc_mode_encrypt_t enc)
{
    return ltc_symmetric_alg_state(base, key, keySize, alg, mode, enc, kLTC_ModeInit);
}

/*!
 * @brief Initializes the LTC for symmetric encrypt/decrypt operation. Mode is set to INITIALIZE/FINALIZE.
 *
 * @param base LTC peripheral base address
 * @param key Input key to use for encryption
 * @param keySize Size of the input key, in bytes. Must be 8, 16, 24, or 32.
 * @param alg Symmetric algorithm
 * @param mode Symmetric block mode
 * @param enc Encrypt/decrypt control
 * @return Status
 */
static status_t ltc_symmetric_init_final(LTC_Type *base,
                                         const uint8_t *key,
                                         uint8_t keySize,
                                         ltc_algorithm_t alg,
                                         ltc_mode_symmetric_alg_t mode,
                                         ltc_mode_encrypt_t enc)
{
    return ltc_symmetric_alg_state(base, key, keySize, alg, mode, enc, kLTC_ModeInitFinal);
}

void ltc_symmetric_process(LTC_Type *base, uint32_t inSize, const uint8_t **inData, uint8_t **outData)
{
    uint32_t outSize;
    uint32_t fifoData;
    uint32_t fifoStatus;

    register const uint8_t *in = *inData;
    register uint8_t *out = *outData;

    outSize = inSize;
    while ((outSize > 0) || (inSize > 0))
    {
        fifoStatus = base->FIFOSTA;

        /* Check output FIFO level to make sure there is at least an entry
         * ready to be read. */
        if (fifoStatus & LTC_FIFOSTA_OFL_MASK)
        {
            /* Read data from the output FIFO. */
            if (outSize > 0)
            {
                if (outSize >= sizeof(uint32_t))
                {
                    ltc_set_unaligned_from_word(base->OFIFO, out);
                    out += sizeof(uint32_t);
                    outSize -= sizeof(uint32_t);
                }
                else /* (outSize > 0) && (outSize < 4) */
                {
                    fifoData = base->OFIFO;
                    ltc_memcpy(out, &fifoData, outSize);
                    out += outSize;
                    outSize = 0;
                }
            }
        }

        /* Check input FIFO status to see if it is full.  We can
         * only write more data when both input and output FIFOs are not at a full state.
         * At the same time we are sure Output FIFO is not full because we have poped at least one entry
         * by the while loop above.
         */
        if (!(fifoStatus & LTC_FIFOSTA_IFF_MASK))
        {
            /* Copy data to the input FIFO.
             * Data can only be copied one word at a time, so pad the data
             * appropriately if it is less than this size. */
            if (inSize > 0)
            {
                if (inSize >= sizeof(uint32_t))
                {
                    base->IFIFO = ltc_get_word_from_unaligned(in);
                    inSize -= sizeof(uint32_t);
                    in += sizeof(uint32_t);
                }
                else /* (inSize > 0) && (inSize < 4) */
                {
                    fifoData = 0;
                    ltc_memcpy(&fifoData, in, inSize);
                    base->IFIFO = fifoData;
                    in += inSize;
                    inSize = 0;
                }
            }
        }
    }
    *inData = in;
    *outData = out;
}

/*!
 * @brief Processes symmetric data through LTC AES and DES engines.
 *
 * @param base LTC peripheral base address
 * @param inData Input data
 * @param inSize Size of input data, in bytes
 * @param outData Output data
 * @return Status from encrypt/decrypt operation
 */
status_t ltc_symmetric_process_data(LTC_Type *base, const uint8_t *inData, uint32_t inSize, uint8_t *outData)
{
    uint32_t lastSize;

    if ((!inData) || (!outData))
    {
        return kStatus_InvalidArgument;
    }

    /* Write the data size. */
    base->DS = inSize;

    /* Split the inSize into full 16-byte chunks and last incomplete block due to LTC AES OFIFO errata */
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
            inSize -= lastSize; /* inSize will be rounded down to 16 byte boundary. remaining bytes in lastSize */
        }
    }

    ltc_symmetric_process(base, inSize, &inData, &outData);
    ltc_symmetric_process(base, lastSize, &inData, &outData);
    return ltc_wait(base);
}

/*!
 * @brief Splits the LTC job into sessions. Used for CBC, CTR, CFB, OFB cipher block modes.
 *
 * @param base LTC peripheral base address
 * @param inData Input data to process.
 * @param inSize Input size of the input buffer.
 * @param outData Output data buffer.
 */
static status_t ltc_process_message_in_sessions(LTC_Type *base,
                                                const uint8_t *inData,
                                                uint32_t inSize,
                                                uint8_t *outData)
{
    uint32_t sz;
    status_t retval;
    ltc_mode_t modeReg; /* read and write LTC mode register */

    sz = LTC_FIFO_SZ_MAX_DOWN_ALGN;
    modeReg = base->MD;
    retval = kStatus_Success;

    while (inSize)
    {
        if (inSize <= sz)
        {
            retval = ltc_symmetric_process_data(base, inData, inSize, outData);
            if (kStatus_Success != retval)
            {
                return retval;
            }
            inSize = 0;
        }
        else
        {
            retval = ltc_symmetric_process_data(base, inData, sz, outData);
            if (kStatus_Success != retval)
            {
                return retval;
            }
            inData += sz;
            inSize -= sz;
            outData += sz;
            base->MD = modeReg;
        }
    }
    return retval;
}

static void ltc_move_block_to_ififo(LTC_Type *base, const ltc_xcm_block_t *blk, uint32_t num_bytes)
{
    uint32_t i = 0;
    uint32_t words;

    words = num_bytes / 4u;
    if (num_bytes % 4u)
    {
        words++;
    }

    if (words > 4)
    {
        words = 4;
    }

    while (i < words)
    {
        if (0U == (base->FIFOSTA & LTC_FIFOSTA_IFF_MASK))
        {
            /* Copy data to the input FIFO. */
            base->IFIFO = blk->w[i++];
        }
    }
}

static void ltc_move_to_ififo(LTC_Type *base, const uint8_t *data, uint32_t dataSize)
{
    ltc_xcm_block_t blk;
    ltc_xcm_block_t blkZero = {{0x0u, 0x0u, 0x0u, 0x0u}};

    while (dataSize)
    {
        if (dataSize > 16u)
        {
            ltc_memcpy(&blk, data, 16u);
            dataSize -= 16u;
            data += 16u;
        }
        else
        {
            ltc_memcpy(&blk, &blkZero, sizeof(ltc_xcm_block_t)); /* memset blk to zeroes */
            ltc_memcpy(&blk, data, dataSize);
            dataSize = 0;
        }
        ltc_move_block_to_ififo(base, &blk, sizeof(ltc_xcm_block_t));
    }
}

/*!
 * @brief Processes symmetric data through LTC AES in multiple sessions.
 *
 * Specific for AES CCM and GCM modes as they need to update mode register.
 *
 * @param base LTC peripheral base address
 * @param inData Input data
 * @param inSize Size of input data, in bytes
 * @param outData Output data
 * @param lastAs The LTC Algorithm state to be set sup for last block during message processing in multiple sessions.
 *               For CCM it is kLTC_ModeFinalize. For GCM it is kLTC_ModeInitFinal.
 * @return Status from encrypt/decrypt operation
 */
static status_t ltc_symmetric_process_data_multiple(LTC_Type *base,
                                                    const uint8_t *inData,
                                                    uint32_t inSize,
                                                    uint8_t *outData,
                                                    ltc_mode_t modeReg,
                                                    ltc_mode_algorithm_state_t lastAs)
{
    uint32_t fifoConsumed;
    uint32_t lastSize;
    uint32_t sz;
    uint32_t max_ltc_fifo_size;
    ltc_mode_algorithm_state_t fsm;
    status_t status;

    if ((!inData) || (!outData))
    {
        return kStatus_InvalidArgument;
    }

    if (!((kLTC_ModeFinalize == lastAs) || (kLTC_ModeInitFinal == lastAs)))
    {
        return kStatus_InvalidArgument;
    }

    if (0 == inSize)
    {
        return kStatus_Success;
    }

    if (inSize <= 16u)
    {
        fsm = lastAs;
        lastSize = inSize;
    }
    else
    {
        fsm = (ltc_mode_algorithm_state_t)(
            modeReg &
            LTC_MD_AS_MASK); /* this will be either kLTC_ModeInit or kLTC_ModeUpdate, based on prior processing */

        /* Process all 16-byte data chunks. */
        lastSize = inSize % 16u;
        if (lastSize == 0u)
        {
            lastSize = 16u;
            inSize -= 16u;
        }
        else
        {
            inSize -= lastSize; /* inSize will be rounded down to 16 byte boundary. remaining bytes in lastSize */
        }
    }

    max_ltc_fifo_size = LTC_FIFO_SZ_MAX_DOWN_ALGN;
    fifoConsumed = base->DS;

    while (lastSize)
    {
        switch (fsm)
        {
            case kLTC_ModeUpdate:
            case kLTC_ModeInit:
                while (inSize)
                {
                    if (inSize > (max_ltc_fifo_size - fifoConsumed))
                    {
                        sz = (max_ltc_fifo_size - fifoConsumed);
                    }
                    else
                    {
                        sz = inSize;
                    }
                    base->DS = sz;
                    ltc_symmetric_process(base, sz, &inData, &outData);
                    inSize -= sz;
                    fifoConsumed = 0;

                    /* after we completed INITIALIZE job, are there still any data left? */
                    if (inSize)
                    {
                        fsm = kLTC_ModeUpdate;
                        status = ltc_wait(base);
                        if (kStatus_Success != status)
                        {
                            return status;
                        }
                        modeReg &= ~LTC_MD_AS_MASK;
                        modeReg |= (uint32_t)fsm;
                        base->MD = modeReg;
                    }
                    else
                    {
                        fsm = lastAs;
                    }
                }
                break;

            case kLTC_ModeFinalize:
            case kLTC_ModeInitFinal:
                /* process last block in FINALIZE */

                status = ltc_wait(base);
                if (kStatus_Success != status)
                {
                    return status;
                }

                modeReg &= ~LTC_MD_AS_MASK;
                modeReg |= (uint32_t)lastAs;
                base->MD = modeReg;

                base->DS = lastSize;
                ltc_symmetric_process(base, lastSize, &inData, &outData);
                lastSize = 0;
                break;

            default:
                break;
        }
    }

    status = ltc_wait(base);
    return status;
}

/*!
 * @brief Receives MAC compare.
 *
 * This function is a sub-process of CCM and GCM decryption.
 * It compares received MAC with the MAC computed during decryption.
 *
 * @param base LTC peripheral base address
 * @param tag Received MAC.
 * @param tagSize Number of bytes in the received MAC.
 * @param modeReg LTC Mode Register current value. It is modified and written to LTC Mode Register.
 */
static status_t ltc_aes_received_mac_compare(LTC_Type *base, const uint8_t *tag, uint32_t tagSize, ltc_mode_t modeReg)
{
    ltc_xcm_block_t blk = {{0x0u, 0x0u, 0x0u, 0x0u}};

    base->CW = kLTC_ClearDataSize;
    base->STA = kLTC_StatusDoneIsr;

    modeReg &= ~LTC_MD_AS_MASK;
    modeReg |= (uint32_t)kLTC_ModeUpdate | LTC_MD_ICV_TEST_MASK;
    base->MD = modeReg;

    base->DS = 0u;
    base->ICVS = tagSize;
    ltc_memcpy(&blk.b[0], &tag[0], tagSize);

    ltc_move_block_to_ififo(base, &blk, tagSize);
    return ltc_wait(base);
}

/*!
 * @brief Processes tag during AES GCM and CCM.
 *
 * This function is a sub-process of CCM and GCM encryption and decryption.
 * For encryption, it writes computed MAC to the output tag.
 * For decryption, it compares the received MAC with the computed MAC.
 *
 * @param base LTC peripheral base address
 * @param[in,out] tag Output computed MAC during encryption or Input received MAC during decryption.
 * @param tagSize Size of MAC buffer in bytes.
 * @param modeReg LTC Mode Register current value. It is checked to read Enc/Dec bit.
 *                 It is modified and written to LTC Mode Register during decryption.
 * @param ctx Index to LTC context registers with computed MAC for encryption process.
 */
static status_t ltc_aes_process_tag(LTC_Type *base, uint8_t *tag, uint32_t tagSize, ltc_mode_t modeReg, uint32_t ctx)
{
    status_t status = kStatus_Success;
    if (tag)
    {
        /* For decrypt, compare received MAC with the computed MAC. */
        if (kLTC_ModeDecrypt == (modeReg & LTC_MD_ENC_MASK))
        {
            status = ltc_aes_received_mac_compare(base, tag, tagSize, modeReg);
        }
        else /* FSL_AES_GCM_TYPE_ENCRYPT */
        {
            /* For encryption, write the computed and encrypted MAC to user buffer */
            ltc_get_context(base, &tag[0], tagSize, ctx);
        }
    }
    return status;
}

/*******************************************************************************
 * LTC Common code public
 ******************************************************************************/
void LTC_Init(LTC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* ungate clock */
    CLOCK_EnableClock(kCLOCK_Ltc0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void LTC_Deinit(LTC_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* gate clock */
    CLOCK_DisableClock(kCLOCK_Ltc0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

#if defined(FSL_FEATURE_LTC_HAS_DPAMS) && FSL_FEATURE_LTC_HAS_DPAMS
void LTC_SetDpaMaskSeed(LTC_Type *base, uint32_t mask)
{
    base->DPAMS = mask;
    /* second write as workaround for DPA mask re-seed errata */
    base->DPAMS = mask;
}
#endif /* FSL_FEATURE_LTC_HAS_DPAMS */

/*******************************************************************************
 * AES Code static
 ******************************************************************************/
static status_t ltc_aes_decrypt_ecb(LTC_Type *base,
                                    const uint8_t *ciphertext,
                                    uint8_t *plaintext,
                                    uint32_t size,
                                    const uint8_t *key,
                                    uint32_t keySize,
                                    ltc_aes_key_t keyType)
{
    status_t retval;

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeECB, kLTC_ModeDecrypt);

    /* set DK bit in the LTC Mode Register AAI field for directly loaded decrypt keys */
    if (keyType == kLTC_DecryptKey)
    {
        base->MD |= (1U << kLTC_ModeRegBitShiftDK);
    }

    /* Process data and return status. */
    retval = ltc_process_message_in_sessions(base, &ciphertext[0], size, &plaintext[0]);
    return retval;
}

/*******************************************************************************
 * AES Code public
 ******************************************************************************/
status_t LTC_AES_GenerateDecryptKey(LTC_Type *base, const uint8_t *encryptKey, uint8_t *decryptKey, uint32_t keySize)
{
    uint8_t plaintext[LTC_AES_BLOCK_SIZE];
    uint8_t ciphertext[LTC_AES_BLOCK_SIZE];
    status_t status;

    if (!ltc_check_key_size(keySize))
    {
        return kStatus_InvalidArgument;
    }

    /* ECB decrypt with encrypt key will convert the key in LTC context into decrypt form of the key */
    status = ltc_aes_decrypt_ecb(base, ciphertext, plaintext, LTC_AES_BLOCK_SIZE, encryptKey, keySize, kLTC_EncryptKey);
    /* now there is decrypt form of the key in the LTC context, so take it */
    ltc_get_key(base, decryptKey, keySize);

    ltc_clear_all(base, false);

    return status;
}

status_t LTC_AES_EncryptEcb(
    LTC_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, uint32_t size, const uint8_t *key, uint32_t keySize)
{
    status_t retval;

    if (!ltc_check_key_size(keySize))
    {
        return kStatus_InvalidArgument;
    }
    /* ECB mode, size must be 16-byte multiple */
    if ((size < 16u) || (size % 16u))
    {
        return kStatus_InvalidArgument;
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeECB, kLTC_ModeEncrypt);

    /* Process data and return status. */
    retval = ltc_process_message_in_sessions(base, &plaintext[0], size, &ciphertext[0]);
    ltc_clear_all(base, false);
    return retval;
}

status_t LTC_AES_DecryptEcb(LTC_Type *base,
                            const uint8_t *ciphertext,
                            uint8_t *plaintext,
                            uint32_t size,
                            const uint8_t *key,
                            uint32_t keySize,
                            ltc_aes_key_t keyType)
{
    status_t status;

    if (!ltc_check_key_size(keySize))
    {
        return kStatus_InvalidArgument;
    }
    /* ECB mode, size must be 16-byte multiple */
    if ((size < 16u) || (size % 16u))
    {
        return kStatus_InvalidArgument;
    }

    status = ltc_aes_decrypt_ecb(base, ciphertext, plaintext, size, key, keySize, keyType);
    ltc_clear_all(base, false);
    return status;
}

status_t LTC_AES_EncryptCbc(LTC_Type *base,
                            const uint8_t *plaintext,
                            uint8_t *ciphertext,
                            uint32_t size,
                            const uint8_t iv[LTC_AES_IV_SIZE],
                            const uint8_t *key,
                            uint32_t keySize)
{
    status_t retval;

    if (!ltc_check_key_size(keySize))
    {
        return kStatus_InvalidArgument;
    }

    /* CBC mode, size must be 16-byte multiple */
    if ((size < 16u) || (size % 16u))
    {
        return kStatus_InvalidArgument;
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeCBC, kLTC_ModeEncrypt);

    /* Write IV data to the context register. */
    ltc_set_context(base, &iv[0], LTC_AES_IV_SIZE, 0);

    /* Process data and return status. */
    retval = ltc_process_message_in_sessions(base, &plaintext[0], size, &ciphertext[0]);
    ltc_clear_all(base, false);
    return retval;
}

status_t LTC_AES_DecryptCbc(LTC_Type *base,
                            const uint8_t *ciphertext,
                            uint8_t *plaintext,
                            uint32_t size,
                            const uint8_t iv[LTC_AES_IV_SIZE],
                            const uint8_t *key,
                            uint32_t keySize,
                            ltc_aes_key_t keyType)
{
    status_t retval;

    if (!ltc_check_key_size(keySize))
    {
        return kStatus_InvalidArgument;
    }
    /* CBC mode, size must be 16-byte multiple */
    if ((size < 16u) || (size % 16u))
    {
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
    retval = ltc_process_message_in_sessions(base, &ciphertext[0], size, &plaintext[0]);
    ltc_clear_all(base, false);
    return retval;
}

status_t LTC_AES_CryptCtr(LTC_Type *base,
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
    retval = ltc_process_message_in_sessions(base, input, size, output);
    if (kStatus_Success != retval)
    {
        return retval;
    }

    input += size;
    output += size;

    if ((counterlast != NULL) && lastSize)
    {
        uint8_t zeroes[16] = {0};
        ltc_mode_t modeReg;

        modeReg = (uint32_t)kLTC_AlgorithmAES | (uint32_t)kLTC_ModeCTR | (uint32_t)kLTC_ModeEncrypt;
        /* Write the mode register to the hardware. */
        base->MD = modeReg | (uint32_t)kLTC_ModeFinalize;

        /* context is re-used (CTRi) */

        /* Process data and return status. */
        retval = ltc_symmetric_process_data(base, input, lastSize, output);
        if (kStatus_Success != retval)
        {
            return retval;
        }
        if (szLeft)
        {
            *szLeft = 16U - lastSize;
        }

        /* Initialize algorithm state. */
        base->MD = modeReg | (uint32_t)kLTC_ModeUpdate;

        /* context is re-used (CTRi) */

        /* Process data and return status. */
        retval = ltc_symmetric_process_data(base, zeroes, 16U, counterlast);
    }
    ltc_get_context(base, &counter[0], 16U, 4U);
    ltc_clear_all(base, false);
    return retval;
}

#if defined(FSL_FEATURE_LTC_HAS_GCM) && FSL_FEATURE_LTC_HAS_GCM
/*******************************************************************************
 * GCM Code static
 ******************************************************************************/
static status_t ltc_aes_gcm_check_input_args(LTC_Type *base,
                                             const uint8_t *src,
                                             const uint8_t *iv,
                                             const uint8_t *aad,
                                             const uint8_t *key,
                                             uint8_t *dst,
                                             uint32_t inputSize,
                                             uint32_t ivSize,
                                             uint32_t aadSize,
                                             uint32_t keySize,
                                             uint32_t tagSize)
{
    if (!base)
    {
        return kStatus_InvalidArgument;
    }

    /* tag can be NULL to skip tag processing */
    if ((!key) || (ivSize && (!iv)) || (aadSize && (!aad)) || (inputSize && ((!src) || (!dst))))
    {
        return kStatus_InvalidArgument;
    }

    /* octet length of tag (tagSize) must be element of 4,8,12,13,14,15,16 */
    if (((tagSize > 16u) || (tagSize < 12u)) && (tagSize != 4u) && (tagSize != 8u))
    {
        return kStatus_InvalidArgument;
    }

    /* check if keySize is supported */
    if (!ltc_check_key_size(keySize))
    {
        return kStatus_InvalidArgument;
    }

    /* no IV AAD DATA makes no sense */
    if (0 == (inputSize + ivSize + aadSize))
    {
        return kStatus_InvalidArgument;
    }

    return kStatus_Success;
}

/*!
 * @brief Process Wrapper for void (*pfunc)(LTC_Type*, uint32_t, bool). Sets IV Size register.
 */
static void ivsize_next(LTC_Type *base, uint32_t ivSize, bool iv_only)
{
    base->IVSZ = LTC_IVSZ_IL(iv_only) | ((ivSize)&LTC_DS_DS_MASK);
}

/*!
 * @brief Process Wrapper for void (*pfunc)(LTC_Type*, uint32_t, bool). Sets AAD Size register.
 */
static void aadsize_next(LTC_Type *base, uint32_t aadSize, bool aad_only)
{
    base->AADSZ = LTC_AADSZ_AL(aad_only) | ((aadSize)&LTC_DS_DS_MASK);
}

/*!
 * @brief Process IV or AAD string in multi-session.
 *
 * @param base LTC peripheral base address
 * @param iv IV or AAD data
 * @param ivSize Size in bytes of IV or AAD data
 * @param modeReg LTC peripheral Mode register value
 * @param iv_only IV only or AAD only flag
 * @param type selects between IV or AAD
 */
static status_t ltc_aes_gcm_process_iv_aad(
    LTC_Type *base, const uint8_t *iv, uint32_t ivSize, ltc_mode_t modeReg, bool iv_only, int type, ltc_mode_t modeLast)
{
    uint32_t sz;
    status_t retval;
    void (*next_size_func)(LTC_Type *ltcBase, uint32_t nextSize, bool authOnly);

    if ((NULL == iv) || (ivSize == 0))
    {
        return kStatus_InvalidArgument;
    }

    sz = LTC_FIFO_SZ_MAX_DOWN_ALGN;
    next_size_func = type == LTC_AES_GCM_TYPE_AAD ? aadsize_next : ivsize_next;

    while (ivSize)
    {
        if (ivSize < sz)
        {
            modeReg &= ~LTC_MD_AS_MASK;
            modeReg |= modeLast;
            base->MD = modeReg;
            next_size_func(base, ivSize, iv_only);
            ltc_move_to_ififo(base, iv, ivSize);
            ivSize = 0;
        }
        else
        {
            /* set algorithm state to UPDATE */
            modeReg &= ~LTC_MD_AS_MASK;
            modeReg |= kLTC_ModeUpdate;
            base->MD = modeReg;

            next_size_func(base, (uint16_t)sz, true);
            ltc_move_to_ififo(base, iv, sz);
            ivSize -= sz;
            iv += sz;
        }

        retval = ltc_wait(base);
        if (kStatus_Success != retval)
        {
            return retval;
        }
    } /* end while */
    return kStatus_Success;
}

static status_t ltc_aes_gcm_process(LTC_Type *base,
                                    ltc_mode_encrypt_t encryptMode,
                                    const uint8_t *src,
                                    uint32_t inputSize,
                                    const uint8_t *iv,
                                    uint32_t ivSize,
                                    const uint8_t *aad,
                                    uint32_t aadSize,
                                    const uint8_t *key,
                                    uint32_t keySize,
                                    uint8_t *dst,
                                    uint8_t *tag,
                                    uint32_t tagSize)
{
    status_t retval;          /* return value */
    uint32_t max_ltc_fifo_sz; /* maximum data size that we can put to LTC FIFO in one session. 12-bit limit. */
    ltc_mode_t modeReg;       /* read and write LTC mode register */

    bool single_ses_proc_all; /* iv, aad and src data can be processed in one session */
    bool iv_only;
    bool aad_only;

    retval = ltc_aes_gcm_check_input_args(base, src, iv, aad, key, dst, inputSize, ivSize, aadSize, keySize, tagSize);

    /* API input validation */
    if (kStatus_Success != retval)
    {
        return retval;
    }

    max_ltc_fifo_sz = LTC_DS_DS_MASK; /* 12-bit field limit */

    /*
     * Write value to LTC AADSIZE (rounded up to next 16 byte boundary)
     * plus the write value to LTC IV (rounded up to next 16 byte boundary)
     * plus the inputSize. If the result is less than max_ltc_fifo_sz
     * then all can be processed in one session FINALIZE.
     * Otherwise, we have to split into multiple session, going through UPDATE(s), INITIALIZE, UPDATE(s) and FINALIZE.
     */
    single_ses_proc_all =
        (((aadSize + 15u) & 0xfffffff0u) + ((ivSize + 15u) & 0xfffffff0u) + inputSize) <= max_ltc_fifo_sz;

    /* setup key, algorithm and set the alg.state */
    if (single_ses_proc_all)
    {
        ltc_symmetric_final(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeGCM, encryptMode);
        modeReg = base->MD;

        iv_only = (aadSize == 0) && (inputSize == 0);
        aad_only = (inputSize == 0);

        /* DS_MASK here is not a bug. IV size field can be written with more than 4-bits,
         * as the IVSZ write value, aligned to next 16 bytes boundary, is written also to the Data Size.
         * For example, I can write 22 to IVSZ, 32 will be written to Data Size and IVSZ will have value 6, which is 22
         * mod 16.
         */
        base->IVSZ = LTC_IVSZ_IL(iv_only) | ((ivSize)&LTC_DS_DS_MASK);
        ltc_move_to_ififo(base, iv, ivSize);
        if (iv_only && ivSize)
        {
            retval = ltc_wait(base);
            if (kStatus_Success != retval)
            {
                return retval;
            }
        }
        base->AADSZ = LTC_AADSZ_AL(aad_only) | ((aadSize)&LTC_DS_DS_MASK);
        ltc_move_to_ififo(base, aad, aadSize);
        if (aad_only && aadSize)
        {
            retval = ltc_wait(base);
            if (kStatus_Success != retval)
            {
                return retval;
            }
        }

        if (inputSize)
        {
            /* Workaround for the LTC Data Size register update errata TKT261180 */
            while (16U < base->DS)
            {
            }

            ltc_symmetric_process_data(base, &src[0], inputSize, &dst[0]);
        }
    }
    else
    {
        ltc_symmetric_init(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeGCM, encryptMode);
        modeReg = base->MD;

        /* process IV */
        if (ivSize)
        {
            /* last chunk of IV is always INITIALIZE (for GHASH to occur) */
            retval = ltc_aes_gcm_process_iv_aad(base, iv, ivSize, modeReg, true, LTC_AES_GCM_TYPE_IV, kLTC_ModeInit);
            if (kStatus_Success != retval)
            {
                return retval;
            }
        }

        /* process AAD */
        if (aadSize)
        {
            /* AS mode to process last chunk of AAD. it differs if we are in GMAC or GCM */
            ltc_mode_t lastModeReg;
            if (0 == inputSize)
            {
                /* if there is no DATA, set mode to compute final MAC. this is GMAC mode */
                lastModeReg = kLTC_ModeInitFinal;
            }
            else
            {
                /* there are confidential DATA. so process last chunk of AAD in UPDATE mode */
                lastModeReg = kLTC_ModeUpdate;
            }
            retval = ltc_aes_gcm_process_iv_aad(base, aad, aadSize, modeReg, true, LTC_AES_GCM_TYPE_AAD, lastModeReg);
            if (kStatus_Success != retval)
            {
                return retval;
            }
        }

        /* there are DATA. */
        if (inputSize)
        {
            /* set algorithm state to UPDATE */
            modeReg &= ~LTC_MD_AS_MASK;
            modeReg |= kLTC_ModeUpdate;
            base->MD = modeReg;
            retval =
                ltc_symmetric_process_data_multiple(base, &src[0], inputSize, &dst[0], modeReg, kLTC_ModeInitFinal);
        }
    }
    if (kStatus_Success != retval)
    {
        return retval;
    }
    retval = ltc_aes_process_tag(base, tag, tagSize, modeReg, LTC_GCM_TAG_IDX);
    return retval;
}

/*******************************************************************************
 * GCM Code public
 ******************************************************************************/
status_t LTC_AES_EncryptTagGcm(LTC_Type *base,
                               const uint8_t *plaintext,
                               uint8_t *ciphertext,
                               uint32_t size,
                               const uint8_t *iv,
                               uint32_t ivSize,
                               const uint8_t *aad,
                               uint32_t aadSize,
                               const uint8_t *key,
                               uint32_t keySize,
                               uint8_t *tag,
                               uint32_t tagSize)
{
    status_t status;

    status = ltc_aes_gcm_process(base, kLTC_ModeEncrypt, plaintext, size, iv, ivSize, aad, aadSize, key, keySize,
                                 ciphertext, tag, tagSize);

    ltc_clear_all(base, false);
    return status;
}

status_t LTC_AES_DecryptTagGcm(LTC_Type *base,
                               const uint8_t *ciphertext,
                               uint8_t *plaintext,
                               uint32_t size,
                               const uint8_t *iv,
                               uint32_t ivSize,
                               const uint8_t *aad,
                               uint32_t aadSize,
                               const uint8_t *key,
                               uint32_t keySize,
                               const uint8_t *tag,
                               uint32_t tagSize)
{
    uint8_t temp_tag[16] = {0}; /* max. octet length of Integrity Check Value ICV (tag) is 16 */
    uint8_t *tag_ptr;
    status_t status;

    tag_ptr = NULL;
    if (tag)
    {
        ltc_memcpy(temp_tag, tag, tagSize);
        tag_ptr = &temp_tag[0];
    }
    status = ltc_aes_gcm_process(base, kLTC_ModeDecrypt, ciphertext, size, iv, ivSize, aad, aadSize, key, keySize,
                                 plaintext, tag_ptr, tagSize);

    ltc_clear_all(base, false);
    return status;
}
#endif /* FSL_FEATURE_LTC_HAS_GCM */

/*******************************************************************************
 * CCM Code static
 ******************************************************************************/
static status_t ltc_aes_ccm_check_input_args(LTC_Type *base,
                                             const uint8_t *src,
                                             const uint8_t *iv,
                                             const uint8_t *key,
                                             uint8_t *dst,
                                             uint32_t ivSize,
                                             uint32_t aadSize,
                                             uint32_t keySize,
                                             uint32_t tagSize)
{
    if (!base)
    {
        return kStatus_InvalidArgument;
    }

    /* tag can be NULL to skip tag processing */
    if ((!src) || (!iv) || (!key) || (!dst))
    {
        return kStatus_InvalidArgument;
    }

    /* size of Nonce (ivSize) must be element of 7,8,9,10,11,12,13 */
    if ((ivSize < 7u) || (ivSize > 13u))
    {
        return kStatus_InvalidArgument;
    }
    /* octet length of MAC (tagSize) must be element of 4,6,8,10,12,14,16 for tag processing or zero to skip tag
     * processing */
    if (((tagSize > 0) && (tagSize < 4u)) || (tagSize > 16u) || (tagSize & 1u))
    {
        return kStatus_InvalidArgument;
    }

    /* check if keySize is supported */
    if (!ltc_check_key_size(keySize))
    {
        return kStatus_InvalidArgument;
    }

    /* LTC does not support more AAD than this */
    if (aadSize >= 65280u)
    {
        return kStatus_InvalidArgument;
    }
    return kStatus_Success;
}

static uint32_t swap_bytes(uint32_t in)
{
    return (((in & 0x000000ffu) << 24) | ((in & 0x0000ff00u) << 8) | ((in & 0x00ff0000u) >> 8) |
            ((in & 0xff000000u) >> 24));
}

static void ltc_aes_ccm_context_init(
    LTC_Type *base, uint32_t inputSize, const uint8_t *iv, uint32_t ivSize, uint32_t aadSize, uint32_t tagSize)
{
    ltc_xcm_block_t blk;
    ltc_xcm_block_t blkZero = {{0x0u, 0x0u, 0x0u, 0x0u}};

    int q; /* octet length of binary representation of the octet length of the payload. computed as (15 - n), where n is
              length of nonce(=ivSize) */
    uint8_t flags; /* flags field in B0 and CTR0 */

    /* compute B0 */
    ltc_memcpy(&blk, &blkZero, sizeof(blk));
    /* tagSize - size of output MAC */
    q = 15 - ivSize;
    flags = (uint8_t)(8 * ((tagSize - 2) / 2) + q - 1); /* 8*M' + L' */
    if (aadSize)
    {
        flags |= 0x40; /* Adata */
    }
    blk.b[0] = flags;                  /* flags field */
    blk.w[3] = swap_bytes(inputSize);  /* message size, most significant byte first */
    ltc_memcpy(&blk.b[1], iv, ivSize); /* nonce field */

    /* Write B0 data to the context register.
     */
    ltc_set_context(base, &blk.b[0], 16, 0);

    /* Write CTR0 to the context register.
     */
    ltc_memcpy(&blk, &blkZero, sizeof(blk)); /* ctr(0) field = zero */
    blk.b[0] = q - 1;                        /* flags field */
    ltc_memcpy(&blk.b[1], iv, ivSize);       /* nonce field */
    ltc_set_context(base, &blk.b[0], 16, 4);
}

static status_t ltc_aes_ccm_process_aad(
    LTC_Type *base, uint32_t inputSize, const uint8_t *aad, uint32_t aadSize, ltc_mode_t *modeReg)
{
    ltc_xcm_block_t blk = {{0x0u, 0x0u, 0x0u, 0x0u}};
    uint32_t swapped; /* holds byte swap of uint32_t */
    status_t retval;

    if (aadSize)
    {
        bool aad_only;
        bool aad_single_session;

        uint32_t sz = 0;

        aad_only = inputSize == 0u;
        aad_single_session = (((aadSize + 2u) + 15u) & 0xfffffff0u) <= LTC_FIFO_SZ_MAX_DOWN_ALGN;

        /* limit by CCM spec: 2^16 - 2^8 = 65280 */

        /* encoding is two octets, msbyte first */
        swapped = swap_bytes(aadSize);
        ltc_memcpy(&blk.b[0], ((uint8_t *)&swapped) + sizeof(uint16_t), sizeof(uint16_t));

        sz = aadSize > 14u ? 14u : aadSize; /* limit aad to the end of 16 bytes blk */
        ltc_memcpy(&blk.b[2], aad, sz);     /* fill B1 with aad */

        if (aad_single_session)
        {
            base->AADSZ = LTC_AADSZ_AL(aad_only) | ((aadSize + 2U) & LTC_DS_DS_MASK);
            /* move first AAD block (16 bytes block B1) to FIFO */
            ltc_move_block_to_ififo(base, &blk, sizeof(blk));
        }
        else
        {
            base->AADSZ = LTC_AADSZ_AL(true) | (16U);
            /* move first AAD block (16 bytes block B1) to FIFO */
            ltc_move_block_to_ififo(base, &blk, sizeof(blk));
        }

        /* track consumed AAD. sz bytes have been moved to fifo. */
        aadSize -= sz;
        aad += sz;

        if (aad_single_session)
        {
            /* move remaining AAD to FIFO, then return, to continue with MDATA */
            ltc_move_to_ififo(base, aad, aadSize);
        }
        else if (aadSize == 0u)
        {
            retval = ltc_wait(base);
            if (kStatus_Success != retval)
            {
                return retval;
            }
        }
        else
        {
            while (aadSize)
            {
                retval = ltc_wait(base);
                if (kStatus_Success != retval)
                {
                    return retval;
                }

                *modeReg &= ~LTC_MD_AS_MASK;
                *modeReg |= (uint32_t)kLTC_ModeUpdate;
                base->MD = *modeReg;

                sz = LTC_FIFO_SZ_MAX_DOWN_ALGN;
                if (aadSize < sz)
                {
                    base->AADSZ = LTC_AADSZ_AL(aad_only) | (aadSize & LTC_DS_DS_MASK);
                    ltc_move_to_ififo(base, aad, aadSize);
                    aadSize = 0;
                }
                else
                {
                    base->AADSZ = LTC_AADSZ_AL(true) | (sz & LTC_DS_DS_MASK);
                    ltc_move_to_ififo(base, aad, sz);
                    aadSize -= sz;
                    aad += sz;
                }
            } /* end while */
        }     /* end else */
    }         /* end if */
    return kStatus_Success;
}

static status_t ltc_aes_ccm_process(LTC_Type *base,
                                    ltc_mode_encrypt_t encryptMode,
                                    const uint8_t *src,
                                    uint32_t inputSize,
                                    const uint8_t *iv,
                                    uint32_t ivSize,
                                    const uint8_t *aad,
                                    uint32_t aadSize,
                                    const uint8_t *key,
                                    uint32_t keySize,
                                    uint8_t *dst,
                                    uint8_t *tag,
                                    uint32_t tagSize)
{
    status_t retval;          /* return value */
    uint32_t max_ltc_fifo_sz; /* maximum data size that we can put to LTC FIFO in one session. 12-bit limit. */
    ltc_mode_t modeReg;       /* read and write LTC mode register */

    bool single_ses_proc_all; /* aad and src data can be processed in one session */

    retval = ltc_aes_ccm_check_input_args(base, src, iv, key, dst, ivSize, aadSize, keySize, tagSize);

    /* API input validation */
    if (kStatus_Success != retval)
    {
        return retval;
    }

    max_ltc_fifo_sz = LTC_DS_DS_MASK; /* 12-bit field limit */

    /* Write value to LTC AADSIZE will be (aadSize+2) value.
     * The value will be rounded up to next 16 byte boundary and added to Data Size register.
     * We then add inputSize to Data Size register. If the resulting Data Size is less than max_ltc_fifo_sz
     * then all can be processed in one session INITIALIZE/FINALIZE.
     * Otherwise, we have to split into multiple session, going through INITIALIZE, UPDATE (if required) and FINALIZE.
     */
    single_ses_proc_all = ((((aadSize + 2) + 15u) & 0xfffffff0u) + inputSize) <= max_ltc_fifo_sz;

    /* setup key, algorithm and set the alg.state to INITIALIZE */
    if (single_ses_proc_all)
    {
        ltc_symmetric_init_final(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeCCM, encryptMode);
    }
    else
    {
        ltc_symmetric_init(base, key, keySize, kLTC_AlgorithmAES, kLTC_ModeCCM, encryptMode);
    }
    modeReg = base->MD;

    /* Initialize LTC context for AES CCM: block B0 and initial counter CTR0 */
    ltc_aes_ccm_context_init(base, inputSize, iv, ivSize, aadSize, tagSize);

    /* Process additional authentication data, if there are any.
     * Need to split the job into individual sessions of up to 4096 bytes, due to LTC IFIFO data size limit.
     */
    retval = ltc_aes_ccm_process_aad(base, inputSize, aad, aadSize, &modeReg);
    if (kStatus_Success != retval)
    {
        return retval;
    }

    /* Workaround for the LTC Data Size register update errata TKT261180 */
    if (inputSize)
    {
        while (16u < base->DS)
        {
        }
    }

    /* Process message */
    if (single_ses_proc_all)
    {
        retval = ltc_symmetric_process_data(base, &src[0], inputSize, &dst[0]);
    }
    else
    {
        retval = ltc_symmetric_process_data_multiple(base, &src[0], inputSize, &dst[0], modeReg, kLTC_ModeFinalize);
    }
    if (kStatus_Success != retval)
    {
        return retval;
    }
    retval = ltc_aes_process_tag(base, tag, tagSize, modeReg, LTC_CCM_TAG_IDX);
    return retval;
}

/*******************************************************************************
 * CCM Code public
 ******************************************************************************/
status_t LTC_AES_EncryptTagCcm(LTC_Type *base,
                               const uint8_t *plaintext,
                               uint8_t *ciphertext,
                               uint32_t size,
                               const uint8_t *iv,
                               uint32_t ivSize,
                               const uint8_t *aad,
                               uint32_t aadSize,
                               const uint8_t *key,
                               uint32_t keySize,
                               uint8_t *tag,
                               uint32_t tagSize)
{
    status_t status;
    status = ltc_aes_ccm_process(base, kLTC_ModeEncrypt, plaintext, size, iv, ivSize, aad, aadSize, key, keySize,
                                 ciphertext, tag, tagSize);

    ltc_clear_all(base, false);
    return status;
}

status_t LTC_AES_DecryptTagCcm(LTC_Type *base,
                               const uint8_t *ciphertext,
                               uint8_t *plaintext,
                               uint32_t size,
                               const uint8_t *iv,
                               uint32_t ivSize,
                               const uint8_t *aad,
                               uint32_t aadSize,
                               const uint8_t *key,
                               uint32_t keySize,
                               const uint8_t *tag,
                               uint32_t tagSize)
{
    uint8_t temp_tag[16] = {0}; /* max. octet length of MAC (tag) is 16 */
    uint8_t *tag_ptr;
    status_t status;

    tag_ptr = NULL;
    if (tag)
    {
        ltc_memcpy(temp_tag, tag, tagSize);
        tag_ptr = &temp_tag[0];
    }

    status = ltc_aes_ccm_process(base, kLTC_ModeDecrypt, ciphertext, size, iv, ivSize, aad, aadSize, key, keySize,
                                 plaintext, tag_ptr, tagSize);

    ltc_clear_all(base, false);
    return status;
}

#if defined(FSL_FEATURE_LTC_HAS_DES) && FSL_FEATURE_LTC_HAS_DES
/*******************************************************************************
 * DES / 3DES Code static
 ******************************************************************************/
static status_t ltc_des_process(LTC_Type *base,
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
        return kStatus_InvalidArgument;
    }

    /* Initialize algorithm state. */
    ltc_symmetric_update(base, &key[0], LTC_DES_KEY_SIZE, kLTC_AlgorithmDES, modeAs, modeEnc);

    if ((modeAs != kLTC_ModeECB))
    {
        ltc_set_context(base, iv, LTC_DES_IV_SIZE, 0);
    }

    /* Process data and return status. */
    retval = ltc_process_message_in_sessions(base, input, size, output);
    ltc_clear_all(base, false);
    return retval;
}

status_t ltc_3des_check_input_args(ltc_mode_symmetric_alg_t modeAs,
                                   uint32_t size,
                                   const uint8_t *key1,
                                   const uint8_t *key2)
{
    /* all but OFB, size must be 8-byte multiple */
    if ((modeAs != kLTC_ModeOFB) && ((size < 8u) || (size % 8u)))
    {
        return kStatus_InvalidArgument;
    }

    if ((key1 == NULL) || (key2 == NULL))
    {
        return kStatus_InvalidArgument;
    }
    return kStatus_Success;
}

static status_t ltc_3des_process(LTC_Type *base,
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
    retval = ltc_process_message_in_sessions(base, input, size, output);
    ltc_clear_all(base, false);
    return retval;
}
/*******************************************************************************
 * DES / 3DES Code public
 ******************************************************************************/
status_t LTC_DES_EncryptEcb(
    LTC_Type *base, const uint8_t *plaintext, uint8_t *ciphertext, uint32_t size, const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, plaintext, ciphertext, size, NULL, key, kLTC_ModeECB, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptEcb(
    LTC_Type *base, const uint8_t *ciphertext, uint8_t *plaintext, uint32_t size, const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, ciphertext, plaintext, size, NULL, key, kLTC_ModeECB, kLTC_ModeDecrypt);
}

status_t LTC_DES_EncryptCbc(LTC_Type *base,
                            const uint8_t *plaintext,
                            uint8_t *ciphertext,
                            uint32_t size,
                            const uint8_t iv[LTC_DES_IV_SIZE],
                            const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, plaintext, ciphertext, size, iv, key, kLTC_ModeCBC, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptCbc(LTC_Type *base,
                            const uint8_t *ciphertext,
                            uint8_t *plaintext,
                            uint32_t size,
                            const uint8_t iv[LTC_DES_IV_SIZE],
                            const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, ciphertext, plaintext, size, iv, key, kLTC_ModeCBC, kLTC_ModeDecrypt);
}

status_t LTC_DES_EncryptCfb(LTC_Type *base,
                            const uint8_t *plaintext,
                            uint8_t *ciphertext,
                            uint32_t size,
                            const uint8_t iv[LTC_DES_IV_SIZE],
                            const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, plaintext, ciphertext, size, iv, key, kLTC_ModeCFB, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptCfb(LTC_Type *base,
                            const uint8_t *ciphertext,
                            uint8_t *plaintext,
                            uint32_t size,
                            const uint8_t iv[LTC_DES_IV_SIZE],
                            const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, ciphertext, plaintext, size, iv, key, kLTC_ModeCFB, kLTC_ModeDecrypt);
}

status_t LTC_DES_EncryptOfb(LTC_Type *base,
                            const uint8_t *plaintext,
                            uint8_t *ciphertext,
                            uint32_t size,
                            const uint8_t iv[LTC_DES_IV_SIZE],
                            const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, plaintext, ciphertext, size, iv, key, kLTC_ModeOFB, kLTC_ModeEncrypt);
}

status_t LTC_DES_DecryptOfb(LTC_Type *base,
                            const uint8_t *ciphertext,
                            uint8_t *plaintext,
                            uint32_t size,
                            const uint8_t iv[LTC_DES_IV_SIZE],
                            const uint8_t key[LTC_DES_KEY_SIZE])
{
    return ltc_des_process(base, ciphertext, plaintext, size, iv, key, kLTC_ModeOFB, kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptEcb(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, NULL, key1, key2, NULL, kLTC_ModeECB, kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptEcb(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, NULL, key1, key2, key3, kLTC_ModeECB, kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptEcb(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, NULL, key1, key2, NULL, kLTC_ModeECB, kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptEcb(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, NULL, key1, key2, key3, kLTC_ModeECB, kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptCbc(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, iv, key1, key2, NULL, kLTC_ModeCBC, kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptCbc(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, iv, key1, key2, key3, kLTC_ModeCBC, kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptCbc(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, iv, key1, key2, NULL, kLTC_ModeCBC, kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptCbc(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, iv, key1, key2, key3, kLTC_ModeCBC, kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptCfb(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, iv, key1, key2, NULL, kLTC_ModeCFB, kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptCfb(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, iv, key1, key2, key3, kLTC_ModeCFB, kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptCfb(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, iv, key1, key2, NULL, kLTC_ModeCFB, kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptCfb(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, iv, key1, key2, key3, kLTC_ModeCFB, kLTC_ModeDecrypt);
}

status_t LTC_DES2_EncryptOfb(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, iv, key1, key2, NULL, kLTC_ModeOFB, kLTC_ModeEncrypt);
}

status_t LTC_DES3_EncryptOfb(LTC_Type *base,
                             const uint8_t *plaintext,
                             uint8_t *ciphertext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, plaintext, ciphertext, size, iv, key1, key2, key3, kLTC_ModeOFB, kLTC_ModeEncrypt);
}

status_t LTC_DES2_DecryptOfb(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, iv, key1, key2, NULL, kLTC_ModeOFB, kLTC_ModeDecrypt);
}

status_t LTC_DES3_DecryptOfb(LTC_Type *base,
                             const uint8_t *ciphertext,
                             uint8_t *plaintext,
                             uint32_t size,
                             const uint8_t iv[LTC_DES_IV_SIZE],
                             const uint8_t key1[LTC_DES_KEY_SIZE],
                             const uint8_t key2[LTC_DES_KEY_SIZE],
                             const uint8_t key3[LTC_DES_KEY_SIZE])
{
    return ltc_3des_process(base, ciphertext, plaintext, size, iv, key1, key2, key3, kLTC_ModeOFB, kLTC_ModeDecrypt);
}
#endif /* FSL_FEATURE_LTC_HAS_DES */

/*******************************************************************************
 * HASH Definitions
 ******************************************************************************/
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
#define LTC_SHA_BLOCK_SIZE 64                  /*!< SHA-1, SHA-224 & SHA-256 block size  */
#define LTC_HASH_BLOCK_SIZE LTC_SHA_BLOCK_SIZE /*!< LTC hash block size  */

enum _ltc_sha_digest_len
{
    kLTC_RunLenSha1 = 28u,
    kLTC_OutLenSha1 = 20u,
    kLTC_RunLenSha224 = 40u,
    kLTC_OutLenSha224 = 28u,
    kLTC_RunLenSha256 = 40u,
    kLTC_OutLenSha256 = 32u,
};
#else
#define LTC_HASH_BLOCK_SIZE LTC_AES_BLOCK_SIZE /*!< LTC hash block size  */
#endif                                         /* FSL_FEATURE_LTC_HAS_SHA */

/*! Internal states of the HASH creation process */
typedef enum _ltc_hash_algo_state
{
    kLTC_HashInit = 1u, /*!< Key in the HASH context is the input key. */
    kLTC_HashUpdate,    /*!< HASH context has algorithm specific context: MAC, K2 and K3 (XCBC-MAC), MAC and L (CMAC),
                           running digest (MDHA). Key in the HASH context is the derived key. */
} ltc_hash_algo_state_t;

/*! 16/64-byte block represented as byte array or 4/16 32-bit words */
typedef union _ltc_hash_block
{
    uint32_t w[LTC_HASH_BLOCK_SIZE / 4]; /*!< array of 32-bit words */
    uint8_t b[LTC_HASH_BLOCK_SIZE];      /*!< byte array */
} ltc_hash_block_t;

/*! Definitions of indexes into hash context array */
typedef enum _ltc_hash_ctx_indexes
{
    kLTC_HashCtxKeyStartIdx = 12, /*!< context word array index where key is stored */
    kLTC_HashCtxKeySize = 20,     /*!< context word array index where key size is stored */
    kLTC_HashCtxNumWords = 21,    /*!< number of context array 32-bit words  */
} ltc_hash_ctx_indexes;

typedef struct _ltc_hash_ctx_internal
{
    ltc_hash_block_t blk; /*!< memory buffer. only full 64/16-byte blocks are written to LTC during hash updates */
    uint32_t blksz;       /*!< number of valid bytes in memory buffer */
    LTC_Type *base;       /*!< LTC peripheral base address */
    ltc_hash_algo_t algo; /*!< selected algorithm from the set of supported algorithms in ltc_drv_hash_algo */
    ltc_hash_algo_state_t state;         /*!< finite machine state of the hash software process */
    uint32_t word[kLTC_HashCtxNumWords]; /*!< LTC module context that needs to be saved/restored between LTC jobs */
} ltc_hash_ctx_internal_t;

/*******************************************************************************
 * HASH Code static
 ******************************************************************************/
static status_t ltc_hash_check_input_alg(ltc_hash_algo_t algo)
{
    if ((algo != kLTC_XcbcMac) && (algo != kLTC_Cmac)
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
        && (algo != kLTC_Sha1) && (algo != kLTC_Sha224) && (algo != kLTC_Sha256)
#endif /* FSL_FEATURE_LTC_HAS_SHA */
            )
    {
        return kStatus_InvalidArgument;
    }
    return kStatus_Success;
}

static inline bool ltc_hash_alg_is_cmac(ltc_hash_algo_t algo)
{
    return ((algo == kLTC_XcbcMac) || (algo == kLTC_Cmac));
}

#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
static inline bool ltc_hash_alg_is_sha(ltc_hash_algo_t algo)
{
    return ((algo == kLTC_Sha1) || (algo == kLTC_Sha224) || (algo == kLTC_Sha256));
}
#endif /* FSL_FEATURE_LTC_HAS_SHA */

static status_t ltc_hash_check_input_args(
    LTC_Type *base, ltc_hash_ctx_t *ctx, ltc_hash_algo_t algo, const uint8_t *key, uint32_t keySize)
{
    /* Check validity of input algorithm */
    if (kStatus_Success != ltc_hash_check_input_alg(algo))
    {
        return kStatus_InvalidArgument;
    }

    if ((NULL == ctx) || (NULL == base))
    {
        return kStatus_InvalidArgument;
    }

    if (ltc_hash_alg_is_cmac(algo))
    {
        if ((NULL == key) || (!ltc_check_key_size(keySize)))
        {
            return kStatus_InvalidArgument;
        }
    }

    return kStatus_Success;
}

static status_t ltc_hash_check_context(ltc_hash_ctx_internal_t *ctxInternal, const uint8_t *data)
{
    if ((NULL == data) || (NULL == ctxInternal) || (NULL == ctxInternal->base) ||
        (kStatus_Success != ltc_hash_check_input_alg(ctxInternal->algo)))
    {
        return kStatus_InvalidArgument;
    }
    return kStatus_Success;
}

static uint32_t ltc_hash_algo2mode(ltc_hash_algo_t algo, ltc_mode_algorithm_state_t asMode, uint32_t *algOutSize)
{
    uint32_t modeReg = 0u;
    uint32_t outSize = 0u;

    /* Set LTC algorithm */
    switch (algo)
    {
        case kLTC_XcbcMac:
            modeReg = (uint32_t)kLTC_AlgorithmAES | (uint32_t)kLTC_ModeXCBCMAC;
            outSize = 16u;
            break;
        case kLTC_Cmac:
            modeReg = (uint32_t)kLTC_AlgorithmAES | (uint32_t)kLTC_ModeCMAC;
            outSize = 16u;
            break;
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
        case kLTC_Sha1:
            modeReg = (uint32_t)kLTC_AlgorithmSHA1;
            outSize = kLTC_OutLenSha1;
            break;
        case kLTC_Sha224:
            modeReg = (uint32_t)kLTC_AlgorithmSHA224;
            outSize = kLTC_OutLenSha224;
            break;
        case kLTC_Sha256:
            modeReg = (uint32_t)kLTC_AlgorithmSHA256;
            outSize = kLTC_OutLenSha256;
            break;
#endif /* FSL_FEATURE_LTC_HAS_SHA */
        default:
            break;
    }

    modeReg |= (uint32_t)asMode;
    if (algOutSize)
    {
        *algOutSize = outSize;
    }

    return modeReg;
}

static void ltc_hash_engine_init(ltc_hash_ctx_internal_t *ctx)
{
    uint8_t *key;
    uint32_t keySize;
    LTC_Type *base;
    ltc_mode_symmetric_alg_t algo;

    base = ctx->base;
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
    if (ltc_hash_alg_is_cmac(ctx->algo))
    {
#endif  /* FSL_FEATURE_LTC_HAS_SHA */
        /*
         *  word[kLtcCmacCtxKeySize] = key_length
         *  word[1-8] = key
         */
        keySize = ctx->word[kLTC_HashCtxKeySize];
        key = (uint8_t *)&ctx->word[kLTC_HashCtxKeyStartIdx];

        /* set LTC mode register to INITIALIZE */
        algo = (ctx->algo == kLTC_XcbcMac) ? kLTC_ModeXCBCMAC : kLTC_ModeCMAC;
        ltc_symmetric_init(base, key, keySize, kLTC_AlgorithmAES, algo, kLTC_ModeEncrypt);
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
    }
    else if (ltc_hash_alg_is_sha(ctx->algo))
    {
        /* Clear internal register states. */
        base->CW = (uint32_t)kLTC_ClearAll;

        /* Set byte swap on for several registers we will be reading and writing
         * user data to/from. */
        base->CTL |= kLTC_CtrlSwapAll;
    }
    else
    {
        /* do nothing in this case */
    }
#endif /* FSL_FEATURE_LTC_HAS_SHA */
}

static void ltc_hash_save_context(ltc_hash_ctx_internal_t *ctx)
{
    uint32_t sz;
    LTC_Type *base;

    base = ctx->base;
    /* Get context size */
    switch (ctx->algo)
    {
        case kLTC_XcbcMac:
            /*
            *  word[0-3] = mac
            *  word[3-7] = k3
            *  word[8-11] = k2
            *  word[kLtcCmacCtxKeySize] = keySize
            */
            sz = 12 * sizeof(uint32_t);
            break;
        case kLTC_Cmac:
            /*
            *  word[0-3] = mac
            *  word[3-7] = L */
            sz = 8 * sizeof(uint32_t);
            break;
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
        case kLTC_Sha1:
            sz = (kLTC_RunLenSha1);
            break;
        case kLTC_Sha224:
            sz = (kLTC_RunLenSha224);
            break;
        case kLTC_Sha256:
            sz = (kLTC_RunLenSha256);
            break;
#endif /* FSL_FEATURE_LTC_HAS_SHA */
        default:
            sz = 0;
            break;
    }

    ltc_get_context(base, (uint8_t *)&ctx->word[0], sz, 0);

    if (true == ltc_hash_alg_is_cmac(ctx->algo))
    {
        /* word[12-19] = key */
        ltc_get_key(base, (uint8_t *)&ctx->word[kLTC_HashCtxKeyStartIdx], ctx->word[kLTC_HashCtxKeySize]);
    }
}

static void ltc_hash_restore_context(ltc_hash_ctx_internal_t *ctx)
{
    uint32_t sz;
    uint32_t keySize;
    LTC_Type *base;

    base = ctx->base;
    /* Get context size */
    switch (ctx->algo)
    {
        case kLTC_XcbcMac:
            /*
            *  word[0-3] = mac
            *  word[3-7] = k3
            *  word[8-11] = k2
            *  word[kLtcCmacCtxKeySize] = keySize
            */
            sz = 12 * sizeof(uint32_t);
            break;
        case kLTC_Cmac:
            /*
            *  word[0-3] = mac
            *  word[3-7] = L */
            sz = 8 * sizeof(uint32_t);
            break;
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
        case kLTC_Sha1:
            sz = (kLTC_RunLenSha1);
            break;
        case kLTC_Sha224:
            sz = (kLTC_RunLenSha224);
            break;
        case kLTC_Sha256:
            sz = (kLTC_RunLenSha256);
            break;
#endif /* FSL_FEATURE_LTC_HAS_SHA */
        default:
            sz = 0;
            break;
    }

    ltc_set_context(base, (const uint8_t *)&ctx->word[0], sz, 0);

    if (ltc_hash_alg_is_cmac(ctx->algo))
    {
        /*
         *  word[12-19] = key
         *  word[kLtcCmacCtxKeySize] = keySize
         */
        base->CW = kLTC_ClearKey; /* clear Key and Key Size registers */

        keySize = ctx->word[kLTC_HashCtxKeySize];
        /* Write the key in place. */
        ltc_set_key(base, (const uint8_t *)&ctx->word[kLTC_HashCtxKeyStartIdx], keySize);

        /* Write the key size.  This must be done after writing the key, and this
         * action locks the ability to modify the key registers. */
        base->KS = keySize;
    }
}

static void ltc_hash_prepare_context_switch(LTC_Type *base)
{
    base->CW = (uint32_t)kLTC_ClearDataSize | (uint32_t)kLTC_ClearMode;
    base->STA = kLTC_StatusDoneIsr;
}

static uint32_t ltc_hash_get_block_size(ltc_hash_algo_t algo)
{
    if ((algo == kLTC_XcbcMac) || (algo == kLTC_Cmac))
    {
        return (uint32_t)LTC_AES_BLOCK_SIZE;
    }
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
    else if ((algo == kLTC_Sha1) || (algo == kLTC_Sha224) || (algo == kLTC_Sha256))
    {
        return (uint32_t)LTC_SHA_BLOCK_SIZE;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

static void ltc_hash_block_to_ififo(LTC_Type *base, const ltc_hash_block_t *blk, uint32_t numBytes, uint32_t blockSize)
{
    uint32_t i = 0;
    uint32_t words;

    words = numBytes / 4u;
    if (numBytes % 4u)
    {
        words++;
    }

    if (words > blockSize / 4u)
    {
        words = blockSize / 4u;
    }

    while (i < words)
    {
        if (0U == (base->FIFOSTA & LTC_FIFOSTA_IFF_MASK))
        {
            /* Copy data to the input FIFO. */
            base->IFIFO = blk->w[i++];
        }
    }
}

static void ltc_hash_move_to_ififo(ltc_hash_ctx_internal_t *ctx,
                                   const uint8_t *data,
                                   uint32_t dataSize,
                                   uint32_t blockSize)
{
    ltc_hash_block_t blkZero;
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(blkZero.w); i++)
    {
        blkZero.w[i] = 0;
    }

    while (dataSize)
    {
        if (dataSize >= blockSize)
        {
            ltc_memcpy(&ctx->blk, data, blockSize);
            ltc_hash_block_to_ififo(ctx->base, &ctx->blk, blockSize, blockSize);
            dataSize -= blockSize;
            data += blockSize;
        }
        else
        {
            /* last incomplete 16/64-bytes block of this message chunk */
            ltc_memcpy(&ctx->blk, &blkZero, sizeof(ctx->blk));
            ltc_memcpy(&ctx->blk, data, dataSize);
            ctx->blksz = dataSize;
            dataSize = 0;
        }
    }
}

static status_t ltc_hash_merge_and_flush_buf(ltc_hash_ctx_internal_t *ctx,
                                             const uint8_t *input,
                                             uint32_t inputSize,
                                             ltc_mode_t modeReg,
                                             uint32_t blockSize,
                                             uint32_t *consumedSize)
{
    uint32_t sz;
    LTC_Type *base;
    status_t status = kStatus_Success;

    base = ctx->base;
    sz = 0;
    if (ctx->blksz)
    {
        sz = blockSize - ctx->blksz;
        if (sz > inputSize)
        {
            sz = inputSize;
        }
        ltc_memcpy(ctx->blk.b + ctx->blksz, input, sz);
        input += sz;
        inputSize -= sz;
        ctx->blksz += sz;

        if (ctx->blksz == blockSize)
        {
            base->DS = blockSize;
            ltc_hash_block_to_ififo(base, &ctx->blk, blockSize, blockSize);
            ctx->blksz = 0;

            status = ltc_wait(base);
            if (kStatus_Success != status)
            {
                return status;
            }

            /* if there is still inputSize left, make sure LTC alg.state is set to UPDATE and continue */
            if (inputSize)
            {
                /* set algorithm state to UPDATE */
                modeReg &= ~LTC_MD_AS_MASK;
                modeReg |= kLTC_ModeUpdate;
                base->MD = modeReg;
            }
        }
    }
    if (consumedSize)
    {
        *consumedSize = sz;
    }
    return status;
}

static status_t ltc_hash_move_rest_to_context(
    ltc_hash_ctx_internal_t *ctx, const uint8_t *data, uint32_t dataSize, ltc_mode_t modeReg, uint32_t blockSize)
{
    status_t status = kStatus_Success;
    ltc_hash_block_t blkZero;
    uint32_t i;

    /* make blkZero clear */
    for (i = 0; i < ARRAY_SIZE(blkZero.w); i++)
    {
        blkZero.w[i] = 0;
    }

    while (dataSize)
    {
        if (dataSize > blockSize)
        {
            dataSize -= blockSize;
            data += blockSize;
        }
        else
        {
            if (dataSize + ctx->blksz > blockSize)
            {
                uint32_t sz;
                status = ltc_hash_merge_and_flush_buf(ctx, data, dataSize, modeReg, blockSize, &sz);
                if (kStatus_Success != status)
                {
                    return status;
                }
                data += sz;
                dataSize -= sz;
            }
            /* last incomplete 16/64-bytes block of this message chunk */
            ltc_memcpy(&ctx->blk, &blkZero, blockSize);
            ltc_memcpy(&ctx->blk, data, dataSize);
            ctx->blksz = dataSize;
            dataSize = 0;
        }
    }
    return status;
}

static status_t ltc_hash_process_input_data(ltc_hash_ctx_internal_t *ctx,
                                            const uint8_t *input,
                                            uint32_t inputSize,
                                            ltc_mode_t modeReg)
{
    uint32_t sz = 0;
    LTC_Type *base;
    uint32_t blockSize = 0;
    status_t status = kStatus_Success;

    blockSize = ltc_hash_get_block_size(ctx->algo);
    base = ctx->base;

    /* fill context struct blk and flush to LTC ififo in case it is full block */
    status = ltc_hash_merge_and_flush_buf(ctx, input, inputSize, modeReg, blockSize, &sz);
    if (kStatus_Success != status)
    {
        return status;
    }
    input += sz;
    inputSize -= sz;

    /* if there is still more than or equal to 64 bytes, move each 64 bytes through LTC */
    sz = LTC_DS_DS_MASK + 1u - LTC_HASH_BLOCK_SIZE;
    while (inputSize)
    {
        if (inputSize < sz)
        {
            uint32_t lastSize;

            lastSize = inputSize % blockSize;
            if (lastSize == 0)
            {
                lastSize = blockSize;
            }
            inputSize -= lastSize;
            if (inputSize)
            {
                /* move all complete blocks to ififo. */
                base->DS = inputSize;
                ltc_hash_move_to_ififo(ctx, input, inputSize, blockSize);

                status = ltc_wait(base);
                if (kStatus_Success != status)
                {
                    return status;
                }

                input += inputSize;
            }
            /* keep last (in)complete 16-bytes block in context struct. */
            /* when 3rd argument of cmac_move_to_ififo() is <= 16 bytes, it only stores the data to context struct */
            status = ltc_hash_move_rest_to_context(ctx, input, lastSize, modeReg, blockSize);
            if (kStatus_Success != status)
            {
                return status;
            }
            inputSize = 0;
        }
        else
        {
            base->DS = sz;
            ltc_hash_move_to_ififo(ctx, input, sz, blockSize);
            inputSize -= sz;
            input += sz;

            status = ltc_wait(base);
            if (kStatus_Success != status)
            {
                return status;
            }

            /* set algorithm state to UPDATE */
            modeReg &= ~LTC_MD_AS_MASK;
            modeReg |= kLTC_ModeUpdate;
            base->MD = modeReg;
        }
    } /* end while */

    return status;
}

/*******************************************************************************
 * HASH Code public
 ******************************************************************************/
status_t LTC_HASH_Init(LTC_Type *base, ltc_hash_ctx_t *ctx, ltc_hash_algo_t algo, const uint8_t *key, uint32_t keySize)
{
    status_t ret;
    ltc_hash_ctx_internal_t *ctxInternal;
    uint32_t i;

    ret = ltc_hash_check_input_args(base, ctx, algo, key, keySize);
    if (ret != kStatus_Success)
    {
        return ret;
    }

    /* set algorithm in context struct for later use */
    ctxInternal = (ltc_hash_ctx_internal_t *)ctx;
    ctxInternal->algo = algo;
    for (i = 0; i < kLTC_HashCtxNumWords; i++)
    {
        ctxInternal->word[i] = 0u;
    }

    /* Steps required only using AES engine */
    if (ltc_hash_alg_is_cmac(algo))
    {
        /* store input key and key length in context struct for later use */
        ctxInternal->word[kLTC_HashCtxKeySize] = keySize;
        ltc_memcpy(&ctxInternal->word[kLTC_HashCtxKeyStartIdx], key, keySize);
    }
    ctxInternal->blksz = 0u;
    for (i = 0; i < sizeof(ctxInternal->blk.w) / sizeof(ctxInternal->blk.w[0]); i++)
    {
        ctxInternal->blk.w[0] = 0u;
    }
    ctxInternal->state = kLTC_HashInit;
    ctxInternal->base = base;

    return kStatus_Success;
}

status_t LTC_HASH_Update(ltc_hash_ctx_t *ctx, const uint8_t *input, uint32_t inputSize)
{
    bool isUpdateState;
    ltc_mode_t modeReg = 0; /* read and write LTC mode register */
    LTC_Type *base;
    status_t status;
    ltc_hash_ctx_internal_t *ctxInternal;
    uint32_t blockSize;

    ctxInternal = (ltc_hash_ctx_internal_t *)ctx;
    status = ltc_hash_check_context(ctxInternal, input);
    if (kStatus_Success != status)
    {
        return status;
    }

    base = ctxInternal->base;
    blockSize = ltc_hash_get_block_size(ctxInternal->algo);
    /* if we are still less than 64 bytes, keep only in context */
    if ((ctxInternal->blksz + inputSize) <= blockSize)
    {
        ltc_memcpy((&ctxInternal->blk.b[0]) + ctxInternal->blksz, input, inputSize);
        ctxInternal->blksz += inputSize;
        return status;
    }
    else
    {
        isUpdateState = ctxInternal->state == kLTC_HashUpdate;
        if (ctxInternal->state == kLTC_HashInit)
        {
            /* set LTC mode register to INITIALIZE job */
            ltc_hash_engine_init(ctxInternal);

#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
            if (ltc_hash_alg_is_cmac(ctxInternal->algo))
            {
#endif /* FSL_FEATURE_LTC_HAS_SHA */
                ctxInternal->state = kLTC_HashUpdate;
                isUpdateState = true;
                base->DS = 0u;
                status = ltc_wait(base);
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
            }
            else
            {
                /* Set the proper block and algorithm mode. */
                modeReg = ltc_hash_algo2mode(ctxInternal->algo, kLTC_ModeInit, NULL);
                base->MD = modeReg;

                ctxInternal->state = kLTC_HashUpdate;
                status = ltc_hash_process_input_data(ctxInternal, input, inputSize, modeReg);
                ltc_hash_save_context(ctxInternal);
            }
#endif /* FSL_FEATURE_LTC_HAS_SHA */
        }
        else if (isUpdateState)
        {
            /* restore LTC context from context struct */
            ltc_hash_restore_context(ctxInternal);
        }
        else
        {
            /* nothing special at this place */
        }
    }

    if (kStatus_Success != status)
    {
        return status;
    }

    if (isUpdateState)
    {
        /* set LTC mode register to UPDATE job */
        ltc_hash_prepare_context_switch(base);
        base->CW = kLTC_ClearDataSize;
        modeReg = ltc_hash_algo2mode(ctxInternal->algo, kLTC_ModeUpdate, NULL);
        base->MD = modeReg;

        /* process input data and save LTC context to context structure */
        status = ltc_hash_process_input_data(ctxInternal, input, inputSize, modeReg);
        ltc_hash_save_context(ctxInternal);
    }
    ltc_clear_all(base, false);
    return status;
}

status_t LTC_HASH_Finish(ltc_hash_ctx_t *ctx, uint8_t *output, uint32_t *outputSize)
{
    ltc_mode_t modeReg; /* read and write LTC mode register */
    LTC_Type *base;
    uint32_t algOutSize = 0;
    status_t status;
    ltc_hash_ctx_internal_t *ctxInternal;
    uint32_t *ctxW;
    uint32_t i;

    ctxInternal = (ltc_hash_ctx_internal_t *)ctx;
    status = ltc_hash_check_context(ctxInternal, output);
    if (kStatus_Success != status)
    {
        return status;
    }

    base = ctxInternal->base;
    ltc_hash_prepare_context_switch(base);

    base->CW = kLTC_ClearDataSize;
    if (ctxInternal->state == kLTC_HashInit)
    {
        ltc_hash_engine_init(ctxInternal);
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
        if (ltc_hash_alg_is_cmac(ctxInternal->algo))
        {
#endif /* FSL_FEATURE_LTC_HAS_SHA */
            base->DS = 0u;
            status = ltc_wait(base);
            if (kStatus_Success != status)
            {
                return status;
            }
            modeReg = ltc_hash_algo2mode(ctxInternal->algo, kLTC_ModeFinalize, &algOutSize);
#if defined(FSL_FEATURE_LTC_HAS_SHA) && FSL_FEATURE_LTC_HAS_SHA
        }
        else
        {
            modeReg = ltc_hash_algo2mode(ctxInternal->algo, kLTC_ModeInitFinal, &algOutSize);
        }
#endif /* FSL_FEATURE_LTC_HAS_SHA */
        base->MD = modeReg;
    }
    else
    {
        modeReg = ltc_hash_algo2mode(ctxInternal->algo, kLTC_ModeFinalize, &algOutSize);
        base->MD = modeReg;

        /* restore LTC context from context struct */
        ltc_hash_restore_context(ctxInternal);
    }

    /* flush message last incomplete block, if there is any, or write zero to data size register. */
    base->DS = ctxInternal->blksz;
    ltc_hash_block_to_ififo(base, &ctxInternal->blk, ctxInternal->blksz, ltc_hash_get_block_size(ctxInternal->algo));
    /* Wait for finish of the encryption */
    status = ltc_wait(base);

    if (outputSize)
    {
        if (algOutSize < *outputSize)
        {
            *outputSize = algOutSize;
        }
        else
        {
            algOutSize = *outputSize;
        }
    }

    ltc_get_context(base, &output[0], algOutSize, 0u);

    ctxW = (uint32_t *)ctx;
    for (i = 0; i < LTC_HASH_CTX_SIZE; i++)
    {
        ctxW[i] = 0u;
    }

    ltc_clear_all(base, false);
    return status;
}

status_t LTC_HASH(LTC_Type *base,
                  ltc_hash_algo_t algo,
                  const uint8_t *input,
                  uint32_t inputSize,
                  const uint8_t *key,
                  uint32_t keySize,
                  uint8_t *output,
                  uint32_t *outputSize)
{
    status_t status;
    ltc_hash_ctx_t ctx;

    status = LTC_HASH_Init(base, &ctx, algo, key, keySize);
    if (status != kStatus_Success)
    {
        return status;
    }
    status = LTC_HASH_Update(&ctx, input, inputSize);
    if (status != kStatus_Success)
    {
        return status;
    }
    status = LTC_HASH_Finish(&ctx, output, outputSize);
    return status;
}

/*******************************************************************************
 * PKHA Code static
 ******************************************************************************/
#if defined(FSL_FEATURE_LTC_HAS_PKHA) && FSL_FEATURE_LTC_HAS_PKHA
static status_t ltc_pkha_clear_regabne(LTC_Type *base, bool A, bool B, bool N, bool E)
{
    ltc_mode_t mode;

    /* Set the PKHA algorithm and the appropriate function. */
    mode = (uint32_t)kLTC_AlgorithmPKHA | 1U;

    /* Set ram area to clear. Clear all. */
    if (A)
    {
        mode |= 1U << 19U;
    }
    if (B)
    {
        mode |= 1U << 18U;
    }
    if (N)
    {
        mode |= 1U << 16U;
    }
    if (E)
    {
        mode |= 1U << 17U;
    }

    /* Write the mode register to the hardware.
     * NOTE: This will begin the operation. */
    base->MDPK = mode;

    /* Wait for 'done' */
    return ltc_wait(base);
}

static void ltc_pkha_default_parms(ltc_pkha_mode_params_t *params)
{
    params->func = (ltc_pkha_func_t)0;
    params->arithType = kLTC_PKHA_IntegerArith;
    params->montFormIn = kLTC_PKHA_NormalValue;
    params->montFormOut = kLTC_PKHA_NormalValue;
    params->srcReg = kLTC_PKHA_RegAll;
    params->srcQuad = kLTC_PKHA_Quad0;
    params->dstReg = kLTC_PKHA_RegAll;
    params->dstQuad = kLTC_PKHA_Quad0;
    params->equalTime = kLTC_PKHA_NoTimingEqualized;
    params->r2modn = kLTC_PKHA_CalcR2;
}

static void ltc_pkha_write_word(LTC_Type *base, ltc_pkha_reg_area_t reg, uint8_t index, uint32_t data)
{
    switch (reg)
    {
        case kLTC_PKHA_RegA:
            base->PKA[index] = data;
            break;

        case kLTC_PKHA_RegB:
            base->PKB[index] = data;
            break;

        case kLTC_PKHA_RegN:
            base->PKN[index] = data;
            break;

        case kLTC_PKHA_RegE:
            base->PKE[index] = data;
            break;

        default:
            break;
    }
}

static uint32_t ltc_pkha_read_word(LTC_Type *base, ltc_pkha_reg_area_t reg, uint8_t index)
{
    uint32_t retval;

    switch (reg)
    {
        case kLTC_PKHA_RegA:
            retval = base->PKA[index];
            break;

        case kLTC_PKHA_RegB:
            retval = base->PKB[index];
            break;

        case kLTC_PKHA_RegN:
            retval = base->PKN[index];
            break;

        case kLTC_PKHA_RegE:
            retval = base->PKE[index];
            break;

        default:
            retval = 0;
            break;
    }
    return retval;
}

static status_t ltc_pkha_write_reg(
    LTC_Type *base, ltc_pkha_reg_area_t reg, uint8_t quad, const uint8_t *data, uint16_t dataSize)
{
    /* Select the word-based start index for each quadrant of 64 bytes. */
    uint8_t startIndex = (quad * 16u);
    uint32_t outWord;

    while (dataSize > 0)
    {
        if (dataSize >= sizeof(uint32_t))
        {
            ltc_pkha_write_word(base, reg, startIndex++, ltc_get_word_from_unaligned(data));
            dataSize -= sizeof(uint32_t);
            data += sizeof(uint32_t);
        }
        else /* (dataSize > 0) && (dataSize < 4) */
        {
            outWord = 0;
            ltc_memcpy(&outWord, data, dataSize);
            ltc_pkha_write_word(base, reg, startIndex, outWord);
            dataSize = 0;
        }
    }

    return kStatus_Success;
}

static void ltc_pkha_read_reg(LTC_Type *base, ltc_pkha_reg_area_t reg, uint8_t quad, uint8_t *data, uint16_t dataSize)
{
    /* Select the word-based start index for each quadrant of 64 bytes. */
    uint8_t startIndex = (quad * 16u);
    uint16_t calcSize;
    uint32_t word;

    while (dataSize > 0)
    {
        word = ltc_pkha_read_word(base, reg, startIndex++);

        calcSize = (dataSize >= sizeof(uint32_t)) ? sizeof(uint32_t) : dataSize;
        ltc_memcpy(data, &word, calcSize);

        data += calcSize;
        dataSize -= calcSize;
    }
}

static void ltc_pkha_init_data(LTC_Type *base,
                               const uint8_t *A,
                               uint16_t sizeA,
                               const uint8_t *B,
                               uint16_t sizeB,
                               const uint8_t *N,
                               uint16_t sizeN,
                               const uint8_t *E,
                               uint16_t sizeE)
{
    uint32_t clearMask = kLTC_ClearMode; /* clear Mode Register */

    /* Clear internal register states. */
    if (sizeA)
    {
        clearMask |= kLTC_ClearPkhaSizeA;
    }
    if (sizeB)
    {
        clearMask |= kLTC_ClearPkhaSizeB;
    }
    if (sizeN)
    {
        clearMask |= kLTC_ClearPkhaSizeN;
    }
    if (sizeE)
    {
        clearMask |= kLTC_ClearPkhaSizeE;
    }

    base->CW = clearMask;
    base->STA = kLTC_StatusDoneIsr;
    ltc_pkha_clear_regabne(base, A, B, N, E);

    /* Write register sizes. */
    /* Write modulus (N) and A and B register arguments. */
    if (sizeN)
    {
        base->PKNSZ = sizeN;
        if (N)
        {
            ltc_pkha_write_reg(base, kLTC_PKHA_RegN, 0, N, sizeN);
        }
    }

    if (sizeA)
    {
        base->PKASZ = sizeA;
        if (A)
        {
            ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 0, A, sizeA);
        }
    }

    if (sizeB)
    {
        base->PKBSZ = sizeB;
        if (B)
        {
            ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 0, B, sizeB);
        }
    }

    if (sizeE)
    {
        base->PKESZ = sizeE;
        if (E)
        {
            ltc_pkha_write_reg(base, kLTC_PKHA_RegE, 0, E, sizeE);
        }
    }
}

static void ltc_pkha_mode_set_src_reg_copy(ltc_mode_t *outMode, ltc_pkha_reg_area_t reg)
{
    int i = 0;

    do
    {
        reg = (ltc_pkha_reg_area_t)(((uint32_t)reg) >> 1u);
        i++;
    } while (reg);

    i = 4 - i;
    /* Source register must not be E. */
    if (i != 2)
    {
        *outMode |= ((uint32_t)i << 17u);
    }
}

static void ltc_pkha_mode_set_dst_reg_copy(ltc_mode_t *outMode, ltc_pkha_reg_area_t reg)
{
    int i = 0;

    do
    {
        reg = (ltc_pkha_reg_area_t)(((uint32_t)reg) >> 1u);
        i++;
    } while (reg);

    i = 4 - i;
    *outMode |= ((uint32_t)i << 10u);
}

static void ltc_pkha_mode_set_src_seg_copy(ltc_mode_t *outMode, const ltc_pkha_quad_area_t quad)
{
    *outMode |= ((uint32_t)quad << 8u);
}

static void ltc_pkha_mode_set_dst_seg_copy(ltc_mode_t *outMode, const ltc_pkha_quad_area_t quad)
{
    *outMode |= ((uint32_t)quad << 6u);
}

/*!
 * @brief Starts the PKHA operation.
 *
 * This function starts an operation configured by the params parameter.
 *
 * @param base LTC peripheral base address
 * @param params Configuration structure containing all settings required for PKHA operation.
 */
static status_t ltc_pkha_init_mode(LTC_Type *base, const ltc_pkha_mode_params_t *params)
{
    ltc_mode_t modeReg;
    status_t retval;

    /* Set the PKHA algorithm and the appropriate function. */
    modeReg = kLTC_AlgorithmPKHA;
    modeReg |= (uint32_t)params->func;

    if ((params->func == kLTC_PKHA_CopyMemSizeN) || (params->func == kLTC_PKHA_CopyMemSizeSrc))
    {
        /* Set source and destination registers and quads. */
        ltc_pkha_mode_set_src_reg_copy(&modeReg, params->srcReg);
        ltc_pkha_mode_set_dst_reg_copy(&modeReg, params->dstReg);
        ltc_pkha_mode_set_src_seg_copy(&modeReg, params->srcQuad);
        ltc_pkha_mode_set_dst_seg_copy(&modeReg, params->dstQuad);
    }
    else
    {
        /* Set the arithmetic type - integer or binary polynomial (F2m). */
        modeReg |= ((uint32_t)params->arithType << 17u);

        /* Set to use Montgomery form of inputs and/or outputs. */
        modeReg |= ((uint32_t)params->montFormIn << 19u);
        modeReg |= ((uint32_t)params->montFormOut << 18u);

        /* Set to use pre-computed R2modN */
        modeReg |= ((uint32_t)params->r2modn << 16u);
    }

    modeReg |= ((uint32_t)params->equalTime << 10u);

    /* Write the mode register to the hardware.
     * NOTE: This will begin the operation. */
    base->MDPK = modeReg;

    retval = ltc_wait(base);
    return (retval);
}

static status_t ltc_pkha_modR2(
    LTC_Type *base, const uint8_t *N, uint16_t sizeN, uint8_t *result, uint16_t *resultSize, ltc_pkha_f2m_t arithType)
{
    status_t status;
    ltc_pkha_mode_params_t params;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModR2;
    params.arithType = arithType;

    ltc_pkha_init_data(base, NULL, 0, NULL, 0, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    return status;
}

static status_t ltc_pkha_modmul(LTC_Type *base,
                                const uint8_t *A,
                                uint16_t sizeA,
                                const uint8_t *B,
                                uint16_t sizeB,
                                const uint8_t *N,
                                uint16_t sizeN,
                                uint8_t *result,
                                uint16_t *resultSize,
                                ltc_pkha_f2m_t arithType,
                                ltc_pkha_montgomery_form_t montIn,
                                ltc_pkha_montgomery_form_t montOut,
                                ltc_pkha_timing_t equalTime)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    if (arithType == kLTC_PKHA_IntegerArith)
    {
        if (LTC_PKHA_CompareBigNum(A, sizeA, N, sizeN) >= 0)
        {
            return (kStatus_InvalidArgument);
        }

        if (LTC_PKHA_CompareBigNum(B, sizeB, N, sizeN) >= 0)
        {
            return (kStatus_InvalidArgument);
        }
    }

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModMul;
    params.arithType = arithType;
    params.montFormIn = montIn;
    params.montFormOut = montOut;
    params.equalTime = equalTime;

    ltc_pkha_init_data(base, A, sizeA, B, sizeB, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    return status;
}

/*******************************************************************************
 * PKHA Code public
 ******************************************************************************/
int LTC_PKHA_CompareBigNum(const uint8_t *a, size_t sizeA, const uint8_t *b, size_t sizeB)
{
    int retval = 0;

    /* skip zero msbytes - integer a */
    while ((sizeA) && (0u == a[sizeA - 1]))
    {
        sizeA--;
    }

    /* skip zero msbytes - integer b */
    while ((sizeB) && (0u == b[sizeB - 1]))
    {
        sizeB--;
    }

    if (sizeA > sizeB)
    {
        retval = 1;
    } /* int a has more non-zero bytes, thus it is bigger than b */
    else if (sizeA < sizeB)
    {
        retval = -1;
    } /* int b has more non-zero bytes, thus it is bigger than a */
    else if (sizeA == 0)
    {
        retval = 0;
    } /* sizeA = sizeB = 0 */
    else
    {
        int n;
        int i;
        int val;
        uint32_t equal;

        n = sizeA - 1;
        i = 0;
        equal = 0;

        while (n >= 0)
        {
            uint32_t chXor = a[i] ^ b[i];

            equal |= chXor;
            val = (int)chXor * (a[i] - b[i]);

            if (val < 0)
            {
                retval = -1;
            }

            if (val > 0)
            {
                retval = 1;
            }

            if (val == 0)
            {
                val = 1;
            }

            if (val)
            {
                i++;
                n--;
            }
        }

        if (0 == equal)
        {
            retval = 0;
        }
    }
    return (retval);
}

status_t LTC_PKHA_NormalToMontgomery(LTC_Type *base,
                                     const uint8_t *N,
                                     uint16_t sizeN,
                                     uint8_t *A,
                                     uint16_t *sizeA,
                                     uint8_t *B,
                                     uint16_t *sizeB,
                                     uint8_t *R2,
                                     uint16_t *sizeR2,
                                     ltc_pkha_timing_t equalTime,
                                     ltc_pkha_f2m_t arithType)
{
    status_t status;

    /* need to convert our Integer inputs into Montgomery format */
    if (N && sizeN && R2 && sizeR2)
    {
        /* 1. R2 = MOD_R2(N) */
        status = ltc_pkha_modR2(base, N, sizeN, R2, sizeR2, arithType);
        if (status != kStatus_Success)
        {
            return status;
        }

        /* 2. A(Montgomery) = MOD_MUL_IM_OM(A, R2, N) */
        if (A && sizeA)
        {
            status = ltc_pkha_modmul(base, A, *sizeA, R2, *sizeR2, N, sizeN, A, sizeA, arithType,
                                     kLTC_PKHA_MontgomeryFormat, kLTC_PKHA_MontgomeryFormat, equalTime);
            if (status != kStatus_Success)
            {
                return status;
            }
        }

        /* 2. B(Montgomery) = MOD_MUL_IM_OM(B, R2, N) */
        if (B && sizeB)
        {
            status = ltc_pkha_modmul(base, B, *sizeB, R2, *sizeR2, N, sizeN, B, sizeB, arithType,
                                     kLTC_PKHA_MontgomeryFormat, kLTC_PKHA_MontgomeryFormat, equalTime);
            if (status != kStatus_Success)
            {
                return status;
            }
        }

        ltc_clear_all(base, true);
    }
    else
    {
        status = kStatus_InvalidArgument;
    }

    return status;
}

status_t LTC_PKHA_MontgomeryToNormal(LTC_Type *base,
                                     const uint8_t *N,
                                     uint16_t sizeN,
                                     uint8_t *A,
                                     uint16_t *sizeA,
                                     uint8_t *B,
                                     uint16_t *sizeB,
                                     ltc_pkha_timing_t equalTime,
                                     ltc_pkha_f2m_t arithType)
{
    uint8_t one = 1;
    status_t status = kStatus_InvalidArgument;

    /* A = MOD_MUL_IM_OM(A(Montgomery), 1, N) */
    if (A && sizeA)
    {
        status = ltc_pkha_modmul(base, A, *sizeA, &one, sizeof(one), N, sizeN, A, sizeA, arithType,
                                 kLTC_PKHA_MontgomeryFormat, kLTC_PKHA_MontgomeryFormat, equalTime);
        if (kStatus_Success != status)
        {
            return status;
        }
    }

    /* B = MOD_MUL_IM_OM(B(Montgomery), 1, N) */
    if (B && sizeB)
    {
        status = ltc_pkha_modmul(base, B, *sizeB, &one, sizeof(one), N, sizeN, B, sizeB, arithType,
                                 kLTC_PKHA_MontgomeryFormat, kLTC_PKHA_MontgomeryFormat, equalTime);
        if (kStatus_Success != status)
        {
            return status;
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModAdd(LTC_Type *base,
                         const uint8_t *A,
                         uint16_t sizeA,
                         const uint8_t *B,
                         uint16_t sizeB,
                         const uint8_t *N,
                         uint16_t sizeN,
                         uint8_t *result,
                         uint16_t *resultSize,
                         ltc_pkha_f2m_t arithType)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    if (arithType == kLTC_PKHA_IntegerArith)
    {
        if (LTC_PKHA_CompareBigNum(A, sizeA, N, sizeN) >= 0)
        {
            return (kStatus_InvalidArgument);
        }

        if (LTC_PKHA_CompareBigNum(B, sizeB, N, sizeN) >= 0)
        {
            return (kStatus_InvalidArgument);
        }
    }

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModAdd;
    params.arithType = arithType;

    ltc_pkha_init_data(base, A, sizeA, B, sizeB, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModSub1(LTC_Type *base,
                          const uint8_t *A,
                          uint16_t sizeA,
                          const uint8_t *B,
                          uint16_t sizeB,
                          const uint8_t *N,
                          uint16_t sizeN,
                          uint8_t *result,
                          uint16_t *resultSize)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    if (LTC_PKHA_CompareBigNum(A, sizeA, N, sizeN) >= 0)
    {
        return (kStatus_InvalidArgument);
    }

    if (LTC_PKHA_CompareBigNum(B, sizeB, N, sizeN) >= 0)
    {
        return (kStatus_InvalidArgument);
    }

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModSub1;
    ltc_pkha_init_data(base, A, sizeA, B, sizeB, N, sizeN, NULL, 0);

    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModSub2(LTC_Type *base,
                          const uint8_t *A,
                          uint16_t sizeA,
                          const uint8_t *B,
                          uint16_t sizeB,
                          const uint8_t *N,
                          uint16_t sizeN,
                          uint8_t *result,
                          uint16_t *resultSize)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModSub2;

    ltc_pkha_init_data(base, A, sizeA, B, sizeB, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModMul(LTC_Type *base,
                         const uint8_t *A,
                         uint16_t sizeA,
                         const uint8_t *B,
                         uint16_t sizeB,
                         const uint8_t *N,
                         uint16_t sizeN,
                         uint8_t *result,
                         uint16_t *resultSize,
                         ltc_pkha_f2m_t arithType,
                         ltc_pkha_montgomery_form_t montIn,
                         ltc_pkha_montgomery_form_t montOut,
                         ltc_pkha_timing_t equalTime)
{
    status_t status;

    status =
        ltc_pkha_modmul(base, A, sizeA, B, sizeB, N, sizeN, result, resultSize, arithType, montIn, montOut, equalTime);

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModExp(LTC_Type *base,
                         const uint8_t *A,
                         uint16_t sizeA,
                         const uint8_t *N,
                         uint16_t sizeN,
                         const uint8_t *E,
                         uint16_t sizeE,
                         uint8_t *result,
                         uint16_t *resultSize,
                         ltc_pkha_f2m_t arithType,
                         ltc_pkha_montgomery_form_t montIn,
                         ltc_pkha_timing_t equalTime)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    if (arithType == kLTC_PKHA_IntegerArith)
    {
        if (LTC_PKHA_CompareBigNum(A, sizeA, N, sizeN) >= 0)
        {
            return (kStatus_InvalidArgument);
        }
    }

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModExp;
    params.arithType = arithType;
    params.montFormIn = montIn;
    params.equalTime = equalTime;

    ltc_pkha_init_data(base, A, sizeA, NULL, 0, N, sizeN, E, sizeE);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModRed(LTC_Type *base,
                         const uint8_t *A,
                         uint16_t sizeA,
                         const uint8_t *N,
                         uint16_t sizeN,
                         uint8_t *result,
                         uint16_t *resultSize,
                         ltc_pkha_f2m_t arithType)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModRed;
    params.arithType = arithType;

    ltc_pkha_init_data(base, A, sizeA, NULL, 0, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModInv(LTC_Type *base,
                         const uint8_t *A,
                         uint16_t sizeA,
                         const uint8_t *N,
                         uint16_t sizeN,
                         uint8_t *result,
                         uint16_t *resultSize,
                         ltc_pkha_f2m_t arithType)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    /* A must be less than N -> LTC_PKHA_CompareBigNum() must return -1 */
    if (arithType == kLTC_PKHA_IntegerArith)
    {
        if (LTC_PKHA_CompareBigNum(A, sizeA, N, sizeN) >= 0)
        {
            return (kStatus_InvalidArgument);
        }
    }

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithModInv;
    params.arithType = arithType;

    ltc_pkha_init_data(base, A, sizeA, NULL, 0, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ModR2(
    LTC_Type *base, const uint8_t *N, uint16_t sizeN, uint8_t *result, uint16_t *resultSize, ltc_pkha_f2m_t arithType)
{
    status_t status;
    status = ltc_pkha_modR2(base, N, sizeN, result, resultSize, arithType);
    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_GCD(LTC_Type *base,
                      const uint8_t *A,
                      uint16_t sizeA,
                      const uint8_t *N,
                      uint16_t sizeN,
                      uint8_t *result,
                      uint16_t *resultSize,
                      ltc_pkha_f2m_t arithType)
{
    ltc_pkha_mode_params_t params;
    status_t status;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithGcd;
    params.arithType = arithType;

    ltc_pkha_init_data(base, A, sizeA, NULL, 0, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the result and size from register B0. */
        if (resultSize && result)
        {
            *resultSize = base->PKBSZ;
            /* Read the data from the result register into place. */
            ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, result, *resultSize);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_PrimalityTest(LTC_Type *base,
                                const uint8_t *A,
                                uint16_t sizeA,
                                const uint8_t *B,
                                uint16_t sizeB,
                                const uint8_t *N,
                                uint16_t sizeN,
                                bool *res)
{
    uint8_t result;
    ltc_pkha_mode_params_t params;
    status_t status;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithPrimalityTest;
    ltc_pkha_init_data(base, A, sizeA, B, sizeB, N, sizeN, NULL, 0);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the data from the result register into place. */
        ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 0, &result, 1);

        *res = (bool)result;
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ECC_PointAdd(LTC_Type *base,
                               const ltc_pkha_ecc_point_t *A,
                               const ltc_pkha_ecc_point_t *B,
                               const uint8_t *N,
                               const uint8_t *R2modN,
                               const uint8_t *aCurveParam,
                               const uint8_t *bCurveParam,
                               uint8_t size,
                               ltc_pkha_f2m_t arithType,
                               ltc_pkha_ecc_point_t *result)
{
    ltc_pkha_mode_params_t params;
    uint32_t clearMask;
    status_t status;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithEccAdd;
    params.arithType = arithType;
    params.r2modn = R2modN ? kLTC_PKHA_InputR2 : kLTC_PKHA_CalcR2;

    clearMask = kLTC_ClearMode;

    /* Clear internal register states. */
    clearMask |= kLTC_ClearPkhaSizeA;
    clearMask |= kLTC_ClearPkhaSizeB;
    clearMask |= kLTC_ClearPkhaSizeN;
    clearMask |= kLTC_ClearPkhaSizeE;

    base->CW = clearMask;
    base->STA = kLTC_StatusDoneIsr;
    ltc_pkha_clear_regabne(base, true, true, true, false);

    /* sizeN should be less than 64 bytes. */
    base->PKNSZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegN, 0, N, size);

    base->PKASZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 0, A->X, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 1, A->Y, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 3, aCurveParam, size);

    base->PKBSZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 0, bCurveParam, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 1, B->X, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 2, B->Y, size);
    if (R2modN)
    {
        ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 3, R2modN, size);
    }

    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the data from the result register into place. */
        ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 1, result->X, size);
        ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 2, result->Y, size);
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ECC_PointDouble(LTC_Type *base,
                                  const ltc_pkha_ecc_point_t *B,
                                  const uint8_t *N,
                                  const uint8_t *aCurveParam,
                                  const uint8_t *bCurveParam,
                                  uint8_t size,
                                  ltc_pkha_f2m_t arithType,
                                  ltc_pkha_ecc_point_t *result)
{
    ltc_pkha_mode_params_t params;
    uint32_t clearMask;
    status_t status;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithEccDouble;
    params.arithType = arithType;

    clearMask = kLTC_ClearMode;

    /* Clear internal register states. */
    clearMask |= kLTC_ClearPkhaSizeA;
    clearMask |= kLTC_ClearPkhaSizeB;
    clearMask |= kLTC_ClearPkhaSizeN;
    clearMask |= kLTC_ClearPkhaSizeE;

    base->CW = clearMask;
    base->STA = kLTC_StatusDoneIsr;
    ltc_pkha_clear_regabne(base, true, true, true, false);

    /* sizeN should be less than 64 bytes. */
    base->PKNSZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegN, 0, N, size);

    base->PKASZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 3, aCurveParam, size);

    base->PKBSZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 0, bCurveParam, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 1, B->X, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 2, B->Y, size);
    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the data from the result register into place. */
        ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 1, result->X, size);
        ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 2, result->Y, size);
    }

    ltc_clear_all(base, true);
    return status;
}

status_t LTC_PKHA_ECC_PointMul(LTC_Type *base,
                               const ltc_pkha_ecc_point_t *A,
                               const uint8_t *E,
                               uint8_t sizeE,
                               const uint8_t *N,
                               const uint8_t *R2modN,
                               const uint8_t *aCurveParam,
                               const uint8_t *bCurveParam,
                               uint8_t size,
                               ltc_pkha_timing_t equalTime,
                               ltc_pkha_f2m_t arithType,
                               ltc_pkha_ecc_point_t *result,
                               bool *infinity)
{
    ltc_pkha_mode_params_t params;
    uint32_t clearMask;
    status_t status;

    ltc_pkha_default_parms(&params);
    params.func = kLTC_PKHA_ArithEccMul;
    params.equalTime = equalTime;
    params.arithType = arithType;
    params.r2modn = R2modN ? kLTC_PKHA_InputR2 : kLTC_PKHA_CalcR2;

    clearMask = kLTC_ClearMode;

    /* Clear internal register states. */
    clearMask |= kLTC_ClearPkhaSizeA;
    clearMask |= kLTC_ClearPkhaSizeB;
    clearMask |= kLTC_ClearPkhaSizeN;
    clearMask |= kLTC_ClearPkhaSizeE;

    base->CW = clearMask;
    base->STA = kLTC_StatusDoneIsr;
    ltc_pkha_clear_regabne(base, true, true, true, true);

    /* sizeN should be less than 64 bytes. */
    base->PKNSZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegN, 0, N, size);

    base->PKESZ = sizeE;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegE, 0, E, sizeE);

    base->PKASZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 0, A->X, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 1, A->Y, size);
    ltc_pkha_write_reg(base, kLTC_PKHA_RegA, 3, aCurveParam, size);

    base->PKBSZ = size;
    ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 0, bCurveParam, size);
    if (R2modN)
    {
        ltc_pkha_write_reg(base, kLTC_PKHA_RegB, 1, R2modN, size);
    }

    status = ltc_pkha_init_mode(base, &params);

    if (status == kStatus_Success)
    {
        /* Read the data from the result register into place. */
        ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 1, result->X, size);
        ltc_pkha_read_reg(base, kLTC_PKHA_RegB, 2, result->Y, size);

        if (infinity)
        {
            *infinity = (bool)(base->STA & kLTC_StatusPublicKeyOpZero);
        }
    }

    ltc_clear_all(base, true);
    return status;
}

#endif /* FSL_FEATURE_LTC_HAS_PKHA */
