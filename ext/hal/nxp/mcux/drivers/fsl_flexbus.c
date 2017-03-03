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

#include "fsl_flexbus.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Gets the instance from the base address
 *
 * @param base FLEXBUS peripheral base address
 *
 * @return The FLEXBUS instance
 */
static uint32_t FLEXBUS_GetInstance(FB_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Pointers to FLEXBUS bases for each instance. */
static FB_Type *const s_flexbusBases[] = FB_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to FLEXBUS clocks for each instance. */
static const clock_ip_name_t s_flexbusClocks[] = FLEXBUS_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint32_t FLEXBUS_GetInstance(FB_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < FSL_FEATURE_SOC_FB_COUNT; instance++)
    {
        if (s_flexbusBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < FSL_FEATURE_SOC_FB_COUNT);

    return instance;
}

void FLEXBUS_Init(FB_Type *base, const flexbus_config_t *config)
{
    assert(config != NULL);
    assert(config->chip < FB_CSAR_COUNT);
    assert(config->waitStates <= 0x3FU);

    uint32_t chip = 0;
    uint32_t reg_value = 0;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate clock for FLEXBUS */
    CLOCK_EnableClock(s_flexbusClocks[FLEXBUS_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset all the register to default state */
    for (chip = 0; chip < FB_CSAR_COUNT; chip++)
    {
        /* Reset CSMR register, all chips not valid (disabled) */
        base->CS[chip].CSMR = 0x0000U;
        /* Set default base address */
        base->CS[chip].CSAR &= (~FB_CSAR_BA_MASK);
        /* Reset FB_CSCRx register */
        base->CS[chip].CSCR = 0x0000U;
    }
    /* Set FB_CSPMCR register */
    /* FlexBus signal group 1 multiplex control */
    reg_value |= kFLEXBUS_MultiplexGroup1_FB_ALE << FB_CSPMCR_GROUP1_SHIFT;
    /* FlexBus signal group 2 multiplex control */
    reg_value |= kFLEXBUS_MultiplexGroup2_FB_CS4 << FB_CSPMCR_GROUP2_SHIFT;
    /* FlexBus signal group 3 multiplex control */
    reg_value |= kFLEXBUS_MultiplexGroup3_FB_CS5 << FB_CSPMCR_GROUP3_SHIFT;
    /* FlexBus signal group 4 multiplex control */
    reg_value |= kFLEXBUS_MultiplexGroup4_FB_TBST << FB_CSPMCR_GROUP4_SHIFT;
    /* FlexBus signal group 5 multiplex control */
    reg_value |= kFLEXBUS_MultiplexGroup5_FB_TA << FB_CSPMCR_GROUP5_SHIFT;
    /* Write to CSPMCR register */
    base->CSPMCR = reg_value;

    /* Update chip value */
    chip = config->chip;

    /* Base address */
    reg_value = config->chipBaseAddress;
    /* Write to CSAR register */
    base->CS[chip].CSAR = reg_value;
    /* Chip-select validation */
    reg_value = 0x1U << FB_CSMR_V_SHIFT;
    /* Write protect */
    reg_value |= (uint32_t)(config->writeProtect) << FB_CSMR_WP_SHIFT;
    /* Base address mask */
    reg_value |= config->chipBaseAddressMask << FB_CSMR_BAM_SHIFT;
    /* Write to CSMR register */
    base->CS[chip].CSMR = reg_value;
    /* Burst write */
    reg_value = (uint32_t)(config->burstWrite) << FB_CSCR_BSTW_SHIFT;
    /* Burst read */
    reg_value |= (uint32_t)(config->burstRead) << FB_CSCR_BSTR_SHIFT;
    /* Byte-enable mode */
    reg_value |= (uint32_t)(config->byteEnableMode) << FB_CSCR_BEM_SHIFT;
    /* Port size */
    reg_value |= (uint32_t)config->portSize << FB_CSCR_PS_SHIFT;
    /* The internal transfer acknowledge for accesses */
    reg_value |= (uint32_t)(config->autoAcknowledge) << FB_CSCR_AA_SHIFT;
    /* Byte-Lane shift */
    reg_value |= (uint32_t)config->byteLaneShift << FB_CSCR_BLS_SHIFT;
    /* The number of wait states */
    reg_value |= (uint32_t)config->waitStates << FB_CSCR_WS_SHIFT;
    /* Write address hold or deselect */
    reg_value |= (uint32_t)config->writeAddressHold << FB_CSCR_WRAH_SHIFT;
    /* Read address hold or deselect */
    reg_value |= (uint32_t)config->readAddressHold << FB_CSCR_RDAH_SHIFT;
    /* Address setup */
    reg_value |= (uint32_t)config->addressSetup << FB_CSCR_ASET_SHIFT;
    /* Extended transfer start/extended address latch */
    reg_value |= (uint32_t)(config->extendTransferAddress) << FB_CSCR_EXTS_SHIFT;
    /* Secondary wait state */
    reg_value |= (uint32_t)(config->secondaryWaitStates) << FB_CSCR_SWSEN_SHIFT;
    /* Write to CSCR register */
    base->CS[chip].CSCR = reg_value;
    /* FlexBus signal group 1 multiplex control */
    reg_value = (uint32_t)config->group1MultiplexControl << FB_CSPMCR_GROUP1_SHIFT;
    /* FlexBus signal group 2 multiplex control */
    reg_value |= (uint32_t)config->group2MultiplexControl << FB_CSPMCR_GROUP2_SHIFT;
    /* FlexBus signal group 3 multiplex control */
    reg_value |= (uint32_t)config->group3MultiplexControl << FB_CSPMCR_GROUP3_SHIFT;
    /* FlexBus signal group 4 multiplex control */
    reg_value |= (uint32_t)config->group4MultiplexControl << FB_CSPMCR_GROUP4_SHIFT;
    /* FlexBus signal group 5 multiplex control */
    reg_value |= (uint32_t)config->group5MultiplexControl << FB_CSPMCR_GROUP5_SHIFT;
    /* Write to CSPMCR register */
    base->CSPMCR = reg_value;
}

void FLEXBUS_Deinit(FB_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate clock for FLEXBUS */
    CLOCK_DisableClock(s_flexbusClocks[FLEXBUS_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void FLEXBUS_GetDefaultConfig(flexbus_config_t *config)
{
    config->chip = 0;                                  /* Chip 0 FlexBus for validation */
    config->writeProtect = 0;                          /* Write accesses are allowed */
    config->burstWrite = 0;                            /* Burst-Write disable */
    config->burstRead = 0;                             /* Burst-Read disable */
    config->byteEnableMode = 0;                        /* Byte-Enable mode is asserted for data write only */
    config->autoAcknowledge = true;                    /* Auto-Acknowledge enable */
    config->extendTransferAddress = 0;                 /* Extend transfer start/extend address latch disable */
    config->secondaryWaitStates = 0;                   /* Secondary wait state disable */
    config->byteLaneShift = kFLEXBUS_NotShifted;       /* Byte-Lane shift disable */
    config->writeAddressHold = kFLEXBUS_Hold1Cycle;    /* Write address hold 1 cycles */
    config->readAddressHold = kFLEXBUS_Hold1Or0Cycles; /* Read address hold 0 cycles */
    config->addressSetup =
        kFLEXBUS_FirstRisingEdge;      /* Assert ~FB_CSn on the first rising clock edge after the address is asserted */
    config->portSize = kFLEXBUS_1Byte; /* 1 byte port size of transfer */
    config->group1MultiplexControl = kFLEXBUS_MultiplexGroup1_FB_ALE;  /* FB_ALE */
    config->group2MultiplexControl = kFLEXBUS_MultiplexGroup2_FB_CS4;  /* FB_CS4 */
    config->group3MultiplexControl = kFLEXBUS_MultiplexGroup3_FB_CS5;  /* FB_CS5 */
    config->group4MultiplexControl = kFLEXBUS_MultiplexGroup4_FB_TBST; /* FB_TBST */
    config->group5MultiplexControl = kFLEXBUS_MultiplexGroup5_FB_TA;   /* FB_TA */
}
