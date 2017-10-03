/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*
 * By default disable both asserts and log for this module.
 * This must be done before DebugP.h is included.
 */
#ifndef DebugP_ASSERT_ENABLED
#define DebugP_ASSERT_ENABLED 0
#endif
#ifndef DebugP_LOG_ENABLED
#define DebugP_LOG_ENABLED 0
#endif

#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include <ti/drivers/crypto/CryptoCC32XX.h>

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_shamd5.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/aes.h>
#include <ti/devices/cc32xx/driverlib/des.h>
#include <ti/devices/cc32xx/driverlib/shamd5.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>


#define CryptoCC32XX_SHAMD5_SIGNATURE_LEN_MD5       16
#define CryptoCC32XX_SHAMD5_SIGNATURE_LEN_SHA1      20
#define CryptoCC32XX_SHAMD5_SIGNATURE_LEN_SHA224    28
#define CryptoCC32XX_SHAMD5_SIGNATURE_LEN_SHA256    32

#define CryptoCC32XX_CONTEXT_READY_MAX_COUNTER      1000

#define CryptoCC32XX_SHAMD5GetSignatureSize(_mode) ((_mode == SHAMD5_ALGO_MD5)     ? CryptoCC32XX_SHAMD5_SIGNATURE_LEN_MD5:      \
                                                    (_mode == SHAMD5_ALGO_SHA1)    ?  CryptoCC32XX_SHAMD5_SIGNATURE_LEN_SHA1:    \
                                                    (_mode == SHAMD5_ALGO_SHA224)  ?  CryptoCC32XX_SHAMD5_SIGNATURE_LEN_SHA224:  \
                                                    (_mode == SHAMD5_ALGO_SHA256)  ?  CryptoCC32XX_SHAMD5_SIGNATURE_LEN_SHA256:0)

#define SHAMD5_SIGNATURE_MAX_LEN    CryptoCC32XX_SHAMD5_SIGNATURE_LEN_SHA256
typedef int8_t CryptoCC32XX_SHAMD5Signature[SHAMD5_SIGNATURE_MAX_LEN];

typedef struct CryptoCC32XX_HwiP {
    /*! Crypto Peripheral's interrupt handler */
    HwiP_Fxn    hwiIntFxn;
    /*! Crypto Peripheral's interrupt vector */
    uint32_t    intNum;
    /*! Crypto Peripheral's interrupt priority */
    uint32_t    intPriority;
} CryptoCC32XX_HwiP;

/* Prototypes */
int32_t         CryptoCC32XX_aesProcess(uint32_t cryptoMode , uint32_t cryptoDirection, uint8_t* pInBuff, uint16_t inLen, uint8_t* pOutBuff , CryptoCC32XX_EncryptParams* pParams);
int32_t         CryptoCC32XX_desProcess(uint32_t cryptoMode , uint32_t cryptoDirection, uint8_t* pInBuff, uint16_t inLen, uint8_t* pOutBuff , CryptoCC32XX_EncryptParams* pParams);
int32_t         CryptoCC32XX_shamd5Process(uint32_t cryptoMode , uint8_t* pBuff, uint32_t len, uint8_t *pSignature, CryptoCC32XX_HmacParams* pParams);
void            CryptoCC32XX_aesIntHandler(void);
void            CryptoCC32XX_desIntHandler(void);
void            CryptoCC32XX_shamd5IntHandler(void);
HwiP_Handle     CryptoCC32XX_register(CryptoCC32XX_Handle handle, CryptoCC32XX_HwiP *hwiP);
void            CryptoCC32XX_unregister(HwiP_Handle handle);


/* Crypto CC32XX interrupts implementation */
CryptoCC32XX_HwiP CryptoCC32XX_HwiPTable[] = {
    { (HwiP_Fxn)CryptoCC32XX_aesIntHandler,     INT_AES, (~0) },    /* AES */
    { (HwiP_Fxn)CryptoCC32XX_desIntHandler,     INT_DES, (~0) },    /* DES */
    { (HwiP_Fxn)CryptoCC32XX_shamd5IntHandler,  INT_SHA, (~0) }     /* SHAMD5 */
};

/* Crypto AES key size to Crypto CC32XX AES key size */
#define CryptoCC32XX_getAesKeySize(_keySize) (                                  \
    (_keySize == CryptoCC32XX_AES_KEY_SIZE_128BIT)? AES_CFG_KEY_SIZE_128BIT:    \
    (_keySize == CryptoCC32XX_AES_KEY_SIZE_192BIT)? AES_CFG_KEY_SIZE_192BIT:    \
    (_keySize == CryptoCC32XX_AES_KEY_SIZE_256BIT)? AES_CFG_KEY_SIZE_256BIT:    \
    CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED)

/* Crypto DES key size to Crypto CC32XX DES key size */
#define CryptoCC32XX_getDesKeySize(_keySize) (                           \
    (_keySize == CryptoCC32XX_DES_KEY_SIZE_SINGLE)? DES_CFG_SINGLE:      \
    (_keySize == CryptoCC32XX_DES_KEY_SIZE_TRIPLE)? DES_CFG_TRIPLE:      \
    CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED)

/* Crypto method to Crypto CC32XX mode */
#define CryptoCC32XX_getMode(_method) (                                  \
    (_method == CryptoCC32XX_AES_ECB)?        AES_CFG_MODE_ECB:          \
    (_method == CryptoCC32XX_AES_CBC)?        AES_CFG_MODE_CBC:          \
    (_method == CryptoCC32XX_AES_CTR)?        AES_CFG_MODE_CTR:          \
    (_method == CryptoCC32XX_AES_ICM)?        AES_CFG_MODE_ICM:          \
    (_method == CryptoCC32XX_AES_CFB)?        AES_CFG_MODE_CFB:          \
    (_method == CryptoCC32XX_AES_GCM)?        AES_CFG_MODE_GCM_HY0CALC:  \
    (_method == CryptoCC32XX_AES_CCM)?        AES_CFG_MODE_CCM:          \
    (_method == CryptoCC32XX_DES_ECB)?        DES_CFG_MODE_ECB:          \
    (_method == CryptoCC32XX_DES_CBC)?        DES_CFG_MODE_CBC:          \
    (_method == CryptoCC32XX_DES_CFB)?        DES_CFG_MODE_CFB:          \
    (_method == CryptoCC32XX_HMAC_MD5)?       SHAMD5_ALGO_MD5:           \
    (_method == CryptoCC32XX_HMAC_SHA1)?      SHAMD5_ALGO_SHA1:          \
    (_method == CryptoCC32XX_HMAC_SHA224)?    SHAMD5_ALGO_SHA224:        \
    (_method == CryptoCC32XX_HMAC_SHA256)?    SHAMD5_ALGO_SHA256:        \
    CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED)


/* global variables for the interrupt handles */
static volatile bool g_bAESReadyFlag;
static volatile bool g_bDESReadyFlag;
static volatile bool g_bSHAMD5ReadyFlag;

/* Externs */
extern const CryptoCC32XX_Config CryptoCC32XX_config[];
extern const uint8_t CryptoCC32XX_count;


/*
 *  ======== CryptoCC32XX_close ========
 */
void CryptoCC32XX_close(CryptoCC32XX_Handle handle)
{
    uintptr_t                   key;
    CryptoCC32XX_Object         *object = handle->object;
    int32_t type;

    /* Mask Crypto interrupts */


    /* Power off the Crypto module */
    Power_releaseDependency(PowerCC32XX_PERIPH_DTHE);

    for (type = 0; type  < CryptoCC32XX_MAX_TYPES;type++)
    {
        if (object->sem[type] != NULL)
        {
            SemaphoreP_delete(object->sem[type]);
        }
        if (object->hwiHandle[type] != NULL)
        {
            CryptoCC32XX_unregister(object->hwiHandle[type]);
        }
   }

    /* Mark the module as available */
    key = HwiP_disable();

    object->isOpen = false;

    HwiP_restore(key);

    return;
}


/*
 *  ======== CryptoCC32XX_init ========
 */
void CryptoCC32XX_init(void)
{
}

/*
 *  ======== CryptoCC32XX_open ========
 */
CryptoCC32XX_Handle CryptoCC32XX_open(uint32_t index, uint32_t types)
{
    CryptoCC32XX_Handle       handle;
    uintptr_t                 key;
    CryptoCC32XX_Object       *object;
    SemaphoreP_Params         semParams;
    int32_t type;


    if (index >= CryptoCC32XX_count)
    {
        return (NULL);
    }

    /* Get handle for this driver instance */
    handle = (CryptoCC32XX_Handle)&(CryptoCC32XX_config[index]);
    object = handle->object;

    /* Determine if the device index was already opened */
    key = HwiP_disable();
    if(object->isOpen == true)
    {
        HwiP_restore(key);
        return (NULL);
    }

    /* Mark the handle as being used */
    object->isOpen = true;
    HwiP_restore(key);

    /* Power on the Crypto module */
    Power_setDependency(PowerCC32XX_PERIPH_DTHE);
    MAP_PRCMPeripheralReset(PRCM_DTHE);

    /* create the semaphore for each crypto type*/
    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;
    for (type=0 ;type < CryptoCC32XX_MAX_TYPES; type++)
    {
        if ((types & (1<<type)) != 0)
        {
            object->sem[type] = SemaphoreP_create(1, &semParams);
            if (object->sem[type] == NULL)
            {
                CryptoCC32XX_close(handle);
                return (NULL);
            }
            /* interrupt handler */
            object->hwiHandle[type] = CryptoCC32XX_register(handle,&CryptoCC32XX_HwiPTable[type]);
            if (object->hwiHandle[type] == NULL)
            {
                CryptoCC32XX_close(handle);
                return (NULL);
            }
        }
        else
        {
            object->sem[type] = NULL;
        }
    }
    /* Return the address of the handle */
    return (handle);
}

/*
 *  ======== CryptoCC32XX_HmacParams_init ========
 */
void CryptoCC32XX_HmacParams_init(CryptoCC32XX_HmacParams *params)
{
    memset(params, 0, sizeof(CryptoCC32XX_HmacParams));
    params->first = 1;
    /* All supported hashing algorithms block size are equal, set one as default */
    params->blockSize = CryptoCC32XX_SHA256_BLOCK_SIZE;
}

/*
 *  ======== CryptoCC32XX_encrypt ========
 */
int32_t CryptoCC32XX_encrypt(CryptoCC32XX_Handle handle, CryptoCC32XX_EncryptMethod method , void *pInBuff, size_t inLen, void *pOutBuff , size_t *outLen , CryptoCC32XX_EncryptParams *pParams)
{
    CryptoCC32XX_Object     *object = handle->object;
    uint8_t cryptoType = method >> 8;
    uint32_t cryptoMode = CryptoCC32XX_getMode((uint32_t)method);
    int32_t status = CryptoCC32XX_STATUS_ERROR;
    /* Convert crypto type bitwise to index */
    uint8_t cryptoIndex = cryptoType >> 1;

    /* check semaphore created  */
    if (object->sem[cryptoIndex] == NULL)
    {
        return CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED;
    }
    /* get the semaphore */
    if (SemaphoreP_OK == SemaphoreP_pend(object->sem[cryptoIndex],
                SemaphoreP_WAIT_FOREVER))
    {
        switch (cryptoType)
        {
            case CryptoCC32XX_AES:
                status = CryptoCC32XX_aesProcess(cryptoMode , AES_CFG_DIR_ENCRYPT, pInBuff, inLen, pOutBuff , pParams);
            break;
            case CryptoCC32XX_DES:
                status = CryptoCC32XX_desProcess(cryptoMode , DES_CFG_DIR_ENCRYPT, pInBuff, inLen, pOutBuff , pParams);
            break;
            default:
            break;
        }

        /* release the semaphore */
        SemaphoreP_post(object->sem[cryptoIndex]);
    }
    return status;
}

/*
 *  ======== CryptoCC32XX_decrypt ========
 */
int32_t CryptoCC32XX_decrypt(CryptoCC32XX_Handle handle, CryptoCC32XX_EncryptMethod method , void *pInBuff, size_t inLen, void *pOutBuff , size_t *outLen , CryptoCC32XX_EncryptParams *pParams)
{
    CryptoCC32XX_Object     *object = handle->object;
    uint8_t cryptoType =  method >> 8;
    uint32_t cryptoMode = CryptoCC32XX_getMode((uint32_t)method);
    int32_t status = CryptoCC32XX_STATUS_ERROR;
    /* Convert crypto type bitwise to index */
    uint8_t cryptoIndex = cryptoType >> 1;

    /* check semaphore created  */
    if (object->sem[cryptoIndex] == NULL)
    {
        return CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED;
    }
    /* get the semaphore */
    if (SemaphoreP_OK == SemaphoreP_pend(object->sem[cryptoIndex],
                SemaphoreP_WAIT_FOREVER))
    {
        switch (cryptoType)
        {
            case CryptoCC32XX_AES:
                status = CryptoCC32XX_aesProcess(cryptoMode , AES_CFG_DIR_DECRYPT, pInBuff, inLen, pOutBuff , pParams);
            break;
            case CryptoCC32XX_DES:
                status = CryptoCC32XX_desProcess(cryptoMode , DES_CFG_DIR_DECRYPT, pInBuff, inLen, pOutBuff , pParams);
            break;
            default:
            break;
        }
        /* release the semaphore */
        SemaphoreP_post(object->sem[cryptoIndex]);
    }
    return status;
}


/*
 *  ======== CryptoCC32XX_sign ========
 */
int32_t CryptoCC32XX_sign(CryptoCC32XX_Handle handle, CryptoCC32XX_HmacMethod method , void *pBuff, size_t len, uint8_t *pSignature, CryptoCC32XX_HmacParams *pParams)
{
    CryptoCC32XX_Object     *object = handle->object;
    uint8_t cryptoType = CryptoCC32XX_HMAC;
    uint32_t cryptoMode = CryptoCC32XX_getMode((uint32_t)method);
    int32_t status = CryptoCC32XX_STATUS_ERROR;
    /* Convert crypto type bitwise to index */
    uint8_t cryptoIndex = cryptoType >> 1;

    /* check semaphore created  */
    if (object->sem[cryptoIndex] == NULL)
    {
        return CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED;
    }
    /* get the semaphore */
    if (SemaphoreP_OK == SemaphoreP_pend(object->sem[cryptoIndex],
                SemaphoreP_WAIT_FOREVER))
    {
        switch (cryptoType)
        {
            case CryptoCC32XX_HMAC:
                status = CryptoCC32XX_shamd5Process(cryptoMode , pBuff, len, pSignature , pParams);
            break;
            default:
            break;
        }
        /* release the semaphore */
        SemaphoreP_post(object->sem[cryptoIndex]);
    }
    return status;
}

/*
 *  ======== CryptoCC32XX_verify ========
 */
int32_t CryptoCC32XX_verify(CryptoCC32XX_Handle handle, CryptoCC32XX_HmacMethod method , void *pBuff, size_t len, uint8_t *pSignature, CryptoCC32XX_HmacParams *pParams)
{
    CryptoCC32XX_Object     *object = handle->object;
    uint8_t cryptoType = CryptoCC32XX_HMAC;
    uint32_t cryptoMode = CryptoCC32XX_getMode((uint32_t)method);
    int32_t status = CryptoCC32XX_STATUS_ERROR;
    uint16_t shamd5SignatureSize;
    CryptoCC32XX_SHAMD5Signature shamd5Signature;
    /* Convert crypto type bitwise to index */
    uint8_t cryptoIndex = cryptoType >> 1;

    /* check semaphore created  */
    if (object->sem[cryptoIndex] == NULL)
    {
        return CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED;
    }
    /* get the semaphore */
    if (SemaphoreP_OK == SemaphoreP_pend(object->sem[cryptoIndex],
                SemaphoreP_WAIT_FOREVER))
    {
        switch (cryptoType)
        {
            case CryptoCC32XX_HMAC:
                shamd5SignatureSize = CryptoCC32XX_SHAMD5GetSignatureSize(cryptoMode);
                if (shamd5SignatureSize > 0)
                {
                    if (pParams->moreData == 0)
                    {
                        /* save the received signature */
                        memcpy(shamd5Signature,pSignature,shamd5SignatureSize);
                    }
                    /* calculate the signature */
                    status = CryptoCC32XX_shamd5Process(cryptoMode , pBuff, len, pSignature , pParams);
                    /* compare the calculated signature to the received signature */
                    if (status == CryptoCC32XX_STATUS_SUCCESS)
                    {
                        if (pParams->moreData == 0)
                        {
                            if (memcmp(shamd5Signature,pSignature,shamd5SignatureSize) != 0)
                            {
                                status = CryptoCC32XX_STATUS_ERROR_VERIFY;
                            }
                        }
                    }
                }
            break;
            default:
            break;
        }
        /* release the semaphore */
        SemaphoreP_post(object->sem[cryptoIndex]);
    }
    return status;
}

/*
 *  ======== CryptoCC32XX_shamd5Process ========
 */
int32_t CryptoCC32XX_shamd5Process(uint32_t cryptoMode , uint8_t *pBuff, uint32_t len, uint8_t *pSignature, CryptoCC32XX_HmacParams *pParams)
{
    int32_t count = CryptoCC32XX_CONTEXT_READY_MAX_COUNTER;
    uint32_t blockComplementLen = 0;
    uint32_t newLen = 0;
    uint32_t copyLen = 0;
    uint32_t totalLen = 0;
    uint32_t blockRemainder = 0;
    /*
    Step1: Enable Interrupts
    Step2: Wait for Context Ready Interrupt
    Step3: Set the Configuration Parameters (Hash Algorithm)
    Step4: Set Key
    Step5: Start Hash Generation
    */

    if((pBuff == NULL) || (0 == len))
    {
      return CryptoCC32XX_STATUS_ERROR;
    }

    /* Clear the flag */
    g_bSHAMD5ReadyFlag = false;

    /* Enable interrupts. */
    MAP_SHAMD5IntEnable(SHAMD5_BASE, SHAMD5_INT_CONTEXT_READY |
                    SHAMD5_INT_PARTHASH_READY |
                    SHAMD5_INT_INPUT_READY |
                    SHAMD5_INT_OUTPUT_READY);

    /* Wait for the context ready flag. */
    while ((!g_bSHAMD5ReadyFlag) && (count > 0))
    {
        count --;
    }
    if (count == 0)
    {
        return CryptoCC32XX_STATUS_ERROR;
    }
        
    if (pParams->moreData == 1)
    {
        /* Less than block size available. Copy the received data to the internal buffer and return */
        if((pParams->buffLen + len) < pParams->blockSize)
        {
            memcpy(&pParams->buff[pParams->buffLen], pBuff, len);
            pParams->buffLen += len;
            return CryptoCC32XX_STATUS_SUCCESS;
        }

        if(pParams->first)
        {
            /* If Keyed Hashing is used, set Key */
            if (pParams->pKey != NULL)
            {
                MAP_SHAMD5HMACKeySet(SHAMD5_BASE, pParams->pKey);
                MAP_SHAMD5ConfigSet(SHAMD5_BASE, cryptoMode, 0, 0, 1, 0);
            }
            else
            {
                MAP_SHAMD5ConfigSet(SHAMD5_BASE, cryptoMode, 1, 0, 0, 0);
            }

            /* Will write data in this iteration. Unset first */
            pParams->first = 0;
        }
        else
        {
            MAP_SHAMD5ConfigSet(SHAMD5_BASE, cryptoMode, 0, 0, 0, 0);
            /* Write the intermediate digest and count in case it isn't the first round */
            SHAMD5ResultWrite(SHAMD5_BASE, pParams->innerDigest);
            SHAMD5WriteDigestCount(SHAMD5_BASE, pParams->digestCount);
        }
    }
    else
    {
        if(pParams->first)
        {    
            /* If Keyed Hashing is used, set Key */
            if (pParams->pKey != NULL)
            {
                MAP_SHAMD5HMACKeySet(SHAMD5_BASE, pParams->pKey);
                MAP_SHAMD5ConfigSet(SHAMD5_BASE, cryptoMode, 0, 1, 1, 1);
            }
            else
            {
                MAP_SHAMD5ConfigSet(SHAMD5_BASE, cryptoMode, 1, 1, 0, 0);
            }
        }
        else
        {
            /* Write the intermediate digest and count in case it isn't the first round */
            SHAMD5ResultWrite(SHAMD5_BASE, pParams->innerDigest);
            SHAMD5WriteDigestCount(SHAMD5_BASE, pParams->digestCount);
            if (pParams->pKey != NULL)
            {
                MAP_SHAMD5ConfigSet(SHAMD5_BASE, cryptoMode, 0, 1, 0, 1);
            }
            else
            {
                MAP_SHAMD5ConfigSet(SHAMD5_BASE, cryptoMode, 0, 1, 0, 0);
            }
            
            /* This is the last iteration for the requested calculation */
            /* Set the first flag to 1 (initial value) for next calculations */
            pParams->first = 1;
        }
    }

    /* Set the length of the data that will be written to the SHA module in this iteration */
    /* In case it isn't the last iteration, it has to be an integer multiple of block size */
    if (pParams->moreData)
    {
        totalLen = ((pParams->buffLen + len) / pParams->blockSize) * pParams->blockSize;
    }
    else
    {
        totalLen = pParams->buffLen + len;
    }
    SHAMD5DataLengthSet(SHAMD5_BASE, totalLen);

    /* In case there is data in the internal buffer, complement to a block size (if the length is sufficient) and write */
    if (pParams->buffLen)
    {
        blockComplementLen = pParams->blockSize - pParams->buffLen;
        /* Copy to the internal buffer */
        /* The length to copy is the minimum between the block complement and the data length */
        copyLen = len < blockComplementLen ? len : blockComplementLen;
        memcpy(&pParams->buff[pParams->buffLen], pBuff, copyLen);
        pParams->buffLen += copyLen;

        /* If buffLen is smaller than block size this must be the last iteration */
        /* Write and set the buffer and buffer length to zero */
        SHAMD5DataWriteMultiple(SHAMD5_BASE, pParams->buff, pParams->buffLen);
        pParams->buffLen = 0;
        memset(pParams->buff, 0, sizeof(pParams->buff));
    }

    /* If data length is greater than block complement, write the rest of the data */
    if (len > blockComplementLen)
    {
        newLen= len - blockComplementLen;
        pBuff += blockComplementLen;
    }

    if (pParams->moreData)
    {
        /* Remaining length is smaller than block size - Save data to the internal buffer */
        if (newLen < pParams->blockSize)
        {
            memcpy(&pParams->buff[pParams->buffLen], pBuff, newLen);
            pParams->buffLen = newLen;
        }
        /* Remaining length is greater than block size - write blocks and save remainder to the internal buffer */
        else
        {
            blockRemainder = newLen % pParams->blockSize;
            newLen -=  blockRemainder;
            SHAMD5DataWriteMultiple(SHAMD5_BASE, pBuff, newLen);

            if (blockRemainder)
            {
                pBuff += newLen;
                memcpy(&pParams->buff[pParams->buffLen], pBuff, blockRemainder);
                pParams->buffLen += blockRemainder;
            }
        }
    }
    else
    {
        /* Last iteration, write all the data */
        if (newLen)
        {
            SHAMD5DataWriteMultiple(SHAMD5_BASE, pBuff, newLen);
        }
    }

    /* Wait for the output to be ready */
    while((HWREG(SHAMD5_BASE + SHAMD5_O_IRQSTATUS) & SHAMD5_INT_OUTPUT_READY) == 0)
    {
    }

    /* Read the result */
    if (pParams->moreData == 1)
    {
        /* Read the digest and count to an internal parameter (will be used in the next iteration) */
        MAP_SHAMD5ResultRead(SHAMD5_BASE, pParams->innerDigest);
        SHAMD5ReadDigestCount(SHAMD5_BASE, &(pParams->digestCount));
    }
    else
    {
        /* Read the final digest result to an outer pointer */
        MAP_SHAMD5ResultRead(SHAMD5_BASE, pSignature);
    }

    return CryptoCC32XX_STATUS_SUCCESS;
}

/*
 *  ======== CryptoCC32XX_aesProcess ========
 */
int32_t CryptoCC32XX_aesProcess(uint32_t cryptoMode , uint32_t cryptoDirection, uint8_t *pInBuff, uint16_t inLen, uint8_t *pOutBuff , CryptoCC32XX_EncryptParams *pParams)
{
    int32_t count = CryptoCC32XX_CONTEXT_READY_MAX_COUNTER;
    /*
    Step1:  Enable Interrupts
    Step2:  Wait for Context Ready Interrupt
    Step3:  Set the Configuration Parameters (Direction,AES Mode and Key Size)
    Step4:  Set the Initialization Vector
    Step5:  Write Key
    Step6:  Start the Crypt Process
    */

    /* Clear the flag. */
    g_bAESReadyFlag = false;

    /* Enable all interrupts. */
    MAP_AESIntEnable(AES_BASE, AES_INT_CONTEXT_IN | AES_INT_CONTEXT_OUT | AES_INT_DATA_IN | AES_INT_DATA_OUT);

    /* Wait for the context in flag, the flag will be set in the Interrupt handler. */
    while((!g_bAESReadyFlag) && (count > 0))
    {
        count --;
    }
    if (count == 0)
    {
        return CryptoCC32XX_STATUS_ERROR;
    }

    /* Configure the AES module with direction (encryption or decryption) and  */
    /* the key size. */
    MAP_AESConfigSet(AES_BASE, cryptoDirection | cryptoMode | CryptoCC32XX_getAesKeySize(pParams->aes.keySize));

    /* Write the initial value registers if needed, depends on the mode. */
    if ((cryptoMode == AES_CFG_MODE_CBC) || (cryptoMode == AES_CFG_MODE_CFB) ||
        (cryptoMode == AES_CFG_MODE_CTR) || (cryptoMode == AES_CFG_MODE_ICM) ||
        (cryptoMode == AES_CFG_MODE_CCM) || (cryptoMode == AES_CFG_MODE_GCM_HY0CALC))
    {
        MAP_AESIVSet(AES_BASE, pParams->aes.pIV);
    }


    /* Write key1. */
    MAP_AESKey1Set(AES_BASE, (uint8_t *)pParams->aes.pKey,CryptoCC32XX_getAesKeySize(pParams->aes.keySize));

    /* Write key2. */
    if (cryptoMode == AES_CFG_MODE_CCM)
    {
        MAP_AESKey2Set(AES_BASE, pParams->aes.aadParams.input.pKey2, CryptoCC32XX_getAesKeySize(pParams->aes.aadParams.input.key2Size));
    }

    /* Start Crypt Process */
    if ((cryptoMode == AES_CFG_MODE_CCM) || (cryptoMode == AES_CFG_MODE_GCM_HY0CALC))
    {
        MAP_AESDataProcessAE(AES_BASE, (uint8_t *)(pInBuff+(pParams->aes.aadParams.input.len)), pOutBuff ,inLen, pInBuff, pParams->aes.aadParams.input.len, pParams->aes.aadParams.tag);

    }
    else
    {
        MAP_AESDataProcess(AES_BASE, pInBuff, pOutBuff, inLen);
    }

    /* Read the initial value registers if needed, depends on the mode. */
    if ((cryptoMode == AES_CFG_MODE_CBC) || (cryptoMode == AES_CFG_MODE_CFB) ||
        (cryptoMode == AES_CFG_MODE_CTR) || (cryptoMode == AES_CFG_MODE_ICM) ||
        (cryptoMode == AES_CFG_MODE_CCM) || (cryptoMode == AES_CFG_MODE_GCM_HY0CALC))
    {
        MAP_AESIVGet(AES_BASE, pParams->aes.pIV);
    }

    return CryptoCC32XX_STATUS_SUCCESS;

}

/*
 *  ======== CryptoCC32XX_desProcess ========
 */
int32_t CryptoCC32XX_desProcess(uint32_t cryptoMode , uint32_t cryptoDirection, uint8_t *pInBuff, uint16_t inLen, uint8_t *pOutBuff , CryptoCC32XX_EncryptParams *pParams)
{
    int32_t count = CryptoCC32XX_CONTEXT_READY_MAX_COUNTER;

    /*
    Step1:  Enable Interrupts
    Step2:  Wait for Context Ready Interrupt
    Step3:  Set the Configuration Parameters (Direction,AES Mode)
    Step4:  Set the Initialization Vector
    Step5:  Write Key
    Step6:  Start the Crypt Process
    */

    /* Clear the flag. */
    g_bDESReadyFlag = false;

    /* Enable all interrupts. */
    MAP_DESIntEnable(DES_BASE, DES_INT_CONTEXT_IN | DES_INT_DATA_IN | DES_INT_DATA_OUT);

    /* Wait for the context in flag. */
    while((!g_bDESReadyFlag) && (count > 0))
    {
        count --;
    }
    if (count == 0)
    {
        return CryptoCC32XX_STATUS_ERROR;
    }

    /* Configure the DES module. */
    MAP_DESConfigSet(DES_BASE, cryptoDirection | cryptoMode | CryptoCC32XX_getDesKeySize(pParams->des.keySize));

    /* Set the key. */
    MAP_DESKeySet(DES_BASE, (uint8_t *)pParams->des.pKey);

    /* Write the initial value registers if needed. */
    if((cryptoMode & DES_CFG_MODE_CBC) || (cryptoMode & DES_CFG_MODE_CFB))
    {
        MAP_DESIVSet(DES_BASE, pParams->des.pIV);
    }

    MAP_DESDataProcess(DES_BASE, pInBuff, pOutBuff,inLen);
    return CryptoCC32XX_STATUS_SUCCESS;
}


/*
 *  ======== CryptoCC32XX_aesIntHandler ========
 */
void CryptoCC32XX_aesIntHandler(void)
{
    uint32_t uiIntStatus;

    /* Read the AES masked interrupt status. */
    uiIntStatus = MAP_AESIntStatus(AES_BASE, true);

    /* Set Different flags depending on the interrupt source. */
    if(uiIntStatus & AES_INT_CONTEXT_IN)
    {
        MAP_AESIntDisable(AES_BASE, AES_INT_CONTEXT_IN);
        g_bAESReadyFlag = true;
    }
    if(uiIntStatus & AES_INT_DATA_IN)
    {
        MAP_AESIntDisable(AES_BASE, AES_INT_DATA_IN);
    }
    if(uiIntStatus & AES_INT_CONTEXT_OUT)
    {
        MAP_AESIntDisable(AES_BASE, AES_INT_CONTEXT_OUT);
    }
    if(uiIntStatus & AES_INT_DATA_OUT)
    {
        MAP_AESIntDisable(AES_BASE, AES_INT_DATA_OUT);
    }
}

/*
 *  ======== CryptoCC32XX_desIntHandler ========
 */
void CryptoCC32XX_desIntHandler(void)
{
    uint32_t ui32IntStatus;

    /* Read the DES masked interrupt status. */
    ui32IntStatus = MAP_DESIntStatus(DES_BASE, true);

    /* set flags depending on the interrupt source. */
    if(ui32IntStatus & DES_INT_CONTEXT_IN)
    {
        MAP_DESIntDisable(DES_BASE, DES_INT_CONTEXT_IN);
        g_bDESReadyFlag = true;
    }
    if(ui32IntStatus & DES_INT_DATA_IN)
    {
        MAP_DESIntDisable(DES_BASE, DES_INT_DATA_IN);
    }
    if(ui32IntStatus & DES_INT_DATA_OUT)
    {
        MAP_DESIntDisable(DES_BASE, DES_INT_DATA_OUT);
    }
}

/*
 *  ======== CryptoCC32XX_shamd5IntHandler ========
 */
void CryptoCC32XX_shamd5IntHandler(void)
{
    uint32_t ui32IntStatus;

    /* Read the SHA/MD5 masked interrupt status. */
    ui32IntStatus = MAP_SHAMD5IntStatus(SHAMD5_BASE, true);

    if(ui32IntStatus & SHAMD5_INT_CONTEXT_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_CONTEXT_READY);
        g_bSHAMD5ReadyFlag = true;
    }
    if(ui32IntStatus & SHAMD5_INT_PARTHASH_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_PARTHASH_READY);
    }
    if(ui32IntStatus & SHAMD5_INT_INPUT_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_INPUT_READY);
    }
    if(ui32IntStatus & SHAMD5_INT_OUTPUT_READY)
    {
        MAP_SHAMD5IntDisable(SHAMD5_BASE, SHAMD5_INT_OUTPUT_READY);
    }
}

/*
 *  ======== CryptoCC32XX_register ========
 */
HwiP_Handle CryptoCC32XX_register(CryptoCC32XX_Handle handle, CryptoCC32XX_HwiP *hwiP)
{
    HwiP_Params             hwiParams;
    HwiP_Handle             hwiHandle = NULL;

    if (hwiP->hwiIntFxn != NULL)
    {
        /* Create Hwi object for this CryptoCC32XX peripheral. */
        HwiP_Params_init(&hwiParams);
        hwiParams.arg = (uintptr_t)handle;
        hwiParams.priority = hwiP->intPriority;
        hwiHandle = HwiP_create(hwiP->intNum, hwiP->hwiIntFxn,&hwiParams);
    }
    return hwiHandle;
}

/*
 *  ======== CryptoCC32XX_unregister ========
 */
void CryptoCC32XX_unregister(HwiP_Handle handle)
{
    if (handle != NULL)
    {
        /* Delete Hwi object */
        HwiP_delete(handle);
    }
}
