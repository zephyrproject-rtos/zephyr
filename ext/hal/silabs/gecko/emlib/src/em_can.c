/***************************************************************************//**
 * @file em_can.c
 * @brief Controller Area Network API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
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

#include "em_can.h"
#include "em_common.h"
#include "em_assert.h"
#include "em_cmu.h"
#include <stddef.h>

#if defined(CAN_COUNT) && (CAN_COUNT > 0)

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/* Macros to use for the ID field in the CANn_MIRx_ARB register as an 11 bit
 * standard ID. The register field can be used for both an 11 bit standard
 * ID and a 29 bit extended ID. */
#define _CAN_MIR_ARB_STD_ID_SHIFT         18
#define _CAN_MIR_MASK_STD_SHIFT           18
#define _CAN_MIR_ARB_STD_ID_MASK          0x1FFC0000UL
#define _CAN_MIR_ARB_STD_ID_MAX           0x7FFUL // = 2^11 - 1

#if (CAN_COUNT == 2)
#define CAN_VALID(can)  ((can == CAN0) || (can == CAN1))
#elif (CAN_COUNT == 1)
#define CAN_VALID(can)  (can == CAN0)
#else
#error "The actual number of CAN busses is not supported."
#endif

/** @endcond */

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CAN
 * @brief Controller Area Network API
 *
 * @details The Controller Area Network Interface Bus (CAN) implements a
 * multi-master serial bus for connecting microcontrollers and devices, also
 * known as nodes, to communicate with each other in applications without a host
 * computer. CAN is a message-based protocol, designed originally for automotive
 * applications, but also used in many other scenarios.
 * The complexity of a node can range from a simple I/O device up to an
 * embedded computer with a CAN interface and sophisticated software. The node
 * may also be a gateway allowing a standard computer to communicate over a USB
 * or Ethernet port to the devices on a CAN network. Devices are connected to
 * the bus through a host processor, a CAN controller, and a CAN transceiver.
 *
 * @include em_can_send_example.c
 *
 * @{
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize CAN.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] init
 *   A pointer to the CAN initialization structure.
 ******************************************************************************/
void CAN_Init(CAN_TypeDef *can, const CAN_Init_TypeDef *init)
{
  EFM_ASSERT(CAN_VALID(can));

  CAN_Enable(can, false);
  can->CTRL = _CAN_CTRL_TEST_MASK;
  can->TEST = _CAN_TEST_RESETVALUE;
  if (init->resetMessages) {
    CAN_ResetMessages(can, 0);
  }
  can->CTRL = CAN_CTRL_INIT;
  CAN_SetBitTiming(can,
                   init->bitrate,
                   init->propagationTimeSegment,
                   init->phaseBufferSegment1,
                   init->phaseBufferSegment2,
                   init->synchronisationJumpWidth);
  CAN_Enable(can, init->enable);
}

/***************************************************************************//**
 * @brief
 *   Get the CAN module frequency.
 *
 * @details
 *   An internal prescaler of 2 is inside the CAN module.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   A clock value.
 ******************************************************************************/
uint32_t CAN_GetClockFrequency(CAN_TypeDef *can)
{
#if defined CAN0
  if (can == CAN0) {
    return CMU_ClockFreqGet(cmuClock_CAN0) / 2;
  }
#endif

#if defined CAN1
  if (can == CAN1) {
    return CMU_ClockFreqGet(cmuClock_CAN1) / 2;
  }
#endif
  EFM_ASSERT(false);
  return 0;
}

/***************************************************************************//**
 * @brief
 *   Read a Message Object to find if a message was lost ; reset the
 *   'Message Lost' flag.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] msgNum
 *   A message number of the Message Object, [1 - 32].
 *
 * @return
 *   True if a message was lost, false otherwise.
 ******************************************************************************/
bool CAN_MessageLost(CAN_TypeDef *can, uint8_t interface, uint8_t msgNum)
{
  CAN_MIR_TypeDef * mir = &can->MIR[interface];
  bool messageLost;

  /* Make sure msgNum is in the correct range. */
  EFM_ASSERT((msgNum > 0) && (msgNum <= 32));

  CAN_ReadyWait(can, interface);

  /* Set which registers to read from RAM. */
  mir->CMDMASK = CAN_MIR_CMDMASK_WRRD_READ
                 | CAN_MIR_CMDMASK_CONTROL
                 | CAN_MIR_CMDMASK_CLRINTPND;

  /* Send reading request and wait (3 to 6 cpu cycle). */
  CAN_SendRequest(can, interface, msgNum, true);

  messageLost = mir->CTRL & _CAN_MIR_CTRL_MESSAGEOF_MASK;

  if (messageLost) {
    mir->CMDMASK = CAN_MIR_CMDMASK_WRRD | CAN_MIR_CMDMASK_CONTROL;

    /* Reset the 'MessageLost' bit. */
    mir->CTRL &= ~_CAN_MIR_CTRL_MESSAGEOF_MASK;

    /* Send reading request and wait (3 to 6 cpu cycle). */
    CAN_SendRequest(can, interface, msgNum, true);
  }

  /* Return the state of the MESSAGEOF bit. */
  return messageLost;
}

/***************************************************************************//**
 * @brief
 *   Set the ROUTE registers.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] active
 *   A boolean indicating whether or not to activate the ROUTE registers.
 *
 * @param[in] pinRxLoc
 *   A location of the RX pin.
 *
 * @param[in] pinTxLoc
 *   A location of the TX pin.
 ******************************************************************************/
void CAN_SetRoute(CAN_TypeDef *can,
                  bool active,
                  uint16_t pinRxLoc,
                  uint16_t pinTxLoc)
{
  if (active) {
    /* Set the ROUTE register */
    can->ROUTE = CAN_ROUTE_TXPEN
                 | (pinRxLoc << _CAN_ROUTE_RXLOC_SHIFT)
                 | (pinTxLoc << _CAN_ROUTE_TXLOC_SHIFT);
  } else {
    /* Deactivate the ROUTE register */
    can->ROUTE = 0x0;
  }
}

/***************************************************************************//**
 * @brief
 *   Set the bitrate and its parameters.
 *
 * @details
 *   Multiple parameters need to be properly configured.
 *   See the reference manual for a detailed description.
 *   Careful : the BRP (Baud Rate Prescaler) is calculated by:
 *   'brp = freq / (period * bitrate);'. freq is the frequency of the CAN
 *   device, period the time of transmission of a bit. The result is an uint32_t.
 *   Hence it's truncated, causing an approximation error. This error is non
 *   negligible when the period is high, the bitrate is high, and frequency is low.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] bitrate
 *   A wanted bitrate on the CAN bus.
 *
 * @param[in] propagationTimeSegment
 *   A value for the Propagation Time Segment.
 *
 * @param[in] phaseBufferSegment1
 *   A value for the Phase Buffer Segment 1.
 *
 * @param[in] phaseBufferSegment2
 *   A value for the Phase Buffer Segment 2.
 *
 * @param[in] synchronisationJumpWidth
 *   A value for the Synchronization Jump Width.
 ******************************************************************************/
void CAN_SetBitTiming(CAN_TypeDef *can,
                      uint32_t bitrate,
                      uint16_t propagationTimeSegment,
                      uint16_t phaseBufferSegment1,
                      uint16_t phaseBufferSegment2,
                      uint16_t synchronisationJumpWidth)
{
  uint32_t sum, brp, period, freq, brpHigh, brpLow;

  /* Verification that the parameters are in range. */
  EFM_ASSERT((propagationTimeSegment <= 8) && (propagationTimeSegment > 0));
  EFM_ASSERT((phaseBufferSegment1 <= 8) && (phaseBufferSegment1 > 0));
  EFM_ASSERT((phaseBufferSegment2 <= 8) && (phaseBufferSegment2 > 0));
  EFM_ASSERT(bitrate > 0);
  EFM_ASSERT((synchronisationJumpWidth <= phaseBufferSegment1)
             && (synchronisationJumpWidth <= phaseBufferSegment2)
             && (synchronisationJumpWidth > 0));

  /* propagationTimeSegment is counted as part of phaseBufferSegment1 in the
     BITTIMING register. */
  sum = phaseBufferSegment1 + propagationTimeSegment;

  /* Period is the total length of one CAN bit. 1 is the Sync_seg. */
  period = 1 + sum + phaseBufferSegment2;
  freq = CAN_GetClockFrequency(can);

  brp = freq / (period * bitrate);
  EFM_ASSERT(brp != 0);

  /* -1 because the hardware reads 'written value + 1'. */
  brp = brp - 1;

  /* brp is divided between two registers. */
  brpHigh = brp / 64;
  brpLow = brp % 64;

  /* Checking register limit. */
  EFM_ASSERT(brpHigh <= 15);

  bool enabled = CAN_IsEnabled(can);

  /* Enable access to the bittiming registers. */
  can->CTRL |= CAN_CTRL_CCE | CAN_CTRL_INIT;

  can->BITTIMING = (brpLow << _CAN_BITTIMING_BRP_SHIFT)
                   | ((synchronisationJumpWidth - 1) << _CAN_BITTIMING_SJW_SHIFT)
                   | ((sum - 1) << _CAN_BITTIMING_TSEG1_SHIFT)
                   | ((phaseBufferSegment2 - 1) << _CAN_BITTIMING_TSEG2_SHIFT);
  can->BRPE = brpHigh;

  if (enabled) {
    can->CTRL &= ~(_CAN_CTRL_CCE_MASK | _CAN_CTRL_INIT_MASK);
  } else {
    can->CTRL &= ~_CAN_CTRL_CCE_MASK;
  }
}

/***************************************************************************//**
 * @brief
 *   Set the CAN operation mode.
 *
 * @details
 *   In initialization mode, the CAN module is deactivated. Reset the messages in all
 *   other modes to be sure that there is no leftover data that
 *   needs to be configured before use.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] mode
 *   Mode of operation : Init, Normal, Loopback, SilentLoopback, Silent, Basic.
 ******************************************************************************/
void CAN_SetMode(CAN_TypeDef *can, CAN_Mode_TypeDef mode)
{
  switch (mode) {
    case canModeNormal:
      can->CTRL |= _CAN_CTRL_TEST_MASK;
      can->TEST = _CAN_TEST_RESETVALUE;
      can->CTRL &= ~_CAN_CTRL_TEST_MASK;

      can->CTRL = _CAN_CTRL_EIE_MASK
                  | _CAN_CTRL_SIE_MASK
                  | _CAN_CTRL_IE_MASK;
      break;

    case canModeBasic:
      can->CTRL = _CAN_CTRL_EIE_MASK
                  | _CAN_CTRL_SIE_MASK
                  | _CAN_CTRL_IE_MASK
                  | CAN_CTRL_TEST;
      can->TEST = CAN_TEST_BASIC;
      break;

    case canModeLoopBack:
      can->CTRL = _CAN_CTRL_EIE_MASK
                  | _CAN_CTRL_SIE_MASK
                  | _CAN_CTRL_IE_MASK
                  | CAN_CTRL_TEST;
      can->TEST = CAN_TEST_LBACK;
      break;

    case canModeSilentLoopBack:
      can->CTRL = _CAN_CTRL_EIE_MASK
                  | _CAN_CTRL_SIE_MASK
                  | _CAN_CTRL_IE_MASK
                  | CAN_CTRL_TEST;
      can->TEST = CAN_TEST_LBACK | CAN_TEST_SILENT;
      break;

    case canModeSilent:
      can->CTRL = _CAN_CTRL_EIE_MASK
                  | _CAN_CTRL_SIE_MASK
                  | _CAN_CTRL_IE_MASK
                  | CAN_CTRL_TEST;
      can->TEST = CAN_TEST_SILENT;
      break;

    default:
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Set the ID and the filter for a specific Message Object.
 *
 * @details
 *   The initialization bit has to be 0 to use this function.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] useMask
 *   A boolean to choose whether or not to use the masks.
 *
 * @param[in] message
 *   A Message Object.
 *
 * @param[in] wait
 *   If true, wait for the end of the transfer between the MIRx registers and
 *   the RAM to exit. If false, exit immediately, the transfer can still be
 *   in progress.
 ******************************************************************************/
void CAN_SetIdAndFilter(CAN_TypeDef *can,
                        uint8_t interface,
                        bool useMask,
                        const CAN_MessageObject_TypeDef *message,
                        bool wait)
{
  /* Make sure msgNum is in the correct range. */
  EFM_ASSERT((message->msgNum > 0) && (message->msgNum <= 32));

  CAN_MIR_TypeDef * mir = &can->MIR[interface];
  CAN_ReadyWait(can, interface);

  /* Set which registers to read from RAM. */
  mir->CMDMASK = CAN_MIR_CMDMASK_WRRD_READ
                 | CAN_MIR_CMDMASK_ARBACC
                 | CAN_MIR_CMDMASK_CONTROL;

  /* Send reading request and wait (3 to 6 CPU cycle). */
  CAN_SendRequest(can, interface, message->msgNum, true);

  /* Reset MSGVAL. */
  mir->CMDMASK |= CAN_MIR_CMDMASK_WRRD;
  mir->ARB &= ~(0x1U << _CAN_MIR_ARB_MSGVAL_SHIFT);
  CAN_SendRequest(can, interface, message->msgNum, true);

  /* Set which registers to write to RAM. */
  mir->CMDMASK |= CAN_MIR_CMDMASK_MASKACC;

  /* Set UMASK bit. */
  BUS_RegBitWrite(&mir->CTRL, _CAN_MIR_CTRL_UMASK_SHIFT, useMask);

  /* Configure the ID. */
  if (message->extended) {
    EFM_ASSERT(message->id <= _CAN_MIR_ARB_ID_MASK);
    mir->ARB = (mir->ARB & ~_CAN_MIR_ARB_ID_MASK)
               | (message->id << _CAN_MIR_ARB_ID_SHIFT)
               | (uint32_t)(0x1U << _CAN_MIR_ARB_MSGVAL_SHIFT)
               | CAN_MIR_ARB_XTD_EXT;
  } else {
    EFM_ASSERT(message->id <= _CAN_MIR_ARB_STD_ID_MAX);
    mir->ARB = (mir->ARB & ~(_CAN_MIR_ARB_ID_MASK | CAN_MIR_ARB_XTD_STD))
               | (message->id << _CAN_MIR_ARB_STD_ID_SHIFT)
               | (uint32_t)(0x1U << _CAN_MIR_ARB_MSGVAL_SHIFT);
  }

  if (message->extendedMask) {
    mir->MASK = (message->mask << _CAN_MIR_MASK_MASK_SHIFT);
  } else {
    mir->MASK = (message->mask << _CAN_MIR_MASK_STD_SHIFT)
                & _CAN_MIR_ARB_STD_ID_MASK;
  }

  /* Configure the masks. */
  mir->MASK |= (message->extendedMask << _CAN_MIR_MASK_MXTD_SHIFT)
               | (message->directionMask << _CAN_MIR_MASK_MDIR_SHIFT);

  /* Send a writing request. */
  CAN_SendRequest(can, interface, message->msgNum, wait);
}

/***************************************************************************//**
 * @brief
 *   Configure valid, TX/RX, remoteTransfer for a specific Message Object.
 *
 * @details
 *   The initialization bit has to be 0 to use this function.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] msgNum
 *   A message number of this Message Object, [1 - 32].
 *
 * @param[in] valid
 *   True if the Message Object is valid, false otherwise.
 *
 * @param[in] tx
 *   True if the Message Object is used for transmission, false if used for
 *   reception.
 *
 * @param[in] remoteTransfer
 *   True if the Message Object is used for remote transmission, false otherwise.
 *
 * @param[in] endOfBuffer
 *   True if it is for a single Message Object or the end of a FIFO buffer,
 *   false if the Message Object is part of a FIFO buffer and not the last.
 *
 * @param[in] wait
 *   If true, wait for the end of the transfer between the MIRx registers and
 *   the RAM to exit. If false, exit immediately, the transfer can still be
 *   in progress.
 ******************************************************************************/
void CAN_ConfigureMessageObject(CAN_TypeDef *can,
                                uint8_t interface,
                                uint8_t msgNum,
                                bool valid,
                                bool tx,
                                bool remoteTransfer,
                                bool endOfBuffer,
                                bool wait)
{
  CAN_MIR_TypeDef * mir = &can->MIR[interface];

  /* Make sure msgNum is in correct range. */
  EFM_ASSERT((msgNum > 0) && (msgNum <= 32));

  CAN_ReadyWait(can, interface);

  /* Set which registers to read from RAM. */
  mir->CMDMASK = CAN_MIR_CMDMASK_WRRD_READ
                 | CAN_MIR_CMDMASK_ARBACC
                 | CAN_MIR_CMDMASK_CONTROL;

  /* Send reading request and wait (3 to 6 CPU cycle). */
  CAN_SendRequest(can, interface, msgNum, true);

  /* Set which registers to write to RAM. */
  mir->CMDMASK |= CAN_MIR_CMDMASK_WRRD;

  /* Configure a valid message and direction. */
  mir->ARB = (mir->ARB & ~(_CAN_MIR_ARB_DIR_MASK | _CAN_MIR_ARB_MSGVAL_MASK))
             | (valid << _CAN_MIR_ARB_MSGVAL_SHIFT)
             | (tx << _CAN_MIR_ARB_DIR_SHIFT);

  /* Set EOB bit, RX, and TX interrupts. */
  mir->CTRL = (endOfBuffer << _CAN_MIR_CTRL_EOB_SHIFT)
              | _CAN_MIR_CTRL_TXIE_MASK
              | _CAN_MIR_CTRL_RXIE_MASK
              | (remoteTransfer << _CAN_MIR_CTRL_RMTEN_SHIFT);

  /* Send a writing request. */
  CAN_SendRequest(can, interface, msgNum, wait);
}

/***************************************************************************//**
 * @brief
 *   Send data from the Message Object message.
 *
 * @details
 *   If the message is configured as TX and remoteTransfer = 0, calling this function
 *   will send the data of this Message Object if its parameters are correct.
 *   If the message is TX and remoteTransfer = 1, this function will set the data of
 *   message to RAM and exit. Data will be automatically sent after
 *   reception of a remote frame.
 *   If the message is RX and remoteTransfer = 1, this function will send a remote
 *   frame to the corresponding ID.
 *   If the message is RX and remoteTransfer = 0, the user shouldn't call this
 *   function. It will also send a remote frame.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] message
 *   A Message Object.
 *
 * @param[in] wait
 *   If true, wait for the end of the transfer between the MIRx registers and
 *   RAM to exit. If false, exit immediately. The transfer can still be
 *   in progress.
 ******************************************************************************/
void CAN_SendMessage(CAN_TypeDef *can,
                     uint8_t interface,
                     const CAN_MessageObject_TypeDef *message,
                     bool wait)
{
  CAN_MIR_TypeDef * mir = &can->MIR[interface];

  /* Make sure msgNum is in correct range. */
  EFM_ASSERT((message->msgNum > 0) && (message->msgNum <= 32));
  /* Make sure dlc is in correct range. */
  EFM_ASSERT(message->dlc <= _CAN_MIR_CTRL_DLC_MASK);

  CAN_ReadyWait(can, interface);

  /* Set LEC to an unused value to be sure it is reset to 0 after sending. */
  BUS_RegMaskedWrite(&can->STATUS, _CAN_STATUS_LEC_MASK, 0x7);

  /* Set which registers to read from RAM. */
  mir->CMDMASK = CAN_MIR_CMDMASK_WRRD_READ
                 | CAN_MIR_CMDMASK_ARBACC
                 | CAN_MIR_CMDMASK_CONTROL;

  /* Send a reading request and wait (3 to 6 CPU cycle). */
  CAN_SendRequest(can, interface, message->msgNum, true);

  /* Reset MSGVAL. */
  mir->CMDMASK |= CAN_MIR_CMDMASK_WRRD;
  mir->ARB &= ~(0x1U << _CAN_MIR_ARB_MSGVAL_SHIFT);
  CAN_SendRequest(can, interface, message->msgNum, true);

  /* Set which registers to write to RAM. */
  mir->CMDMASK |= CAN_MIR_CMDMASK_DATAA
                  | CAN_MIR_CMDMASK_DATAB;

  /* If TX = 1 and remoteTransfer = 1, nothing is sent. */
  if ( ((mir->CTRL & _CAN_MIR_CTRL_RMTEN_MASK) == 0)
       || ((mir->ARB & _CAN_MIR_ARB_DIR_MASK) == _CAN_MIR_ARB_DIR_RX)) {
    mir->CTRL |= CAN_MIR_CTRL_TXRQST;
    /* DATAVALID is set only if it is not sending a remote message. */
    if ((mir->CTRL & _CAN_MIR_CTRL_RMTEN_MASK) == 0) {
      mir->CTRL |= CAN_MIR_CTRL_DATAVALID;
    }
  }

  /* Set the data length code. */
  mir->CTRL = (mir->CTRL & ~_CAN_MIR_CTRL_DLC_MASK)
              | message->dlc;

  /* Configure the ID. */
  if (message->extended) {
    EFM_ASSERT(message->id <= _CAN_MIR_ARB_ID_MASK);
    mir->ARB = (mir->ARB & ~_CAN_MIR_ARB_ID_MASK)
               | (message->id << _CAN_MIR_ARB_ID_SHIFT)
               | (uint32_t)(0x1U << _CAN_MIR_ARB_MSGVAL_SHIFT)
               | CAN_MIR_ARB_XTD_EXT;
  } else {
    EFM_ASSERT(message->id <= _CAN_MIR_ARB_STD_ID_MAX);
    mir->ARB = (mir->ARB & ~(_CAN_MIR_ARB_ID_MASK | _CAN_MIR_ARB_XTD_MASK))
               | (uint32_t)(0x1U << _CAN_MIR_ARB_MSGVAL_SHIFT)
               | (message->id << _CAN_MIR_ARB_STD_ID_SHIFT)
               | CAN_MIR_ARB_XTD_STD;
  }

  /* Set data. */
  CAN_WriteData(can, interface, message);

  /* Send a writing request. */
  CAN_SendRequest(can, interface, message->msgNum, wait);
}

/***************************************************************************//**
 * @brief
 *   Read data from a Message Object in RAM and store it in a message.
 *
 * @details
 *   Read the information from  RAM on this Message Object : data but
 *   also the configuration of the other registers.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] message
 *   A Message Object.
 ******************************************************************************/
void CAN_ReadMessage(CAN_TypeDef *can,
                     uint8_t interface,
                     CAN_MessageObject_TypeDef *message)
{
  CAN_MIR_TypeDef * mir = &can->MIR[interface];
  uint32_t buffer;
  uint32_t i;

  /* Make sure msgNum is in correct range. */
  EFM_ASSERT((message->msgNum > 0) && (message->msgNum <= 32));

  CAN_ReadyWait(can, interface);

  /* Set which registers to read from RAM. */
  mir->CMDMASK = CAN_MIR_CMDMASK_WRRD_READ
                 | CAN_MIR_CMDMASK_MASKACC
                 | CAN_MIR_CMDMASK_ARBACC
                 | CAN_MIR_CMDMASK_CONTROL
                 | CAN_MIR_CMDMASK_CLRINTPND
                 | CAN_MIR_CMDMASK_TXRQSTNEWDAT
                 | CAN_MIR_CMDMASK_DATAA
                 | CAN_MIR_CMDMASK_DATAB;

  /* Send a reading request and wait (3 to 6 cpu cycle). */
  CAN_SendRequest(can, interface, message->msgNum, true);

  /* Get dlc from the control register. */
  message->dlc = ((mir->CTRL & _CAN_MIR_CTRL_DLC_MASK) >> _CAN_MIR_CTRL_DLC_SHIFT);

  /* Make sure dlc is in correct range. */
  EFM_ASSERT(message->dlc <= 8);

  /* Copy data from the MIR registers to the Message Object message. */
  buffer = mir->DATAL;
  for (i = 0; i < SL_MIN(message->dlc, 4U); ++i) {
    message->data[i] = buffer & 0xFF;
    buffer = buffer >> 8;
  }
  if (message->dlc > 3) {
    buffer = mir->DATAH;
    for (i = 0; i < message->dlc - 4U; ++i) {
      message->data[i + 4] = buffer & 0xFF;
      buffer = buffer >> 8;
    }
  }
}

/***************************************************************************//**
 * @brief
 *   Abort sending a message.
 *
 * @details
 *   Set the TXRQST of the CTRL register to 0. Doesn't touch data or the
 *   other parameters. The user can call CAN_SendMessage() to send the object
 *   after using CAN_AbortSendMessage().
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] msgNum
 *   A message number of this Message Object, [1 - 32].
 *
 * @param[in] wait
 *   If true, wait for the end of the transfer between the MIRx registers and
 *   the RAM to exit. If false, exit immediately. The transfer can still be
 *   in progress.
 ******************************************************************************/
void CAN_AbortSendMessage(CAN_TypeDef *can,
                          uint8_t interface,
                          uint8_t msgNum,
                          bool wait)
{
  /* Make sure msgNum is in correct range. */
  EFM_ASSERT((msgNum > 0) && (msgNum <= 32));

  CAN_MIR_TypeDef * mir = &can->MIR[interface];
  CAN_ReadyWait(can, interface);

  /* Set which registers to write to RAM. */
  mir->CMDMASK = CAN_MIR_CMDMASK_WRRD
                 | CAN_MIR_CMDMASK_ARBACC;

  /* Set TXRQST bit to 0. */
  mir->ARB &= ~_CAN_MIR_CTRL_TXRQST_MASK;

  /* Send a writing request. */
  CAN_SendRequest(can, interface, msgNum, wait);
}

/***************************************************************************//**
 * @brief
 *   Reset all Message Objects and set their data to 0.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 ******************************************************************************/
void CAN_ResetMessages(CAN_TypeDef *can, uint8_t interface)
{
  CAN_MIR_TypeDef * mir = &can->MIR[interface];
  CAN_ReadyWait(can, interface);

  /* Set which registers to read from RAM. */
  mir->CMDMASK = CAN_MIR_CMDMASK_WRRD
                 | CAN_MIR_CMDMASK_MASKACC
                 | CAN_MIR_CMDMASK_ARBACC
                 | CAN_MIR_CMDMASK_CONTROL
                 | CAN_MIR_CMDMASK_DATAA
                 | CAN_MIR_CMDMASK_DATAB;

  mir->MASK    = _CAN_MIR_MASK_RESETVALUE;
  mir->ARB     = _CAN_MIR_ARB_RESETVALUE;
  mir->CTRL    = _CAN_MIR_CTRL_RESETVALUE;
  mir->DATAL   = 0x00000000;
  mir->DATAH   = 0x00000000;

  /* Write each reset Message Object to RAM. */
  for (int i = 1; i <= 32; ++i) {
    CAN_SendRequest(can, interface, i, true);
  }
}

/***************************************************************************//**
 * @brief
 *   Set all CAN registers to RESETVALUE. Leave the CAN device disabled.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 ******************************************************************************/
void CAN_Reset(CAN_TypeDef *can)
{
  CAN_ReadyWait(can, 0);
  CAN_ReadyWait(can, 1);

  CAN_Enable(can, false);
  can->STATUS = _CAN_STATUS_RESETVALUE;

  can->CTRL |= _CAN_CTRL_CCE_MASK;
  can->BITTIMING = _CAN_BITTIMING_RESETVALUE;
  can->CTRL &= ~_CAN_CTRL_CCE_MASK;

  can->CTRL |= _CAN_CTRL_TEST_MASK;
  can->TEST = _CAN_TEST_RESETVALUE;
  can->CTRL &= ~_CAN_CTRL_TEST_MASK;

  can->BRPE = _CAN_BRPE_RESETVALUE;
  can->CONFIG = _CAN_CONFIG_RESETVALUE;
  can->IF0IFS = _CAN_IF0IFS_RESETVALUE;
  can->IF0IFC = _CAN_IF0IFC_RESETVALUE;
  can->IF0IEN = _CAN_IF0IEN_RESETVALUE;
  can->IF1IFS = _CAN_IF1IF_RESETVALUE;
  can->IF1IFC = _CAN_IF1IFC_RESETVALUE;
  can->IF1IEN = _CAN_IF1IEN_RESETVALUE;
  can->ROUTE = _CAN_ROUTE_RESETVALUE;

  for (int i = 0; i < 2; i++) {
    can->MIR[i].CMDMASK = _CAN_MIR_CMDMASK_RESETVALUE;
    can->MIR[i].MASK = _CAN_MIR_MASK_RESETVALUE;
    can->MIR[i].ARB = _CAN_MIR_ARB_RESETVALUE;
    can->MIR[i].CTRL = _CAN_MIR_CTRL_RESETVALUE;
    can->MIR[i].DATAL = _CAN_MIR_DATAL_RESETVALUE;
    can->MIR[i].DATAH = _CAN_MIR_DATAH_RESETVALUE;
    can->MIR[i].CMDREQ = _CAN_MIR_CMDREQ_RESETVALUE;
  }
}

/***************************************************************************//**
 * @brief
 *   Write data from a message to the MIRx registers.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] message
 *   A Message Object.
 ******************************************************************************/
void CAN_WriteData(CAN_TypeDef *can,
                   uint8_t interface,
                   const CAN_MessageObject_TypeDef *message)
{
  uint32_t tmp;
  uint8_t data[8] = { 0 };
  size_t length = SL_MIN(8, message->dlc);
  CAN_MIR_TypeDef * mir = &can->MIR[interface];

  for (size_t i = 0; i < length; i++) {
    data[i] = message->data[i];
  }

  CAN_ReadyWait(can, interface);

  tmp = data[0];
  tmp |= data[1] << 8;
  tmp |= data[2] << 16;
  tmp |= data[3] << 24;
  mir->DATAL = tmp;

  tmp = data[4];
  tmp |= data[5] << 8;
  tmp |= data[6] << 16;
  tmp |= data[7] << 24;
  mir->DATAH = tmp;
}

/***************************************************************************//**
 * @brief
 *   Send a request for writing or reading RAM of the Message Object msgNum.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 * @param[in] msgNum
 *   A message number of the Message Object, [1 - 32].
 *
 * @param[in] wait
 *   If true, wait for the end of the transfer between the MIRx registers and
 *   the RAM to exit. If false, exit immediately. The transfer can still be
 *   in progress.
 ******************************************************************************/
void CAN_SendRequest(CAN_TypeDef *can,
                     uint8_t interface,
                     uint8_t msgNum,
                     bool wait)
{
  CAN_MIR_TypeDef * mir = &can->MIR[interface];

  /* Make sure msgNum is in correct range. */
  EFM_ASSERT((msgNum > 0) && (msgNum <= 32));

  /* Make sure the MIRx registers aren't busy. */
  CAN_ReadyWait(can, interface);

  /* Write msgNum to the CMDREQ register. */
  mir->CMDREQ = msgNum << _CAN_MIR_CMDREQ_MSGNUM_SHIFT;

  if (wait) {
    CAN_ReadyWait(can, interface);
  }
}

/** @} (end addtogroup CAN) */
/** @} (end addtogroup emlib) */

#endif /* defined(CAN_COUNT) && (CAN_COUNT > 0) */
