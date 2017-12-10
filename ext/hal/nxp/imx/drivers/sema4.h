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

#ifndef __SEMA4_H__
#define __SEMA4_H__

#include <stdint.h>
#include <stdbool.h>
#include "device_imx.h"

/*!
 * @addtogroup sema4_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SEMA4_PROCESSOR_NONE         (0xFF)
#define SEMA4_GATE_STATUS_FLAG(gate) ((uint16_t)(1U << ((gate) ^ 7)))

/*! @brief Status flag. */
enum _sema4_status_flag
{
    sema4StatusFlagGate0  = 1U << 7,  /*!< Sema4 Gate 0 flag. */
    sema4StatusFlagGate1  = 1U << 6,  /*!< Sema4 Gate 1 flag. */
    sema4StatusFlagGate2  = 1U << 5,  /*!< Sema4 Gate 2 flag. */
    sema4StatusFlagGate3  = 1U << 4,  /*!< Sema4 Gate 3 flag. */
    sema4StatusFlagGate4  = 1U << 3,  /*!< Sema4 Gate 4 flag. */
    sema4StatusFlagGate5  = 1U << 2,  /*!< Sema4 Gate 5 flag. */
    sema4StatusFlagGate6  = 1U << 1,  /*!< Sema4 Gate 6 flag. */
    sema4StatusFlagGate7  = 1U << 0,  /*!< Sema4 Gate 7 flag. */
    sema4StatusFlagGate8  = 1U << 15, /*!< Sema4 Gate 8 flag. */
    sema4StatusFlagGate9  = 1U << 14, /*!< Sema4 Gate 9 flag. */
    sema4StatusFlagGate10 = 1U << 13, /*!< Sema4 Gate 10 flag. */
    sema4StatusFlagGate11 = 1U << 12, /*!< Sema4 Gate 11 flag. */
    sema4StatusFlagGate12 = 1U << 11, /*!< Sema4 Gate 12 flag. */
    sema4StatusFlagGate13 = 1U << 10, /*!< Sema4 Gate 13 flag. */
    sema4StatusFlagGate14 = 1U << 9,  /*!< Sema4 Gate 14 flag. */
    sema4StatusFlagGate15 = 1U << 8,  /*!< Sema4 Gate 15 flag. */
};

/*! @brief SEMA4 reset finite state machine. */
enum _sema4_reset_state
{
    sema4ResetIdle     = 0U, /*!< Idle, waiting for the first data pattern write. */
    sema4ResetMid      = 1U, /*!< Waiting for the second data pattern write.      */
    sema4ResetFinished = 2U, /*!< Reset completed. Software can't get this state. */
};

/*! @brief SEMA4 status return codes. */
typedef enum _sema4_status
{
    statusSema4Success = 0U, /*!< Success.                                       */
    statusSema4Busy    = 1U, /*!< SEMA4 gate has been locked by other processor. */
} sema4_status_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name SEMA4 State Control
 * @{
 */

/*!
 * @brief Lock SEMA4 gate for exclusive access between multicore.
 *
 * @param base SEMA4 base pointer.
 * @param gateIndex SEMA4 gate index.
 * @retval statusSema4Success Lock the gate successfully.
 * @retval statusSema4Busy    SEMA4 gate has been locked by other processor.
 */
sema4_status_t SEMA4_TryLock(SEMA4_Type *base, uint32_t gateIndex);

/*!
 * @brief Lock SEMA4 gate for exclusive access between multicore, polling until success.
 *
 * @param base SEMA4 base pointer.
 * @param gateIndex SEMA4 gate index.
 */
void SEMA4_Lock(SEMA4_Type *base, uint32_t gateIndex);

/*!
 * @brief Unlock SEMA4 gate.
 *
 * @param base SEMA4 base pointer.
 * @param gateIndex SEMA4 gate index.
 */
void SEMA4_Unlock(SEMA4_Type *base, uint32_t gateIndex);

/*!
 * @brief Get processor number which locks the SEMA4 gate.
 *
 * @param base SEMA4 base pointer.
 * @param gateIndex SEMA4 gate index.
 * @return processor number which locks the SEMA4 gate, or SEMA4_PROCESSOR_NONE
 *         to indicate the gate is not locked.
 */
uint32_t SEMA4_GetLockProcessor(SEMA4_Type *base, uint32_t gateIndex);

/*@}*/

/*!
 * @name SEMA4 Reset Control
 * @{
 */

/*!
 * @brief Reset SEMA4 gate to unlocked status.
 *
 * @param base SEMA4 base pointer.
 * @param gateIndex SEMA4 gate index.
 */
void SEMA4_ResetGate(SEMA4_Type *base, uint32_t gateIndex);

/*!
 * @brief Reset all SEMA4 gates to unlocked status.
 *
 * @param base SEMA4 base pointer.
 */
void SEMA4_ResetAllGates(SEMA4_Type *base);

/*!
 * @brief Get bus master number which performing the gate reset function.
 *        This function gets the bus master number which performing the
 *        gate reset function.
 *
 * @param base SEMA4 base pointer.
 * @return Bus master number.
 */
static inline uint8_t SEMA4_GetGateResetBus(SEMA4_Type *base)
{
    return (uint8_t)(base->RSTGT & 7);
}

/*!
 * @brief Get sema4 gate reset state.
 *        This function gets current state of the sema4 reset gate finite
 *        state machine.
 *
 * @param base SEMA4 base pointer.
 * @return Current state (see @ref _sema4_reset_state).
 */
static inline uint8_t SEMA4_GetGateResetState(SEMA4_Type *base)
{
    return (uint8_t)((base->RSTGT & 0x30) >> 4);
}

/*!
 * @brief Reset SEMA4 IRQ notification.
 *
 * @param base SEMA4 base pointer.
 * @param gateIndex SEMA4 gate index.
 */
void SEMA4_ResetNotification(SEMA4_Type *base, uint32_t gateIndex);

/*!
 * @brief Reset all IRQ notifications.
 *
 * @param base SEMA4 base pointer.
 */
void SEMA4_ResetAllNotifications(SEMA4_Type *base);

/*!
 * @brief Get bus master number which performing the notification reset function.
 *        This function gets the bus master number which performing the notification
 *        reset function.
 *
 * @param base SEMA4 base pointer.
 * @return Bus master number.
 */
static inline uint8_t SEMA4_GetNotificationResetBus(SEMA4_Type *base)
{
    return (uint8_t)(base->RSTNTF & 7);
}

/*!
 * @brief Get sema4 notification reset state.
 *
 * This function gets current state of the sema4 reset notification finite state machine.
 *
 * @param base SEMA4 base pointer.
 * @return Current state (See @ref _sema4_reset_state).
 */
static inline uint8_t SEMA4_GetNotificationResetState(SEMA4_Type *base)
{
    return (uint8_t)((base->RSTNTF & 0x30) >> 4);
}

/*@}*/

/*!
 * @name SEMA4 Interrupt and Status Control
 * @{
 */

/*!
 * @brief Get SEMA4 notification status.
 *
 * @param base SEMA4 base pointer.
 * @param flags SEMA4 gate status mask (See @ref _sema4_status_flag).
 * @return SEMA4 notification status bits. If bit value is set, the corresponding
 *         gate's notification is available.
 */
static inline uint16_t SEMA4_GetStatusFlag(SEMA4_Type * base, uint16_t flags)
{
    return base->CPnNTF[SEMA4_PROCESSOR_SELF].NTF & flags;
}

/*!
 * @brief Enable or disable SEMA4 IRQ notification.
 *
 * @param base SEMA4 base pointer.
 * @param intMask SEMA4 gate status mask (see @ref _sema4_status_flag).
 * @param enable Enable/Disable Sema4 interrupt, only those gates whose intMask is set are affected.
 *               - true: Enable Sema4 interrupt.
 *               - false: Disable Sema4 interrupt.
 */
void SEMA4_SetIntCmd(SEMA4_Type * base, uint16_t intMask, bool enable);

/*!
 * @brief check whether SEMA4 IRQ notification enabled.
 *
 * @param base SEMA4 base pointer.
 * @param flags SEMA4 gate status mask (see @ref _sema4_status_flag).
 * @return SEMA4 notification interrupt enable status bits. If bit value is set,
 *         the corresponding gate's notification is enabled
 */
static inline uint16_t SEMA4_GetIntEnabled(SEMA4_Type * base, uint16_t flags)
{
    return base->CPnINE[SEMA4_PROCESSOR_SELF].INE & flags;
}

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __SEMA4_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
