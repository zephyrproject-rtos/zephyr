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

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/flash_a.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/cpu.h>
#include <ti/devices/msp432p4xx/driverlib/sysctl_a.h>

/* Define to ensure that our current MSP432 has the FLCTL_A module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_FLCTL_A__

static const uint32_t MAX_ERASE_NO_TLV = 50;
static const uint32_t MAX_PROGRAM_NO_TLV = 5;

static const uint32_t __getBurstProgramRegs[16] =
{ (uint32_t)&FLCTL_A->PRGBRST_DATA0_0, (uint32_t)&FLCTL_A->PRGBRST_DATA0_1,
       (uint32_t) &FLCTL_A->PRGBRST_DATA0_2, (uint32_t)&FLCTL_A->PRGBRST_DATA0_3,
        (uint32_t)&FLCTL_A->PRGBRST_DATA1_0,(uint32_t) &FLCTL_A->PRGBRST_DATA1_1,
        (uint32_t)&FLCTL_A->PRGBRST_DATA1_2, (uint32_t)&FLCTL_A->PRGBRST_DATA1_3,
       (uint32_t) &FLCTL_A->PRGBRST_DATA2_0, (uint32_t)&FLCTL_A->PRGBRST_DATA2_1,
        (uint32_t)&FLCTL_A->PRGBRST_DATA2_2,(uint32_t) &FLCTL_A->PRGBRST_DATA2_3,
       (uint32_t) &FLCTL_A->PRGBRST_DATA3_0,(uint32_t) &FLCTL_A->PRGBRST_DATA3_1,
       (uint32_t) &FLCTL_A->PRGBRST_DATA3_2,(uint32_t) &FLCTL_A->PRGBRST_DATA3_3 };

static void __saveProtectionRegisters(__FlashCtl_ProtectionRegister *pReg)
{
    pReg->B0_INFO_R0 = FLCTL_A->BANK0_INFO_WEPROT;
    pReg->B1_INFO_R0 = FLCTL_A->BANK1_INFO_WEPROT;
    pReg->B0_MAIN_R0 = FLCTL_A->BANK0_MAIN_WEPROT0;
    pReg->B0_MAIN_R1 = FLCTL_A->BANK0_MAIN_WEPROT1;
    pReg->B0_MAIN_R2 = FLCTL_A->BANK0_MAIN_WEPROT2;
    pReg->B0_MAIN_R3 = FLCTL_A->BANK0_MAIN_WEPROT3;
    pReg->B0_MAIN_R4 = FLCTL_A->BANK0_MAIN_WEPROT4;
    pReg->B0_MAIN_R5 = FLCTL_A->BANK0_MAIN_WEPROT5;
    pReg->B0_MAIN_R6 = FLCTL_A->BANK0_MAIN_WEPROT6;
    pReg->B0_MAIN_R7 = FLCTL_A->BANK0_MAIN_WEPROT7;
    pReg->B1_MAIN_R0 = FLCTL_A->BANK1_MAIN_WEPROT0;
    pReg->B1_MAIN_R1 = FLCTL_A->BANK1_MAIN_WEPROT1;
    pReg->B1_MAIN_R2 = FLCTL_A->BANK1_MAIN_WEPROT2;
    pReg->B1_MAIN_R3 = FLCTL_A->BANK1_MAIN_WEPROT3;
    pReg->B1_MAIN_R4 = FLCTL_A->BANK1_MAIN_WEPROT4;
    pReg->B1_MAIN_R5 = FLCTL_A->BANK1_MAIN_WEPROT5;
    pReg->B1_MAIN_R6 = FLCTL_A->BANK1_MAIN_WEPROT6;
    pReg->B1_MAIN_R7 = FLCTL_A->BANK1_MAIN_WEPROT7;
}

static void __restoreProtectionRegisters(__FlashCtl_ProtectionRegister *pReg)
{
    FLCTL_A->BANK0_INFO_WEPROT = pReg->B0_INFO_R0;
    FLCTL_A->BANK1_INFO_WEPROT = pReg->B1_INFO_R0;
    FLCTL_A->BANK0_MAIN_WEPROT0 = pReg->B0_MAIN_R0;
    FLCTL_A->BANK0_MAIN_WEPROT1 = pReg->B0_MAIN_R1;
    FLCTL_A->BANK0_MAIN_WEPROT2 = pReg->B0_MAIN_R2;
    FLCTL_A->BANK0_MAIN_WEPROT3 = pReg->B0_MAIN_R3;
    FLCTL_A->BANK0_MAIN_WEPROT4 = pReg->B0_MAIN_R4;
    FLCTL_A->BANK0_MAIN_WEPROT5 = pReg->B0_MAIN_R5;
    FLCTL_A->BANK0_MAIN_WEPROT6 = pReg->B0_MAIN_R6;
    FLCTL_A->BANK0_MAIN_WEPROT7 = pReg->B0_MAIN_R7;
    FLCTL_A->BANK1_MAIN_WEPROT0 = pReg->B1_MAIN_R0;
    FLCTL_A->BANK1_MAIN_WEPROT1 = pReg->B1_MAIN_R1;
    FLCTL_A->BANK1_MAIN_WEPROT2 = pReg->B1_MAIN_R2;
    FLCTL_A->BANK1_MAIN_WEPROT3 = pReg->B1_MAIN_R3;
    FLCTL_A->BANK1_MAIN_WEPROT4 = pReg->B1_MAIN_R4;
    FLCTL_A->BANK1_MAIN_WEPROT5 = pReg->B1_MAIN_R5;
    FLCTL_A->BANK1_MAIN_WEPROT6 = pReg->B1_MAIN_R6;
    FLCTL_A->BANK1_MAIN_WEPROT7 = pReg->B1_MAIN_R7;
}

void FlashCtl_A_getMemoryInfo(uint32_t addr, uint32_t *bankNum,
            uint32_t *sectorNum)
{
    uint32_t bankLimit;

    bankLimit = SysCtl_A_getFlashSize() / 2;

    if (addr > bankLimit)
    {
        *(bankNum) = FLASH_A_BANK1;
        addr = (addr - bankLimit);
    } else
    {
        *(bankNum) = FLASH_A_BANK0;
    }

    *(sectorNum) = (addr) / FLASH_A_SECTOR_SIZE;
}

static bool _FlashCtl_A_Program8(uint32_t src, uint32_t dest, uint32_t mTries)
{
    uint32_t ii;
    uint8_t data;

    /* Enabling the correct verification settings  */
    FlashCtl_A_setProgramVerification(FLASH_A_REGPRE | FLASH_A_REGPOST);
    FlashCtl_A_clearProgramVerification(FLASH_A_BURSTPOST | FLASH_A_BURSTPRE);

    data = HWREG8(src);

    for (ii = 0; ii < mTries; ii++)
    {
        /* Clearing flags */
        FLCTL_A->CLRIFG |= (FLASH_A_PROGRAM_ERROR | FLASH_A_POSTVERIFY_FAILED
                | FLASH_A_PREVERIFY_FAILED | FLASH_A_WRDPRGM_COMPLETE);

        HWREG8(dest) = data;

        while (!(FlashCtl_A_getInterruptStatus() & FLASH_A_WRDPRGM_COMPLETE))
        {
            __no_operation();
        }

        /* Pre-Verify */
        if ((BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_VER_PRE_OFS)
                && BITBAND_PERI(FLCTL_A->IFG, FLCTL_A_IFG_AVPRE_OFS)))
        {
            data = __FlashCtl_A_remaskData8Pre(data, dest);

            if (data != 0xFF)
            {
                FlashCtl_A_clearProgramVerification(FLASH_A_REGPRE);
                continue;
            }

        }

        /* Post Verify */
        if ((BITBAND_PERI(FLCTL_A->IFG, FLCTL_A_IFG_AVPST_OFS)))
        {
            data = __FlashCtl_A_remaskData8Post(data, dest);

            /* Seeing if we actually need to do another pulse */
            if (data == 0xFF)
                return true;

            FlashCtl_A_setProgramVerification(FLASH_A_REGPRE | FLASH_A_REGPOST);
            continue;
        }

        /* If we got this far, return true */
        return true;

    }

    return false;

}

static bool _FlashCtl_A_Program32(uint32_t src, uint32_t dest, uint32_t mTries)
{
    uint32_t ii;
    uint32_t data;

    /* Enabling the correct verification settings  */
    FlashCtl_A_setProgramVerification(FLASH_A_REGPRE | FLASH_A_REGPOST);
    FlashCtl_A_clearProgramVerification(FLASH_A_BURSTPOST | FLASH_A_BURSTPRE);

    data = HWREG32(src);

    for (ii = 0; ii < mTries; ii++)
    {
        /* Clearing flags */
        FLCTL_A->CLRIFG |= (FLASH_A_PROGRAM_ERROR | FLASH_A_POSTVERIFY_FAILED
                | FLASH_A_PREVERIFY_FAILED | FLASH_A_WRDPRGM_COMPLETE);

        HWREG32(dest) = data;

        while (!(FlashCtl_A_getInterruptStatus() & FLASH_A_WRDPRGM_COMPLETE))
        {
            __no_operation();
        }

        /* Pre-Verify */
        if ((BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_VER_PRE_OFS)
                && BITBAND_PERI(FLCTL_A->IFG, FLCTL_A_IFG_AVPRE_OFS)))
        {
            data = __FlashCtl_A_remaskData32Pre(data, dest);

            if (data != 0xFFFFFFFF)
            {

                FlashCtl_A_clearProgramVerification(FLASH_A_REGPRE);
                continue;
            }

        }

        /* Post Verify */
        if ((BITBAND_PERI(FLCTL_A->IFG, FLCTL_A_IFG_AVPST_OFS)))
        {
            data = __FlashCtl_A_remaskData32Post(data, dest);

            /* Seeing if we actually need to do another pulse */
            if (data == 0xFFFFFFFF)
                return true;

            FlashCtl_A_setProgramVerification(FLASH_A_REGPRE | FLASH_A_REGPOST);
            continue;
        }

        /* If we got this far, return true */
        return true;

    }

    return false;

}

static bool _FlashCtl_A_ProgramBurst(uint32_t src, uint32_t dest,
        uint32_t length, uint32_t mTries)
{
    uint32_t bCalc, otpOffset, ii, jj;
    bool res;

    /* Setting verification */
    FlashCtl_A_clearProgramVerification(FLASH_A_REGPRE | FLASH_A_REGPOST);
    FlashCtl_A_setProgramVerification(FLASH_A_BURSTPOST | FLASH_A_BURSTPRE);

    /* Assume Failure */
    res = false;

    /* Waiting for idle status */
    while ((FLCTL_A->PRGBRST_CTLSTAT & FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }

    /* Setting/clearing INFO flash flags as appropriate */
    if (dest >= SysCtl_A_getFlashSize())
    {
        FLCTL_A->PRGBRST_CTLSTAT = (FLCTL_A->PRGBRST_CTLSTAT
                & ~FLCTL_A_PRGBRST_CTLSTAT_TYPE_MASK)
                | FLCTL_A_PRGBRST_CTLSTAT_TYPE_1;
        otpOffset = __INFO_FLASH_A_TECH_START__;
    } else
    {
        FLCTL_A->PRGBRST_CTLSTAT = (FLCTL_A->PRGBRST_CTLSTAT
                & ~FLCTL_A_PRGBRST_CTLSTAT_TYPE_MASK)
                | FLCTL_A_PRGBRST_CTLSTAT_TYPE_0;
        otpOffset = 0;
    }

    bCalc = 0;
    FLCTL_A->PRGBRST_STARTADDR = (dest - otpOffset);

    /* Initially populating the burst registers */
    while (bCalc < 16 && length != 0)
    {
        HWREG32(__getBurstProgramRegs[bCalc]) = HWREG32(src);
        bCalc++;
        length -= 4;
        src += 4;
    }

    for (ii = 0; ii < mTries; ii++)
    {
        /* Clearing Flags */
        FLCTL_A->CLRIFG |= (FLASH_A_BRSTPRGM_COMPLETE
                | FLASH_A_POSTVERIFY_FAILED | FLASH_A_PREVERIFY_FAILED);

        /* Waiting for idle status */
        while ((FLCTL_A->PRGBRST_CTLSTAT
                & FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
                != FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_0)
        {
            BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                    FLCTL_A_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
        }

        /* Start the burst program */
        FLCTL_A->PRGBRST_CTLSTAT = (FLCTL_A->PRGBRST_CTLSTAT
                & ~(FLCTL_A_PRGBRST_CTLSTAT_LEN_MASK))
                | ((bCalc / 4) << FLASH_A_BURST_PRG_BIT)
                | FLCTL_A_PRGBRST_CTLSTAT_START;

        /* Waiting for the burst to complete */
        while ((FLCTL_A->PRGBRST_CTLSTAT
                & FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
                != FLASH_A_PRGBRSTCTLSTAT_BURSTSTATUS_COMPLETE)
        {
            __no_operation();
        }

        /* Checking for errors and clearing/masking */

        /* Address Error */
        if (BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_ADDR_ERR_OFS))
        {
            goto BurstCleanUp;
        }

        /* Pre-Verify Error */
        if (BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_AUTO_PRE_OFS)
                && BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                        FLCTL_A_PRGBRST_CTLSTAT_PRE_ERR_OFS))
        {
            __FlashCtl_A_remaskBurstDataPre(dest, bCalc * 4);

            for (jj = 0; jj < bCalc; jj++)
            {
                if (HWREG32(__getBurstProgramRegs[jj]) != 0xFFFFFFFF)
                {
                    FlashCtl_A_clearProgramVerification(FLASH_A_BURSTPRE);
                    break;
                }
            }

            if (jj != bCalc)
                continue;
        }

        /* Post-Verify Error */
        if (BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_PST_ERR_OFS))
        {
            __FlashCtl_A_remaskBurstDataPost(dest, bCalc * 4);

            for (jj = 0; jj < bCalc; jj++)
            {
                if ((HWREG32(__getBurstProgramRegs[jj])) != 0xFFFFFFFF)
                {
                    FlashCtl_A_setProgramVerification(
                    FLASH_A_BURSTPOST | FLASH_A_BURSTPRE);
                    break;
                }
            }

            if (jj != bCalc)
                continue;

        }

        /* If we got this far, the program happened */
        res = true;
        goto BurstCleanUp;
    }

    BurstCleanUp:
    /* Waiting for idle status */
    while ((FLCTL_A->PRGBRST_CTLSTAT & FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }
    return res;
}

void FlashCtl_A_enableReadBuffering(uint_fast8_t memoryBank,
        uint_fast8_t accessMethod)
{
    if (memoryBank == FLASH_A_BANK0 && accessMethod == FLASH_A_DATA_READ)
        BITBAND_PERI(FLCTL_A->BANK0_RDCTL, FLCTL_A_BANK0_RDCTL_BUFD_OFS) = 1;
    else if (memoryBank == FLASH_A_BANK1 && accessMethod == FLASH_A_DATA_READ)
        BITBAND_PERI(FLCTL_A->BANK1_RDCTL, FLCTL_A_BANK1_RDCTL_BUFD_OFS) = 1;
    else if (memoryBank == FLASH_A_BANK0
            && accessMethod == FLASH_A_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL_A->BANK0_RDCTL, FLCTL_A_BANK0_RDCTL_BUFI_OFS) = 1;
    else if (memoryBank == FLASH_A_BANK1
            && accessMethod == FLASH_A_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL_A->BANK1_RDCTL, FLCTL_A_BANK1_RDCTL_BUFI_OFS) = 1;
    else
        ASSERT(false);
}

void FlashCtl_A_disableReadBuffering(uint_fast8_t memoryBank,
        uint_fast8_t accessMethod)
{
    if (memoryBank == FLASH_A_BANK0 && accessMethod == FLASH_A_DATA_READ)
        BITBAND_PERI(FLCTL_A->BANK0_RDCTL, FLCTL_A_BANK0_RDCTL_BUFD_OFS) = 0;
    else if (memoryBank == FLASH_A_BANK1 && accessMethod == FLASH_A_DATA_READ)
        BITBAND_PERI(FLCTL_A->BANK1_RDCTL, FLCTL_A_BANK1_RDCTL_BUFD_OFS) = 0;
    else if (memoryBank == FLASH_A_BANK0
            && accessMethod == FLASH_A_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL_A->BANK0_RDCTL, FLCTL_A_BANK0_RDCTL_BUFI_OFS) = 0;
    else if (memoryBank == FLASH_A_BANK1
            && accessMethod == FLASH_A_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL_A->BANK1_RDCTL, FLCTL_A_BANK1_RDCTL_BUFI_OFS) = 0;
    else
        ASSERT(false);
}

bool FlashCtl_A_isMemoryProtected(uint32_t addr)
{
    volatile uint32_t *configRegister;
    uint32_t memoryBit, cutOffMemory;

    if (addr >= SysCtl_A_getFlashSize())
    {
        cutOffMemory = (SysCtl_A_getInfoFlashSize() >> 1) + __INFO_FLASH_A_TECH_START__;

        if (addr < cutOffMemory)
        {
            memoryBit = (addr - __INFO_FLASH_A_TECH_START__)
                    / FLASH_A_SECTOR_SIZE;
            configRegister = &FLCTL_A->BANK0_INFO_WEPROT
                    + ((memoryBit >> 5));
            memoryBit &= 0x1F;
        } else
        {
            memoryBit = (addr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            configRegister = &FLCTL_A->BANK1_INFO_WEPROT
                    + ((memoryBit >> 5));
            memoryBit &= 0x1F;
        }
    } else
    {
        cutOffMemory = SysCtl_A_getFlashSize() >> 1;

        if (addr < cutOffMemory)
        {
            memoryBit = addr / FLASH_A_SECTOR_SIZE;
            configRegister = &FLCTL_A->BANK0_MAIN_WEPROT0
                    + ((memoryBit >> 5));
            memoryBit &= 0x1F;
        } else
        {
            memoryBit = (addr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            configRegister = &FLCTL_A->BANK1_MAIN_WEPROT0
                    + ((memoryBit >> 5));
            memoryBit &= 0x1F;
        }
    }

    return (*configRegister & (1 << memoryBit));
}

bool FlashCtl_A_isMemoryRangeProtected(uint32_t startAddr, uint32_t endAddr)
{
    uint32_t ii;

    if (startAddr > endAddr)
        return false;

    startAddr = (startAddr & 0xFFFFF000);
    endAddr = (endAddr & 0xFFFFF000);

    for (ii = startAddr; ii <= endAddr; ii+=FLASH_A_SECTOR_SIZE)
    {
        if (!FlashCtl_A_isMemoryProtected(ii))
            return false;
    }
    return true;
}

bool FlashCtl_A_unprotectMemory(uint32_t startAddr, uint32_t endAddr)
{
    uint32_t startBankBit, endBankBit, cutOffMemory;
    volatile uint32_t* baseStartConfig, *baseEndConfig;

    if (startAddr > endAddr)
        return false;

    if (endAddr >= SysCtl_A_getFlashSize())
    {
        cutOffMemory = (SysCtl_A_getInfoFlashSize() >> 1)
                + __INFO_FLASH_A_TECH_START__;

        if (endAddr < cutOffMemory && startAddr < cutOffMemory)
        {
            startBankBit = (startAddr - __INFO_FLASH_A_TECH_START__)
                    / FLASH_A_SECTOR_SIZE;
            endBankBit = (endAddr - __INFO_FLASH_A_TECH_START__)
                    / FLASH_A_SECTOR_SIZE;
            FLCTL_A->BANK0_INFO_WEPROT &= ~(0xFFFFFFFF >> (31 - endBankBit))
                    & (0xFFFFFFFF << startBankBit);
        } else if (endAddr > cutOffMemory && startAddr > cutOffMemory)
        {
            startBankBit = (startAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            endBankBit = (endAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            FLCTL_A->BANK1_INFO_WEPROT &= ~((0xFFFFFFFF >> (31 - endBankBit))
                    & (0xFFFFFFFF << startBankBit));
        } else
        {
            startBankBit = (startAddr - __INFO_FLASH_A_TECH_START__)
                    / FLASH_A_SECTOR_SIZE;
            endBankBit = (endAddr - cutOffMemory)/FLASH_A_SECTOR_SIZE;
            FLCTL_A->BANK0_INFO_WEPROT &= ~(0xFFFFFFFF << startBankBit) & 0xF;
            FLCTL_A->BANK1_INFO_WEPROT &= ~(0xFFFFFFFF >> (31 - (endBankBit))) & 0xF;
        }

        return true;

    } else
    {
        cutOffMemory = SysCtl_A_getFlashSize() >> 1;

        if (endAddr < cutOffMemory)
        {
            endBankBit = endAddr / FLASH_A_SECTOR_SIZE;
            baseEndConfig = &FLCTL_A->BANK0_MAIN_WEPROT0
                    + ((endBankBit >> 5));
            endBankBit &= 0x1F;
        } else
        {
            endBankBit = (endAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            baseEndConfig = &FLCTL_A->BANK1_MAIN_WEPROT0
                    + (endBankBit >> 5);
            endBankBit &= 0x1F;
        }

        if (startAddr < cutOffMemory)
        {
            startBankBit = (startAddr / FLASH_A_SECTOR_SIZE);
            baseStartConfig = &FLCTL_A->BANK0_MAIN_WEPROT0
                    + (startBankBit >> 5);
            startBankBit &= 0x1F;
        } else
        {
            startBankBit = (startAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            baseStartConfig = &FLCTL_A->BANK1_MAIN_WEPROT0
                    + (startBankBit >> 5);
            startBankBit &= 0x1F;
        }

        if (baseStartConfig == baseEndConfig)
        {
            (*baseStartConfig) &= ~((0xFFFFFFFF >> (31 - endBankBit))
                    & (0xFFFFFFFF << startBankBit));
            return true;
        }

        *baseStartConfig &= ~(0xFFFFFFFF << startBankBit);

        if (baseStartConfig == &FLCTL_A->BANK0_MAIN_WEPROT7)
        {
            baseStartConfig = &FLCTL_A->BANK1_MAIN_WEPROT0;
        } else
        {
            baseStartConfig++;
        }

        while (baseStartConfig != baseEndConfig)
        {
            *baseStartConfig = 0;

            if (baseStartConfig == &FLCTL_A->BANK0_MAIN_WEPROT7)
            {
                baseStartConfig = &FLCTL_A->BANK1_MAIN_WEPROT0;
            } else
            {
                baseStartConfig++;
            }
        }

        (*baseEndConfig) &= ~(0xFFFFFFFF >> (31 - (endBankBit)));
    }

    return true;

}

bool FlashCtl_A_protectMemory(uint32_t startAddr, uint32_t endAddr)
{
    uint32_t startBankBit, endBankBit, cutOffMemory;
    volatile uint32_t* baseStartConfig, *baseEndConfig;

    if (startAddr > endAddr)
        return false;

    if (endAddr >= SysCtl_A_getFlashSize())
    {
        cutOffMemory = (SysCtl_A_getInfoFlashSize() >> 1)
                        + __INFO_FLASH_A_TECH_START__;

        if (endAddr < cutOffMemory && startAddr < cutOffMemory)
        {
            startBankBit = (startAddr - __INFO_FLASH_A_TECH_START__)
                    / FLASH_A_SECTOR_SIZE;
            endBankBit = (endAddr - __INFO_FLASH_A_TECH_START__)
                    / FLASH_A_SECTOR_SIZE;
            FLCTL_A->BANK0_INFO_WEPROT |= (0xFFFFFFFF >> (31 - endBankBit))
                    & (0xFFFFFFFF << startBankBit);
        } else if (endAddr > cutOffMemory && startAddr > cutOffMemory)
        {
            startBankBit = (startAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            endBankBit = (endAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            FLCTL_A->BANK1_INFO_WEPROT |= (0xFFFFFFFF >> (31 - endBankBit))
                    & (0xFFFFFFFF << startBankBit);
        } else
        {
            startBankBit = (startAddr - __INFO_FLASH_A_TECH_START__)
                    / FLASH_A_SECTOR_SIZE;
            endBankBit = (endAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            FLCTL_A->BANK0_INFO_WEPROT |= (0xFFFFFFFF << startBankBit);
            FLCTL_A->BANK1_INFO_WEPROT |= (0xFFFFFFFF >> (31 - (endBankBit)));
        }

        return true;

    } else
    {
        cutOffMemory = SysCtl_A_getFlashSize() >> 1;

        if (endAddr < cutOffMemory)
        {
            endBankBit = endAddr / FLASH_A_SECTOR_SIZE;
            baseEndConfig = &FLCTL_A->BANK0_MAIN_WEPROT0
                    + ((endBankBit >> 5));
            endBankBit &= 0x1F;
        } else
        {
            endBankBit = (endAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            baseEndConfig = &FLCTL_A->BANK1_MAIN_WEPROT0
                    + (endBankBit >> 5);
            endBankBit &= 0x1F;
        }

        if (startAddr < cutOffMemory)
        {
            startBankBit = (startAddr / FLASH_A_SECTOR_SIZE);
            baseStartConfig = &FLCTL_A->BANK0_MAIN_WEPROT0
                    + (startBankBit >> 5);
            startBankBit &= 0x1F;
        } else
        {
            startBankBit = (startAddr - cutOffMemory) / FLASH_A_SECTOR_SIZE;
            baseStartConfig = &FLCTL_A->BANK1_MAIN_WEPROT0
                    + (startBankBit >> 5);
            startBankBit &= 0x1F;
        }

        if (baseStartConfig == baseEndConfig)
        {
            (*baseStartConfig) |= (0xFFFFFFFF >> (31 - endBankBit))
                    & (0xFFFFFFFF << startBankBit);
            return true;
        }

        *baseStartConfig |= (0xFFFFFFFF << startBankBit);

        if (baseStartConfig == &FLCTL_A->BANK0_MAIN_WEPROT7)
        {
            baseStartConfig = &FLCTL_A->BANK1_MAIN_WEPROT0;
        } else
        {
            baseStartConfig++;
        }

        while (baseStartConfig != baseEndConfig)
        {
            *baseStartConfig = 0xFFFFFFFF;

            if (baseStartConfig == &FLCTL_A->BANK0_MAIN_WEPROT7)
            {
                baseStartConfig = &FLCTL_A->BANK1_MAIN_WEPROT0;
            } else
            {
                baseStartConfig++;
            }
        }

        (*baseEndConfig) |= (0xFFFFFFFF >> (31 - (endBankBit)));
    }

    return true;

}

bool FlashCtl_A_verifyMemory(void* verifyAddr, uint32_t length,
        uint_fast8_t pattern)
{
    uint32_t memoryPattern, addr, otpOffset;
    uint32_t b0WaitState, b1WaitState, intStatus;
    uint32_t bankOneStart, startBank, endBank;
    uint_fast8_t b0readMode, b1readMode;
    uint_fast8_t memoryType;
    bool res;

    ASSERT(pattern == FLASH_A_0_PATTERN || pattern == FLASH_A_1_PATTERN);

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Casting and determining the memory that we need to use */
    addr = (uint32_t) verifyAddr;
    memoryType = (addr >= SysCtl_A_getFlashSize()) ?
    FLASH_A_INFO_SPACE :
                                                FLASH_A_MAIN_SPACE;

    /* Assuming Failure */
    res = false;

    /* Finding out which bank we are in */
    if (addr >= SysCtl_A_getFlashSize())
    {
        bankOneStart = __INFO_FLASH_A_TECH_MIDDLE__;
    } else
    {
        bankOneStart = SysCtl_A_getFlashSize() / 2;
    }
    startBank = addr < (bankOneStart) ? FLASH_A_BANK0 : FLASH_A_BANK1;
    endBank = (addr + length) < (bankOneStart) ? FLASH_A_BANK0 : FLASH_A_BANK1;

    /* Saving context and changing read modes */
    b0WaitState = FlashCtl_A_getWaitState(startBank);
    b0readMode = FlashCtl_A_getReadMode(startBank);

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setWaitState(startBank, (2 * b0WaitState) + 1);

    if (startBank != endBank)
    {
        b1WaitState = FlashCtl_A_getWaitState(endBank);
        b1readMode = FlashCtl_A_getReadMode(endBank);
        FlashCtl_A_setWaitState(endBank, (2 * b1WaitState) + 1);
    }

    /* Changing to the relevant VERIFY mode */
    if (pattern == FLASH_A_1_PATTERN)
    {
        FlashCtl_A_setReadMode(startBank, FLASH_A_ERASE_VERIFY_READ_MODE);

        if (startBank != endBank)
        {
            FlashCtl_A_setReadMode(endBank, FLASH_A_ERASE_VERIFY_READ_MODE);
        }

        memoryPattern = 0xFFFFFFFF;
    } else
    {
        FlashCtl_A_setReadMode(startBank, FLASH_A_PROGRAM_VERIFY_READ_MODE);

        if (startBank != endBank)
        {
            FlashCtl_A_setReadMode(endBank, FLASH_A_PROGRAM_VERIFY_READ_MODE);
        }

        memoryPattern = 0;
    }

    /* Taking care of byte accesses */
    while ((addr & 0x03) && (length > 0))
    {
        if (HWREG8(addr++) != ((uint8_t) memoryPattern))
            goto FlashVerifyCleanup;
        length--;
    }

    /* Making sure we are aligned by 128-bit address */
    while (((addr & 0x0F)) && (length > 3))
    {
        if (HWREG32(addr) != memoryPattern)
            goto FlashVerifyCleanup;

        addr = addr + 4;
        length = length - 4;
    }

    /* Burst Verify */
    if (length > 63)
    {
        /* Setting/clearing INFO flash flags as appropriate */
        if (addr >= SysCtl_A_getFlashSize())
        {
            FLCTL_A->RDBRST_CTLSTAT = (FLCTL_A->RDBRST_CTLSTAT
                    & ~FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_MASK)
                    | FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_1;
            otpOffset = __INFO_FLASH_A_TECH_START__;
        } else
        {
            FLCTL_A->RDBRST_CTLSTAT = (FLCTL_A->RDBRST_CTLSTAT
                    & ~FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_MASK)
                    | FLCTL_A_RDBRST_CTLSTAT_MEM_TYPE_0;
            otpOffset = 0;
        }

        /* Clearing any lingering fault flags  and preparing burst verify*/
        BITBAND_PERI(FLCTL_A->RDBRST_CTLSTAT,
                FLCTL_A_RDBRST_CTLSTAT_CLR_STAT_OFS) = 1;
        FLCTL_A->RDBRST_FAILCNT = 0;
        FLCTL_A->RDBRST_STARTADDR = addr - otpOffset;
        FLCTL_A->RDBRST_LEN = (length & 0xFFFFFFF0);
        addr += FLCTL_A->RDBRST_LEN;
        length = length & 0xF;

        /* Starting Burst Verify */
        FLCTL_A->RDBRST_CTLSTAT = (FLCTL_A_RDBRST_CTLSTAT_STOP_FAIL | pattern
                | memoryType | FLCTL_A_RDBRST_CTLSTAT_START);

        /* While the burst read hasn't finished */
        while ((FLCTL_A->RDBRST_CTLSTAT & FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_MASK)
                != FLCTL_A_RDBRST_CTLSTAT_BRST_STAT_3)
        {
            __no_operation();
        }

        /* Checking  for a verification/access error/failure */
        if (BITBAND_PERI(FLCTL_A->RDBRST_CTLSTAT,
                FLCTL_A_RDBRST_CTLSTAT_CMP_ERR_OFS)
                || BITBAND_PERI(FLCTL_A->RDBRST_CTLSTAT,
                        FLCTL_A_RDBRST_CTLSTAT_ADDR_ERR_OFS)
                || FLCTL_A->RDBRST_FAILCNT)
        {
            goto FlashVerifyCleanup;
        }
    }

    /* Remaining Words */
    while (length > 3)
    {
        if (HWREG32(addr) != memoryPattern)
            goto FlashVerifyCleanup;

        addr = addr + 4;
        length = length - 4;
    }

    /* Remaining Bytes */
    while (length > 0)
    {
        if (HWREG8(addr++) != ((uint8_t) memoryPattern))
            goto FlashVerifyCleanup;
        length--;
    }

    /* If we got this far, that means it no failure happened */
    res = true;

    FlashVerifyCleanup:

    /* Clearing the Read Burst flag and returning */
    BITBAND_PERI(FLCTL_A->RDBRST_CTLSTAT, FLCTL_A_RDBRST_CTLSTAT_CLR_STAT_OFS) =
            1;

    FlashCtl_A_setReadMode(startBank, b0readMode);
    FlashCtl_A_setWaitState(startBank, b0WaitState);

    if (startBank != endBank)
    {
        FlashCtl_A_setReadMode(endBank, b1readMode);
        FlashCtl_A_setWaitState(endBank, b1WaitState);
    }

    if (intStatus == 0)
        Interrupt_enableMaster();

    return res;
}

bool FlashCtl_A_setReadMode(uint32_t flashBank, uint32_t readMode)
{

    if (FLCTL_A->POWER_STAT & FLCTL_A_POWER_STAT_RD_2T)
        return false;

    if (flashBank == FLASH_A_BANK0)
    {
        FLCTL_A->BANK0_RDCTL = (FLCTL_A->BANK0_RDCTL
                & ~FLCTL_A_BANK0_RDCTL_RD_MODE_MASK) | readMode;
        while ((FLCTL_A->BANK0_RDCTL & FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_MASK)
                != (readMode<<16))
            ;
    } else if (flashBank == FLASH_A_BANK1)
    {
        FLCTL_A->BANK1_RDCTL = (FLCTL_A->BANK1_RDCTL
                & ~FLCTL_A_BANK1_RDCTL_RD_MODE_MASK) | readMode;
        while ((FLCTL_A->BANK1_RDCTL & FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_MASK)
                != (readMode<<16))
            ;
    } else
    {
        ASSERT(false);
        return false;
    }

    return true;
}

uint32_t FlashCtl_A_getReadMode(uint32_t flashBank)
{
    if (flashBank == FLASH_A_BANK0)
    {
        return (FLCTL_A->BANK0_RDCTL & FLCTL_A_BANK0_RDCTL_RD_MODE_STATUS_MASK)
                    >> 16;
    } else if (flashBank == FLASH_A_BANK1)
    {
        return (FLCTL_A->BANK1_RDCTL & FLCTL_A_BANK1_RDCTL_RD_MODE_STATUS_MASK) 
                    >> 16;
    } else
    {
        ASSERT(false);
        return 0;
    }
}

void FlashCtl_A_initiateMassErase(void)
{
    /* Clearing old mass erase flags */
    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) =
            1;

    /* Performing the mass erase */
    FLCTL_A->ERASE_CTLSTAT |= (FLCTL_A_ERASE_CTLSTAT_MODE
            | FLCTL_A_ERASE_CTLSTAT_START);
}

bool FlashCtl_A_performMassErase(void)
{
    uint32_t flashSize, ii, intStatus, jj;
    uint32_t mTries, tlvLength;
    SysCtl_A_FlashTLV_Info *flInfo;
    __FlashCtl_ProtectionRegister protectRegs;
    bool res, needAnotherPulse;

    /* Parsing the TLV and getting the maximum erase pulses */
    SysCtl_A_getTLVInfo(TLV_TAG_FLASHCTL, 0, &tlvLength, (uint32_t**) &flInfo);

    if (tlvLength == 0 || flInfo->maxErasePulses == 0)
    {
        mTries = MAX_ERASE_NO_TLV;
    } else
    {
        mTries = flInfo->maxErasePulses;
    }

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Assume Failure */
    res = false;

    /* Saving off protection settings so we can restore them later */
    __saveProtectionRegisters(&protectRegs);

    for(jj=0;jj<mTries;jj++)
    {
        needAnotherPulse = false;

        /* Clearing old mass erase flags */
        BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) =
                1;

        /* Performing the mass erase */
        FLCTL_A->ERASE_CTLSTAT |= (FLCTL_A_ERASE_CTLSTAT_MODE
                | FLCTL_A_ERASE_CTLSTAT_START);

        while ((FLCTL_A->ERASE_CTLSTAT & FLCTL_A_ERASE_CTLSTAT_STATUS_MASK)
                == FLCTL_A_ERASE_CTLSTAT_STATUS_1
                || (FLCTL_A->ERASE_CTLSTAT & FLCTL_A_ERASE_CTLSTAT_STATUS_MASK)
                        == FLCTL_A_ERASE_CTLSTAT_STATUS_2)
        {
            __no_operation();
        }

        /* Return false if an address error */
        if (BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT,
                FLCTL_A_ERASE_CTLSTAT_ADDR_ERR_OFS))
            goto MassEraseCleanup;

        /* Verifying the user memory that might have been erased */
        flashSize = SysCtl_A_getFlashSize();

        /* Clearing old flag */
        BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) =
                1;

        for (ii = 0; ii < flashSize; ii += FLASH_A_SECTOR_SIZE)
        {
            if (!FlashCtl_A_isMemoryProtected(ii))
            {
                if (!FlashCtl_A_verifyMemory((void*) ii, FLASH_A_SECTOR_SIZE,
                FLASH_A_1_PATTERN))
                {
                    needAnotherPulse = true;
                }
                else
                {
                    FlashCtl_A_protectMemory(ii,ii);
                }
            }
        }

        /* Verifying the INFO memory that might be protected */
        flashSize = SysCtl_A_getInfoFlashSize() + __INFO_FLASH_A_TECH_START__;

        for (ii = __INFO_FLASH_A_TECH_START__; ii < flashSize; ii +=
                FLASH_A_SECTOR_SIZE)
        {
            if (!FlashCtl_A_isMemoryProtected(ii))
            {
                if (!FlashCtl_A_verifyMemory((void*) ii, FLASH_A_SECTOR_SIZE,
                FLASH_A_1_PATTERN))
                {
                    needAnotherPulse = true;
                }
                else
                {
                    FlashCtl_A_protectMemory(ii,ii);
                }
            }
        }

        if(!needAnotherPulse)
        {
            break;
        }
    }

    /* If we got this far and didn't do the max number of tries,
     * the mass erase happened */
    if(jj != mTries)
    {
        res = true;
    }

MassEraseCleanup:

    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT,
            FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) = 1;
    __restoreProtectionRegisters(&protectRegs);

    if (intStatus == 0)
        Interrupt_enableMaster();

    return res;
}

bool FlashCtl_A_eraseSector(uint32_t addr)
{
    uint_fast8_t memoryType, ii;
    uint32_t otpOffset = 0;
    uint32_t intStatus;
    uint_fast8_t mTries, tlvLength;
    SysCtl_A_FlashTLV_Info *flInfo;
    bool res;

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Assuming Failure */
    res = false;

    memoryType = addr >= SysCtl_A_getFlashSize() ? FLASH_A_INFO_SPACE :
                                                    FLASH_A_MAIN_SPACE;

    /* Parsing the TLV and getting the maximum erase pulses */
    SysCtl_A_getTLVInfo(TLV_TAG_FLASHCTL, 0, &tlvLength, (uint32_t**) &flInfo);

    if (tlvLength == 0 || flInfo->maxErasePulses == 0)
    {
        mTries = MAX_ERASE_NO_TLV;
    } else
    {
        mTries = flInfo->maxErasePulses;
    }

    /* We can only erase on 4KB boundaries */
    while (addr & 0xFFF)
    {
        addr--;
    }

    /* Clearing the status */
    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) =
            1;

    if (memoryType == FLASH_A_INFO_SPACE)
    {
        otpOffset = __INFO_FLASH_A_TECH_START__;
        FLCTL_A->ERASE_CTLSTAT = (FLCTL_A->ERASE_CTLSTAT
                & ~(FLCTL_A_ERASE_CTLSTAT_TYPE_MASK))
                | FLCTL_A_ERASE_CTLSTAT_TYPE_1;

    } else
    {
        otpOffset = 0;
        FLCTL_A->ERASE_CTLSTAT = (FLCTL_A->ERASE_CTLSTAT
                & ~(FLCTL_A_ERASE_CTLSTAT_TYPE_MASK))
                | FLCTL_A_ERASE_CTLSTAT_TYPE_0;
    }

    /* Clearing old flags  and setting up the erase */
    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_MODE_OFS) = 0;
    FLCTL_A->ERASE_SECTADDR = addr - otpOffset;

    for (ii = 0; ii < mTries; ii++)
    {
        /* Clearing the status */
        BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) =
                1;

        /* Starting the erase */
        BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_START_OFS) =
                1;

        while ((FLCTL_A->ERASE_CTLSTAT & FLCTL_A_ERASE_CTLSTAT_STATUS_MASK)
                == FLCTL_A_ERASE_CTLSTAT_STATUS_1
                || (FLCTL_A->ERASE_CTLSTAT & FLCTL_A_ERASE_CTLSTAT_STATUS_MASK)
                        == FLCTL_A_ERASE_CTLSTAT_STATUS_2)
        {
            __no_operation();
        }

        /* Return false if an address error */
        if (BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT,
                FLCTL_A_ERASE_CTLSTAT_ADDR_ERR_OFS))
        {
            goto SectorEraseCleanup;
        }
        /* Erase verifying */
        if (FlashCtl_A_verifyMemory((void*) addr, FLASH_A_SECTOR_SIZE,
        FLASH_A_1_PATTERN))
        {
            res = true;
            goto SectorEraseCleanup;
        }

    }

    SectorEraseCleanup:

    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) =
            1;

    if (intStatus == 0)
        Interrupt_enableMaster();

    return res;
}

void FlashCtl_A_initiateSectorErase(uint32_t addr)
{
    uint_fast8_t memoryType;
    uint32_t otpOffset = 0;

    memoryType = addr >= SysCtl_A_getFlashSize() ?
    FLASH_A_INFO_SPACE :
                                              FLASH_A_MAIN_SPACE;

    /* We can only erase on 4KB boundaries */
    while (addr & 0xFFF)
    {
        addr--;
    }

    /* Clearing the status */
    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_CLR_STAT_OFS) =
            1;

    if (memoryType == FLASH_A_INFO_SPACE)
    {
        otpOffset = __INFO_FLASH_A_TECH_START__;
        FLCTL_A->ERASE_CTLSTAT = (FLCTL_A->ERASE_CTLSTAT
                & ~(FLCTL_A_ERASE_CTLSTAT_TYPE_MASK))
                | FLCTL_A_ERASE_CTLSTAT_TYPE_1;

    } else
    {
        otpOffset = 0;
        FLCTL_A->ERASE_CTLSTAT = (FLCTL_A->ERASE_CTLSTAT
                & ~(FLCTL_A_ERASE_CTLSTAT_TYPE_MASK))
                | FLCTL_A_ERASE_CTLSTAT_TYPE_0;
    }

    /* Clearing old flags  and setting up the erase */
    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_MODE_OFS) = 0;
    FLCTL_A->ERASE_SECTADDR = addr - otpOffset;

    /* Starting the erase */
    BITBAND_PERI(FLCTL_A->ERASE_CTLSTAT, FLCTL_A_ERASE_CTLSTAT_START_OFS) = 1;

}

bool FlashCtl_A_programMemory(void* src, void* dest, uint32_t length)
{
    uint32_t destAddr, srcAddr, burstLength, intStatus;
    bool res;
    uint_fast8_t mTries, tlvLength;
    SysCtl_A_FlashTLV_Info *flInfo;

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Parsing the TLV and getting the maximum erase pulses */
    SysCtl_A_getTLVInfo(TLV_TAG_FLASHCTL, 0, &tlvLength, (uint32_t**) &flInfo);

    if (tlvLength == 0 || flInfo->maxProgramPulses == 0)
    {
        mTries = MAX_PROGRAM_NO_TLV;
    } else
    {
        mTries = flInfo->maxProgramPulses;
    }

    /* Casting to integers */
    srcAddr = (uint32_t) src;
    destAddr = (uint32_t) dest;

    /* Enabling word programming */
    FlashCtl_A_enableWordProgramming(FLASH_A_IMMEDIATE_WRITE_MODE);

    /* Assume failure */
    res = false;

    /* Taking care of byte accesses */
    while ((destAddr & 0x03) && length > 0)
    {
        if (!_FlashCtl_A_Program8(srcAddr, destAddr, mTries))
        {
            goto FlashProgramCleanUp;
        } else
        {
            srcAddr++;
            destAddr++;
            length--;
        }
    }

    /* Taking care of word accesses */
    while ((destAddr & 0x0F) && (length > 3))
    {
        if (!_FlashCtl_A_Program32(srcAddr, destAddr, mTries))
        {
            goto FlashProgramCleanUp;
        } else
        {
            srcAddr += 4;
            destAddr += 4;
            length -= 4;
        }
    }

    /* Taking care of burst programs */
    while (length > 16)
    {
        burstLength = length > 63 ? 64 : length & 0xFFFFFFF0;

        if (!_FlashCtl_A_ProgramBurst(srcAddr, destAddr, burstLength, mTries))
        {
            goto FlashProgramCleanUp;
        } else
        {
            srcAddr += burstLength;
            destAddr += burstLength;
            length -= burstLength;
        }
    }

    /* Remaining word accesses */
    while (length > 3)
    {
        if (!_FlashCtl_A_Program32(srcAddr, destAddr, mTries))
        {
            goto FlashProgramCleanUp;
        } else
        {
            srcAddr += 4;
            destAddr += 4;
            length -= 4;
        }
    }

    /* Remaining byte accesses */
    while (length > 0)
    {
        if (!_FlashCtl_A_Program8(srcAddr, destAddr, mTries))
        {
            goto FlashProgramCleanUp;
        } else
        {
            srcAddr++;
            destAddr++;
            length--;
        }
    }

    /* If we got this far that means that we succeeded  */
    res = true;

    FlashProgramCleanUp:

    if (intStatus == 0)
        Interrupt_enableMaster();

    FlashCtl_A_disableWordProgramming();
    return res;

}
void FlashCtl_A_setProgramVerification(uint32_t verificationSetting)
{
    if ((verificationSetting & FLASH_A_BURSTPOST))
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_AUTO_PST_OFS) = 1;

    if ((verificationSetting & FLASH_A_BURSTPRE))
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_AUTO_PRE_OFS) = 1;

    if ((verificationSetting & FLASH_A_REGPRE))
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_VER_PRE_OFS) = 1;

    if ((verificationSetting & FLASH_A_REGPOST))
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_VER_PST_OFS) = 1;
}

void FlashCtl_A_clearProgramVerification(uint32_t verificationSetting)
{
    if ((verificationSetting & FLASH_A_BURSTPOST))
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_AUTO_PST_OFS) = 0;

    if ((verificationSetting & FLASH_A_BURSTPRE))
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_AUTO_PRE_OFS) = 0;

    if ((verificationSetting & FLASH_A_REGPRE))
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_VER_PRE_OFS) = 0;

    if ((verificationSetting & FLASH_A_REGPOST))
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_VER_PST_OFS) = 0;

}

void FlashCtl_A_enableWordProgramming(uint32_t mode)
{
    if (mode == FLASH_A_IMMEDIATE_WRITE_MODE)
    {
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_ENABLE_OFS) = 1;
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_MODE_OFS) = 0;

    } else if (mode == FLASH_A_COLLATED_WRITE_MODE)
    {
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_ENABLE_OFS) = 1;
        BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_MODE_OFS) = 1;
    }
}

void FlashCtl_A_disableWordProgramming(void)
{
    BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_ENABLE_OFS) = 0;
}

uint32_t FlashCtl_A_isWordProgrammingEnabled(void)
{
    if (!BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_ENABLE_OFS))
    {
        return 0;
    } else if (BITBAND_PERI(FLCTL_A->PRG_CTLSTAT, FLCTL_A_PRG_CTLSTAT_MODE_OFS))
        return FLASH_A_COLLATED_WRITE_MODE;
    else
        return FLASH_A_IMMEDIATE_WRITE_MODE;
}

void FlashCtl_A_setWaitState(uint32_t flashBank, uint32_t waitState)
{
    if (flashBank == FLASH_A_BANK0)
    {
        FLCTL_A->BANK0_RDCTL = (FLCTL_A->BANK0_RDCTL
                & ~FLCTL_A_BANK0_RDCTL_WAIT_MASK)
                | (waitState << FLCTL_A_BANK0_RDCTL_WAIT_OFS);
    } else if (flashBank == FLASH_A_BANK1)
    {
        FLCTL_A->BANK1_RDCTL = (FLCTL_A->BANK1_RDCTL
                & ~FLCTL_A_BANK1_RDCTL_WAIT_MASK)
                | (waitState << FLCTL_A_BANK1_RDCTL_WAIT_OFS);
    } else
    {
        ASSERT(false);
    }
}

uint32_t FlashCtl_A_getWaitState(uint32_t flashBank)
{
    if (flashBank == FLASH_A_BANK0)
    {
        return (FLCTL_A->BANK0_RDCTL & FLCTL_A_BANK0_RDCTL_WAIT_MASK)
                >> FLCTL_A_BANK0_RDCTL_WAIT_OFS;
    } else if (flashBank == FLASH_A_BANK1)
    {
        return (FLCTL_A->BANK1_RDCTL & FLCTL_A_BANK1_RDCTL_WAIT_MASK)
                >> FLCTL_A_BANK1_RDCTL_WAIT_OFS;
    } else
    {
        ASSERT(false);
        return 0;
    }
}

void FlashCtl_A_enableInterrupt(uint32_t flags)
{
    FLCTL_A->IE |= flags;
}

void FlashCtl_A_disableInterrupt(uint32_t flags)
{
    FLCTL_A->IE &= ~flags;
}

uint32_t FlashCtl_A_getInterruptStatus(void)
{
    return FLCTL_A->IFG;
}

uint32_t FlashCtl_A_getEnabledInterruptStatus(void)
{
    return FlashCtl_A_getInterruptStatus() & FLCTL_A->IE;
}

void FlashCtl_A_clearInterruptFlag(uint32_t flags)
{
    FLCTL_A->CLRIFG |= flags;
}

void FlashCtl_A_registerInterrupt(void (*intHandler)(void))
{
    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    Interrupt_registerInterrupt(INT_FLCTL, intHandler);

    //
    // Enable the system control interrupt.
    //
    Interrupt_enableInterrupt(INT_FLCTL);
}

void FlashCtl_A_unregisterInterrupt(void)
{
    //
    // Disable the interrupt.
    //
    Interrupt_disableInterrupt(INT_FLCTL);

    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(INT_FLCTL);
}

uint8_t __FlashCtl_A_remaskData8Post(uint8_t data, uint32_t addr)
{
    uint32_t readMode, waitState, bankProgram, bankOneStart;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if (addr >= SysCtl_A_getFlashSize())
    {
        bankOneStart = __INFO_FLASH_A_TECH_MIDDLE__;
    } else
    {
        bankOneStart = SysCtl_A_getFlashSize() / 2;
    }

    bankProgram = addr < (bankOneStart) ? FLASH_A_BANK0 : FLASH_A_BANK1;

    /* Saving the current wait states and read mode */
    waitState = FlashCtl_A_getWaitState(bankProgram);
    readMode = FlashCtl_A_getReadMode(bankProgram);

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setWaitState(bankProgram, (2 * waitState) + 1);

    /* Changing to PROGRAM VERIFY mode */
    FlashCtl_A_setReadMode(bankProgram, FLASH_A_PROGRAM_VERIFY_READ_MODE);

    data = ~(~(data) & HWREG8(addr));

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setReadMode(bankProgram, readMode);
    FlashCtl_A_setWaitState(bankProgram, waitState);

    return data;
}

uint8_t __FlashCtl_A_remaskData8Pre(uint8_t data, uint32_t addr)
{
    uint32_t readMode, waitState, bankProgram, bankOneStart;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if (addr >= SysCtl_A_getFlashSize())
    {
        bankOneStart = __INFO_FLASH_A_TECH_MIDDLE__;
    } else
    {
        bankOneStart = SysCtl_A_getFlashSize() / 2;
    }

    bankProgram = addr < (bankOneStart) ? FLASH_A_BANK0 : FLASH_A_BANK1;

    /* Saving the current wait states and read mode */
    waitState = FlashCtl_A_getWaitState(bankProgram);
    readMode = FlashCtl_A_getReadMode(bankProgram);

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setWaitState(bankProgram, (2 * waitState) + 1);

    /* Changing to PROGRAM VERIFY mode */
    FlashCtl_A_setReadMode(bankProgram, FLASH_A_PROGRAM_VERIFY_READ_MODE);

    data |= ~(HWREG8(addr) | data);

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setReadMode(bankProgram, readMode);
    FlashCtl_A_setWaitState(bankProgram, waitState);

    return data;
}

uint32_t __FlashCtl_A_remaskData32Post(uint32_t data, uint32_t addr)
{
    uint32_t bankProgramStart, bankProgramEnd, bank1Start;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if (addr >= SysCtl_A_getFlashSize())
    {
        bank1Start = __INFO_FLASH_A_TECH_MIDDLE__;
    } else
    {
        bank1Start = SysCtl_A_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;
    bankProgramEnd = (addr + 4) < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_A_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_A_getReadMode(bankProgramStart);
    FlashCtl_A_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_A_setReadMode(bankProgramStart, FLASH_A_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_A_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_A_getReadMode(bankProgramEnd);
        FlashCtl_A_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_A_setReadMode(bankProgramEnd,
        FLASH_A_PROGRAM_VERIFY_READ_MODE);
    }

    data = ~(~(data) & HWREG32(addr));

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_A_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_A_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_A_setWaitState(bankProgramEnd, b1WaitState);
    }

    return data;
}

uint32_t __FlashCtl_A_remaskData32Pre(uint32_t data, uint32_t addr)
{
    uint32_t bankProgramStart, bankProgramEnd, bank1Start;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if (addr >= SysCtl_A_getFlashSize())
    {
        bank1Start = __INFO_FLASH_A_TECH_MIDDLE__;
    } else
    {
        bank1Start = SysCtl_A_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;
    bankProgramEnd = (addr + 4) < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_A_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_A_getReadMode(bankProgramStart);
    FlashCtl_A_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_A_setReadMode(bankProgramStart, FLASH_A_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_A_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_A_getReadMode(bankProgramEnd);
        FlashCtl_A_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_A_setReadMode(bankProgramEnd,
        FLASH_A_PROGRAM_VERIFY_READ_MODE);
    }

    data |= ~(HWREG32(addr) | data);

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_A_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_A_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_A_setWaitState(bankProgramEnd, b1WaitState);
    }

    return data;
}

void __FlashCtl_A_remaskBurstDataPre(uint32_t addr, uint32_t size)
{

    uint32_t bankProgramStart, bankProgramEnd, bank1Start, ii;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Waiting for idle status */
    while ((FLCTL_A->PRGBRST_CTLSTAT & FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if (addr >= SysCtl_A_getFlashSize())
    {
        bank1Start = __INFO_FLASH_A_TECH_MIDDLE__;
    } else
    {
        bank1Start = SysCtl_A_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;
    bankProgramEnd = (addr + size) < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_A_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_A_getReadMode(bankProgramStart);
    FlashCtl_A_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_A_setReadMode(bankProgramStart, FLASH_A_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_A_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_A_getReadMode(bankProgramEnd);
        FlashCtl_A_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_A_setReadMode(bankProgramEnd,
        FLASH_A_PROGRAM_VERIFY_READ_MODE);
    }

    /* Going through each BURST program register and masking out for pre
     * verifcation
     */
    size = (size / 4);
    for (ii = 0; ii < size; ii++)
    {
        HWREG32(__getBurstProgramRegs[ii]) |=
                ~(HWREG32(__getBurstProgramRegs[ii]) | HWREG32(addr));
        addr += 4;
    }

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_A_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_A_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_A_setWaitState(bankProgramEnd, b1WaitState);
    }

}
void __FlashCtl_A_remaskBurstDataPost(uint32_t addr, uint32_t size)
{
    uint32_t bankProgramStart, bankProgramEnd, bank1Start, ii;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Waiting for idle status */
    while ((FLCTL_A->PRGBRST_CTLSTAT & FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_A_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL_A->PRGBRST_CTLSTAT,
                FLCTL_A_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if (addr >= SysCtl_A_getFlashSize())
    {
        bank1Start = __INFO_FLASH_A_TECH_MIDDLE__;
    } else
    {
        bank1Start = SysCtl_A_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;
    bankProgramEnd = (addr + size) < bank1Start ? FLASH_A_BANK0 : FLASH_A_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_A_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_A_getReadMode(bankProgramStart);
    FlashCtl_A_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_A_setReadMode(bankProgramStart, FLASH_A_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_A_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_A_getReadMode(bankProgramEnd);
        FlashCtl_A_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_A_setReadMode(bankProgramEnd,
        FLASH_A_PROGRAM_VERIFY_READ_MODE);
    }

    /* Going through each BURST program register and masking out for post
     * verifcation if needed
     */
    size = (size / 4);
    for (ii = 0; ii < size; ii++)
    {
        HWREG32(__getBurstProgramRegs[ii]) = ~(~(HWREG32(
                __getBurstProgramRegs[ii])) & HWREG32(addr));

        addr += 4;
    }

    /* Setting the wait state to account for the mode */
    FlashCtl_A_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_A_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_A_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_A_setWaitState(bankProgramEnd, b1WaitState);
    }
}

#endif /* __MCU_HAS_FLCTL_A__ */

