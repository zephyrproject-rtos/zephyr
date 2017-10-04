/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/sysctl_a.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

/* Define to ensure that our current MSP432 has the SYSCTL_A module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_SYSCTL_A__

void SysCtl_A_getTLVInfo(uint_fast8_t tag, uint_fast8_t instance,
        uint_fast8_t *length, uint32_t **data_address)
{
    /* TLV Structure Start Address */
    uint32_t *TLV_address = (uint32_t *) TLV_START;

    if(*TLV_address == 0xFFFFFFFF)
    {
        *length = 0;
        // Return 0 for TAG not found
        *data_address = 0;
        return;
    }

    while (((*TLV_address != tag)) // check for tag and instance
    && (*TLV_address != TLV_TAGEND))         // do range check first
    {
        if (*TLV_address == tag)
        {
            if (instance == 0)
            {
                break;
            }

            /* Repeat until requested instance is reached */
            instance--;
        }

        TLV_address += (*(TLV_address + 1)) + 2;
    }

    /* Check if Tag match happened... */
    if (*TLV_address == tag)
    {
        /* Return length = Address + 1 */
        *length = (*(TLV_address + 1)) * 4;
        /* Return address of first data/value info = Address + 2 */
        *data_address = (uint32_t *) (TLV_address + 2);
    }
    // If there was no tag match and the end of TLV structure was reached..
    else
    {
        // Return 0 for TAG not found
        *length = 0;
        // Return 0 for TAG not found
        *data_address = 0;
    }
}

uint_least32_t SysCtl_A_getSRAMSize(void)
{
    return SYSCTL_A->SRAM_SIZE;
}

uint_least32_t SysCtl_A_getFlashSize(void)
{
    return SYSCTL_A->MAINFLASH_SIZE;
}

uint_least32_t SysCtl_A_getInfoFlashSize(void)
{
    return SYSCTL_A->INFOFLASH_SIZE;
}

void SysCtl_A_disableNMISource(uint_fast8_t flags)
{
    SYSCTL_A->NMI_CTLSTAT &= ~(flags);
}

void SysCtl_A_enableNMISource(uint_fast8_t flags)
{
    SYSCTL_A->NMI_CTLSTAT |= flags;
}

uint_fast8_t SysCtl_A_getNMISourceStatus(void)
{
    return SYSCTL_A->NMI_CTLSTAT & (SYSCTL_A_NMI_CTLSTAT_CS_FLG | 
                                    SYSCTL_A_NMI_CTLSTAT_PSS_FLG | 
                                    SYSCTL_A_NMI_CTLSTAT_PCM_FLG | 
                                    SYSCTL_A_NMI_CTLSTAT_PIN_FLG);
}

void SysCtl_A_rebootDevice(void)
{
    SYSCTL_A->REBOOT_CTL = (SYSCTL_A_REBOOT_CTL_REBOOT | SYSCTL_A_REBOOT_KEY);
}

void SysCtl_A_enablePeripheralAtCPUHalt(uint_fast16_t devices)
{
    SYSCTL_A->PERIHALT_CTL &= ~devices;
}

void SysCtl_A_disablePeripheralAtCPUHalt(uint_fast16_t devices)
{
    SYSCTL_A->PERIHALT_CTL |= devices;
}

void SysCtl_A_setWDTTimeoutResetType(uint_fast8_t resetType)
{
    if (resetType)
        SYSCTL_A->WDTRESET_CTL |= SYSCTL_A_WDTRESET_CTL_TIMEOUT;
    else
        SYSCTL_A->WDTRESET_CTL &= ~SYSCTL_A_WDTRESET_CTL_TIMEOUT;
}

void SysCtl_A_setWDTPasswordViolationResetType(uint_fast8_t resetType)
{
    if (resetType)
        SYSCTL_A->WDTRESET_CTL |= SYSCTL_A_WDTRESET_CTL_VIOLATION;
    else
        SYSCTL_A->WDTRESET_CTL &= ~SYSCTL_A_WDTRESET_CTL_VIOLATION;
}

void SysCtl_A_enableGlitchFilter(void)
{
    SYSCTL_A->DIO_GLTFLT_CTL |= SYSCTL_A_DIO_GLTFLT_CTL_GLTCH_EN;
}

void SysCtl_A_disableGlitchFilter(void)
{
    SYSCTL_A->DIO_GLTFLT_CTL &= ~SYSCTL_A_DIO_GLTFLT_CTL_GLTCH_EN;
}

uint_fast16_t SysCtl_A_getTempCalibrationConstant(uint32_t refVoltage,
        uint32_t temperature)
{
    return HWREG16(TLV_BASE + refVoltage + temperature);
}

bool SysCtl_A_enableSRAM(uint32_t addr)
{
    uint32_t bankSize, bankBit;

    /* If SRAM is busy, return false */
    if(!(SYSCTL_A->SRAM_STAT & SYSCTL_A_SRAM_STAT_BNKEN_RDY))
        return false;

    /* Grabbing the bank size */
    bankSize = SysCtl_A_getSRAMSize()  / SYSCTL_A->SRAM_NUMBANKS;
    bankBit = (addr - SRAM_BASE) / bankSize;

    if (bankBit < 32)
    {
        SYSCTL_A->SRAM_BANKEN_CTL0 |= (1 << bankBit);
    } else if (bankBit < 64)
    {
        SYSCTL_A->SRAM_BANKEN_CTL1 |= (1 << (bankBit - 32));
    } else if (bankBit < 96)
    {
        SYSCTL_A->SRAM_BANKEN_CTL2 |= (1 << (bankBit - 64));
    } else
    {
        SYSCTL_A->SRAM_BANKEN_CTL3 |= (1 << (bankBit - 96));
    }


    return true;
}

bool SysCtl_A_disableSRAM(uint32_t addr)
{
    uint32_t bankSize, bankBit;

    /* If SRAM is busy, return false */
    if(!(SYSCTL_A->SRAM_STAT & SYSCTL_A_SRAM_STAT_BNKEN_RDY))
        return false;

    /* Grabbing the bank size */
    bankSize = SysCtl_A_getSRAMSize() / SYSCTL_A->SRAM_NUMBANKS;
    bankBit = (addr - SRAM_BASE) / bankSize;

    if (bankBit < 32)
    {
        SYSCTL_A->SRAM_BANKEN_CTL0 &= ~(0xFFFFFFFF << bankBit);
    } else if (bankBit < 64)
    {
        SYSCTL_A->SRAM_BANKEN_CTL1 &= ~(0xFFFFFFFF << (bankBit - 32));
    } else if (bankBit < 96)
    {
        SYSCTL_A->SRAM_BANKEN_CTL2 &= ~(0xFFFFFFFF << (bankBit - 64));
    } else
    {
        SYSCTL_A->SRAM_BANKEN_CTL3 &= ~(0xFFFFFFFF << (bankBit - 96));
    }


    return true;
}

bool SysCtl_A_enableSRAMRetention(uint32_t startAddr,
        uint32_t endAddr)
{
    uint32_t blockSize, blockBitStart, blockBitEnd;

    if (startAddr > endAddr)
        return false;

    /* If SRAM is busy, return false */
    if(!(SYSCTL_A->SRAM_STAT & SYSCTL_A_SRAM_STAT_BLKRET_RDY))
        return false;

    /* Getting how big each bank is and how many blocks we have per bank */
    blockSize = SysCtl_A_getSRAMSize() / SYSCTL_A->SRAM_NUMBLOCKS;
    blockBitStart = (startAddr - SRAM_BASE) / blockSize;
    blockBitEnd = (endAddr - SRAM_BASE) / blockSize;

    if (blockBitStart < 32)
    {
        if (blockBitEnd < 32)
        {
            SYSCTL_A->SRAM_BLKRET_CTL0 |= (0xFFFFFFFF >> (31 - blockBitEnd))
                    & (0xFFFFFFFF << blockBitStart);
            return true;
        } else if (blockBitEnd < 64)
        {
            SYSCTL_A->SRAM_BLKRET_CTL0 |= (0xFFFFFFFF << blockBitStart);
            SYSCTL_A->SRAM_BLKRET_CTL1 |= (0xFFFFFFFF
                    >> (31 - (blockBitEnd - 32)));
        } else if (blockBitEnd < 96)
        {
            SYSCTL_A->SRAM_BLKRET_CTL0 |= (0xFFFFFFFF << blockBitStart);
            SYSCTL_A->SRAM_BLKRET_CTL1 = 0xFFFFFFFF;
            SYSCTL_A->SRAM_BLKRET_CTL2 |= (0xFFFFFFFF
                    >> (31 - (blockBitEnd - 64)));
        } else
        {
            SYSCTL_A->SRAM_BLKRET_CTL0 |= (0xFFFFFFFF << blockBitStart);
            SYSCTL_A->SRAM_BLKRET_CTL1 = 0xFFFFFFFF;
            SYSCTL_A->SRAM_BLKRET_CTL2 = 0xFFFFFFFF;
            SYSCTL_A->SRAM_BLKRET_CTL3 |= (0xFFFFFFFF
                    >> (31 - (blockBitEnd - 96)));
        }
    } else if (blockBitStart < 64)
    {
        if (blockBitEnd < 64)
        {
            SYSCTL_A->SRAM_BLKRET_CTL1 |= ((0xFFFFFFFF
                    >> (31 - (blockBitEnd - 32)))
                    & (0xFFFFFFFF << (blockBitStart - 32)));
            return true;
        }

        SYSCTL_A->SRAM_BLKRET_CTL1 = (0xFFFFFFFF << (blockBitStart - 32));

        if (blockBitEnd < 96)
        {
            SYSCTL_A->SRAM_BLKRET_CTL2 |= (0xFFFFFFFF
                    >> (31 - (blockBitEnd - 64)));
        } else
        {

            SYSCTL_A->SRAM_BLKRET_CTL2 |= 0xFFFFFFFF;
            SYSCTL_A->SRAM_BLKRET_CTL3 |= (0xFFFFFFFF
                    >> (31 - (blockBitEnd - 96)));
        }
    } else if (blockBitStart < 96)
    {
        if (blockBitEnd < 96)
        {
            SYSCTL_A->SRAM_BLKRET_CTL2 |= (0xFFFFFFFF
                    >> (31 - (blockBitEnd - 64)))
                    & (0xFFFFFFFF << (blockBitStart - 64));
            return true;
        } else
        {
            SYSCTL_A->SRAM_BLKRET_CTL2 |= (0xFFFFFFFF << (blockBitStart - 64));
            SYSCTL_A->SRAM_BLKRET_CTL3 |= (0xFFFFFFFF
                    >> (31 - (blockBitEnd - 96)));
        }
    } else
    {
        SYSCTL_A->SRAM_BLKRET_CTL3 |= (0xFFFFFFFF >> (31 - (blockBitEnd - 96)))
                & (0xFFFFFFFF << (blockBitStart - 96));
    }

    return true;

}

bool SysCtl_A_disableSRAMRetention(uint32_t startAddr,
        uint32_t endAddr)
{
    uint32_t blockSize, blockBitStart, blockBitEnd;

    if (startAddr > endAddr)
        return false;

    /* If SRAM is busy, return false */
    if(!(SYSCTL_A->SRAM_STAT & SYSCTL_A_SRAM_STAT_BLKRET_RDY))
        return false;


    /* Getting how big each bank is and how many blocks we have per bank */
    blockSize = SysCtl_A_getSRAMSize() / SYSCTL_A->SRAM_NUMBLOCKS;
    blockBitStart = (startAddr - SRAM_BASE) / blockSize;
    blockBitEnd = (endAddr - SRAM_BASE) / blockSize;

    if (blockBitStart < 32)
    {
        if (blockBitEnd < 32)
        {
            SYSCTL_A->SRAM_BLKRET_CTL0 &= ~((0xFFFFFFFF >> (31 - blockBitEnd))
                    & (0xFFFFFFFF << blockBitStart));
            return true;
        }

        SYSCTL_A->SRAM_BLKRET_CTL0 &= ~((0xFFFFFFFF << blockBitStart));

        if (blockBitEnd < 64)
        {
            SYSCTL_A->SRAM_BLKRET_CTL1 &= ~((0xFFFFFFFF
                    >> (31 - (blockBitEnd - 32))));
        } else if (blockBitEnd < 96)
        {
            SYSCTL_A->SRAM_BLKRET_CTL1 = 0;
            SYSCTL_A->SRAM_BLKRET_CTL2 &= ~(0xFFFFFFFF
                    >> (31 - (blockBitEnd - 64)));
        } else
        {
            SYSCTL_A->SRAM_BLKRET_CTL1 = 0;
            SYSCTL_A->SRAM_BLKRET_CTL2 = 0;
            SYSCTL_A->SRAM_BLKRET_CTL3 &= ~(0xFFFFFFFF
                    >> (31 - (blockBitEnd - 96)));
        }
    } else if (blockBitStart < 64)
    {
        if (blockBitEnd < 64)
        {
            SYSCTL_A->SRAM_BLKRET_CTL1 &= ~((0xFFFFFFFF
                    >> (31 - (blockBitEnd - 32)))
                    & (0xFFFFFFFF << (blockBitStart - 32)));
            return true;
        }

        SYSCTL_A->SRAM_BLKRET_CTL1 &= ~(0xFFFFFFFF << (blockBitStart - 32));

        if (blockBitEnd < 96)
        {
            SYSCTL_A->SRAM_BLKRET_CTL2 &= ~(0xFFFFFFFF
                    >> (31 - (blockBitEnd - 64)));
        } else
        {

            SYSCTL_A->SRAM_BLKRET_CTL2 = 0;
            SYSCTL_A->SRAM_BLKRET_CTL3 &= ~(0xFFFFFFFF
                    >> (31 - (blockBitEnd - 96)));
        }
    } else if (blockBitStart < 96)
    {
        if (blockBitEnd < 96)
        {
            SYSCTL_A->SRAM_BLKRET_CTL2 &= ~((0xFFFFFFFF
                    >> (31 - (blockBitEnd - 64)))
                    & (0xFFFFFFFF << (blockBitStart - 64)));
        } else
        {
            SYSCTL_A->SRAM_BLKRET_CTL2 &= ~(0xFFFFFFFF << (blockBitStart - 64));
            SYSCTL_A->SRAM_BLKRET_CTL3 &= ~(0xFFFFFFFF
                    >> (31 - (blockBitEnd - 96)));
        }
    } else
    {
        SYSCTL_A->SRAM_BLKRET_CTL3 &= ~((0xFFFFFFFF >> (31 - (blockBitEnd - 96)))
                & (0xFFFFFFFF << (blockBitStart - 96)));
    }

    return true;
}

#endif /* __MCU_HAS_SYSCTL_A__ */
