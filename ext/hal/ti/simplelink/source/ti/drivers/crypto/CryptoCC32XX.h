/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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
/** ============================================================================
 *  @file       CryptoCC32XX.h
 *
 *  @brief      Crypto driver implementation for a CC32XX Crypto controller.
 *
 *  The Crypto header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/crypto/CryptoCC32XX.h>
 *  @endcode
 *
 *  # Operation #
 *
 *  The CryptoCC32XX driver is used several security methods (AES, DES and HMAC Hash functions).
 *  This driver provides API for encrypt/decrypt (AES and DES)
 *  and sign/verify (HMAC hash)
 *
 *  The application initializes the CryptoCC32XX driver by calling CryptoCC32XX_init()
 *  and is then ready to open a Crypto by calling CryptoCC32XX_open().
 *
 *  The APIs in this driver serve as an interface to a typical TI-RTOS
 *  application. The specific peripheral implementations are responsible to
 *  create all the OSAL specific primitives to allow for thread-safe
 *  operation.
 *
 *  ## Opening the driver #
 *
 *  @code
 *  CryptoCC32XX_Handle      handle;
 *
 *  handle = CryptoCC32XX_open(CryptoCC32XX_configIndexValue,   CryptoCC32XX_AES |
                                                                CryptoCC32XX_DES |
                                                                CryptoCC32XX_HMAC);
 *  if (!handle) {
 *      System_printf("CryptoCC32XX did not open");
 *  }
 *  @endcode
 *
 *
 *  ## AES data encryption #
 *
 *  @code
 *  CryptoCC32XX_EncryptMethod  method = desiredMethod;
 *  CryptoCC32XX_Params         params;
 *  unsigned char               plainData[16] = "whatsoever123456";
 *  unsigned int                plainDataLen = sizeof(plainData);
 *  unsigned char               cipherData[16];
 *  unsigned int                cipherDataLen;
 *
 *  params.aes.keySize = desiredKeySize;
 *  params.aes.pKey = (CryptoCC32XX_KeyPtr)desiredKey; // desiredKey length should be as the desiredKeySize
 *  params.aes.pIV = (void *)pointerToInitVector;
 *  ret = CryptoCC32XX_encrypt(handle, method , plainData, plainDataLen, cipherData , &cipherDataLen , &params);
 *
 *  @endcode
 *
 *  ## Generate HMAC Hash signature #
 *
 *  @code
 *  CryptoCC32XX_HmacMethod hmacMethod = desiredHmacMethod;
 *  CryptoCC32XX_Params     params;
 *  unsigned char           dataBuff[] = "whatsoever";
 *  unsigned int            dataLength = sizeof(dataBuff);
 *  unsigned char           signatureBuff[32];
 *
 *  params.pKey = pointerToHMACkey;
 *  params.moreData = 0;
 *  ret = CryptoCC32XX_sign(handle, hmacMethod , &dataBuff, dataLength, &signatureBuff, &params);
 *
 *  @endcode
 *
 *  # Implementation #
 *
 *  The CryptoCC32XX driver interface module is joined (at link time) to a
 *  NULL-terminated array of CryptoCC32XX_Config data structures named *CryptoCC32XX_config*.
 *  *CryptoCC32XX_config* is implemented in the application with each entry being an
 *  instance of a CryptoCC32XX peripheral. Each entry in *CryptoCC32XX_config* contains a:
 *  - (void *) data object that is pointed to CryptoCC32XX_Object
 *
 *
 *  ============================================================================
 */

#ifndef ti_drivers_crypto_CryptoCC32XX__include
#define ti_drivers_crypto_CryptoCC32XX__include

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>


#define CryptoCC32XX_CMD_RESERVED             32

#define CryptoCC32XX_STATUS_RESERVED          -32

/*!
 * @brief   Successful status code returned by Crypto Common functions.
 *
 */
#define CryptoCC32XX_STATUS_SUCCESS           0

/*!
 * @brief   Generic error status code returned by Crypto Common functions.
 *
 */
#define CryptoCC32XX_STATUS_ERROR             -1

/*!
 * @brief   An error status code returned by Crypto Common functions for undefined
 * command codes.
 *
 */
#define CryptoCC32XX_STATUS_UNDEFINEDCMD      -2

/*!
 * @brief   An error status code returned by CryptoCC32XX_verify for define error in
 * verifying a given Hash value.
 *
 */
#define CryptoCC32XX_STATUS_ERROR_VERIFY      -3

/*!
 * @brief   An error status code returned by Crypto Common functions for define
 * cryptographic type not supported.
 *
 */
#define CryptoCC32XX_STATUS_ERROR_NOT_SUPPORTED -4


#define CryptoCC32XX_MAX_TYPES  3

#define CryptoCC32XX_MD5_BLOCK_SIZE     64
#define CryptoCC32XX_SHA1_BLOCK_SIZE    64
#define CryptoCC32XX_SHA256_BLOCK_SIZE  64

#define CryptoCC32XX_MD5_DIGEST_SIZE    16
#define CryptoCC32XX_SHA1_DIGEST_SIZE   20
#define CryptoCC32XX_SHA256_DIGEST_SIZE 32

#define CryptoCC32XX_MAX_DIGEST_SIZE CryptoCC32XX_SHA256_DIGEST_SIZE
#define CryptoCC32XX_MAX_BLOCK_SIZE  CryptoCC32XX_SHA256_BLOCK_SIZE


/*!
 *  @brief  Cryptography types configuration
 *
 *  This enum defines bitwise Cryptography types.
 */
typedef enum
{
    CryptoCC32XX_AES  = 0x01,    /*!< Advanced Encryption Standard */
    CryptoCC32XX_DES  = 0x02,    /*!< Data Encryption Standard */
    CryptoCC32XX_HMAC = 0x04,    /*!< Cryptographic hash function */
}CryptoCC32XX_Type;

/*!
 *  @brief  AES and DES Cryptography methods configuration
 *  Keep the Crypto method in the lower 8 bit and
 *  Crypto type in the upper 8 bits
 *
 *  This enum defines the AES and DES Cryptography modes.
 */
typedef enum
{
    CryptoCC32XX_AES_ECB = (CryptoCC32XX_AES << 8) | 1, /*!< AES Electronic CodeBook */
    CryptoCC32XX_AES_CBC,                         /*!< AES Cipher Block Chaining */
    CryptoCC32XX_AES_CTR,                         /*!< AES Counter */
    CryptoCC32XX_AES_ICM,                         /*!< AES Integer Counter Mode */
    CryptoCC32XX_AES_CFB,                         /*!< AES Cipher FeedBack */
    CryptoCC32XX_AES_GCM,                         /*!< AES Galois/Counter Mode */
    CryptoCC32XX_AES_CCM,                         /*!< AES Counter with CBC-MAC Mode */

    CryptoCC32XX_DES_ECB = (CryptoCC32XX_DES << 8) | 1, /*!< DES Electronic CodeBook */
    CryptoCC32XX_DES_CBC,                         /*!< DES Cipher Block Chaining */
    CryptoCC32XX_DES_CFB,                         /*!< DES Cipher FeedBack */

}CryptoCC32XX_EncryptMethod;

/*!
 *  @brief  HMAC Cryptography methods configuration
 *  Keep the Crypto method in the lower 8 bit and
 *  Crypto type in the upper 8 bits
 *
 *  This enum defines the HMAC HASH algorithms modes.
 */
typedef enum
{
    CryptoCC32XX_HMAC_MD5 = (CryptoCC32XX_HMAC << 8) | 1,   /*!< MD5 used keyed-hash message authentication code */
    CryptoCC32XX_HMAC_SHA1,                           /*!< SHA1 used keyed-hash message authentication code */
    CryptoCC32XX_HMAC_SHA224,                         /*!< SHA224 used keyed-hash message authentication code */
    CryptoCC32XX_HMAC_SHA256                          /*!< SHA256 used keyed-hash message authentication code */

}CryptoCC32XX_HmacMethod;

/*!
 *  @brief  AES Cryptography key size type configuration
 *
 *  This enum defines the AES key size types
 */
typedef enum
{
    CryptoCC32XX_AES_KEY_SIZE_128BIT,
    CryptoCC32XX_AES_KEY_SIZE_192BIT,
    CryptoCC32XX_AES_KEY_SIZE_256BIT

}CryptoCC32XX_AesKeySize;

/*!
 *  @brief  DES Cryptography key size type configuration
 *
 *  This enum defines the DES key size types
 */
typedef enum
{
    CryptoCC32XX_DES_KEY_SIZE_SINGLE,
    CryptoCC32XX_DES_KEY_SIZE_TRIPLE

}CryptoCC32XX_DesKeySize;


/*!
 *  @brief  AES Additional Authentication Data input parameters
 *
 *  This structure defines the AES Additional Authentication Data input parameters used for
 *  CryptoCC32XX_AES_GCM and CryptoCC32XX_AES_CCM
 */
typedef struct
{
    uint8_t                 *pKey2;     /*!< pointer to AES second key (CryptoCC32XX_AES_CCM) */
    CryptoCC32XX_AesKeySize key2Size;   /*!< AES second Key size type (CryptoCC32XX_AES_CCM) */
    size_t                  len;        /*!< length of the additional authentication data in bytes */
}CryptoCC32XX_AesAadInputParams;

/*!
 *  @brief  AES Additional Authentication Data Parameters
 *
 *  This union defines the AES additional authentication parameters used for
 *  CryptoCC32XX_AES_GCM and CryptoCC32XX_AES_CCM
 */
typedef union
{
    CryptoCC32XX_AesAadInputParams input;   /*!<an input - additional authentication data */
    uint8_t                        tag[16]; /*!<an output - pointer to a 4-word array where the hash tag is written */
}CryptoCC32XX_AesAadParams;

/*!
 *  @brief  AES Parameters
 *
 *  This structure defines the AES parameters used in CryptoCC32XX_encrypt and CryptoCC32XX_decrypt functions.
 */
typedef struct
{
    const uint8_t             *pKey;      /*!< pointer to AES key */
    CryptoCC32XX_AesKeySize   keySize;    /*!< AES Key size type */
    void                      *pIV;       /*!< Pointer to AES Initialization Vector */
    CryptoCC32XX_AesAadParams aadParams;
}CryptoCC32XX_AesParams;

/*!
 *  @brief  DES Parameters
 *
 *  This structure defines the DES parameters used in CryptoCC32XX_encrypt and CryptoCC32XX_decrypt functions.
 */
typedef struct
{
    const uint8_t           *pKey;      /*!< pointer to DES key */
    CryptoCC32XX_DesKeySize keySize;    /*!< DES Key size type */
    void                    *pIV;       /*!< Pointer to DES Initialization Vector */
}CryptoCC32XX_DesParams;

/*!
 *  @brief  Cryptography Parameters
 *
 *  This union defines the AES and DES Cryptographic types
 */
typedef union
{
    CryptoCC32XX_AesParams        aes;
    CryptoCC32XX_DesParams        des;
}CryptoCC32XX_EncryptParams;

/*!
 *  @brief  HMAC Parameters
 *
 *  This structure defines the Hmac parameters used in CryptoCC32XX_sign and CryptoCC32XX_verify functions.
 */
typedef struct
{
    /*! pointer to hash key */
    uint8_t  *pKey;
    /*! True value will NOT close the HMAC HW machine */
    uint8_t  moreData;
    /*! Reserved for future use */
    void     *pContext;
    /*! True if no data was written to the HMAC HW machine */
    uint8_t  first;
    /*! Number of bytes that was written to the HMAC HW machine */
    uint32_t digestCount;
    /*! Intermediate digest */
    uint8_t  innerDigest[CryptoCC32XX_MAX_DIGEST_SIZE];
    /*! Internal buffer - used when moreData sets to true and the data length is not an integer multiple of blockSize */
    uint8_t  buff[CryptoCC32XX_MAX_BLOCK_SIZE];
    /*! Number of bytes in buff */
    uint32_t buffLen;
    /*! Block size of the hashing algorithm in use */
    uint32_t blockSize;
}CryptoCC32XX_HmacParams;

/*!
 *  @brief      A handle that is returned from a CryptoCC32XX_open() call.
 */
typedef struct CryptoCC32XX_Config    *CryptoCC32XX_Handle;


/*!
 *  @brief  CryptoCC32XX Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct CryptoCC32XX_Object {
    /*! Interrupt handles */
    HwiP_Handle     hwiHandle[CryptoCC32XX_MAX_TYPES];
    /*! flag to indicate module is open */
    bool            isOpen;
    /*! Semaphore handles */
    SemaphoreP_Handle   sem[CryptoCC32XX_MAX_TYPES];
} CryptoCC32XX_Object;


/*!
 *  @brief  Crypto Global configuration
 *
 *  The CryptoCC32XX_Config structure contains a set of pointers used to characterize
 *  the Crypto driver implementation.
 *
 *  This structure needs to be defined before calling CryptoCC32XX_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     CryptoCC32XX_init()
 */
typedef struct CryptoCC32XX_Config {

    /*! Pointer to a driver specific data object */
    void               *object;

} CryptoCC32XX_Config;


/*!
 *  @brief  Function to close a given Crypto peripheral specified by the Crypto
 *  handle.
 *
 *  @pre    CryptoCC32XX_open() had to be called first.
 *
 *  @param  handle  A CryptoCC32XX_Handle returned from CryptoCC32XX_open
 *
 *  @sa     CryptoCC32XX_open()
 */
void CryptoCC32XX_close(CryptoCC32XX_Handle handle);

/*!
 *  @brief  Function to initializes the Crypto module
 *
 *  @pre    The CryptoCC32XX_Config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other Crypto driver APIs. This function call does not modify any
 *          peripheral registers.
 */
void CryptoCC32XX_init(void);

/*!
 *  @brief  Opens a Crypto object with a given index and returns a CryptoCC32XX_Handle.
 *
 *  @pre    Crypto module has been initialized
 *
 *  @param  index         Logical peripheral number for the Crypto indexed into
 *                        the CryptoCC32XX_config table
 *
 *  @param  types         Define bitwise Crypto Types to support
 *
 *  @return A CryptoCC32XX_Handle on success or a NULL on an error or if it has been
 *          opened already.
 *
 *  @sa     CryptoCC32XX_init()
 *  @sa     CryptoCC32XX_HmacParams_init()
 *  @sa     CryptoCC32XX_close()
 */
CryptoCC32XX_Handle CryptoCC32XX_open(uint32_t index, uint32_t types);

/*!
 *  @brief  Initialize params structure to default values.
 *
 *  The default parameters are:
 *   - pKey: 0
 *   - moreData: 0
 *   - *pContext: 0 
 *   - first: 1
 *   - digestCount: 0
 *   - innerDigest: 0
 *   - buff: 0
 *   - buffLen: 0
 *   - blockSize: CryptoCC32XX_SHA256_BLOCK_SIZE
 *
 *  @param params  Pointer to the instance configuration parameters.
 */
void CryptoCC32XX_HmacParams_init(CryptoCC32XX_HmacParams *params);


/*!
 *  @brief Function which encrypt given data by a given AES or DES method.
 *  relevant to CryptoCC32XX_AES and CryptoCC32XX_DES
 *
 *  @param  handle      A CryptoCC32XX_Handle
 *
 *  @param  method      An AES or DES encryption method to use on a given plain data.
 *
 *  @param  pInBuff     Pointer to plain data to encrypt.
 *
 *  @param  inLen       Size of plain data to encrypt.
 *
 *  @param  pOutBuff    Pointer to encrypted data (cipher text).
 *
 *  @param  outLen      Size of encrypted data.
 *
 *  @param  pParams     Specific parameters according to Crypto Type (AES or DES).
 *
 *  @return             Returns CryptoCC32XX_STATUS_SUCCESS if successful else would return
 *                      CryptoCC32XX_STATUS_ERROR on an error.
 *
 *  @sa                 CryptoCC32XX_open()
 */
int32_t CryptoCC32XX_encrypt(  CryptoCC32XX_Handle handle, CryptoCC32XX_EncryptMethod method ,
                            void *pInBuff, size_t inLen,
                            void *pOutBuff , size_t *outLen , CryptoCC32XX_EncryptParams *pParams);

/*!
 *  @brief Function which decrypt given cipher data by a given AES or DES method.
 *  relevant to CryptoCC32XX_AES and CryptoCC32XX_DES
 *
 *  @param  handle      A CryptoCC32XX_Handle
 *
 *  @param  method      An AES or DES decryption method to use on a given cipher data.
 *
 *  @param  pInBuff     Pointer to cipher data to decrypt.
 *
 *  @param  inLen       Size of cipher data to decrypt.
 *
 *  @param  pOutBuff    Pointer to decrypted data (plain text).
 *
 *  @param  outLen      Size of decrypted data.
 *
 *  @param  pParams     Specific parameters according to Crypto Type (AES or DES).
 *
 *  @return             Returns CryptoCC32XX_STATUS_SUCCESS if successful else would return
 *                      CryptoCC32XX_STATUS_ERROR on an error.
 *
 *  @sa                 CryptoCC32XX_open()
 */
int32_t CryptoCC32XX_decrypt(  CryptoCC32XX_Handle handle, CryptoCC32XX_EncryptMethod method ,
                            void *pInBuff, size_t inLen,
                            void *pOutBuff , size_t *outLen , CryptoCC32XX_EncryptParams *pParams);

/*!
 *  @brief Function which generates the HMAC Hash value of given plain Text.
 *  relevant to CryptoCC32XX_HMAC
 *
 *  @param  handle      A CryptoCC32XX_Handle
 *
 *  @param  method      HMAC Hash algorithm to use in order to generates the hash value
 *
 *  @param  pBuff       Pointer to plain data.
 *
 *  @param  len         Size of plain data.
 *
 *  @param  pSignature  As input pointer to the given HMAC Hash value in case the HMAC flag was set
 *                      and as output pointer for the generated Hash value.
 *
 *  @param  pParams     Specific parameters according to HMAC algorithm
 *
 *  @return             Returns CryptoCC32XX_STATUS_SUCCESS if successful else would return
 *                      CryptoCC32XX_STATUS_ERROR on an error.
 *
 *  @sa                 CryptoCC32XX_open()
 */
int32_t CryptoCC32XX_sign( CryptoCC32XX_Handle handle, CryptoCC32XX_HmacMethod method ,
                        void *pBuff, size_t len,
                        uint8_t *pSignature, CryptoCC32XX_HmacParams *pParams);

/*!
 *  @brief Function which verify a given Hash value on given plain Text.
 *  relevant to CryptoCC32XX_HMAC
 *
 *  @param  handle      A CryptoCC32XX_Handle
 *
 *  @param  method      HMAC Hash algorithm to use in order to verify the hash value
 *
 *  @param  pBuff       Pointer to plain data.
 *
 *  @param  len         Size of plain data.
 *
 *  @param  pSignature  As input pointer to the given HMAC Hash value in case the HMAC flag was set
 *                      and as output pointer for the generated Hash value.
 *
 *  @param  pParams     Specific parameters according to HMAC algorithm.
 *
 *  @return             Returns CryptoCC32XX_STATUS_SUCCESS if value was successfully verified
 *                      else would return CryptoCC32XX_STATUS_ERROR.
 *
 *  @sa                 CryptoCC32XX_open()
 */
int32_t CryptoCC32XX_verify(   CryptoCC32XX_Handle handle, CryptoCC32XX_HmacMethod method ,
                            void *pBuff, size_t len,
                            uint8_t *pSignature, CryptoCC32XX_HmacParams *pParams);


#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_CryptoCC32XX__include */
