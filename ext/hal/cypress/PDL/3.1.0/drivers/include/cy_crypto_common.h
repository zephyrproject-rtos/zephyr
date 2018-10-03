/***************************************************************************//**
* \file cy_crypto_common.h
* \version 2.10
*
* \brief
*  This file provides common constants and parameters
*  for the Crypto driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/


#if !defined(CY_CRYPTO_COMMON_H)
#define CY_CRYPTO_COMMON_H

#include "cy_device.h"
#include "cy_device_headers.h"
#include <stddef.h>
#include <stdbool.h>
#include "cy_sysint.h"
#include "cy_syslib.h"

#if defined(CY_IP_MXCRYPTO) && (CY_IP_MXCRYPTO_VERSION == 1)

#if (CPUSS_CRYPTO_PRESENT == 1)

#define CY_CRYPTO_CORE_ENABLE               (1)

/** Driver major version */
#define CY_CRYPTO_DRV_VERSION_MAJOR         2

/** Driver minor version */
#define CY_CRYPTO_DRV_VERSION_MINOR         10

/** Defines the Crypto notify interrupt number */
#define CY_CRYPTO_IPC_INTR_NOTIFY_NUM       CY_IPC_INTR_CRYPTO_SRV

/** Defines the Crypto release interrupt number */
#define CY_CRYPTO_IPC_INTR_RELEASE_NUM      CY_IPC_INTR_CRYPTO_CLI

/**
* \addtogroup group_crypto_macros
* \{
*/

/** Defines Crypto_Sync blocking execution type parameter */
#define CY_CRYPTO_SYNC_BLOCKING           (true)

/** Defines Crypto_Sync non-blocking execution type parameter */
#define CY_CRYPTO_SYNC_NON_BLOCKING       (false)

/** Defines the Crypto DES key size (in bytes) */
#define CY_CRYPTO_DES_KEY_SIZE            (8u)

/** Defines the Crypto TDES key size (in bytes) */
#define CY_CRYPTO_TDES_KEY_SIZE           (24u)

/** Defines the Crypto AES block size (in bytes) */
#define CY_CRYPTO_AES_BLOCK_SIZE          (16u)

/** Defines the Crypto AES_128 key maximum size (in bytes) */
#define CY_CRYPTO_AES_128_KEY_SIZE        (16u)

/** Defines the Crypto AES_192 key maximum size (in bytes) */
#define CY_CRYPTO_AES_192_KEY_SIZE        (24u)

/** Defines the Crypto AES_256 key maximum size (in bytes) */
#define CY_CRYPTO_AES_256_KEY_SIZE        (32u)

/** Defines size of the AES block, in four-byte words */
#define CY_CRYPTO_AES_BLOCK_SIZE_U32      (uint32_t)(CY_CRYPTO_AES_BLOCK_SIZE / 4ul)

#if (CPUSS_CRYPTO_SHA == 1)

/** Hash size for the SHA1 mode (in bytes)   */
#define CY_CRYPTO_SHA1_DIGEST_SIZE          (20u)
/** Hash size for the SHA224 mode (in bytes) */
#define CY_CRYPTO_SHA224_DIGEST_SIZE        (28u)
/** Hash size for the SHA256 mode (in bytes) */
#define CY_CRYPTO_SHA256_DIGEST_SIZE        (32u)
/** Hash size for the SHA384 mode (in bytes) */
#define CY_CRYPTO_SHA384_DIGEST_SIZE        (48u)
/** Hash size for the SHA512 mode (in bytes) */
#define CY_CRYPTO_SHA512_DIGEST_SIZE        (64u)
/** Hash size for the SHA512_224 mode (in bytes) */
#define CY_CRYPTO_SHA512_224_DIGEST_SIZE    (28u)
/** Hash size for the SHA512_256 mode (in bytes) */
#define CY_CRYPTO_SHA512_256_DIGEST_SIZE    (32u)

#endif /* #if (CPUSS_CRYPTO_SHA == 1) */


#if (CPUSS_CRYPTO_VU == 1)

/** Processed message size for the RSA 1024Bit mode (in bytes) */
#define CY_CRYPTO_RSA1024_MESSAGE_SIZE      (128)
/** Processed message size for the RSA 1536Bit mode (in bytes) */
#define CY_CRYPTO_RSA1536_MESSAGE_SIZE      (192)
/** Processed message size for the RSA 2048Bit mode (in bytes) */
#define CY_CRYPTO_RSA2048_MESSAGE_SIZE      (256)

#endif /* #if (CPUSS_CRYPTO_VU == 1) */


/** Crypto Driver PDL ID */
#define CY_CRYPTO_ID                        CY_PDL_DRV_ID(0x0Cu)

/** \} group_crypto_macros */

/**
* \addtogroup group_crypto_config_structure
* \{
*/

/** The Crypto user callback function type.
    Callback is called at the end of Crypto calculation. */
typedef void (*cy_crypto_callback_ptr_t)(void);

/** The Crypto configuration structure. */
typedef struct
{
    /** Defines the IPC channel used for client-server data exchange */
    uint32_t ipcChannel;

    /** Specifies the IPC notifier channel (IPC interrupt structure number)
        to notify server that data for the operation is prepared */
    uint32_t acquireNotifierChannel;

    /** Specifies the IPC notifier channel (IPC interrupt structure number)
        to notify client that operation is complete and data is valid */
    uint32_t releaseNotifierChannel;

    /** Specifies the release notifier interrupt configuration. It used for
        internal purposes and user doesn't fill it. */
    cy_stc_sysint_t releaseNotifierConfig;

    /** User callback function.
        If this field is NOT NULL, it called when Crypto operation
        is complete. */
    cy_crypto_callback_ptr_t userCompleteCallback;

#if (CY_CRYPTO_CORE_ENABLE)
    /** Server-side user IRQ handler function, called when data for the
        operation is prepared to process.
        - If this field is NULL, server will use own interrupt handler
          to get data.
        - If this field is not NULL, server will call this interrupt handler.
          This interrupt handler must call the
          \ref Cy_Crypto_Server_GetDataHandler to get data to process.

          Note: In the second case user should process data separately and
          clear interrupt by calling \ref Cy_Crypto_Server_Process.
          This model is used in the
          multitasking environment. */
    cy_israddress userGetDataHandler;

    /** Server-side user IRQ handler function, called when a Crypto hardware
        error occurs (interrupt was raised).
        - If this field is NULL - server will use own interrupt handler
          for error processing.
        - If this field is not NULL - server will call this interrupt handler.
          This interrupt handler must call the
          \ref Cy_Crypto_Server_ErrorHandler to clear the interrupt. */
    cy_israddress userErrorHandler;

    /** Specifies the prepared data notifier interrupt configuration. It used
        for internal purposes and user doesn't fill it. */
    cy_stc_sysint_t acquireNotifierConfig;

    /** Specifies the hardware error processing interrupt configuration. It used
        for internal purposes and user doesn't fill it. */
    cy_stc_sysint_t cryptoErrorIntrConfig;
#endif /* (CY_CRYPTO_CORE_ENABLE) */

} cy_stc_crypto_config_t;

/** \} group_crypto_config_structure */

/**
* \addtogroup group_crypto_cli_data_structures
* \{
*/

#if (CPUSS_CRYPTO_VU == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the RSA public key and
 * additional coefficients to accelerate RSA calculation.
 *
 *  RSA key contained from two fields:
 *  - n - modulus part of the key
 *  - e - exponent part of the key.
 *
 * Other fields are accelerating coefficients and can be calculated by
 * \ref Cy_Crypto_Rsa_CalcCoefs.
 *
 * \note The <b>modulus</b> and <b>exponent</b> values in the
 * \ref cy_stc_crypto_rsa_pub_key_t must also be in little-endian order.<br>
 * Use \ref Cy_Crypto_Rsa_InvertEndianness function to convert to or from
 * little-endian order.
 *
*/
typedef struct
{
    /** The pointer to the modulus part of public key. */
    uint8_t *moduloPtr;
    /** The modulus length, in bits, maximum supported size is 2048Bit */
    uint32_t moduloLength;

    /** The pointer to the exponent part of public key */
    uint8_t *pubExpPtr;
    /** The exponent length, in bits, maximum supported size is 256Bit */
    uint32_t pubExpLength;

    /** The pointer to the Barrett coefficient. Memory for it should be
        allocated by user with size moduloLength + 1. */
    uint8_t *barretCoefPtr;

    /** The pointer to the binary inverse of the modulo. Memory for it
        should be allocated by user with size moduloLength. */
    uint8_t *inverseModuloPtr;

    /** The pointer to the (2^moduloLength mod modulo). Memory for it should
        be allocated by user with size moduloLength */
    uint8_t *rBarPtr;
} cy_stc_crypto_rsa_pub_key_t;

#endif /* #if (CPUSS_CRYPTO_VU == 1) */

/** Structure for storing a description of a Crypto hardware error  */
typedef struct
{
    /**
     Captures error description information:
      for INSTR_OPC_ERROR: - violating the instruction.
      for INSTR_CC_ERROR : - violating the instruction condition code.
      for BUS_ERROR      : - violating the transfer address. */
    uint32_t errorStatus0;

    /**
     [31]     - Specifies if ERROR_STATUS0 and ERROR_STATUS1 capture valid
                error-information.
     [26..24] - The error source:
                "0": INSTR_OPC_ERROR - an instruction decoder error.
                "1": INSTR_CC_ERROR - an instruction condition code-error.
                "2": BUS_ERROR - a bus master interface AHB-Lite bus-error.
                "3": TR_AP_DETECT_ERROR.
     [23..0]  - Captures error description information.
                For BUS_ERROR:
                  - violating the transfer, read the attribute (DATA23[0]).
                  - violating the transfer, the size attribute (DATA23[5:4]).
                     "0": an 8-bit transfer;
                     "1": 16 bits transfer;
                     "2": 32-bit transfer. */
    uint32_t errorStatus1;
} cy_stc_crypto_hw_error_t;

/** \} group_crypto_cli_data_structures */


/** The Crypto library functionality level. */
typedef enum
{
    CY_CRYPTO_NO_LIBRARY    = 0x00u,
    CY_CRYPTO_BASE_LIBRARY  = 0x01u,
    CY_CRYPTO_EXTRA_LIBRARY = 0x02u,
    CY_CRYPTO_FULL_LIBRARY  = 0x03u,
} cy_en_crypto_lib_info_t;


/**
* \addtogroup group_crypto_enums
* \{
*/

#if (CPUSS_CRYPTO_AES == 1)
/** The key length options for the AES method. */
typedef enum
{
    CY_CRYPTO_KEY_AES_128   = 0x00u,   /**< The AES key size is 128 bits */
    CY_CRYPTO_KEY_AES_192   = 0x01u,   /**< The AES key size is 192 bits */
    CY_CRYPTO_KEY_AES_256   = 0x02u    /**< The AES key size is 256 bits */
} cy_en_crypto_aes_key_length_t;
#endif /* #if (CPUSS_CRYPTO_AES == 1) */

/** Defines the direction of the Crypto methods */
typedef enum
{
    /** The forward mode, plain text will be encrypted into cipher text */
    CY_CRYPTO_ENCRYPT   = 0x00u,
    /** The reverse mode, cipher text will be decrypted into plain text */
    CY_CRYPTO_DECRYPT   = 0x01u
} cy_en_crypto_dir_mode_t;

#if (CPUSS_CRYPTO_SHA == 1)
/** Defines modes of SHA method */
typedef enum
{
#if (CPUSS_CRYPTO_SHA1 == 1)
    CY_CRYPTO_MODE_SHA1        = 0x00u,   /**< Sets the SHA1 mode */
#endif /* #if (CPUSS_CRYPTO_SHA1 == 1) */

#if (CPUSS_CRYPTO_SHA256 == 1)
    CY_CRYPTO_MODE_SHA224      = 0x11u,   /**< Sets the SHA224 mode */
    CY_CRYPTO_MODE_SHA256      = 0x01u,   /**< Sets the SHA256 mode */
#endif /* #if (CPUSS_CRYPTO_SHA256 == 1) */

#if (CPUSS_CRYPTO_SHA512 == 1)
    CY_CRYPTO_MODE_SHA384      = 0x12u,   /**< Sets the SHA384 mode */
    CY_CRYPTO_MODE_SHA512      = 0x02u,   /**< Sets the SHA512 mode */
    CY_CRYPTO_MODE_SHA512_256  = 0x22u,   /**< Sets the SHA512/256 mode */
    CY_CRYPTO_MODE_SHA512_224  = 0x32u    /**< Sets the SHA512/224 mode */
#endif /* #if (CPUSS_CRYPTO_SHA512 == 1) */
} cy_en_crypto_sha_mode_t;
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

/** Signature verification status */
typedef enum
{
    CY_CRYPTO_RSA_VERIFY_SUCCESS = 0x00u,   /**< PKCS1-v1.5 verify SUCCESS */
    CY_CRYPTO_RSA_VERIFY_FAIL    = 0x01u    /**< PKCS1-v1.5 verify FAILED */
} cy_en_crypto_rsa_ver_result_t;

/** Errors of the Crypto block */
typedef enum
{
    /** Operation completed successfully. */
    CY_CRYPTO_SUCCESS             = 0x00u,

    /** A hardware error occurred, detailed information is in stc_crypto_hw_error_t. */
    CY_CRYPTO_HW_ERROR            = CY_CRYPTO_ID | CY_PDL_STATUS_ERROR   | 0x01u,

    /** The size of input data is not multiple of 16. */
    CY_CRYPTO_SIZE_NOT_X16        = CY_CRYPTO_ID | CY_PDL_STATUS_ERROR   | 0x02u,

    /** The key for the DES method is weak. */
    CY_CRYPTO_DES_WEAK_KEY        = CY_CRYPTO_ID | CY_PDL_STATUS_WARNING | 0x03u,

    /** Communication between the client and server via IPC is broken. */
    CY_CRYPTO_COMM_FAIL           = CY_CRYPTO_ID | CY_PDL_STATUS_ERROR   | 0x04u,

    /** The Crypto server is not started. */
    CY_CRYPTO_SERVER_NOT_STARTED  = CY_CRYPTO_ID | CY_PDL_STATUS_ERROR   | 0x06u,

    /** The Crypto server in process state. */
    CY_CRYPTO_SERVER_BUSY         = CY_CRYPTO_ID | CY_PDL_STATUS_INFO    | 0x07u,

    /** The Crypto driver is not initialized. */
    CY_CRYPTO_NOT_INITIALIZED     = CY_CRYPTO_ID | CY_PDL_STATUS_ERROR   | 0x08u,

    /** The Crypto hardware is not enabled. */
    CY_CRYPTO_HW_NOT_ENABLED      = CY_CRYPTO_ID | CY_PDL_STATUS_ERROR   | 0x09u,

    /** The Crypto operation is not supported. */
    CY_CRYPTO_NOT_SUPPORTED       = CY_CRYPTO_ID | CY_PDL_STATUS_ERROR   | 0x0Au

} cy_en_crypto_status_t;

/** \} group_crypto_enums */

/** \cond INTERNAL */

/** Instruction to communicate between Client and Server */
typedef enum
{
    CY_CRYPTO_INSTR_UNKNOWN      = 0x00u,
    CY_CRYPTO_INSTR_ENABLE       = 0x01u,
    CY_CRYPTO_INSTR_DISABLE      = 0x02u,

#if (CPUSS_CRYPTO_PR == 1)
    CY_CRYPTO_INSTR_PRNG_INIT    = 0x03u,
    CY_CRYPTO_INSTR_PRNG         = 0x04u,
#endif /* #if (CPUSS_CRYPTO_PR == 1) */

#if (CPUSS_CRYPTO_TR == 1)
    CY_CRYPTO_INSTR_TRNG_INIT    = 0x05u,
    CY_CRYPTO_INSTR_TRNG         = 0x06u,
#endif /* #if (CPUSS_CRYPTO_PR == 1) */

#if (CPUSS_CRYPTO_AES == 1)
    CY_CRYPTO_INSTR_AES_INIT     = 0x07u,
    CY_CRYPTO_INSTR_AES_ECB      = 0x08u,
    CY_CRYPTO_INSTR_AES_CBC      = 0x09u,
    CY_CRYPTO_INSTR_AES_CFB      = 0x0Au,
    CY_CRYPTO_INSTR_AES_CTR      = 0x0Bu,
    CY_CRYPTO_INSTR_CMAC         = 0x0Cu,
#endif /* #if (CPUSS_CRYPTO_AES == 1) */

#if (CPUSS_CRYPTO_SHA == 1)
    CY_CRYPTO_INSTR_SHA          = 0x0Du,
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

#if (CPUSS_CRYPTO_SHA == 1)
    CY_CRYPTO_INSTR_HMAC         = 0x0Eu,
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

#if (CPUSS_CRYPTO_STR == 1)
    CY_CRYPTO_INSTR_MEM_CPY      = 0x0Fu,
    CY_CRYPTO_INSTR_MEM_SET      = 0x10u,
    CY_CRYPTO_INSTR_MEM_CMP      = 0x11u,
    CY_CRYPTO_INSTR_MEM_XOR      = 0x12u,
#endif /* #if (CPUSS_CRYPTO_STR == 1) */

#if (CPUSS_CRYPTO_CRC == 1)
    CY_CRYPTO_INSTR_CRC_INIT     = 0x13u,
    CY_CRYPTO_INSTR_CRC          = 0x14u,
#endif /* #if (CPUSS_CRYPTO_CRC == 1) */

#if (CPUSS_CRYPTO_DES == 1)
    CY_CRYPTO_INSTR_DES          = 0x15u,
    CY_CRYPTO_INSTR_3DES         = 0x16u,
#endif /* #if (CPUSS_CRYPTO_DES == 1) */

#if (CPUSS_CRYPTO_VU == 1)
    CY_CRYPTO_INSTR_RSA_PROC     = 0x17u,
    CY_CRYPTO_INSTR_RSA_COEF     = 0x18u,
#endif /* #if (CPUSS_CRYPTO_VU == 1) */

#if (CPUSS_CRYPTO_SHA == 1)
    CY_CRYPTO_INSTR_RSA_VER      = 0x19u,
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

    CY_CRYPTO_INSTR_SRV_INFO     = 0x55u
} cy_en_crypto_comm_instr_t;

/** \endcond */

/**
* \addtogroup group_crypto_cli_data_structures
* \{
*/

#if (CPUSS_CRYPTO_AES == 1)
/** Structure for storing the AES state */
typedef struct
{
    /** Pointer to AES key */
    uint32_t *key;
    /** Pointer to AES inversed key */
    uint32_t *invKey;
    /** AES key length */
    cy_en_crypto_aes_key_length_t keyLength;
    /** AES processed block index (for CMAC, SHA operations) */
    uint32_t blockIdx;
} cy_stc_crypto_aes_state_t;
#endif /* #if (CPUSS_CRYPTO_AES == 1) */

/** \} group_crypto_cli_data_structures */

/*************************************************************
*  Structures used for communication between Client and Server
***************************************************************/

/**
* \addtogroup group_crypto_srv_data_structures
* \{
*/

/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the server
 * context.
 */
typedef struct
{
    /** IPC communication channel number */
    uint32_t ipcChannel;
    /** IPC acquire interrupt channel number */
    uint32_t acquireNotifierChannel;
    /** IPC release interrupt channel number */
    cy_israddress   getDataHandlerPtr;
    /** Crypto hardware errors interrupt handler */
    cy_israddress   errorHandlerPtr;
    /** Acquire notifier interrupt configuration */
    cy_stc_sysint_t acquireNotifierConfig;
    /** Crypto hardware errors interrupt configuration */
    cy_stc_sysint_t cryptoErrorIntrConfig;
    /** Hardware error occurrence flag */
    bool            isHwErrorOccured;
} cy_stc_crypto_server_context_t;

/** \} group_crypto_srv_data_structures */

/**
* \addtogroup group_crypto_cli_data_structures
* \{
*/

/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the global
 * context.
 */
typedef struct
{
    /** Operation instruction code */
    cy_en_crypto_comm_instr_t instr;
    /** Response from executed crypto function */
    cy_en_crypto_status_t resp;
    /** Hardware processing errors */
    cy_stc_crypto_hw_error_t hwErrorStatus;
    /** IPC communication channel number */
    uint32_t ipcChannel;
    /** IPC acquire interrupt channel number */
    uint32_t acquireNotifierChannel;
    /** IPC release interrupt channel number */
    uint32_t releaseNotifierChannel;
    /** User callback for Crypto HW calculation complete event */
    cy_crypto_callback_ptr_t userCompleteCallback;
    /** Release notifier interrupt configuration */
    cy_stc_sysint_t releaseNotifierConfig;
    /** Pointer to the crypto function specific context data */
    void *xdata;
} cy_stc_crypto_context_t;


#if (CPUSS_CRYPTO_DES == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the DES operational
 * context.
 */
typedef struct
{
    /**  Operation direction (Encrypt / Decrypt) */
    cy_en_crypto_dir_mode_t dirMode;
    /**  Pointer to key data */
    uint32_t *key;
    /**  Pointer to data destination block */
    uint32_t *dst;
    /**  Pointer to data source block */
    uint32_t *src;
} cy_stc_crypto_context_des_t;
#endif /* #if (CPUSS_CRYPTO_DES == 1) */

#if (CPUSS_CRYPTO_AES == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the AES operational
 * context.
 */
typedef struct
{
    /** AES state data */
    cy_stc_crypto_aes_state_t aesState;
    /** Operation direction (Encrypt / Decrypt) */
    cy_en_crypto_dir_mode_t dirMode;
    /** Operation data size */
    uint32_t srcSize;
    /** Size of the last non-complete block (for CTR mode only) */
    uint32_t *srcOffset;
    /** Initialization vector, in the CTR mode is used as nonceCounter */
    uint32_t *ivPtr;
    /** AES processed block pointer (for CTR mode only) */
    uint32_t *streamBlock;
    /** Pointer to data destination block */
    uint32_t *dst;
    /** Pointer to data source block */
    uint32_t *src;
} cy_stc_crypto_context_aes_t;
#endif /* #if (CPUSS_CRYPTO_AES == 1) */

#if (CPUSS_CRYPTO_SHA == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the SHA operational
 * context.
 */
typedef struct
{
    /** Pointer to data source block */
    uint32_t *message;
    /** Operation data size */
    uint32_t  messageSize;
    /** Pointer to data destination block */
    uint32_t *dst;
    /** SHA mode */
    cy_en_crypto_sha_mode_t mode;
    /** Pointer to key data (for HMAC only) */
    uint32_t *key;
    /** Key data length (for HMAC only) */
    uint32_t  keyLength;
} cy_stc_crypto_context_sha_t;
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

#if (CPUSS_CRYPTO_PR == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the PRNG operational
 * context.
 */
typedef struct
{
    uint32_t lfsr32InitState;             /**< lfsr32 initialization data */
    uint32_t lfsr31InitState;             /**< lfsr31 initialization data */
    uint32_t lfsr29InitState;             /**< lfsr29 initialization data */
    uint32_t max;                         /**< Maximum of the generated value */
    uint32_t *prngNum;                    /**< Pointer to generated value */
} cy_stc_crypto_context_prng_t;
#endif /* #if (CPUSS_CRYPTO_PR == 1) */

#if (CPUSS_CRYPTO_TR == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the TRNG operational
 * context.
 */
typedef struct
{
    /**
     The polynomial for the programmable Galois ring oscillator (TR_GARO_CTL).
     The polynomial is represented WITHOUT the high order bit (this bit is
     always assumed '1').
     The polynomial should be aligned so that more significant bits
     (bit 30 and down) contain the polynomial and less significant bits
     (bit 0 and up) contain padding '0's. */
    uint32_t GAROPol;

    /**
     The polynomial for the programmable Fibonacci ring oscillator(TR_FIRO_CTL).
     The polynomial is represented WITHOUT the high order bit (this bit is
     always assumed '1').
     The polynomial should be aligned so that more significant bits
     (bit 30 and down) contain the polynomial and less significant bits
     (bit 0 and up) contain padding '0's. */
    uint32_t FIROPol;
    /** Maximum of the generated value */
    uint32_t max;
    /** Pointer to generated value */
    uint32_t *trngNum;
} cy_stc_crypto_context_trng_t;
#endif /* #if (CPUSS_CRYPTO_TR == 1) */

#if (CPUSS_CRYPTO_STR == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the STR operational
 * context.
 */
typedef struct
{
    void const *src0;         /**<  Pointer to 1-st string source */
    void const *src1;         /**<  Pointer to 2-nd string source */
    void       *dst;          /**<  Pointer to string destination */
    uint32_t   dataSize;      /**<  Operation data size */
    uint32_t   data;          /**<  Operation data value (for memory setting) */
} cy_stc_crypto_context_str_t;
#endif /* #if (CPUSS_CRYPTO_STR == 1) */

#if (CPUSS_CRYPTO_CRC == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the CRC operational
 * context.
 */
typedef struct
{
    void*    srcData;               /**<  Pointer to data source block */
    uint32_t dataSize;              /**<  Operation data size */
    uint32_t *crc;                  /**<  Pointer to CRC destination variable */
    uint32_t polynomial;            /**<  Polynomial for CRC calculate */
    uint32_t lfsrInitState;         /**<  CRC calculation initial value */
    uint32_t dataReverse;           /**<  Input data reverse flag */
    uint32_t dataXor;               /**<  Input data  XOR flag */
    uint32_t remReverse;            /**<  Output data reverse flag */
    uint32_t remXor;                /**<  Output data XOR flag */
} cy_stc_crypto_context_crc_t;
#endif /* #if (CPUSS_CRYPTO_CRC == 1) */

#if (CPUSS_CRYPTO_VU == 1)

#if (CPUSS_CRYPTO_SHA == 1)
/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the RSA verifying
 * context.
 */
typedef struct
{
    /** Pointer to verification result /ref cy_en_crypto_rsa_ver_result_t */
    cy_en_crypto_rsa_ver_result_t *verResult;
    /** SHA digest type, used with SHA calculation of the message */
    cy_en_crypto_sha_mode_t digestType;
    /** SHA digest of the message, calculated with digestType */
    uint32_t const *hash;
    /** Previously decrypted RSA signature */
    uint32_t const *decryptedSignature;
    /** Length of the decrypted RSA signature */
    uint32_t decryptedSignatureLength;
} cy_stc_crypto_context_rsa_ver_t;
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

/**
 * Firmware allocates memory and provides a pointer to this structure in
 * function calls. Firmware does not write or read values in this structure.
 * The driver uses this structure to store and manipulate the RSA operational
 * context.
 */
typedef struct
{
    /** Pointer to key data */
    cy_stc_crypto_rsa_pub_key_t const *key;
    /** Pointer to data source block */
    uint32_t const *message;
    /** Operation data size */
    uint32_t messageSize;
    /** Pointer to data destination block */
    uint32_t *result;
} cy_stc_crypto_context_rsa_t;
#endif /* #if (CPUSS_CRYPTO_VU == 1) */

/** \} group_crypto_cli_data_structures */

#endif  /* (CPUSS_CRYPTO_PRESENT == 1) */

#endif /* CY_IP_MXCRYPTO */

#endif /* #if !defined(CY_CRYPTO_COMMON_H) */

/* [] END OF FILE */
