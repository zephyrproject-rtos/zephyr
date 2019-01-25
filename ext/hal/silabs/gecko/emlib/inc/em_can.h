/***************************************************************************//**
 * @file em_can.h
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

#ifndef EM_CAN_H
#define EM_CAN_H

#include "em_bus.h"
#include "em_device.h"
#include <stdbool.h>

#if defined(CAN_COUNT) && (CAN_COUNT > 0)

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CAN
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** CAN Status codes. */
typedef enum {
  /** No error occurred during the last CAN bus event. */
  canErrorNoError  = CAN_STATUS_LEC_NONE,

  /**
   * More than 5 equal bits in a sequence have occurred in a part of a received
   * message where this is not allowed.
   */
  canErrorStuff    = CAN_STATUS_LEC_STUFF,

  /** A fixed format part of a received frame has the wrong format. */
  canErrorForm     = CAN_STATUS_LEC_FORM,

  /** The message this CAN Core transmitted was not acknowledged by another node. */
  canErrorAck      = CAN_STATUS_LEC_ACK,

  /** A wrong monitored bus value : dominant when the module wants to send a recessive. */
  canErrorBit1     = CAN_STATUS_LEC_BIT1,

  /** A wrong monitored bus value : recessive when the module intends to send a dominant. */
  canErrorBit0     = CAN_STATUS_LEC_BIT0,

  /** CRC check sum incorrect. */
  canErrorCrc      = CAN_STATUS_LEC_CRC,

  /** Unused. No new error since the CPU wrote this value. */
  canErrorUnused   = CAN_STATUS_LEC_UNUSED
} CAN_ErrorCode_TypeDef;

/** CAN peripheral mode. */
typedef enum {
  /** CAN peripheral in Normal mode : ready to send and receive messages. */
  canModeNormal,

  /** CAN peripheral in Basic mode : no use of the RAM. */
  canModeBasic,

  /**
   * CAN peripheral in Loopback mode : input from the CAN bus is disregarded
   * and comes from TX instead.
   */
  canModeLoopBack,

  /**
   * CAN peripheral in SilentLoopback mode : input from the CAN bus is
   * disregarded and comes from TX instead ; no output on the CAN bus.
   */
  canModeSilentLoopBack,

  /** CAN peripheral in Silent mode : no output on the CAN bus. If required to
   * send a dominant bit, it's rerouted internally so that the CAN module
   * monitors it but the CAN bus stays recessive.
   */
  canModeSilent
} CAN_Mode_TypeDef;

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** CAN Message Object TypeDef structure. LSBs is used. */
typedef struct {
  /** A message number of this Message Object, [1 - 32]. */
  uint8_t   msgNum;

  /** ID extended if true, standard if false.  */
  bool      extended;

  /**
   * ID of the message with 11 bits (standard) or 28 bits (extended).
   * LSBs are used for both.
   */
  uint32_t  id;

  /** Data Length Code [0 - 8].  */
  uint8_t   dlc;

  /** A pointer to data, [0 - 8] bytes.  */
  uint8_t   data[8];

  /** A mask for ID filtering. */
  uint32_t  mask;

  /** Enable the use of 'extended' value for filtering. */
  bool      extendedMask;

  /** Enable the use of 'direction' value for filtering. */
  bool      directionMask;
} CAN_MessageObject_TypeDef;

/** CAN initialization structure. */
typedef struct {
  /** True to set the CAN Device in normal mode after initialization. */
  bool      enable;

  /** True to reset messages during initialization. */
  bool      resetMessages;

  /** Default bitrate. */
  uint32_t  bitrate;

  /** Default Propagation Time Segment. */
  uint8_t   propagationTimeSegment;

  /** Default Phase Buffer Segment 1. */
  uint8_t   phaseBufferSegment1;

  /** Default Phase Buffer Segment 2. */
  uint8_t   phaseBufferSegment2;

  /** Default Synchronisation Jump Width. */
  uint8_t   synchronisationJumpWidth;
} CAN_Init_TypeDef;

/**
 * Default initialization of CAN_Init_TypeDef. The total duration of a bit with
 * these default parameters is 10 tq (time quantum : tq = brp/fsys, brp being
 * the baudrate prescaler and being set according to the wanted bitrate, fsys
 * beeing the CAN device frequency).
 */
#define CAN_INIT_DEFAULT                                                      \
  {                                                                           \
    true,     /** Set the CAN Device in normal mode after initialization.  */ \
    true,     /** Reset messages during initialization.         */            \
    100000,   /** Set bitrate to 100 000                        */            \
    1,        /** Set the Propagation Time Segment to 1.         */           \
    4,        /** Set the Phase Buffer Segment 1 to 4.           */           \
    4,        /** Set the Phase Buffer Segment 2 to 4.           */           \
    1         /** Set the Synchronization Jump Width to 1.       */           \
  }

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void CAN_Init(CAN_TypeDef *can, const CAN_Init_TypeDef *init);

uint32_t CAN_GetClockFrequency(CAN_TypeDef *can);

bool CAN_MessageLost(CAN_TypeDef *can, uint8_t interface, uint8_t msgNum);

void CAN_SetRoute(CAN_TypeDef *can,
                  bool active,
                  uint16_t pinRxLoc,
                  uint16_t pinTxLoc);

void CAN_SetBitTiming(CAN_TypeDef *can,
                      uint32_t bitrate,
                      uint16_t propagationTimeSegment,
                      uint16_t phaseBufferSegment1,
                      uint16_t phaseBufferSegment2,
                      uint16_t synchronisationJumpWidth);

void CAN_SetMode(CAN_TypeDef *can, CAN_Mode_TypeDef mode);

void CAN_SetIdAndFilter(CAN_TypeDef *can,
                        uint8_t interface,
                        bool useMask,
                        const CAN_MessageObject_TypeDef *message,
                        bool wait);

void CAN_ConfigureMessageObject(CAN_TypeDef *can,
                                uint8_t interface,
                                uint8_t msgNum,
                                bool valid,
                                bool tx,
                                bool remoteTransfer,
                                bool endOfBuffer,
                                bool wait);

void CAN_SendMessage(CAN_TypeDef *can,
                     uint8_t interface,
                     const CAN_MessageObject_TypeDef *message,
                     bool wait);

void CAN_ReadMessage(CAN_TypeDef *can,
                     uint8_t interface,
                     CAN_MessageObject_TypeDef *message);

void CAN_AbortSendMessage(CAN_TypeDef *can,
                          uint8_t interface,
                          uint8_t msgNum,
                          bool wait);

void CAN_ResetMessages(CAN_TypeDef *can, uint8_t interface);

void CAN_Reset(CAN_TypeDef *can);

void CAN_WriteData(CAN_TypeDef *can,
                   uint8_t interface,
                   const CAN_MessageObject_TypeDef *message);

void CAN_SendRequest(CAN_TypeDef *can,
                     uint8_t interface,
                     uint8_t msgNum,
                     bool wait);

/***************************************************************************//**
 * @brief
 *   Enable the Host Controller to send messages.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] enable
 *   True to enable CAN device, false to disable it. If the CAN device is
 *   enabled, it goes to normal mode (the default working mode).
 ******************************************************************************/
__STATIC_INLINE void CAN_Enable(CAN_TypeDef *can, bool enable)
{
  BUS_RegBitWrite(&can->CTRL, _CAN_CTRL_INIT_SHIFT, (enable ? 0 : 1));
}

/***************************************************************************//**
 * @brief
 *   Give the communication capabilities state.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   True if the Host Controller can send messages, false otherwise.
 ******************************************************************************/
__STATIC_INLINE bool CAN_IsEnabled(CAN_TypeDef *can)
{
  return (can->CTRL & _CAN_CTRL_INIT_MASK) == 0;
}

/***************************************************************************//**
 * @brief
 *   Waiting function.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] interface
 *   Indicate which Message Interface Register to use.
 *
 ******************************************************************************/
__STATIC_INLINE void CAN_ReadyWait(CAN_TypeDef *can,
                                   uint8_t interface)
{
  while ((_CAN_MIR_CMDREQ_BUSY_MASK & can->MIR[interface].CMDREQ) != 0) {
  }
}

/***************************************************************************//**
 * @brief
 *   Get the last error code and clear its register.
 *
 * @param[in] can
 *   Pointer to CAN peripheral register block.
 *
 * @return
 *   return Last error code.
 ******************************************************************************/
__STATIC_INLINE CAN_ErrorCode_TypeDef CAN_GetLastErrorCode(CAN_TypeDef *can)
{
  CAN_ErrorCode_TypeDef errorCode = (CAN_ErrorCode_TypeDef)
                                    (can->STATUS & _CAN_STATUS_LEC_MASK);
  can->STATUS |= ~_CAN_STATUS_LEC_MASK;
  return errorCode;
}

/***************************************************************************//**
 * @brief
 *   Indicates which message objects have received new data.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   State of MESSAGEDATA register indicating which message objects have received
 *   new data.
 ******************************************************************************/
__STATIC_INLINE uint32_t CAN_HasNewdata(CAN_TypeDef *can)
{
  return can->MESSAGEDATA;
}

/***************************************************************************//**
 * @brief
 *   Clear one or more pending CAN status interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   Pending CAN status interrupt source(s) to clear.
 ******************************************************************************/
__STATIC_INLINE void CAN_StatusIntClear(CAN_TypeDef *can, uint32_t flags)
{
  can->IF1IFC = flags;
}

/***************************************************************************//**
 * @brief
 *   Disable CAN status interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   CAN status interrupt source(s) to disable.
 ******************************************************************************/
__STATIC_INLINE void CAN_StatusIntDisable(CAN_TypeDef *can, uint32_t flags)
{
  can->IF1IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable CAN status interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   CAN status interrupt source(s) to enable.
 ******************************************************************************/
__STATIC_INLINE void CAN_StatusIntEnable(CAN_TypeDef *can, uint32_t flags)
{
  can->IF1IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending CAN status interrupt flags.
 *
 * @note
 *   This function does not clear event bits.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   CAN interrupt source(s) pending.
 ******************************************************************************/
__STATIC_INLINE uint32_t CAN_StatusIntGet(CAN_TypeDef *can)
{
  return can->IF1IF;
}

/***************************************************************************//**
 * @brief
 *   Get pending and enabled CAN status interrupt flags.
 *
 * @note
 *   This function does not clear event bits.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   CAN interrupt source(s) pending and enabled.
 ******************************************************************************/
__STATIC_INLINE uint32_t CAN_StatusIntGetEnabled(CAN_TypeDef *can)
{
  uint32_t ien;

  ien = can->IF1IEN;
  return can->IF1IF & ien;
}

/***************************************************************************//**
 * @brief
 *   Set one or more CAN status interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   CAN status interrupt source(s) to set to pending.
 ******************************************************************************/
__STATIC_INLINE void CAN_StatusIntSet(CAN_TypeDef *can, uint32_t flags)
{
  can->IF1IFS = flags;
}

/***************************************************************************//**
 * @brief
 *   Get CAN status.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   A value of CAN register STATUS.
 ******************************************************************************/
__STATIC_INLINE uint32_t CAN_StatusGet(CAN_TypeDef *can)
{
  return can->STATUS & ~_CAN_STATUS_LEC_MASK;
}

/***************************************************************************//**
 * @brief
 *   Clear CAN status.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   CAN status bits to clear.
 ******************************************************************************/
__STATIC_INLINE void CAN_StatusClear(CAN_TypeDef *can, uint32_t flags)
{
  can->STATUS &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Get the error count.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   Error count.
 ******************************************************************************/
__STATIC_INLINE uint32_t CAN_GetErrorCount(CAN_TypeDef *can)
{
  return can->ERRCNT;
}

/***************************************************************************//**
 * @brief
 *   Clear one or more pending CAN message interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   Pending CAN message interrupt source(s) to clear.
 ******************************************************************************/
__STATIC_INLINE void CAN_MessageIntClear(CAN_TypeDef *can, uint32_t flags)
{
  can->IF0IFC = flags;
}

/***************************************************************************//**
 * @brief
 *   Disable CAN message interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   CAN message interrupt source(s) to disable.
 ******************************************************************************/
__STATIC_INLINE void CAN_MessageIntDisable(CAN_TypeDef *can, uint32_t flags)
{
  can->IF0IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable CAN message interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   CAN message interrupt source(s) to enable.
 ******************************************************************************/
__STATIC_INLINE void CAN_MessageIntEnable(CAN_TypeDef *can, uint32_t flags)
{
  can->IF0IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending CAN message interrupt flags.
 *
 * @note
 *   This function does not clear event bits.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   CAN message interrupt source(s) pending.
 ******************************************************************************/
__STATIC_INLINE uint32_t CAN_MessageIntGet(CAN_TypeDef *can)
{
  return can->IF0IF;
}

/***************************************************************************//**
 * @brief
 *   Get CAN message interrupt flags that are pending and enabled.
 *
 * @note
 *   This function does not clear event bits.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @return
 *   CAN message interrupt source(s) pending and enabled.
 ******************************************************************************/
__STATIC_INLINE uint32_t CAN_MessageIntGetEnabled(CAN_TypeDef *can)
{
  uint32_t ien;

  ien = can->IF0IEN;
  return can->IF0IF & ien;
}

/***************************************************************************//**
 * @brief
 *   Set one or more CAN message interrupts.
 *
 * @param[in] can
 *   A pointer to the CAN peripheral register block.
 *
 * @param[in] flags
 *   CAN message interrupt source(s) to set as pending.
 ******************************************************************************/
__STATIC_INLINE void CAN_MessageIntSet(CAN_TypeDef *can, uint32_t flags)
{
  can->IF0IFS = flags;
}

/** @} (end addtogroup CAN) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(CAN_COUNT) && (CAN_COUNT > 0) */
#endif /* EM_CAN_H */
