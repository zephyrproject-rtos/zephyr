/******************************************************************************
*  Filename:       flash.c
*  Revised:        2017-10-30 13:37:49 +0100 (Mon, 30 Oct 2017)
*  Revision:       50105
*
*  Description:    Driver for on chip Flash.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#include "../inc/hw_types.h"
#include "../inc/hw_ccfg.h"
#include "flash.h"
#include "rom.h"
#include "chipinfo.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  FlashPowerModeSet
    #define FlashPowerModeSet               NOROM_FlashPowerModeSet
    #undef  FlashPowerModeGet
    #define FlashPowerModeGet               NOROM_FlashPowerModeGet
    #undef  FlashProtectionSet
    #define FlashProtectionSet              NOROM_FlashProtectionSet
    #undef  FlashProtectionGet
    #define FlashProtectionGet              NOROM_FlashProtectionGet
    #undef  FlashProtectionSave
    #define FlashProtectionSave             NOROM_FlashProtectionSave
    #undef  FlashSectorErase
    #define FlashSectorErase                NOROM_FlashSectorErase
    #undef  FlashProgram
    #define FlashProgram                    NOROM_FlashProgram
    #undef  FlashEfuseReadRow
    #define FlashEfuseReadRow               NOROM_FlashEfuseReadRow
    #undef  FlashDisableSectorsForWrite
    #define FlashDisableSectorsForWrite     NOROM_FlashDisableSectorsForWrite
#endif


//*****************************************************************************
//
// Defines for accesses to the security control in the customer configuration
// area in flash top sector.
//
//*****************************************************************************
#define CCFG_OFFSET_SECURITY   CCFG_O_BL_CONFIG
#define CCFG_OFFSET_SECT_PROT  CCFG_O_CCFG_PROT_31_0
#define CCFG_SIZE_SECURITY     0x00000014
#define CCFG_SIZE_SECT_PROT    0x00000004

//*****************************************************************************
//
// Default values for security control in customer configuration area in flash
// top sector.
//
//*****************************************************************************
const uint8_t g_pui8CcfgDefaultSec[] = {0xFF, 0xFF, 0xFF, 0xC5,
                                        0xFF, 0xFF, 0xFF, 0xFF,
                                        0xC5, 0xFF, 0xFF, 0xFF,
                                        0xC5, 0xC5, 0xC5, 0xFF,
                                        0xC5, 0xC5, 0xC5, 0xFF
                                       };

typedef uint32_t (* FlashPrgPointer_t) (uint8_t *, uint32_t, uint32_t);

typedef uint32_t (* FlashSectorErasePointer_t) (uint32_t);

//*****************************************************************************
//
// Function prototypes for static functions
//
//*****************************************************************************
static void SetReadMode(void);

//*****************************************************************************
//
// Set power mode
//
//*****************************************************************************
void
FlashPowerModeSet(uint32_t ui32PowerMode, uint32_t ui32BankGracePeriod,
                  uint32_t ui32PumpGracePeriod)
{
    // Check the arguments.
    ASSERT(ui32PowerMode == FLASH_PWR_ACTIVE_MODE ||
           ui32PowerMode == FLASH_PWR_OFF_MODE    ||
           ui32PowerMode == FLASH_PWR_DEEP_STDBY_MODE);
    ASSERT(ui32BankGracePeriod <= 0xFF);
    ASSERT(ui32PumpGracePeriod <= 0xFFFF);

    switch(ui32PowerMode)
    {
    case FLASH_PWR_ACTIVE_MODE:
        // Set bank power mode to ACTIVE.
        HWREG(FLASH_BASE + FLASH_O_FBFALLBACK) =
            (HWREG(FLASH_BASE + FLASH_O_FBFALLBACK) &
             ~FLASH_FBFALLBACK_BANKPWR0_M) | FBFALLBACK_ACTIVE;

        // Set charge pump power mode to ACTIVE mode.
        HWREG(FLASH_BASE + FLASH_O_FPAC1) =
            (HWREG(FLASH_BASE + FLASH_O_FPAC1) & ~FLASH_FPAC1_PUMPPWR_M) | (1 << FLASH_FPAC1_PUMPPWR_S);
        break;

    case FLASH_PWR_OFF_MODE:
        // Set bank grace period.
        HWREG(FLASH_BASE + FLASH_O_FBAC) =
            (HWREG(FLASH_BASE + FLASH_O_FBAC) & (~FLASH_FBAC_BAGP_M)) |
            ((ui32BankGracePeriod << FLASH_FBAC_BAGP_S) & FLASH_FBAC_BAGP_M);

        // Set pump grace period.
        HWREG(FLASH_BASE + FLASH_O_FPAC2) =
            (HWREG(FLASH_BASE + FLASH_O_FPAC2) & (~FLASH_FPAC2_PAGP_M)) |
            ((ui32PumpGracePeriod << FLASH_FPAC2_PAGP_S) & FLASH_FPAC2_PAGP_M);

        // Set bank power mode to SLEEP.
        HWREG(FLASH_BASE + FLASH_O_FBFALLBACK) &= ~FLASH_FBFALLBACK_BANKPWR0_M;

        // Set charge pump power mode to SLEEP mode.
        HWREG(FLASH_BASE + FLASH_O_FPAC1) &= ~FLASH_FPAC1_PUMPPWR_M;
        break;

    case FLASH_PWR_DEEP_STDBY_MODE:
        // Set bank grace period.
        HWREG(FLASH_BASE + FLASH_O_FBAC) =
            (HWREG(FLASH_BASE + FLASH_O_FBAC) & (~FLASH_FBAC_BAGP_M)) |
            ((ui32BankGracePeriod << FLASH_FBAC_BAGP_S) & FLASH_FBAC_BAGP_M);

        // Set pump grace period.
        HWREG(FLASH_BASE + FLASH_O_FPAC2) =
            (HWREG(FLASH_BASE + FLASH_O_FPAC2) & (~FLASH_FPAC2_PAGP_M)) |
            ((ui32PumpGracePeriod << FLASH_FPAC2_PAGP_S) & FLASH_FPAC2_PAGP_M);

        // Set bank power mode to DEEP STANDBY mode.
        HWREG(FLASH_BASE + FLASH_O_FBFALLBACK) =
            (HWREG(FLASH_BASE + FLASH_O_FBFALLBACK) &
             ~FLASH_FBFALLBACK_BANKPWR0_M) | FBFALLBACK_DEEP_STDBY;

        // Set charge pump power mode to STANDBY mode.
        HWREG(FLASH_BASE + FLASH_O_FPAC1) |= FLASH_FPAC1_PUMPPWR_M;
        break;
    }
}

//*****************************************************************************
//
// Get current configured power mode
//
//*****************************************************************************
uint32_t
FlashPowerModeGet(void)
{
    uint32_t ui32PowerMode;
    uint32_t ui32BankPwrMode;

    ui32BankPwrMode = HWREG(FLASH_BASE + FLASH_O_FBFALLBACK) &
                      FLASH_FBFALLBACK_BANKPWR0_M;

    if(ui32BankPwrMode == FBFALLBACK_SLEEP)
    {
        ui32PowerMode = FLASH_PWR_OFF_MODE;
    }
    else if(ui32BankPwrMode == FBFALLBACK_DEEP_STDBY)
    {
        ui32PowerMode = FLASH_PWR_DEEP_STDBY_MODE;
    }
    else
    {
        ui32PowerMode = FLASH_PWR_ACTIVE_MODE;
    }

    // Return power mode.
    return(ui32PowerMode);
}

//*****************************************************************************
//
// Set sector protection
//
//*****************************************************************************
void
FlashProtectionSet(uint32_t ui32SectorAddress, uint32_t ui32ProtectMode)
{
    uint32_t ui32SectorNumber;

    // Check the arguments.
    ASSERT(ui32SectorAddress <= (FLASHMEM_BASE + FlashSizeGet() -
                                 FlashSectorSizeGet()));
    ASSERT((ui32SectorAddress & (FlashSectorSizeGet() - 1)) == 00);

    if(ui32ProtectMode == FLASH_WRITE_PROTECT)
    {
        ui32SectorNumber = (ui32SectorAddress - FLASHMEM_BASE) /
                           FlashSectorSizeGet();
        HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_ENABLE;

        if(ui32SectorNumber <= 31)
        {
            HWREG(FLASH_BASE + FLASH_O_FSM_BSLE0) |= (1 << ui32SectorNumber);
            HWREG(FLASH_BASE + FLASH_O_FSM_BSLP0) |= (1 << ui32SectorNumber);
        }
        else if(ui32SectorNumber <= 63)
        {
            HWREG(FLASH_BASE + FLASH_O_FSM_BSLE1) |=
                (1 << (ui32SectorNumber & 0x1F));
            HWREG(FLASH_BASE + FLASH_O_FSM_BSLP1) |=
                (1 << (ui32SectorNumber & 0x1F));
        }

        HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_DISABLE;
    }
}

//*****************************************************************************
//
// Get sector protection
//
//*****************************************************************************
uint32_t
FlashProtectionGet(uint32_t ui32SectorAddress)
{
    uint32_t ui32SectorProtect;
    uint32_t ui32SectorNumber;

    // Check the arguments.
    ASSERT(ui32SectorAddress <= (FLASHMEM_BASE + FlashSizeGet() -
                                 FlashSectorSizeGet()));
    ASSERT((ui32SectorAddress & (FlashSectorSizeGet() - 1)) == 00);

    ui32SectorProtect = FLASH_NO_PROTECT;
    ui32SectorNumber = (ui32SectorAddress - FLASHMEM_BASE) / FlashSectorSizeGet();

    if(ui32SectorNumber <= 31)
    {
        if((HWREG(FLASH_BASE + FLASH_O_FSM_BSLE0) & (1 << ui32SectorNumber)) &&
                (HWREG(FLASH_BASE + FLASH_O_FSM_BSLP0) & (1 << ui32SectorNumber)))
        {
            ui32SectorProtect = FLASH_WRITE_PROTECT;
        }
    }
    else if(ui32SectorNumber <= 63)
    {
        if((HWREG(FLASH_BASE + FLASH_O_FSM_BSLE1) &
                (1 << (ui32SectorNumber & 0x1F))) &&
                (HWREG(FLASH_BASE + FLASH_O_FSM_BSLP1) &
                 (1 << (ui32SectorNumber & 0x1F))))
        {
            ui32SectorProtect = FLASH_WRITE_PROTECT;
        }
    }

    return(ui32SectorProtect);
}

//*****************************************************************************
//
// Save sector protection to make it permanent
//
//*****************************************************************************
uint32_t
FlashProtectionSave(uint32_t ui32SectorAddress)
{
    uint32_t ui32ErrorReturn;
    uint32_t ui32SectorNumber;
    uint32_t ui32CcfgSectorAddr;
    uint32_t ui32ProgBuf;

    ui32ErrorReturn = FAPI_STATUS_SUCCESS;

    // Check the arguments.
    ASSERT(ui32SectorAddress <= (FLASHMEM_BASE + FlashSizeGet() -
                                 FlashSectorSizeGet()));
    ASSERT((ui32SectorAddress & (FlashSectorSizeGet() - 1)) == 00);

    if(FlashProtectionGet(ui32SectorAddress) == FLASH_WRITE_PROTECT)
    {
        // Find sector number for specified sector.
        ui32SectorNumber = (ui32SectorAddress - FLASHMEM_BASE) / FlashSectorSizeGet();
        ui32CcfgSectorAddr = FLASHMEM_BASE + FlashSizeGet() - FlashSectorSizeGet();

        // Adjust CCFG address to the 32-bit CCFG word holding the
        // protect-bit for the specified sector.
        ui32CcfgSectorAddr += (((ui32SectorNumber >> 5) * 4) + CCFG_OFFSET_SECT_PROT);

        // Find value to program by setting the protect-bit which
        // corresponds to specified sector number, to 0.
        // Leave other protect-bits unchanged.
        ui32ProgBuf = (~(1 << (ui32SectorNumber & 0x1F))) &
                                   *(uint32_t *)ui32CcfgSectorAddr;

        ui32ErrorReturn = FlashProgram((uint8_t*)&ui32ProgBuf, ui32CcfgSectorAddr,
                                       CCFG_SIZE_SECT_PROT);
    }

    // Return status.
    return(ui32ErrorReturn);
}

//*****************************************************************************
//
// Erase a flash sector
//
//*****************************************************************************
uint32_t
FlashSectorErase(uint32_t ui32SectorAddress)
{
    uint32_t ui32ErrorReturn;
    FlashSectorErasePointer_t FuncPointer;

    // Check the arguments.
    ASSERT(ui32SectorAddress <= (FLASHMEM_BASE + FlashSizeGet() -
                                 FlashSectorSizeGet()));
    ASSERT((ui32SectorAddress & (FlashSectorSizeGet() - 1)) == 00);

    // Call ROM function that handles the actual erase operation
    FuncPointer = (uint32_t (*)(uint32_t)) (ROM_API_FLASH_TABLE[5]);
    ui32ErrorReturn = FuncPointer(ui32SectorAddress);

    // Enable standby in flash bank since ROM function might have disabled it
    HWREGBITW(FLASH_BASE + FLASH_O_CFG, FLASH_CFG_DIS_STANDBY_BITN ) = 0;

    // Return status of operation.
    return(ui32ErrorReturn);

}


//*****************************************************************************
//
// Programs unprotected main bank flash sectors
//
//*****************************************************************************
uint32_t
FlashProgram(uint8_t *pui8DataBuffer, uint32_t ui32Address, uint32_t ui32Count)
{
    uint32_t ui32ErrorReturn;
    FlashPrgPointer_t FuncPointer;

    // Check the arguments.
    ASSERT((ui32Address + ui32Count) <= (FLASHMEM_BASE + FlashSizeGet()));

    // Call ROM function that handles the actual program operation
    FuncPointer = (uint32_t (*)(uint8_t *, uint32_t, uint32_t)) (ROM_API_FLASH_TABLE[6]);
    ui32ErrorReturn = FuncPointer( pui8DataBuffer, ui32Address, ui32Count);

    // Enable standby in flash bank since ROM function might have disabled it
    HWREGBITW(FLASH_BASE + FLASH_O_CFG, FLASH_CFG_DIS_STANDBY_BITN ) = 0;

    // Return status of operation.
    return(ui32ErrorReturn);

}

//*****************************************************************************
//
// Reads efuse data from specified row
//
//*****************************************************************************
bool
FlashEfuseReadRow(uint32_t *pui32EfuseData, uint32_t ui32RowAddress)
{
    bool bStatus;

    // Make sure the clock for the efuse is enabled
    HWREG(FLASH_BASE + FLASH_O_CFG) &= ~FLASH_CFG_DIS_EFUSECLK;

    // Set timing for EFUSE read operations.
    HWREG(FLASH_BASE + FLASH_O_EFUSEREAD) |= ((5 << FLASH_EFUSEREAD_READCLOCK_S) &
            FLASH_EFUSEREAD_READCLOCK_M);

    // Clear status register.
    HWREG(FLASH_BASE + FLASH_O_EFUSEERROR) = 0;

    // Select the FuseROM block 0.
    HWREG(FLASH_BASE + FLASH_O_EFUSEADDR) = 0x00000000;

    // Start the read operation.
    HWREG(FLASH_BASE + FLASH_O_EFUSE) =
        (DUMPWORD_INSTR << FLASH_EFUSE_INSTRUCTION_S) |
        (ui32RowAddress & FLASH_EFUSE_DUMPWORD_M);

    // Wait for operation to finish.
    while(!(HWREG(FLASH_BASE + FLASH_O_EFUSEERROR) & FLASH_EFUSEERROR_DONE))
    {
    }

    // Check if error reported.
    if(HWREG(FLASH_BASE + FLASH_O_EFUSEERROR) & FLASH_EFUSEERROR_CODE_M)
    {
        // Set error status.
        bStatus = 1;

        // Clear data.
        *pui32EfuseData = 0;
    }
    else
    {
        // Set ok status.
        bStatus = 0;

        // No error. Get data from data register.
        *pui32EfuseData = HWREG(FLASH_BASE + FLASH_O_DATALOWER);
    }

    // Disable the efuse clock to conserve power
    HWREG(FLASH_BASE + FLASH_O_CFG) |= FLASH_CFG_DIS_EFUSECLK;

    // Return the data.
    return(bStatus);
}


//*****************************************************************************
//
// Disables all sectors for erase and programming on the active bank
//
//*****************************************************************************
void
FlashDisableSectorsForWrite(void)
{
    // Configure flash back to read mode
    SetReadMode();

    // Disable Level 1 Protection.
    HWREG(FLASH_BASE + FLASH_O_FBPROT) = FLASH_FBPROT_PROTL1DIS;

    // Disable all sectors for erase and programming.
    HWREG(FLASH_BASE + FLASH_O_FBSE) = 0x0000;

    // Enable Level 1 Protection.
    HWREG(FLASH_BASE + FLASH_O_FBPROT) = 0;

    // Protect sectors from sector erase.
    HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_ENABLE;
    HWREG(FLASH_BASE + FLASH_O_FSM_SECTOR1) = 0xFFFFFFFF;
    HWREG(FLASH_BASE + FLASH_O_FSM_SECTOR2) = 0xFFFFFFFF;
    HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_DISABLE;
}

//*****************************************************************************
//
//! \internal
//! Used to set flash in read mode.
//!
//! Flash is configured with values loaded from OTP dependent on the current
//! regulator mode.
//!
//! \return None.
//
//*****************************************************************************
static void
SetReadMode(void)
{
    uint32_t ui32TrimValue;
    uint32_t ui32Value;

    // Configure the STANDBY_MODE_SEL, STANDBY_PW_SEL, DIS_STANDBY, DIS_IDLE,
    // VIN_AT_X and VIN_BY_PASS for read mode
    if(HWREG(AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL) &
       AON_PMCTL_PWRCTL_EXT_REG_MODE)
    {
        // Select trim values for external regulator mode:
        // Configure STANDBY_MODE_SEL (OTP offset 0x308 bit 7)
        // COnfigure STANDBY_PW_SEL   (OTP offset 0x308 bit 6:5)
        // Must be done while the register bit field CONFIG.DIS_STANDBY = 1
        HWREG(FLASH_BASE + FLASH_O_CFG) |= FLASH_CFG_DIS_STANDBY;

        ui32TrimValue =
           HWREG(FLASH_CFG_BASE + FCFG1_OFFSET + FCFG1_O_FLASH_OTP_DATA4);

        ui32Value = ((ui32TrimValue &
                      FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_RD_M) >>
                      FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_RD_S) <<
                    FLASH_CFG_STANDBY_MODE_SEL_S;

        ui32Value |= ((ui32TrimValue &
                       FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_RD_M) >>
                       FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_RD_S) <<
                     FLASH_CFG_STANDBY_PW_SEL_S;

        // Configure DIS_STANDBY (OTP offset 0x308 bit 4).
        // Configure DIS_IDLE    (OTP offset 0x308 bit 3).
        ui32Value |= ((ui32TrimValue &
                       (FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_RD_M |
                        FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_RD_M)) >>
                        FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_RD_S) <<
                        FLASH_CFG_DIS_IDLE_S;

        HWREG(FLASH_BASE + FLASH_O_CFG) = (HWREG(FLASH_BASE + FLASH_O_CFG) &
                                           ~(FLASH_CFG_STANDBY_MODE_SEL_M |
                                             FLASH_CFG_STANDBY_PW_SEL_M   |
                                             FLASH_CFG_DIS_STANDBY_M      |
                                             FLASH_CFG_DIS_IDLE_M)) | ui32Value;

        // Check if sample and hold functionality is disabled.
        if(HWREG(FLASH_BASE + FLASH_O_CFG) & FLASH_CFG_DIS_IDLE)
        {
            // Wait for disabled sample and hold functionality to be stable.
            while(!(HWREG(FLASH_BASE + FLASH_O_STAT) & FLASH_STAT_SAMHOLD_DIS))
            {
            }
        }

        // Configure VIN_AT_X (OTP offset 0x308 bits 2:0)
        ui32Value = ((ui32TrimValue &
                      FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_RD_M) >>
                      FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_RD_S) <<
                    FLASH_FSEQPMP_VIN_AT_X_S;

        // Configure VIN_BY_PASS which is dependent on the VIN_AT_X value.
        // If VIN_AT_X = 7 then VIN_BY_PASS should be 0 otherwise
        // VIN_BY_PASS should be 1
        if(((ui32Value & FLASH_FSEQPMP_VIN_AT_X_M) >>
            FLASH_FSEQPMP_VIN_AT_X_S) != 0x7)
        {
            ui32Value |= FLASH_FSEQPMP_VIN_BY_PASS;
        }

        HWREG(FLASH_BASE + FLASH_O_FLOCK) = 0xAAAA;
        HWREG(FLASH_BASE + FLASH_O_FSEQPMP) =
                                    (HWREG(FLASH_BASE + FLASH_O_FSEQPMP) &
                                     ~(FLASH_FSEQPMP_VIN_BY_PASS_M |
                                       FLASH_FSEQPMP_VIN_AT_X_M))  | ui32Value;
        HWREG(FLASH_BASE + FLASH_O_FLOCK) = 0x55AA;
    }
    else
    {
        // Select trim values for internal regulator mode:
        // Configure STANDBY_MODE_SEL (OTP offset 0x308 bit 15)
        // COnfigure STANDBY_PW_SEL   (OTP offset 0x308 bit 14:13)
        // Must be done while the register bit field CONFIG.DIS_STANDBY = 1
        HWREG(FLASH_BASE + FLASH_O_CFG) |= FLASH_CFG_DIS_STANDBY;

        ui32TrimValue =
           HWREG(FLASH_CFG_BASE + FCFG1_OFFSET + FCFG1_O_FLASH_OTP_DATA4);

        ui32Value = ((ui32TrimValue &
                      FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_RD_M) >>
                      FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_RD_S) <<
                    FLASH_CFG_STANDBY_MODE_SEL_S;

        ui32Value |= ((ui32TrimValue &
                       FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_RD_M) >>
                       FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_RD_S) <<
                     FLASH_CFG_STANDBY_PW_SEL_S;

        // Configure DIS_STANDBY (OTP offset 0x308 bit 12).
        // Configure DIS_IDLE    (OTP offset 0x308 bit 11).
        ui32Value |= ((ui32TrimValue &
                       (FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_RD_M |
                        FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_RD_M)) >>
                        FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_RD_S) <<
                        FLASH_CFG_DIS_IDLE_S;

        HWREG(FLASH_BASE + FLASH_O_CFG) = (HWREG(FLASH_BASE + FLASH_O_CFG) &
                                           ~(FLASH_CFG_STANDBY_MODE_SEL_M |
                                             FLASH_CFG_STANDBY_PW_SEL_M   |
                                             FLASH_CFG_DIS_STANDBY_M      |
                                             FLASH_CFG_DIS_IDLE_M)) | ui32Value;

        // Check if sample and hold functionality is disabled.
        if(HWREG(FLASH_BASE + FLASH_O_CFG) & FLASH_CFG_DIS_IDLE)
        {
            // Wait for disabled sample and hold functionality to be stable.
            while(!(HWREG(FLASH_BASE + FLASH_O_STAT) & FLASH_STAT_SAMHOLD_DIS))
            {
            }
        }

        // Configure VIN_AT_X (OTP offset 0x308 bits 10:8)
        ui32Value = (((ui32TrimValue &
                       FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_RD_M) >>
                       FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_RD_S) <<
                       FLASH_FSEQPMP_VIN_AT_X_S);

        // Configure VIN_BY_PASS which is dependent on the VIN_AT_X value.
        // If VIN_AT_X = 7 then VIN_BY_PASS should be 0 otherwise
        // VIN_BY_PASS should be 1
        if(((ui32Value & FLASH_FSEQPMP_VIN_AT_X_M) >>
            FLASH_FSEQPMP_VIN_AT_X_S) != 0x7)
        {
            ui32Value |= FLASH_FSEQPMP_VIN_BY_PASS;
        }

        HWREG(FLASH_BASE + FLASH_O_FLOCK) = 0xAAAA;
        HWREG(FLASH_BASE + FLASH_O_FSEQPMP) =
                                    (HWREG(FLASH_BASE + FLASH_O_FSEQPMP) &
                                     ~(FLASH_FSEQPMP_VIN_BY_PASS_M |
                                       FLASH_FSEQPMP_VIN_AT_X_M)) | ui32Value;
        HWREG(FLASH_BASE + FLASH_O_FLOCK) = 0x55AA;
    }
}

//*****************************************************************************
//
// HAPI Flash program function
//
//*****************************************************************************
uint32_t
MemBusWrkAroundHapiProgramFlash(uint8_t *pui8DataBuffer, uint32_t ui32Address,
                                uint32_t ui32Count)
{
    uint32_t ui32ErrorReturn;
    FlashPrgPointer_t FuncPointer;
    uint32_t ui32RomAddr = HWREG(ROM_HAPI_TABLE_ADDR + (5 * 4));

    // Call ROM function
    FuncPointer = (uint32_t (*)(uint8_t *, uint32_t, uint32_t)) (ui32RomAddr);
    ui32ErrorReturn = FuncPointer( pui8DataBuffer, ui32Address, ui32Count);

    // Enable standby in flash bank since ROM function might have disabled it
    HWREGBITW(FLASH_BASE + FLASH_O_CFG, FLASH_CFG_DIS_STANDBY_BITN ) = 0;

    // Return status of operation.
    return(ui32ErrorReturn);
}

//*****************************************************************************
//
// HAPI Flash sector erase function
//
//*****************************************************************************
uint32_t
MemBusWrkAroundHapiEraseSector(uint32_t ui32Address)
{
    uint32_t ui32ErrorReturn;

    FlashSectorErasePointer_t FuncPointer;
    uint32_t ui32RomAddr = HWREG(ROM_HAPI_TABLE_ADDR + (3 * 4));

    // Call ROM function
    FuncPointer = (uint32_t (*)(uint32_t)) (ui32RomAddr);
    ui32ErrorReturn = FuncPointer(ui32Address);

    // Enable standby in flash bank since ROM function might have disabled it
    HWREGBITW(FLASH_BASE + FLASH_O_CFG, FLASH_CFG_DIS_STANDBY_BITN ) = 0;

    // Return status of operation.
    return(ui32ErrorReturn);
}
