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

#include "fsl_phy.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Defines the timeout macro. */
#define PHY_TIMEOUT_COUNT 0xFFFFFU

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get the ENET instance from peripheral base address.
 *
 * @param base ENET peripheral base address.
 * @return ENET instance.
 */
extern uint32_t ENET_GetInstance(ENET_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Pointers to enet clocks for each instance. */
extern clock_ip_name_t s_enetClock[FSL_FEATURE_SOC_ENET_COUNT];

/*******************************************************************************
 * Code
 ******************************************************************************/

status_t PHY_Init(ENET_Type *base, uint32_t phyAddr, uint32_t srcClock_Hz)
{
    uint32_t bssReg;
    uint32_t counter = PHY_TIMEOUT_COUNT;
    status_t result = kStatus_Success;
    uint32_t instance = ENET_GetInstance(base);

    /* Set SMI first. */
    CLOCK_EnableClock(s_enetClock[instance]);
    ENET_SetSMI(base, srcClock_Hz, false);

    /* Reset PHY. */
    result = PHY_Write(base, phyAddr, PHY_BASICCONTROL_REG, PHY_BCTL_RESET_MASK);
    if (result == kStatus_Success)
    {
        /* Set the negotiation. */
        result = PHY_Write(base, phyAddr, PHY_AUTONEG_ADVERTISE_REG,
                           (PHY_100BASETX_FULLDUPLEX_MASK | PHY_100BASETX_HALFDUPLEX_MASK |
                            PHY_10BASETX_FULLDUPLEX_MASK | PHY_10BASETX_HALFDUPLEX_MASK | 0x1U));
        if (result == kStatus_Success)
        {
            result = PHY_Write(base, phyAddr, PHY_BASICCONTROL_REG,
                               (PHY_BCTL_AUTONEG_MASK | PHY_BCTL_RESTART_AUTONEG_MASK));
            if (result == kStatus_Success)
            {
                /* Check auto negotiation complete. */
                while (counter --)
                {
                    result = PHY_Read(base, phyAddr, PHY_BASICSTATUS_REG, &bssReg);
                    if ( result == kStatus_Success)
                    {
                        if ((bssReg & PHY_BSTATUS_AUTONEGCOMP_MASK) != 0)
                        {
                            break;
                        } 
                    } 

                    if (!counter)
                    {
                        return kStatus_PHY_AutoNegotiateFail;
                    }
                }
            }
        }
    }

    return result;
}

status_t PHY_Write(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, uint32_t data)
{
    uint32_t counter;

    /* Clear the SMI interrupt event. */
    ENET_ClearInterruptStatus(base, ENET_EIR_MII_MASK);

    /* Starts a SMI write command. */
    ENET_StartSMIWrite(base, phyAddr, phyReg, kENET_MiiWriteValidFrame, data);

    /* Wait for SMI complete. */
    for (counter = PHY_TIMEOUT_COUNT; counter > 0; counter--)
    {
        if (ENET_GetInterruptStatus(base) & ENET_EIR_MII_MASK)
        {
            break;
        }
    }

    /* Check for timeout. */
    if (!counter)
    {
        return kStatus_PHY_SMIVisitTimeout;
    }

    /* Clear MII interrupt event. */
    ENET_ClearInterruptStatus(base, ENET_EIR_MII_MASK);

    return kStatus_Success;
}

status_t PHY_Read(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, uint32_t *dataPtr)
{
    assert(dataPtr);

    uint32_t counter;

    /* Clear the MII interrupt event. */
    ENET_ClearInterruptStatus(base, ENET_EIR_MII_MASK);

    /* Starts a SMI read command operation. */
    ENET_StartSMIRead(base, phyAddr, phyReg, kENET_MiiReadValidFrame);

    /* Wait for MII complete. */
    for (counter = PHY_TIMEOUT_COUNT; counter > 0; counter--)
    {
        if (ENET_GetInterruptStatus(base) & ENET_EIR_MII_MASK)
        {
            break;
        }
    }

    /* Check for timeout. */
    if (!counter)
    {
        return kStatus_PHY_SMIVisitTimeout;
    }

    /* Get data from MII register. */
    *dataPtr = ENET_ReadSMIData(base);

    /* Clear MII interrupt event. */
    ENET_ClearInterruptStatus(base, ENET_EIR_MII_MASK);

    return kStatus_Success;
}

status_t PHY_EnableLoopback(ENET_Type *base, uint32_t phyAddr, phy_loop_t mode, bool enable)
{
    status_t result;
    uint32_t data = 0;

    /* Set the loop mode. */
    if (enable)
    {
        if (mode == kPHY_LocalLoop)
        {
            /* First read the current status in control register. */
            result = PHY_Read(base, phyAddr, PHY_BASICCONTROL_REG, &data);
            if (result == kStatus_Success)
            {
                return PHY_Write(base, phyAddr, PHY_BASICCONTROL_REG, (data | PHY_BCTL_LOOP_MASK));
            }
        }
        else
        {
            /* First read the current status in control register. */
            result = PHY_Read(base, phyAddr, PHY_CONTROL2_REG, &data);
            if (result == kStatus_Success)
            {
                return PHY_Write(base, phyAddr, PHY_CONTROL2_REG, (data | PHY_CTL2_REMOTELOOP_MASK));
            }
        }
    }
    else
    {
        /* Disable the loop mode. */
        if (mode == kPHY_LocalLoop)
        {
            /* First read the current status in the basic control register. */
            result = PHY_Read(base, phyAddr, PHY_BASICCONTROL_REG, &data);
            if (result == kStatus_Success)
            {
                return PHY_Write(base, phyAddr, PHY_BASICCONTROL_REG, (data & ~PHY_BCTL_LOOP_MASK));
            }
        }
        else
        {
            /* First read the current status in control one register. */
            result = PHY_Read(base, phyAddr, PHY_CONTROL2_REG, &data);
            if (result == kStatus_Success)
            {
                return PHY_Write(base, phyAddr, PHY_CONTROL2_REG, (data & ~PHY_CTL2_REMOTELOOP_MASK));
            }
        }
    }
    return result;
}

status_t PHY_GetLinkStatus(ENET_Type *base, uint32_t phyAddr, bool *status)
{
    assert(status);

    status_t result = kStatus_Success;
    uint32_t data;

    /* Read the basic status register. */
    result = PHY_Read(base, phyAddr, PHY_BASICSTATUS_REG, &data);
    if (result == kStatus_Success)
    {
        if (!(PHY_BSTATUS_LINKSTATUS_MASK & data))
        {
            /* link down. */
            *status = false;
        }
        else
        {
            /* link up. */
            *status = true;
        }
    }
    return result;
}

status_t PHY_GetLinkSpeedDuplex(ENET_Type *base, uint32_t phyAddr, phy_speed_t *speed, phy_duplex_t *duplex)
{
    assert(duplex);

    status_t result = kStatus_Success;
    uint32_t data, ctlReg;

    /* Read the control two register. */
    result = PHY_Read(base, phyAddr, PHY_CONTROL1_REG, &ctlReg);
    if (result == kStatus_Success)
    {
        data = ctlReg & PHY_CTL1_SPEEDUPLX_MASK;
        if ((PHY_CTL1_10FULLDUPLEX_MASK == data) || (PHY_CTL1_100FULLDUPLEX_MASK == data))
        {
            /* Full duplex. */
            *duplex = kPHY_FullDuplex;
        }
        else
        {
            /* Half duplex. */
            *duplex = kPHY_HalfDuplex;
        }

        data = ctlReg & PHY_CTL1_SPEEDUPLX_MASK;
        if ((PHY_CTL1_100HALFDUPLEX_MASK == data) || (PHY_CTL1_100FULLDUPLEX_MASK == data))
        {
            /* 100M speed. */
            *speed = kPHY_Speed100M;
        }
        else
        { /* 10M speed. */
            *speed = kPHY_Speed10M;
        }
    }

    return result;
}
