/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32a455_usart.c
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#include "n32a455_usart.h"
#include "n32a455_rcc.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup USART
 * @brief USART driver modules
 * @{
 */

/** @addtogroup USART_Private_TypesDefinitions
 * @{
 */

/**
 * @}
 */

/** @addtogroup USART_Private_Defines
 * @{
 */

#define CTRL1_UEN_SET   ((uint16_t)0x2000) /*!< USART Enable Mask */
#define CTRL1_UEN_RESET ((uint16_t)0xDFFF) /*!< USART Disable Mask */

#define CTRL1_WUM_MASK ((uint16_t)0xF7FF) /*!< USART WakeUp Method Mask */

#define CTRL1_RCVWU_SET   ((uint16_t)0x0002) /*!< USART mute mode Enable Mask */
#define CTRL1_RCVWU_RESET ((uint16_t)0xFFFD) /*!< USART mute mode Enable Mask */
#define CTRL1_SDBRK_SET   ((uint16_t)0x0001) /*!< USART Break Character send Mask */
#define CTRL1_CLR_MASK    ((uint16_t)0xE9F3) /*!< USART CTRL1 Mask */
#define CTRL2_ADDR_MASK   ((uint16_t)0xFFF0) /*!< USART address Mask */

#define CTRL2_LINMEN_SET   ((uint16_t)0x4000) /*!< USART LIN Enable Mask */
#define CTRL2_LINMEN_RESET ((uint16_t)0xBFFF) /*!< USART LIN Disable Mask */

#define CTRL2_LINBDL_MASK    ((uint16_t)0xFFDF) /*!< USART LIN Break detection Mask */
#define CTRL2_STPB_CLR_MASK  ((uint16_t)0xCFFF) /*!< USART CTRL2 STOP Bits Mask */
#define CTRL2_CLOCK_CLR_MASK ((uint16_t)0xF0FF) /*!< USART CTRL2 Clock Mask */

#define CTRL3_SCMEN_SET   ((uint16_t)0x0020) /*!< USART SC Enable Mask */
#define CTRL3_SCMEN_RESET ((uint16_t)0xFFDF) /*!< USART SC Disable Mask */

#define CTRL3_SCNACK_SET   ((uint16_t)0x0010) /*!< USART SC NACK Enable Mask */
#define CTRL3_SCNACK_RESET ((uint16_t)0xFFEF) /*!< USART SC NACK Disable Mask */

#define CTRL3_HDMEN_SET   ((uint16_t)0x0008) /*!< USART Half-Duplex Enable Mask */
#define CTRL3_HDMEN_RESET ((uint16_t)0xFFF7) /*!< USART Half-Duplex Disable Mask */

#define CTRL3_IRDALP_MASK ((uint16_t)0xFFFB) /*!< USART IrDA LowPower mode Mask */
#define CTRL3_CLR_MASK    ((uint16_t)0xFCFF) /*!< USART CTRL3 Mask */

#define CTRL3_IRDAMEN_SET   ((uint16_t)0x0002) /*!< USART IrDA Enable Mask */
#define CTRL3_IRDAMEN_RESET ((uint16_t)0xFFFD) /*!< USART IrDA Disable Mask */
#define GTP_LSB_MASK        ((uint16_t)0x00FF) /*!< Guard Time Register LSB Mask */
#define GTP_MSB_MASK        ((uint16_t)0xFF00) /*!< Guard Time Register MSB Mask */
#define INT_MASK            ((uint16_t)0x001F) /*!< USART Interrupt Mask */

/**
 * @}
 */

/** @addtogroup USART_Private_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup USART_Private_Variables
 * @{
 */

/**
 * @}
 */

/** @addtogroup USART_Private_FunctionPrototypes
 * @{
 */

/**
 * @}
 */

/** @addtogroup USART_Private_Functions
 * @{
 */

/**
 * @brief  Deinitializes the USARTx peripheral registers to their default reset values.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *      USART1, USART2, USART3, UART4 or UART5.
 */
void USART_DeInit(USART_Module* USARTx)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));

    if (USARTx == USART1)
    {
        RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_USART1, ENABLE);
        RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_USART1, DISABLE);
    }
    else if (USARTx == USART2)
    {
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_USART2, ENABLE);
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_USART2, DISABLE);
    }
    else if (USARTx == USART3)
    {
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_USART3, ENABLE);
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_USART3, DISABLE);
    }
    else if (USARTx == UART4)
    {
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_UART4, ENABLE);
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_UART4, DISABLE);
    }
    else if (USARTx == UART5)
    {
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_UART5, ENABLE);
        RCC_EnableAPB1PeriphReset(RCC_APB1_PERIPH_UART5, DISABLE);
    }
    else if (USARTx == UART6)
    {
        RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_UART6, ENABLE);
        RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_UART6, DISABLE);
    }
    else
    {
        if (USARTx == UART7)
        {
            RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_UART7, ENABLE);
            RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_UART7, DISABLE);
        }
    }
}

/**
 * @brief  Initializes the USARTx peripheral according to the specified
 *         parameters in the USART_InitStruct .
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4 or UART5.
 * @param USART_InitStruct pointer to a USART_InitType structure
 *         that contains the configuration information for the specified USART
 *         peripheral.
 */
void USART_Init(USART_Module* USARTx, USART_InitType* USART_InitStruct)
{
    uint32_t tmpregister = 0x00, apbclock = 0x00;
    uint32_t integerdivider    = 0x00;
    uint32_t fractionaldivider = 0x00;
    uint32_t usartxbase        = 0;
    RCC_ClocksType RCC_ClocksStatus;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_BAUDRATE(USART_InitStruct->BaudRate));
    assert_param(IS_USART_WORD_LENGTH(USART_InitStruct->WordLength));
    assert_param(IS_USART_STOPBITS(USART_InitStruct->StopBits));
    assert_param(IS_USART_PARITY(USART_InitStruct->Parity));
    assert_param(IS_USART_MODE(USART_InitStruct->Mode));
    assert_param(IS_USART_HARDWARE_FLOW_CONTROL(USART_InitStruct->HardwareFlowControl));
    /* The hardware flow control is available only for USART1, USART2 and USART3 */
    if (USART_InitStruct->HardwareFlowControl != USART_HFCTRL_NONE)
    {
        assert_param(IS_USART_123_PERIPH(USARTx));
    }

    usartxbase = (uint32_t)USARTx;

    /*---------------------------- USART CTRL2 Configuration -----------------------*/
    tmpregister = USARTx->CTRL2;
    /* Clear STOP[13:12] bits */
    tmpregister &= CTRL2_STPB_CLR_MASK;
    /* Configure the USART Stop Bits, Clock, CPOL, CPHA and LastBit ------------*/
    /* Set STOP[13:12] bits according to StopBits value */
    tmpregister |= (uint32_t)USART_InitStruct->StopBits;

    /* Write to USART CTRL2 */
    USARTx->CTRL2 = (uint16_t)tmpregister;

    /*---------------------------- USART CTRL1 Configuration -----------------------*/
    tmpregister = USARTx->CTRL1;
    /* Clear M, PCE, PS, TE and RE bits */
    tmpregister &= CTRL1_CLR_MASK;
    /* Configure the USART Word Length, Parity and mode ----------------------- */
    /* Set the M bits according to WordLength value */
    /* Set PCE and PS bits according to Parity value */
    /* Set TE and RE bits according to Mode value */
    tmpregister |= (uint32_t)USART_InitStruct->WordLength | USART_InitStruct->Parity | USART_InitStruct->Mode;
    /* Write to USART CTRL1 */
    USARTx->CTRL1 = (uint16_t)tmpregister;

    /*---------------------------- USART CTRL3 Configuration -----------------------*/
    tmpregister = USARTx->CTRL3;
    /* Clear CTSE and RTSE bits */
    tmpregister &= CTRL3_CLR_MASK;
    /* Configure the USART HFC -------------------------------------------------*/
    /* Set CTSE and RTSE bits according to HardwareFlowControl value */
    tmpregister |= USART_InitStruct->HardwareFlowControl;
    /* Write to USART CTRL3 */
    USARTx->CTRL3 = (uint16_t)tmpregister;

    /*---------------------------- USART PBC Configuration -----------------------*/
    /* Configure the USART Baud Rate -------------------------------------------*/
    RCC_GetClocksFreqValue(&RCC_ClocksStatus);
    if ((usartxbase == USART1_BASE) || (usartxbase == UART6_BASE) || (usartxbase == UART7_BASE))
    {
        apbclock = RCC_ClocksStatus.Pclk2Freq;
    }
    else
    {
        apbclock = RCC_ClocksStatus.Pclk1Freq;
    }

    /* Determine the integer part */
    integerdivider = ((25 * apbclock) / (4 * (USART_InitStruct->BaudRate)));
    tmpregister = (integerdivider / 100) << 4;

    /* Determine the fractional part */
    fractionaldivider = (((((integerdivider - (100 * (tmpregister >> 4))) * 16) + 50) / 100));

    /*Determine whether the fractional part needs to carried*/
    if((fractionaldivider >> 4) == 1){
        tmpregister = ((integerdivider / 100) + 1) << 4;
    }
    
    /* Implement the fractional part in the register */
    tmpregister |= fractionaldivider & ((uint8_t)0x0F);

    /* Write to USART PBC */
    USARTx->BRCF = (uint16_t)tmpregister;
}

/**
 * @brief  Fills each USART_InitStruct member with its default value.
 * @param USART_InitStruct pointer to a USART_InitType structure
 *         which will be initialized.
 */
void USART_StructInit(USART_InitType* USART_InitStruct)
{
    /* USART_InitStruct members default value */
    USART_InitStruct->BaudRate            = 9600;
    USART_InitStruct->WordLength          = USART_WL_8B;
    USART_InitStruct->StopBits            = USART_STPB_1;
    USART_InitStruct->Parity              = USART_PE_NO;
    USART_InitStruct->Mode                = USART_MODE_RX | USART_MODE_TX;
    USART_InitStruct->HardwareFlowControl = USART_HFCTRL_NONE;
}

/**
 * @brief  Initializes the USARTx peripheral Clock according to the
 *          specified parameters in the USART_ClockInitStruct .
 * @param USARTx where x can be 1, 2, 3 to select the USART peripheral.
 * @param USART_ClockInitStruct pointer to a USART_ClockInitType
 *         structure that contains the configuration information for the specified
 *         USART peripheral.
 * @note The Smart Card and Synchronous modes are not available for UART4/UART5/UART6/UART7.
 */
void USART_ClockInit(USART_Module* USARTx, USART_ClockInitType* USART_ClockInitStruct)
{
    uint32_t tmpregister = 0x00;
    /* Check the parameters */
    assert_param(IS_USART_123_PERIPH(USARTx));
    assert_param(IS_USART_CLOCK(USART_ClockInitStruct->Clock));
    assert_param(IS_USART_CPOL(USART_ClockInitStruct->Polarity));
    assert_param(IS_USART_CPHA(USART_ClockInitStruct->Phase));
    assert_param(IS_USART_LASTBIT(USART_ClockInitStruct->LastBit));

    /*---------------------------- USART CTRL2 Configuration -----------------------*/
    tmpregister = USARTx->CTRL2;
    /* Clear CLKEN, CPOL, CPHA and LBCL bits */
    tmpregister &= CTRL2_CLOCK_CLR_MASK;
    /* Configure the USART Clock, CPOL, CPHA and LastBit ------------*/
    /* Set CLKEN bit according to Clock value */
    /* Set CPOL bit according to Polarity value */
    /* Set CPHA bit according to Phase value */
    /* Set LBCL bit according to LastBit value */
    tmpregister |= (uint32_t)USART_ClockInitStruct->Clock | USART_ClockInitStruct->Polarity
                   | USART_ClockInitStruct->Phase | USART_ClockInitStruct->LastBit;
    /* Write to USART CTRL2 */
    USARTx->CTRL2 = (uint16_t)tmpregister;
}

/**
 * @brief  Fills each USART_ClockInitStruct member with its default value.
 * @param USART_ClockInitStruct pointer to a USART_ClockInitType
 *         structure which will be initialized.
 */
void USART_ClockStructInit(USART_ClockInitType* USART_ClockInitStruct)
{
    /* USART_ClockInitStruct members default value */
    USART_ClockInitStruct->Clock    = USART_CLK_DISABLE;
    USART_ClockInitStruct->Polarity = USART_CLKPOL_LOW;
    USART_ClockInitStruct->Phase    = USART_CLKPHA_1EDGE;
    USART_ClockInitStruct->LastBit  = USART_CLKLB_DISABLE;
}

/**
 * @brief  Enables or disables the specified USART peripheral.
 * @param USARTx Select the USART or the UART peripheral.
 *         This parameter can be one of the following values:
 *           USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param Cmd new state of the USARTx peripheral.
 *         This parameter can be: ENABLE or DISABLE.
 */
void USART_Enable(USART_Module* USARTx, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    if (Cmd != DISABLE)
    {
        /* Enable the selected USART by setting the UE bit in the CTRL1 register */
        USARTx->CTRL1 |= CTRL1_UEN_SET;
    }
    else
    {
        /* Disable the selected USART by clearing the UE bit in the CTRL1 register */
        USARTx->CTRL1 &= CTRL1_UEN_RESET;
    }
}

/**
 * @brief  Enables or disables the specified USART interrupts.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_INT specifies the USART interrupt sources to be enabled or disabled.
 *   This parameter can be one of the following values:
 *     @arg USART_INT_CTSF CTS change interrupt (not available for UART4 and UART5)
 *     @arg USART_INT_LINBD LIN Break detection interrupt
 *     @arg USART_INT_TXDE Transmit Data Register empty interrupt
 *     @arg USART_INT_TXC Transmission complete interrupt
 *     @arg USART_INT_RXDNE Receive Data register not empty interrupt
 *     @arg USART_INT_IDLEF Idle line detection interrupt
 *     @arg USART_INT_PEF Parity Error interrupt
 *     @arg USART_INT_ERRF Error interrupt(Frame error, noise error, overrun error)
 * @param Cmd new state of the specified USARTx interrupts.
 *   This parameter can be: ENABLE or DISABLE.
 */
void USART_ConfigInt(USART_Module* USARTx, uint16_t USART_INT, FunctionalState Cmd)
{
    uint32_t usartreg = 0x00, itpos = 0x00, itmask = 0x00;
    uint32_t usartxbase = 0x00;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_CFG_INT(USART_INT));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    /* The CTS interrupt is not available for UART4/UART5/UART6/UART7 */
    if (USART_INT == USART_INT_CTSF)
    {
        assert_param(IS_USART_123_PERIPH(USARTx));
    }

    usartxbase = (uint32_t)USARTx;

    /* Get the USART register index */
    usartreg = (((uint8_t)USART_INT) >> 0x05);

    /* Get the interrupt position */
    itpos  = USART_INT & INT_MASK;
    itmask = (((uint32_t)0x01) << itpos);

    if (usartreg == 0x01) /* The IT is in CTRL1 register */
    {
        usartxbase += 0x0C;
    }
    else if (usartreg == 0x02) /* The IT is in CTRL2 register */
    {
        usartxbase += 0x10;
    }
    else /* The IT is in CTRL3 register */
    {
        usartxbase += 0x14;
    }
    if (Cmd != DISABLE)
    {
        *(__IO uint32_t*)usartxbase |= itmask;
    }
    else
    {
        *(__IO uint32_t*)usartxbase &= ~itmask;
    }
}

/**
 * @brief  Enables or disables the USART's DMA interface.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_DMAReq specifies the DMA request.
 *   This parameter can be any combination of the following values:
 *     @arg USART_DMAREQ_TX USART DMA transmit request
 *     @arg USART_DMAREQ_RX USART DMA receive request
 * @param Cmd new state of the DMA Request sources.
 *   This parameter can be: ENABLE or DISABLE.
 */
void USART_EnableDMA(USART_Module* USARTx, uint16_t USART_DMAReq, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_DMAREQ(USART_DMAReq));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        /* Enable the DMA transfer for selected requests by setting the DMAT and/or
           DADDR bits in the USART CTRL3 register */
        USARTx->CTRL3 |= USART_DMAReq;
    }
    else
    {
        /* Disable the DMA transfer for selected requests by clearing the DMAT and/or
           DADDR bits in the USART CTRL3 register */
        USARTx->CTRL3 &= (uint16_t)~USART_DMAReq;
    }
}

/**
 * @brief  Sets the address of the USART node.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_Addr Indicates the address of the USART node.
 */
void USART_SetAddr(USART_Module* USARTx, uint8_t USART_Addr)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_ADDRESS(USART_Addr));

    /* Clear the USART address */
    USARTx->CTRL2 &= CTRL2_ADDR_MASK;
    /* Set the USART address node */
    USARTx->CTRL2 |= USART_Addr;
}

/**
 * @brief  Selects the USART WakeUp method.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_WakeUpMode specifies the USART wakeup method.
 *   This parameter can be one of the following values:
 *     @arg USART_WUM_IDLELINE WakeUp by an idle line detection
 *     @arg USART_WUM_ADDRMASK WakeUp by an address mark
 */
void USART_ConfigWakeUpMode(USART_Module* USARTx, uint16_t USART_WakeUpMode)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_WAKEUP(USART_WakeUpMode));

    USARTx->CTRL1 &= CTRL1_WUM_MASK;
    USARTx->CTRL1 |= USART_WakeUpMode;
}

/**
 * @brief  Determines if the USART is in mute mode or not.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param Cmd new state of the USART mute mode.
 *   This parameter can be: ENABLE or DISABLE.
 */
void USART_EnableRcvWakeUp(USART_Module* USARTx, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    if (Cmd != DISABLE)
    {
        /* Enable the USART mute mode  by setting the RWU bit in the CTRL1 register */
        USARTx->CTRL1 |= CTRL1_RCVWU_SET;
    }
    else
    {
        /* Disable the USART mute mode by clearing the RWU bit in the CTRL1 register */
        USARTx->CTRL1 &= CTRL1_RCVWU_RESET;
    }
}

/**
 * @brief  Sets the USART LIN Break detection length.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_LINBreakDetectLength specifies the LIN break detection length.
 *   This parameter can be one of the following values:
 *     @arg USART_LINBDL_10B 10-bit break detection
 *     @arg USART_LINBDL_11B 11-bit break detection
 */
void USART_ConfigLINBreakDetectLength(USART_Module* USARTx, uint16_t USART_LINBreakDetectLength)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_LIN_BREAK_DETECT_LENGTH(USART_LINBreakDetectLength));

    USARTx->CTRL2 &= CTRL2_LINBDL_MASK;
    USARTx->CTRL2 |= USART_LINBreakDetectLength;
}

/**
 * @brief  Enables or disables the USART's LIN mode.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param Cmd new state of the USART LIN mode.
 *   This parameter can be: ENABLE or DISABLE.
 */
void USART_EnableLIN(USART_Module* USARTx, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    if (Cmd != DISABLE)
    {
        /* Enable the LIN mode by setting the LINEN bit in the CTRL2 register */
        USARTx->CTRL2 |= CTRL2_LINMEN_SET;
    }
    else
    {
        /* Disable the LIN mode by clearing the LINEN bit in the CTRL2 register */
        USARTx->CTRL2 &= CTRL2_LINMEN_RESET;
    }
}

/**
 * @brief  Transmits single data through the USARTx peripheral.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param Data the data to transmit.
 */
void USART_SendData(USART_Module* USARTx, uint16_t Data)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_DATA(Data));

    /* Transmit Data */
    USARTx->DAT = (Data & (uint16_t)0x01FF);
}

/**
 * @brief  Returns the most recent received data by the USARTx peripheral.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @return The received data.
 */
uint16_t USART_ReceiveData(USART_Module* USARTx)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));

    /* Receive Data */
    return (uint16_t)(USARTx->DAT & (uint16_t)0x01FF);
}

/**
 * @brief  Transmits break characters.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 */
void USART_SendBreak(USART_Module* USARTx)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));

    /* Send break characters */
    USARTx->CTRL1 |= CTRL1_SDBRK_SET;
}

/**
 * @brief  Sets the specified USART guard time.
 * @param USARTx where x can be 1, 2 or 3 to select the USART peripheral.
 * @param USART_GuardTime specifies the guard time.
 * @note The guard time bits are not available for UART4/UART5/UART6/UART7.
 */
void USART_SetGuardTime(USART_Module* USARTx, uint8_t USART_GuardTime)
{
    /* Check the parameters */
    assert_param(IS_USART_123_PERIPH(USARTx));

    /* Clear the USART Guard time */
    USARTx->GTP &= GTP_LSB_MASK;
    /* Set the USART guard time */
    USARTx->GTP |= (uint16_t)((uint16_t)USART_GuardTime << 0x08);
}

/**
 * @brief  Sets the system clock prescaler.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_Prescaler specifies the prescaler clock.
 * @note   The function is used for IrDA mode with UART4 and UART5.
 */
void USART_SetPrescaler(USART_Module* USARTx, uint8_t USART_Prescaler)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));

    /* Clear the USART prescaler */
    USARTx->GTP &= GTP_MSB_MASK;
    /* Set the USART prescaler */
    USARTx->GTP |= USART_Prescaler;
}

/**
 * @brief  Enables or disables the USART's Smart Card mode.
 * @param USARTx where x can be 1, 2 or 3 to select the USART peripheral.
 * @param Cmd new state of the Smart Card mode.
 *   This parameter can be: ENABLE or DISABLE.
 * @note The Smart Card mode is not available for UART4/UART5/UART6/UART7.
 */
void USART_EnableSmartCard(USART_Module* USARTx, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_123_PERIPH(USARTx));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        /* Enable the SC mode by setting the SCEN bit in the CTRL3 register */
        USARTx->CTRL3 |= CTRL3_SCMEN_SET;
    }
    else
    {
        /* Disable the SC mode by clearing the SCEN bit in the CTRL3 register */
        USARTx->CTRL3 &= CTRL3_SCMEN_RESET;
    }
}

/**
 * @brief  Enables or disables NACK transmission.
 * @param USARTx where x can be 1, 2 or 3 to select the USART peripheral.
 * @param Cmd new state of the NACK transmission.
 *   This parameter can be: ENABLE or DISABLE.
 * @note The Smart Card mode is not available for UART4/UART5/UART6/UART7.
 */
void USART_SetSmartCardNACK(USART_Module* USARTx, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_123_PERIPH(USARTx));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));
    if (Cmd != DISABLE)
    {
        /* Enable the NACK transmission by setting the NACK bit in the CTRL3 register */
        USARTx->CTRL3 |= CTRL3_SCNACK_SET;
    }
    else
    {
        /* Disable the NACK transmission by clearing the NACK bit in the CTRL3 register */
        USARTx->CTRL3 &= CTRL3_SCNACK_RESET;
    }
}

/**
 * @brief  Enables or disables the USART's Half Duplex communication.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param Cmd new state of the USART Communication.
 *   This parameter can be: ENABLE or DISABLE.
 */
void USART_EnableHalfDuplex(USART_Module* USARTx, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    if (Cmd != DISABLE)
    {
        /* Enable the Half-Duplex mode by setting the HDSEL bit in the CTRL3 register */
        USARTx->CTRL3 |= CTRL3_HDMEN_SET;
    }
    else
    {
        /* Disable the Half-Duplex mode by clearing the HDSEL bit in the CTRL3 register */
        USARTx->CTRL3 &= CTRL3_HDMEN_RESET;
    }
}

/**
 * @brief  Configures the USART's IrDA interface.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_IrDAMode specifies the IrDA mode.
 *   This parameter can be one of the following values:
 *     @arg USART_IRDAMODE_LOWPPWER
 *     @arg USART_IRDAMODE_NORMAL
 */
void USART_ConfigIrDAMode(USART_Module* USARTx, uint16_t USART_IrDAMode)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_IRDA_MODE(USART_IrDAMode));

    USARTx->CTRL3 &= CTRL3_IRDALP_MASK;
    USARTx->CTRL3 |= USART_IrDAMode;
}

/**
 * @brief  Enables or disables the USART's IrDA interface.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param Cmd new state of the IrDA mode.
 *   This parameter can be: ENABLE or DISABLE.
 */
void USART_EnableIrDA(USART_Module* USARTx, FunctionalState Cmd)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_FUNCTIONAL_STATE(Cmd));

    if (Cmd != DISABLE)
    {
        /* Enable the IrDA mode by setting the IREN bit in the CTRL3 register */
        USARTx->CTRL3 |= CTRL3_IRDAMEN_SET;
    }
    else
    {
        /* Disable the IrDA mode by clearing the IREN bit in the CTRL3 register */
        USARTx->CTRL3 &= CTRL3_IRDAMEN_RESET;
    }
}

/**
 * @brief  Checks whether the specified USART flag is set or not.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_FLAG specifies the flag to check.
 *   This parameter can be one of the following values:
 *     @arg USART_FLAG_CTSF CTS Change flag (not available for UART4 and UART5)
 *     @arg USART_FLAG_LINBD LIN Break detection flag
 *     @arg USART_FLAG_TXDE Transmit data register empty flag
 *     @arg USART_FLAG_TXC Transmission Complete flag
 *     @arg USART_FLAG_RXDNE Receive data register not empty flag
 *     @arg USART_FLAG_IDLEF Idle Line detection flag
 *     @arg USART_FLAG_OREF OverRun Error flag
 *     @arg USART_FLAG_NEF Noise Error flag
 *     @arg USART_FLAG_FEF Framing Error flag
 *     @arg USART_FLAG_PEF Parity Error flag
 * @return The new state of USART_FLAG (SET or RESET).
 */
FlagStatus USART_GetFlagStatus(USART_Module* USARTx, uint16_t USART_FLAG)
{
    FlagStatus bitstatus = RESET;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_FLAG(USART_FLAG));
    /* The CTS flag is not available for UART4/UART5/UART6/UART7 */
    if (USART_FLAG == USART_FLAG_CTSF)
    {
        assert_param(IS_USART_123_PERIPH(USARTx));
    }

    if ((USARTx->STS & USART_FLAG) != (uint16_t)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }
    return bitstatus;
}

/**
 * @brief  Clears the USARTx's pending flags.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_FLAG specifies the flag to clear.
 *   This parameter can be any combination of the following values:
 *     @arg USART_FLAG_CTSF CTS Change flag (not available for UART4 and UART5).
 *     @arg USART_FLAG_LINBD LIN Break detection flag.
 *     @arg USART_FLAG_TXC Transmission Complete flag.
 *     @arg USART_FLAG_RXDNE Receive data register not empty flag.
 *
 * @note
 *   - PE (Parity error), FE (Framing error), NE (Noise error), ORE (OverRun
 *     error) and IDLE (Idle line detected) flags are cleared by software
 *     sequence: a read operation to USART_SR register (USART_GetFlagStatus())
 *     followed by a read operation to USART_DR register (USART_ReceiveData()).
 *   - RXNE flag can be also cleared by a read to the USART_DR register
 *     (USART_ReceiveData()).
 *   - TC flag can be also cleared by software sequence: a read operation to
 *     USART_SR register (USART_GetFlagStatus()) followed by a write operation
 *     to USART_DR register (USART_SendData()).
 *   - TXE flag is cleared only by a write to the USART_DR register
 *     (USART_SendData()).
 */
void USART_ClrFlag(USART_Module* USARTx, uint16_t USART_FLAG)
{
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_CLEAR_FLAG(USART_FLAG));
    /* The CTS flag is not available for UART4/UART5/UART6/UART7 */
    if ((USART_FLAG & USART_FLAG_CTSF) == USART_FLAG_CTSF)
    {
        assert_param(IS_USART_123_PERIPH(USARTx));
    }

    USARTx->STS = (uint16_t)~USART_FLAG;
}

/**
 * @brief  Checks whether the specified USART interrupt has occurred or not.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_INT specifies the USART interrupt source to check.
 *   This parameter can be one of the following values:
 *     @arg USART_INT_CTSF CTS change interrupt (not available for UART4 and UART5)
 *     @arg USART_INT_LINBD LIN Break detection interrupt
 *     @arg USART_INT_TXDE Tansmit Data Register empty interrupt
 *     @arg USART_INT_TXC Transmission complete interrupt
 *     @arg USART_INT_RXDNE Receive Data register not empty interrupt
 *     @arg USART_INT_IDLEF Idle line detection interrupt
 *     @arg USART_INT_OREF OverRun Error interrupt
 *     @arg USART_INT_NEF Noise Error interrupt
 *     @arg USART_INT_FEF Framing Error interrupt
 *     @arg USART_INT_PEF Parity Error interrupt
 * @return The new state of USART_INT (SET or RESET).
 */
INTStatus USART_GetIntStatus(USART_Module* USARTx, uint16_t USART_INT)
{
    uint32_t bitpos = 0x00, itmask = 0x00, usartreg = 0x00;
    INTStatus bitstatus = RESET;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_GET_INT(USART_INT));
    /* The CTS interrupt is not available for UART4/UART5/UART6/UART7 */
    if (USART_INT == USART_INT_CTSF)
    {
        assert_param(IS_USART_123_PERIPH(USARTx));
    }

    /* Get the USART register index */
    usartreg = (((uint8_t)USART_INT) >> 0x05);
    /* Get the interrupt position */
    itmask = USART_INT & INT_MASK;
    itmask = (uint32_t)0x01 << itmask;

    if (usartreg == 0x01) /* The IT  is in CTRL1 register */
    {
        itmask &= USARTx->CTRL1;
    }
    else if (usartreg == 0x02) /* The IT  is in CTRL2 register */
    {
        itmask &= USARTx->CTRL2;
    }
    else /* The IT  is in CTRL3 register */
    {
        itmask &= USARTx->CTRL3;
    }

    bitpos = USART_INT >> 0x08;
    bitpos = (uint32_t)0x01 << bitpos;
    bitpos &= USARTx->STS;
    if ((itmask != (uint16_t)RESET) && (bitpos != (uint16_t)RESET))
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }

    return bitstatus;
}

/**
 * @brief  Clears the USARTx's interrupt pending bits.
 * @param USARTx Select the USART or the UART peripheral.
 *   This parameter can be one of the following values:
 *   USART1, USART2, USART3, UART4, UART5, UART6 or UART7.
 * @param USART_INT specifies the interrupt pending bit to clear.
 *   This parameter can be one of the following values:
 *     @arg USART_INT_CTSF CTS change interrupt (not available for UART4 and UART5)
 *     @arg USART_INT_LINBD LIN Break detection interrupt
 *     @arg USART_INT_TXC Transmission complete interrupt.
 *     @arg USART_INT_RXDNE Receive Data register not empty interrupt.
 *
 * @note
 *   - PE (Parity error), FE (Framing error), NE (Noise error), ORE (OverRun
 *     error) and IDLE (Idle line detected) pending bits are cleared by
 *     software sequence: a read operation to USART_SR register
 *     (USART_GetIntStatus()) followed by a read operation to USART_DR register
 *     (USART_ReceiveData()).
 *   - RXNE pending bit can be also cleared by a read to the USART_DR register
 *     (USART_ReceiveData()).
 *   - TC pending bit can be also cleared by software sequence: a read
 *     operation to USART_SR register (USART_GetIntStatus()) followed by a write
 *     operation to USART_DR register (USART_SendData()).
 *   - TXE pending bit is cleared only by a write to the USART_DR register
 *     (USART_SendData()).
 */
void USART_ClrIntPendingBit(USART_Module* USARTx, uint16_t USART_INT)
{
    uint16_t bitpos = 0x00, itmask = 0x00;
    /* Check the parameters */
    assert_param(IS_USART_ALL_PERIPH(USARTx));
    assert_param(IS_USART_CLR_INT(USART_INT));
    /* The CTS interrupt is not available for UART4/UART5/UART6/UART7 */
    if (USART_INT == USART_INT_CTSF)
    {
        assert_param(IS_USART_123_PERIPH(USARTx));
    }

    bitpos      = USART_INT >> 0x08;
    itmask      = ((uint16_t)0x01 << (uint16_t)bitpos);
    USARTx->STS = (uint16_t)~itmask;
}
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
