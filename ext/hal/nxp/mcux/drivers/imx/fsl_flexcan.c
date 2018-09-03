/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flexcan.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flexcan"
#endif


/*! @brief FlexCAN Internal State. */
enum _flexcan_state
{
    kFLEXCAN_StateIdle = 0x0,     /*!< MB/RxFIFO idle.*/
    kFLEXCAN_StateRxData = 0x1,   /*!< MB receiving.*/
    kFLEXCAN_StateRxRemote = 0x2, /*!< MB receiving remote reply.*/
    kFLEXCAN_StateTxData = 0x3,   /*!< MB transmitting.*/
    kFLEXCAN_StateTxRemote = 0x4, /*!< MB transmitting remote request.*/
    kFLEXCAN_StateRxFifo = 0x5,   /*!< RxFIFO receiving.*/
};

/*! @brief FlexCAN message buffer CODE for Rx buffers. */
enum _flexcan_mb_code_rx
{
    kFLEXCAN_RxMbInactive = 0x0, /*!< MB is not active.*/
    kFLEXCAN_RxMbFull = 0x2,     /*!< MB is full.*/
    kFLEXCAN_RxMbEmpty = 0x4,    /*!< MB is active and empty.*/
    kFLEXCAN_RxMbOverrun = 0x6,  /*!< MB is overwritten into a full buffer.*/
    kFLEXCAN_RxMbBusy = 0x8,     /*!< FlexCAN is updating the contents of the MB.*/
                                 /*!  The CPU must not access the MB.*/
    kFLEXCAN_RxMbRanswer = 0xA,  /*!< A frame was configured to recognize a Remote Request Frame */
                                 /*!  and transmit a Response Frame in return.*/
    kFLEXCAN_RxMbNotUsed = 0xF,  /*!< Not used.*/
};

/*! @brief FlexCAN message buffer CODE FOR Tx buffers. */
enum _flexcan_mb_code_tx
{
    kFLEXCAN_TxMbInactive = 0x8,     /*!< MB is not active.*/
    kFLEXCAN_TxMbAbort = 0x9,        /*!< MB is aborted.*/
    kFLEXCAN_TxMbDataOrRemote = 0xC, /*!< MB is a TX Data Frame(when MB RTR = 0) or */
                                     /*!< MB is a TX Remote Request Frame (when MB RTR = 1).*/
    kFLEXCAN_TxMbTanswer = 0xE,      /*!< MB is a TX Response Request Frame from */
                                     /*!  an incoming Remote Request Frame.*/
    kFLEXCAN_TxMbNotUsed = 0xF,      /*!< Not used.*/
};

/* Typedef for interrupt handler. */
typedef void (*flexcan_isr_t)(CAN_Type *base, flexcan_handle_t *handle);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Enter FlexCAN Freeze Mode.
 *
 * This function makes the FlexCAN work under Freeze Mode.
 *
 * @param base FlexCAN peripheral base address.
 */
static void FLEXCAN_EnterFreezeMode(CAN_Type *base);

/*!
 * @brief Exit FlexCAN Freeze Mode.
 *
 * This function makes the FlexCAN leave Freeze Mode.
 *
 * @param base FlexCAN peripheral base address.
 */
static void FLEXCAN_ExitFreezeMode(CAN_Type *base);

#if !defined(NDEBUG)
/*!
 * @brief Check if Message Buffer is occupied by Rx FIFO.
 *
 * This function check if Message Buffer is occupied by Rx FIFO.
 *
 * @param base FlexCAN peripheral base address.
 * @param mbIdx The FlexCAN Message Buffer index.
 */
static bool FLEXCAN_IsMbOccupied(CAN_Type *base, uint8_t mbIdx);
#endif

#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
/*!
 * @brief Get the first valid Message buffer ID of give FlexCAN instance.
 *
 * This function is a helper function for Errata 5641 workaround.
 *
 * @param base FlexCAN peripheral base address.
 * @return The first valid Message Buffer Number.
 */
static uint32_t FLEXCAN_GetFirstValidMb(CAN_Type *base);
#endif

/*!
 * @brief Check if Message Buffer interrupt is enabled.
 *
 * This function check if Message Buffer interrupt is enabled.
 *
 * @param base FlexCAN peripheral base address.
 * @param mbIdx The FlexCAN Message Buffer index.
 */
static bool FLEXCAN_IsMbIntEnabled(CAN_Type *base, uint8_t mbIdx);

/*!
 * @brief Reset the FlexCAN Instance.
 *
 * Restores the FlexCAN module to reset state, notice that this function
 * will set all the registers to reset state so the FlexCAN module can not work
 * after calling this API.
 *
 * @param base FlexCAN peripheral base address.
*/
static void FLEXCAN_Reset(CAN_Type *base);

/*!
 * @brief Set Baud Rate of FlexCAN.
 *
 * This function set the baud rate of FlexCAN.
 *
 * @param base FlexCAN peripheral base address.
 * @param sourceClock_Hz Source Clock in Hz.
 * @param baudRate_Bps Baud Rate in Bps.
 * @param timingConfig FlexCAN timingConfig.
 */
static void FLEXCAN_SetBaudRate(CAN_Type *base,
                                uint32_t sourceClock_Hz,
                                uint32_t baudRate_Bps,
                                flexcan_timing_config_t timingConfig);

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
/*!
 * @brief Set Baud Rate of FlexCAN FD frame.
 *
 * This function set the baud rate of FlexCAN FD frame.
 *
 * @param base FlexCAN peripheral base address.
 * @param sourceClock_Hz Source Clock in Hz.
 * @param baudRateFD_Bps FD frame Baud Rate in Bps.
 * @param timingConfig FlexCAN timingConfig.
 */
static void FLEXCAN_SetFDBaudRate(CAN_Type *base, uint32_t sourceClock_Hz, uint32_t baudRateFD_Bps, flexcan_timing_config_t timingConfig);

/*!
 * @brief Get Mailbox offset number by dword.
 *
 * This function gets the offset number of the specified mailbox.
 * Mailbox is not consecutive between memory regions when payload is not 8 bytes
 * so need to calculate the specified mailbox address.
 * For example, in the first memory region, MB[0].CS address is 0x4002_4080. For 32 bytes
 * payload frame, the second mailbox is ((1/12)*512 + 1%12*40)/4 = 10, meaning 10 dword
 * after the 0x4002_4080, which is actually the address of mailbox MB[1].CS.
 *
 * @param base FlexCAN peripheral base address.
 * @param mbIdx Mailbox index.
 */
static uint32_t FLEXCAN_GetFDMailboxOffset(CAN_Type *base, uint8_t mbIdx);
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Array of FlexCAN peripheral base address. */
static CAN_Type *const s_flexcanBases[] = CAN_BASE_PTRS;

/* Array of FlexCAN IRQ number. */
static const IRQn_Type s_flexcanRxWarningIRQ[] = CAN_Rx_Warning_IRQS;
static const IRQn_Type s_flexcanTxWarningIRQ[] = CAN_Tx_Warning_IRQS;
static const IRQn_Type s_flexcanWakeUpIRQ[] = CAN_Wake_Up_IRQS;
static const IRQn_Type s_flexcanErrorIRQ[] = CAN_Error_IRQS;
static const IRQn_Type s_flexcanBusOffIRQ[] = CAN_Bus_Off_IRQS;
static const IRQn_Type s_flexcanMbIRQ[] = CAN_ORed_Message_buffer_IRQS;

/* Array of FlexCAN handle. */
static flexcan_handle_t *s_flexcanHandle[ARRAY_SIZE(s_flexcanBases)];

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of FlexCAN clock name. */
static const clock_ip_name_t s_flexcanClock[] = FLEXCAN_CLOCKS;
#if defined(FLEXCAN_PERIPH_CLOCKS)
/* Array of FlexCAN serial clock name. */
static const clock_ip_name_t s_flexcanPeriphClock[] = FLEXCAN_PERIPH_CLOCKS;
#endif /* FLEXCAN_PERIPH_CLOCKS */
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/* FlexCAN ISR for transactional APIs. */
static flexcan_isr_t s_flexcanIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t FLEXCAN_GetInstance(CAN_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_flexcanBases); instance++)
    {
        if (s_flexcanBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_flexcanBases));

    return instance;
}

static void FLEXCAN_EnterFreezeMode(CAN_Type *base)
{
    /* Set Freeze, Halt bits. */
    base->MCR |= CAN_MCR_FRZ_MASK;
    base->MCR |= CAN_MCR_HALT_MASK;

    /* Wait until the FlexCAN Module enter freeze mode. */
    while (!(base->MCR & CAN_MCR_FRZACK_MASK))
    {
    }
}

static void FLEXCAN_ExitFreezeMode(CAN_Type *base)
{
    /* Clear Freeze, Halt bits. */
    base->MCR &= ~CAN_MCR_HALT_MASK;
    base->MCR &= ~CAN_MCR_FRZ_MASK;

    /* Wait until the FlexCAN Module exit freeze mode. */
    while (base->MCR & CAN_MCR_FRZACK_MASK)
    {
    }
}

#if !defined(NDEBUG)
static bool FLEXCAN_IsMbOccupied(CAN_Type *base, uint8_t mbIdx)
{
    uint8_t lastOccupiedMb;

    /* Is Rx FIFO enabled? */
    if (base->MCR & CAN_MCR_RFEN_MASK)
    {
        /* Get RFFN value. */
        lastOccupiedMb = ((base->CTRL2 & CAN_CTRL2_RFFN_MASK) >> CAN_CTRL2_RFFN_SHIFT);
        /* Calculate the number of last Message Buffer occupied by Rx FIFO. */
        lastOccupiedMb = ((lastOccupiedMb + 1) * 2) + 5;

#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
        if (mbIdx <= (lastOccupiedMb + 1))
#else
        if (mbIdx <= lastOccupiedMb)
#endif
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
        if (0 == mbIdx)
        {
            return true;
        }
        else
        {
            return false;
        }
#else
        return false;
#endif
    }
}
#endif

#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
static uint32_t FLEXCAN_GetFirstValidMb(CAN_Type *base)
{
    uint32_t firstValidMbNum;

    if (base->MCR & CAN_MCR_RFEN_MASK)
    {
        firstValidMbNum = ((base->CTRL2 & CAN_CTRL2_RFFN_MASK) >> CAN_CTRL2_RFFN_SHIFT);
        firstValidMbNum = ((firstValidMbNum + 1) * 2) + 6;
    }
    else
    {
        firstValidMbNum = 0;
    }

    return firstValidMbNum;
}
#endif

static bool FLEXCAN_IsMbIntEnabled(CAN_Type *base, uint8_t mbIdx)
{
    /* Assertion. */
    assert(mbIdx < FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base));

#if (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    if (mbIdx < 32)
    {
#endif
        if (base->IMASK1 & ((uint32_t)(1 << mbIdx)))
        {
            return true;
        }
        else
        {
            return false;
        }
#if (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    }
    else
    {
        if (base->IMASK2 & ((uint32_t)(1 << (mbIdx - 32))))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
#endif
}

static void FLEXCAN_Reset(CAN_Type *base)
{
    /* The module must should be first exit from low power
     * mode, and then soft reset can be applied.
     */
    assert(!(base->MCR & CAN_MCR_MDIS_MASK));

    uint8_t i;

#if (defined(FSL_FEATURE_FLEXCAN_HAS_DOZE_MODE_SUPPORT) && FSL_FEATURE_FLEXCAN_HAS_DOZE_MODE_SUPPORT)
    if (FSL_FEATURE_FLEXCAN_INSTANCE_HAS_DOZE_MODE_SUPPORTn(base))
    {
        /* De-assert DOZE Enable Bit. */
        base->MCR &= ~CAN_MCR_DOZE_MASK;
    }
#endif

    /* Wait until FlexCAN exit from any Low Power Mode. */
    while (base->MCR & CAN_MCR_LPMACK_MASK)
    {
    }

    /* Assert Soft Reset Signal. */
    base->MCR |= CAN_MCR_SOFTRST_MASK;
    /* Wait until FlexCAN reset completes. */
    while (base->MCR & CAN_MCR_SOFTRST_MASK)
    {
    }

    /* Reset MCR register. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_GLITCH_FILTER) && FSL_FEATURE_FLEXCAN_HAS_GLITCH_FILTER)
    base->MCR |= CAN_MCR_WRNEN_MASK | CAN_MCR_WAKSRC_MASK |
                 CAN_MCR_MAXMB(FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base) - 1);
#else
    base->MCR |= CAN_MCR_WRNEN_MASK | CAN_MCR_MAXMB(FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base) - 1);
#endif

    /* Reset CTRL1 and CTRL2 register. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
    /* SMP bit cannot be asserted when CAN FD is enabled */
    if (FSL_FEATURE_FLEXCAN_INSTANCE_HAS_FLEXIBLE_DATA_RATEn(base))
    {
        base->CTRL1 = 0x0;
    }
    else
    {
        base->CTRL1 = CAN_CTRL1_SMP_MASK;
    }
#else
    base->CTRL1 = CAN_CTRL1_SMP_MASK;
#endif
    base->CTRL2 = CAN_CTRL2_TASD(0x16) | CAN_CTRL2_RRS_MASK | CAN_CTRL2_EACEN_MASK;

    /* Clean all individual Rx Mask of Message Buffers. */
    for (i = 0; i < FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base); i++)
    {
        base->RXIMR[i] = 0x3FFFFFFF;
    }

    /* Clean Global Mask of Message Buffers. */
    base->RXMGMASK = 0x3FFFFFFF;
    /* Clean Global Mask of Message Buffer 14. */
    base->RX14MASK = 0x3FFFFFFF;
    /* Clean Global Mask of Message Buffer 15. */
    base->RX15MASK = 0x3FFFFFFF;
    /* Clean Global Mask of Rx FIFO. */
    base->RXFGMASK = 0x3FFFFFFF;

    /* Clean all Message Buffer CS fields. */
    for (i = 0; i < FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base); i++)
    {
        base->MB[i].CS = 0x0;
    }
}

static void FLEXCAN_SetBaudRate(CAN_Type *base,
                                uint32_t sourceClock_Hz,
                                uint32_t baudRate_Bps,
                                flexcan_timing_config_t timingConfig)
{
    /* FlexCAN timing setting formula:
     * quantum = 1 + (PSEG1 + 1) + (PSEG2 + 1) + (PROPSEG + 1);
     */
    uint32_t quantum = 1 + (timingConfig.phaseSeg1 + 1) + (timingConfig.phaseSeg2 + 1) + (timingConfig.propSeg + 1);
    uint32_t priDiv = baudRate_Bps * quantum;

    /* Assertion: Desired baud rate is too high. */
    assert(baudRate_Bps <= 1000000U);
    /* Assertion: Source clock should greater than baud rate * quantum. */
    assert(priDiv <= sourceClock_Hz);

    if (0 == priDiv)
    {
        priDiv = 1;
    }

    priDiv = (sourceClock_Hz / priDiv) - 1;

    /* Desired baud rate is too low. */
    if (priDiv > 0xFF)
    {
        priDiv = 0xFF;
    }

    timingConfig.preDivider = priDiv;

    /* Update actual timing characteristic. */
    FLEXCAN_SetTimingConfig(base, &timingConfig);
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
static void FLEXCAN_SetFDBaudRate(CAN_Type *base,
                                  uint32_t sourceClock_Hz,
                                  uint32_t baudRateFD_Bps,
                                  flexcan_timing_config_t timingConfig)
{
    /* FlexCAN FD timing setting formula:
     * quantum = 1 + (FPSEG1 + 1) + (FPSEG2 + 1) + FPROPSEG;
     */
    uint32_t quantum = 1 + (timingConfig.fphaseSeg1 + 1) + (timingConfig.fphaseSeg2 + 1) + timingConfig.fpropSeg;
    uint32_t priDiv = baudRateFD_Bps * quantum;

    /* Assertion: Desired baud rate is too high. */
    assert(baudRateFD_Bps <= 8000000U);
    /* Assertion: Source clock should greater than baud rate * FLEXCAN_TIME_QUANTA_NUM. */
    assert(priDiv <= sourceClock_Hz);

    if (0 == priDiv)
    {
        priDiv = 1;
    }

    priDiv = (sourceClock_Hz / priDiv) - 1;

    /* Desired baud rate is too low. */
    if (priDiv > 0xFF)
    {
        priDiv = 0xFF;
    }

    timingConfig.fpreDivider = priDiv;

    /* Update actual timing characteristic. */
    FLEXCAN_SetFDTimingConfig(base, &timingConfig);
}
#endif

void FLEXCAN_Init(CAN_Type *base, const flexcan_config_t *config, uint32_t sourceClock_Hz)
{
    uint32_t mcrTemp;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    uint32_t instance;
#endif

    /* Assertion. */
    assert(config);
    assert((config->maxMbNum > 0) && (config->maxMbNum <= FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base)));

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    instance = FLEXCAN_GetInstance(base);
    /* Enable FlexCAN clock. */
    CLOCK_EnableClock(s_flexcanClock[instance]);
#if defined(FLEXCAN_PERIPH_CLOCKS)
    /* Enable FlexCAN serial clock. */
    CLOCK_EnableClock(s_flexcanPeriphClock[instance]);
#endif /* FLEXCAN_PERIPH_CLOCKS */
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if (!defined(FSL_FEATURE_FLEXCAN_SUPPORT_ENGINE_CLK_SEL_REMOVE)) || !FSL_FEATURE_FLEXCAN_SUPPORT_ENGINE_CLK_SEL_REMOVE
    /* Disable FlexCAN Module. */
    FLEXCAN_Enable(base, false);

    /* Protocol-Engine clock source selection, This bit must be set
     * when FlexCAN Module in Disable Mode.
     */
    base->CTRL1 = (kFLEXCAN_ClkSrcOsc == config->clkSrc) ? base->CTRL1 & ~CAN_CTRL1_CLKSRC_MASK :
                                                           base->CTRL1 | CAN_CTRL1_CLKSRC_MASK;
#else
#if defined(CAN_CTRL1_CLKSRC_MASK)
    if (!FSL_FEATURE_FLEXCAN_INSTANCE_SUPPORT_ENGINE_CLK_SEL_REMOVEn(base))
    {
        /* Disable FlexCAN Module. */
        FLEXCAN_Enable(base, false);

        /* Protocol-Engine clock source selection, This bit must be set
         * when FlexCAN Module in Disable Mode.
         */
        base->CTRL1 = (kFLEXCAN_ClkSrcOsc == config->clkSrc) ? base->CTRL1 & ~CAN_CTRL1_CLKSRC_MASK :
                                                           base->CTRL1 | CAN_CTRL1_CLKSRC_MASK;
    }
#endif
#endif /* FSL_FEATURE_FLEXCAN_SUPPORT_ENGINE_CLK_SEL_REMOVE */

    /* Enable FlexCAN Module for configuration. */
    FLEXCAN_Enable(base, true);

    /* Reset to known status. */
    FLEXCAN_Reset(base);

    /* Save current MCR value and enable to enter Freeze mode(enabled by default). */
    mcrTemp = base->MCR;

    /* Set the maximum number of Message Buffers */
    mcrTemp = (mcrTemp & ~CAN_MCR_MAXMB_MASK) | CAN_MCR_MAXMB(config->maxMbNum - 1);

    /* Enable Loop Back Mode? */
    base->CTRL1 = (config->enableLoopBack) ? base->CTRL1 | CAN_CTRL1_LPB_MASK : base->CTRL1 & ~CAN_CTRL1_LPB_MASK;

    /* Enable Timer Sync? */
    base->CTRL1 = (config->enableTimerSync) ? base->CTRL1 | CAN_CTRL1_TSYN_MASK : base->CTRL1 & ~CAN_CTRL1_TSYN_MASK;

    /* Enable Self Wake Up Mode? */
    mcrTemp = (config->enableSelfWakeup) ? mcrTemp | CAN_MCR_SLFWAK_MASK : mcrTemp & ~CAN_MCR_SLFWAK_MASK;

    /* Enable Individual Rx Masking? */
    mcrTemp = (config->enableIndividMask) ? mcrTemp | CAN_MCR_IRMQ_MASK : mcrTemp & ~CAN_MCR_IRMQ_MASK;

#if (defined(FSL_FEATURE_FLEXCAN_HAS_DOZE_MODE_SUPPORT) && FSL_FEATURE_FLEXCAN_HAS_DOZE_MODE_SUPPORT)
    if (FSL_FEATURE_FLEXCAN_INSTANCE_HAS_DOZE_MODE_SUPPORTn(base))
    {
        /* Enable Doze Mode? */
        mcrTemp = (config->enableDoze) ? mcrTemp | CAN_MCR_DOZE_MASK : mcrTemp & ~CAN_MCR_DOZE_MASK;
    }
#endif

    /* Save MCR Configuration. */
    base->MCR = mcrTemp;

    /* Baud Rate Configuration.*/
    FLEXCAN_SetBaudRate(base, sourceClock_Hz, config->baudRate, config->timingConfig);
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
void FLEXCAN_FDInit(CAN_Type *base, const flexcan_config_t *config, uint32_t sourceClock_Hz, flexcan_mb_size_t dataSize, bool brs)
{
    assert(dataSize <= 3U);

    /* Initialization of classical CAN. */
    FLEXCAN_Init(base, config, sourceClock_Hz);

    /* Extra bitrate setting for CANFD. */
    FLEXCAN_SetFDBaudRate(base, sourceClock_Hz, config->baudRateFD, config->timingConfig);

    /* Enable FD operation and set bitrate switch. */
    if (brs)
    {
        base->FDCTRL &= CAN_FDCTRL_FDRATE_MASK;
    }
    else
    {
        base->FDCTRL &= ~CAN_FDCTRL_FDRATE_MASK;
    }
    /* Enter Freeze Mode. */
    FLEXCAN_EnterFreezeMode(base);
    if (brs && !config->enableLoopBack)
    {
        base->FDCTRL |= CAN_FDCTRL_TDCEN_MASK | CAN_FDCTRL_TDCOFF(0x2U);
    }
    base->MCR |= CAN_MCR_FDEN_MASK;
    base->FDCTRL |= CAN_FDCTRL_MBDSR0(dataSize);
#if defined(CAN_FDCTRL_MBDSR1_MASK)
    base->FDCTRL |= CAN_FDCTRL_MBDSR1(dataSize);
#endif
#if defined(CAN_FDCTRL_MBDSR2_MASK)
    base->FDCTRL |= CAN_FDCTRL_MBDSR2(dataSize);
#endif
#if defined(CAN_FDCTRL_MBDSR3_MASK)
    base->FDCTRL |= CAN_FDCTRL_MBDSR3(dataSize);
#endif
    /* Exit Freeze Mode. */
    FLEXCAN_ExitFreezeMode(base);
}
#endif

void FLEXCAN_Deinit(CAN_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    uint32_t instance;
#endif
    /* Reset all Register Contents. */
    FLEXCAN_Reset(base);

    /* Disable FlexCAN module. */
    FLEXCAN_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    instance = FLEXCAN_GetInstance(base);
#if defined(FLEXCAN_PERIPH_CLOCKS)
    /* Disable FlexCAN serial clock. */
    CLOCK_DisableClock(s_flexcanPeriphClock[instance]);
#endif /* FLEXCAN_PERIPH_CLOCKS */
    /* Disable FlexCAN clock. */
    CLOCK_DisableClock(s_flexcanClock[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void FLEXCAN_GetDefaultConfig(flexcan_config_t *config)
{
    /* Assertion. */
    assert(config);

    /* Initialize FlexCAN Module config struct with default value. */
    config->clkSrc = kFLEXCAN_ClkSrcOsc;
    config->baudRate = 1000000U;
#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
    config->baudRateFD = 2000000U;
#endif
    config->maxMbNum = 16;
    config->enableLoopBack = false;
    config->enableTimerSync = true;
    config->enableSelfWakeup = false;
    config->enableIndividMask = false;
#if (defined(FSL_FEATURE_FLEXCAN_HAS_DOZE_MODE_SUPPORT) && FSL_FEATURE_FLEXCAN_HAS_DOZE_MODE_SUPPORT)
    config->enableDoze = false;
#endif
    /* Default protocol timing configuration, time quantum is 10. */
    config->timingConfig.phaseSeg1 = 3;
    config->timingConfig.phaseSeg2 = 2;
    config->timingConfig.propSeg = 1;
    config->timingConfig.rJumpwidth = 1;
#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
    config->timingConfig.fphaseSeg1 = 3;
    config->timingConfig.fphaseSeg2 = 3;
    config->timingConfig.fpropSeg = 1;
    config->timingConfig.frJumpwidth = 1;
#endif
}

void FLEXCAN_SetTimingConfig(CAN_Type *base, const flexcan_timing_config_t *config)
{
    /* Assertion. */
    assert(config);

    /* Enter Freeze Mode. */
    FLEXCAN_EnterFreezeMode(base);

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
    if (FSL_FEATURE_FLEXCAN_INSTANCE_HAS_FLEXIBLE_DATA_RATEn(base))
    {
        /* Cleaning previous Timing Setting. */
        base->CBT &= ~(CAN_CBT_EPRESDIV_MASK | CAN_CBT_ERJW_MASK | CAN_CBT_EPSEG1_MASK | CAN_CBT_EPSEG2_MASK |
                   CAN_CBT_EPROPSEG_MASK);

        /* Updating Timing Setting according to configuration structure. */
        base->CBT |=
            (CAN_CBT_EPRESDIV(config->preDivider) | CAN_CBT_ERJW(config->rJumpwidth) | CAN_CBT_EPSEG1(config->phaseSeg1) |
             CAN_CBT_EPSEG2(config->phaseSeg2) | CAN_CBT_EPROPSEG(config->propSeg));
    }
    else
    {
        /* Cleaning previous Timing Setting. */
        base->CTRL1 &= ~(CAN_CTRL1_PRESDIV_MASK | CAN_CTRL1_RJW_MASK | CAN_CTRL1_PSEG1_MASK | CAN_CTRL1_PSEG2_MASK |
                         CAN_CTRL1_PROPSEG_MASK);

        /* Updating Timing Setting according to configuration structure. */
        base->CTRL1 |=
            (CAN_CTRL1_PRESDIV(config->preDivider) | CAN_CTRL1_RJW(config->rJumpwidth) |
             CAN_CTRL1_PSEG1(config->phaseSeg1) | CAN_CTRL1_PSEG2(config->phaseSeg2) | CAN_CTRL1_PROPSEG(config->propSeg));
    }
#else
    /* Cleaning previous Timing Setting. */
    base->CTRL1 &= ~(CAN_CTRL1_PRESDIV_MASK | CAN_CTRL1_RJW_MASK | CAN_CTRL1_PSEG1_MASK | CAN_CTRL1_PSEG2_MASK |
                     CAN_CTRL1_PROPSEG_MASK);

    /* Updating Timing Setting according to configuration structure. */
    base->CTRL1 |=
        (CAN_CTRL1_PRESDIV(config->preDivider) | CAN_CTRL1_RJW(config->rJumpwidth) |
         CAN_CTRL1_PSEG1(config->phaseSeg1) | CAN_CTRL1_PSEG2(config->phaseSeg2) | CAN_CTRL1_PROPSEG(config->propSeg));
#endif

    /* Exit Freeze Mode. */
    FLEXCAN_ExitFreezeMode(base);
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
void FLEXCAN_SetFDTimingConfig(CAN_Type *base, const flexcan_timing_config_t *config)
{
    /* Assertion. */
    assert(config);

    /* Enter Freeze Mode. */
    FLEXCAN_EnterFreezeMode(base);

    base->CBT |= CAN_CBT_BTF(1);
    /* Cleaning previous Timing Setting. */
    base->FDCBT &= ~(CAN_FDCBT_FPRESDIV_MASK | CAN_FDCBT_FRJW_MASK | CAN_FDCBT_FPSEG1_MASK | CAN_FDCBT_FPSEG2_MASK |
                     CAN_FDCBT_FPROPSEG_MASK);

    /* Updating Timing Setting according to configuration structure. */
    base->FDCBT |= (CAN_FDCBT_FPRESDIV(config->fpreDivider) | CAN_FDCBT_FRJW(config->frJumpwidth) |
                    CAN_FDCBT_FPSEG1(config->fphaseSeg1) | CAN_FDCBT_FPSEG2(config->fphaseSeg2) |
                    CAN_FDCBT_FPROPSEG(config->fpropSeg));

    /* Exit Freeze Mode. */
    FLEXCAN_ExitFreezeMode(base);
}
#endif

void FLEXCAN_SetRxMbGlobalMask(CAN_Type *base, uint32_t mask)
{
    /* Enter Freeze Mode. */
    FLEXCAN_EnterFreezeMode(base);

    /* Setting Rx Message Buffer Global Mask value. */
    base->RXMGMASK = mask;
    base->RX14MASK = mask;
    base->RX15MASK = mask;

    /* Exit Freeze Mode. */
    FLEXCAN_ExitFreezeMode(base);
}

void FLEXCAN_SetRxFifoGlobalMask(CAN_Type *base, uint32_t mask)
{
    /* Enter Freeze Mode. */
    FLEXCAN_EnterFreezeMode(base);

    /* Setting Rx FIFO Global Mask value. */
    base->RXFGMASK = mask;

    /* Exit Freeze Mode. */
    FLEXCAN_ExitFreezeMode(base);
}

void FLEXCAN_SetRxIndividualMask(CAN_Type *base, uint8_t maskIdx, uint32_t mask)
{
    assert(maskIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));

    /* Enter Freeze Mode. */
    FLEXCAN_EnterFreezeMode(base);

    /* Setting Rx Individual Mask value. */
    base->RXIMR[maskIdx] = mask;

    /* Exit Freeze Mode. */
    FLEXCAN_ExitFreezeMode(base);
}

void FLEXCAN_SetTxMbConfig(CAN_Type *base, uint8_t mbIdx, bool enable)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    /* Inactivate Message Buffer. */
    if (enable)
    {
        base->MB[mbIdx].CS = CAN_CS_CODE(kFLEXCAN_TxMbInactive);
    }
    else
    {
        base->MB[mbIdx].CS = 0;
    }

    /* Clean Message Buffer content. */
    base->MB[mbIdx].ID = 0x0;
    base->MB[mbIdx].WORD0 = 0x0;
    base->MB[mbIdx].WORD1 = 0x0;
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
static uint32_t FLEXCAN_GetFDMailboxOffset(CAN_Type *base, uint8_t mbIdx)
{
    uint32_t dataSize;
    uint32_t offset = 0;
    dataSize = (base->FDCTRL & CAN_FDCTRL_MBDSR0_MASK) >> CAN_FDCTRL_MBDSR0_SHIFT;
    switch (dataSize)
    {
        case kFLEXCAN_8BperMB:
            offset = (mbIdx / 32) * 512 + mbIdx % 32 * 16;
            break;
        case kFLEXCAN_16BperMB:
            offset = (mbIdx / 21) * 512 + mbIdx % 21 * 24;
            break;
        case kFLEXCAN_32BperMB:
            offset = (mbIdx / 12) * 512 + mbIdx % 12 * 40;
            break;
        case kFLEXCAN_64BperMB:
            offset = (mbIdx / 7) * 512 + mbIdx % 7 * 72;
            break;
        default:
            break;
    }
    /* To get the dword aligned offset, need to divide by 4. */
    offset = offset / 4;
    return offset;
}
#endif

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
void FLEXCAN_SetFDTxMbConfig(CAN_Type *base, uint8_t mbIdx, bool enable)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));
    uint8_t cnt = 0;
    uint8_t payload_dword = 1;
    uint32_t dataSize;
    dataSize = (base->FDCTRL & CAN_FDCTRL_MBDSR0_MASK) >> CAN_FDCTRL_MBDSR0_SHIFT;
    volatile uint32_t *mbAddr = &(base->MB[0].CS);
    uint32_t offset = FLEXCAN_GetFDMailboxOffset(base, mbIdx);
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
    uint32_t availoffset = FLEXCAN_GetFDMailboxOffset(base, FLEXCAN_GetFirstValidMb(base));
#endif

    /* Inactivate Message Buffer. */
    if (enable)
    {
        /* Inactivate by writing CS. */
        mbAddr[offset] = CAN_CS_CODE(kFLEXCAN_TxMbInactive);
    }
    else
    {
        mbAddr[offset] = 0x0;
    }

    /* Calculate the DWORD number, dataSize 0/1/2/3 corresponds to 8/16/32/64
       Bytes payload. */
    for (cnt = 0; cnt < dataSize + 1; cnt++)
    {
        payload_dword *= 2;
    }

    /* Clean ID. */
    mbAddr[offset + 1] = 0x0;
    /* Clean Message Buffer content, DWORD by DWORD. */
    for (cnt = 0; cnt < payload_dword; cnt++)
    {
        mbAddr[offset + 2 + cnt] = 0x0;
    }

#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
    mbAddr[availoffset] = CAN_CS_CODE(kFLEXCAN_TxMbInactive);
#endif
}
#endif

void FLEXCAN_SetRxMbConfig(CAN_Type *base, uint8_t mbIdx, const flexcan_rx_mb_config_t *config, bool enable)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(((config) || (false == enable)));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    uint32_t cs_temp = 0;

    /* Inactivate Message Buffer. */
    base->MB[mbIdx].CS = 0;

    /* Clean Message Buffer content. */
    base->MB[mbIdx].ID = 0x0;
    base->MB[mbIdx].WORD0 = 0x0;
    base->MB[mbIdx].WORD1 = 0x0;

    if (enable)
    {
        /* Setup Message Buffer ID. */
        base->MB[mbIdx].ID = config->id;

        /* Setup Message Buffer format. */
        if (kFLEXCAN_FrameFormatExtend == config->format)
        {
            cs_temp |= CAN_CS_IDE_MASK;
        }

        /* Setup Message Buffer type. */
        if (kFLEXCAN_FrameTypeRemote == config->type)
        {
            cs_temp |= CAN_CS_RTR_MASK;
        }

        /* Activate Rx Message Buffer. */
        cs_temp |= CAN_CS_CODE(kFLEXCAN_RxMbEmpty);
        base->MB[mbIdx].CS = cs_temp;
    }
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
void FLEXCAN_SetFDRxMbConfig(CAN_Type *base, uint8_t mbIdx, const flexcan_rx_mb_config_t *config, bool enable)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(((config) || (false == enable)));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    uint32_t cs_temp = 0;
    uint8_t cnt = 0;
    volatile uint32_t *mbAddr = &(base->MB[0].CS);
    uint32_t offset = FLEXCAN_GetFDMailboxOffset(base, mbIdx);

    /* Inactivate all mailboxes first, clean ID and Message Buffer content. */
    for (cnt = 0; cnt < FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base); cnt++)
    {
       base->MB[cnt].CS = 0;
       base->MB[cnt].ID = 0;
       base->MB[cnt].WORD0 = 0;
       base->MB[cnt].WORD1 = 0;
    }

    if (enable)
    {
        /* Setup Message Buffer ID. */
        mbAddr[offset + 1] = config->id;

        /* Setup Message Buffer format. */
        if (kFLEXCAN_FrameFormatExtend == config->format)
        {
            cs_temp |= CAN_CS_IDE_MASK;
        }

        /* Activate Rx Message Buffer. */
        cs_temp |= CAN_CS_CODE(kFLEXCAN_RxMbEmpty);
        mbAddr[offset] = cs_temp;
    }
}
#endif

void FLEXCAN_SetRxFifoConfig(CAN_Type *base, const flexcan_rx_fifo_config_t *config, bool enable)
{
    /* Assertion. */
    assert((config) || (false == enable));

    volatile uint32_t *idFilterRegion = (volatile uint32_t *)(&base->MB[6].CS);
    uint8_t setup_mb, i, rffn = 0;

    /* Enter Freeze Mode. */
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
    {
        assert(config->idFilterNum <= 128);

        /* Get the setup_mb value. */
        setup_mb = (base->MCR & CAN_MCR_MAXMB_MASK) >> CAN_MCR_MAXMB_SHIFT;
        setup_mb = (setup_mb < FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base)) ?
                       setup_mb :
                       FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base);

        /* Determine RFFN value. */
        for (i = 0; i <= 0xF; i++)
        {
            if ((8 * (i + 1)) >= config->idFilterNum)
            {
                rffn = i;
                assert(((setup_mb - 8) - (2 * rffn)) > 0);

                base->CTRL2 = (base->CTRL2 & ~CAN_CTRL2_RFFN_MASK) | CAN_CTRL2_RFFN(rffn);
                break;
            }
        }
    }
    else
    {
        rffn = (base->CTRL2 & CAN_CTRL2_RFFN_MASK) >> CAN_CTRL2_RFFN_SHIFT;
    }

    /* Clean ID filter table occuyied Message Buffer Region. */
    rffn = (rffn + 1) * 8;
    for (i = 0; i < rffn; i++)
    {
        idFilterRegion[i] = 0x0;
    }

    if (enable)
    {
        /* Disable unused Rx FIFO Filter. */
        for (i = config->idFilterNum; i < rffn; i++)
        {
            idFilterRegion[i] = 0xFFFFFFFFU;
        }

        /* Copy ID filter table to Message Buffer Region. */
        for (i = 0; i < config->idFilterNum; i++)
        {
            idFilterRegion[i] = config->idFilterTable[i];
        }

        /* Setup ID Fitlter Type. */
        switch (config->idFilterType)
        {
            case kFLEXCAN_RxFifoFilterTypeA:
                base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x0);
                break;
            case kFLEXCAN_RxFifoFilterTypeB:
                base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x1);
                break;
            case kFLEXCAN_RxFifoFilterTypeC:
                base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x2);
                break;
            case kFLEXCAN_RxFifoFilterTypeD:
                /* All frames rejected. */
                base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x3);
                break;
            default:
                break;
        }

        /* Setting Message Reception Priority. */
        base->CTRL2 = (config->priority == kFLEXCAN_RxFifoPrioHigh) ? base->CTRL2 & ~CAN_CTRL2_MRP_MASK :
                                                                      base->CTRL2 | CAN_CTRL2_MRP_MASK;

        /* Enable Rx Message FIFO. */
        base->MCR |= CAN_MCR_RFEN_MASK;
    }
    else
    {
        /* Disable Rx Message FIFO. */
        base->MCR &= ~CAN_MCR_RFEN_MASK;

        /* Clean MB0 ~ MB5. */
        FLEXCAN_SetRxMbConfig(base, 0, NULL, false);
        FLEXCAN_SetRxMbConfig(base, 1, NULL, false);
        FLEXCAN_SetRxMbConfig(base, 2, NULL, false);
        FLEXCAN_SetRxMbConfig(base, 3, NULL, false);
        FLEXCAN_SetRxMbConfig(base, 4, NULL, false);
        FLEXCAN_SetRxMbConfig(base, 5, NULL, false);
    }

    /* Exit Freeze Mode. */
    FLEXCAN_ExitFreezeMode(base);
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_RX_FIFO_DMA) && FSL_FEATURE_FLEXCAN_HAS_RX_FIFO_DMA)
void FLEXCAN_EnableRxFifoDMA(CAN_Type *base, bool enable)
{
    if (enable)
    {
        /* Enter Freeze Mode. */
        FLEXCAN_EnterFreezeMode(base);

        /* Enable FlexCAN DMA. */
        base->MCR |= CAN_MCR_DMA_MASK;

        /* Exit Freeze Mode. */
        FLEXCAN_ExitFreezeMode(base);
    }
    else
    {
        /* Enter Freeze Mode. */
        FLEXCAN_EnterFreezeMode(base);

        /* Disable FlexCAN DMA. */
        base->MCR &= ~CAN_MCR_DMA_MASK;

        /* Exit Freeze Mode. */
        FLEXCAN_ExitFreezeMode(base);
    }
}
#endif /* FSL_FEATURE_FLEXCAN_HAS_RX_FIFO_DMA */

status_t FLEXCAN_WriteTxMb(CAN_Type *base, uint8_t mbIdx, const flexcan_frame_t *txFrame)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(txFrame);
    assert(txFrame->length <= 8);
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    uint32_t cs_temp = 0;

    /* Check if Message Buffer is available. */
    if (CAN_CS_CODE(kFLEXCAN_TxMbDataOrRemote) != (base->MB[mbIdx].CS & CAN_CS_CODE_MASK))
    {
        /* Inactive Tx Message Buffer. */
        base->MB[mbIdx].CS = (base->MB[mbIdx].CS & ~CAN_CS_CODE_MASK) | CAN_CS_CODE(kFLEXCAN_TxMbInactive);

        /* Fill Message ID field. */
        base->MB[mbIdx].ID = txFrame->id;

        /* Fill Message Format field. */
        if (kFLEXCAN_FrameFormatExtend == txFrame->format)
        {
            cs_temp |= CAN_CS_SRR_MASK | CAN_CS_IDE_MASK;
        }

        /* Fill Message Type field. */
        if (kFLEXCAN_FrameTypeRemote == txFrame->type)
        {
            cs_temp |= CAN_CS_RTR_MASK;
        }

        cs_temp |= CAN_CS_CODE(kFLEXCAN_TxMbDataOrRemote) | CAN_CS_DLC(txFrame->length);

        /* Load Message Payload. */
        base->MB[mbIdx].WORD0 = txFrame->dataWord0;
        base->MB[mbIdx].WORD1 = txFrame->dataWord1;

        /* Activate Tx Message Buffer. */
        base->MB[mbIdx].CS = cs_temp;

#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
        base->MB[FLEXCAN_GetFirstValidMb(base)].CS = CAN_CS_CODE(kFLEXCAN_TxMbInactive);
        base->MB[FLEXCAN_GetFirstValidMb(base)].CS = CAN_CS_CODE(kFLEXCAN_TxMbInactive);
#endif

        return kStatus_Success;
    }
    else
    {
        /* Tx Message Buffer is activated, return immediately. */
        return kStatus_Fail;
    }
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
status_t FLEXCAN_WriteFDTxMb(CAN_Type *base, uint8_t mbIdx, const flexcan_fd_frame_t *txFrame)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(txFrame);
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    uint32_t cs_temp = 0;
    uint8_t cnt = 0;
    uint32_t can_cs = 0;
    uint8_t payload_dword = 1;
    uint32_t dataSize;
    dataSize = (base->FDCTRL & CAN_FDCTRL_MBDSR0_MASK) >> CAN_FDCTRL_MBDSR0_SHIFT;
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
    uint32_t availoffset = FLEXCAN_GetFDMailboxOffset(base, FLEXCAN_GetFirstValidMb(base));
#endif
    volatile uint32_t *mbAddr = &(base->MB[0].CS);
    uint32_t offset = FLEXCAN_GetFDMailboxOffset(base, mbIdx);

    can_cs = mbAddr[0];
    /* Check if Message Buffer is available. */
    if (CAN_CS_CODE(kFLEXCAN_TxMbDataOrRemote) != (can_cs & CAN_CS_CODE_MASK))
    {
        /* Inactive Tx Message Buffer and Fill Message ID field. */
        mbAddr[offset] = (can_cs & ~CAN_CS_CODE_MASK) | CAN_CS_CODE(kFLEXCAN_TxMbInactive);
        mbAddr[offset + 1] = txFrame->id;

        /* Fill Message Format field. */
        if (kFLEXCAN_FrameFormatExtend == txFrame->format)
        {
            cs_temp |= CAN_CS_SRR_MASK | CAN_CS_IDE_MASK;
        }

        cs_temp |= CAN_CS_CODE(kFLEXCAN_TxMbDataOrRemote) | CAN_CS_DLC(txFrame->length) | CAN_CS_EDL(1) | CAN_CS_BRS(txFrame->brs);

        /* Calculate the DWORD number, dataSize 0/1/2/3 corresponds to 8/16/32/64
           Bytes payload. */
        for (cnt = 0; cnt < dataSize + 1; cnt++)
        {
            payload_dword *= 2;
        }

        /* Load Message Payload and Activate Tx Message Buffer. */
        for (cnt = 0; cnt < payload_dword; cnt++)
        {
            mbAddr[offset + 2 + cnt] = txFrame->dataWord[cnt];
        }
        mbAddr[offset] = cs_temp;

#if (defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641)
        mbAddr[availoffset] = CAN_CS_CODE(kFLEXCAN_TxMbInactive);
        mbAddr[availoffset] = CAN_CS_CODE(kFLEXCAN_TxMbInactive);
#endif

        return kStatus_Success;
    }
    else
    {
        /* Tx Message Buffer is activated, return immediately. */
        return kStatus_Fail;
    }
}
#endif

status_t FLEXCAN_ReadRxMb(CAN_Type *base, uint8_t mbIdx, flexcan_frame_t *rxFrame)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(rxFrame);
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    uint32_t cs_temp;
    uint8_t rx_code;

    /* Read CS field of Rx Message Buffer to lock Message Buffer. */
    cs_temp = base->MB[mbIdx].CS;
    /* Get Rx Message Buffer Code field. */
    rx_code = (cs_temp & CAN_CS_CODE_MASK) >> CAN_CS_CODE_SHIFT;

    /* Check to see if Rx Message Buffer is full. */
    if ((kFLEXCAN_RxMbFull == rx_code) || (kFLEXCAN_RxMbOverrun == rx_code))
    {
        /* Store Message ID. */
        rxFrame->id = base->MB[mbIdx].ID & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK);

        /* Get the message ID and format. */
        rxFrame->format = (cs_temp & CAN_CS_IDE_MASK) ? kFLEXCAN_FrameFormatExtend : kFLEXCAN_FrameFormatStandard;

        /* Get the message type. */
        rxFrame->type = (cs_temp & CAN_CS_RTR_MASK) ? kFLEXCAN_FrameTypeRemote : kFLEXCAN_FrameTypeData;

        /* Get the message length. */
        rxFrame->length = (cs_temp & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT;

        /* Get the time stamp. */
        rxFrame->timestamp = (cs_temp & CAN_CS_TIME_STAMP_MASK) >> CAN_CS_TIME_STAMP_SHIFT;

        /* Store Message Payload. */
        rxFrame->dataWord0 = base->MB[mbIdx].WORD0;
        rxFrame->dataWord1 = base->MB[mbIdx].WORD1;

        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        if (kFLEXCAN_RxMbFull == rx_code)
        {
            return kStatus_Success;
        }
        else
        {
            return kStatus_FLEXCAN_RxOverflow;
        }
    }
    else
    {
        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        return kStatus_Fail;
    }
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
status_t FLEXCAN_ReadFDRxMb(CAN_Type *base, uint8_t mbIdx, flexcan_fd_frame_t *rxFrame)
{
    /* Assertion. */
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(rxFrame);
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    uint32_t cs_temp;
    uint8_t rx_code;
    uint8_t cnt = 0;
    uint32_t can_id = 0;
    uint32_t dataSize;
    dataSize = (base->FDCTRL & CAN_FDCTRL_MBDSR0_MASK) >> CAN_FDCTRL_MBDSR0_SHIFT;
    uint8_t payload_dword = 1;
    volatile uint32_t *mbAddr = &(base->MB[0].CS);
    uint32_t offset = FLEXCAN_GetFDMailboxOffset(base, mbIdx);

    /* Read CS field of Rx Message Buffer to lock Message Buffer. */
    cs_temp = mbAddr[offset];
    can_id = mbAddr[offset + 1];

    /* Get Rx Message Buffer Code field. */
    rx_code = (cs_temp & CAN_CS_CODE_MASK) >> CAN_CS_CODE_SHIFT;

    /* Check to see if Rx Message Buffer is full. */
    if ((kFLEXCAN_RxMbFull == rx_code) || (kFLEXCAN_RxMbOverrun == rx_code))
    {
        /* Store Message ID. */
        rxFrame->id = can_id & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK);

        /* Get the message ID and format. */
        rxFrame->format = (cs_temp & CAN_CS_IDE_MASK) ? kFLEXCAN_FrameFormatExtend : kFLEXCAN_FrameFormatStandard;

        /* Get the message type. */
        rxFrame->type = (cs_temp & CAN_CS_RTR_MASK) ? kFLEXCAN_FrameTypeRemote : kFLEXCAN_FrameTypeData;

        /* Get the message length. */
        rxFrame->length = (cs_temp & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT;

        /* Get the time stamp. */
        rxFrame->timestamp = (cs_temp & CAN_CS_TIME_STAMP_MASK) >> CAN_CS_TIME_STAMP_SHIFT;

        /* Calculate the DWORD number, dataSize 0/1/2/3 corresponds to 8/16/32/64
           Bytes payload. */
        for (cnt = 0; cnt < dataSize + 1; cnt++)
        {
            payload_dword *= 2;
        }

        /* Store Message Payload. */
        for (cnt = 0; cnt < payload_dword; cnt++)
        {
            rxFrame->dataWord[cnt] = mbAddr[offset + 2 + cnt];
        }

        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        if (kFLEXCAN_RxMbFull == rx_code)
        {
            return kStatus_Success;
        }
        else
        {
            return kStatus_FLEXCAN_RxOverflow;
        }
    }
    else
    {
        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        return kStatus_Fail;
    }
}
#endif

status_t FLEXCAN_ReadRxFifo(CAN_Type *base, flexcan_frame_t *rxFrame)
{
    /* Assertion. */
    assert(rxFrame);

    uint32_t cs_temp;

    /* Check if Rx FIFO is Enabled. */
    if (base->MCR & CAN_MCR_RFEN_MASK)
    {
        /* Read CS field of Rx Message Buffer to lock Message Buffer. */
        cs_temp = base->MB[0].CS;

        /* Read data from Rx FIFO output port. */
        /* Store Message ID. */
        rxFrame->id = base->MB[0].ID & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK);

        /* Get the message ID and format. */
        rxFrame->format = (cs_temp & CAN_CS_IDE_MASK) ? kFLEXCAN_FrameFormatExtend : kFLEXCAN_FrameFormatStandard;

        /* Get the message type. */
        rxFrame->type = (cs_temp & CAN_CS_RTR_MASK) ? kFLEXCAN_FrameTypeRemote : kFLEXCAN_FrameTypeData;

        /* Get the message length. */
        rxFrame->length = (cs_temp & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT;

        /* Store Message Payload. */
        rxFrame->dataWord0 = base->MB[0].WORD0;
        rxFrame->dataWord1 = base->MB[0].WORD1;

        /* Store ID Filter Hit Index. */
        rxFrame->idhit = (uint8_t)(base->RXFIR & CAN_RXFIR_IDHIT_MASK);

        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        return kStatus_Success;
    }
    else
    {
        return kStatus_Fail;
    }
}

status_t FLEXCAN_TransferSendBlocking(CAN_Type *base, uint8_t mbIdx, flexcan_frame_t *txFrame)
{
    /* Write Tx Message Buffer to initiate a data sending. */
    if (kStatus_Success == FLEXCAN_WriteTxMb(base, mbIdx, txFrame))
    {
        /* Wait until CAN Message send out. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
        while (!FLEXCAN_GetMbStatusFlags(base, (uint64_t)1 << mbIdx))
#else
        while (!FLEXCAN_GetMbStatusFlags(base, 1 << mbIdx))
#endif
        {
        }

        /* Clean Tx Message Buffer Flag. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
        FLEXCAN_ClearMbStatusFlags(base, (uint64_t)1 << mbIdx);
#else
        FLEXCAN_ClearMbStatusFlags(base, 1 << mbIdx);
#endif

        return kStatus_Success;
    }
    else
    {
        return kStatus_Fail;
    }
}

status_t FLEXCAN_TransferReceiveBlocking(CAN_Type *base, uint8_t mbIdx, flexcan_frame_t *rxFrame)
{
    /* Wait until Rx Message Buffer non-empty. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    while (!FLEXCAN_GetMbStatusFlags(base, (uint64_t)1 << mbIdx))
#else
    while (!FLEXCAN_GetMbStatusFlags(base, 1 << mbIdx))
#endif
    {
    }

    /* Clean Rx Message Buffer Flag. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    FLEXCAN_ClearMbStatusFlags(base, (uint64_t)1 << mbIdx);
#else
    FLEXCAN_ClearMbStatusFlags(base, 1 << mbIdx);
#endif

    /* Read Received CAN Message. */
    return FLEXCAN_ReadRxMb(base, mbIdx, rxFrame);
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
status_t FLEXCAN_TransferFDSendBlocking(CAN_Type *base, uint8_t mbIdx, flexcan_fd_frame_t *txFrame)
{
    /* Write Tx Message Buffer to initiate a data sending. */
    if (kStatus_Success == FLEXCAN_WriteFDTxMb(base, mbIdx, txFrame))
    {
        /* Wait until CAN Message send out. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
        while (!FLEXCAN_GetMbStatusFlags(base, (uint64_t)1 << mbIdx))
#else
        while (!FLEXCAN_GetMbStatusFlags(base, 1 << mbIdx))
#endif
        {
        }

        /* Clean Tx Message Buffer Flag. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
        FLEXCAN_ClearMbStatusFlags(base, (uint64_t)1 << mbIdx);
#else
        FLEXCAN_ClearMbStatusFlags(base, 1 << mbIdx);
#endif

        return kStatus_Success;
    }
    else
    {
        return kStatus_Fail;
    }
}

status_t FLEXCAN_TransferFDReceiveBlocking(CAN_Type *base, uint8_t mbIdx, flexcan_fd_frame_t *rxFrame)
{
    /* Wait until Rx Message Buffer non-empty. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    while (!FLEXCAN_GetMbStatusFlags(base, (uint64_t)1 << mbIdx))
#else
    while (!FLEXCAN_GetMbStatusFlags(base, 1 << mbIdx))
#endif
    {
    }

    /* Clean Rx Message Buffer Flag. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    FLEXCAN_ClearMbStatusFlags(base, (uint64_t)1 << mbIdx);
#else
    FLEXCAN_ClearMbStatusFlags(base, 1 << mbIdx);
#endif

    /* Read Received CAN Message. */
    return FLEXCAN_ReadFDRxMb(base, mbIdx, rxFrame);
}
#endif

status_t FLEXCAN_TransferReceiveFifoBlocking(CAN_Type *base, flexcan_frame_t *rxFrame)
{
    status_t rxFifoStatus;

    /* Wait until Rx FIFO non-empty. */
    while (!FLEXCAN_GetMbStatusFlags(base, kFLEXCAN_RxFifoFrameAvlFlag))
    {
    }

    /*  */
    rxFifoStatus = FLEXCAN_ReadRxFifo(base, rxFrame);

    /* Clean Rx Fifo available flag. */
    FLEXCAN_ClearMbStatusFlags(base, kFLEXCAN_RxFifoFrameAvlFlag);

    return rxFifoStatus;
}

void FLEXCAN_TransferCreateHandle(CAN_Type *base,
                                  flexcan_handle_t *handle,
                                  flexcan_transfer_callback_t callback,
                                  void *userData)
{
    assert(handle);

    uint8_t instance;

    /* Clean FlexCAN transfer handle. */
    memset(handle, 0, sizeof(*handle));

    /* Get instance from peripheral base address. */
    instance = FLEXCAN_GetInstance(base);

    /* Save the context in global variables to support the double weak mechanism. */
    s_flexcanHandle[instance] = handle;

    /* Register Callback function. */
    handle->callback = callback;
    handle->userData = userData;

    s_flexcanIsr = FLEXCAN_TransferHandleIRQ;

    /* We Enable Error & Status interrupt here, because this interrupt just
     * report current status of FlexCAN module through Callback function.
     * It is insignificance without a available callback function.
     */
    if (handle->callback != NULL)
    {
        FLEXCAN_EnableInterrupts(base, kFLEXCAN_BusOffInterruptEnable | kFLEXCAN_ErrorInterruptEnable |
                                           kFLEXCAN_RxWarningInterruptEnable | kFLEXCAN_TxWarningInterruptEnable |
                                           kFLEXCAN_WakeUpInterruptEnable);
    }
    else
    {
        FLEXCAN_DisableInterrupts(base, kFLEXCAN_BusOffInterruptEnable | kFLEXCAN_ErrorInterruptEnable |
                                            kFLEXCAN_RxWarningInterruptEnable | kFLEXCAN_TxWarningInterruptEnable |
                                            kFLEXCAN_WakeUpInterruptEnable);
    }

    /* Enable interrupts in NVIC. */
    EnableIRQ((IRQn_Type)(s_flexcanRxWarningIRQ[instance]));
    EnableIRQ((IRQn_Type)(s_flexcanTxWarningIRQ[instance]));
    EnableIRQ((IRQn_Type)(s_flexcanWakeUpIRQ[instance]));
    EnableIRQ((IRQn_Type)(s_flexcanErrorIRQ[instance]));
    EnableIRQ((IRQn_Type)(s_flexcanBusOffIRQ[instance]));
    EnableIRQ((IRQn_Type)(s_flexcanMbIRQ[instance]));
}

status_t FLEXCAN_TransferSendNonBlocking(CAN_Type *base, flexcan_handle_t *handle, flexcan_mb_transfer_t *xfer)
{
    /* Assertion. */
    assert(handle);
    assert(xfer);
    assert(xfer->mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, xfer->mbIdx));

    /* Check if Message Buffer is idle. */
    if (kFLEXCAN_StateIdle == handle->mbState[xfer->mbIdx])
    {
        /* Distinguish transmit type. */
        if (kFLEXCAN_FrameTypeRemote == xfer->frame->type)
        {
            handle->mbState[xfer->mbIdx] = kFLEXCAN_StateTxRemote;

            /* Register user Frame buffer to receive remote Frame. */
            handle->mbFrameBuf[xfer->mbIdx] = xfer->frame;
        }
        else
        {
            handle->mbState[xfer->mbIdx] = kFLEXCAN_StateTxData;
        }

        if (kStatus_Success == FLEXCAN_WriteTxMb(base, xfer->mbIdx, xfer->frame))
        {
            /* Enable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
            FLEXCAN_EnableMbInterrupts(base, (uint64_t)1 << xfer->mbIdx);
#else
            FLEXCAN_EnableMbInterrupts(base, 1 << xfer->mbIdx);
#endif

            return kStatus_Success;
        }
        else
        {
            handle->mbState[xfer->mbIdx] = kFLEXCAN_StateIdle;
            return kStatus_Fail;
        }
    }
    else
    {
        return kStatus_FLEXCAN_TxBusy;
    }
}

status_t FLEXCAN_TransferReceiveNonBlocking(CAN_Type *base, flexcan_handle_t *handle, flexcan_mb_transfer_t *xfer)
{
    /* Assertion. */
    assert(handle);
    assert(xfer);
    assert(xfer->mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, xfer->mbIdx));

    /* Check if Message Buffer is idle. */
    if (kFLEXCAN_StateIdle == handle->mbState[xfer->mbIdx])
    {
        handle->mbState[xfer->mbIdx] = kFLEXCAN_StateRxData;

        /* Register Message Buffer. */
        handle->mbFrameBuf[xfer->mbIdx] = xfer->frame;

        /* Enable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
        FLEXCAN_EnableMbInterrupts(base, (uint64_t)1 << xfer->mbIdx);
#else
        FLEXCAN_EnableMbInterrupts(base, 1 << xfer->mbIdx);
#endif

        return kStatus_Success;
    }
    else
    {
        return kStatus_FLEXCAN_RxBusy;
    }
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
status_t FLEXCAN_TransferFDSendNonBlocking(CAN_Type *base, flexcan_handle_t *handle, flexcan_mb_transfer_t *xfer)
{
    /* Assertion. */
    assert(handle);
    assert(xfer);
    assert(xfer->mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, xfer->mbIdx));

    /* Check if Message Buffer is idle. */
    if (kFLEXCAN_StateIdle == handle->mbState[xfer->mbIdx])
    {
        /* Distinguish transmit type. */
        if (kFLEXCAN_FrameTypeRemote == xfer->frame->type)
        {
            handle->mbState[xfer->mbIdx] = kFLEXCAN_StateTxRemote;

            /* Register user Frame buffer to receive remote Frame. */
            handle->mbFDFrameBuf[xfer->mbIdx] = xfer->framefd;
        }
        else
        {
            handle->mbState[xfer->mbIdx] = kFLEXCAN_StateTxData;
        }

        if (kStatus_Success == FLEXCAN_WriteFDTxMb(base, xfer->mbIdx, xfer->framefd))
        {
            /* Enable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
            FLEXCAN_EnableMbInterrupts(base, (uint64_t)1 << xfer->mbIdx);
#else
            FLEXCAN_EnableMbInterrupts(base, 1 << xfer->mbIdx);
#endif

            return kStatus_Success;
        }
        else
        {
            handle->mbState[xfer->mbIdx] = kFLEXCAN_StateIdle;
            return kStatus_Fail;
        }
    }
    else
    {
        return kStatus_FLEXCAN_TxBusy;
    }
}

status_t FLEXCAN_TransferFDReceiveNonBlocking(CAN_Type *base, flexcan_handle_t *handle, flexcan_mb_transfer_t *xfer)
{
    /* Assertion. */
    assert(handle);
    assert(xfer);
    assert(xfer->mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, xfer->mbIdx));

    /* Check if Message Buffer is idle. */
    if (kFLEXCAN_StateIdle == handle->mbState[xfer->mbIdx])
    {
        handle->mbState[xfer->mbIdx] = kFLEXCAN_StateRxData;

        /* Register Message Buffer. */
        handle->mbFDFrameBuf[xfer->mbIdx] = xfer->framefd;

        /* Enable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
        FLEXCAN_EnableMbInterrupts(base, (uint64_t)1 << xfer->mbIdx);
#else
        FLEXCAN_EnableMbInterrupts(base, 1 << xfer->mbIdx);
#endif

        return kStatus_Success;
    }
    else
    {
        return kStatus_FLEXCAN_RxBusy;
    }
}
#endif

status_t FLEXCAN_TransferReceiveFifoNonBlocking(CAN_Type *base, flexcan_handle_t *handle, flexcan_fifo_transfer_t *xfer)
{
    /* Assertion. */
    assert(handle);
    assert(xfer);

    /* Check if Message Buffer is idle. */
    if (kFLEXCAN_StateIdle == handle->rxFifoState)
    {
        handle->rxFifoState = kFLEXCAN_StateRxFifo;

        /* Register Message Buffer. */
        handle->rxFifoFrameBuf = xfer->frame;

        /* Enable Message Buffer Interrupt. */
        FLEXCAN_EnableMbInterrupts(
            base, kFLEXCAN_RxFifoOverflowFlag | kFLEXCAN_RxFifoWarningFlag | kFLEXCAN_RxFifoFrameAvlFlag);

        return kStatus_Success;
    }
    else
    {
        return kStatus_FLEXCAN_RxFifoBusy;
    }
}

void FLEXCAN_TransferAbortSend(CAN_Type *base, flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    assert(handle);
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    /* Disable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    FLEXCAN_DisableMbInterrupts(base, (uint64_t)1 << mbIdx);
#else
    FLEXCAN_DisableMbInterrupts(base, 1 << mbIdx);
#endif

    /* Un-register handle. */
    handle->mbFrameBuf[mbIdx] = 0x0;

    /* Clean Message Buffer. */
    FLEXCAN_SetTxMbConfig(base, mbIdx, true);

    handle->mbState[mbIdx] = kFLEXCAN_StateIdle;
}

#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
void FLEXCAN_TransferFDAbortSend(CAN_Type *base, flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    assert(handle);
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    /* Disable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    FLEXCAN_DisableMbInterrupts(base, (uint64_t)1 << mbIdx);
#else
    FLEXCAN_DisableMbInterrupts(base, 1 << mbIdx);
#endif

    /* Un-register handle. */
    handle->mbFDFrameBuf[mbIdx] = 0x0;

    /* Clean Message Buffer. */
    FLEXCAN_SetFDTxMbConfig(base, mbIdx, true);

    handle->mbState[mbIdx] = kFLEXCAN_StateIdle;
}

void FLEXCAN_TransferFDAbortReceive(CAN_Type *base, flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    assert(handle);
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    /* Disable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    FLEXCAN_DisableMbInterrupts(base, (uint64_t)1 << mbIdx);
#else
    FLEXCAN_DisableMbInterrupts(base, 1 << mbIdx);
#endif

    /* Un-register handle. */
    handle->mbFDFrameBuf[mbIdx] = 0x0;
    handle->mbState[mbIdx] = kFLEXCAN_StateIdle;
}
#endif

void FLEXCAN_TransferAbortReceive(CAN_Type *base, flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    assert(handle);
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    assert(!FLEXCAN_IsMbOccupied(base, mbIdx));

    /* Disable Message Buffer Interrupt. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    FLEXCAN_DisableMbInterrupts(base, (uint64_t)1 << mbIdx);
#else
    FLEXCAN_DisableMbInterrupts(base, 1 << mbIdx);
#endif

    /* Un-register handle. */
    handle->mbFrameBuf[mbIdx] = 0x0;
    handle->mbState[mbIdx] = kFLEXCAN_StateIdle;
}

void FLEXCAN_TransferAbortReceiveFifo(CAN_Type *base, flexcan_handle_t *handle)
{
    /* Assertion. */
    assert(handle);

    /* Check if Rx FIFO is enabled. */
    if (base->MCR & CAN_MCR_RFEN_MASK)
    {
        /* Disable Rx Message FIFO Interrupts. */
        FLEXCAN_DisableMbInterrupts(
            base, kFLEXCAN_RxFifoOverflowFlag | kFLEXCAN_RxFifoWarningFlag | kFLEXCAN_RxFifoFrameAvlFlag);

        /* Un-register handle. */
        handle->rxFifoFrameBuf = 0x0;
    }

    handle->rxFifoState = kFLEXCAN_StateIdle;
}

void FLEXCAN_TransferHandleIRQ(CAN_Type *base, flexcan_handle_t *handle)
{
    /* Assertion. */
    assert(handle);

    status_t status = kStatus_FLEXCAN_UnHandled;
    uint32_t result;

    /* Store Current FlexCAN Module Error and Status. */
    result = base->ESR1;

    do
    {
        /* Solve FlexCAN Error and Status Interrupt. */
        if (result & (kFLEXCAN_TxWarningIntFlag | kFLEXCAN_RxWarningIntFlag | kFLEXCAN_BusOffIntFlag |
                      kFLEXCAN_ErrorIntFlag | kFLEXCAN_WakeUpIntFlag))
        {
            status = kStatus_FLEXCAN_ErrorStatus;

            /* Clear FlexCAN Error and Status Interrupt. */
            FLEXCAN_ClearStatusFlags(base, kFLEXCAN_TxWarningIntFlag | kFLEXCAN_RxWarningIntFlag |
                                               kFLEXCAN_BusOffIntFlag | kFLEXCAN_ErrorIntFlag | kFLEXCAN_WakeUpIntFlag);
        }
        /* Solve FlexCAN Rx FIFO & Message Buffer Interrupt. */
        else
        {
            /* For this implementation, we solve the Message with lowest MB index first. */
            for (result = 0; result < FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base); result++)
            {
                /* Get the lowest unhandled Message Buffer */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
                if ((FLEXCAN_GetMbStatusFlags(base, (uint64_t)1 << result)) && (FLEXCAN_IsMbIntEnabled(base, result)))
#else
                if ((FLEXCAN_GetMbStatusFlags(base, 1 << result)) && (FLEXCAN_IsMbIntEnabled(base, result)))
#endif
                {
                    break;
                }
            }

            /* Does not find Message to deal with. */
            if (result == FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base))
            {
                break;
            }

            /* Solve Rx FIFO interrupt. */
            if ((kFLEXCAN_StateIdle != handle->rxFifoState) && ((1 << result) <= kFLEXCAN_RxFifoOverflowFlag))
            {
                switch (1 << result)
                {
                    case kFLEXCAN_RxFifoOverflowFlag:
                        status = kStatus_FLEXCAN_RxFifoOverflow;
                        break;

                    case kFLEXCAN_RxFifoWarningFlag:
                        status = kStatus_FLEXCAN_RxFifoWarning;
                        break;

                    case kFLEXCAN_RxFifoFrameAvlFlag:
                        status = FLEXCAN_ReadRxFifo(base, handle->rxFifoFrameBuf);
                        if (kStatus_Success == status)
                        {
                            status = kStatus_FLEXCAN_RxFifoIdle;
                        }
                        FLEXCAN_TransferAbortReceiveFifo(base, handle);
                        break;

                    default:
                        status = kStatus_FLEXCAN_UnHandled;
                        break;
                }
            }
            else
            {
                /* Get current State of Message Buffer. */
                switch (handle->mbState[result])
                {
                    /* Solve Rx Data Frame. */
                    case kFLEXCAN_StateRxData:
#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
                        if (base->MCR & CAN_MCR_FDEN_MASK)
                        {
                            status = FLEXCAN_ReadFDRxMb(base, result, handle->mbFDFrameBuf[result]);
                        }
                        else
                        {
                            status = FLEXCAN_ReadRxMb(base, result, handle->mbFrameBuf[result]);
                        }
#else
                        status = FLEXCAN_ReadRxMb(base, result, handle->mbFrameBuf[result]);
#endif
                        if (kStatus_Success == status)
                        {
                            status = kStatus_FLEXCAN_RxIdle;
                        }
#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
                        if (base->MCR & CAN_MCR_FDEN_MASK)
                        {
                            FLEXCAN_TransferFDAbortReceive(base, handle, result);
                        }
                        else
                        {
                            FLEXCAN_TransferAbortReceive(base, handle, result);
                        }
#else
                        FLEXCAN_TransferAbortReceive(base, handle, result);
#endif
                        break;

                    /* Solve Rx Remote Frame. */
                    case kFLEXCAN_StateRxRemote:
                        status = FLEXCAN_ReadRxMb(base, result, handle->mbFrameBuf[result]);
                        if (kStatus_Success == status)
                        {
                            status = kStatus_FLEXCAN_RxIdle;
                        }
                        FLEXCAN_TransferAbortReceive(base, handle, result);
                        break;

                    /* Solve Tx Data Frame. */
                    case kFLEXCAN_StateTxData:
                        status = kStatus_FLEXCAN_TxIdle;
#if (defined(FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE) && FSL_FEATURE_FLEXCAN_HAS_FLEXIBLE_DATA_RATE)
                        if (base->MCR & CAN_MCR_FDEN_MASK)
                        {
                            FLEXCAN_TransferFDAbortSend(base, handle, result);
                        }
                        else
                        {
                            FLEXCAN_TransferAbortSend(base, handle, result);
                        }
#else
                        FLEXCAN_TransferAbortSend(base, handle, result);
#endif
                        break;

                    /* Solve Tx Remote Frame. */
                    case kFLEXCAN_StateTxRemote:
                        handle->mbState[result] = kFLEXCAN_StateRxRemote;
                        status = kStatus_FLEXCAN_TxSwitchToRx;
                        break;

                    default:
                        status = kStatus_FLEXCAN_UnHandled;
                        break;
                }
            }

            /* Clear resolved Message Buffer IRQ. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
            FLEXCAN_ClearMbStatusFlags(base, (uint64_t)1 << result);
#else
            FLEXCAN_ClearMbStatusFlags(base, 1 << result);
#endif
        }

        /* Calling Callback Function if has one. */
        if (handle->callback != NULL)
        {
            handle->callback(base, handle, status, result, handle->userData);
        }

        /* Reset return status */
        status = kStatus_FLEXCAN_UnHandled;

        /* Store Current FlexCAN Module Error and Status. */
        result = base->ESR1;
    }
#if (defined(FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER)) && (FSL_FEATURE_FLEXCAN_HAS_EXTENDED_FLAG_REGISTER > 0)
    while ((0 != FLEXCAN_GetMbStatusFlags(base, 0xFFFFFFFFFFFFFFFFU)) ||
           (0 != (result & (kFLEXCAN_TxWarningIntFlag | kFLEXCAN_RxWarningIntFlag | kFLEXCAN_BusOffIntFlag |
                            kFLEXCAN_ErrorIntFlag | kFLEXCAN_WakeUpIntFlag))));
#else
    while ((0 != FLEXCAN_GetMbStatusFlags(base, 0xFFFFFFFFU)) ||
           (0 != (result & (kFLEXCAN_TxWarningIntFlag | kFLEXCAN_RxWarningIntFlag | kFLEXCAN_BusOffIntFlag |
                            kFLEXCAN_ErrorIntFlag | kFLEXCAN_WakeUpIntFlag))));
#endif
}

#if defined(CAN0)
void CAN0_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[0]);

    s_flexcanIsr(CAN0, s_flexcanHandle[0]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CAN1)
void CAN1_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[1]);

    s_flexcanIsr(CAN1, s_flexcanHandle[1]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CAN2)
void CAN2_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[2]);

    s_flexcanIsr(CAN2, s_flexcanHandle[2]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CAN3)
void CAN3_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[3]);

    s_flexcanIsr(CAN3, s_flexcanHandle[3]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(CAN4)
void CAN4_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[4]);

    s_flexcanIsr(CAN4, s_flexcanHandle[4]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(DMA__CAN0)
void DMA_FLEXCAN0_INT_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[FLEXCAN_GetInstance(DMA__CAN0)]);

    s_flexcanIsr(DMA__CAN0, s_flexcanHandle[FLEXCAN_GetInstance(DMA__CAN0)]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(DMA__CAN1)
void DMA_FLEXCAN1_INT_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[FLEXCAN_GetInstance(DMA__CAN1)]);

    s_flexcanIsr(DMA__CAN1, s_flexcanHandle[FLEXCAN_GetInstance(DMA__CAN1)]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(DMA__CAN2)
void DMA_FLEXCAN2_INT_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[FLEXCAN_GetInstance(DMA__CAN2)]);

    s_flexcanIsr(DMA__CAN2, s_flexcanHandle[FLEXCAN_GetInstance(DMA__CAN2)]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(ADMA__CAN0)
void ADMA_FLEXCAN0_INT_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[FLEXCAN_GetInstance(ADMA__CAN0)]);

    s_flexcanIsr(ADMA__CAN0, s_flexcanHandle[FLEXCAN_GetInstance(ADMA__CAN0)]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(ADMA__CAN1)
void ADMA_FLEXCAN1_INT_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[FLEXCAN_GetInstance(ADMA__CAN1)]);

    s_flexcanIsr(ADMA__CAN1, s_flexcanHandle[FLEXCAN_GetInstance(ADMA__CAN1)]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

#if defined(ADMA__CAN2)
void ADMA_FLEXCAN2_INT_DriverIRQHandler(void)
{
    assert(s_flexcanHandle[FLEXCAN_GetInstance(ADMA__CAN2)]);

    s_flexcanIsr(ADMA__CAN2, s_flexcanHandle[FLEXCAN_GetInstance(ADMA__CAN2)]);
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif
