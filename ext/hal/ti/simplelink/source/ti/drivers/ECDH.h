/*
 * Copyright (c) 2017, Texas Instruments Incorporated
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
/*!
 * @file ECDH.h
 *
 * @brief TI Driver for Elliptic Curve Diffie-Hellman key agreement scheme.
 *
 *
 * @warning This is a beta API. It may change in future releases.
 *
 * # Overview #
 *
 * Elliptic Curve Diffie-Hellman (ECDH) is a key agreement scheme between
 * two parties based on the Diffie-Hellman key exchange protocol.
 *
 * It provides a means of generating a shared secret and derived symmetric key
 * between the two parties over an insecure channel.
 *
 * It does not provide authentication. As such, it does not guarantee that the
 * party you are exchanging keys with is truly the party you wish to establish a
 * secured channel with.
 *
 * The two parties each generate a private key and a public key. The private key
 * is a random integer in the interval [1, n - 1], where n is the order of a
 * previously agreed upon curve. The public key is generated
 * by multiplying the private key by the generator point of a previously agreed
 * upon elliptic curve such as NISTP256 or Curve 25519. The public key is itself
 * a point upon the elliptic curve. Each public key is then transmitted to the
 * other party over a potentially insecure channel. The other party's public key
 * is then multiplied with the private key, generating a shared secret. This
 * shared secret is also a point on the curve. However, the entropy in the secret
 * is not spread evenly throughout the shared secret. In order to generate one or more
 * shared symmetric keys, the shared secret must be run through a key derivation
 * function (KDF) that was previously agreed upon. The key derivation function
 * can take many forms, from simply hashing the X coordinate of the shared secret
 * with SHA2 and truncating the result to generating multiple symmetric keys with
 * HKDF, an HMAC based KDF.
 *
 * # Usage #
 *
 * ## Before starting an ECDH operation #
 *
 * Before starting a CCM operation, the application must do the following:
 *      - Call ECDH_init() to initialize the driver
 *      - Call ECDH_Params_init() to initialize the ECDH_Params to default values.
 *      - Modify the ECDH_Params as desired
 *      - Call ECDH_open() to open an instance of the driver
 *
 * ## Generating your public-private key pair #
 * To generate a public-private key pair for an agreed upon curve, the application
 * must do the following:
 *      - Generate the keying material for the private key. This keying material must
 *        be an integer in the interval [1, n - 1], where n is the order of the curve.
 *        It should be stored in an array with the least significant byte of the integer
 *        hex representation stored in the lowest address of the array (little-endian).
 *        The array should be the same length as the curve parameters of the curve used.
 *        The driver can be configured to validate public and private keys against the
 *        provided curve.
 *      - Initialize the private key CryptoKey. CryptoKeys are opaque datastructures and representations
 *        of keying material and its storage. Depending on how the keying material
 *        is stored (RAM or flash, key store, key blob), the CryptoKey must be
 *        initialized differently. The ECDH API can handle all types of CryptoKey.
 *        However, not all device-specific implementions support all types of CryptoKey.
 *        Devices without a key store will not support CryptoKeys with keying material
 *        stored in a key store for example.
 *        All devices support plaintext CryptoKeys.
 *      - Initialize a blank CryptoKey for the public key. The CryptoKey will keep track
 *        of where the keying material for the public key should be copied and how
 *        long it is. It should have twice the length of the private key.
 *      - Call ECDH_generatePublicKey(). The generated keying material will be copied
 *        according the the CryptoKey passed in as the public key parameter. The CryptoKey
 *        will no longer be considered 'blank' after the operation.
 *
 * ## Calculating a shared secret #
 * After trading public keys with the other party, the application should do the following
 * to calculate the shared secret:
 *      - Initialize a CryptoKey as public key with the keying material received from the other
 *        party.
 *      - Initialize a blank CryptoKey with the same size as the previously initialized
 *        public key.
 *      - Call ECDH_computeSharedSecret(). The shared secret will be copied to a location
 *        according to the shared secret CryptoKey passed to the function call.
 *
 *
 * ## Creating one or more symmetric keys from the shared secret #
 * After calculating the shared secret between the application and the other party,
 * the entropy in the shared secret must be evened out and stretched as needed. The API
 * allows for any number of keys to be derived from the shared secret using an application-provided
 * key-derivation function. There are uncountable methods and algorithms to stretch an original
 * seed entropy (the share secret) to generate symmetric keys.
 *      - Initialize n blank CryptoKeys in an array. These CryptoKeys is where the generated
 *        king material will be copied to. The keying material they represent does
 *        not need to be a contiguous range or even of the same CryptoKey type. The array may
 *        contain Plaintext, key blob, or key store CryptoKeys. It may contain different types
 *        of CryptoKeys.
 *        While the API allows for full flexibility in key material location and length, not all
 *        key derivation functions support the full scope of flexibility. Check the KDF's documentation
 *        before using it.
 *      - Call ECDH_calculateSharedEntropy(). The generated keying material will be copied to a
 *        location according to the CryptoKeys passed to the function.
 *
 * ## After a key exchange #
 * After the ECDH key exchange completes, the application should either start another operation
 * or close the driver by calling ECDH_close()
 *
 * ## General usage #
 * The API expects elliptic curves in short Weierstrass form as defined in
 * ti/drivers/types/ECCParams.h. Several commonly used curves are provided.
 *
 * Public keys and shared secrets are points on an elliptic curve. These points can
 * be expressed in several ways. The most common one is in affine coordinates as an
 * X,Y pair. The Y value can be omitted when using point compression and doing
 * the calculations in the Montgomery domain with X,Z coordinates.
 * This API only uses points expressed in affine coordinates. The point is stored as a
 * concatenated array of X followed by Y.
 *
 * ## ECDH Driver Configuration #
 *
 * In order to use the ECDH APIs, the application is required
 * to provide device-specific ECDH configuration in the Board.c file.
 * The ECDH driver interface defines a configuration data structure:
 *
 * @code
 * typedef struct ECDH_Config_ {
 *     void                   *object;
 *     void          const    *hwAttrs;
 * } ECDH_Config;
 * @endcode
 *
 * The application must declare an array of ECDH_Config elements, named
 * ECDH_config[].  Each element of ECDH_config[] must be populated with
 * pointers to a device specific ECDH driver implementation's
 * driver object, hardware attributes.  The hardware attributes
 * define properties such as the ECDH peripheral's base address.
 * Each element in ECDH_config[] corresponds to
 * an ECDH instance, and none of the elements should have NULL pointers.
 * There is no correlation between the index and the
 * peripheral designation (such as ECDH0 or ECDH1).  For example, it is
 * possible to use ECDH_config[0] for ECDH1. Multiple drivers and driver
 * instances may all access the same underlying hardware. This is transparent
 * to the application. Mutual exclusion is performed automatically by the
 * drivers as necessary.
 *
 * Because the ECDH configuration is very device dependent, you will need to
 * check the doxygen for the device specific ECDH implementation.  There you
 * will find a description of the ECDH hardware attributes.  Please also
 * refer to the Board.c file of any of your examples to see the ECDH
 * configuration.
 *
 * ## ECDH Parameters #
 *
 * The #ECDH_Params structure is passed to the ECDH_open() call.  If NULL
 * is passed for the parameters, ECDH_open() uses default parameters.
 * An #ECDH_Params structure is initialized with default values by passing
 * it to ECDH_Params_init().
 * Some of the ECDH parameters are described below. To see brief descriptions
 * of all the parameters, see #ECDH_Params.
 *
 * ## ECDH Return Behavior
 * The ECDH driver supports three return behaviors when processing data: blocking, polling, and
 * callback. The ECDH driver defaults to blocking mode, if the application does not set it.
 * Once an ECDH driver is opened, the only way to change the return behavior
 * is to close and re-open the ECDH instance with the new return behavior.
 *
 * In blocking mode, a task's code execution is blocked until an ECDH
 * operation has completed. This ensures that only one ECDH operation
 * operates at a given time. Other tasks requesting ECDH operations while
 * a operation is currently taking place are also placed into a blocked
 * state. ECDH operations are executed in the order in which they were
 * received.  In blocking mode, you cannot perform ECDH operations
 * in the context of a software or hardware ISR.
 *
 * In callback mode, an ECDH operation functions asynchronously, which
 * means that it does not block code execution. After an ECDH operation
 * has been completed, the ECDH driver calls a user-provided hook function.
 * Callback mode is supported in the task, SWI, and HWI execution context,
 * However, if an ECDH operation is requested while a operation is taking place,
 * the call returns an error code.
 *
 * In polling mode, an ECDH operation behaves the almost the same way as
 * in blocking mode. Instead of pending on a semaphore and letting other
 * scheduled tasks run, the application task, SWI, or HWI continuously polls
 * a flag until the operation completes. If an ECDH operation is
 * requested while a operation is taking place, the call returns an error code.
 * When starting an ECDH operation in polling mode from HWI or SWI context,
 * the ECDH HWI and SWI must be configured to have a higher priority to pre-empt
 * the polling context.
 *
 * # Examples #
 *
 * ## ECDH exchange with plaintext CryptoKeys #
 *
 * @code
 *
 * #include <ti/drivers/cryptoutils/cryptokey/CryptoKeyPlaintext.h>
 * #include <ti/drivers/ECDH.h>
 *
 * ...
 *
 * // Our private key is 0x0000000000000000000000000000000000000000000000000000000000000001
 * // In practice, this value should come from a TRNG, PRNG, PUF, or device-specific pre-seeded key
 * uint8_t myPrivateKeyingMaterial[32] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 *                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 *                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 *                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 * uint8_t myPublicKeyingMaterial[64] = {0};
 * uint8_t theirPublicKeyingMaterial[64] = {0};
 * uint8_t sharedSecretKeyingMaterial[64] = {0};
 * uint8_t symmetricKeyingMaterial[16] = {0};
 *
 * CryptoKey myPrivateKey;
 * CryptoKey myPublicKey;
 * CryptoKey theirPublicKey;
 * CryptoKey sharedSecret;
 * CryptoKey symmetricKey;
 *
 * ECDH_Handle ecdhHandle;
 *
 * int_fast16_t operationResult;
 *
 * // Since we are using default ECDH_Params, we just pass in NULL for that parameter.
 * ecdhHandle = ECDH_open(0, NULL);
 *
 * if (!ecdhHandle) {
 *     // Handle error
 * }
 *
 * // Initialize myPrivateKey and myPublicKey
 * CryptoKeyPlaintext_initKey(&myPrivateKey, myPrivateKeyingMaterial, sizeof(myPrivateKeyingMaterial));
 * CryptoKeyPlaintext_initBlankKey(&myPublicKey, myPublicKeyingMaterial, sizeof(myPublicKeyingMaterial));
 *
 * // Generate the keying material for myPublicKey and store it in myPublicKeyingMaterial
 * operationResult = ECDH_generatePublicKey(ecdhHandle, &ECC_NISTP256, &myPrivateKey, &myPublicKey);
 *
 * if (operationResult != ECDH_STATUS_SUCCESS) {
 *     // Handle error
 * }
 *
 * // Now send the content of myPublicKeyingMaterial to theother party,
 * // receive their public key, and copy their public keying material to theirPublicKeyingMaterial
 *
 * // Initialise their public CryptoKey and the shared secret CryptoKey
 * CryptoKeyPlaintext_initKey(&theirPublicKey, theirPublicKeyingMaterial, sizeof(theirPublicKeyingMaterial));
 * CryptoKeyPlaintext_initBlankKey(&sharedSecret, sharedSecretKeyingMaterial, sizeof(sharedSecretKeyingMaterial));
 *
 * // Compute the shared secret and copy it to sharedSecretKeyingMaterial
 * // The ECC_NISTP256 struct is provided in ti/drivers/types/EccPArams.h and the corresponding device-specific implementation
 * operationResult = ECDH_computeSharedSecret(ecdhHandle, &ECC_NISTP256, &myPrivateKey, &theirPublicKey, &sharedSecret);
 *
 * if (operationResult != ECDH_STATUS_SUCCESS) {
 *     // Handle error
 * }
 *
 * CryptoKeyPlaintext_initBlankKey(&symmetricKey, symmetricKeyingMaterial, sizeof(symmetricKeyingMaterial));
 *
 * // Set up a KDF such as HKDF and open the requisite cryptographic primitive driver to implement it
 * // HKDF and SHA2 were chosen as an example and may not be available directly
 * // Since we only have one symmetric CryptoKey we wish to generate, we can pass it directly to the function
 * operationResult = ECDH_calculateSharedEntropy(ecdhHandle, &sharedSecret, KDF_HKDF, sha2Handle, &symmetricKey, 1);
 *
 * if (operationResult != ECDH_STATUS_SUCCESS) {
 *     // Handle error
 * }
 *
 * // At this point, you and the other party have both created the content within symmetricKeyingMaterial without
 * // someone else listening to your communication channel being able to do so
 *
 * @endcode
 *
 *
 */


#ifndef ti_drivers_ECDH__include
#define ti_drivers_ECDH__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/drivers/cryptoutils/cryptokey/CryptoKey.h>
#include <ti/drivers/cryptoutils/ecc/ECCParams.h>

/**
 *  @defgroup ECDH_CONTROL ECDH_control command and status codes
 *  These ECC macros are reservations for ECC.h
 *  @{
 */

/*!
 * Common ECDH_control command code reservation offset.
 * ECC driver implementations should offset command codes with ECDH_CMD_RESERVED
 * growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define ECCXYZ_CMD_COMMAND0     ECDH_CMD_RESERVED + 0
 * #define ECCXYZ_CMD_COMMAND1     ECDH_CMD_RESERVED + 1
 * @endcode
 */
#define ECDH_CMD_RESERVED           (32)

/*!
 * Common ECDH_control status code reservation offset.
 * ECC driver implementations should offset status codes with
 * ECDH_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define ECCXYZ_STATUS_ERROR0    ECDH_STATUS_RESERVED - 0
 * #define ECCXYZ_STATUS_ERROR1    ECDH_STATUS_RESERVED - 1
 * #define ECCXYZ_STATUS_ERROR2    ECDH_STATUS_RESERVED - 2
 * @endcode
 */
#define ECDH_STATUS_RESERVED        (-32)

/**
 *  @defgroup ECDH_STATUS Status Codes
 *  ECDH_STATUS_* macros are general status codes returned by ECC functions
 *  @{
 *  @ingroup ECDH_CONTROL
 */

/*!
 * @brief   Successful status code.
 *
 * Function return ECDH_STATUS_SUCCESS if the control code was executed
 * successfully.
 */
#define ECDH_STATUS_SUCCESS         (0)

/*!
 * @brief   Generic error status code.
 *
 * Functions return ECDH_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define ECDH_STATUS_ERROR           (-1)

/*!
 * @brief   An error status code returned by ECDH_control() for undefined
 * command codes.
 *
 * ECDH_control() returns ECDH_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define ECDH_STATUS_UNDEFINEDCMD    (-2)

/*!
 * @brief   An error status code returned if the hardware or software resource
 * is currently unavailable.
 *
 * ECC driver implementations may have hardware or software limitations on how
 * many clients can simultaneously perform operations. This status code is returned
 * if the mutual exclusion mechanism signals that an operation cannot currently be performed.
 */
#define ECDH_STATUS_RESOURCE_UNAVAILABLE (-3)

/*!
 * @brief   The result of the operation is the point at infinity.
 *
 * The operation yielded the point at infinity on this curve. This point is
 * not permitted for further use in ECC operations.
 */
#define ECDH_STATUS_RESULT_POINT_AT_INFINITY (-4)

/*!
 * @brief   The private key passed in is larger than the order of the curve.
 *
 * Private keys must be integers in the interval [1, n - 1], where n is the
 * order of the curve.
 */
#define ECDH_STATUS_RESULT_PRIVATE_KEY_LARGER_THAN_ORDER (-5)

/*!
 * @brief   The public key of the other party does not lie upon the curve.
 *
 * The public key received from the other party does not lie upon the agreed upon
 * curve.
 */
#define ECDH_STATUS_RESULT_PUBLIC_KEY_NOT_ON_CURVE (-6)


/** @}*/

/**
 *  @defgroup ECDH_CMD Command Codes
 *  ECDH_CMD_* macros are general command codes for ECDH_control(). Not all ECC
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup ECDH_CONTROL
 */

/* Add ECDH_CMD_<commands> here */

/** @}*/

/** @}*/

/*!
 *  @brief  A handle that is returned from an ECDH_open() call.
 */
typedef struct ECDH_Config_    *ECDH_Handle;

/*!
 *  @brief  The definition of a callback function used by the ECC driver
 *          when used in ::ECDH_RETURN_BEHAVIOR_CALLBACK
 *
 *  @param  handle Handle of the client that started the ECC operation.
 *
 *  @param  operationStatus The result of the ECC operation. May contain an error code
 *          if the result is the point at infinity for example.
 */
typedef void (*ECDH_CallbackFxn) (ECDH_Handle handle, int_fast16_t operationStatus);

/*!
 * @brief   The way in which ECC function calls return after performing an
 * encryption + authentication or decryption + verification operation.
 *
 * Not all ECC operations exhibit the specified return behavor. Functions that do not
 * require significant computation and cannot offload that computation to a background thread
 * behave like regular functions. Which functions exhibit the specfied return behavior is not
 * implementation dependent. Specifically, a software-backed implementation run on the same
 * CPU as the application will emulate the return behavior while not actually offloading
 * the computation to the background thread.
 *
 * ECC functions exhibiting the specified return behavior have restrictions on the
 * context from which they may be called.
 *
 * |                                | Task  | Hwi   | Swi   |
 * |--------------------------------|-------|-------|-------|
 * |ECDH_RETURN_BEHAVIOR_CALLBACK | X     | X     | X     |
 * |ECDH_RETURN_BEHAVIOR_BLOCKING | X     |       |       |
 * |ECDH_RETURN_BEHAVIOR_POLLING  | X     | X     | X     |
 *
 */
typedef enum ECDH_ReturnBehavior_ {
    ECDH_RETURN_BEHAVIOR_CALLBACK = 1,    /*!< The function call will return immediately while the
                                             *   ECC operation goes on in the background. The registered
                                             *   callback function is called after the operation completes.
                                             *   The context the callback function is called (task, HWI, SWI)
                                             *   is implementation-dependent.
                                             */
    ECDH_RETURN_BEHAVIOR_BLOCKING = 2,    /*!< The function call will block while ECC operation goes
                                             *   on in the background. ECC operation results are available
                                             *   after the function returns.
                                             */
    ECDH_RETURN_BEHAVIOR_POLLING  = 4,    /*!< The function call will continuously poll a flag while ECC
                                             *   operation goes on in the background. ECC operation results
                                             *   are available after the function returns.
                                             */
} ECDH_ReturnBehavior;

/*!
 *  @brief  ECC Parameters
 *
 *  ECC Parameters are used to with the ECDH_open() call. Default values for
 *  these parameters are set using ECDH_Params_init().
 *
 *  @sa     ECDH_Params_init()
 */
typedef struct ECDH_Params_ {
    bool                    doNotValidateKeys;          /*!< Whether or not to validate public and private keys in operations. */
    ECDH_ReturnBehavior     returnBehavior;             /*!< Blocking, callback, or polling return behavior */
    ECDH_CallbackFxn        callbackFxn;                /*!< Callback function pointer */
    uint32_t                timeout;
    void                   *custom;                     /*!< Custom argument used by driver
                                                         *   implementation
                                                         */
} ECDH_Params;

/*!
 *  @brief ECC Global configuration
 *
 *  The ECDH_Config structure contains a set of pointers used to characterize
 *  the ECC driver implementation.
 *
 *  This structure needs to be defined before calling ECDH_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     ECDH_init()
 */
typedef struct ECDH_Config_ {
    /*! Pointer to a driver specific data object */
    void               *object;

    /*! Pointer to a driver specific hardware attributes structure */
    void         const *hwAttrs;
} ECDH_Config;

/*!
 *  @brief  Function that implements a key derivation function (KDF)
 *
 *  Key derivation functions take a string of random entropy as their input and generate
 *  a deterministic pseudo-random bit stream of arbitrary length. Specialised KDFs may
 *  produce outputs of fixed length or with maximum lengths.
 *
 *  @param [in]     driverHandle        Handle of the of the driver used to execute the crypto
 *                                      primitive underlying the KDF. This primitive could be a
 *                                      cryptographic hash or a block cipher.
 *
 *  @param [in]     seedEntropy         The entropy used to seed the KDF. Reusing the same seed
 *                                      will yield the same pseudo-random bit stream.
 *
 *  @param [in]     seedEntropyLength   Length of the seed entropy.
 *
 *  @param [out]    derivedKeys         The CryptoKeys seeded with deterministic pseudo-random output of the KDF.
 *
 *  @param [in]     derivedKeysCount    The number of keys to seed.
 */
typedef int_fast16_t (*ECDH_KDFFxn) (void *driverHandle, const uint8_t *seedEntropy, size_t seedEntropyLength, CryptoKey derivedKeys[], uint32_t derivedKeysCount);

/*!
 *  @brief Default ECDH_Params structure
 *
 *  @sa     ECDH_Params_init()
 */
extern const ECDH_Params ECDH_defaultParams;

/*!
 *  @brief  This function initializes the ECC module.
 *
 *  @pre    The ECDH_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other ECC driver APIs. This function call does not modify any
 *          peripheral registers.
 */
void ECDH_init(void);

/*!
 *  @brief  Function to initialize the ECDH_Params struct to its defaults
 *
 *  @param  params      An pointer to ECDH_Params structure for
 *                      initialization
 *
 *  Defaults values are:
 *      returnBehavior              = ECDH_RETURN_BEHAVIOR_BLOCKING
 *      callbackFxn                 = NULL
 *      timeout                     = SemaphoreP_WAIT_FOREVER
 *      custom                      = NULL
 */
void ECDH_Params_init(ECDH_Params *params);

/*!
 *  @brief  This function opens a given ECC peripheral.
 *
 *  @pre    ECC controller has been initialized using ECDH_init()
 *
 *  @param  index         Logical peripheral number for the ECC indexed into
 *                        the ECDH_config table
 *
 *  @param  params        Pointer to an parameter block, if NULL it will use
 *                        default values.
 *
 *  @return A ECDH_Handle on success or a NULL on an error or if it has been
 *          opened already.
 *
 *  @sa     ECDH_init()
 *  @sa     ECDH_close()
 */
ECDH_Handle ECDH_open(uint_least8_t index, ECDH_Params *params);

/*!
 *  @brief  Function to close a ECC peripheral specified by the ECC handle
 *
 *  @pre    ECDH_open() has to be called first.
 *
 *  @param  handle A ECC handle returned from ECDH_open()
 *
 *  @sa     ECDH_open()
 */
void ECDH_close(ECDH_Handle handle);

/*!
 *  @brief  Function performs implementation specific features on a given
 *          ECDH_Handle.
 *
 *  Commands for ECDH_control can originate from ECC.h or from implementation
 *  specific ECC*.h (_ECCCC26XX.h_, _EECCSP432.h_, etc.. ) files.
 *  While commands from ECC.h are API portable across driver implementations,
 *  not all implementations may support all these commands.
 *  Conversely, commands from driver implementation specific ECC*.h files add
 *  unique driver capabilities but are not API portable across all ECC driver
 *  implementations.
 *
 *  Commands supported by ECC.h follow an ECDH_CMD_\<cmd\> naming
 *  convention.<br>
 *  Commands supported by ECC*.h follow an ECC*_CMD_\<cmd\> naming
 *  convention.<br>
 *  Each control command defines @b arg differently. The types of @b arg are
 *  documented with each command.
 *
 *  See @ref ECDH_CMD "ECDH_control command codes" for command codes.
 *
 *  See @ref ECDH_STATUS "ECDH_control return status codes" for status codes.
 *
 *  @pre    ECDH_open() has to be called first.
 *
 *  @param  handle      A ECC handle returned from ECDH_open()
 *
 *  @param  cmd         ECC.h or ECC*.h commands.
 *
 *  @param  args        An optional R/W (read/write) command argument
 *                      accompanied with cmd
 *
 *  @return Implementation specific return codes. Negative values indicate
 *          unsuccessful operations.
 *
 *  @sa     ECDH_open()
 */
int_fast16_t ECDH_control(ECDH_Handle handle, uint32_t cmd, void *args);





/*!
 * @brief Generates a public key for use in key agreement.
 *
 *  ECDH_generateKey() can be used for generating ephemeral keys.
 *
 *  @param      handle          A ECC handle returned from ECDH_open()
 *  @param      eccParams       A pointer to the elliptic curve parameters for myPrivateKey
 *  @param      myPrivateKey    A pointer to the private ECC key from which the new public key will be generated. (maybe your static key)
 *  @param      myPublicKey     A pointer to a public ECC key which has been initialized blank.  Newly generated
 *                              key will be placed in this location.
 *
 *  \post ECDH_computeSharedSecret()
 *
 */
int_fast16_t ECDH_generatePublicKey(ECDH_Handle handle, const ECCParams_CurveParams *eccParams, const CryptoKey *myPrivateKey, CryptoKey *myPublicKey);

/*!
 *  @brief Computes a shared secret
 *
 *  This secret can be used to generate shared keys for encryption and authentication.
 *
 *  @param      handle              A ECC handle returned from ECDH_open()
 *  @param      eccParams           A pointer to the elliptic curve parameters for myPrivateKey (if ECDH_generateKey() was used, this should be the same private key)
 *  @param      myPrivateKey        A pointer to the private ECC key which will be used in to compute the shared secret
 *  @param      theirPublicKey      A pointer to the public key of the party with whom the shared secret will be generated
 *  @param      sharedSecret        A pointer to a CryptoKey which has been initialized blank.  The shared secret will be placed here.
 *
 *  \post ECDH_calculateSharedEntropy()
 */
int_fast16_t ECDH_computeSharedSecret(ECDH_Handle handle, const ECCParams_CurveParams *eccParams, const CryptoKey *myPrivateKey, const CryptoKey *theirPublicKey, CryptoKey *sharedSecret);

/*!
 *  @brief Calculates key material
 *
 *  \pre ECDH_computeSharedSecret()
 *
 *  @param      handle                      A ECC handle returned from ECDH_open()
 *
 *  @param      sharedSecret                The shared secret produced by ECDH_calculateSharedEntropy()
 *
 *  @param      kdf                         A pointer to the key derivation function to be used
 *
 *  @param      kdfPrimitiveDriverHandle    A pointer to the handle of a driver of a cryptographic primitive used to implement the KDF
 *
 *  @param      derivedKeys                 A an array of CryptoKeys to seed with calculated entropy
 *
 *  @param      derivedKeysCount            The number of CryptoKeys to seed with calculated entropy
 *
 */
int_fast16_t ECDH_calculateSharedEntropy(ECDH_Handle handle, const CryptoKey *sharedSecret, const ECDH_KDFFxn kdf, void *kdfPrimitiveDriverHandle, CryptoKey derivedKeys[], uint32_t derivedKeysCount);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_ECDH__include */
