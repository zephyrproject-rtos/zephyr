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

#include "flexcan.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATAB_RTR_SHIFT (31U)            /*! format A&B RTR mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATAB_IDE_SHIFT (30U)            /*! format A&B IDE mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_RTR_SHIFT  (15U)            /*! format B RTR-2 mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_IDE_SHIFT  (14U)            /*! format B IDE-2 mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_EXT_MASK   (0x3FFFFFFFU)    /*! format A extended mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_EXT_SHIFT  (1U)             /*! format A extended shift.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_STD_MASK   (0x3FF80000U)    /*! format A standard mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_STD_SHIFT  (19U)            /*! format A standard shift.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_EXT_MASK   (0x3FFFU)        /*! format B extended mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_EXT_SHIFT1 (16U)            /*! format B extended mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_EXT_SHIFT2 (0U)             /*! format B extended mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_STD_MASK   (0x7FFU)         /*! format B standard mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_STD_SHIFT1 (19U)            /*! format B standard shift1.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_STD_SHIFT2 (3U)             /*! format B standard shift2.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_MASK       (0xFFU)          /*! format C mask.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT1     (24U)            /*! format C shift1.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT2     (16U)            /*! format C shift2.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT3     (8U)             /*! format C shift3.*/
#define FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT4     (0U)             /*! format C shift4.*/
#define FLEXCAN_BYTE_DATA_FIELD_MASK                 (0xFFU)          /*! masks for byte data field.*/
#define RxFifoFilterElementNum(x)                    ((x + 1) * 8)

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * FLEXCAN Freeze control function
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_EnterFreezeMode
 * Description   : Set FlexCAN module enter freeze mode.
 *
 *END**************************************************************************/
static void FLEXCAN_EnterFreezeMode(CAN_Type* base)
{
    /* Set Freeze, Halt */
    CAN_MCR_REG(base) |= CAN_MCR_FRZ_MASK;
    CAN_MCR_REG(base) |= CAN_MCR_HALT_MASK;
    /* Wait for entering the freeze mode */
    while (!(CAN_MCR_REG(base) & CAN_MCR_FRZ_ACK_MASK));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_ExitFreezeMode
 * Description   : Set FlexCAN module exit freeze mode.
 *
 *END**************************************************************************/
static void FLEXCAN_ExitFreezeMode(CAN_Type* base)
{
    /* De-assert Freeze Mode */
    CAN_MCR_REG(base) &= ~CAN_MCR_HALT_MASK;
    CAN_MCR_REG(base) &= ~CAN_MCR_FRZ_MASK;
    /* Wait for exit the freeze mode */
    while (CAN_MCR_REG(base) & CAN_MCR_FRZ_ACK_MASK);
}

/*******************************************************************************
 * FlexCAN Initialization and Configuration functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_Init
 * Description   : Initialize Flexcan module with given initialize structure.
 *
 *END**************************************************************************/
void FLEXCAN_Init(CAN_Type* base, const flexcan_init_config_t* initConfig)
{
    assert(initConfig);

    /* Enable Flexcan module */
    FLEXCAN_Enable(base);

    /* Reset Flexcan module register content to default value */
    FLEXCAN_Deinit(base);

    /* Set maximum MessageBox numbers and
     * Initialize all message buffers as inactive
     */
    FLEXCAN_SetMaxMsgBufNum(base, initConfig->maxMsgBufNum);

    /* Initialize Flexcan module timing character */
    FLEXCAN_SetTiming(base, &initConfig->timing);

    /* Set desired operating mode */
    FLEXCAN_SetOperatingMode(base, initConfig->operatingMode);

    /* Disable Flexcan module */
    FLEXCAN_Disable(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_Deinit
 * Description   : This function reset Flexcan module register content to its
 *                 default value.
 *
 *END**************************************************************************/
void FLEXCAN_Deinit(CAN_Type* base)
{
    uint32_t i;

    /* Reset the FLEXCAN module */
    CAN_MCR_REG(base) |= CAN_MCR_SOFT_RST_MASK;
    /* Wait for reset cycle to complete */
    while (CAN_MCR_REG(base) & CAN_MCR_SOFT_RST_MASK);

    /* Assert Flexcan module Freeze */
    FLEXCAN_EnterFreezeMode(base);

    /* Reset CTRL1 Register */
    CAN_CTRL1_REG(base) = 0x0;

    /* Reset CTRL2 Register */
    CAN_CTRL2_REG(base) = 0x0;

    /* Reset All Message Buffer Content */
    for (i = 0; i < CAN_CS_COUNT; i++)
    {
        base->MB[i].CS = 0x0;
        base->MB[i].ID = 0x0;
        base->MB[i].WORD0 = 0x0;
        base->MB[i].WORD1 = 0x0;
    }

    /* Reset Rx Individual Mask */
    for (i = 0; i < CAN_RXIMR_COUNT; i++)
        CAN_RXIMR_REG(base, i) = 0x0;

    /* Reset Rx Mailboxes Global Mask */
    CAN_RXMGMASK_REG(base) = 0xFFFFFFFF;

    /* Reset Rx Buffer 14 Mask */
    CAN_RX14MASK_REG(base) = 0xFFFFFFFF;

    /* Reset Rx Buffer 15 Mask */
    CAN_RX15MASK_REG(base) = 0xFFFFFFFF;

    /* Rx FIFO Global Mask */
    CAN_RXFGMASK_REG(base) = 0xFFFFFFFF;

    /* Disable all MB interrupts */
    CAN_IMASK1_REG(base) = 0x0;
    CAN_IMASK2_REG(base) = 0x0;

    // Clear all MB interrupt flags
    CAN_IFLAG1_REG(base) = 0xFFFFFFFF;
    CAN_IFLAG2_REG(base) = 0xFFFFFFFF;

    // Clear all Error interrupt flags
    CAN_ESR1_REG(base) = 0xFFFFFFFF;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_Enable
 * Description   : This function is used to Enable the Flexcan Module.
 *
 *END**************************************************************************/
void FLEXCAN_Enable(CAN_Type* base)
{
    /* Enable clock */
    CAN_MCR_REG(base) &= ~CAN_MCR_MDIS_MASK;
    /* Wait until enabled */
    while (CAN_MCR_REG(base) & CAN_MCR_LPM_ACK_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_Disable
 * Description   : This function is used to Disable the CAN Module.
 *
 *END**************************************************************************/
void FLEXCAN_Disable(CAN_Type* base)
{
    /* Disable clock*/
    CAN_MCR_REG(base) |= CAN_MCR_MDIS_MASK;
    /* Wait until disabled */
    while (!(CAN_MCR_REG(base) & CAN_MCR_LPM_ACK_MASK));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetTiming
 * Description   : Sets the FlexCAN time segments for setting up bit rate.
 *
 *END**************************************************************************/
void FLEXCAN_SetTiming(CAN_Type* base, const flexcan_timing_t* timing)
{
    assert(timing);

    /* Assert Flexcan module Freeze */
    FLEXCAN_EnterFreezeMode(base);

    /* Set Flexcan module Timing Character */
    CAN_CTRL1_REG(base) &= ~(CAN_CTRL1_PRESDIV_MASK | \
                             CAN_CTRL1_RJW_MASK     | \
                             CAN_CTRL1_PSEG1_MASK   | \
                             CAN_CTRL1_PSEG2_MASK   | \
                             CAN_CTRL1_PROP_SEG_MASK);
    CAN_CTRL1_REG(base) |= (CAN_CTRL1_PRESDIV(timing->preDiv)  | \
                            CAN_CTRL1_RJW(timing->rJumpwidth)  | \
                            CAN_CTRL1_PSEG1(timing->phaseSeg1) | \
                            CAN_CTRL1_PSEG2(timing->phaseSeg2) | \
                            CAN_CTRL1_PROP_SEG(timing->propSeg));

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetOperatingMode
 * Description   : Set operation mode.
 *
 *END**************************************************************************/
void FLEXCAN_SetOperatingMode(CAN_Type* base, uint8_t mode)
{
    assert((mode & flexcanNormalMode) ||
           (mode & flexcanListenOnlyMode) ||
           (mode & flexcanLoopBackMode));

    /* Assert Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    if (mode & flexcanNormalMode)
        CAN_MCR_REG(base) &= ~CAN_MCR_SUPV_MASK;
    else
        CAN_MCR_REG(base) |= CAN_MCR_SUPV_MASK;

    if (mode & flexcanListenOnlyMode)
        CAN_CTRL1_REG(base) |= CAN_CTRL1_LOM_MASK;
    else
        CAN_CTRL1_REG(base) &= ~CAN_CTRL1_LOM_MASK;

    if (mode & flexcanLoopBackMode)
        CAN_CTRL1_REG(base) |= CAN_CTRL1_LPB_MASK;
    else
        CAN_CTRL1_REG(base) &= ~CAN_CTRL1_LPB_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetMaxMsgBufNum
 * Description   : Set the maximum number of Message Buffers.
 *
 *END**************************************************************************/
void FLEXCAN_SetMaxMsgBufNum(CAN_Type* base, uint32_t bufNum)
{
    assert((bufNum <= CAN_CS_COUNT) && (bufNum > 0));

    /* Assert Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    /* Set the maximum number of MBs*/
    CAN_MCR_REG(base) = (CAN_MCR_REG(base) & (~CAN_MCR_MAXMB_MASK)) | CAN_MCR_MAXMB(bufNum-1);

    /* De-assert Freeze Mode*/
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetAbortCmd
 * Description   : Set the Transmit abort feature enablement.
 *
 *END**************************************************************************/
void FLEXCAN_SetAbortCmd(CAN_Type* base, bool enable)
{
    /* Assert Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
        CAN_MCR_REG(base) |= CAN_MCR_AEN_MASK;
    else
        CAN_MCR_REG(base) &= ~CAN_MCR_AEN_MASK;

    /* De-assert Freeze Mode*/
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetLocalPrioCmd
 * Description   : Set the local transmit priority enablement.
 *
 *END**************************************************************************/
void FLEXCAN_SetLocalPrioCmd(CAN_Type* base, bool enable)
{
    /* Assert Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
    {
        CAN_MCR_REG(base) |= CAN_MCR_LPRIO_EN_MASK;
        CAN_CTRL1_REG(base) &= ~CAN_CTRL1_LBUF_MASK;
    }
    else
    {
        CAN_CTRL1_REG(base) |= CAN_CTRL1_LBUF_MASK;
        CAN_MCR_REG(base) &= ~CAN_MCR_LPRIO_EN_MASK;
    }

    /* De-assert Freeze Mode*/
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetMatchPrioCmd
 * Description   : Set the Rx matching process priority.
 *
 *END**************************************************************************/
void FLEXCAN_SetMatchPrioCmd(CAN_Type* base, bool priority)
{
    /* Assert Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    if (priority)
        CAN_CTRL2_REG(base) |= CAN_CTRL2_MRP_MASK;
    else
        CAN_CTRL2_REG(base) &= ~CAN_CTRL2_MRP_MASK;

    /* De-assert Freeze Mode*/
    FLEXCAN_ExitFreezeMode(base);
}

/*******************************************************************************
 * FlexCAN Message buffer control functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_GetMsgBufPtr
 * Description   : Get message buffer pointer for transition.
 *
 *END**************************************************************************/
flexcan_msgbuf_t* FLEXCAN_GetMsgBufPtr(CAN_Type* base, uint8_t msgBufIdx)
{
    assert(msgBufIdx < CAN_CS_COUNT);

    return (flexcan_msgbuf_t*) &base->MB[msgBufIdx];
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_LockRxMsgBuf
 * Description   : Locks the FlexCAN Rx message buffer.
 *
 *END**************************************************************************/
bool FLEXCAN_LockRxMsgBuf(CAN_Type* base, uint8_t msgBufIdx)
{
    volatile uint32_t temp;

    /* Check if the MB to be Locked is enabled */
    if (msgBufIdx > (CAN_MCR_REG(base) & CAN_MCR_MAXMB_MASK))
        return false;

    /* ARM Core read MB's CS to lock MB */
    temp = base->MB[msgBufIdx].CS;

    /* Read temp itself to avoid ARMGCC warning */
    temp++;

    return true;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_UnlockAllRxMsgBuf
 * Description   : Unlocks the FlexCAN Rx message buffer.
 *
 *END**************************************************************************/
uint16_t FLEXCAN_UnlockAllRxMsgBuf(CAN_Type* base)
{
    /* Read Free Running Timer to unlock all MessageBox */
    return CAN_TIMER_REG(base);
}

/*******************************************************************************
 * FlexCAN Interrupts and flags management functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetMsgBufIntCmd
 * Description   : Enables/Disables the FlexCAN Message Buffer interrupt.
 *
 *END**************************************************************************/
void FLEXCAN_SetMsgBufIntCmd(CAN_Type* base, uint8_t msgBufIdx, bool enable)
{
    volatile uint32_t* interruptMaskPtr;
    uint8_t index;

    assert(msgBufIdx < CAN_CS_COUNT);

    if (msgBufIdx > 0x31)
    {
        index = msgBufIdx - 32;
        interruptMaskPtr = &base->IMASK2;
    }
    else
    {
        index = msgBufIdx;
        interruptMaskPtr = &base->IMASK1;
    }

    if (enable)
        *interruptMaskPtr |= 0x1 << index;
    else
        *interruptMaskPtr &= ~(0x1 << index);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_GetMsgBufStatusFlag
 * Description   : Gets the individual FlexCAN MB interrupt flag.
 *
 *END**************************************************************************/
bool FLEXCAN_GetMsgBufStatusFlag(CAN_Type* base, uint8_t msgBufIdx)
{
    volatile uint32_t* interruptFlagPtr;
    volatile uint8_t index;

    assert(msgBufIdx < CAN_CS_COUNT);

    if (msgBufIdx > 0x31)
    {
        index = msgBufIdx - 32;
        interruptFlagPtr = &base->IFLAG2;
    }
    else
    {
        index = msgBufIdx;
        interruptFlagPtr = &base->IFLAG1;
    }

    return (bool)((*interruptFlagPtr >> index) & 0x1);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_ClearMsgBufStatusFlag
 * Description   : Clears the interrupt flag of the message buffers.
 *
 *END**************************************************************************/
void FLEXCAN_ClearMsgBufStatusFlag(CAN_Type* base, uint32_t msgBufIdx)
{
    volatile uint8_t index;

    assert(msgBufIdx < CAN_CS_COUNT);

    if (msgBufIdx > 0x31)
    {
        index = msgBufIdx - 32;
        /* write 1 to clear. */
        base->IFLAG2 = 0x1 << index;
    }
    else
    {
        index = msgBufIdx;
        /* write 1 to clear. */
        base->IFLAG1 = 0x1 << index;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetErrIntCmd
 * Description   : Enables error interrupt of the FlexCAN module.
 *
 *END**************************************************************************/
void FLEXCAN_SetErrIntCmd(CAN_Type* base, uint32_t errorType, bool enable)
{
    assert((errorType & flexcanIntRxWarning) ||
           (errorType & flexcanIntTxWarning) ||
           (errorType & flexcanIntWakeUp) ||
           (errorType & flexcanIntBusOff) ||
           (errorType & flexcanIntError));

    if (enable)
    {
        if (errorType & flexcanIntRxWarning)
        {
            CAN_MCR_REG(base) |= CAN_MCR_WRN_EN_MASK;
            CAN_CTRL1_REG(base) |= CAN_CTRL1_RWRN_MSK_MASK;
        }

        if (errorType & flexcanIntTxWarning)
        {
            CAN_MCR_REG(base) |= CAN_MCR_WRN_EN_MASK;
            CAN_CTRL1_REG(base) |= CAN_CTRL1_TWRN_MSK_MASK;
        }

        if (errorType & flexcanIntWakeUp)
            CAN_MCR_REG(base) |= CAN_MCR_WAK_MSK_MASK;

        if (errorType & flexcanIntBusOff)
            CAN_CTRL1_REG(base) |= CAN_CTRL1_BOFF_MSK_MASK;

        if (errorType & flexcanIntError)
            CAN_CTRL1_REG(base) |= CAN_CTRL1_ERR_MSK_MASK;
    }
    else
    {
        if (errorType & flexcanIntRxWarning)
            CAN_CTRL1_REG(base) &= ~CAN_CTRL1_RWRN_MSK_MASK;

        if (errorType & flexcanIntTxWarning)
            CAN_CTRL1_REG(base) &= ~CAN_CTRL1_TWRN_MSK_MASK;

        if (errorType & flexcanIntWakeUp)
            CAN_MCR_REG(base) &= ~CAN_MCR_WAK_MSK_MASK;

        if (errorType & flexcanIntBusOff)
            CAN_CTRL1_REG(base) &= ~CAN_CTRL1_BOFF_MSK_MASK;

        if (errorType & flexcanIntError)
            CAN_CTRL1_REG(base) &= ~CAN_CTRL1_ERR_MSK_MASK;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_GetErrStatusFlag
 * Description   : Gets the FlexCAN module interrupt flag.
 *
 *END**************************************************************************/
uint32_t FLEXCAN_GetErrStatusFlag(CAN_Type* base, uint32_t errFlags)
{
    return CAN_ESR1_REG(base) & errFlags;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_ClearErrStatusFlag
 * Description   : Clears the interrupt flag of the FlexCAN module.
 *
 *END**************************************************************************/
void FLEXCAN_ClearErrStatusFlag(CAN_Type* base, uint32_t errorType)
{
    /* The Interrupt flag must be cleared by writing it to '1'.
     * Writing '0' has no effect.
     */
    CAN_ESR1_REG(base) = errorType;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_GetErrCounter
 * Description   : Get the error counter of FlexCAN module.
 *
 *END**************************************************************************/
void FLEXCAN_GetErrCounter(CAN_Type* base, uint8_t* txError, uint8_t* rxError)
{
    *txError = CAN_ECR_REG(base) & CAN_ECR_Tx_Err_Counter_MASK;
    *rxError = (CAN_ECR_REG(base) & CAN_ECR_Rx_Err_Counter_MASK) >> \
               CAN_ECR_Rx_Err_Counter_SHIFT;
}

/*******************************************************************************
 * Rx FIFO management functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_EnableRxFifo
 * Description   : Enables the Rx FIFO.
 *
 *END**************************************************************************/
void FLEXCAN_EnableRxFifo(CAN_Type* base, uint8_t numOfFilters)
{
    uint8_t maxNumMb;

    assert(numOfFilters <= 0xF);

    /* Set Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    /* Set the number of the RX FIFO filters needed*/
    CAN_CTRL2_REG(base) = (CAN_CTRL2_REG(base) & ~CAN_CTRL2_RFFN_MASK) | CAN_CTRL2_RFFN(numOfFilters);

    /* Enable RX FIFO*/
    CAN_MCR_REG(base) |= CAN_MCR_RFEN_MASK;

    /* RX FIFO global mask*/
    CAN_RXFGMASK_REG(base) = CAN_RXFGMASK_FGM31_FGM0_MASK;

    maxNumMb = (CAN_MCR_REG(base) & CAN_MCR_MAXMB_MASK) + 1;

    for (uint8_t i = 0; i < maxNumMb; i++)
    {
        /* RX individual mask*/
        CAN_RXIMR_REG(base,i) = CAN_RXIMR0_RXIMR63_MI31_MI0_MASK;
    }

    /* De-assert Freeze Mode*/
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_DisableRxFifo
 * Description   : Disables the Rx FIFO.
 *
 *END**************************************************************************/
void FLEXCAN_DisableRxFifo(CAN_Type* base)
{
    /* Set Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    /* Disable RX FIFO*/
    CAN_MCR_REG(base) &= ~CAN_MCR_RFEN_MASK;

    /* De-assert Freeze Mode*/
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxFifoFilterNum
 * Description   : Set the number of the Rx FIFO filters.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxFifoFilterNum(CAN_Type* base, uint32_t numOfFilters)
{
    assert(numOfFilters <= 0xF);

    /* Set Freeze mode*/
    FLEXCAN_EnterFreezeMode(base);

    /* Set the number of RX FIFO ID filters*/
    CAN_CTRL2_REG(base) = (CAN_CTRL2_REG(base) & ~CAN_CTRL2_RFFN_MASK) | CAN_CTRL2_RFFN(numOfFilters);

    /* De-assert Freeze Mode*/
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxFifoFilter
 * Description   : Set the FlexCAN Rx FIFO fields.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxFifoFilter(CAN_Type* base, uint32_t idFormat, flexcan_id_table_t *idFilterTable)
{
    /* Set RX FIFO ID filter table elements*/
    uint32_t i, j, numOfFilters;
    uint32_t val1 = 0, val2 = 0, val = 0;
    volatile uint32_t *filterTable;

    numOfFilters = (CAN_CTRL2_REG(base) & CAN_CTRL2_RFFN_MASK) >> CAN_CTRL2_RFFN_SHIFT;
    /* Rx FIFO Ocuppied First Message Box is MB6 */
    filterTable = (volatile uint32_t *)&(base->MB[6]);

    CAN_MCR_REG(base) |= CAN_MCR_IDAM(idFormat);

    switch (idFormat)
    {
        case flexcanRxFifoIdElementFormatA:
            /* One full ID (standard and extended) per ID Filter Table element.*/
            if (idFilterTable->isRemoteFrame)
            {
                val = (uint32_t)0x1 << FLEXCAN_RX_FIFO_ID_FILTER_FORMATAB_RTR_SHIFT;
            }
            if (idFilterTable->isExtendedFrame)
            {
                val |= 0x1 << FLEXCAN_RX_FIFO_ID_FILTER_FORMATAB_IDE_SHIFT;
            }
            for (i = 0; i < RxFifoFilterElementNum(numOfFilters); i++)
            {
                if(idFilterTable->isExtendedFrame)
                {
                    filterTable[i] = val + ((*(idFilterTable->idFilter + i)) <<
                                             FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_EXT_SHIFT &
                                             FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_EXT_MASK);
                }else
                {
                    filterTable[i] = val + ((*(idFilterTable->idFilter + i)) <<
                                             FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_STD_SHIFT &
                                             FLEXCAN_RX_FIFO_ID_FILTER_FORMATA_STD_MASK);
                }
            }
            break;
        case flexcanRxFifoIdElementFormatB:
            /* Two full standard IDs or two partial 14-bit (standard and extended) IDs*/
            /* per ID Filter Table element.*/
            if (idFilterTable->isRemoteFrame)
            {
                val1 = (uint32_t)0x1 << FLEXCAN_RX_FIFO_ID_FILTER_FORMATAB_RTR_SHIFT;
                val2 = 0x1 << FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_RTR_SHIFT;
            }
            if (idFilterTable->isExtendedFrame)
            {
                val1 |= 0x1 << FLEXCAN_RX_FIFO_ID_FILTER_FORMATAB_IDE_SHIFT;
                val2 |= 0x1 << FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_IDE_SHIFT;
            }
            j = 0;
            for (i = 0; i < RxFifoFilterElementNum(numOfFilters); i++)
            {
                if (idFilterTable->isExtendedFrame)
                {
                    filterTable[i] = val1 + (((*(idFilterTable->idFilter + j)) &
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_EXT_MASK) <<
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_EXT_SHIFT1);
                    filterTable[i] |= val2 + (((*(idFilterTable->idFilter + j + 1)) &
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_EXT_MASK) <<
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_EXT_SHIFT2);
                }else
                {
                    filterTable[i] = val1 + (((*(idFilterTable->idFilter + j)) &
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_STD_MASK) <<
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_STD_SHIFT1);
                    filterTable[i] |= val2 + (((*(idFilterTable->idFilter + j + 1)) &
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_STD_MASK) <<
                                              FLEXCAN_RX_FIFO_ID_FILTER_FORMATB_STD_SHIFT2);
                }
                j = j + 2;
            }
            break;
        case flexcanRxFifoIdElementFormatC:
            /* Four partial 8-bit Standard IDs per ID Filter Table element.*/
            j = 0;
            for (i = 0; i < RxFifoFilterElementNum(numOfFilters); i++)
            {
                filterTable[i] = (((*(idFilterTable->idFilter + j)) &
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_MASK) <<
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT1);
                filterTable[i] = (((*(idFilterTable->idFilter + j + 1)) &
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_MASK) <<
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT2);
                filterTable[i] = (((*(idFilterTable->idFilter + j + 2)) &
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_MASK) <<
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT3);
                filterTable[i] = (((*(idFilterTable->idFilter + j + 3)) &
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_MASK) <<
                                  FLEXCAN_RX_FIFO_ID_FILTER_FORMATC_SHIFT4);
                j = j + 4;
            }
            break;
        case flexcanRxFifoIdElementFormatD:
            /* All frames rejected.*/
            break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_GetRxFifoPtr
 * Description   : Gets the FlexCAN Rx FIFO data pointer.
 *
 *END**************************************************************************/
flexcan_msgbuf_t* FLEXCAN_GetRxFifoPtr(CAN_Type* base)
{
    /* Rx-Fifo occupy MB0 ~ MB5 */
    return (flexcan_msgbuf_t*)&base->MB;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_GetRxFifoInfo
 * Description   : Set the FlexCAN RX Fifo global mask.
 *
 *END**************************************************************************/
uint16_t FLEXCAN_GetRxFifoInfo(CAN_Type* base)
{
    return CAN_RXFIR_REG(base) & CAN_RXFIR_IDHIT_MASK;
}

/*******************************************************************************
 * Rx Mask Setting functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxMaskMode
 * Description   : Set the Rx masking mode.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxMaskMode(CAN_Type* base, uint32_t mode)
{
    assert((mode == flexcanRxMaskGlobal) ||
           (mode == flexcanRxMaskIndividual));

    /* Assert Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    if (mode == flexcanRxMaskIndividual)
        CAN_MCR_REG(base) |= CAN_MCR_IRMQ_MASK;
    else
        CAN_MCR_REG(base) &= ~CAN_MCR_IRMQ_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxMaskRtrCmd
 * Description   : Set the remote trasmit request mask enablement.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxMaskRtrCmd(CAN_Type* base, bool enable)
{
    /* Assert Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
        CAN_CTRL2_REG(base) |= CAN_CTRL2_EACEN_MASK;
    else
        CAN_CTRL2_REG(base) &= ~CAN_CTRL2_EACEN_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxGlobalMask
 * Description   : Set the FlexCAN RX global mask.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxGlobalMask(CAN_Type* base, uint32_t mask)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    /* load mask */
    CAN_RXMGMASK_REG(base) = mask;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxIndividualMask
 * Description   : Set the FlexCAN Rx individual mask for ID filtering in
 *                 the Rx MBs and the Rx FIFO.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxIndividualMask(CAN_Type* base, uint32_t msgBufIdx, uint32_t mask)
{
    assert(msgBufIdx < CAN_RXIMR_COUNT);

    /* Assert Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    CAN_RXIMR_REG(base,msgBufIdx) = mask;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxMsgBuff14Mask
 * Description   : Set the FlexCAN RX Message Buffer BUF14 mask.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxMsgBuff14Mask(CAN_Type* base, uint32_t mask)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    /* load mask */
    CAN_RX14MASK_REG(base) = mask;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxMsgBuff15Mask
 * Description   : Set the FlexCAN RX Message Buffer BUF15 mask.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxMsgBuff15Mask(CAN_Type* base, uint32_t mask)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    /* load mask */
    CAN_RX15MASK_REG(base) = mask;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxFifoGlobalMask
 * Description   : Set the FlexCAN RX Fifo global mask.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxFifoGlobalMask(CAN_Type* base, uint32_t mask)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    /* load mask */
    CAN_RXFGMASK_REG(base) = mask;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*******************************************************************************
 * Misc. Functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetSelfWakeUpCmd
 * Description   : Enable/disable the FlexCAN self wakeup feature.
 *
 *END**************************************************************************/
void FLEXCAN_SetSelfWakeUpCmd(CAN_Type* base, bool lpfEnable, bool enable)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    if (lpfEnable)
        CAN_MCR_REG(base) |= CAN_MCR_WAK_SRC_MASK;
    else
        CAN_MCR_REG(base) &= ~CAN_MCR_WAK_SRC_MASK;

    if (enable)
        CAN_MCR_REG(base) |= CAN_MCR_SLF_WAK_MASK;
    else
        CAN_MCR_REG(base) &= ~CAN_MCR_SLF_WAK_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetSelfReceptionCmd
 * Description   : Enable/disable the FlexCAN self reception feature.
 *
 *END**************************************************************************/
void FLEXCAN_SetSelfReceptionCmd(CAN_Type* base, bool enable)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
        CAN_MCR_REG(base) &= ~CAN_MCR_SRX_DIS_MASK;
    else
        CAN_MCR_REG(base) |= CAN_MCR_SRX_DIS_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetRxVoteCmd
 * Description   : Enable/disable the enhance FlexCAN Rx vote.
 *
 *END**************************************************************************/
void FLEXCAN_SetRxVoteCmd(CAN_Type* base, bool enable)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
        CAN_CTRL1_REG(base) |= CAN_CTRL1_SMP_MASK;
    else
        CAN_CTRL1_REG(base) &= ~CAN_CTRL1_SMP_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetAutoBusOffRecoverCmd
 * Description   : Enable/disable the Auto Busoff recover feature.
 *
 *END**************************************************************************/
void FLEXCAN_SetAutoBusOffRecoverCmd(CAN_Type* base, bool enable)
{
    if (enable)
        CAN_CTRL1_REG(base) &= ~CAN_CTRL1_BOFF_MSK_MASK;
    else
        CAN_CTRL1_REG(base) |= CAN_CTRL1_BOFF_MSK_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetTimeSyncCmd
 * Description   : Enable/disable the Time Sync feature.
 *
 *END**************************************************************************/
void FLEXCAN_SetTimeSyncCmd(CAN_Type* base, bool enable)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
        CAN_CTRL1_REG(base) |= CAN_CTRL1_TSYN_MASK;
    else
        CAN_CTRL1_REG(base) &= ~CAN_CTRL1_TSYN_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : FLEXCAN_SetAutoRemoteResponseCmd
 * Description   : Enable/disable the Auto Remote Response feature.
 *
 *END**************************************************************************/
void FLEXCAN_SetAutoRemoteResponseCmd(CAN_Type* base, bool enable)
{
    /* Set Freeze mode */
    FLEXCAN_EnterFreezeMode(base);

    if (enable)
        CAN_CTRL2_REG(base) &= ~CAN_CTRL2_RRS_MASK;
    else
        CAN_CTRL2_REG(base) |= CAN_CTRL2_RRS_MASK;

    /* De-assert Freeze Mode */
    FLEXCAN_ExitFreezeMode(base);
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
