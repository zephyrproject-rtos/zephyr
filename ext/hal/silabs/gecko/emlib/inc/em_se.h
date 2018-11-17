/***************************************************************************//**
 * @file em_se.h
 * @brief Secure Element API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/
#ifndef EM_SE_H
#define EM_SE_H

#include "em_device.h"

#if defined(SEMAILBOX_PRESENT)

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup SE
 *
 * @brief Secure Element peripheral API
 *
 * @details
 *   Abstraction of the Secure Element's mailbox interface.
 *
 * @{
 ******************************************************************************/

/*******************************************************************************
 ******************************   DEFINES    ***********************************
 ******************************************************************************/

/* Command words for the Secure Element. */
#define SE_COMMAND_CREATE_KEY           0x02000000UL

#define SE_COMMAND_HASH                 0x03000000UL
#define SE_COMMAND_HASHUPDATE           0x03010000UL
#define SE_COMMAND_HMAC                 0x03020000UL

#define SE_COMMAND_AES_ENCRYPT          0x04000000UL
#define SE_COMMAND_AES_DECRYPT          0x04010000UL
#define SE_COMMAND_AES_CMAC             0x04040000UL
#define SE_COMMAND_AES_CCM_ENCRYPT      0x04050000UL
#define SE_COMMAND_AES_CCM_DECRYPT      0x04060000UL

#define SE_COMMAND_SIGNATURE_SIGN       0x06000000UL
#define SE_COMMAND_SIGNATURE_VERIFY     0x06010000UL

#define SE_COMMAND_TRNG_GET_RANDOM      0x07000000UL

#define SE_COMMAND_DH                   0x0E000000UL

#define SE_COMMAND_WRITE_USER_DATA      0x43090000UL
#define SE_COMMAND_ERASE_USER_DATA      0x430A0000UL

/* Command options for the Secure Element commands. */
/** Use SHA1 as hash algorithm */
#define SE_COMMAND_OPTION_HASH_SHA1     0x00000200UL
/** Use SHA224 as hash algorithm */
#define SE_COMMAND_OPTION_HASH_SHA224   0x00000300UL
/** Use SHA256 as hash algorithm */
#define SE_COMMAND_OPTION_HASH_SHA256   0x00000400UL
/** Use SHA384 as hash algorithm */
#define SE_COMMAND_OPTION_HASH_SHA384   0x00000500UL
/** Use SHA512 as hash algorithm */
#define SE_COMMAND_OPTION_HASH_SHA512   0x00000600UL

/** Execute algorithm in ECB mode */
#define SE_COMMAND_OPTION_MODE_ECB      0x00000100UL
/** Execute algorithm in CBC mode */
#define SE_COMMAND_OPTION_MODE_CBC      0x00000200UL
/** Execute algorithm in CTR mode */
#define SE_COMMAND_OPTION_MODE_CTR      0x00000300UL
/** Execute algorithm in CFB mode */
#define SE_COMMAND_OPTION_MODE_CFB      0x00000400UL

/** Run the whole algorithm, all data present */
#define SE_COMMAND_OPTION_CONTEXT_WHOLE 0x00000000UL
/** Start the algorithm, but get a context to later add more data */
#define SE_COMMAND_OPTION_CONTEXT_START 0x00000001UL
/** End the algorithm, get the result */
#define SE_COMMAND_OPTION_CONTEXT_END   0x00000002UL
/** Add more data input to the algorithm. Need to supply previous context,
 *  and get a context back */
#define SE_COMMAND_OPTION_CONTEXT_ADD   0x00000003UL

/** Magic paramater for deleting user data */
#define SE_COMMAND_OPTION_ERASE_UD      0xDE1E7EADUL

/* Response status codes for the Secure Element */
#define SE_RESPONSE_MASK                0x000F0000UL
/** Command executed successfully or signature was successfully validated. */
#define SE_RESPONSE_OK                  0x00000000UL
/**
 * Command was not recognized as a valid command, or is not allowed in the
 * current context.
 */
#define SE_RESPONSE_INVALID_COMMAND     0x00010000UL
/**
 * User did not provide the required credentials to be allowed to execute the
 * command.
 */
#define SE_RESPONSE_AUTHORIZATION_ERROR 0x00020000UL
/**
 * Signature validation command (e.g. SE_COMMAND_SIGNATURE_VERIFY) failed to
 * verify the given signature as being correct.
 */
#define SE_RESPONSE_INVALID_SIGNATURE   0x00030000UL
/** A command started in non-secure mode is trying to access secure memory. */
#define SE_RESPONSE_BUS_ERROR           0x00040000UL
/** Internal test failed */
#define SE_RESPONSE_TEST_FAILED         0x00050000UL
/** An internal error was raised and the command did not execute. */
#define SE_RESPONSE_CRYPTO_ERROR        0x00060000UL
/** One of the passed parameters is deemed invalid (e.g. out of bounds). */
#define SE_RESPONSE_INVALID_PARAMETER   0x00070000UL

#define SE_DATATRANSFER_STOP            0x00000001UL
#define SE_DATATRANSFER_DISCARD         0x40000000UL
#define SE_DATATRANSFER_REALIGN         0x20000000UL
#define SE_DATATRANSFER_CONSTADDRESS    0x10000000UL
#define SE_DATATRANSFER_LENGTH_MASK     0x0FFFFFFFUL

/** Maximum amount of parameters for largest command in defined command set */
#ifndef SE_MAX_PARAMETERS
#define SE_MAX_PARAMETERS               4U
#endif

/** Maximum amount of parameters supported by the hardware FIFO */
#define SE_FIFO_MAX_PARAMETERS          13U

/* Sanity-check defines */
#if SE_MAX_PARAMETERS > SE_FIFO_MAX_PARAMETERS
#error "Trying to configure more parameters than supported by the hardware"
#endif

/*******************************************************************************
 ******************************   TYPEDEFS   ***********************************
 ******************************************************************************/

/**
 * SE DMA transfer descriptor. Can be linked to each other to provide
 * scatter-gather behavior.
 */
typedef struct {
  void* volatile data;
  void* volatile next;
  volatile uint32_t length;
} SE_DataTransfer_t;

/** Default initialization of data transfer struct */
#define SE_DATATRANSFER_DEFAULT(address, length)                               \
  {                                                                            \
    (address),                         /* Pointer to data block */             \
    (void*)SE_DATATRANSFER_STOP,       /* This is the last block by default */ \
    (length) | SE_DATATRANSFER_REALIGN /* Add size, use realign by default */  \
  }

/**
 * SE Command structure to which all commands to the SE must adhere.
 */
typedef struct {
  uint32_t command;
  SE_DataTransfer_t* data_in;
  SE_DataTransfer_t* data_out;
  uint32_t parameters[SE_MAX_PARAMETERS];
  size_t num_parameters;
} SE_Command_t;

/** Default initialization of command struct */
#define SE_COMMAND_DEFAULT(command)       \
  {                                       \
    (command),        /* Given command */ \
    NULL,             /* No data in */    \
    NULL,             /* No data out */   \
    { 0, 0, 0, 0 },   /* No parameters */ \
    0                 /* No parameters */ \
  }

/** Possible responses to a command */
typedef uint32_t SE_Response_t;

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void SE_addDataInput(SE_Command_t *command,
                     SE_DataTransfer_t *data);

void SE_addDataOutput(SE_Command_t *command,
                      SE_DataTransfer_t *data);

void SE_addParameter(SE_Command_t *command, uint32_t parameter);

void SE_executeCommand(SE_Command_t *command);

SE_Response_t SE_writeUserData(uint32_t offset,
                               void *data,
                               uint32_t numBytes);

SE_Response_t SE_eraseUserData(void);

__STATIC_INLINE bool SE_isCommandCompleted(void);

__STATIC_INLINE void SE_waitCommandCompletion(void);

__STATIC_INLINE SE_Response_t SE_readCommandResponse(void);

__STATIC_INLINE void SE_disableInterrupt(uint32_t flags);

__STATIC_INLINE void SE_enableInterrupt(uint32_t flags);

/***************************************************************************//**
 * @brief
 *   Check whether the running command has completed.
 *
 * @details
 *   This function polls the SE-to-host mailbox interrupt flag.
 *
 * @return True if a command has completed and the result is available
 ******************************************************************************/
__STATIC_INLINE bool SE_isCommandCompleted(void)
{
  return (bool)(SEMAILBOX_HOST->RX_STATUS & SEMAILBOX_RX_STATUS_RXINT);
}

/***************************************************************************//**
 * @brief
 *   Wait for completion of the current command.
 *
 * @details
 *   This function "busy"-waits until the execution of the ongoing instruction
 *   has completed.
 ******************************************************************************/
__STATIC_INLINE void SE_waitCommandCompletion(void)
{
  /* Wait for completion */
  while (!SE_isCommandCompleted()) {
  }
}

/***************************************************************************//**
 * @brief
 *   Read the status of the previously executed command.
 *
 * @details
 *   This function reads the status of the previously executed command.
 *
 * @note
 *   The command response needs to be read for every executed command, and can
 *   only be read once per executed command (FIFO behavior).
 *
 * @return
 *   One of the SE_RESPONSE return codes:
 *   SE_RESPONSE_OK when the command was executed successfully or a signature
 *   was successfully verified,
 *   SE_RESPONSE_INVALID_COMMAND when the command ID was not recognized,
 *   SE_RESPONSE_AUTHORIZATION_ERROR when the command is not authorized,
 *   SE_RESPONSE_INVALID_SIGNATURE when signature verification failed,
 *   SE_RESPONSE_BUS_ERROR when a bus error was thrown during the command, e.g.
 *   because of conflicting Secure/Non-Secure memory accesses,
 *   SE_RESPONSE_CRYPTO_ERROR on an internal SE failure, or
 *   SE_RESPONSE_INVALID_PARAMETER when an invalid parameter was passed
 ******************************************************************************/
__STATIC_INLINE SE_Response_t SE_readCommandResponse(void)
{
  SE_waitCommandCompletion();
  return (SE_Response_t)(SEMAILBOX_HOST->RX_HEADER & SE_RESPONSE_MASK);
}

/***************************************************************************//**
 * @brief
 *   Disable one or more SE interrupts.
 *
 * @param[in] flags
 *   SE interrupt sources to disable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the Secure Element module
 *    (SE_CONFIGURATION_(TX/RX)INTEN).
 ******************************************************************************/
__STATIC_INLINE void SE_disableInterrupt(uint32_t flags)
{
  SEMAILBOX_HOST->CONFIGURATION &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more SE interrupts.
 *
 * @param[in] flags
 *   SE interrupt sources to enable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the Secure Element module
 *   (SEMAILBOX_CONFIGURATION_TXINTEN or SEMAILBOX_CONFIGURATION_RXINTEN).
 ******************************************************************************/
__STATIC_INLINE void SE_enableInterrupt(uint32_t flags)
{
  SEMAILBOX_HOST->CONFIGURATION |= flags;
}

#ifdef __cplusplus
}
#endif

/** @} (end addtogroup SE) */
/** @} (end addtogroup emlib) */

#endif /* defined(SEMAILBOX_PRESENT) */

#endif /* EM_SE_H */
