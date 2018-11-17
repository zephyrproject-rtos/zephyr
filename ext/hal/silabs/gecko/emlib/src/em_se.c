/***************************************************************************//**
 * @file em_se.c
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
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
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
#include "em_device.h"

#if defined(SEMAILBOX_PRESENT)

#include "em_se.h"
#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup SE
 * @{
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Add input data to a command
 *
 * @details
 *   This function adds a buffer of input data to the given SE command structure
 *   The buffer gets appended by reference at the end of the list of already
 *   added buffers.
 *
 * @note
 *   Note that this function does not copy either the data buffer or the buffer
 *   structure, so make sure to keep the data object in scope until the command
 *   has been executed by the secure element.
 *
 * @param[in]  command
 *   Pointer to an SE command structure.
 *
 * @param[in]  data
 *   Pointer to a data transfer structure.
 ******************************************************************************/
void SE_addDataInput(SE_Command_t *command, SE_DataTransfer_t *data)
{
  if (command->data_in == NULL) {
    command->data_in = data;
  } else {
    SE_DataTransfer_t *next = command->data_in;
    while (next->next != (void*)SE_DATATRANSFER_STOP) {
      next = (SE_DataTransfer_t*)next->next;
    }
    next->next = data;
  }
}

/***************************************************************************//**
 * @brief
 *   Add output data to a command
 *
 * @details
 *   This function adds a buffer of output data to the given SE command structure
 *   The buffer gets appended by reference at the end of the list of already
 *   added buffers.
 *
 * @note
 *   Note that this function does not copy either the data buffer or the buffer
 *   structure, so make sure to keep the data object in scope until the command
 *   has been executed by the secure element.
 *
 * @param[in]  command
 *   Pointer to an SE command structure.
 *
 * @param[in]  data
 *   Pointer to a data transfer structure.
 ******************************************************************************/
void SE_addDataOutput(SE_Command_t *command,
                      SE_DataTransfer_t *data)
{
  if (command->data_out == NULL) {
    command->data_out = data;
  } else {
    SE_DataTransfer_t *next = command->data_out;
    while (next->next != (void*)SE_DATATRANSFER_STOP) {
      next = (SE_DataTransfer_t*)next->next;
    }
    next->next = data;
  }
}

/***************************************************************************//**
 * @brief
 *   Add a parameter to a command
 *
 * @details
 *   This function adds a parameter word to the passed command.
 *
 * @note
 *   Make sure to not exceed @ref SE_MAX_PARAMETERS.
 *
 * @param[in]  command
 *   Pointer to a filled-out SE command structure.
 * @param[in]  parameter
 *   Parameter to add.
 ******************************************************************************/
void SE_addParameter(SE_Command_t *command, uint32_t parameter)
{
  if (command->num_parameters >= SE_MAX_PARAMETERS) {
    EFM_ASSERT(command->num_parameters < SE_MAX_PARAMETERS);
    return;
  }

  command->parameters[command->num_parameters] = parameter;
  command->num_parameters += 1;
}

/***************************************************************************//**
 * @brief
 *   Execute the passed command
 *
 * @details
 *   This function starts the execution of the passed command by the secure
 *   element. When started, wait for the RXINT interrupt flag, or call
 *   @ref SE_waitCommandCompletion to busy-wait. After completion, you have to
 *   call @ref SE_readCommandResponse to get the command's execution status.
 *
 * @param[in]  command
 *   Pointer to a filled-out SE command structure.
 ******************************************************************************/
void SE_executeCommand(SE_Command_t *command)
{
  // Don't overflow our struct
  if (command->num_parameters > SE_MAX_PARAMETERS) {
    EFM_ASSERT(command->num_parameters <= SE_MAX_PARAMETERS);
    return;
  }

  // Wait for room available in the mailbox
  while (!(SEMAILBOX_HOST->TX_STATUS & SEMAILBOX_TX_STATUS_TXINT)) ;

  // Write header to start transaction
  SEMAILBOX_HOST->TX_HEADER = sizeof(uint32_t) * (4 + command->num_parameters);

  // Write command into FIFO
  SEMAILBOX_HOST->FIFO[0].DATA = command->command;

  // Write DMA descriptors into FIFO
  SEMAILBOX_HOST->FIFO[0].DATA = (uint32_t)command->data_in;
  SEMAILBOX_HOST->FIFO[0].DATA = (uint32_t)command->data_out;

  // Write applicable parameters into FIFO
  for (size_t i = 0; i < command->num_parameters; i++) {
    SEMAILBOX_HOST->FIFO[0].DATA = command->parameters[i];
  }

  return;
}

/***************************************************************************//**
 * @brief
 *   Writes data to User Data section in MTP. Write data must be aligned to words
 *    and contain a number of bytes that is divisable by four.
 * @note
 *   It is recommended to erase the flash page before performing a write.
 *
 * @param[in] offset
 *   Offset to the flash word to write to. Must be aligned to words.
 * @param[in] data
 *   Data to write to flash.
 * @param[in] numBytes
 *   Number of bytes to write to flash. NB: Must be divisable by four.
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
SE_Response_t SE_writeUserData(uint32_t offset,
                               void *data,
                               uint32_t numBytes)
{
  /* SE command structures */
  SE_Command_t command = SE_COMMAND_DEFAULT(SE_COMMAND_WRITE_USER_DATA);
  SE_DataTransfer_t userData = SE_DATATRANSFER_DEFAULT(data, numBytes);

  SE_addDataInput(&command, &userData);

  SE_addParameter(&command, offset);
  SE_addParameter(&command, numBytes);

  SE_executeCommand(&command);
  SE_Response_t res = SE_readCommandResponse();
  return res;
}

/***************************************************************************//**
 * @brief
 *   Erases User Data section in MTP.
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
SE_Response_t SE_eraseUserData()
{
  /* SE command structures */
  SE_Command_t command = SE_COMMAND_DEFAULT(SE_COMMAND_ERASE_USER_DATA);

  SE_addParameter(&command, SE_COMMAND_OPTION_ERASE_UD);
  SE_executeCommand(&command);
  SE_Response_t res = SE_readCommandResponse();
  return res;
}

/** @} (end addtogroup SE) */
/** @} (end addtogroup emlib) */

#endif /* defined(SEMAILBOX_PRESENT) */
