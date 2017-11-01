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

#include "fsl_flexio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*< @brief user configurable flexio handle count. */
#define FLEXIO_HANDLE_COUNT 2

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*< @brief pointer to array of FLEXIO handle. */
static void *s_flexioHandle[FLEXIO_HANDLE_COUNT];

/*< @brief pointer to array of FLEXIO IP types. */
static void *s_flexioType[FLEXIO_HANDLE_COUNT];

/*< @brief pointer to array of FLEXIO Isr. */
static flexio_isr_t s_flexioIsr[FLEXIO_HANDLE_COUNT];

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to flexio clocks for each instance. */
const clock_ip_name_t s_flexioClocks[] = FLEXIO_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief Pointers to flexio bases for each instance. */
FLEXIO_Type *const s_flexioBases[] = FLEXIO_BASE_PTRS;

/*******************************************************************************
 * Codes
 ******************************************************************************/

uint32_t FLEXIO_GetInstance(FLEXIO_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_flexioBases); instance++)
    {
        if (s_flexioBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_flexioBases));

    return instance;
}

void FLEXIO_Init(FLEXIO_Type *base, const flexio_config_t *userConfig)
{
    uint32_t ctrlReg = 0;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(s_flexioClocks[FLEXIO_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    FLEXIO_Reset(base);

    ctrlReg = base->CTRL;
    ctrlReg &= ~(FLEXIO_CTRL_DOZEN_MASK | FLEXIO_CTRL_DBGE_MASK | FLEXIO_CTRL_FASTACC_MASK | FLEXIO_CTRL_FLEXEN_MASK);
    ctrlReg |= (FLEXIO_CTRL_DBGE(userConfig->enableInDebug) | FLEXIO_CTRL_FASTACC(userConfig->enableFastAccess) |
                FLEXIO_CTRL_FLEXEN(userConfig->enableFlexio));
    if (!userConfig->enableInDoze)
    {
        ctrlReg |= FLEXIO_CTRL_DOZEN_MASK;
    }

    base->CTRL = ctrlReg;
}

void FLEXIO_Deinit(FLEXIO_Type *base)
{
    FLEXIO_Enable(base, false);
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(s_flexioClocks[FLEXIO_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void FLEXIO_GetDefaultConfig(flexio_config_t *userConfig)
{
    assert(userConfig);

    userConfig->enableFlexio = true;
    userConfig->enableInDoze = false;
    userConfig->enableInDebug = true;
    userConfig->enableFastAccess = false;
}

void FLEXIO_Reset(FLEXIO_Type *base)
{
    /*do software reset, software reset operation affect all other FLEXIO registers except CTRL*/
    base->CTRL |= FLEXIO_CTRL_SWRST_MASK;
    base->CTRL = 0;
}

uint32_t FLEXIO_GetShifterBufferAddress(FLEXIO_Type *base, flexio_shifter_buffer_type_t type, uint8_t index)
{
    assert(index < FLEXIO_SHIFTBUF_COUNT);

    uint32_t address = 0;

    switch (type)
    {
        case kFLEXIO_ShifterBuffer:
            address = (uint32_t) & (base->SHIFTBUF[index]);
            break;

        case kFLEXIO_ShifterBufferBitSwapped:
            address = (uint32_t) & (base->SHIFTBUFBIS[index]);
            break;

        case kFLEXIO_ShifterBufferByteSwapped:
            address = (uint32_t) & (base->SHIFTBUFBYS[index]);
            break;

        case kFLEXIO_ShifterBufferBitByteSwapped:
            address = (uint32_t) & (base->SHIFTBUFBBS[index]);
            break;

#if defined(FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_NIBBLE_BYTE_SWAP) && FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_NIBBLE_BYTE_SWAP
        case kFLEXIO_ShifterBufferNibbleByteSwapped:
            address = (uint32_t) & (base->SHIFTBUFNBS[index]);
            break;

#endif
#if defined(FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_HALF_WORD_SWAP) && FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_HALF_WORD_SWAP
        case kFLEXIO_ShifterBufferHalfWordSwapped:
            address = (uint32_t) & (base->SHIFTBUFHWS[index]);
            break;

#endif
#if defined(FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_NIBBLE_SWAP) && FSL_FEATURE_FLEXIO_HAS_SHFT_BUFFER_NIBBLE_SWAP
        case kFLEXIO_ShifterBufferNibbleSwapped:
            address = (uint32_t) & (base->SHIFTBUFNIS[index]);
            break;

#endif
        default:
            break;
    }
    return address;
}

void FLEXIO_SetShifterConfig(FLEXIO_Type *base, uint8_t index, const flexio_shifter_config_t *shifterConfig)
{
    base->SHIFTCFG[index] = FLEXIO_SHIFTCFG_INSRC(shifterConfig->inputSource)
#if defined(FSL_FEATURE_FLEXIO_HAS_PARALLEL_WIDTH) && FSL_FEATURE_FLEXIO_HAS_PARALLEL_WIDTH
                            | FLEXIO_SHIFTCFG_PWIDTH(shifterConfig->parallelWidth)
#endif /* FSL_FEATURE_FLEXIO_HAS_PARALLEL_WIDTH */
                            | FLEXIO_SHIFTCFG_SSTOP(shifterConfig->shifterStop) |
                            FLEXIO_SHIFTCFG_SSTART(shifterConfig->shifterStart);

    base->SHIFTCTL[index] =
        FLEXIO_SHIFTCTL_TIMSEL(shifterConfig->timerSelect) | FLEXIO_SHIFTCTL_TIMPOL(shifterConfig->timerPolarity) |
        FLEXIO_SHIFTCTL_PINCFG(shifterConfig->pinConfig) | FLEXIO_SHIFTCTL_PINSEL(shifterConfig->pinSelect) |
        FLEXIO_SHIFTCTL_PINPOL(shifterConfig->pinPolarity) | FLEXIO_SHIFTCTL_SMOD(shifterConfig->shifterMode);
}

void FLEXIO_SetTimerConfig(FLEXIO_Type *base, uint8_t index, const flexio_timer_config_t *timerConfig)
{
    base->TIMCFG[index] =
        FLEXIO_TIMCFG_TIMOUT(timerConfig->timerOutput) | FLEXIO_TIMCFG_TIMDEC(timerConfig->timerDecrement) |
        FLEXIO_TIMCFG_TIMRST(timerConfig->timerReset) | FLEXIO_TIMCFG_TIMDIS(timerConfig->timerDisable) |
        FLEXIO_TIMCFG_TIMENA(timerConfig->timerEnable) | FLEXIO_TIMCFG_TSTOP(timerConfig->timerStop) |
        FLEXIO_TIMCFG_TSTART(timerConfig->timerStart);

    base->TIMCMP[index] = FLEXIO_TIMCMP_CMP(timerConfig->timerCompare);

    base->TIMCTL[index] = FLEXIO_TIMCTL_TRGSEL(timerConfig->triggerSelect) |
                          FLEXIO_TIMCTL_TRGPOL(timerConfig->triggerPolarity) |
                          FLEXIO_TIMCTL_TRGSRC(timerConfig->triggerSource) |
                          FLEXIO_TIMCTL_PINCFG(timerConfig->pinConfig) | FLEXIO_TIMCTL_PINSEL(timerConfig->pinSelect) |
                          FLEXIO_TIMCTL_PINPOL(timerConfig->pinPolarity) | FLEXIO_TIMCTL_TIMOD(timerConfig->timerMode);
}

status_t FLEXIO_RegisterHandleIRQ(void *base, void *handle, flexio_isr_t isr)
{
    assert(base);
    assert(handle);
    assert(isr);

    uint8_t index = 0;

    /* Find the an empty handle pointer to store the handle. */
    for (index = 0; index < FLEXIO_HANDLE_COUNT; index++)
    {
        if (s_flexioHandle[index] == NULL)
        {
            /* Register FLEXIO simulated driver base, handle and isr. */
            s_flexioType[index] = base;
            s_flexioHandle[index] = handle;
            s_flexioIsr[index] = isr;
            break;
        }
    }

    if (index == FLEXIO_HANDLE_COUNT)
    {
        return kStatus_OutOfRange;
    }
    else
    {
        return kStatus_Success;
    }
}

status_t FLEXIO_UnregisterHandleIRQ(void *base)
{
    assert(base);

    uint8_t index = 0;

    /* Find the index from base address mappings. */
    for (index = 0; index < FLEXIO_HANDLE_COUNT; index++)
    {
        if (s_flexioType[index] == base)
        {
            /* Unregister FLEXIO simulated driver handle and isr. */
            s_flexioType[index] = NULL;
            s_flexioHandle[index] = NULL;
            s_flexioIsr[index] = NULL;
            break;
        }
    }

    if (index == FLEXIO_HANDLE_COUNT)
    {
        return kStatus_OutOfRange;
    }
    else
    {
        return kStatus_Success;
    }
}

void FLEXIO_CommonIRQHandler(void)
{
    uint8_t index;

    for (index = 0; index < FLEXIO_HANDLE_COUNT; index++)
    {
        if (s_flexioHandle[index])
        {
            s_flexioIsr[index](s_flexioType[index], s_flexioHandle[index]);
        }
    }
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

void FLEXIO_DriverIRQHandler(void)
{
    FLEXIO_CommonIRQHandler();
}

void FLEXIO0_DriverIRQHandler(void)
{
    FLEXIO_CommonIRQHandler();
}

void FLEXIO1_DriverIRQHandler(void)
{
    FLEXIO_CommonIRQHandler();
}

void UART2_FLEXIO_DriverIRQHandler(void)
{
    FLEXIO_CommonIRQHandler();
}

void FLEXIO2_DriverIRQHandler(void)
{
    FLEXIO_CommonIRQHandler();
}
