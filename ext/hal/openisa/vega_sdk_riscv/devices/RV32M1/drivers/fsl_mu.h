/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_MU_H_
#define _FSL_MU_H_

#include "fsl_common.h"

/*!
 * @addtogroup mu
 * @{
 */

/******************************************************************************
 * Definitions
 *****************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief MU driver version 2.0.1. */
#define FSL_MU_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*!
 * @brief MU status flags.
 */
enum _mu_status_flags
{
    kMU_Tx0EmptyFlag = (1U << (MU_SR_TEn_SHIFT + 3U)), /*!< TX0 empty. */
    kMU_Tx1EmptyFlag = (1U << (MU_SR_TEn_SHIFT + 2U)), /*!< TX1 empty. */
    kMU_Tx2EmptyFlag = (1U << (MU_SR_TEn_SHIFT + 1U)), /*!< TX2 empty. */
    kMU_Tx3EmptyFlag = (1U << (MU_SR_TEn_SHIFT + 0U)), /*!< TX3 empty. */

    kMU_Rx0FullFlag = (1U << (MU_SR_RFn_SHIFT + 3U)), /*!< RX0 full.  */
    kMU_Rx1FullFlag = (1U << (MU_SR_RFn_SHIFT + 2U)), /*!< RX1 full.  */
    kMU_Rx2FullFlag = (1U << (MU_SR_RFn_SHIFT + 1U)), /*!< RX2 full.  */
    kMU_Rx3FullFlag = (1U << (MU_SR_RFn_SHIFT + 0U)), /*!< RX3 full.  */

    kMU_GenInt0Flag = (1U << (MU_SR_GIPn_SHIFT + 3U)), /*!< General purpose interrupt 0 pending. */
    kMU_GenInt1Flag = (1U << (MU_SR_GIPn_SHIFT + 2U)), /*!< General purpose interrupt 0 pending. */
    kMU_GenInt2Flag = (1U << (MU_SR_GIPn_SHIFT + 1U)), /*!< General purpose interrupt 0 pending. */
    kMU_GenInt3Flag = (1U << (MU_SR_GIPn_SHIFT + 0U)), /*!< General purpose interrupt 0 pending. */

    kMU_EventPendingFlag = MU_SR_EP_MASK,   /*!< MU event pending.               */
    kMU_FlagsUpdatingFlag = MU_SR_FUP_MASK, /*!< MU flags update is on-going.    */

#if (defined(FSL_FEATURE_MU_HAS_RESET_INT) && FSL_FEATURE_MU_HAS_RESET_INT)
    kMU_ResetAssertInterruptFlag = MU_SR_RAIP_MASK,   /*!< The other core reset assert interrupt pending.    */
    kMU_ResetDeassertInterruptFlag = MU_SR_RDIP_MASK, /*!< The other core reset de-assert interrupt pending. */
#endif

#if (defined(FSL_FEATURE_MU_HAS_SR_RS) && FSL_FEATURE_MU_HAS_SR_RS)
    kMU_OtherSideInResetFlag = MU_SR_RS_MASK /*!< The other side is in reset. */
#endif

#if (defined(FSL_FEATURE_MU_HAS_SR_MURIP) && FSL_FEATURE_MU_HAS_SR_MURIP)
    kMU_MuResetInterruptFlag = MU_SR_MURIP_MASK, /*!< The other side initializes MU reset. */
#endif
#if (defined(FSL_FEATURE_MU_HAS_SR_HRIP) && FSL_FEATURE_MU_HAS_SR_HRIP)
    kMU_HardwareResetInterruptFlag = MU_SR_HRIP_MASK, /*!< Current side has been hardware reset by the other side. */
#endif
};

/*!
 * @brief MU interrupt source to enable.
 */
enum _mu_interrupt_enable
{
    kMU_Tx0EmptyInterruptEnable = (1U << (MU_CR_TIEn_SHIFT + 3U)), /*!< TX0 empty. */
    kMU_Tx1EmptyInterruptEnable = (1U << (MU_CR_TIEn_SHIFT + 2U)), /*!< TX1 empty. */
    kMU_Tx2EmptyInterruptEnable = (1U << (MU_CR_TIEn_SHIFT + 1U)), /*!< TX2 empty. */
    kMU_Tx3EmptyInterruptEnable = (1U << (MU_CR_TIEn_SHIFT + 0U)), /*!< TX3 empty. */

    kMU_Rx0FullInterruptEnable = (1U << (MU_CR_RIEn_SHIFT + 3U)), /*!< RX0 full.  */
    kMU_Rx1FullInterruptEnable = (1U << (MU_CR_RIEn_SHIFT + 2U)), /*!< RX1 full.  */
    kMU_Rx2FullInterruptEnable = (1U << (MU_CR_RIEn_SHIFT + 1U)), /*!< RX2 full.  */
    kMU_Rx3FullInterruptEnable = (1U << (MU_CR_RIEn_SHIFT + 0U)), /*!< RX3 full.  */

    kMU_GenInt0InterruptEnable = (1U << (MU_CR_GIEn_SHIFT + 3U)), /*!< General purpose interrupt 0. */
    kMU_GenInt1InterruptEnable = (1U << (MU_CR_GIEn_SHIFT + 2U)), /*!< General purpose interrupt 1. */
    kMU_GenInt2InterruptEnable = (1U << (MU_CR_GIEn_SHIFT + 1U)), /*!< General purpose interrupt 2. */
    kMU_GenInt3InterruptEnable = (1U << (MU_CR_GIEn_SHIFT + 0U)), /*!< General purpose interrupt 3. */

#if (defined(FSL_FEATURE_MU_HAS_RESET_INT) && FSL_FEATURE_MU_HAS_RESET_INT)
    kMU_ResetAssertInterruptEnable = MU_CR_RAIE_MASK,   /*!< The other core reset assert interrupt.    */
    kMU_ResetDeassertInterruptEnable = MU_CR_RDIE_MASK, /*!< The other core reset de-assert interrupt. */
#endif
#if (defined(FSL_FEATURE_MU_HAS_SR_MURIP) && FSL_FEATURE_MU_HAS_SR_MURIP)
    kMU_MuResetInterruptEnable = MU_CR_MURIE_MASK, /*!< The other side initializes MU reset. The interrupt
                                                     is ORed with the general purpose interrupt 3. The
                                                     general purpose interrupt 3 is issued when the other side
                                                     set the MU reset and this interrupt is enabled. */
#endif
#if (defined(FSL_FEATURE_MU_HAS_SR_HRIP) && FSL_FEATURE_MU_HAS_SR_HRIP)
    kMU_HardwareResetInterruptEnable = MU_CR_HRIE_MASK, /*!< Current side has been hardware reset by the other side. */
#endif
};

/*!
 * @brief MU interrupt that could be triggered to the other core.
 */
enum _mu_interrupt_trigger
{
    kMU_NmiInterruptTrigger = MU_CR_NMI_MASK,                      /*!< NMI interrupt.               */
    kMU_GenInt0InterruptTrigger = (1U << (MU_CR_GIRn_SHIFT + 3U)), /*!< General purpose interrupt 0. */
    kMU_GenInt1InterruptTrigger = (1U << (MU_CR_GIRn_SHIFT + 2U)), /*!< General purpose interrupt 1. */
    kMU_GenInt2InterruptTrigger = (1U << (MU_CR_GIRn_SHIFT + 1U)), /*!< General purpose interrupt 2. */
    kMU_GenInt3InterruptTrigger = (1U << (MU_CR_GIRn_SHIFT + 0U))  /*!< General purpose interrupt 3. */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name MU initialization.
 * @{
 */
/*!
 * @brief Initializes the MU module.
 *
 * This function enables the MU clock only.
 *
 * @param base MU peripheral base address.
 */
void MU_Init(MU_Type *base);

/*!
 * @brief De-initializes the MU module.
 *
 * This function disables the MU clock only.
 *
 * @param base MU peripheral base address.
 */
void MU_Deinit(MU_Type *base);

/* @} */

/*!
 * @name MU Message
 * @{
 */

/*!
 * @brief Writes a message to the TX register.
 *
 * This function writes a message to the specific TX register. It does not check
 * whether the TX register is empty or not. The upper layer should make sure the TX
 * register is empty before calling this function. This function can be used
 * in ISR for better performance.
 *
 * @code
 * while (!(kMU_Tx0EmptyFlag & MU_GetStatusFlags(base))) { } // Wait for TX0 register empty.
 * MU_SendMsgNonBlocking(base, 0U, MSG_VAL); // Write message to the TX0 register.
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param regIndex  TX register index.
 * @param msg      Message to send.
 */
static inline void MU_SendMsgNonBlocking(MU_Type *base, uint32_t regIndex, uint32_t msg)
{
    assert(regIndex < MU_TR_COUNT);

    base->TR[regIndex] = msg;
}

/*!
 * @brief Blocks to send a message.
 *
 * This function waits until the TX register is empty and sends the message.
 *
 * @param base MU peripheral base address.
 * @param regIndex  TX register index.
 * @param msg      Message to send.
 */
void MU_SendMsg(MU_Type *base, uint32_t regIndex, uint32_t msg);

/*!
 * @brief Reads a message from the RX register.
 *
 * This function reads a message from the specific RX register. It does not check
 * whether the RX register is full or not. The upper layer should make sure the RX
 * register is full before calling this function. This function can be used
 * in ISR for better performance.
 *
 * @code
 * uint32_t msg;
 * while (!(kMU_Rx0FullFlag & MU_GetStatusFlags(base)))
 * {
 * } // Wait for the RX0 register full.
 *
 * msg = MU_ReceiveMsgNonBlocking(base, 0U);  // Read message from RX0 register.
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param regIndex  TX register index.
 * @return The received message.
 */
static inline uint32_t MU_ReceiveMsgNonBlocking(MU_Type *base, uint32_t regIndex)
{
    assert(regIndex < MU_TR_COUNT);

    return base->RR[regIndex];
}

/*!
 * @brief Blocks to receive a message.
 *
 * This function waits until the RX register is full and receives the message.
 *
 * @param base MU peripheral base address.
 * @param regIndex  RX register index.
 * @return The received message.
 */
uint32_t MU_ReceiveMsg(MU_Type *base, uint32_t regIndex);

/* @} */

/*!
 * @name MU Flags
 * @{
 */

/*!
 * @brief Sets the 3-bit MU flags reflect on the other MU side.
 *
 * This function sets the 3-bit MU flags directly. Every time the 3-bit MU flags are changed,
 * the status flag \c kMU_FlagsUpdatingFlag asserts indicating the 3-bit MU flags are
 * updating to the other side. After the 3-bit MU flags are updated, the status flag
 * \c kMU_FlagsUpdatingFlag is cleared by hardware. During the flags updating period,
 * the flags cannot be changed. The upper layer should make sure the status flag
 * \c kMU_FlagsUpdatingFlag is cleared before calling this function.
 *
 * @code
 * while (kMU_FlagsUpdatingFlag & MU_GetStatusFlags(base))
 * {
 * } // Wait for previous MU flags updating.
 *
 * MU_SetFlagsNonBlocking(base, 0U); // Set the mU flags.
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param flags The 3-bit MU flags to set.
 */
static inline void MU_SetFlagsNonBlocking(MU_Type *base, uint32_t flags)
{
    uint32_t reg = base->CR;
    reg = (reg & ~((MU_CR_GIRn_MASK | MU_CR_NMI_MASK) | MU_CR_Fn_MASK)) | MU_CR_Fn(flags);
    base->CR = reg;
}

/*!
 * @brief Blocks setting the 3-bit MU flags reflect on the other MU side.
 *
 * This function blocks setting the 3-bit MU flags. Every time the 3-bit MU flags are changed,
 * the status flag \c kMU_FlagsUpdatingFlag asserts indicating the 3-bit MU flags are
 * updating to the other side. After the 3-bit MU flags are updated, the status flag
 * \c kMU_FlagsUpdatingFlag is cleared by hardware. During the flags updating period,
 * the flags cannot be changed. This function waits for the MU status flag
 * \c kMU_FlagsUpdatingFlag cleared and sets the 3-bit MU flags.
 *
 * @param base MU peripheral base address.
 * @param flags The 3-bit MU flags to set.
 */
void MU_SetFlags(MU_Type *base, uint32_t flags);

/*!
 * @brief Gets the current value of the 3-bit MU flags set by the other side.
 *
 * This functions gets the current 3-bit MU flags on the current side.
 *
 * @param base MU peripheral base address.
 * @return flags   Current value of the 3-bit flags.
 */
static inline uint32_t MU_GetFlags(MU_Type *base)
{
    return (base->SR & MU_SR_Fn_MASK) >> MU_SR_Fn_SHIFT;
}

/* @} */

/*!
 * @name Status and Interrupt.
 * @{
 */

/*!
 * @brief Gets the MU status flags.
 *
 * This function returns the bit mask of the MU status flags. See _mu_status_flags.
 *
 * @code
 * uint32_t flags;
 * flags = MU_GetStatusFlags(base); // Get all status flags.
 * if (kMU_Tx0EmptyFlag & flags)
 * {
 *     // The TX0 register is empty. Message can be sent.
 *     MU_SendMsgNonBlocking(base, 0U, MSG0_VAL);
 * }
 * if (kMU_Tx1EmptyFlag & flags)
 * {
 *     // The TX1 register is empty. Message can be sent.
 *     MU_SendMsgNonBlocking(base, 1U, MSG1_VAL);
 * }
 * @endcode
 *
 * @param base MU peripheral base address.
 * @return      Bit mask of the MU status flags, see _mu_status_flags.
 */
static inline uint32_t MU_GetStatusFlags(MU_Type *base)
{
    return (base->SR & (MU_SR_TEn_MASK | MU_SR_RFn_MASK | MU_SR_GIPn_MASK | MU_SR_EP_MASK | MU_SR_FUP_MASK
#if (defined(FSL_FEATURE_MU_HAS_SR_RS) && FSL_FEATURE_MU_HAS_SR_RS)
                | MU_SR_RS_MASK
#endif
#if (defined(FSL_FEATURE_MU_HAS_RESET_INT) && FSL_FEATURE_MU_HAS_RESET_INT)
                | MU_SR_RDIP_MASK | MU_SR_RAIP_MASK
#endif
#if (defined(FSL_FEATURE_MU_HAS_SR_MURIP) && FSL_FEATURE_MU_HAS_SR_MURIP)
                | MU_SR_MURIP_MASK
#endif
#if (defined(FSL_FEATURE_MU_HAS_SR_HRIP) && FSL_FEATURE_MU_HAS_SR_HRIP)
                | MU_SR_HRIP_MASK
#endif
                ));
}

/*!
 * @brief Clears the specific MU status flags.
 *
 * This function clears the specific MU status flags. The flags to clear should
 * be passed in as bit mask. See _mu_status_flags.
 *
 * @code
 * //Clear general interrupt 0 and general interrupt 1 pending flags.
 * MU_ClearStatusFlags(base, kMU_GenInt0Flag | kMU_GenInt1Flag);
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param mask  Bit mask of the MU status flags. See _mu_status_flags. The following
 *              flags are cleared by hardware, this function could not clear them.
 *                - kMU_Tx0EmptyFlag
 *                - kMU_Tx1EmptyFlag
 *                - kMU_Tx2EmptyFlag
 *                - kMU_Tx3EmptyFlag
 *                - kMU_Rx0FullFlag
 *                - kMU_Rx1FullFlag
 *                - kMU_Rx2FullFlag
 *                - kMU_Rx3FullFlag
 *                - kMU_EventPendingFlag
 *                - kMU_FlagsUpdatingFlag
 *                - kMU_OtherSideInResetFlag
 */
static inline void MU_ClearStatusFlags(MU_Type *base, uint32_t mask)
{
    /* regMask is the mask of w1c status bits. */
    uint32_t regMask = MU_SR_GIPn_MASK;

#if (defined(FSL_FEATURE_MU_HAS_RESET_INT) && FSL_FEATURE_MU_HAS_RESET_INT)
    regMask |= (MU_SR_RDIP_MASK | MU_SR_RAIP_MASK);
#endif

#if (defined(FSL_FEATURE_MU_HAS_SR_MURIP) && FSL_FEATURE_MU_HAS_SR_MURIP)
    regMask |= MU_SR_MURIP_MASK;
#endif

#if (defined(FSL_FEATURE_MU_HAS_SR_HRIP) && FSL_FEATURE_MU_HAS_SR_HRIP)
    regMask |= MU_SR_HRIP_MASK;
#endif

    base->SR = (mask & regMask);
}

/*!
 * @brief Enables the specific MU interrupts.
 *
 * This function enables the specific MU interrupts. The interrupts to enable
 * should be passed in as bit mask. See _mu_interrupt_enable.
 *
 * @code
 * // Enable general interrupt 0 and TX0 empty interrupt.
 * MU_EnableInterrupts(base, kMU_GenInt0InterruptEnable | kMU_Tx0EmptyInterruptEnable);
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param mask  Bit mask of the MU interrupts. See _mu_interrupt_enable.
 */
static inline void MU_EnableInterrupts(MU_Type *base, uint32_t mask)
{
    uint32_t reg = base->CR;
    reg = (reg & ~(MU_CR_GIRn_MASK | MU_CR_NMI_MASK)) | mask;
    base->CR = reg;
}

/*!
 * @brief Disables the specific MU interrupts.
 *
 * This function disables the specific MU interrupts. The interrupts to disable
 * should be passed in as bit mask. See _mu_interrupt_enable.
 *
 * @code
 * // Disable general interrupt 0 and TX0 empty interrupt.
 * MU_DisableInterrupts(base, kMU_GenInt0InterruptEnable | kMU_Tx0EmptyInterruptEnable);
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param mask  Bit mask of the MU interrupts. See _mu_interrupt_enable.
 */
static inline void MU_DisableInterrupts(MU_Type *base, uint32_t mask)
{
    uint32_t reg = base->CR;
    reg &= ~((MU_CR_GIRn_MASK | MU_CR_NMI_MASK) | mask);
    base->CR = reg;
}

/*!
 * @brief Triggers interrupts to the other core.
 *
 * This function triggers the specific interrupts to the other core. The interrupts
 * to trigger are passed in as bit mask. See \ref _mu_interrupt_trigger.
 * The MU should not trigger an interrupt to the other core when the previous interrupt
 * has not been processed by the other core. This function checks whether the
 * previous interrupts have been processed. If not, it returns an error.
 *
 * @code
 * if (kStatus_Success != MU_TriggerInterrupts(base, kMU_GenInt0InterruptTrigger | kMU_GenInt2InterruptTrigger))
 * {
 *     // Previous general purpose interrupt 0 or general purpose interrupt 2
 *     // has not been processed by the other core.
 * }
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param mask Bit mask of the interrupts to trigger. See _mu_interrupt_trigger.
 * @retval kStatus_Success    Interrupts have been triggered successfully.
 * @retval kStatus_Fail       Previous interrupts have not been accepted.
 */
status_t MU_TriggerInterrupts(MU_Type *base, uint32_t mask);

/*!
 * @brief Clear non-maskable interrupt (NMI) sent by the other core.
 *
 * This functions clears non-maskable interrupt (NMI) sent by the other core.
 *
 * @param base MU peripheral base address.
 */
static inline void MU_ClearNmi(MU_Type *base)
{
    base->SR = MU_SR_NMIC_MASK;
}

/* @} */

/*!
 * @name MU misc functions
 * @{
 */

/*!
 * @brief Boots the core at B side.
 *
 * This function sets the B side core's boot configuration and releases the
 * core from reset.
 *
 * @param base MU peripheral base address.
 * @param mode Core B boot mode.
 * @note Only MU side A can use this function.
 */
void MU_BootCoreB(MU_Type *base, mu_core_boot_mode_t mode);

/*!
 * @brief Holds the core reset of B side.
 *
 * This function causes the core of B side to be held in reset following any reset event.
 *
 * @param base MU peripheral base address.
 * @note Only A side could call this function.
 */
static inline void MU_HoldCoreBReset(MU_Type *base)
{
#if (defined(FSL_FEATURE_MU_HAS_CCR) && FSL_FEATURE_MU_HAS_CCR)
    base->CCR |= MU_CCR_RSTH_MASK;
#else /* FSL_FEATURE_MU_HAS_CCR */
    uint32_t reg = base->CR;
    reg = (reg & ~(MU_CR_GIRn_MASK | MU_CR_NMI_MASK)) | MU_CR_RSTH_MASK;
    base->CR = reg;
#endif /* FSL_FEATURE_MU_HAS_CCR */
}

/*!
 * @brief Boots the other core.
 *
 * This function boots the other core with a boot configuration.
 *
 * @param base MU peripheral base address.
 * @param mode The other core boot mode.
 */
void MU_BootOtherCore(MU_Type *base, mu_core_boot_mode_t mode);

/*!
 * @brief Holds the other core reset.
 *
 * This function causes the other core to be held in reset following any reset event.
 *
 * @param base MU peripheral base address.
 */
static inline void MU_HoldOtherCoreReset(MU_Type *base)
{
    /*
     * MU_HoldOtherCoreReset and MU_HoldCoreBReset are the same, MU_HoldCoreBReset
     * is kept for compatible with older platforms.
     */
    MU_HoldCoreBReset(base);
}

/*!
 * @brief Resets the MU for both A side and B side.
 *
 * This function resets the MU for both A side and B side. Before reset, it is
 * recommended to interrupt processor B, because this function may affect the
 * ongoing processor B programs.
 *
 * @param base MU peripheral base address.
 * @note For some platforms, only MU side A could use this function, check
 * reference manual for details.
 */
static inline void MU_ResetBothSides(MU_Type *base)
{
    uint32_t reg = base->CR;
    reg = (reg & ~(MU_CR_GIRn_MASK | MU_CR_NMI_MASK)) | MU_CR_MUR_MASK;
    base->CR = reg;

#if (defined(FSL_FEATURE_MU_HAS_SR_RS) && FSL_FEATURE_MU_HAS_SR_RS)
    /* Wait for the other side out of reset. */
    while (base->SR & MU_SR_RS_MASK)
    {
    }
#endif /* FSL_FEATURE_MU_HAS_SR_RS */
}

#if (defined(FSL_FEATURE_MU_HAS_CCR) && FSL_FEATURE_MU_HAS_CCR)
/*!
 * @brief Mask hardware reset by the other core.
 *
 * The other core could call MU_HardwareResetOtherCore() to reset current core.
 * To mask the reset, call this function and pass in true.
 *
 * @param base MU peripheral base address.
 * @param mask Pass true to mask the hardware reset, pass false to unmask it.
 */
static inline void MU_MaskHardwareReset(MU_Type *base, bool mask)
{
    if (mask)
    {
        base->CCR |= MU_CCR_HRM_MASK;
    }
    else
    {
        base->CCR &= ~MU_CCR_HRM_MASK;
    }
}
#endif

/*!
 * @brief Hardware reset the other core.
 *
 * This function resets the other core, the other core could mask the
 * hardware reset by calling @ref MU_MaskHardwareReset. The hardware reset
 * mask feature is only available for some platforms.
 * This function could be used together with MU_BootOtherCore to control the
 * other core reset workflow.
 *
 * Example 1: Reset the other core, and no hold reset
 * @code
 * MU_HardwareResetOtherCore(MU_A, true, false, bootMode);
 * @endcode
 * In this example, the core at MU side B will reset with the specified boot mode.
 *
 * Example 2: Reset the other core and hold it, then boot the other core later.
 * @code
 * // Here the other core enters reset, and the reset is hold
 * MU_HardwareResetOtherCore(MU_A, true, true, modeDontCare);
 * // Current core boot the other core when necessary.
 * MU_BootOtherCore(MU_A, bootMode);
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param waitReset Wait the other core enters reset.
 *                    - true: Wait until the other core enters reset, if the other
 *                      core has masked the hardware reset, then this function will
 *                      be blocked.
 *                    - false: Don't wait the reset.
 * @param holdReset Hold the other core reset or not.
 *                    - true: Hold the other core in reset, this function returns
 *                      directly when the other core enters reset.
 *                    - false: Don't hold the other core in reset, this function
 *                      waits until the other core out of reset.
 * @param bootMode Boot mode of the other core, if @p holdReset is true, this
 *                 parameter is useless.
 */
void MU_HardwareResetOtherCore(MU_Type *base, bool waitReset, bool holdReset, mu_core_boot_mode_t bootMode);

/*!
 * @brief Enables or disables the clock on the other core.
 *
 * This function enables or disables the platform clock on the other core when
 * that core enters a stop mode. If disabled, the platform clock for the other
 * core is disabled when it enters stop mode. If enabled, the platform clock
 * keeps running on the other core in stop mode, until this core also enters
 * stop mode.
 *
 * @param base MU peripheral base address.
 * @param enable   Enable or disable the clock on the other core.
 */
static inline void MU_SetClockOnOtherCoreEnable(MU_Type *base, bool enable)
{
#if (defined(FSL_FEATURE_MU_HAS_CCR) && FSL_FEATURE_MU_HAS_CCR)
    if (enable)
    {
        base->CCR |= MU_CCR_CLKE_MASK;
    }
    else
    {
        base->CCR &= ~MU_CCR_CLKE_MASK;
    }
#else /* FSL_FEATURE_MU_HAS_CCR */
    uint32_t reg = base->CR;

    reg &= ~(MU_CR_GIRn_MASK | MU_CR_NMI_MASK);

    if (enable)
    {
        reg |= MU_CR_CLKE_MASK;
    }
    else
    {
        reg &= ~MU_CR_CLKE_MASK;
    }

    base->CR = reg;
#endif /* FSL_FEATURE_MU_HAS_CCR */
}

/*!
 * @brief Gets the power mode of the other core.
 *
 * This function gets the power mode of the other core.
 *
 * @param base MU peripheral base address.
 * @return Power mode of the other core.
 */
static inline mu_power_mode_t MU_GetOtherCorePowerMode(MU_Type *base)
{
    return (mu_power_mode_t)((base->SR & MU_SR_PM_MASK) >> MU_SR_PM_SHIFT);
}

/* @} */

#if defined(__cplusplus)
}
#endif /*_cplusplus*/
/*@}*/

#endif /* _FSL_MU_H_*/
