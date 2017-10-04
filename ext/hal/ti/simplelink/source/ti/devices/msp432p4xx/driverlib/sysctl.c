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
#include <ti/devices/msp432p4xx/driverlib/sysctl.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

/* Define to ensure that our current MSP432 has the SYSCTL module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_SYSCTL__

#ifdef DEBUG

static bool SysCtlSRAMBankValid(uint8_t sramBank)
{
    return(
            sramBank == SYSCTL_SRAM_BANK7 ||
            sramBank == SYSCTL_SRAM_BANK6 ||
            sramBank == SYSCTL_SRAM_BANK5 ||
            sramBank == SYSCTL_SRAM_BANK4 ||
            sramBank == SYSCTL_SRAM_BANK3 ||
            sramBank == SYSCTL_SRAM_BANK2 ||
            sramBank == SYSCTL_SRAM_BANK1
    );
}

static bool SysCtlSRAMBankValidRet(uint8_t sramBank)
{
    sramBank &= ~(SYSCTL_SRAM_BANK7 & SYSCTL_SRAM_BANK6 &
            SYSCTL_SRAM_BANK5 & SYSCTL_SRAM_BANK4 &
            SYSCTL_SRAM_BANK3 & SYSCTL_SRAM_BANK2 &
            SYSCTL_SRAM_BANK1);

    return (sramBank == 0);
}

static bool SysCtlPeripheralIsValid (uint16_t hwPeripheral)
{
    hwPeripheral &= ~(SYSCTL_PERIPH_DMA & SYSCTL_PERIPH_WDT &
            SYSCTL_PERIPH_ADC & SYSCTL_PERIPH_EUSCIB3 &
            SYSCTL_PERIPH_EUSCIB2 & SYSCTL_PERIPH_EUSCIB1 &
            SYSCTL_PERIPH_EUSCIB0 & SYSCTL_PERIPH_EUSCIA3 &
            SYSCTL_PERIPH_EUSCIA2 & SYSCTL_PERIPH_EUSCIA1 &
            SYSCTL_PERIPH_EUSCIA0 & SYSCTL_PERIPH_TIMER32_0_MODULE &
            SYSCTL_PERIPH_TIMER16_3 & SYSCTL_PERIPH_TIMER16_2 &
            SYSCTL_PERIPH_TIMER16_2 & SYSCTL_PERIPH_TIMER16_1 &
            SYSCTL_PERIPH_TIMER16_0);

    return (hwPeripheral == 0);
}
#endif

void SysCtl_getTLVInfo(uint_fast8_t tag, uint_fast8_t instance,
        uint_fast8_t *length, uint32_t **data_address)
{
    /* TLV Structure Start Address */
    uint32_t *TLV_address = (uint32_t *) TLV_START;

    while (((*TLV_address != tag)) // check for tag and instance
            && (*TLV_address != TLV_TAGEND))         // do range check first
    {
        if (*TLV_address == tag)
        {
            if(instance == 0)
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
        *length = (*(TLV_address + 1))*4;
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

uint_least32_t SysCtl_getSRAMSize(void)
{
    return SYSCTL->SRAM_SIZE;
}

uint_least32_t SysCtl_getFlashSize(void)
{
    return SYSCTL->FLASH_SIZE;
}

void SysCtl_disableNMISource(uint_fast8_t flags)
{
    SYSCTL->NMI_CTLSTAT &= ~(flags);
}

void SysCtl_enableNMISource(uint_fast8_t flags)
{
    SYSCTL->NMI_CTLSTAT |= flags;
}

uint_fast8_t SysCtl_getNMISourceStatus(void)
{
    return SYSCTL->NMI_CTLSTAT & (SYSCTL_NMI_CTLSTAT_CS_FLG | 
                                    SYSCTL_NMI_CTLSTAT_PSS_FLG | 
                                    SYSCTL_NMI_CTLSTAT_PCM_FLG | 
                                    SYSCTL_NMI_CTLSTAT_PIN_FLG);
}

void SysCtl_enableSRAMBank(uint_fast8_t sramBank)
{
    ASSERT(SysCtlSRAMBankValid(sramBank));

    /* Waiting for SRAM Ready Bit to be set */
    while (!(SYSCTL->SRAM_BANKEN & SYSCTL_SRAM_BANKEN_SRAM_RDY))
        ;

    SYSCTL->SRAM_BANKEN = (sramBank | SYSCTL_SRAM_BANKEN_BNK0_EN);
}

void SysCtl_disableSRAMBank(uint_fast8_t sramBank)
{
    ASSERT(SysCtlSRAMBankValid(sramBank));

    /* Waiting for SRAM Ready Bit to be set */
    while (!(SYSCTL->SRAM_BANKEN & SYSCTL_SRAM_BANKEN_SRAM_RDY))
       ;

    switch (sramBank)
    {
    case SYSCTL_SRAM_BANK7:
        sramBank = SYSCTL_SRAM_BANK6 +  SYSCTL_SRAM_BANK5 + SYSCTL_SRAM_BANK4
                        + SYSCTL_SRAM_BANK3 + SYSCTL_SRAM_BANK2
                        + SYSCTL_SRAM_BANK1;
        break;
    case SYSCTL_SRAM_BANK6:
        sramBank =   SYSCTL_SRAM_BANK5 + SYSCTL_SRAM_BANK4
                        + SYSCTL_SRAM_BANK3 + SYSCTL_SRAM_BANK2
                        + SYSCTL_SRAM_BANK1;
        break;
    case SYSCTL_SRAM_BANK5:
        sramBank =   SYSCTL_SRAM_BANK4 + SYSCTL_SRAM_BANK3
                + SYSCTL_SRAM_BANK2 + SYSCTL_SRAM_BANK1;
        break;
    case SYSCTL_SRAM_BANK4:
        sramBank =   SYSCTL_SRAM_BANK3 + SYSCTL_SRAM_BANK2
                        + SYSCTL_SRAM_BANK1;
        break;
    case SYSCTL_SRAM_BANK3:
        sramBank =   SYSCTL_SRAM_BANK2 + SYSCTL_SRAM_BANK1;
        break;
    case SYSCTL_SRAM_BANK2:
        sramBank = SYSCTL_SRAM_BANK1;
        break;
    case SYSCTL_SRAM_BANK1:
        sramBank = 0;
        break;
    default:
        return;
    }

    SYSCTL->SRAM_BANKEN = (sramBank | SYSCTL_SRAM_BANKEN_BNK0_EN);
}

void SysCtl_enableSRAMBankRetention(uint_fast8_t sramBank)
{
    ASSERT(SysCtlSRAMBankValidRet(sramBank));

    /* Waiting for SRAM Ready Bit to be set */
    while (!(SYSCTL->SRAM_BANKRET & SYSCTL_SRAM_BANKRET_SRAM_RDY))
            ;

    SYSCTL->SRAM_BANKRET |= sramBank;
}

void SysCtl_disableSRAMBankRetention(uint_fast8_t sramBank)
{
    ASSERT(SysCtlSRAMBankValidRet(sramBank));

    /* Waiting for SRAM Ready Bit to be set */
    while (!(SYSCTL->SRAM_BANKRET & SYSCTL_SRAM_BANKRET_SRAM_RDY))
            ;

    SYSCTL->SRAM_BANKRET &= ~sramBank;
}

void SysCtl_rebootDevice(void)
{
    SYSCTL->REBOOT_CTL = (SYSCTL_REBOOT_CTL_REBOOT | SYSCTL_REBOOT_KEY);
}

void SysCtl_enablePeripheralAtCPUHalt(uint_fast16_t devices)
{
    ASSERT(SysCtlPeripheralIsValid(devices));
    SYSCTL->PERIHALT_CTL &= ~devices;
}

void SysCtl_disablePeripheralAtCPUHalt(uint_fast16_t devices)
{
    ASSERT(SysCtlPeripheralIsValid(devices));
    SYSCTL->PERIHALT_CTL |= devices;
}

void SysCtl_setWDTTimeoutResetType(uint_fast8_t resetType)
{
    if (resetType)
        SYSCTL->WDTRESET_CTL |=
                SYSCTL_WDTRESET_CTL_TIMEOUT;
    else
        SYSCTL->WDTRESET_CTL &= ~SYSCTL_WDTRESET_CTL_TIMEOUT;
}

void SysCtl_setWDTPasswordViolationResetType(uint_fast8_t resetType)
{
    ASSERT(resetType <= SYSCTL_HARD_RESET);

    if (resetType)
        SYSCTL->WDTRESET_CTL |=
                SYSCTL_WDTRESET_CTL_VIOLATION;
    else
        SYSCTL->WDTRESET_CTL &= ~SYSCTL_WDTRESET_CTL_VIOLATION;
}

void SysCtl_enableGlitchFilter(void)
{
    SYSCTL->DIO_GLTFLT_CTL |= SYSCTL_DIO_GLTFLT_CTL_GLTCH_EN;
}

void SysCtl_disableGlitchFilter(void)
{
    SYSCTL->DIO_GLTFLT_CTL &= ~SYSCTL_DIO_GLTFLT_CTL_GLTCH_EN;
}

uint_fast16_t SysCtl_getTempCalibrationConstant(uint32_t refVoltage,
        uint32_t temperature)
{
    return HWREG16(TLV_BASE + refVoltage + temperature);
}

#endif /* __MCU_HAS_SYSCTL__ */
