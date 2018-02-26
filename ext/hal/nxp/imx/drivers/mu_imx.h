/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
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

#ifndef __MU_IMX_H__
#define __MU_IMX_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup mu_driver
 * @{
 */

/******************************************************************************
 * Definitions
 *****************************************************************************/

/*!@brief Bit mask for general purpose interrupt 0 pending. */
#define MU_SR_GIP0_MASK (1U<<31U)
/*!@brief Bit mask for RX full interrupt 0 pending. */
#define MU_SR_RF0_MASK (1U<<27U)
/*!@brief Bit mask for TX empty interrupt 0 pending. */
#define MU_SR_TE0_MASK (1U<<23U)
/*!@brief Bit mask for general purpose interrupt 0 enable. */
#define MU_CR_GIE0_MASK (1U<<31U)
/*!@brief Bit mask for RX full interrupt 0 enable. */
#define MU_CR_RIE0_MASK (1U<<27U)
/*!@brief Bit mask for TX empty interrupt 0 enable. */
#define MU_CR_TIE0_MASK (1U<<23U)
/*!@brief Bit mask to trigger general purpose interrupt 0. */
#define MU_CR_GIR0_MASK (1U<<19U)

/*!@brief Number of general purpose interrupt. */
#define MU_GPn_COUNT (4U)

/* Mask for MU_CR_GIRN. When read-modify-write to MU_CR, should
   pay attention to these bits in case of trigger interrupts by mistake.*/

/*! @brief MU status return codes. */
typedef enum _mu_status
{
    kStatus_MU_Success      = 0U, /*!< Success.                              */
    kStatus_MU_TxNotEmpty   = 1U, /*!< TX register is not empty.             */
    kStatus_MU_RxNotFull    = 2U, /*!< RX register is not full.              */
    kStatus_MU_FlagPending  = 3U, /*!< Previous flags update pending.        */
    kStatus_MU_EventPending = 4U, /*!< MU event is pending.                  */
    kStatus_MU_Initialized  = 5U, /*!< MU driver has initialized previously. */
    kStatus_MU_IntPending   = 6U, /*!< Previous general interrupt still pending. */
    kStatus_MU_Failed       = 7U  /*!< Execution failed.                     */
} mu_status_t;

/*! @brief MU message status. */
typedef enum _mu_msg_status
{
    kMuTxEmpty0 = MU_SR_TE0_MASK,        /*!< TX0 empty status.                          */
    kMuTxEmpty1 = MU_SR_TE0_MASK >> 1U,  /*!< TX1 empty status.                          */
    kMuTxEmpty2 = MU_SR_TE0_MASK >> 2U,  /*!< TX2 empty status.                          */
    kMuTxEmpty3 = MU_SR_TE0_MASK >> 3U,  /*!< TX3 empty status.                          */
    kMuTxEmpty  = kMuTxEmpty0  |
                  kMuTxEmpty1  |
                  kMuTxEmpty2  |
                  kMuTxEmpty3,           /*!< TX empty status.                            */

    kMuRxFull0  = MU_SR_RF0_MASK,        /*!< RX0 full status.                            */
    kMuRxFull1  = MU_SR_RF0_MASK >> 1U,  /*!< RX1 full status.                            */
    kMuRxFull2  = MU_SR_RF0_MASK >> 2U,  /*!< RX2 full status.                            */
    kMuRxFull3  = MU_SR_RF0_MASK >> 3U,  /*!< RX3 full status.                            */
    kMuRxFull   = kMuRxFull0   |
                  kMuRxFull1   |
                  kMuRxFull2   |
                  kMuRxFull3,            /*!< RX empty status.                            */

    kMuGenInt0  = MU_SR_GIP0_MASK,       /*!< General purpose interrupt 0 pending status. */
    kMuGenInt1  = MU_SR_GIP0_MASK >> 1U, /*!< General purpose interrupt 2 pending status. */
    kMuGenInt2  = MU_SR_GIP0_MASK >> 2U, /*!< General purpose interrupt 2 pending status. */
    kMuGenInt3  = MU_SR_GIP0_MASK >> 3U, /*!< General purpose interrupt 3 pending status. */
    kMuGenInt   = kMuGenInt0    |
                  kMuGenInt1    |
                  kMuGenInt2    |
                  kMuGenInt3,      /*!< General purpose interrupt pending status.   */

    kMuStatusAll = kMuTxEmpty |
                   kMuRxFull  |
                   kMuGenInt,      /*!< All MU status.                              */

} mu_msg_status_t;

/*! @brief Power mode definition. */
typedef enum _mu_power_mode
{
    kMuPowerModeRun  = 0x00U, /*!< Run mode.  */
    kMuPowerModeWait = 0x01U, /*!< WAIT mode. */
    kMuPowerModeStop = 0x02U, /*!< STOP mode. */
    kMuPowerModeDsm  = 0x03U, /*!< DSM mode.  */
} mu_power_mode_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization.
 * @{
 */
/*!
 * @brief Initializes the MU module to reset state.
 *        This function sets the MU module control register to its default reset value.
 *
 * @param base Register base address for the module.
 */
static inline void MU_Init(MU_Type * base)
{
    // Clear GIEn, RIEn, TIEn, GIRn and ABFn.
    base->CR &= ~(MU_CR_GIEn_MASK | MU_CR_RIEn_MASK | MU_CR_TIEn_MASK | MU_CR_GIRn_MASK | MU_CR_Fn_MASK);
}

/* @} */

/*!
 * @name Send Messages.
 * @{
 */

/*!
 * @brief Try to send a message.
 *
 * This function tries to send a message, if the TX register is not empty,
 * this function returns kStatus_MU_TxNotEmpty.
 *
 * @param base Register base address for the module.
 * @param regIdex  Tx register index.
 * @param msg      Message to send.
 * @retval kStatus_MU_Success     Message send successfully.
 * @retval kStatus_MU_TxNotEmpty  Message not send because TX is not empty.
 */
mu_status_t MU_TrySendMsg(MU_Type * base, uint32_t regIndex, uint32_t msg);

/*!
 * @brief Block to send a message.
 *
 * This function waits until TX register is empty and send the message.
 *
 * @param base Register base address for the module.
 * @param regIdex  Tx register index.
 * @param msg      Message to send.
 */
void MU_SendMsg(MU_Type * base, uint32_t regIndex, uint32_t msg);

/*!
 * @brief Check TX empty status.
 *
 * This function checks the specific transmit register empty status.
 *
 * @param base Register base address for the module.
 * @param index TX register index to check.
 * @retval true TX register is empty.
 * @retval false TX register is not empty.
 */
static inline bool MU_IsTxEmpty(MU_Type * base, uint32_t index)
{
    return (bool)(base->SR & (MU_SR_TE0_MASK >> index));
}

/*!
 * @brief Enable TX empty interrupt.
 *
 * This function enables specific TX empty interrupt.
 *
 * @param base Register base address for the module.
 * @param index TX interrupt index to enable.
 *
 * Example:
   @code
   // To enable TX0 empty interrupts.
   MU_EnableTxEmptyInt(MU0_BASE, 0U);
   @endcode
 */
static inline void MU_EnableTxEmptyInt(MU_Type * base, uint32_t index)
{
    base->CR = (base->CR & ~ MU_CR_GIRn_MASK)   // Clear GIRn
                         | (MU_CR_TIE0_MASK>>index); // Set TIEn
}

/*!
 * @brief Disable TX empty interrupt.
 *
 * This function disables specific TX empty interrupt.
 *
 * @param base Register base address for the module.
 * @param disableMask Bitmap of the interrupts to disable.
 *
 * Example:
   @code
   // To disable TX0 empty interrupts.
   MU_DisableTxEmptyInt(MU0_BASE, 0U);
   @endcode
 */
static inline void MU_DisableTxEmptyInt(MU_Type * base, uint32_t index)
{
    base->CR &= ~(MU_CR_GIRn_MASK | (MU_CR_TIE0_MASK>>index)); // Clear GIRn , clear TIEn
}

/* @} */

/*!
 * @name Receive Messages.
 * @{
 */

/*!
 * @brief Try to receive a message.
 *
 * This function tries to receive a message, if the RX register is not full,
 * this function returns kStatus_MU_RxNotFull.
 *
 * @param base Register base address for the module.
 * @param regIdex Rx register index.
 * @param msg Message to receive.
 * @retval kStatus_MU_Success    Message receive successfully.
 * @retval kStatus_MU_RxNotFull  Message not received because RX is not full.
 */
mu_status_t MU_TryReceiveMsg(MU_Type * base, uint32_t regIndex, uint32_t *msg);

/*!
 * @brief Block to receive a message.
 *
 * This function waits until RX register is full and receive the message.
 *
 * @param base Register base address for the module.
 * @param regIdex Rx register index.
 * @param msg Message to receive.
 */
void MU_ReceiveMsg(MU_Type * base, uint32_t regIndex, uint32_t *msg);

/*!
 * @brief Check RX full status.
 *
 * This function checks the specific receive register full status.
 *
 * @param base Register base address for the module.
 * @param index RX register index to check.
 * @retval true RX register is full.
 * @retval false RX register is not full.
 */
static inline bool MU_IsRxFull(MU_Type * base, uint32_t index)
{
    return (bool)(base->SR & (MU_SR_RF0_MASK >> index));
}

/*!
 * @brief Enable RX full interrupt.
 *
 * This function enables specific RX full interrupt.
 *
 * @param base Register base address for the module.
 * @param index RX interrupt index to enable.
 *
 * Example:
   @code
   // To enable RX0 full interrupts.
   MU_EnableRxFullInt(MU0_BASE, 0U);
   @endcode
 */
static inline void MU_EnableRxFullInt(MU_Type * base, uint32_t index)
{
    base->CR = (base->CR & ~MU_CR_GIRn_MASK)  // Clear GIRn
                         | (MU_CR_RIE0_MASK>>index); // Set RIEn
}

/*!
 * @brief Disable RX full interrupt.
 *
 * This function disables specific RX full interrupt.
 *
 * @param base Register base address for the module.
 * @param disableMask Bitmap of the interrupts to disable.
 *
 * Example:
   @code
   // To disable RX0 full interrupts.
   MU_DisableRxFullInt(MU0_BASE, 0U);
   @endcode
 */
static inline void MU_DisableRxFullInt(MU_Type * base, uint32_t index)
{
    base->CR &= ~(MU_CR_GIRn_MASK | (MU_CR_RIE0_MASK>>index)); // Clear GIRn, clear RIEn
}

/* @} */

/*!
 * @name General Purpose Interrupt.
 * @{
 */

/*!
 * @brief Enable general purpose interrupt.
 *
 * This function enables specific general purpose interrupt.
 *
 * @param base Register base address for the module.
 * @param index General purpose interrupt index to enable.
 *
 * Example:
   @code
   // To enable general purpose interrupts 0.
   MU_EnableGeneralInt(MU0_BASE, 0U);
   @endcode
 */
static inline void MU_EnableGeneralInt(MU_Type * base, uint32_t index)
{
    base->CR = (base->CR & ~MU_CR_GIRn_MASK)  // Clear GIRn
                         | (MU_CR_GIE0_MASK>>index); // Set GIEn
}

/*!
 * @brief Disable general purpose interrupt.
 *
 * This function disables specific general purpose interrupt.
 *
 * @param base Register base address for the module.
 * @param index General purpose interrupt index to disable.
 *
 * Example:
   @code
   // To disable general purpose interrupts 0.
   MU_DisableGeneralInt(MU0_BASE, 0U);
   @endcode
 */
static inline void MU_DisableGeneralInt(MU_Type * base, uint32_t index)
{
    base->CR &= ~(MU_CR_GIRn_MASK | (MU_CR_GIE0_MASK>>index));  // Clear GIRn, clear GIEn
}

/*!
 * @brief Check specific general purpose interrupt pending flag.
 *
 * This function checks the specific general purpose interrupt pending status.
 *
 * @param base Register base address for the module.
 * @param index Index of the general purpose interrupt flag to check.
 * @retval true General purpose interrupt is pending.
 * @retval false General purpose interrupt is not pending.
 */
static inline bool MU_IsGeneralIntPending(MU_Type * base, uint32_t index)
{
    return (bool)(base->SR & (MU_SR_GIP0_MASK >> index));
}

/*!
 * @brief Clear specific general purpose interrupt pending flag.
 *
 * This function clears the specific general purpose interrupt pending status.
 *
 * @param base Register base address for the module.
 * @param index Index of the general purpose interrupt flag to clear.
 */
static inline void MU_ClearGeneralIntPending(MU_Type * base, uint32_t index)
{
    base->SR = (MU_SR_GIP0_MASK >> index);
}

/*!
 * @brief Trigger specific general purpose interrupt.
 *
 * This function triggers specific general purpose interrupt to other core.
 *
 * To ensure proper operations, make sure the correspond general purpose
 * interrupt triggered previously has been accepted by the other core. The
 * function MU_IsGeneralIntAccepted can be used for this check. If the
 * previous general interrupt has not been accepted by the other core, this
 * function does not trigger interrupt actually and returns an error.
 *
 * @param base Register base address for the module.
 * @param index Index of general purpose interrupt to trigger.
 * @retval kStatus_MU_Success    Interrupt has been triggered successfully.
 * @retval kStatus_MU_IntPending Previous interrupt has not been accepted.
 */
mu_status_t MU_TriggerGeneralInt(MU_Type * base, uint32_t index);

/*!
 * @brief Check specific general purpose interrupt is accepted or not.
 *
 * This function checks whether the specific general purpose interrupt has
 * been accepted by the other core or not.
 *
 * @param base Register base address for the module.
 * @param index Index of the general purpose interrupt to check.
 * @retval true  General purpose interrupt is accepted.
 * @retval false General purpose interrupt is not accepted.
 */
static inline bool MU_IsGeneralIntAccepted(MU_Type * base, uint32_t index)
{
    return !(bool)(base->CR & (MU_CR_GIR0_MASK >> index));
}

/* @} */

/*!
 * @name Flags
 * @{
 */

/*!
 * @brief Try to set some bits of the 3-bit flag reflect on the other MU side.
 *
 * This functions tries to set some bits of the 3-bit flag. If previous flags
 * update is still pending, this function returns kStatus_MU_FlagPending.
 *
 * @param base Register base address for the module.
 * @retval kStatus_MU_Success     Flag set successfully.
 * @retval kStatus_MU_FlagPending Previous flag update is pending.
 */
mu_status_t MU_TrySetFlags(MU_Type * base, uint32_t flags);

/*!
 * @brief Set some bits of the 3-bit flag reflect on the other MU side.
 *
 * This functions set some bits of the 3-bit flag. If previous flags update is
 * still pending, this function  blocks and polls to set the flag.
 *
 * @param base Register base address for the module.
 */
void MU_SetFlags(MU_Type * base, uint32_t flags);

/*!
 * @brief Checks whether the previous flag update is pending.
 *
 * After setting flags, the flags update request is pending until internally
 * acknowledged. During the pending period, it is not allowed to set flags again.
 * This function is used to check the pending status, it can be used together
 * with function MU_TrySetFlags.
 *
 * @param base Register base address for the module.
 * @return True if pending, false if not.
 */
static inline bool MU_IsFlagPending(MU_Type * base)
{
    return (bool)(base->SR & MU_SR_FUP_MASK);
}

/*!
 * @brief Get the current value of the 3-bit flag set by other side.
 *
 * This functions gets the current value of the 3-bit flag.
 *
 * @param base Register base address for the module.
 * @return flags Current value of the 3-bit flag.
 */
static inline uint32_t MU_GetFlags(MU_Type * base)
{
    return base->SR & MU_SR_Fn_MASK;
}

/* @} */

/*!
 * @name Misc.
 * @{
 */

/*!
 * @brief Get the power mode of the other core.
 *
 * This functions gets the power mode of the other core.
 *
 * @param base Register base address for the module.
 * @return powermode Power mode of the other core.
 */
static inline mu_power_mode_t MU_GetOtherCorePowerMode(MU_Type * base)
{
    return (mu_power_mode_t)((base->SR & MU_SR_PM_MASK) >> MU_SR_PM_SHIFT);
}

/*!
 * @brief Get the event pending status.
 *
 * This functions gets the event pending status. To ensure events have been
 * posted to the other side before entering STOP mode, verify the
 * event pending status using this function.
 *
 * @param base Register base address for the module.
 * @retval true  Event is pending.
 * @retval false Event is not pending.
 */
static inline bool MU_IsEventPending(MU_Type * base)
{
    return (bool)(base->SR & MU_SR_EP_MASK);
}

/*!
 * @brief Get the the MU message status.
 *
 * This functions gets TX/RX and general purpose interrupt pending status. The
 * parameter is passed in as bitmask of the status to check.
 *
 * @param base Register base address for the module.
 * @param statusToCheck The status to check, see mu_msg_status_t.
 * @return Status checked.
 *
 * Example:
   @code
   // To check TX0 empty status.
   MU_GetMsgStatus(MU0_BASE, kMuTxEmpty0);

   // To check all RX full status.
   MU_GetMsgStatus(MU0_BASE, kMuRxFull);

   // To check general purpose interrupt 0 and 3 pending status.
   MU_GetMsgStatus(MU0_BASE, kMuGenInt0 | kMuGenInt3);

   // To check all status.
   MU_GetMsgStatus(MU0_BASE, kMuStatusAll);

   @endcode
 */
static inline uint32_t MU_GetMsgStatus(MU_Type * base, uint32_t statusToCheck)
{
    return base->SR & statusToCheck;
}

/* @} */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

#endif /* __MU_IMX_H__ */
/******************************************************************************
 * EOF
 *****************************************************************************/
