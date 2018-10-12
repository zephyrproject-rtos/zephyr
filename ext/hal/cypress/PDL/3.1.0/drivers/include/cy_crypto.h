/***************************************************************************//**
* \file cy_crypto.h
* \version 2.10
*
* \brief
*  This file provides the public interface for the Crypto driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_crypto Cryptography (Crypto)
* \{
* The Crypto driver provides a public API to perform cryptographic and hash
* operations, as well as generate both true and pseudo random numbers
* (TRNG, PRNG). It uses a hardware IP block to accelerate operations. The driver
* supports these standards: DES, TDES, AES (128, 192, 256 bits), CMAC-AES, SHA,
* HMAC, PRNG, TRNG, CRC, and RSA.
*
* This document contains the following topics:
*   - \ref group_crypto_overview
*   - \ref group_crypto_configuration_considerations
*   - \ref group_crypto_server_init
*   - \ref group_crypto_client_init
*   - \ref group_crypto_irq_implements
*   - \ref group_crypto_rsa_considerations
*   - \ref group_crypto_definitions
*   - \ref group_crypto_more_information
*
* \section group_crypto_overview Overview
* The Crypto driver uses a client-server architecture.
*
* Firmware initializes and starts the Crypto server. The server can run on
* either core and works with the Crypto hardware. The Crypto server is
* implemented as a secure block. It performs all cryptographic operations for
* the client. Access to the server is through the Inter Process Communication
* (IPC) driver. Direct access is not allowed.
*
* The Crypto client can run on either core too. Firmware initializes and starts
* the client. The firmware then provides the configuration data required for the
* desired cryptographic technique, and requests that the server run the
* cryptographic operation.
*
* \image html crypto_architecture.png
*
* Firmware sets up a cryptographic operation using configuration structures, or
* by passing in required data as parameters in function calls.
*
* Most Crypto functions require one or more contexts. A context is a data
* structure that the driver uses for its operations. Firmware declares the
* context (allocates memory) but does not write or read the values in the
* context. In effect the context is a scratch pad you provide to the driver.
* The driver uses the context to store and manipulate data during cryptographic
* operations.
*
* There is a common context shared by all cryptographic methods. Several methods
* require an additional context unique to the particular cryptographic technique.
* The Crypto driver header files declare all the required structures for both
* configuration and context.
*
* IPC communication between the client and server is completely transparent.
* Default PDL source files configure the IPC channel. Using IPC for communication
* provides a simple synchronization mechanism to handle concurrent requests from
* different cores.
*
* The Crypto driver (both client and server) is provided as compiled binary
* libraries, not as source code. Header files define the API and required data
* structures.
*
* The Crypto driver uses:
*     - One IPC channel for data exchange between client and server
*     - Three interrupts: an IPC notify interrupt, an IPC release interrupt,
*     and an interrupt for error handling
*
* The \ref group_crypto_definitions section provides more information on cryptographic terms of
* art, and the particular supported encryption standards.
*
* \section group_crypto_configuration_considerations Configuration Considerations
*
* IPC communication for the Crypto driver is handled transparently. The
* cy_crytpo_config files set up the IPC channel, and configure the required
* notify, release, and error interrupts.
*
* Initialization routines, one of \ref Cy_Crypto_Server_Start_Base,
* \ref Cy_Crypto_Server_Start_Extra or \ref Cy_Crypto_Server_Start_Full (server)
* and \ref Cy_Crypto_Init (client), use separate instances of the same
* cy_stc_crypto_config_t configuration structure. Some fields should be the same,
* and some are set specifically by either the server or client. See the
* cy_crypto_config files for the default implementation. The table lists each
* field in the config structure, which initialization routine sets the value,
* and the default MACRO that defines the value.
*
* <table>
* <tr><th>Field</th><th>Which</th><th>MACRO / Function</th><th>Notes</th></tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::ipcChannel ipcChannel\endlink</td>
* <td>Server and Client</td>
* <td>CY_IPC_CHAN_CRYPTO</td>
* <td>IPC Channel, same for both</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::acquireNotifierChannel acquireNotifierChannel\endlink</td>
* <td>Server and Client</td>
* <td>CY_CRYPTO_IPC_INTR_NOTIFY_NUM</td>
* <td>Notify interrupt number, same for both</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::releaseNotifierChannel releaseNotifierChannel\endlink</td>
* <td>Server and Client</td>
* <td>CY_CRYPTO_IPC_INTR_RELEASE_NUM</td>
* <td>Release interrupt number, same for both</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::userCompleteCallback userCompleteCallback\endlink</td>
* <td>Client</td>
* <td>User-defined callback for the Release interrupt handler; can be NULL</td>
* <td>See Implementing Crypto Interrupts</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::releaseNotifierConfig releaseNotifierConfig \endlink</td>
* <td>Client</td>
* <td>Source: CY_CRYPTO_IPC_INTR_RELEASE_NUM; Priority: \ref CY_CRYPTO_RELEASE_INTR_PR</td>
* <td>configuration for the interrupt</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::userGetDataHandler userGetDataHandler\endlink</td>
* <td>Server</td>
* <td>User-defined function to override default interrupt handler; NULL = use default</td>
* <td>ISR for the Notify interrupt</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::acquireNotifierConfig acquireNotifierConfig\endlink</td>
* <td>Server</td>
* <td>Source: \ref CY_CRYPTO_CM0_NOTIFY_INTR_NR; Priority: \ref CY_CRYPTO_NOTIFY_INTR_PR</td>
* <td>configuration for the interrupt</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::userErrorHandler userErrorHandler\endlink</td>
* <td>Server</td>
* <td>User-defined function to override default interrupt handler; NULL = use default</td>
* <td>ISR for a server error</td>
* </tr>
* <tr>
* <td>\link cy_stc_crypto_config_t::cryptoErrorIntrConfig cryptoErrorIntrConfig\endlink</td>
* <td>Server</td>
* <td>Source: \ref CY_CRYPTO_CM0_ERROR_INTR_NR; Priority: \ref CY_CRYPTO_ERROR_INTR_PR</td>
* <td>configuration for the interrupt</td>
* </tr>
* </table>
*
* On the CM0+, the notify, release and error interrupts are assigned to
* NVIC IRQn <b>2, 30, 31</b> respectively. Do not modify these values.
* See System Interrupt (SysInt) for background on CM0+ interrupts.
*
* \section group_crypto_server_init Server Initialization
*
* Use Crypto Server Start function (one of \ref Cy_Crypto_Server_Start_Base,
* \ref Cy_Crypto_Server_Start_Extra or \ref Cy_Crypto_Server_Start_Full).
* Provide the configuration parameters (cy_stc_crypto_config_t) and a pointer
* to the server context (cy_stc_crypto_server_context_t).
* Do not fill in the values for the context structure.
*
* Because the two cores operate asynchronously, ensure that server
* initialization is complete before initializing the client.
* There are several ways to do this:
*
*   - Use \ref Cy_Crypto_Sync as a blocking call, before initializing the client.
*   - Enable the CM4 core (\ref Cy_SysEnableCM4) after
*     Crypto Server Start executes successfully.
*   - Check the return status from calls to \ref Cy_Crypto_Init or
*     \ref Cy_Crypto_Enable to ensure \ref CY_CRYPTO_SUCCESS.
*
* All crypto operations are asynchronous. To ensure that any crypto operation
* is complete and the result is valid, use \ref Cy_Crypto_Sync.
* Use the \ref CY_CRYPTO_SYNC_NON_BLOCKING parameter to check status.
* Use \ref CY_CRYPTO_SYNC_BLOCKING  to wait for the operation to complete.
*
* \section group_crypto_client_init Client initialization
*
* Use \ref Cy_Crypto_Init to initialize the Crypto client with the configuration
* parameters (cy_stc_crypto_config_t) and a pointer to the context
* (cy_stc_crypto_context_t). Do not fill in the values for the context structure.
*
* Then call \ref Cy_Crypto_Enable to enable the Crypto hardware IP block.
* After this, the Crypto driver is ready to execute crypto functions.
* These calls must be made on the client side.
* Firmware can implement the client on either core.
*
* Some encryption techniques require additional initialization specific to the
* technique. If there is an Init function, you must call it before using any
* other function for that technique, and reinitialize after you use a different
* encryption technique.
*
* For example, use \ref Cy_Crypto_Aes_Init to configure an AES encryption
* operation with the encryption key, and key length.
* Provide pointers to two context structures. You can then call AES Run functions.
* If later on you use DES, you must re-initialize AES encryption before using
* it again.
*
* See the documentation for details about each Cy_Crypto_XXX_Init function.
*
* \section group_crypto_irq_implements Implementing Crypto Interrupts
*
* The Crypto driver uses three interrupts:
*   - A notify interrupt when data is ready for a cryptographic operation
*   - A release interrupt when a cryptographic operation is complete
*   - An error interrupt if the server encounters a hardware error
*
* You can modify default behavior for each interrupt.
*
* <b>Notify Interrupt:</b> the Crypto server has a default ISR to handle this
* interrupt, \ref Cy_Crypto_Server_GetDataHandler. The default ISR clears the
* interrupt, retrieves the data from the IPC channel, and dispatches control to
* the desired cryptographic operation.
*
* To use the default handler, set the \link
* cy_stc_crypto_config_t::userGetDataHandler userGetDataHandler \endlink field
* of the cy_stc_crypto_config_t structure to NULL. To override, populate this
* field with your ISR. Then call Crypto Server Start function.
* Your ISR can perform additional tasks required by your application logic,
* but must also call \ref Cy_Crypto_Server_GetDataHandler to dispatch the data
* to the correct cryptographic operation.
*
* <b>Release Interrupt:</b> The Crypto driver includes a handler for this
* interrupt. The interrupt handler clears the interrupt and calls a user-provided
* callback routine. You cannot override this interrupt handler.
* By default the interrupt is disabled.
*
* To use default behavior (interrupt disabled), set the \link
* cy_stc_crypto_config_t::userCompleteCallback userCompleteCallback \endlink
* field of the cy_stc_crypto_config_t structure to NULL.
* To enable the interrupt, populate this field with your callback function.
* Then call \ref Cy_Crypto_Init. If the callback function is not NULL, the Init
* function enables the interrupt, and default behavior calls your routine.
*
* When performing cryptographic operations, firmware must ensure the operation
* is complete before acting on the results. If the release interrupt is disabled,
* typically calls to \ref Cy_Crypto_Sync should be blocking. If the interrupt is
* enabled, your callback function is called when the operation is complete.
* This lets you avoid blocking calls to \ref Cy_Crypto_Sync.
*
* <b>Error Interrupt:</b> The Crypto server has a default ISR to handle this
* interrupt. It clears the interrupt and sets an internal flag that an error
* has occurred.
*
* To use the default handler, set the userErrorHandler field of the
* cy_stc_crypto_config_t structure to NULL. To override, populate this field
* with your ISR. Then call Crypto Server Start function.
*
* Your ISR must call \ref Cy_Crypto_Server_ErrorHandler, and can perform any
* additional tasks required by your application logic.
*
* \section group_crypto_rsa_considerations RSA Usage Considerations
*
* General RSA encryption and decryption is supported.
* \ref Cy_Crypto_Rsa_Proc encrypts or decrypts data based on the parameters
* passed to the function. If you pass in plain text and a public key, the output
* is encrypted (cipher text). If you pass in cipher text and a private key, the
* output is decrypted (plain text).
*
* One parameter for this function call is a structure that defines the key:
* cy_stc_crypto_rsa_pub_key_t. The four modulus and exponent fields are
* mandatory, and represent the data for either the public or private key as
* appropriate.
*
* \note The <b>modulus</b> and <b>exponent</b> values in the
* \ref cy_stc_crypto_rsa_pub_key_t must be in little-endian order.<br>
* Use the \ref Cy_Crypto_Rsa_InvertEndianness function to convert to or from
* little-endian order.
*
* The remaining fields represent three pre-calculated coefficients that can
* reduce execution time by up to 5x. The fields are: coefficient for Barrett
* reduction, binary inverse of the modulus, and the result of
* (2^moduloLength mod modulo). These fields are optional, and can be set to NULL.
*
* Calculate these coefficients with \ref Cy_Crypto_Rsa_CalcCoefs.
* Pass in the address of the key structure with the modulus and exponent values
* for the key. The function returns the coefficients for the key in the key
* structure, replacing any previous values.
*
* The RSA functionality also implements functions to decrypt a signature using
* a public key. This signature must follow the RSASSA-PKCS-v1_5 standard.
* The signature must contain a SHA digest (hash).
* MD2, MD4, and MD5 message digests are not supported.
*
* An encrypted signature is stored as big-endian data. It must be inverted for
* RSA processing. To use the provided signature decryption, firmware must
*   -# Calculate the SHA digest of the data to be verified with
*      \ref Cy_Crypto_Sha_Run.
*   -# Ensure that the RSA signature is in little-endian format.
*      Use \ref Cy_Crypto_Rsa_InvertEndianness.
*   -# Decrypt the RSA signature with a public key, by calling
*      \ref Cy_Crypto_Rsa_Proc.
*   -# Invert the byte order of the output, to return to big-endian format.
*      Use \ref Cy_Crypto_Rsa_InvertEndianness.
*   -# Call \ref Cy_Crypto_Rsa_Verify (which requires data in big-endian format).
*
* \section group_crypto_definitions Definitions
*
* <table class="doxtable">
*   <tr>
*     <th>Term</th>
*     <th>Definition</th>
*   </tr>
*
*   <tr>
*     <td>Plaintext</td>
*     <td>An unencrypted message</td>
*   </tr>
*
*   <tr>
*     <td>Ciphertext</td>
*     <td>An encrypted message</td>
*   </tr>
*
*   <tr>
*     <td>Block Cipher</td>
*     <td>An encryption function for fixed-size blocks of data.
*     This function takes a fixed-size key and a block of plaintext data from
*     the message and encrypts it to generate ciphertext. Block ciphers are
*     reversible. The function performed on a block of encrypted data will
*     decrypt it.</td>
*   </tr>
*
*   <tr>
*     <td>Block Cipher Mode</td>
*     <td>A mode of encrypting a message using block ciphers for messages of
*     arbitrary length. The message is padded so that its length is an integer
*     multiple of the block size. ECB (Electronic Code Book), CBC (Cipher Block
*     Chaining), and CFB (Cipher Feedback) are all modes of using block ciphers
*     to create an encrypted message of arbitrary length.</td>
*   </tr>
*
*   <tr>
*     <td>Data Encryption Standard (DES)</td>
*     <td>Is a symmetric-key algorithm for the encryption of electronic data.
*     It uses a 56-bit key and a 64-bit message block size.</td>
*   </tr>
*
*   <tr>
*     <td>Triple DES (3DES or TDES)</td>
*     <td>Is a symmetric-key block cipher, which applies the Data Encryption
*     Standard (DES) cipher algorithm three times to each data block.
*     It uses three 56-bit keys. The block size is 64-bits.</td>
*   </tr>
*
*   <tr>
*     <td>Advanced Encryption Standard (AES)</td>
*     <td>The standard specifies the Rijndael algorithm, a symmetric block
*     cipher that can process data blocks of 128 bits, using cipher keys with
*     lengths of 128, 192, and 256 bits. Rijndael was designed to handle
*     additional block sizes and key lengths, however they are not adopted in
*     this standard. AES is also used for message authentication.</td>
*   </tr>
*
*   <tr>
*     <td>Cipher-based Message Authentication Code (CMAC)</td>
*     <td>This is a block cipher-based message authentication code algorithm.
*     It computes the MAC value using the AES block cipher algorithm.</td>
*   </tr>
*
*   <tr>
*     <td>Secure Hash Algorithm (SHA)</td>
*     <td>Is a cryptographic hash function.
*     This function takes a message of the arbitrary length and reduces it to a
*     fixed-length residue or message digest after performing a series of
*     mathematically defined operations that practically guarantee that any
*     change in the message will change the hash value. It is used for message
*     authentication by transmitting a message with a hash value appended to it
*     and recalculating the message hash value using the same algorithm at the
*     recipient's end. If the hashes differ, then the message is corrupted.</td>
*   </tr>
*
*   <tr>
*     <td>Message Authentication Code (MAC)</td>
*     <td>MACs are used to verify that a received message has not been altered.
*     This is done by first computing a MAC value at the sender's end and
*     appending it to the transmitted message. When the message is received,
*     the MAC is computed again and checked against the MAC value transmitted
*     with the message. If they do not match, the message has been altered.
*     Either a Hash algorithm (such as SHA) or a block cipher (such as AES) can
*     be used to produce the MAC value. Keyed MAC schemes use a Secret Key
*     along with the message, thus the Key value must be known to be able to
*     compute the MAC value.</td>
*   </tr>
*
*   <tr>
*     <td>Hash Message Authentication Code (HMAC)</td>
*     <td>Is a specific type of message authentication code (MAC) involving a
*     cryptographic hash function and a secret cryptographic key.
*     It computes the MAC value using a Hash algorithm.</td>
*   </tr>
*
*   <tr>
*     <td>Pseudo Random Number Generator (PRNG)</td>
*     <td>Is a Linear Feedback Shift Registers-based algorithm for generating a
*     sequence of numbers starting from a non-zero seed.</td>
*   </tr>
*
*   <tr>
*     <td>True Random Number Generator (TRNG)</td>
*     <td>A block that generates a number that is statistically random and based
*     on some physical random variation. The number cannot be duplicated by
*     running the process again.</td>
*   </tr>
*
*   <tr>
*     <td>Symmetric Key Cryptography</td>
*     <td>Uses a common, known key to encrypt and decrypt messages (a shared
*     secret between sender and receiver). An efficient method used for
*     encrypting and decrypting messages after the authenticity of the other
*     party has been established. DES (now obsolete), 3DES, and AES (currently
*     used) are well-known symmetric cryptography methods.</td>
*   </tr>
*
*   <tr>
*     <td>Asymmetric Key Cryptography</td>
*     <td>Also referred to as Public Key encryption. Someone who wishes to
*     receive a message, publishes a very large public key (up to 4096 bits
*     currently), which is one of two prime factors of a very large number. The
*     other prime factor is the private key of the recipient and a secret.
*     Someone wishing to send a message to the publisher of the public key
*     encrypts the message with the public key. This message can now be
*     decrypted only with the private key (the other prime factor held secret by
*     the recipient). The message is now sent over any channel to the recipient
*     who can decrypt it with the private, secret, key. The same process is used
*     to send messages to the sender of the original message. The asymmetric
*     cryptography relies on the mathematical impracticality (usually related to
*     the processing power available at any given time) of factoring the keys.
*     Common, computationally intensive, asymmetric algorithms are RSA and ECC.
*     The public key is described by the pair (n, e) where n is a product of two
*     randomly chosen primes p and q. The exponent e is a random integer
*     1 < e < Q where Q = (p-1) (q-1). The private key d is uniquely defined
*     by the integer 1 < d < Q such that ed congruent to 1 (mod Q ).</td>
*   </tr>
* </table>
*
* \section group_crypto_more_information More Information
*
* RSASSA-PKCS1-v1_5 described here, page 31:
* http://www.emc.com/collateral/white-papers/h11300-pkcs-1v2-2-rsa-cryptography-standard-wp.pdf*
*
* See the Cryptographic Function Block chapter of the Technical Reference Manual.
*
* \section group_crypto_MISRA MISRA-C Compliance
* This driver does not contains any driver-specific MISRA violations.
*
* \section group_crypto_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>2.10</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td rowspan="2">2.0</td>
*     <td>Clarified what parameters must be 4-byte aligned for the functions:
*         \ref Cy_Crypto_Aes_Cmac_Run, \ref Cy_Crypto_Sha_Run,
*         \ref Cy_Crypto_Hmac_Run, \ref Cy_Crypto_Str_MemCmp,
*         \ref Cy_Crypto_Trng_Generate, \ref Cy_Crypto_Des_Run,
*         \ref Cy_Crypto_Tdes_Run, \ref Cy_Crypto_Rsa_Proc
*     </td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td>
*         Changed crypto IP power control<br>
*         Enhanced Vector Unit functionality for RSA crypto algorithm<br>
*         Added support of the single-core devices
*     </td>
*     <td>New device support</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
*
* \defgroup group_crypto_common Configuration
* \{
*   \defgroup group_crypto_config_macros Macros
*   \defgroup group_crypto_config_structure Data Structures
* \}
*
* \defgroup group_crypto_client Crypto Client
* \{
*   \defgroup group_crypto_macros Macros
*   \defgroup group_crypto_cli_functions Functions
*   \defgroup group_crypto_cli_data_structures Data Structures
*   \defgroup group_crypto_enums Enumerated Types
* \}
*
* \defgroup group_crypto_server Crypto Server
* \{
*   \defgroup group_crypto_srv_functions Functions
*   \defgroup group_crypto_srv_data_structures Data Structures
* \}
*/

#if !defined(CY_CRYPTO_H)
#define CY_CRYPTO_H


#include <stddef.h>
#include <stdbool.h>
#include "cy_crypto_common.h"

#if (CPUSS_CRYPTO_PRESENT == 1)


#if defined(__cplusplus)
extern "C" {
#endif


/** \cond INTERNAL */

cy_en_crypto_status_t Cy_Crypto_GetLibraryInfo(cy_en_crypto_lib_info_t *cryptoInfo);

/** \endcond */

/**
* \addtogroup group_crypto_cli_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_Crypto_Init
****************************************************************************//**
*
* This function initializes the Crypto context buffer and
* configures the Crypto driver. Must be called at first.
*
* To start working with Crypto methods after Crypto_Init(),
* call Crypto_Enable() to turn-on the Crypto Hardware.
*
* \param config
* The pointer to the Crypto configuration structure.
* Could be used with default values from cy_crypto_config.h
*
* \param context
* The pointer to the \ref cy_stc_crypto_context_t instance of structure
* that stores the Crypto driver common context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Init(cy_stc_crypto_config_t const *config, cy_stc_crypto_context_t *context);

/*******************************************************************************
* Function Name: Cy_Crypto_DeInit
****************************************************************************//**
*
* This function de-initializes the Crypto driver.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_DeInit(void);

/*******************************************************************************
* Function Name: Cy_Crypto_Enable
****************************************************************************//**
*
* This function enables (turns on) the Crypto hardware.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Enable(void);

/*******************************************************************************
* Function Name: Cy_Crypto_Disable
****************************************************************************//**
*
* This function disables (turns off) the Crypto hardware.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Disable(void);

/*******************************************************************************
* Function Name: Cy_Crypto_Sync
****************************************************************************//**
*
* This function waits or just checks (depending on the parameter)
* for the Crypto operation to complete.
*
* \param isBlocking
* Set whether Crypto_Sync is blocking:
* True - is blocking.
* False - is not blocking.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Sync(bool isBlocking);

/*******************************************************************************
* Function Name: Cy_Crypto_GetErrorStatus
****************************************************************************//**
*
* This function returns a cause of a Crypto hardware error.
* It is independent of the Crypto previous state.
*
* \param hwErrorCause
* \ref cy_stc_crypto_hw_error_t.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_GetErrorStatus(cy_stc_crypto_hw_error_t *hwErrorCause);

#if (CPUSS_CRYPTO_PR == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Prng_Init
****************************************************************************//**
*
* This function initializes parameters of the PRNG.
*
* Call to initialize this encryption technique before using any associated
* functions. You must initialize this technique again after using any other
* encryption technique.
* Invoking this function resets the pseudo random sequence.
*
* \param lfsr32InitState
* A non-zero seed value for the first LFSR.

* \param lfsr31InitState
* A non-zero seed value for the second LFSR.

* \param lfsr29InitState
* A non-zero seed value for the third LFSR.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_prng_t structure that stores
* the Crypto function context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Prng_Init(uint32_t lfsr32InitState,
                                          uint32_t lfsr31InitState,
                                          uint32_t lfsr29InitState,
                                          cy_stc_crypto_context_prng_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Prng_Generate
****************************************************************************//**
*
* This function generates 32-bit the Pseudo Random Number.
* It depends on Cy_Crypto_Prng_Init that should be called before.
*
* \param max
* The maximum value of a random number.
*
* \param randomNum
* The pointer to a variable to store the generated pseudo random number.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_prng_t structure that stores
* the Crypto function context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Prng_Generate(uint32_t max,
                                              uint32_t *randomNum,
                                              cy_stc_crypto_context_prng_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_PR == 1) */

#if (CPUSS_CRYPTO_AES == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Aes_Init
****************************************************************************//**
*
* This function initializes the AES operation by setting key and key length.
*
* Call to initialize this encryption technique before using any associated
* functions. You must initialize this technique again after using any other
* encryption technique.
*
* \param key
* The pointer to the encryption/decryption key.
*
* \param keyLength
* \ref cy_en_crypto_aes_key_length_t
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_aes_t structure that stores all internal variables
* the Crypto driver requires.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Aes_Init(uint32_t *key,
                                         cy_en_crypto_aes_key_length_t keyLength,
                                         cy_stc_crypto_context_aes_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Aes_Ecb_Run
****************************************************************************//**
*
* This function performs AES operation on one block.
* The key must be set before by invoking Cy_Crypto_Aes_Init().
*
* \param dirMode
* Can be CRYPTO_ENCRYPT or CRYPTO_DECRYPT (\ref cy_en_crypto_dir_mode_t).
*
* \param srcBlock
* The pointer to a source block.
*
* \param dstBlock
* The pointer to a destination cipher block.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_aes_t instance of structure
* that stores all AES internal variables.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Aes_Ecb_Run(cy_en_crypto_dir_mode_t dirMode,
                                            uint32_t *dstBlock,
                                            uint32_t *srcBlock,
                                            cy_stc_crypto_context_aes_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Aes_Cbc_Run
****************************************************************************//**
*
* This function performs AES operation on a plain text with Cipher Block Chaining (CBC).
* The key must be set before by invoking Cy_Crypto_Aes_Init().
*
* \param dirMode
* Can be CRYPTO_ENCRYPT or CRYPTO_DECRYPT (\ref cy_en_crypto_dir_mode_t)
*
* \param srcSize
* The size of the source plain text.
*
* \param ivPtr
* The pointer to the initial vector.
*
* \param dst
* The pointer to a destination cipher text.
*
* \param src
* The pointer to a source plain text. Must be 4-Byte aligned.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_aes_t structure that stores all internal variables
* the Crypto driver requires.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Aes_Cbc_Run(cy_en_crypto_dir_mode_t dirMode,
                                            uint32_t srcSize,
                                            uint32_t *ivPtr,
                                            uint32_t *dst,
                                            uint32_t *src,
                                            cy_stc_crypto_context_aes_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Aes_Cfb_Run
****************************************************************************//**
*
* This function performs AES operation on a plain text with Cipher Feedback mode (CFB).
* The key must be set before by invoking Cy_Crypto_Aes_Init().
*
* \param dirMode
* Can be CRYPTO_ENCRYPT or CRYPTO_DECRYPT (\ref cy_en_crypto_dir_mode_t)
*
* \param srcSize
* The size of the source plain text.
*
* \param ivPtr
* The pointer to the initial vector.
*
* \param dst
* The pointer to a destination cipher text.
*
* \param src
* The pointer to a source plain text. Must be 4-Byte aligned.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_aes_t structure that stores all internal variables
* the Crypto driver requires.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Aes_Cfb_Run(cy_en_crypto_dir_mode_t dirMode,
                                            uint32_t srcSize,
                                            uint32_t *ivPtr,
                                            uint32_t *dst,
                                            uint32_t *src,
                                            cy_stc_crypto_context_aes_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Aes_Ctr_Run
****************************************************************************//**
*
* This function performs AES operation on a plain text with Cipher Block Counter mode (CTR).
* NOTE: preparation of the unique nonceCounter for each block is
* the user's responsibility. This function is dependent on
* the key being set before invoking \ref Cy_Crypto_Aes_Init().
*
* \param dirMode
* Can be CRYPTO_ENCRYPT or CRYPTO_DECRYPT (\ref cy_en_crypto_dir_mode_t)
*
* \param srcSize
* The size of a source plain text.
*
* \param srcOffset
* The size of an offset within the current block stream for resuming within the current cipher stream.
*
* \param nonceCounter
* The 128-bit nonce and counter.
*
* \param streamBlock
* The saved stream-block for resuming. Is over-written by the function.
*
* \param dst
* The pointer to a destination cipher text.
*
* \param src
* The pointer to a source plain text. Must be 4-Byte aligned.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_aes_t structure that stores all internal variables
* the Crypto driver requires.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Aes_Ctr_Run(cy_en_crypto_dir_mode_t dirMode,
                                            uint32_t srcSize,
                                            uint32_t *srcOffset,
                                            uint32_t nonceCounter[CY_CRYPTO_AES_BLOCK_SIZE / 8u],
                                            uint32_t streamBlock[CY_CRYPTO_AES_BLOCK_SIZE / 8u],
                                            uint32_t *dst,
                                            uint32_t *src,
                                            cy_stc_crypto_context_aes_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Aes_Cmac_Run
****************************************************************************//**
*
* This function performs the cipher-block chaining-message authentication-code.
*
* There is no Init function. Provide the required parameters and the pointer to
* the context structure when making this function call.
*
* \param src
* The pointer to a source plain text. Must be 4-byte aligned.
*
* \param srcSize
* The size of a source plain text.
*
* \param key
* The pointer to the encryption key. Must be 4-byte aligned.
*
* \param keyLength
* \ref cy_en_crypto_aes_key_length_t
*
* \param cmacPtr
* The pointer to the calculated CMAC. Must be 4-byte aligned.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_aes_t structure that stores all
* internal variables the Crypto driver requires.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Aes_Cmac_Run(uint32_t *src,
                                             uint32_t srcSize,
                                             uint32_t *key,
                                             cy_en_crypto_aes_key_length_t keyLength,
                                             uint32_t *cmacPtr,
                                             cy_stc_crypto_context_aes_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_AES == 1) */

#if (CPUSS_CRYPTO_SHA == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Sha_Run
****************************************************************************//**
*
* This function performs the SHA Hash function.
* There is no Init function. Provide the required parameters and the pointer
* to the context structure when making this function call.
* It is independent of the previous Crypto state because it already contains
* preparation, calculation, and finalization steps.
*
* \param mode
* \ref cy_en_crypto_sha_mode_t
*
* \param message
* The pointer to a message whose hash value is being computed.
* Must be 4-byte aligned.
*
* \param messageSize
* The size of a message.
*
* \param digest
* The pointer to the hash digest. Must be 4-byte aligned.
*
* \param cfContext
* the pointer to the \ref cy_stc_crypto_context_sha_t structure that stores all
* internal variables for Crypto driver.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Sha_Run(uint32_t *message,
                                        uint32_t  messageSize,
                                        uint32_t *digest,
                                        cy_en_crypto_sha_mode_t mode,
                                        cy_stc_crypto_context_sha_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

#if (CPUSS_CRYPTO_SHA == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Hmac_Run
****************************************************************************//**
*
* This function performs HMAC calculation.
* There is no Init function. Provide the required parameters and the pointer
* to the context structure when making this function call.
* It is independent of the previous Crypto state because it already contains
* preparation, calculation, and finalization steps.
*
* \param hmac
* The pointer to the calculated HMAC. Must be 4-byte aligned.
*
* \param message
* The pointer to a message whose hash value is being computed.
* Must be 4-byte aligned.
*
* \param messageSize
* The size of a message.
*
* \param key
* The pointer to the key. Must be 4-byte aligned.
*
* \param keyLength
* The length of the key.
*
* \param mode
* \ref cy_en_crypto_sha_mode_t
*
* \param cfContext
* the pointer to the \ref cy_stc_crypto_context_sha_t structure that stores all internal variables
* for the Crypto driver.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Hmac_Run(uint32_t *hmac,
                                         uint32_t *message,
                                         uint32_t  messageSize,
                                         uint32_t *key,
                                         uint32_t keyLength,
                                         cy_en_crypto_sha_mode_t mode,
                                         cy_stc_crypto_context_sha_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

#if (CPUSS_CRYPTO_STR == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Str_MemCpy
****************************************************************************//**
*
* This function copies a memory block. It operates on data in the user SRAM and doesn't
* use Crypto internal SRAM.
*
* \note Memory blocks should not overlap.
*
* There is no alignment restriction.
* This function is independent of the previous Crypto state.
*
* \param dst
* The pointer to the destination of MemCpy.
*
* \param src
* The pointer to the source of MemCpy.
*
* \param size
* The size in bytes of the copy operation. Maximum size is 65535 Bytes.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_str_t structure that stores all internal variables
* for the Crypto driver.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Str_MemCpy(void *dst,
                                           void const *src,
                                           uint16_t size,
                                           cy_stc_crypto_context_str_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Str_MemSet
****************************************************************************//**
*
* This function sets the memory block. It operates on data in the user SRAM and
* doesn't use Crypto internal SRAM.
*
* There is no alignment restriction.
* This function is independent from the previous Crypto state.
*
* \param dst
* The pointer to the destination of MemSet.
*
* \param data
* The value to be set.
*
* \param size
* The size in bytes of the set operation. Maximum size is 65535 Bytes.
*
* \param cfContext
* the pointer to the \ref cy_stc_crypto_context_str_t structure that stores all internal variables
* for the Crypto driver.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Str_MemSet(void *dst,
                                           uint8_t data,
                                           uint16_t size,
                                           cy_stc_crypto_context_str_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Str_MemCmp
****************************************************************************//**
*
* This function compares memory blocks. It operates on data in the user SRAM and
* doesn't use Crypto internal SRAM.
*
* There is no alignment restriction.
* This function is independent from the previous Crypto state.
*
* \param src0
* The pointer to the first source of MemCmp.
*
* \param src1
* The pointer to the second source of MemCmp.
*
* \param size
* The size in bytes of the compare operation. Maximum size is 65535 Bytes.
*
* \param resultPtr
* The pointer to the result of compare (must be 4-byte aligned):
*     - 0 - if Source 1 equal Source 2
*     - 1 - if Source 1 not equal Source 2
*
* \param cfContext
* the pointer to the \ref cy_stc_crypto_context_str_t structure that stores all internal variables
* for the Crypto driver.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Str_MemCmp(void const *src0,
                                           void const *src1,
                                           uint16_t size,
                                           uint32_t *resultPtr,
                                           cy_stc_crypto_context_str_t *cfContext);

/*******************************************************************************
* Function Name: Crypto_Str_MemXor
****************************************************************************//**
*
* This function calculates the XOR of two memory blocks. It operates on data in the user
* SRAM and doesn't use Crypto internal SRAM.
*
* \note Memory structures should not overlap.
*
* There is no alignment restriction.
* This function is independent from the previous Crypto state.
*
* \param src0
* The pointer to the first source of MemXor.

* \param src1
* The pointer to the second source of MemXor.

* \param dst
* The pointer to the destination of MemXor.
*
* \param size
* The size in bytes of the compare operation. Maximum size is 65535 Bytes.
*
* \param cfContext
* the pointer to the \ref cy_stc_crypto_context_str_t structure that stores all internal variables
* for the Crypto driver.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Str_MemXor(void const *src0,
                                           void const *src1,
                                           void *dst,
                                           uint16_t size,
                                           cy_stc_crypto_context_str_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_STR == 1) */

#if (CPUSS_CRYPTO_CRC == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Crc_Init
****************************************************************************//**
*
* This function performs CRC initialization.
*
* Call to initialize this encryption technique before using any associated
* functions. You must initialize this technique again after using any other
* encryption technique.
*
* One peculiar of the CRC hardware block is that for some polynomials
* calculated, CRC is MSB aligned and others are LSB aligned.
* Below is the table with known polynomials and their
* calculated CRCs from the string "123456789".
*
*  <table>
*  <caption id="crc_modes_table">CRC modes and parameters</caption>
*   <tr><th>Name</th><th>Width</th><th>Poly</th><th>Init</th><th>Data Rev</th><th>Data XOR</th><th>Rem Rev</th><th>Rem XOR</th><th>Expected CRC</th><th>Output of the CRC block</th>
*   </tr>
*   <tr>
*     <td>CRC-3 / ROHC</td><td>3</td><td>0x3</td><td>0x7</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x6</td><td>0x00000006</td>
*   </tr>
*   <tr>
*     <td>CRC-4 / ITU</td><td>4</td><td>0x3</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x7</td><td>0x00000007</td>
*   </tr>
*   <tr>
*     <td>CRC-5 / EPC</td><td>5</td><td>0x9</td><td>0x9</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x0</td><td>0x00000000</td>
*   </tr>
*   <tr>
*     <td>CRC-5 / ITU</td><td>5</td><td>0x15</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x7</td><td>0x00000007</td>
*   </tr>
*   <tr>
*     <td>CRC-5 / USB</td><td>5</td><td>0x5</td><td>0x1F</td><td>1</td><td>0</td><td>1</td><td>0x1F</td><td>0x19</td><td>0x00000019</td>
*   </tr>
*   <tr>
*     <td>CRC-6 / CDMA2000-A</td><td>6</td><td>0x27</td><td>0x3F</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0xD</td><td>0xD0000000</td>
*   </tr>
*   <tr>
*     <td>CRC-6 / CDMA2000-B</td><td>6</td><td>0x7</td><td>0x3F</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x3B</td><td>0x3B000000</td>
*   </tr>
*   <tr>
*     <td>CRC-6 / DARC</td><td>6</td><td>0x19</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x26</td><td>0x00000026</td>
*   </tr>
*   <tr>
*     <td>CRC-6 / ITU</td><td>6</td><td>0x3</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x6</td><td>0x00000006</td>
*   </tr>
*   <tr>
*     <td>CRC-7</td><td>7</td><td>0x9</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x75</td><td>0x75000000</td>
*   </tr>
*   <tr>
*     <td>CRC-7 / ROHC</td><td>7</td><td>0x4F</td><td>0x7F</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x53</td><td>0x00000053</td>
*   </tr>
*   <tr>
*     <td>CRC-8</td><td>8</td><td>0x7</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0xF4</td><td>0xF4000000</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / CDMA2000</td><td>8</td><td>0x9B</td><td>0xFF</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0xDA</td><td>0xDA000000</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / DARC</td><td>8</td><td>0x39</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x15</td><td>0x00000015</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / DVB-S2</td><td>8</td><td>0xD5</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0xBC</td><td>0xBC000000</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / EBU</td><td>8</td><td>0x1D</td><td>0xFF</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x97</td><td>0x00000097</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / I-CODE</td><td>8</td><td>0x1D</td><td>0xFD</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x7E</td><td>0x7E000000</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / ITU</td><td>8</td><td>0x7</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x55</td><td>0xA1</td><td>0xA1000000</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / MAXIM</td><td>8</td><td>0x31</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0xA1</td><td>0x000000A1</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / ROHC</td><td>8</td><td>0x7</td><td>0xFF</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0xD0</td><td>0x000000D0</td>
*   </tr>
*   <tr>
*     <td>CRC-8 / WCDMA</td><td>8</td><td>0x9B</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x25</td><td>0x00000025</td>
*   </tr>
*   <tr>
*     <td>CRC-10</td><td>10</td><td>0x233</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x199</td><td>0x19900000</td>
*   </tr>
*   <tr>
*     <td>CRC-10 / CDMA2000</td><td>10</td><td>0x3D9</td><td>0x3FF</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x233</td><td>0x23300000</td>
*   </tr>
*   <tr>
*     <td>CRC-11</td><td>11</td><td>0x385</td><td>0x1A</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x5A3</td><td>0x5A300000</td>
*   </tr>
*   <tr>
*     <td>CRC-12 / 3GPP</td><td>12</td><td>0x80F</td><td>0x0</td><td>0</td><td>0</td><td>1</td><td>0x0</td><td>0xDAF</td><td>0x00000DAF</td>
*   </tr>
*   <tr>
*     <td>CRC-12 / CDMA2000</td><td>12</td><td>0xF13</td><td>0xFFF</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0xD4D</td><td>0xD4D00000</td>
*   </tr>
*   <tr>
*     <td>CRC-12 / DECT</td><td>12</td><td>0x80F</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0xF5B</td><td>0xF5B00000</td>
*   </tr>
*   <tr>
*     <td>CRC-13 / BBC</td><td>13</td><td>0x1CF5</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x4FA</td><td>0x4FA00000</td>
*   </tr>
*   <tr>
*     <td>CRC-14 / DARC</td><td>14</td><td>0x805</td><td>0x0</td><td>1</td><td>0</td><td>1</td><td>0x0</td><td>0x82D</td><td>0x0000082D</td>
*   </tr>
*   <tr>
*     <td>CRC-15</td><td>15</td><td>0x4599</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x59E</td><td>0x59E00000</td>
*   </tr>
*   <tr>
*     <td>CRC-15 / MPT1327</td><td>15</td><td>0x6815</td><td>0x0</td><td>0</td><td>0</td><td>0</td><td>0x1</td><td>0x2566</td><td>0x25660000</td>
*   </tr>
*   <tr>
*     <td>CRC-24</td><td>24</td><td>0x0864CFB</td><td>0x00B704CE</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x21CF02</td><td>0x21CF0200</td>
*   </tr>
*   <tr>
*     <td>CRC-24 / FLEXRAY-A</td><td>24</td><td>0x05D6DCB</td><td>0x00FEDCBA</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x7979BD</td><td>0x7979BD00</td>
*   </tr>
*   <tr>
*     <td>CRC-24 / FLEXRAY-B</td><td>24</td><td>0x05D6DCB</td><td>0x00ABCDEF</td><td>0</td><td>0</td><td>0</td><td>0x0</td><td>0x1F23B8</td><td>0x1F23B800</td>
*   </tr>
*   <tr>
*     <td>CRC-31 / PHILIPS</td><td>31</td><td>0x4C11DB7</td><td>0x7FFFFFFF</td><td>0</td><td>0</td><td>0</td><td>0x7FFFFFFF</td><td>0xCE9E46C</td><td>0xCE9E46C0</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / ARC</td><td>16</td><td>0x8005</td><td>0x0000</td><td>1</td><td>0</td><td>1</td><td>0x0000</td><td>0xBB3D</td><td>0x0000BB3D</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / AUG-CCITT</td><td>16</td><td>0x1021</td><td>0x1D0F</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0xE5CC</td><td>0xE5CC0000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / BUYPASS</td><td>16</td><td>0x8005</td><td>0x0000</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0xFEE8</td><td>0xFEE80000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / CCITT-0</td><td>16</td><td>0x1021</td><td>0xFFFF</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0x29B1</td><td>0x29B10000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / CDMA2000</td><td>16</td><td>0xC867</td><td>0xFFFF</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0x4C06</td><td>0x4C060000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / DDS-110</td><td>16</td><td>0x8005</td><td>0x800D</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0x9ECF</td><td>0x9ECF0000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / DECT-R</td><td>16</td><td>0x0589</td><td>0x0000</td><td>0</td><td>0</td><td>0</td><td>0x0001</td><td>0x007E</td><td>0x007E0000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / DECT-X</td><td>16</td><td>0x0589</td><td>0x0000</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0x007F</td><td>0x007F0000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / DNP</td><td>16</td><td>0x3D65</td><td>0x0000</td><td>1</td><td>0</td><td>1</td><td>0xFFFF</td><td>0xEA82</td><td>0x0000EA82</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / EN-13757</td><td>16</td><td>0x3D65</td><td>0x0000</td><td>0</td><td>0</td><td>0</td><td>0xFFFF</td><td>0xC2B7</td><td>0xC2B70000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / GENIBUS</td><td>16</td><td>0x1021</td><td>0xFFFF</td><td>0</td><td>0</td><td>0</td><td>0xFFFF</td><td>0xD64E</td><td>0xD64E0000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / MAXIM</td><td>16</td><td>0x8005</td><td>0x0000</td><td>1</td><td>0</td><td>1</td><td>0xFFFF</td><td>0x44C2</td><td>0x000044C2</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / MCRF4XX</td><td>16</td><td>0x1021</td><td>0xFFFF</td><td>1</td><td>0</td><td>1</td><td>0x0000</td><td>0x6F91</td><td>0x00006F91</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / RIELLO</td><td>16</td><td>0x1021</td><td>0xB2AA</td><td>1</td><td>0</td><td>1</td><td>0x0000</td><td>0x63D0</td><td>0x000063D0</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / T10-DIF</td><td>16</td><td>0x8BB7</td><td>0x0000</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0xD0DB</td><td>0xD0DB0000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / TELEDISK</td><td>16</td><td>0xA097</td><td>0x0000</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0x0FB3</td><td>0x0FB30000</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / TMS37157</td><td>16</td><td>0x1021</td><td>0x89EC</td><td>1</td><td>0</td><td>1</td><td>0x0000</td><td>0x26B1</td><td>0x000026B1</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / USB</td><td>16</td><td>0x8005</td><td>0xFFFF</td><td>1</td><td>0</td><td>1</td><td>0xFFFF</td><td>0xB4C8</td><td>0x0000B4C8</td>
*   </tr>
*   <tr>
*     <td>CRC-A</td><td>16</td><td>0x1021</td><td>0xC6C6</td><td>1</td><td>0</td><td>1</td><td>0x0000</td><td>0xBF05</td><td>0x0000BF05</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / KERMIT</td><td>16</td><td>0x1021</td><td>0x0000</td><td>1</td><td>0</td><td>1</td><td>0x0000</td><td>0x2189</td><td>0x00002189</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / MODBUS</td><td>16</td><td>0x8005</td><td>0xFFFF</td><td>1</td><td>0</td><td>1</td><td>0x0000</td><td>0x4B37</td><td>0x00004B37</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / X-25</td><td>16</td><td>0x1021</td><td>0xFFFF</td><td>1</td><td>0</td><td>1</td><td>0xFFFF</td><td>0x906E</td><td>0x0000906E</td>
*   </tr>
*   <tr>
*     <td>CRC-16 / XMODEM</td><td>16</td><td>0x1021</td><td>0x0000</td><td>0</td><td>0</td><td>0</td><td>0x0000</td><td>0x31C3</td><td>0x31C30000</td>
*   </tr>
*   <tr>
*     <td>CRC-32</td><td>32</td><td>0x04C11DB7</td><td>0xFFFFFFFF</td><td>1</td><td>0</td><td>1</td><td>0xFFFFFFFF</td><td>0xCBF43926</td><td>0xCBF43926</td>
*   </tr>
*   <tr>
*     <td>CRC-32 / BZIP2</td><td>32</td><td>0x04C11DB7</td><td>0xFFFFFFFF</td><td>0</td><td>0</td><td>0</td><td>0xFFFFFFFF</td><td>0xFC891918</td><td>0xFC891918</td>
*   </tr>
*   <tr>
*     <td>CRC-32C</td><td>32</td><td>0x1EDC6F41</td><td>0xFFFFFFFF</td><td>1</td><td>0</td><td>1</td><td>0xFFFFFFFF</td><td>0xE3069283</td><td>0xE3069283</td>
*   </tr>
*   <tr>
*     <td>CRC-32D</td><td>32</td><td>0xA833982B</td><td>0xFFFFFFFF</td><td>1</td><td>0</td><td>1</td><td>0xFFFFFFFF</td><td>0x87315576</td><td>0x87315576</td>
*   </tr>
*   <tr>
*     <td>CRC-32 / MPEG-2</td><td>32</td><td>0x04C11DB7</td><td>0xFFFFFFFF</td><td>0</td><td>0</td><td>0</td><td>0x00000000</td><td>0x0376E6E7</td><td>0x0376E6E7</td>
*   </tr>
*   <tr>
*     <td>CRC-32 / POSIX</td><td>32</td><td>0x04C11DB7</td><td>0x00000000</td><td>0</td><td>0</td><td>0</td><td>0xFFFFFFFF</td><td>0x765E7680</td><td>0x765E7680</td>
*   </tr>
*   <tr>
*     <td>CRC-32Q</td><td>32</td><td>0x814141AB</td><td>0x00000000</td><td>0</td><td>0</td><td>0</td><td>0x00000000</td><td>0x3010BF7F</td><td>0x3010BF7F</td>
*   </tr>
*   <tr>
*     <td>CRC-32 / JAMCRC</td><td>32</td><td>0x04C11DB7</td><td>0xFFFFFFFF</td><td>1</td><td>0</td><td>1</td><td>0x00000000</td><td>0x340BC6D9</td><td>0x340BC6D9</td>
*   </tr>
*   <tr>
*     <td>CRC-32 / XFER</td><td>32</td><td>0x000000AF</td><td>0x00000000</td><td>0</td><td>0</td><td>0</td><td>0x00000000</td><td>0xBD0BE338</td><td>0xBD0BE338</td>
*   </tr>
* </table>
*
*
* \param polynomial
* The polynomial (specified using 32 bits) used in the computing CRC.
*
* \param dataReverse
* The order in which data bytes are processed. 0 - MSB first; 1- LSB first.
*
* \param dataXor
* The byte mask for XORing data
*
* \param remReverse
* A reminder reverse: 0 means the remainder is not reversed. 1 means reversed.
*
* \param remXor
* Specifies a mask with which the LFSR32 register is XORed to produce a remainder.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_crc_t structure that stores
* the Crypto driver context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Crc_Init(uint32_t polynomial,
                                         uint8_t  dataReverse,
                                         uint8_t  dataXor,
                                         uint8_t  remReverse,
                                         uint32_t remXor,
                                         cy_stc_crypto_context_crc_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Crc_Run
****************************************************************************//**
*
* This function performs CRC calculation on a message.
* It depends on \ref Cy_Crypto_Crc_Init(),
* which should be called before.
*
* \param data
* The pointer to the message whose CRC is being computed.
*
* \param dataSize
* The size of a message in bytes.
*
* \param crc
* The pointer to a computed CRC value. Must be 4-byte aligned.
*
* \param lfsrInitState
* The initial state of the LFSR.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_crc_t structure that stores
* the Crypto driver context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Crc_Run(void     *data,
                                        uint16_t dataSize,
                                        uint32_t *crc,
                                        uint32_t  lfsrInitState,
                                        cy_stc_crypto_context_crc_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_CRC == 1) */

#if (CPUSS_CRYPTO_TR == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Trng_Generate
****************************************************************************//**
*
* This function generates a 32-bit True Random Number.
*
* \param GAROPol;
* The polynomial for the programmable Galois ring oscillator.
*
* \param FIROPol;
* The polynomial for the programmable Fibonacci ring oscillator.
*
* \param max
* The maximum length of a random number, in the range [0, 32] bits.
*
* \param randomNum
* The pointer to a generated true random number. Must be 4-byte aligned.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_trng_t structure that stores
* the Crypto driver context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Trng_Generate(uint32_t  GAROPol,
                                              uint32_t  FIROPol,
                                              uint32_t  max,
                                              uint32_t *randomNum,
                                              cy_stc_crypto_context_trng_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_TR == 1) */

#if (CPUSS_CRYPTO_DES == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Des_Run
****************************************************************************//**
*
* This function performs DES operation on a Single Block. All addresses must be
* 4-Byte aligned.
* Ciphertext (dstBlock) may overlap with plaintext (srcBlock)
* There is no Init function. Provide the required parameters and the pointer
* to the context structure when making this function call.
*
* \param dirMode
* Can be CRYPTO_ENCRYPT or CRYPTO_DECRYPT (\ref cy_en_crypto_dir_mode_t)
*
* \param key
* The pointer to the encryption/decryption key. Must be 4-byte aligned.
*
* \param srcBlock
* The pointer to a source block. Must be 4-byte aligned. Must be 4-byte aligned.
*
* \param dstBlock
* The pointer to a destination cipher block. Must be 4-byte aligned.
*
* \param cfContext
* The pointer to the cy_stc_crypto_context_des_t structure that stores
* the Crypto driver context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Des_Run(cy_en_crypto_dir_mode_t dirMode,
                                        uint32_t *key,
                                        uint32_t *dstBlock,
                                        uint32_t *srcBlock,
                                        cy_stc_crypto_context_des_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Tdes_Run
****************************************************************************//**
*
* This function performs the TDES operation on a single block. All addresses
* must be 4-byte aligned.
* Ciphertext (dstBlock) may overlap with plaintext (srcBlock).
* There is no Init function. Provide the required parameters and the pointer
* to the context structure when making this function call.
*
* \param dirMode
* Can be CRYPTO_ENCRYPT or CRYPTO_DECRYPT (\ref cy_en_crypto_dir_mode_t)
*
* \param key
* The pointer to the encryption/decryption key. Must be 4-byte aligned.
*
* \param srcBlock
* The pointer to a source block. Must be 4-vyte aligned. Must be 4-byte aligned.
*
* \param dstBlock
* The pointer to a destination cipher block. Must be 4-byte aligned.
*
* \param cfContext
* The pointer to the cy_stc_crypto_context_des_t structure that stores
* the Crypto driver context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Tdes_Run(cy_en_crypto_dir_mode_t dirMode,
                                         uint32_t *key,
                                         uint32_t *dstBlock,
                                         uint32_t *srcBlock,
                                         cy_stc_crypto_context_des_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_DES == 1) */

#if (CPUSS_CRYPTO_VU == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Rsa_Proc
****************************************************************************//**
*
* This function calculates (m^e mod modulo) where m is Message (Signature), e - public exponent
* using a public key in the next representation, it contains:
* modulo,
* public exponent,
* coefficient for Barrett reduction,
* binary inverse of the modulo, and
* result of (2^moduloLength mod modulo).
*
* Not all fields in a key must be given. Modulo and public exponents are mandatory;
* Barrett coefficient, inverse modulo, and r-bar are optional.
* If they don't exist, their according pointers should be NULL. These coefficients
* could be calculated by \ref Cy_Crypto_Rsa_CalcCoefs.
* Their presence accelerates performance by five times.
* Approximate performance for 1024-bit modulo is 41.6 ms; for 2048-bit modulo is 142 ms
* when using a 25 MHz clock for Crypto HW. These numbers just for reference.
* They depend on many factors (compiler, optimization level, etc.).
*
* Returns the processed value and a success value.
*
* \note <b>Incoming message</b> and <b>result processed message</b> must be in
* little-endian order.<br>
* The <b>modulus</b> and <b>exponent</b> values in the \ref cy_stc_crypto_rsa_pub_key_t
* must also be in little-endian order.<br>
* Use \ref Cy_Crypto_Rsa_InvertEndianness function to convert to or from
* little-endian order.
*
* \param pubKey
* The pointer to the \ref cy_stc_crypto_rsa_pub_key_t structure that stores
* public key.
*
* \param message
* The pointer to the message to be processed.
*
* \param messageSize
* The length of the message to be processed.
*
* \param processedMessage
* The pointer to processed message. Must be 4-byte aligned.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_rsa_t structure that stores
* the RSA context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Rsa_Proc(cy_stc_crypto_rsa_pub_key_t const *pubKey,
                                         uint32_t const *message,
                                         uint32_t messageSize,
                                         uint32_t *processedMessage,
                                         cy_stc_crypto_context_rsa_t *cfContext);

/*******************************************************************************
* Function Name: Cy_Crypto_Rsa_CalcCoefs
****************************************************************************//**
*
* This function calculates constant coefficients (which is dependent only on modulo
* and independent on message). With this pre-calculated coefficients calculations
* speed-up by five times.
*
* These coefficients are:
*                         coefficient for Barrett reduction,
*                         binary inverse of the modulo,
*                         result of (2^moduloLength mod modulo)
*
* Calculated coefficients will be placed by addresses provided in the
* pubKey structure for according coefficients.
* Function overwrites previous values.
* Approximate performance for 1024-bit modulo is 33.2 ms; for 2048-bit modulo is 113 ms
* when using a 25 MHz clock for Crypto HW. These numbers are just for reference.
* They depend on many factors (compiler, optimization level, etc.).
*
* \param pubKey
* The pointer to the \ref cy_stc_crypto_rsa_pub_key_t structure that stores a
* public key.
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_rsa_t structure that stores
* the RSA context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Rsa_CalcCoefs(cy_stc_crypto_rsa_pub_key_t const *pubKey,
                                         cy_stc_crypto_context_rsa_t *cfContext);

#if (CPUSS_CRYPTO_SHA == 1)
/*******************************************************************************
* Function Name: Cy_Crypto_Rsa_Verify
****************************************************************************//**
*
* This function does an RSA verification with checks for content, paddings, and
* signature format.
* The SHA digest of the message and decrypted message should be calculated first.
* Supports only PKCS1-v1_5 format. Inside of this format supported padding
* using only SHA. Cases with MD2 and MD5 are not supported.
*
* PKCS1-v1_5 described here, page 31:
* http://www.emc.com/collateral/white-papers/h11300-pkcs-1v2-2-rsa-cryptography-standard-wp.pdf
*
* Returns the verification result \ref cy_en_crypto_rsa_ver_result_t.
*
* \param verResult
* The pointer to the verification result \ref cy_en_crypto_rsa_ver_result_t.
*
* \param digestType
* SHA mode used for hash calculation \ref cy_en_crypto_sha_mode_t.
*
* \param digest
* The pointer to the hash of the message whose signature is to be verified.
*
* \param decryptedSignature
* The pointer to the decrypted signature to be verified.
*
* \param decryptedSignatureLength
* The length of the decrypted signature to be verified (in bytes)
*
* \param cfContext
* The pointer to the \ref cy_stc_crypto_context_rsa_ver_t structure that stores
* the RSA context.
*
* \return
* A Crypto status \ref cy_en_crypto_status_t.
*
*******************************************************************************/
cy_en_crypto_status_t Cy_Crypto_Rsa_Verify(cy_en_crypto_rsa_ver_result_t *verResult,
                                           cy_en_crypto_sha_mode_t digestType,
                                           uint32_t const *digest,
                                           uint32_t const *decryptedSignature,
                                           uint32_t decryptedSignatureLength,
                                           cy_stc_crypto_context_rsa_ver_t *cfContext);
#endif /* #if (CPUSS_CRYPTO_SHA == 1) */

#endif /* #if (CPUSS_CRYPTO_VU == 1) */

/*******************************************************************************
* Function Name: Cy_Crypto_Rsa_InvertEndianness
****************************************************************************//**
*
* This function reverts byte-array memory block, like:<br>
* inArr[0] <---> inArr[n]<br>
* inArr[1] <---> inArr[n-1]<br>
* inArr[2] <---> inArr[n-2]<br>
* ........................<br>
* inArr[n/2] <---> inArr[n/2-1]<br>
*
* Odd or even byteSize are acceptable.
*
* \param inArrPtr
* The pointer to the memory whose endianness is to be inverted.
*
* \param byteSize
* The length of the memory array whose endianness is to be inverted (in bytes)
*
*******************************************************************************/
void Cy_Crypto_Rsa_InvertEndianness(void *inArrPtr, uint32_t byteSize);


/** \} group_crypto_cli_functions */


#if defined(__cplusplus)
}
#endif

#endif /* #if (CPUSS_CRYPTO_PRESENT == 1) */

#endif /* (CY_CRYPTO_H) */

/** \} group_crypto */


/* [] END OF FILE */
