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
#include <ti/devices/msp432p4xx/driverlib/flash.h>
#include <ti/devices/msp432p4xx/driverlib/sysctl.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/cpu.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

/* Define to ensure that our current MSP432 has the FLCTL module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_FLCTL__

static const uint32_t MAX_ERASE_NO_TLV = 50;
static const uint32_t MAX_PROGRAM_NO_TLV = 5;

static volatile uint32_t* __getBurstProgramRegs[16] =
{ &FLCTL->PRGBRST_DATA0_0, &FLCTL->PRGBRST_DATA0_1,
&FLCTL->PRGBRST_DATA0_2, &FLCTL->PRGBRST_DATA0_3,
&FLCTL->PRGBRST_DATA1_0, &FLCTL->PRGBRST_DATA1_1,
&FLCTL->PRGBRST_DATA1_2, &FLCTL->PRGBRST_DATA1_3,
&FLCTL->PRGBRST_DATA2_0, &FLCTL->PRGBRST_DATA2_1,
&FLCTL->PRGBRST_DATA2_2, &FLCTL->PRGBRST_DATA2_3,
&FLCTL->PRGBRST_DATA3_0, &FLCTL->PRGBRST_DATA3_1,
&FLCTL->PRGBRST_DATA3_2, &FLCTL->PRGBRST_DATA3_3 };

static uint32_t getUserFlashSector(uint32_t addr)
{
    if (addr > 0x1ffff)
    {
        addr = addr - 0x20000;
    }

    switch (addr)
    {
    case 0:
        return FLASH_SECTOR0;
    case 0x1000:
        return FLASH_SECTOR1;
    case 0x2000:
        return FLASH_SECTOR2;
    case 0x3000:
        return FLASH_SECTOR3;
    case 0x4000:
        return FLASH_SECTOR4;
    case 0x5000:
        return FLASH_SECTOR5;
    case 0x6000:
        return FLASH_SECTOR6;
    case 0x7000:
        return FLASH_SECTOR7;
    case 0x8000:
        return FLASH_SECTOR8;
    case 0x9000:
        return FLASH_SECTOR9;
    case 0xA000:
        return FLASH_SECTOR10;
    case 0xB000:
        return FLASH_SECTOR11;
    case 0xC000:
        return FLASH_SECTOR12;
    case 0xD000:
        return FLASH_SECTOR13;
    case 0xE000:
        return FLASH_SECTOR14;
    case 0xF000:
        return FLASH_SECTOR15;
    case 0x10000:
        return FLASH_SECTOR16;
    case 0x11000:
        return FLASH_SECTOR17;
    case 0x12000:
        return FLASH_SECTOR18;
    case 0x13000:
        return FLASH_SECTOR19;
    case 0x14000:
        return FLASH_SECTOR20;
    case 0x15000:
        return FLASH_SECTOR21;
    case 0x16000:
        return FLASH_SECTOR22;
    case 0x17000:
        return FLASH_SECTOR23;
    case 0x18000:
        return FLASH_SECTOR24;
    case 0x19000:
        return FLASH_SECTOR25;
    case 0x1A000:
        return FLASH_SECTOR26;
    case 0x1B000:
        return FLASH_SECTOR27;
    case 0x1C000:
        return FLASH_SECTOR28;
    case 0x1D000:
        return FLASH_SECTOR29;
    case 0x1E000:
        return FLASH_SECTOR30;
    case 0x1F000:
        return FLASH_SECTOR31;
    default:
        ASSERT(false);
        return 0;
    }
}

void FlashCtl_getMemoryInfo(uint32_t addr, uint32_t *bankNum,
                                   uint32_t *sectorNum)
{
    uint32_t bankLimit;

    bankLimit = SysCtl_getFlashSize() / 2;

    if (addr > bankLimit)
    {
        *(bankNum) = FLASH_BANK1;
        addr = (addr - bankLimit);
    } else
    {
        *(bankNum) = FLASH_BANK0;
    }

    *(sectorNum) = (addr) / 4096;
}

static bool _FlashCtl_Program8(uint32_t src, uint32_t dest, uint32_t mTries)
{
    uint32_t ii;
    uint8_t data;

    /* Enabling the correct verification settings  */
    FlashCtl_setProgramVerification(FLASH_REGPRE | FLASH_REGPOST);
    FlashCtl_clearProgramVerification(FLASH_BURSTPOST | FLASH_BURSTPRE);

    data = HWREG8(src);

    for (ii = 0; ii < mTries; ii++)
    {
        /* Clearing flags */
        FLCTL->CLRIFG |= (FLASH_PROGRAM_ERROR | FLASH_POSTVERIFY_FAILED
                | FLASH_PREVERIFY_FAILED | FLASH_WRDPRGM_COMPLETE);

        HWREG8(dest) = data;

        while (!(FlashCtl_getInterruptStatus() & FLASH_WRDPRGM_COMPLETE))
        {
            __no_operation();
        }

        /* Pre-Verify */
        if ((BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_VER_PRE_OFS)
                && BITBAND_PERI(FLCTL->IFG, FLCTL_IFG_AVPRE_OFS)))
        {
            data = __FlashCtl_remaskData8Pre(data, dest);

            if (data != 0xFF)
            {
                FlashCtl_clearProgramVerification(FLASH_REGPRE);
                continue;
            }

        }

        /* Post Verify */
        if ((BITBAND_PERI(FLCTL->IFG, FLCTL_IFG_AVPST_OFS)))
        {
            data = __FlashCtl_remaskData8Post(data, dest);

            /* Seeing if we actually need to do another pulse */
            if (data == 0xFF)
                return true;

            FlashCtl_setProgramVerification(FLASH_REGPRE | FLASH_REGPOST);
            continue;
        }

        /* If we got this far, return true */
        return true;

    }

    return false;

}

static bool _FlashCtl_Program32(uint32_t src, uint32_t dest, uint32_t mTries)
{
    uint32_t ii;
    uint32_t data;

    /* Enabling the correct verification settings  */
    FlashCtl_setProgramVerification(FLASH_REGPRE | FLASH_REGPOST);
    FlashCtl_clearProgramVerification(FLASH_BURSTPOST | FLASH_BURSTPRE);

    data = HWREG32(src);

    for (ii = 0; ii < mTries; ii++)
    {
        /* Clearing flags */
        FLCTL->CLRIFG |= (FLASH_PROGRAM_ERROR | FLASH_POSTVERIFY_FAILED
                | FLASH_PREVERIFY_FAILED | FLASH_WRDPRGM_COMPLETE);

        HWREG32(dest) = data;

        while (!(FlashCtl_getInterruptStatus() & FLASH_WRDPRGM_COMPLETE))
        {
            __no_operation();
        }

        /* Pre-Verify */
        if ((BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_VER_PRE_OFS)
                && BITBAND_PERI(FLCTL->IFG, FLCTL_IFG_AVPRE_OFS)))
        {
            data = __FlashCtl_remaskData32Pre(data, dest);

            if (data != 0xFFFFFFFF)
            {

                FlashCtl_clearProgramVerification(FLASH_REGPRE);
                continue;
            }

        }

        /* Post Verify */
        if ((BITBAND_PERI(FLCTL->IFG, FLCTL_IFG_AVPST_OFS)))
        {
            data = __FlashCtl_remaskData32Post(data, dest);

            /* Seeing if we actually need to do another pulse */
            if (data == 0xFFFFFFFF)
                return true;

            FlashCtl_setProgramVerification(FLASH_REGPRE | FLASH_REGPOST);
            continue;
        }

        /* If we got this far, return true */
        return true;

    }

    return false;

}

static bool _FlashCtl_ProgramBurst(uint32_t src, uint32_t dest, uint32_t length,
        uint32_t mTries)
{
    uint32_t bCalc, otpOffset, ii, jj;
    bool res;

    /* Setting verification */
    FlashCtl_clearProgramVerification(FLASH_REGPRE | FLASH_REGPOST);
    FlashCtl_setProgramVerification(FLASH_BURSTPOST | FLASH_BURSTPRE);

    /* Assume Failure */
    res = false;

    /* Waiting for idle status */
    while ((FLCTL->PRGBRST_CTLSTAT & FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }

    /* Setting/clearing INFO flash flags as appropriate */
    if (dest > SysCtl_getFlashSize())
    {
        FLCTL->PRGBRST_CTLSTAT = (FLCTL->PRGBRST_CTLSTAT
                & ~FLCTL_PRGBRST_CTLSTAT_TYPE_MASK) | FLCTL_PRGBRST_CTLSTAT_TYPE_1;
        otpOffset = __INFO_FLASH_TECH_START__;
    } else
    {
        FLCTL->PRGBRST_CTLSTAT = (FLCTL->PRGBRST_CTLSTAT
                & ~FLCTL_PRGBRST_CTLSTAT_TYPE_MASK) | FLCTL_PRGBRST_CTLSTAT_TYPE_0;
        otpOffset = 0;
    }

    bCalc = 0;
    FLCTL->PRGBRST_STARTADDR = (dest - otpOffset);

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
        FLCTL->CLRIFG |= (FLASH_BRSTPRGM_COMPLETE | FLASH_POSTVERIFY_FAILED
                | FLASH_PREVERIFY_FAILED);

        /* Waiting for idle status */
        while ((FLCTL->PRGBRST_CTLSTAT & FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
                != FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_0)
        {
            BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                    FLCTL_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
        }

        /* Start the burst program */
        FLCTL->PRGBRST_CTLSTAT = (FLCTL->PRGBRST_CTLSTAT
                & ~(FLCTL_PRGBRST_CTLSTAT_LEN_MASK))
                | ((bCalc / 4) << FLASH_BURST_PRG_BIT)
                | FLCTL_PRGBRST_CTLSTAT_START;

        /* Waiting for the burst to complete */
        while ((FLCTL->PRGBRST_CTLSTAT &
        FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
                != FLASH_PRGBRSTCTLSTAT_BURSTSTATUS_COMPLETE)
        {
            __no_operation();
        }

        /* Checking for errors and clearing/masking */

        /* Address Error */
        if (BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_ADDR_ERR_OFS))
        {
            goto BurstCleanUp;
        }

        /* Pre-Verify Error */
        if (BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_AUTO_PRE_OFS) && BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                        FLCTL_PRGBRST_CTLSTAT_PRE_ERR_OFS))
        {
            __FlashCtl_remaskBurstDataPre(dest, bCalc * 4);

            for (jj = 0; jj < bCalc; jj++)
            {
                if (HWREG32(__getBurstProgramRegs[jj])
                        != 0xFFFFFFFF)
                {
                    FlashCtl_clearProgramVerification(FLASH_BURSTPRE);
                    break;
                }
            }

            if (jj != bCalc)
                continue;
        }

        /* Post-Verify Error */
        if (BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_PST_ERR_OFS))
        {
            __FlashCtl_remaskBurstDataPost(dest, bCalc * 4);

            for (jj = 0; jj < bCalc; jj++)
            {
                if ((HWREG32(__getBurstProgramRegs[jj]))
                        != 0xFFFFFFFF)
                {
                    FlashCtl_setProgramVerification(
                            FLASH_BURSTPOST | FLASH_BURSTPRE);
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
    while ((FLCTL->PRGBRST_CTLSTAT & FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }
    return res;
}

void FlashCtl_enableReadBuffering(uint_fast8_t memoryBank,
        uint_fast8_t accessMethod)
{
    if (memoryBank == FLASH_BANK0 && accessMethod == FLASH_DATA_READ)
        BITBAND_PERI(FLCTL->BANK0_RDCTL, FLCTL_BANK0_RDCTL_BUFD_OFS) = 1;
    else if (memoryBank == FLASH_BANK1 && accessMethod == FLASH_DATA_READ)
        BITBAND_PERI(FLCTL->BANK1_RDCTL, FLCTL_BANK1_RDCTL_BUFD_OFS) = 1;
    else if (memoryBank == FLASH_BANK0
            && accessMethod == FLASH_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL->BANK0_RDCTL, FLCTL_BANK0_RDCTL_BUFI_OFS) = 1;
    else if (memoryBank == FLASH_BANK1
            && accessMethod == FLASH_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL->BANK1_RDCTL, FLCTL_BANK1_RDCTL_BUFI_OFS) = 1;
    else
        ASSERT(false);
}

void FlashCtl_disableReadBuffering(uint_fast8_t memoryBank,
        uint_fast8_t accessMethod)
{
    if (memoryBank == FLASH_BANK0 && accessMethod == FLASH_DATA_READ)
        BITBAND_PERI(FLCTL->BANK0_RDCTL, FLCTL_BANK0_RDCTL_BUFD_OFS) = 0;
    else if (memoryBank == FLASH_BANK1 && accessMethod == FLASH_DATA_READ)
        BITBAND_PERI(FLCTL->BANK1_RDCTL, FLCTL_BANK1_RDCTL_BUFD_OFS) = 0;
    else if (memoryBank == FLASH_BANK0
            && accessMethod == FLASH_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL->BANK0_RDCTL, FLCTL_BANK0_RDCTL_BUFI_OFS) = 0;
    else if (memoryBank == FLASH_BANK1
            && accessMethod == FLASH_INSTRUCTION_FETCH)
        BITBAND_PERI(FLCTL->BANK1_RDCTL, FLCTL_BANK1_RDCTL_BUFI_OFS) = 0;
    else
        ASSERT(false);
}

bool FlashCtl_unprotectSector(uint_fast8_t memorySpace, uint32_t sectorMask)
{
    switch (memorySpace)
    {
    case FLASH_MAIN_MEMORY_SPACE_BANK0:
        FLCTL->BANK0_MAIN_WEPROT &= ~sectorMask;
        break;
    case FLASH_MAIN_MEMORY_SPACE_BANK1:
        FLCTL->BANK1_MAIN_WEPROT &= ~sectorMask;
        break;
    case FLASH_INFO_MEMORY_SPACE_BANK0:
        ASSERT(sectorMask <= 0x04);
        FLCTL->BANK0_INFO_WEPROT &= ~sectorMask;
        break;
    case FLASH_INFO_MEMORY_SPACE_BANK1:
        ASSERT(sectorMask <= 0x04);
        FLCTL->BANK1_INFO_WEPROT &= ~sectorMask;
        break;

    default:
        ASSERT(false);

    }

    return !FlashCtl_isSectorProtected(memorySpace, sectorMask);
}

bool FlashCtl_protectSector(uint_fast8_t memorySpace, uint32_t sectorMask)
{
    switch (memorySpace)
    {
    case FLASH_MAIN_MEMORY_SPACE_BANK0:
        FLCTL->BANK0_MAIN_WEPROT |= sectorMask;
        break;
    case FLASH_MAIN_MEMORY_SPACE_BANK1:
        FLCTL->BANK1_MAIN_WEPROT |= sectorMask;
        break;
    case FLASH_INFO_MEMORY_SPACE_BANK0:
        ASSERT(sectorMask <= 0x04);
        FLCTL->BANK0_INFO_WEPROT |= sectorMask;
        break;
    case FLASH_INFO_MEMORY_SPACE_BANK1:
        ASSERT(sectorMask <= 0x04);
        FLCTL->BANK1_INFO_WEPROT |= sectorMask;
        break;

    default:
        ASSERT(false);

    }

    return FlashCtl_isSectorProtected(memorySpace, sectorMask);
}

bool FlashCtl_isSectorProtected(uint_fast8_t memorySpace, uint32_t sector)
{
    switch (memorySpace)
    {
    case FLASH_MAIN_MEMORY_SPACE_BANK0:
        return FLCTL->BANK0_MAIN_WEPROT & sector;
    case FLASH_MAIN_MEMORY_SPACE_BANK1:
        return FLCTL->BANK1_MAIN_WEPROT & sector;
    case FLASH_INFO_MEMORY_SPACE_BANK0:
        ASSERT(sector <= 0x04);
        return FLCTL->BANK0_INFO_WEPROT & sector;
    case FLASH_INFO_MEMORY_SPACE_BANK1:
        ASSERT(sector <= 0x04);
        return FLCTL->BANK1_INFO_WEPROT & sector;
    default:
        return false;
    }
}

bool FlashCtl_verifyMemory(void* verifyAddr, uint32_t length,
        uint_fast8_t pattern)
{
    uint32_t memoryPattern, addr, otpOffset;
    uint32_t b0WaitState, b1WaitState, intStatus;
    uint32_t bankOneStart, startBank, endBank;
    uint_fast8_t b0readMode, b1readMode;
    uint_fast8_t memoryType;
    bool res;

    ASSERT(pattern == FLASH_0_PATTERN || pattern == FLASH_1_PATTERN);

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Casting and determining the memory that we need to use */
    addr = (uint32_t) verifyAddr;
    memoryType =
            (addr > SysCtl_getFlashSize()) ? FLASH_INFO_SPACE : FLASH_MAIN_SPACE;

    /* Assuming Failure */
    res = false;

    /* Finding out which bank we are in */
    if(addr >  SysCtl_getFlashSize())
    {
        bankOneStart = __INFO_FLASH_TECH_MIDDLE__;
    }
    else
    {
        bankOneStart = SysCtl_getFlashSize() / 2;
    }
    startBank = addr < (bankOneStart) ? FLASH_BANK0 : FLASH_BANK1;
    endBank = (addr + length) < (bankOneStart) ? FLASH_BANK0 : FLASH_BANK1;

    /* Saving context and changing read modes */
    b0WaitState = FlashCtl_getWaitState(startBank);
    b0readMode = FlashCtl_getReadMode(startBank);

    /* Setting the wait state to account for the mode */
    FlashCtl_setWaitState(startBank, (2 * b0WaitState) + 1);

    if(startBank != endBank)
    {
        b1WaitState = FlashCtl_getWaitState(endBank);
        b1readMode = FlashCtl_getReadMode(endBank);
        FlashCtl_setWaitState(endBank, (2 * b1WaitState) + 1);
    }

    /* Changing to the relevant VERIFY mode */
    if (pattern == FLASH_1_PATTERN)
    {
        FlashCtl_setReadMode(startBank, FLASH_ERASE_VERIFY_READ_MODE);

        if(startBank != endBank)
        {
            FlashCtl_setReadMode(endBank, FLASH_ERASE_VERIFY_READ_MODE);
        }

        memoryPattern = 0xFFFFFFFF;
    } else
    {
        FlashCtl_setReadMode(startBank, FLASH_PROGRAM_VERIFY_READ_MODE);

        if(startBank != endBank)
        {
            FlashCtl_setReadMode(endBank, FLASH_PROGRAM_VERIFY_READ_MODE);
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
        if (addr > SysCtl_getFlashSize())
        {
            FLCTL->RDBRST_CTLSTAT = (FLCTL->RDBRST_CTLSTAT
                    & ~FLCTL_RDBRST_CTLSTAT_MEM_TYPE_MASK)
                    | FLCTL_RDBRST_CTLSTAT_MEM_TYPE_1;
            otpOffset = __INFO_FLASH_TECH_START__;
        } else
        {
            FLCTL->RDBRST_CTLSTAT = (FLCTL->RDBRST_CTLSTAT
                    & ~FLCTL_RDBRST_CTLSTAT_MEM_TYPE_MASK)
                    | FLCTL_RDBRST_CTLSTAT_MEM_TYPE_0;
            otpOffset = 0;
        }

        /* Clearing any lingering fault flags  and preparing burst verify*/
        BITBAND_PERI(FLCTL->RDBRST_CTLSTAT,
                FLCTL_RDBRST_CTLSTAT_CLR_STAT_OFS) = 1;
        FLCTL->RDBRST_FAILCNT = 0;
        FLCTL->RDBRST_STARTADDR = addr - otpOffset;
        FLCTL->RDBRST_LEN = (length & 0xFFFFFFF0);
        addr += FLCTL->RDBRST_LEN;
        length = length & 0xF;

        /* Starting Burst Verify */
        FLCTL->RDBRST_CTLSTAT = (FLCTL_RDBRST_CTLSTAT_STOP_FAIL | pattern
                | memoryType | FLCTL_RDBRST_CTLSTAT_START);

        /* While the burst read hasn't finished */
        while ((FLCTL->RDBRST_CTLSTAT & FLCTL_RDBRST_CTLSTAT_BRST_STAT_MASK)
                != FLCTL_RDBRST_CTLSTAT_BRST_STAT_3)
        {
            __no_operation();
        }

        /* Checking  for a verification/access error/failure */
        if (BITBAND_PERI(FLCTL->RDBRST_CTLSTAT,
                FLCTL_RDBRST_CTLSTAT_CMP_ERR_OFS)
                || BITBAND_PERI(FLCTL->RDBRST_CTLSTAT,
                        FLCTL_RDBRST_CTLSTAT_ADDR_ERR_OFS)
                || FLCTL->RDBRST_FAILCNT)
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
    BITBAND_PERI(FLCTL->RDBRST_CTLSTAT,
            FLCTL_RDBRST_CTLSTAT_CLR_STAT_OFS) = 1;

    FlashCtl_setReadMode(startBank, b0readMode);
    FlashCtl_setWaitState(startBank, b0WaitState);

    if(startBank != endBank)
    {
        FlashCtl_setReadMode(endBank, b1readMode);
        FlashCtl_setWaitState(endBank, b1WaitState);
    }

    if(intStatus == 0)
        Interrupt_enableMaster();

    return res;
}

bool FlashCtl_setReadMode(uint32_t flashBank, uint32_t readMode)
{

    if (FLCTL->POWER_STAT & FLCTL_POWER_STAT_RD_2T)
        return false;

    if (flashBank == FLASH_BANK0)
    {
        FLCTL->BANK0_RDCTL = (FLCTL->BANK0_RDCTL
                & ~FLCTL_BANK0_RDCTL_RD_MODE_MASK) | readMode;
        while ((FLCTL->BANK0_RDCTL & FLCTL_BANK0_RDCTL_RD_MODE_STATUS_MASK)
                != (readMode<<16))
            ;
    } else if (flashBank == FLASH_BANK1)
    {
        FLCTL->BANK1_RDCTL = (FLCTL->BANK1_RDCTL
                & ~FLCTL_BANK1_RDCTL_RD_MODE_MASK) | readMode;
        while ((FLCTL->BANK1_RDCTL & FLCTL_BANK1_RDCTL_RD_MODE_STATUS_MASK)
                != (readMode<<16))
            ;
    } else
    {
        ASSERT(false);
        return false;
    }

    return true;
}

uint32_t FlashCtl_getReadMode(uint32_t flashBank)
{
    if (flashBank == FLASH_BANK0)
    {
        return (FLCTL->BANK0_RDCTL & FLCTL_BANK0_RDCTL_RD_MODE_STATUS_MASK) >> 16;
    } else if (flashBank == FLASH_BANK1)
    {
        return (FLCTL->BANK1_RDCTL & FLCTL_BANK1_RDCTL_RD_MODE_STATUS_MASK) >> 16;
    } else
    {
        ASSERT(false);
        return 0;
    }
}

void FlashCtl_initiateMassErase(void)
{
    /* Clearing old mass erase flags */
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS) = 1;

    /* Performing the mass erase */
    FLCTL->ERASE_CTLSTAT |= (FLCTL_ERASE_CTLSTAT_MODE
            | FLCTL_ERASE_CTLSTAT_START);
}

bool FlashCtl_performMassErase(void)
{
    uint32_t userFlash, ii, sector, intStatus;
    bool res;

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Assume Failure */
    res = false;

    /* Clearing old mass erase flags */
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS) = 1;

    /* Performing the mass erase */
    FLCTL->ERASE_CTLSTAT |= (FLCTL_ERASE_CTLSTAT_MODE
            | FLCTL_ERASE_CTLSTAT_START);

    while ((FLCTL->ERASE_CTLSTAT & FLCTL_ERASE_CTLSTAT_STATUS_MASK)
            == FLCTL_ERASE_CTLSTAT_STATUS_1
            || (FLCTL->ERASE_CTLSTAT & FLCTL_ERASE_CTLSTAT_STATUS_MASK)
                    == FLCTL_ERASE_CTLSTAT_STATUS_2)
    {
        __no_operation();
    }

    /* Return false if an address error */
    if (BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_ADDR_ERR_OFS))
        goto MassEraseCleanup;

    /* Changing to erase verify */
    userFlash = SysCtl_getFlashSize() / 2;

    for (ii = 0; ii < userFlash; ii += 4096)
    {
        sector = getUserFlashSector(ii);

        if (!((FLCTL->BANK0_MAIN_WEPROT) & sector))
        {
            if (!FlashCtl_verifyMemory((void*) ii, 4096, FLASH_1_PATTERN))
            {
                if (!FlashCtl_eraseSector(ii))
                    goto MassEraseCleanup;
            }
        }

        if (!(FLCTL->BANK1_MAIN_WEPROT & sector))
        {
            if (!FlashCtl_verifyMemory((void*) (ii + userFlash), 4096,
            FLASH_1_PATTERN))
            {
                if (!FlashCtl_eraseSector(ii + userFlash))
                    goto MassEraseCleanup;
            }
        }

        if (sector < FLCTL_BANK0_MAIN_WEPROT_PROT2)
        {
            if (!(FLCTL->BANK0_INFO_WEPROT & sector))
            {
                if (!FlashCtl_verifyMemory(
                        (void*) (ii + __INFO_FLASH_TECH_START__), 4096,
                        FLASH_1_PATTERN))
                {
                    if (!FlashCtl_eraseSector(ii + __INFO_FLASH_TECH_START__))
                        goto MassEraseCleanup;
                }
            }

            if (!(FLCTL->BANK1_INFO_WEPROT & sector))
            {
                if (!FlashCtl_verifyMemory((void*) (ii + (0x202000)), 4096,
                FLASH_1_PATTERN))
                {
                    if (!FlashCtl_eraseSector(ii + (0x202000)))
                        goto MassEraseCleanup;
                }
            }

        }
    }

    /* If we got this far, the mass erase happened */
    res = true;

    MassEraseCleanup:
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS) = 1;

    if(intStatus == 0)
        Interrupt_enableMaster();

    return res;
}

bool FlashCtl_eraseSector(uint32_t addr)
{
    uint_fast8_t memoryType, ii;
    uint32_t otpOffset = 0;
    uint32_t intStatus;
    uint_fast8_t mTries, tlvLength;
    SysCtl_FlashTLV_Info *flInfo;
    bool res;

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Assuming Failure */
    res = false;

    memoryType =
            addr > SysCtl_getFlashSize() ? FLASH_INFO_SPACE : FLASH_MAIN_SPACE;

    /* Parsing the TLV and getting the maximum erase pulses */
    SysCtl_getTLVInfo(TLV_TAG_FLASHCTL, 0, &tlvLength, (uint32_t**) &flInfo);

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
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS) = 1;

    if (memoryType == FLASH_INFO_SPACE)
    {
        otpOffset = __INFO_FLASH_TECH_START__;
        FLCTL->ERASE_CTLSTAT = (FLCTL->ERASE_CTLSTAT
                & ~(FLCTL_ERASE_CTLSTAT_TYPE_MASK)) | FLCTL_ERASE_CTLSTAT_TYPE_1;

    } else
    {
        otpOffset = 0;
        FLCTL->ERASE_CTLSTAT = (FLCTL->ERASE_CTLSTAT
                & ~(FLCTL_ERASE_CTLSTAT_TYPE_MASK)) | FLCTL_ERASE_CTLSTAT_TYPE_0;
    }

    /* Clearing old flags  and setting up the erase */
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_MODE_OFS) = 0;
    FLCTL->ERASE_SECTADDR = addr - otpOffset;

    for (ii = 0; ii < mTries; ii++)
    {
        /* Clearing the status */
        BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS) =
                1;

        /* Starting the erase */
        BITBAND_PERI(FLCTL->ERASE_CTLSTAT,
                FLCTL_ERASE_CTLSTAT_START_OFS) = 1;

        while ((FLCTL->ERASE_CTLSTAT & FLCTL_ERASE_CTLSTAT_STATUS_MASK)
                == FLCTL_ERASE_CTLSTAT_STATUS_1
                || (FLCTL->ERASE_CTLSTAT & FLCTL_ERASE_CTLSTAT_STATUS_MASK)
                        == FLCTL_ERASE_CTLSTAT_STATUS_2)
        {
            __no_operation();
        }

        /* Return false if an address error */
        if (BITBAND_PERI(FLCTL->ERASE_CTLSTAT,
                FLCTL_ERASE_CTLSTAT_ADDR_ERR_OFS))
        {
            goto SectorEraseCleanup;
        }
        /* Erase verifying */
        if (FlashCtl_verifyMemory((void*) addr, 4096, FLASH_1_PATTERN))
        {
            res = true;
            goto SectorEraseCleanup;
        }

    }

SectorEraseCleanup:

    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS) = 1;

    if(intStatus == 0)
        Interrupt_enableMaster();

    return res;
}

void FlashCtl_initiateSectorErase(uint32_t addr)
{
    uint_fast8_t memoryType;
    uint32_t otpOffset = 0;

    memoryType =
            addr > SysCtl_getFlashSize() ? FLASH_INFO_SPACE : FLASH_MAIN_SPACE;

    /* We can only erase on 4KB boundaries */
    while (addr & 0xFFF)
    {
        addr--;
    }

    /* Clearing the status */
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_CLR_STAT_OFS) = 1;

    if (memoryType == FLASH_INFO_SPACE)
    {
        otpOffset = __INFO_FLASH_TECH_START__;
        FLCTL->ERASE_CTLSTAT = (FLCTL->ERASE_CTLSTAT
                & ~(FLCTL_ERASE_CTLSTAT_TYPE_MASK)) | FLCTL_ERASE_CTLSTAT_TYPE_1;

    } else
    {
        otpOffset = 0;
        FLCTL->ERASE_CTLSTAT = (FLCTL->ERASE_CTLSTAT
                & ~(FLCTL_ERASE_CTLSTAT_TYPE_MASK)) | FLCTL_ERASE_CTLSTAT_TYPE_0;
    }

    /* Clearing old flags  and setting up the erase */
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT, FLCTL_ERASE_CTLSTAT_MODE_OFS) = 0;
    FLCTL->ERASE_SECTADDR = addr - otpOffset;

    /* Starting the erase */
    BITBAND_PERI(FLCTL->ERASE_CTLSTAT,
            FLCTL_ERASE_CTLSTAT_START_OFS) = 1;

}

bool FlashCtl_programMemory(void* src, void* dest, uint32_t length)
{
    uint32_t destAddr, srcAddr, burstLength, intStatus;
    bool res;
    uint_fast8_t mTries, tlvLength;
    SysCtl_FlashTLV_Info *flInfo;

    /* Saving interrupt context and disabling interrupts for program
     * operation
     */
    intStatus = CPU_primask();
    Interrupt_disableMaster();

    /* Parsing the TLV and getting the maximum erase pulses */
    SysCtl_getTLVInfo(TLV_TAG_FLASHCTL, 0, &tlvLength, (uint32_t**) &flInfo);

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
    FlashCtl_enableWordProgramming(FLASH_IMMEDIATE_WRITE_MODE);

    /* Assume failure */
    res = false;

    /* Taking care of byte accesses */
    while ((destAddr & 0x03) && length > 0)
    {
        if (!_FlashCtl_Program8(srcAddr, destAddr, mTries))
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
        if (!_FlashCtl_Program32(srcAddr, destAddr, mTries))
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

        if (!_FlashCtl_ProgramBurst(srcAddr, destAddr, burstLength, mTries))
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
        if (!_FlashCtl_Program32(srcAddr, destAddr, mTries))
        {
            goto FlashProgramCleanUp;
        } else
        {
            srcAddr+=4;
            destAddr+=4;
            length-=4;
        }
    }

    /* Remaining byte accesses */
    while (length > 0)
    {
        if (!_FlashCtl_Program8(srcAddr, destAddr, mTries))
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

    if(intStatus == 0)
        Interrupt_enableMaster();

    FlashCtl_disableWordProgramming();
    return res;

}
void FlashCtl_setProgramVerification(uint32_t verificationSetting)
{
    if ((verificationSetting & FLASH_BURSTPOST))
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_AUTO_PST_OFS) = 1;

    if ((verificationSetting & FLASH_BURSTPRE))
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_AUTO_PRE_OFS) = 1;

    if ((verificationSetting & FLASH_REGPRE))
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_VER_PRE_OFS) = 1;

    if ((verificationSetting & FLASH_REGPOST))
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_VER_PST_OFS) = 1;
}

void FlashCtl_clearProgramVerification(uint32_t verificationSetting)
{
    if ((verificationSetting & FLASH_BURSTPOST))
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_AUTO_PST_OFS) = 0;

    if ((verificationSetting & FLASH_BURSTPRE))
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_AUTO_PRE_OFS) = 0;

    if ((verificationSetting & FLASH_REGPRE))
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_VER_PRE_OFS) = 0;

    if ((verificationSetting & FLASH_REGPOST))
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_VER_PST_OFS) = 0;

}

void FlashCtl_enableWordProgramming(uint32_t mode)
{
    if (mode == FLASH_IMMEDIATE_WRITE_MODE)
    {
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_ENABLE_OFS) = 1;
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_MODE_OFS) = 0;

    } else if (mode == FLASH_COLLATED_WRITE_MODE)
    {
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_ENABLE_OFS) = 1;
        BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_MODE_OFS) = 1;
    }
}

void FlashCtl_disableWordProgramming(void)
{
    BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_ENABLE_OFS) = 0;
}

uint32_t FlashCtl_isWordProgrammingEnabled(void)
{
    if (!BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_ENABLE_OFS))
    {
        return 0;
    } else if (BITBAND_PERI(FLCTL->PRG_CTLSTAT, FLCTL_PRG_CTLSTAT_MODE_OFS))
        return FLASH_COLLATED_WRITE_MODE;
    else
        return FLASH_IMMEDIATE_WRITE_MODE;
}

void FlashCtl_setWaitState(uint32_t flashBank, uint32_t waitState)
{
    if (flashBank == FLASH_BANK0)
    {
        FLCTL->BANK0_RDCTL = (FLCTL->BANK0_RDCTL
                & ~FLCTL_BANK0_RDCTL_WAIT_MASK) | (waitState << FLCTL_BANK0_RDCTL_WAIT_OFS);
    } else if (flashBank == FLASH_BANK1)
    {
        FLCTL->BANK1_RDCTL = (FLCTL->BANK1_RDCTL
                & ~FLCTL_BANK1_RDCTL_WAIT_MASK) | (waitState << FLCTL_BANK1_RDCTL_WAIT_OFS);
    } else
    {
        ASSERT(false);
    }
}

uint32_t FlashCtl_getWaitState(uint32_t flashBank)
{
    if (flashBank == FLASH_BANK0)
    {
        return (FLCTL->BANK0_RDCTL & FLCTL_BANK0_RDCTL_WAIT_MASK) >> FLCTL_BANK0_RDCTL_WAIT_OFS;
    } else if (flashBank == FLASH_BANK1)
    {
        return (FLCTL->BANK1_RDCTL & FLCTL_BANK1_RDCTL_WAIT_MASK) >> FLCTL_BANK1_RDCTL_WAIT_OFS;
    } else
    {
        ASSERT(false);
        return 0;
    }
}

void FlashCtl_enableInterrupt(uint32_t flags)
{
    FLCTL->IE |= flags;
}

void FlashCtl_disableInterrupt(uint32_t flags)
{
    FLCTL->IE &= ~flags;
}

uint32_t FlashCtl_getInterruptStatus(void)
{
    return FLCTL->IFG;
}

uint32_t FlashCtl_getEnabledInterruptStatus(void)
{
    return FlashCtl_getInterruptStatus() & FLCTL->IE;
}

void FlashCtl_clearInterruptFlag(uint32_t flags)
{
    FLCTL->CLRIFG |= flags;
}

void FlashCtl_registerInterrupt(void (*intHandler)(void))
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

void FlashCtl_unregisterInterrupt(void)
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

uint8_t __FlashCtl_remaskData8Post(uint8_t data, uint32_t addr)
{
    uint32_t readMode, waitState, bankProgram, bankOneStart;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if(addr >  SysCtl_getFlashSize())
    {
        bankOneStart = __INFO_FLASH_TECH_MIDDLE__;
    }
    else
    {
        bankOneStart = SysCtl_getFlashSize() / 2;
    }

    bankProgram =
            addr < (bankOneStart) ? FLASH_BANK0 : FLASH_BANK1;

    /* Saving the current wait states and read mode */
    waitState = FlashCtl_getWaitState(bankProgram);
    readMode = FlashCtl_getReadMode(bankProgram);

    /* Setting the wait state to account for the mode */
    FlashCtl_setWaitState(bankProgram, (2 * waitState) + 1);

    /* Changing to PROGRAM VERIFY mode */
    FlashCtl_setReadMode(bankProgram, FLASH_PROGRAM_VERIFY_READ_MODE);

    data = ~(~(data) & HWREG8(addr));

    /* Setting the wait state to account for the mode */
    FlashCtl_setReadMode(bankProgram, readMode);
    FlashCtl_setWaitState(bankProgram, waitState);

    return data;
}

uint8_t __FlashCtl_remaskData8Pre(uint8_t data, uint32_t addr)
{
    uint32_t readMode, waitState, bankProgram, bankOneStart;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if(addr >  SysCtl_getFlashSize())
    {
        bankOneStart = __INFO_FLASH_TECH_MIDDLE__;
    }
    else
    {
        bankOneStart = SysCtl_getFlashSize() / 2;
    }

    bankProgram =
            addr < (bankOneStart) ? FLASH_BANK0 : FLASH_BANK1;

    /* Saving the current wait states and read mode */
    waitState = FlashCtl_getWaitState(bankProgram);
    readMode = FlashCtl_getReadMode(bankProgram);

    /* Setting the wait state to account for the mode */
    FlashCtl_setWaitState(bankProgram, (2 * waitState) + 1);

    /* Changing to PROGRAM VERIFY mode */
    FlashCtl_setReadMode(bankProgram, FLASH_PROGRAM_VERIFY_READ_MODE);

    data |= ~(HWREG8(addr) | data);

    /* Setting the wait state to account for the mode */
    FlashCtl_setReadMode(bankProgram, readMode);
    FlashCtl_setWaitState(bankProgram, waitState);

    return data;
}

uint32_t __FlashCtl_remaskData32Post(uint32_t data, uint32_t addr)
{
    uint32_t bankProgramStart, bankProgramEnd, bank1Start;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if(addr >  SysCtl_getFlashSize())
    {
        bank1Start = __INFO_FLASH_TECH_MIDDLE__;
    }
    else
    {
        bank1Start = SysCtl_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_BANK0 : FLASH_BANK1;
    bankProgramEnd = (addr + 4) < bank1Start ? FLASH_BANK0 : FLASH_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_getReadMode(bankProgramStart);
    FlashCtl_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_setReadMode(bankProgramStart, FLASH_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_getReadMode(bankProgramEnd);
        FlashCtl_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_setReadMode(bankProgramEnd, FLASH_PROGRAM_VERIFY_READ_MODE);
    }

    data = ~(~(data) & HWREG32(addr));

    /* Setting the wait state to account for the mode */
    FlashCtl_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_setWaitState(bankProgramEnd, b1WaitState);
    }

    return data;
}

uint32_t __FlashCtl_remaskData32Pre(uint32_t data, uint32_t addr)
{
    uint32_t bankProgramStart, bankProgramEnd, bank1Start;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if(addr >  SysCtl_getFlashSize())
    {
        bank1Start = __INFO_FLASH_TECH_MIDDLE__;
    }
    else
    {
        bank1Start = SysCtl_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_BANK0 : FLASH_BANK1;
    bankProgramEnd = (addr + 4) < bank1Start ? FLASH_BANK0 : FLASH_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_getReadMode(bankProgramStart);
    FlashCtl_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_setReadMode(bankProgramStart, FLASH_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_getReadMode(bankProgramEnd);
        FlashCtl_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_setReadMode(bankProgramEnd, FLASH_PROGRAM_VERIFY_READ_MODE);
    }

    data |= ~(HWREG32(addr) | data);

    /* Setting the wait state to account for the mode */
    FlashCtl_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_setWaitState(bankProgramEnd, b1WaitState);
    }

    return data;
}

void __FlashCtl_remaskBurstDataPre(uint32_t addr, uint32_t size)
{

    uint32_t bankProgramStart, bankProgramEnd, bank1Start, ii;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Waiting for idle status */
    while ((FLCTL->PRGBRST_CTLSTAT & FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if(addr >  SysCtl_getFlashSize())
    {
        bank1Start = __INFO_FLASH_TECH_MIDDLE__;
    }
    else
    {
        bank1Start = SysCtl_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_BANK0 : FLASH_BANK1;
    bankProgramEnd = (addr + size) < bank1Start ? FLASH_BANK0 : FLASH_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_getReadMode(bankProgramStart);
    FlashCtl_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_setReadMode(bankProgramStart, FLASH_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_getReadMode(bankProgramEnd);
        FlashCtl_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_setReadMode(bankProgramEnd, FLASH_PROGRAM_VERIFY_READ_MODE);
    }

    /* Going through each BURST program register and masking out for pre
     * verifcation
     */
    size = (size / 4);
    for (ii = 0; ii < size; ii++)
    {
        HWREG32(__getBurstProgramRegs[ii]) |=
                ~(HWREG32(__getBurstProgramRegs[ii])
                        | HWREG32(addr));
        addr += 4;
    }

    /* Setting the wait state to account for the mode */
    FlashCtl_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_setWaitState(bankProgramEnd, b1WaitState);
    }

}
void __FlashCtl_remaskBurstDataPost(uint32_t addr, uint32_t size)
{
    uint32_t bankProgramStart, bankProgramEnd, bank1Start, ii;
    uint32_t b0WaitState, b0ReadMode, b1WaitState, b1ReadMode;

    /* Waiting for idle status */
    while ((FLCTL->PRGBRST_CTLSTAT & FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_MASK)
            != FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_0)
    {
        BITBAND_PERI(FLCTL->PRGBRST_CTLSTAT,
                FLCTL_PRGBRST_CTLSTAT_CLR_STAT_OFS) = 1;
    }

    /* Changing the waitstate and read mode of whichever bank we are in */
    /* Finding out which bank we are in */
    if(addr >  SysCtl_getFlashSize())
    {
        bank1Start = __INFO_FLASH_TECH_MIDDLE__;
    }
    else
    {
        bank1Start = SysCtl_getFlashSize() / 2;
    }

    bankProgramStart = addr < bank1Start ? FLASH_BANK0 : FLASH_BANK1;
    bankProgramEnd = (addr + size) < bank1Start ? FLASH_BANK0 : FLASH_BANK1;

    /* Saving the current wait states and read mode */
    b0WaitState = FlashCtl_getWaitState(bankProgramStart);
    b0ReadMode = FlashCtl_getReadMode(bankProgramStart);
    FlashCtl_setWaitState(bankProgramStart, (2 * b0WaitState) + 1);
    FlashCtl_setReadMode(bankProgramStart, FLASH_PROGRAM_VERIFY_READ_MODE);

    if (bankProgramStart != bankProgramEnd)
    {
        b1WaitState = FlashCtl_getWaitState(bankProgramEnd);
        b1ReadMode = FlashCtl_getReadMode(bankProgramEnd);
        FlashCtl_setWaitState(bankProgramEnd, (2 * b1WaitState) + 1);
        FlashCtl_setReadMode(bankProgramEnd, FLASH_PROGRAM_VERIFY_READ_MODE);
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
    FlashCtl_setReadMode(bankProgramStart, b0ReadMode);
    FlashCtl_setWaitState(bankProgramStart, b0WaitState);

    if (bankProgramStart != bankProgramEnd)
    {
        FlashCtl_setReadMode(bankProgramEnd, b1ReadMode);
        FlashCtl_setWaitState(bankProgramEnd, b1WaitState);
    }
}

#endif /* __MCU_HAS_FLCTL__ */
