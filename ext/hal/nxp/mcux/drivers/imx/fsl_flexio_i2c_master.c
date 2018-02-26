/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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

#include "fsl_flexio_i2c_master.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief  FLEXIO I2C transfer state */
enum _flexio_i2c_master_transfer_states
{
    kFLEXIO_I2C_Idle = 0x0U,             /*!< I2C bus idle */
    kFLEXIO_I2C_CheckAddress = 0x1U,     /*!< 7-bit address check state */
    kFLEXIO_I2C_SendCommand = 0x2U,      /*!< Send command byte phase */
    kFLEXIO_I2C_SendData = 0x3U,         /*!< Send data transfer phase*/
    kFLEXIO_I2C_ReceiveDataBegin = 0x4U, /*!< Receive data begin transfer phase*/
    kFLEXIO_I2C_ReceiveData = 0x5U,      /*!< Receive data transfer phase*/
};

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
extern const clock_ip_name_t s_flexioClocks[];
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

extern FLEXIO_Type *const s_flexioBases[];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

extern uint32_t FLEXIO_GetInstance(FLEXIO_Type *base);

/*!
 * @brief Set up master transfer, send slave address and decide the initial
 * transfer state.
 *
 * @param base pointer to FLEXIO_I2C_Type structure
 * @param handle pointer to flexio_i2c_master_handle_t structure which stores the transfer state
 * @param transfer pointer to flexio_i2c_master_transfer_t structure
 */
static status_t FLEXIO_I2C_MasterTransferInitStateMachine(FLEXIO_I2C_Type *base,
                                                          flexio_i2c_master_handle_t *handle,
                                                          flexio_i2c_master_transfer_t *xfer);

/*!
 * @brief Master run transfer state machine to perform a byte of transfer.
 *
 * @param base pointer to FLEXIO_I2C_Type structure
 * @param handle pointer to flexio_i2c_master_handle_t structure which stores the transfer state
 * @param statusFlags flexio i2c hardware status
 * @retval kStatus_Success Successfully run state machine
 * @retval kStatus_FLEXIO_I2C_Nak Receive Nak during transfer
 */
static status_t FLEXIO_I2C_MasterTransferRunStateMachine(FLEXIO_I2C_Type *base,
                                                         flexio_i2c_master_handle_t *handle,
                                                         uint32_t statusFlags);

/*!
 * @brief Complete transfer, disable interrupt and call callback.
 *
 * @param base pointer to FLEXIO_I2C_Type structure
 * @param handle pointer to flexio_i2c_master_handle_t structure which stores the transfer state
 * @param status flexio transfer status
 */
static void FLEXIO_I2C_MasterTransferComplete(FLEXIO_I2C_Type *base,
                                              flexio_i2c_master_handle_t *handle,
                                              status_t status);

/*******************************************************************************
 * Codes
 ******************************************************************************/

uint32_t FLEXIO_I2C_GetInstance(FLEXIO_I2C_Type *base)
{
    return FLEXIO_GetInstance(base->flexioBase);
}

static status_t FLEXIO_I2C_MasterTransferInitStateMachine(FLEXIO_I2C_Type *base,
                                                          flexio_i2c_master_handle_t *handle,
                                                          flexio_i2c_master_transfer_t *xfer)
{
    bool needRestart;
    uint32_t byteCount;

    /* Init the handle member. */
    handle->transfer.slaveAddress = xfer->slaveAddress;
    handle->transfer.direction = xfer->direction;
    handle->transfer.subaddress = xfer->subaddress;
    handle->transfer.subaddressSize = xfer->subaddressSize;
    handle->transfer.data = xfer->data;
    handle->transfer.dataSize = xfer->dataSize;
    handle->transfer.flags = xfer->flags;
    handle->transferSize = xfer->dataSize;

    /* Initial state, i2c check address state. */
    handle->state = kFLEXIO_I2C_CheckAddress;

    /* Clear all status before transfer. */
    FLEXIO_I2C_MasterClearStatusFlags(base, kFLEXIO_I2C_ReceiveNakFlag);

    /* Calculate whether need to send re-start. */
    needRestart = (handle->transfer.subaddressSize != 0) && (handle->transfer.direction == kFLEXIO_I2C_Read);

    /* Calculate total byte count in a frame. */
    byteCount = 1;

    if (!needRestart)
    {
        byteCount += handle->transfer.dataSize;
    }

    if (handle->transfer.subaddressSize != 0)
    {
        byteCount += handle->transfer.subaddressSize;
        /* Next state, send command byte. */
        handle->state = kFLEXIO_I2C_SendCommand;
    }

    /* Configure data count. */
    if (FLEXIO_I2C_MasterSetTransferCount(base, byteCount) != kStatus_Success)
    {
        return kStatus_InvalidArgument;
    }

    while (!((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[0]))))
    {
    }

    /* Send address byte first. */
    if (needRestart)
    {
        FLEXIO_I2C_MasterStart(base, handle->transfer.slaveAddress, kFLEXIO_I2C_Write);
    }
    else
    {
        FLEXIO_I2C_MasterStart(base, handle->transfer.slaveAddress, handle->transfer.direction);
    }

    return kStatus_Success;
}

static status_t FLEXIO_I2C_MasterTransferRunStateMachine(FLEXIO_I2C_Type *base,
                                                         flexio_i2c_master_handle_t *handle,
                                                         uint32_t statusFlags)
{
    if (statusFlags & kFLEXIO_I2C_ReceiveNakFlag)
    {
        /* Clear receive nak flag. */
        FLEXIO_ClearShifterErrorFlags(base->flexioBase, 1U << base->shifterIndex[1]);

        if ((!((handle->state == kFLEXIO_I2C_SendData) && (handle->transfer.dataSize == 0U))) &&
            (!(((handle->state == kFLEXIO_I2C_ReceiveData) || (handle->state == kFLEXIO_I2C_ReceiveDataBegin)) &&
               (handle->transfer.dataSize == 1U))))
        {
            FLEXIO_I2C_MasterReadByte(base);

            FLEXIO_I2C_MasterAbortStop(base);

            handle->state = kFLEXIO_I2C_Idle;

            return kStatus_FLEXIO_I2C_Nak;
        }
    }

    if (handle->state == kFLEXIO_I2C_CheckAddress)
    {
        if (handle->transfer.direction == kFLEXIO_I2C_Write)
        {
            /* Next state, send data. */
            handle->state = kFLEXIO_I2C_SendData;
        }
        else
        {
            /* Next state, receive data begin. */
            handle->state = kFLEXIO_I2C_ReceiveDataBegin;
        }
    }

    if ((statusFlags & kFLEXIO_I2C_RxFullFlag) && (handle->state != kFLEXIO_I2C_ReceiveData))
    {
        FLEXIO_I2C_MasterReadByte(base);
    }

    switch (handle->state)
    {
        case kFLEXIO_I2C_SendCommand:
            if (statusFlags & kFLEXIO_I2C_TxEmptyFlag)
            {
                if (handle->transfer.subaddressSize > 0)
                {
                    handle->transfer.subaddressSize--;
                    FLEXIO_I2C_MasterWriteByte(
                        base, ((handle->transfer.subaddress) >> (8 * handle->transfer.subaddressSize)));

                    if (handle->transfer.subaddressSize == 0)
                    {
                        /* Load re-start in advance. */
                        if (handle->transfer.direction == kFLEXIO_I2C_Read)
                        {
                            while (!((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[0]))))
                            {
                            }
                            FLEXIO_I2C_MasterRepeatedStart(base);
                        }
                    }
                }
                else
                {
                    if (handle->transfer.direction == kFLEXIO_I2C_Write)
                    {
                        /* Next state, send data. */
                        handle->state = kFLEXIO_I2C_SendData;

                        /* Send first byte of data. */
                        if (handle->transfer.dataSize > 0)
                        {
                            FLEXIO_I2C_MasterWriteByte(base, *handle->transfer.data);

                            handle->transfer.data++;
                            handle->transfer.dataSize--;
                        }
                    }
                    else
                    {
                        FLEXIO_I2C_MasterSetTransferCount(base, (handle->transfer.dataSize + 1));
                        FLEXIO_I2C_MasterStart(base, handle->transfer.slaveAddress, kFLEXIO_I2C_Read);

                        /* Next state, receive data begin. */
                        handle->state = kFLEXIO_I2C_ReceiveDataBegin;
                    }
                }
            }
            break;

        /* Send command byte. */
        case kFLEXIO_I2C_SendData:
            if (statusFlags & kFLEXIO_I2C_TxEmptyFlag)
            {
                /* Send one byte of data. */
                if (handle->transfer.dataSize > 0)
                {
                    FLEXIO_I2C_MasterWriteByte(base, *handle->transfer.data);

                    handle->transfer.data++;
                    handle->transfer.dataSize--;
                }
                else
                {
                    FLEXIO_I2C_MasterStop(base);

                    while (!(FLEXIO_I2C_MasterGetStatusFlags(base) & kFLEXIO_I2C_RxFullFlag))
                    {
                    }
                    FLEXIO_I2C_MasterReadByte(base);

                    handle->state = kFLEXIO_I2C_Idle;
                }
            }
            break;

        case kFLEXIO_I2C_ReceiveDataBegin:
            if (statusFlags & kFLEXIO_I2C_RxFullFlag)
            {
                handle->state = kFLEXIO_I2C_ReceiveData;
                /* Send nak at the last receive byte. */
                if (handle->transfer.dataSize == 1)
                {
                    FLEXIO_I2C_MasterEnableAck(base, false);
                    while (!((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[0]))))
                    {
                    }
                    FLEXIO_I2C_MasterStop(base);
                }
                else
                {
                    FLEXIO_I2C_MasterEnableAck(base, true);
                }
            }
            else if (statusFlags & kFLEXIO_I2C_TxEmptyFlag)
            {
                /* Read one byte of data. */
                FLEXIO_I2C_MasterWriteByte(base, 0xFFFFFFFFU);
            }
            else
            {
            }
            break;

        case kFLEXIO_I2C_ReceiveData:
            if (statusFlags & kFLEXIO_I2C_RxFullFlag)
            {
                *handle->transfer.data = FLEXIO_I2C_MasterReadByte(base);
                handle->transfer.data++;
                if (handle->transfer.dataSize--)
                {
                    if (handle->transfer.dataSize == 0)
                    {
                        FLEXIO_I2C_MasterDisableInterrupts(base, kFLEXIO_I2C_RxFullInterruptEnable);
                        handle->state = kFLEXIO_I2C_Idle;
                    }

                    /* Send nak at the last receive byte. */
                    if (handle->transfer.dataSize == 1)
                    {
                        FLEXIO_I2C_MasterEnableAck(base, false);
                        while (!((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[0]))))
                        {
                        }
                        FLEXIO_I2C_MasterStop(base);
                    }
                }
            }
            else if (statusFlags & kFLEXIO_I2C_TxEmptyFlag)
            {
                if (handle->transfer.dataSize > 1)
                {
                    FLEXIO_I2C_MasterWriteByte(base, 0xFFFFFFFFU);
                }
            }
            else
            {
            }
            break;

        default:
            break;
    }

    return kStatus_Success;
}

static void FLEXIO_I2C_MasterTransferComplete(FLEXIO_I2C_Type *base,
                                              flexio_i2c_master_handle_t *handle,
                                              status_t status)
{
    FLEXIO_I2C_MasterDisableInterrupts(base, kFLEXIO_I2C_TxEmptyInterruptEnable | kFLEXIO_I2C_RxFullInterruptEnable);

    if (handle->completionCallback)
    {
        handle->completionCallback(base, handle, status, handle->userData);
    }
}

status_t FLEXIO_I2C_MasterInit(FLEXIO_I2C_Type *base, flexio_i2c_master_config_t *masterConfig, uint32_t srcClock_Hz)
{
    assert(base && masterConfig);

    flexio_shifter_config_t shifterConfig;
    flexio_timer_config_t timerConfig;
    uint32_t controlVal = 0;
    uint16_t timerDiv = 0;
    status_t result = kStatus_Success;

    memset(&shifterConfig, 0, sizeof(shifterConfig));
    memset(&timerConfig, 0, sizeof(timerConfig));

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate flexio clock. */
    CLOCK_EnableClock(s_flexioClocks[FLEXIO_I2C_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Do hardware configuration. */
    /* 1. Configure the shifter 0 for tx. */
    shifterConfig.timerSelect = base->timerIndex[1];
    shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive;
    shifterConfig.pinConfig = kFLEXIO_PinConfigOpenDrainOrBidirection;
    shifterConfig.pinSelect = base->SDAPinIndex;
    shifterConfig.pinPolarity = kFLEXIO_PinActiveLow;
    shifterConfig.shifterMode = kFLEXIO_ShifterModeTransmit;
    shifterConfig.inputSource = kFLEXIO_ShifterInputFromPin;
    shifterConfig.shifterStop = kFLEXIO_ShifterStopBitHigh;
    shifterConfig.shifterStart = kFLEXIO_ShifterStartBitLow;

    FLEXIO_SetShifterConfig(base->flexioBase, base->shifterIndex[0], &shifterConfig);

    /* 2. Configure the shifter 1 for rx. */
    shifterConfig.timerSelect = base->timerIndex[1];
    shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnNegitive;
    shifterConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
    shifterConfig.pinSelect = base->SDAPinIndex;
    shifterConfig.pinPolarity = kFLEXIO_PinActiveHigh;
    shifterConfig.shifterMode = kFLEXIO_ShifterModeReceive;
    shifterConfig.inputSource = kFLEXIO_ShifterInputFromPin;
    shifterConfig.shifterStop = kFLEXIO_ShifterStopBitLow;
    shifterConfig.shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable;

    FLEXIO_SetShifterConfig(base->flexioBase, base->shifterIndex[1], &shifterConfig);

    /*3. Configure the timer 0 for generating bit clock. */
    timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(base->shifterIndex[0]);
    timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveLow;
    timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
    timerConfig.pinConfig = kFLEXIO_PinConfigOpenDrainOrBidirection;
    timerConfig.pinSelect = base->SCLPinIndex;
    timerConfig.pinPolarity = kFLEXIO_PinActiveHigh;
    timerConfig.timerMode = kFLEXIO_TimerModeDual8BitBaudBit;
    timerConfig.timerOutput = kFLEXIO_TimerOutputZeroNotAffectedByReset;
    timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
    timerConfig.timerReset = kFLEXIO_TimerResetOnTimerPinEqualToTimerOutput;
    timerConfig.timerDisable = kFLEXIO_TimerDisableOnTimerCompare;
    timerConfig.timerEnable = kFLEXIO_TimerEnableOnTriggerHigh;
    timerConfig.timerStop = kFLEXIO_TimerStopBitEnableOnTimerDisable;
    timerConfig.timerStart = kFLEXIO_TimerStartBitEnabled;

    /* Set TIMCMP[7:0] = (baud rate divider / 2) - 1. */
    timerDiv = (srcClock_Hz / masterConfig->baudRate_Bps) / 2 - 1;

    if (timerDiv > 0xFFU)
    {
        result = kStatus_InvalidArgument;
        return result;
    }

    timerConfig.timerCompare = timerDiv;

    FLEXIO_SetTimerConfig(base->flexioBase, base->timerIndex[0], &timerConfig);

    /* 4. Configure the timer 1 for controlling shifters. */
    timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(base->shifterIndex[0]);
    timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveLow;
    timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
    timerConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
    timerConfig.pinSelect = base->SCLPinIndex;
    timerConfig.pinPolarity = kFLEXIO_PinActiveLow;
    timerConfig.timerMode = kFLEXIO_TimerModeSingle16Bit;
    timerConfig.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
    timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnPinInputShiftPinInput;
    timerConfig.timerReset = kFLEXIO_TimerResetNever;
    timerConfig.timerDisable = kFLEXIO_TimerDisableOnPreTimerDisable;
    timerConfig.timerEnable = kFLEXIO_TimerEnableOnPrevTimerEnable;
    timerConfig.timerStop = kFLEXIO_TimerStopBitEnableOnTimerCompare;
    timerConfig.timerStart = kFLEXIO_TimerStartBitEnabled;

    /* Set TIMCMP[15:0] = (number of bits x 2) - 1. */
    timerConfig.timerCompare = 8 * 2 - 1;

    FLEXIO_SetTimerConfig(base->flexioBase, base->timerIndex[1], &timerConfig);

    /* Configure FLEXIO I2C Master. */
    controlVal = base->flexioBase->CTRL;
    controlVal &=
        ~(FLEXIO_CTRL_DOZEN_MASK | FLEXIO_CTRL_DBGE_MASK | FLEXIO_CTRL_FASTACC_MASK | FLEXIO_CTRL_FLEXEN_MASK);
    controlVal |= (FLEXIO_CTRL_DBGE(masterConfig->enableInDebug) | FLEXIO_CTRL_FASTACC(masterConfig->enableFastAccess) |
                   FLEXIO_CTRL_FLEXEN(masterConfig->enableMaster));
    if (!masterConfig->enableInDoze)
    {
        controlVal |= FLEXIO_CTRL_DOZEN_MASK;
    }

    base->flexioBase->CTRL = controlVal;
    return result;
}

void FLEXIO_I2C_MasterDeinit(FLEXIO_I2C_Type *base)
{
    base->flexioBase->SHIFTCFG[base->shifterIndex[0]] = 0;
    base->flexioBase->SHIFTCTL[base->shifterIndex[0]] = 0;
    base->flexioBase->SHIFTCFG[base->shifterIndex[1]] = 0;
    base->flexioBase->SHIFTCTL[base->shifterIndex[1]] = 0;
    base->flexioBase->TIMCFG[base->timerIndex[0]] = 0;
    base->flexioBase->TIMCMP[base->timerIndex[0]] = 0;
    base->flexioBase->TIMCTL[base->timerIndex[0]] = 0;
    base->flexioBase->TIMCFG[base->timerIndex[1]] = 0;
    base->flexioBase->TIMCMP[base->timerIndex[1]] = 0;
    base->flexioBase->TIMCTL[base->timerIndex[1]] = 0;
    /* Clear the shifter flag. */
    base->flexioBase->SHIFTSTAT = (1U << base->shifterIndex[0]);
    base->flexioBase->SHIFTSTAT = (1U << base->shifterIndex[1]);
    /* Clear the timer flag. */
    base->flexioBase->TIMSTAT = (1U << base->timerIndex[0]);
    base->flexioBase->TIMSTAT = (1U << base->timerIndex[1]);
}

void FLEXIO_I2C_MasterGetDefaultConfig(flexio_i2c_master_config_t *masterConfig)
{
    assert(masterConfig);

    masterConfig->enableMaster = true;
    masterConfig->enableInDoze = false;
    masterConfig->enableInDebug = true;
    masterConfig->enableFastAccess = false;

    /* Default baud rate at 100kbps. */
    masterConfig->baudRate_Bps = 100000U;
}

uint32_t FLEXIO_I2C_MasterGetStatusFlags(FLEXIO_I2C_Type *base)
{
    uint32_t status = 0;

    status =
        ((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[0])) >> base->shifterIndex[0]);
    status |=
        (((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[1])) >> (base->shifterIndex[1]))
         << 1U);
    status |=
        (((FLEXIO_GetShifterErrorFlags(base->flexioBase) & (1U << base->shifterIndex[1])) >> (base->shifterIndex[1]))
         << 2U);

    return status;
}

void FLEXIO_I2C_MasterClearStatusFlags(FLEXIO_I2C_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_I2C_TxEmptyFlag)
    {
        FLEXIO_ClearShifterStatusFlags(base->flexioBase, 1U << base->shifterIndex[0]);
    }

    if (mask & kFLEXIO_I2C_RxFullFlag)
    {
        FLEXIO_ClearShifterStatusFlags(base->flexioBase, 1U << base->shifterIndex[1]);
    }

    if (mask & kFLEXIO_I2C_ReceiveNakFlag)
    {
        FLEXIO_ClearShifterErrorFlags(base->flexioBase, 1U << base->shifterIndex[1]);
    }
}

void FLEXIO_I2C_MasterEnableInterrupts(FLEXIO_I2C_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_I2C_TxEmptyInterruptEnable)
    {
        FLEXIO_EnableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[0]);
    }
    if (mask & kFLEXIO_I2C_RxFullInterruptEnable)
    {
        FLEXIO_EnableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[1]);
    }
}

void FLEXIO_I2C_MasterDisableInterrupts(FLEXIO_I2C_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_I2C_TxEmptyInterruptEnable)
    {
        FLEXIO_DisableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[0]);
    }
    if (mask & kFLEXIO_I2C_RxFullInterruptEnable)
    {
        FLEXIO_DisableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[1]);
    }
}

void FLEXIO_I2C_MasterSetBaudRate(FLEXIO_I2C_Type *base, uint32_t baudRate_Bps, uint32_t srcClock_Hz)
{
    uint16_t timerDiv = 0;
    uint16_t timerCmp = 0;
    FLEXIO_Type *flexioBase = base->flexioBase;

    /* Set TIMCMP[7:0] = (baud rate divider / 2) - 1.*/
    timerDiv = srcClock_Hz / baudRate_Bps;
    timerDiv = timerDiv / 2 - 1U;

    timerCmp = flexioBase->TIMCMP[base->timerIndex[0]];
    timerCmp &= 0xFF00;
    timerCmp |= timerDiv;

    flexioBase->TIMCMP[base->timerIndex[0]] = timerCmp;
}

status_t FLEXIO_I2C_MasterSetTransferCount(FLEXIO_I2C_Type *base, uint8_t count)
{
    if (count > 14U)
    {
        return kStatus_InvalidArgument;
    }

    uint16_t timerCmp = 0;
    uint32_t timerConfig = 0;
    FLEXIO_Type *flexioBase = base->flexioBase;

    timerCmp = flexioBase->TIMCMP[base->timerIndex[0]];
    timerCmp &= 0x00FFU;
    timerCmp |= (count * 18 + 1U) << 8U;
    flexioBase->TIMCMP[base->timerIndex[0]] = timerCmp;
    timerConfig = flexioBase->TIMCFG[base->timerIndex[0]];
    timerConfig &= ~FLEXIO_TIMCFG_TIMDIS_MASK;
    timerConfig |= FLEXIO_TIMCFG_TIMDIS(kFLEXIO_TimerDisableOnTimerCompare);
    flexioBase->TIMCFG[base->timerIndex[0]] = timerConfig;

    return kStatus_Success;
}

void FLEXIO_I2C_MasterStart(FLEXIO_I2C_Type *base, uint8_t address, flexio_i2c_direction_t direction)
{
    uint32_t data;

    data = ((uint32_t)address) << 1U | ((direction == kFLEXIO_I2C_Read) ? 1U : 0U);

    FLEXIO_I2C_MasterWriteByte(base, data);
}

void FLEXIO_I2C_MasterRepeatedStart(FLEXIO_I2C_Type *base)
{
    /* Prepare for RESTART condition, no stop.*/
    FLEXIO_I2C_MasterWriteByte(base, 0xFFFFFFFFU);
}

void FLEXIO_I2C_MasterStop(FLEXIO_I2C_Type *base)
{
    /* Prepare normal stop. */
    FLEXIO_I2C_MasterSetTransferCount(base, 0x0U);
    FLEXIO_I2C_MasterWriteByte(base, 0x0U);
}

void FLEXIO_I2C_MasterAbortStop(FLEXIO_I2C_Type *base)
{
    uint32_t tmpConfig;

    /* Prepare abort stop. */
    tmpConfig = base->flexioBase->TIMCFG[base->timerIndex[0]];
    tmpConfig &= ~FLEXIO_TIMCFG_TIMDIS_MASK;
    tmpConfig |= FLEXIO_TIMCFG_TIMDIS(kFLEXIO_TimerDisableOnPinBothEdge);
    base->flexioBase->TIMCFG[base->timerIndex[0]] = tmpConfig;
}

void FLEXIO_I2C_MasterEnableAck(FLEXIO_I2C_Type *base, bool enable)
{
    uint32_t tmpConfig = 0;

    tmpConfig = base->flexioBase->SHIFTCFG[base->shifterIndex[0]];
    tmpConfig &= ~FLEXIO_SHIFTCFG_SSTOP_MASK;
    if (enable)
    {
        tmpConfig |= FLEXIO_SHIFTCFG_SSTOP(kFLEXIO_ShifterStopBitLow);
    }
    else
    {
        tmpConfig |= FLEXIO_SHIFTCFG_SSTOP(kFLEXIO_ShifterStopBitHigh);
    }
    base->flexioBase->SHIFTCFG[base->shifterIndex[0]] = tmpConfig;
}

status_t FLEXIO_I2C_MasterWriteBlocking(FLEXIO_I2C_Type *base, const uint8_t *txBuff, uint8_t txSize)
{
    assert(txBuff);
    assert(txSize);

    uint32_t status;

    while (txSize--)
    {
        FLEXIO_I2C_MasterWriteByte(base, *txBuff++);

        /* Wait until data transfer complete. */
        while (!((status = FLEXIO_I2C_MasterGetStatusFlags(base)) & kFLEXIO_I2C_RxFullFlag))
        {
        }

        if (status & kFLEXIO_I2C_ReceiveNakFlag)
        {
            FLEXIO_ClearShifterErrorFlags(base->flexioBase, 1U << base->shifterIndex[1]);
            return kStatus_FLEXIO_I2C_Nak;
        }
    }
    return kStatus_Success;
}

void FLEXIO_I2C_MasterReadBlocking(FLEXIO_I2C_Type *base, uint8_t *rxBuff, uint8_t rxSize)
{
    assert(rxBuff);
    assert(rxSize);

    while (rxSize--)
    {
        /* Wait until data transfer complete. */
        while (!(FLEXIO_I2C_MasterGetStatusFlags(base) & kFLEXIO_I2C_RxFullFlag))
        {
        }

        *rxBuff++ = FLEXIO_I2C_MasterReadByte(base);
    }
}

status_t FLEXIO_I2C_MasterTransferBlocking(FLEXIO_I2C_Type *base, flexio_i2c_master_transfer_t *xfer)
{
    assert(xfer);

    flexio_i2c_master_handle_t tmpHandle;
    uint32_t statusFlags;
    uint32_t result = kStatus_Success;

    /* Zero the handle. */
    memset(&tmpHandle, 0, sizeof(tmpHandle));

    /* Set up transfer machine. */
    FLEXIO_I2C_MasterTransferInitStateMachine(base, &tmpHandle, xfer);

    do
    {
        /* Wait either tx empty or rx full flag is asserted. */
        while (!((statusFlags = FLEXIO_I2C_MasterGetStatusFlags(base)) &
                 (kFLEXIO_I2C_TxEmptyFlag | kFLEXIO_I2C_RxFullFlag)))
        {
        }

        result = FLEXIO_I2C_MasterTransferRunStateMachine(base, &tmpHandle, statusFlags);

    } while ((tmpHandle.state != kFLEXIO_I2C_Idle) && (result == kStatus_Success));

    return result;
}

status_t FLEXIO_I2C_MasterTransferCreateHandle(FLEXIO_I2C_Type *base,
                                               flexio_i2c_master_handle_t *handle,
                                               flexio_i2c_master_transfer_callback_t callback,
                                               void *userData)
{
    assert(handle);

    IRQn_Type flexio_irqs[] = FLEXIO_IRQS;

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Register callback and userData. */
    handle->completionCallback = callback;
    handle->userData = userData;

    /* Enable interrupt in NVIC. */
    EnableIRQ(flexio_irqs[FLEXIO_I2C_GetInstance(base)]);

    /* Save the context in global variables to support the double weak mechanism. */
    return FLEXIO_RegisterHandleIRQ(base, handle, FLEXIO_I2C_MasterTransferHandleIRQ);
}

status_t FLEXIO_I2C_MasterTransferNonBlocking(FLEXIO_I2C_Type *base,
                                              flexio_i2c_master_handle_t *handle,
                                              flexio_i2c_master_transfer_t *xfer)
{
    assert(handle);
    assert(xfer);

    if (handle->state != kFLEXIO_I2C_Idle)
    {
        return kStatus_FLEXIO_I2C_Busy;
    }
    else
    {
        /* Set up transfer machine. */
        FLEXIO_I2C_MasterTransferInitStateMachine(base, handle, xfer);

        /* Enable both tx empty and rxfull interrupt. */
        FLEXIO_I2C_MasterEnableInterrupts(base, kFLEXIO_I2C_TxEmptyInterruptEnable | kFLEXIO_I2C_RxFullInterruptEnable);

        return kStatus_Success;
    }
}

void FLEXIO_I2C_MasterTransferAbort(FLEXIO_I2C_Type *base, flexio_i2c_master_handle_t *handle)
{
    assert(handle);

    /* Disable interrupts. */
    FLEXIO_I2C_MasterDisableInterrupts(base, kFLEXIO_I2C_TxEmptyInterruptEnable | kFLEXIO_I2C_RxFullInterruptEnable);

    /* Reset to idle state. */
    handle->state = kFLEXIO_I2C_Idle;
}

status_t FLEXIO_I2C_MasterTransferGetCount(FLEXIO_I2C_Type *base, flexio_i2c_master_handle_t *handle, size_t *count)
{
    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    *count = handle->transferSize - handle->transfer.dataSize;

    return kStatus_Success;
}

void FLEXIO_I2C_MasterTransferHandleIRQ(void *i2cType, void *i2cHandle)
{
    FLEXIO_I2C_Type *base = (FLEXIO_I2C_Type *)i2cType;
    flexio_i2c_master_handle_t *handle = (flexio_i2c_master_handle_t *)i2cHandle;
    uint32_t statusFlags;
    status_t result;

    statusFlags = FLEXIO_I2C_MasterGetStatusFlags(base);

    result = FLEXIO_I2C_MasterTransferRunStateMachine(base, handle, statusFlags);

    if (handle->state == kFLEXIO_I2C_Idle)
    {
        FLEXIO_I2C_MasterTransferComplete(base, handle, result);
    }
}
