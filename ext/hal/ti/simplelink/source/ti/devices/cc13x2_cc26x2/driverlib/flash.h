/******************************************************************************
*  Filename:       flash.h
*  Revised:        2017-11-02 16:09:32 +0100 (Thu, 02 Nov 2017)
*  Revision:       50166
*
*  Description:    Defines and prototypes for the Flash driver.
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

//*****************************************************************************
//
//! \addtogroup system_control_group
//! @{
//! \addtogroup flash_api
//! @{
//
//*****************************************************************************

#ifndef __FLASH_H__
#define __FLASH_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_flash.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_aon_pmctl.h"
#include "../inc/hw_fcfg1.h"
#include "interrupt.h"
#include "debug.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define FlashPowerModeSet               NOROM_FlashPowerModeSet
    #define FlashPowerModeGet               NOROM_FlashPowerModeGet
    #define FlashProtectionSet              NOROM_FlashProtectionSet
    #define FlashProtectionGet              NOROM_FlashProtectionGet
    #define FlashProtectionSave             NOROM_FlashProtectionSave
    #define FlashSectorErase                NOROM_FlashSectorErase
    #define FlashProgram                    NOROM_FlashProgram
    #define FlashEfuseReadRow               NOROM_FlashEfuseReadRow
    #define FlashDisableSectorsForWrite     NOROM_FlashDisableSectorsForWrite
#endif

//*****************************************************************************
//
// Values that can be returned from the API functions
//
//*****************************************************************************
#define FAPI_STATUS_SUCCESS     0x00000000  // Function completed successfully
#define FAPI_STATUS_FSM_BUSY    0x00000001  // FSM is Busy
#define FAPI_STATUS_FSM_READY   0x00000002  // FSM is Ready
#define FAPI_STATUS_INCORRECT_DATABUFFER_LENGTH \
                                0x00000003  // Incorrect parameter value
#define FAPI_STATUS_FSM_ERROR   0x00000004  // Operation failed

//*****************************************************************************
//
// Values passed to FlashIntEnable(), FlashIntDisable() and FlashIntClear() and
// returned from FlashIntStatus().
//
//*****************************************************************************
#define FLASH_INT_FSM_DONE      0x00400000  // FSM Done Interrupt Mask
#define FLASH_INT_RV            0x00010000  // Read Verify error Interrupt Mask

//*****************************************************************************
//
// Values passed to FlashSetPowerMode() and returned from FlashGetPowerMode().
//
//*****************************************************************************
#define FLASH_PWR_ACTIVE_MODE   0x00000000
#define FLASH_PWR_OFF_MODE      0x00000001
#define FLASH_PWR_DEEP_STDBY_MODE \
                                0x00000002

//*****************************************************************************
//
// Values passed to FlashSetProtection() and returned from FlashGetProtection().
//
//*****************************************************************************
#define FLASH_NO_PROTECT        0x00000000 // Sector not protected
#define FLASH_WRITE_PROTECT     0x00000001 // Sector erase and program
                                           // protected

//*****************************************************************************
//
// Define used by the flash programming and erase functions
//
//*****************************************************************************
#define ADDR_OFFSET            (0x1F800000 - FLASHMEM_BASE)

//*****************************************************************************
//
// Define used for access to factory configuration area.
//
//*****************************************************************************
#define FCFG1_OFFSET           0x1000

//*****************************************************************************
//
// Define for the clock frequency input to the flash module in number of MHz
//
//*****************************************************************************
#define FLASH_MODULE_CLK_FREQ  48

//*****************************************************************************
//
//! \brief Defined values for Flash State Machine commands
//
//*****************************************************************************
typedef enum
{
    FAPI_PROGRAM_DATA    = 0x0002, //!< Program data.
    FAPI_ERASE_SECTOR    = 0x0006, //!< Erase sector.
    FAPI_ERASE_BANK      = 0x0008, //!< Erase bank.
    FAPI_VALIDATE_SECTOR = 0x000E, //!< Validate sector.
    FAPI_CLEAR_STATUS    = 0x0010, //!< Clear status.
    FAPI_PROGRAM_RESUME  = 0x0014, //!< Program resume.
    FAPI_ERASE_RESUME    = 0x0016, //!< Erase resume.
    FAPI_CLEAR_MORE      = 0x0018, //!< Clear more.
    FAPI_PROGRAM_SECTOR  = 0x0020, //!< Program sector.
    FAPI_ERASE_OTP       = 0x0030  //!< Erase OTP.
} tFlashStateCommandsType;

//*****************************************************************************
//
// Defines for values written to the FLASH_O_FSM_WR_ENA register
//
//*****************************************************************************
#define FSM_REG_WRT_ENABLE     5
#define FSM_REG_WRT_DISABLE    2

//*****************************************************************************
//
// Defines for the bank power mode field the FLASH_O_FBFALLBACK register
//
//*****************************************************************************
#define FBFALLBACK_SLEEP       0
#define FBFALLBACK_DEEP_STDBY  1
#define FBFALLBACK_ACTIVE      3

//*****************************************************************************
//
// Defines for the bank grace period and pump grace period
//
//*****************************************************************************
#define FLASH_BAGP             0x14
#define FLASH_PAGP             0x14

//*****************************************************************************
//
// Defines used by the FlashProgramPattern() function
//
//*****************************************************************************
#define PATTERN_BITS           0x20  // No of bits in data pattern to program

//*****************************************************************************
//
// Defines for the FW flag bits in the FLASH_O_FWFLAG register
//
//*****************************************************************************
#define FW_WRT_TRIMMED         0x00000001

//*****************************************************************************
//
// Defines used by the flash programming functions
//
//*****************************************************************************
typedef volatile uint8_t tFwpWriteByte;
#define FWPWRITE_BYTE_ADDRESS ((tFwpWriteByte *)((FLASH_BASE + FLASH_O_FWPWRITE0)))

//*****************************************************************************
//
// Define for efuse instruction
//
//*****************************************************************************
#define DUMPWORD_INSTR         0x04

//*****************************************************************************
//
// Define for FSM command execution
//
//*****************************************************************************
#define FLASH_CMD_EXEC         0x15

//*****************************************************************************
//
//! \brief Get size of a flash sector in number of bytes.
//!
//! This function will return the size of a flash sector in number of bytes.
//!
//! \return Returns size of a flash sector in number of bytes.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
FlashSectorSizeGet(void)
{
    uint32_t ui32SectorSizeInKbyte;

    ui32SectorSizeInKbyte = (HWREG(FLASH_BASE + FLASH_O_FCFG_B0_SSIZE0) &
                             FLASH_FCFG_B0_SSIZE0_B0_SECT_SIZE_M) >>
                            FLASH_FCFG_B0_SSIZE0_B0_SECT_SIZE_S;

    // Return flash sector size in number of bytes.
    return(ui32SectorSizeInKbyte * 1024);
}

//*****************************************************************************
//
//! \brief Get the size of the flash.
//!
//! This function returns the size of the flash main bank in number of bytes.
//!
//! \return Returns the flash size in number of bytes.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
FlashSizeGet(void)
{
    uint32_t ui32NoOfSectors;

    // Get number of flash sectors
    ui32NoOfSectors = (HWREG(FLASH_BASE + FLASH_O_FLASH_SIZE) &
                       FLASH_FLASH_SIZE_SECTORS_M) >>
                      FLASH_FLASH_SIZE_SECTORS_S;

    // Return flash size in number of bytes
    return(ui32NoOfSectors * FlashSectorSizeGet());
}

//*****************************************************************************
//
//! \brief Set power mode.
//!
//! This function will set the specified power mode.
//!
//! Any access to the bank causes a reload of the specified bank grace period
//! input value into the bank down counter. After the last access to the
//! flash bank, the down counter delays from 0 to 255 prescaled HCLK clock
//! cycles before putting the bank into one of the fallback power modes as
//! determined by \c ui32PowerMode. This value must be greater than 1 when the
//! fallback mode is not \ref FLASH_PWR_ACTIVE_MODE.
//!
//! Note: The prescaled clock used for the down counter is a clock divided by
//! 16 from input HCLK. The \c ui32BankGracePeriod parameter is ignored if
//! \c ui32PowerMode is equal to \ref FLASH_PWR_ACTIVE_MODE.
//! Any access to flash memory causes the pump grace period down counter to
//! reload with value of \c ui32PumpGracePeriod. After the bank has gone to sleep,
//! the down counter delays this number of prescaled HCLK clock cycles before
//! entering one of the charge pump fallback power modes as determined by
//! \c ui32PowerMode. The prescaled clock used for the pump grace period down
//! counter is a clock divided by 16 from input HCLK. This parameter is ignored
//! if \c ui32PowerMode is equal to \ref FLASH_PWR_ACTIVE_MODE.
//!
//! Changing the power mode of the flash module must be a part within a
//! device power mode transition requiring configuration of multiple modules.
//! Refer to documents describing the device power modes.
//!
//! \param ui32PowerMode is the wanted power mode.
//! The defined flash power modes are:
//! - \ref FLASH_PWR_ACTIVE_MODE
//! - \ref FLASH_PWR_OFF_MODE
//! - \ref FLASH_PWR_DEEP_STDBY_MODE
//! \param ui32BankGracePeriod is the starting count value for the bank grace
//! period down counter.
//! \param ui32PumpGracePeriod is the starting count value for the pump grace
//! period down counter.
//!
//! \return None
//
//*****************************************************************************
extern void FlashPowerModeSet(uint32_t ui32PowerMode,
                              uint32_t ui32BankGracePeriod,
                              uint32_t ui32PumpGracePeriod);

//*****************************************************************************
//
//! \brief Get current configured power mode.
//!
//! This function will return the current configured power mode.
//!
//! \return Returns the current configured power mode.
//! The defined power modes are:
//! - \ref FLASH_PWR_ACTIVE_MODE
//! - \ref FLASH_PWR_OFF_MODE
//! - \ref FLASH_PWR_DEEP_STDBY_MODE
//
//*****************************************************************************
extern uint32_t FlashPowerModeGet(void);

//*****************************************************************************
//
//! \brief Set sector protection.
//!
//! This function will set the specified protection on specified flash bank
//! sector. A sector can either have no protection or have write protection
//! which guards for both program and erase of that sector.
//! Sector protection can only be changed from \ref FLASH_NO_PROTECT to
//! \ref FLASH_WRITE_PROTECT! After write protecting a sector this sector can
//! only be set back to unprotected by a device reset.
//!
//! \param ui32SectorAddress is the start address of the sector to protect.
//! \param ui32ProtectMode is the enumerated sector protection mode.
//! - \ref FLASH_NO_PROTECT
//! - \ref FLASH_WRITE_PROTECT
//!
//! \return None
//
//*****************************************************************************
extern void FlashProtectionSet(uint32_t ui32SectorAddress,
                               uint32_t ui32ProtectMode);

//*****************************************************************************
//
//! \brief Get sector protection.
//!
//! This return the protection mode for the specified flash bank sector.
//!
//! \param ui32SectorAddress is the start address of the desired sector.
//!
//! \return Returns the sector protection:
//! - \ref FLASH_NO_PROTECT
//! - \ref FLASH_WRITE_PROTECT
//
//*****************************************************************************
extern uint32_t FlashProtectionGet(uint32_t ui32SectorAddress);

//*****************************************************************************
//
//! \brief Save sector protection to make it permanent.
//!
//! This function will save the current protection mode for the specified
//! flash bank sector.
//!
//! This function must only be executed from ROM or SRAM.
//!
//! \note A write protected sector will become permanent write
//! protected!! A device reset will not change the write protection!
//!
//! \param ui32SectorAddress is the start address of the sector to be protected.
//!
//! \return Returns the status of the sector protection:
//! - \ref FAPI_STATUS_SUCCESS : Success.
//! - \ref FAPI_STATUS_FSM_ERROR : An erase error is encountered.
//
//*****************************************************************************
extern uint32_t FlashProtectionSave(uint32_t ui32SectorAddress);

//*****************************************************************************
//
//! \brief Checks if the Flash state machine has detected an error.
//!
//! This function returns the status of the Flash State Machine indicating if
//! an error is detected or not. Primary use is to check if an Erase or
//! Program operation has failed.
//!
//! \note Please note that code can not execute in flash while any part of the flash
//! is being programmed or erased. This function must be called from ROM or
//! SRAM while any part of the flash is being programmed or erased.
//!
//! \return Returns status of Flash state machine:
//! - \ref FAPI_STATUS_FSM_ERROR
//! - \ref FAPI_STATUS_SUCCESS
//
//*****************************************************************************
__STATIC_INLINE uint32_t
FlashCheckFsmForError(void)
{
    if(HWREG(FLASH_BASE + FLASH_O_FMSTAT) & FLASH_FMSTAT_CSTAT)
    {
        return(FAPI_STATUS_FSM_ERROR);
    }
    else
    {
        return(FAPI_STATUS_SUCCESS);
    }
}

//*****************************************************************************
//
//! \brief Checks if the Flash state machine is ready.
//!
//! This function returns the status of the Flash State Machine indicating if
//! it is ready to accept a new command or not. Primary use is to check if an
//! Erase or Program operation has finished.
//!
//! \note Please note that code can not execute in flash while any part of the flash
//! is being programmed or erased. This function must be called from ROM or
//! SRAMh while any part of the flash is being programmed or erased.
//!
//! \return Returns readiness status of Flash state machine:
//! - \ref FAPI_STATUS_FSM_READY
//! - \ref FAPI_STATUS_FSM_BUSY
//
//*****************************************************************************
__STATIC_INLINE uint32_t
FlashCheckFsmForReady(void)
{
    if(HWREG(FLASH_BASE + FLASH_O_STAT) & FLASH_STAT_BUSY)
    {
        return(FAPI_STATUS_FSM_BUSY);
    }
    else
    {
        return(FAPI_STATUS_FSM_READY);
    }
}

//*****************************************************************************
//
//! \brief Registers an interrupt handler for the flash interrupt in the dynamic interrupt table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific FLASH interrupts must be enabled via \ref FlashIntEnable(). It is the
//! interrupt handler's responsibility to clear the interrupt source.
//!
//! \param pfnHandler is a pointer to the function to be called when the flash
//! interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void
FlashIntRegister(void (*pfnHandler)(void))
{
    // Register the interrupt handler.
    IntRegister(INT_FLASH, pfnHandler);

    // Enable the flash interrupt.
    IntEnable(INT_FLASH);
}

//*****************************************************************************
//
//! \brief Unregisters the interrupt handler for the flash interrupt in the dynamic interrupt table.
//!
//! This function does the actual unregistering of the interrupt handler. It
//! clears the handler to be called when a FLASH interrupt occurs. This
//! function also masks off the interrupt in the interrupt controller so that
//! the interrupt handler no longer is called.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void
FlashIntUnregister(void)
{
    // Disable the interrupts.
    IntDisable(INT_FLASH);

    // Unregister the interrupt handler.
    IntUnregister(INT_FLASH);
}

//*****************************************************************************
//
//! \brief Enables flash controller interrupt sources.
//!
//! This function enables the flash controller interrupt sources.
//!
//! \param ui32IntFlags is the bit mask of the interrupt sources to be enabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref FLASH_INT_FSM_DONE : FSM Done interrupt.
//! - \ref FLASH_INT_RV       : Read verify error interrupt.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
FlashIntEnable(uint32_t ui32IntFlags)
{
    HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_ENABLE;
    HWREG(FLASH_BASE + FLASH_O_FSM_ST_MACHINE) |= ui32IntFlags;
    HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_DISABLE;
}

//*****************************************************************************
//
//! \brief Disables individual flash controller interrupt sources.
//!
//! This function disables the flash controller interrupt sources.
//!
//! \param ui32IntFlags is the bit mask of the interrupt sources to be disabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref FLASH_INT_FSM_DONE : FSM Done interrupt.
//! - \ref FLASH_INT_RV       : Read verify error interrupt.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
FlashIntDisable(uint32_t ui32IntFlags)
{
    HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_ENABLE;
    HWREG(FLASH_BASE + FLASH_O_FSM_ST_MACHINE) &= ~ui32IntFlags;
    HWREG(FLASH_BASE + FLASH_O_FSM_WR_ENA) = FSM_REG_WRT_DISABLE;
}

//*****************************************************************************
//
//! \brief Gets the current interrupt status.
//!
//! This function returns the interrupt status for the Flash.
//!
//! \return Returns the current interrupt status as values described in
//! \ref FlashIntEnable().
//
//*****************************************************************************
__STATIC_INLINE uint32_t
FlashIntStatus(void)
{
    uint32_t ui32IntFlags;

    ui32IntFlags = 0;

    // Check if FSM_DONE interrupt status is set.
    if(HWREG(FLASH_BASE + FLASH_O_FEDACSTAT) & FLASH_FEDACSTAT_FSM_DONE)
    {
        ui32IntFlags = FLASH_INT_FSM_DONE;
    }

    // Check if RVF_INT interrupt status is set.
    if(HWREG(FLASH_BASE + FLASH_O_FEDACSTAT) & FLASH_FEDACSTAT_RVF_INT)
    {
        ui32IntFlags |= FLASH_INT_RV;
    }

    return(ui32IntFlags);
}

//*****************************************************************************
//
//! \brief Clears flash controller interrupt source.
//!
//! The flash controller interrupt source is cleared, so that it no longer
//! asserts. This must be done in the interrupt handler to keep it from being
//! called again immediately upon exit.
//!
//! \note Due to write buffers and synchronizers in the system it may take several
//! clock cycles from a register write clearing an event in a module and until the
//! event is actually cleared in the NVIC of the system CPU. It is recommended to
//! clear the event source early in the interrupt service routine (ISR) to allow
//! the event clear to propagate to the NVIC before returning from the ISR.
//! At the same time, an early event clear allows new events of the same type to be
//! pended instead of ignored if the event is cleared later in the ISR.
//! It is the responsibility of the programmer to make sure that enough time has passed
//! before returning from the ISR to avoid false re-triggering of the cleared event.
//! A simple, although not necessarily optimal, way of clearing an event before
//! returning from the ISR is:
//! -# Write to clear event (interrupt source). (buffered write)
//! -# Dummy read from the event source module. (making sure the write has propagated)
//! -# Wait two system CPU clock cycles (user code or two NOPs). (allowing cleared event to propagate through any synchronizers)
//!
//! \param ui32IntFlags is the bit mask of the interrupt sources to be cleared.
//! Can be any of:
//! - \ref FLASH_INT_FSM_DONE
//! - \ref FLASH_INT_RV
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
FlashIntClear(uint32_t ui32IntFlags)
{
    uint32_t ui32TempVal;

    ui32TempVal = 0;

    if(ui32IntFlags & FLASH_INT_FSM_DONE)
    {
        ui32TempVal = FLASH_FEDACSTAT_FSM_DONE;
    }

    if(ui32IntFlags & FLASH_INT_RV)
    {
        ui32TempVal |= FLASH_FEDACSTAT_RVF_INT;
    }

    // Clear the flash interrupt source.
    HWREG(FLASH_BASE + FLASH_O_FEDACSTAT) = ui32TempVal;
}

//*****************************************************************************
//
//! \brief Erase a flash sector.
//!
//! This function will erase the specified flash sector. The function will
//! not return until the flash sector has been erased or an error condition
//! occurred. If flash top sector is erased the function will program the
//! the device security data bytes with default values. The device security
//! data located in the customer configuration area of the flash top sector,
//! must have valid values at all times. These values affect the configuration
//! of the device during boot.
//!
//! \warning Please note that code can not execute in flash while any part of the flash
//! is being programmed or erased. The application must disable interrupts that have
//! interrupt routines in flash. This function calls a ROM function which handles the
//! actual program operation.
//!
//! \param ui32SectorAddress is the starting address in flash of the sector to be
//! erased.
//!
//! \return Returns the status of the sector erase:
//! - \ref FAPI_STATUS_SUCCESS                     : Success.
//! - \ref FAPI_STATUS_INCORRECT_DATABUFFER_LENGTH : Invalid argument.
//! - \ref FAPI_STATUS_FSM_ERROR                   : A programming error is encountered.
//
//*****************************************************************************
extern uint32_t FlashSectorErase(uint32_t ui32SectorAddress);


//*****************************************************************************
//
//! \brief Programs unprotected flash sectors in the main bank.
//!
//! This function programs a sequence of bytes into the on-chip flash.
//! Programming each location consists of the result of an AND operation
//! of the new data and the existing data; in other words bits that contain
//! 1 can remain 1 or be changed to 0, but bits that are 0 cannot be changed
//! to 1. Therefore, a byte can be programmed multiple times as long as these
//! rules are followed; if a program operation attempts to change a 0 bit to
//! a 1 bit, that bit will not have its value changed.
//!
//! This function does not return until the data has been programmed or a
//! programming error occurs.
//!
//! \note It is recommended to disable cache and line buffer before programming the
//! flash. Cache and line buffer are not automatically updated if a flash program
//! causes a mismatch between new flash content and old content in cache and
//! line buffer. Remember to enable cache and line buffer when the program
//! operation completes. See \ref VIMSModeSafeSet(), \ref VIMSLineBufDisable(),
//! and \ref VIMSLineBufEnable() for more information.
//!
//! \warning Please note that code can not execute in flash while any part of the flash
//! is being programmed or erased. The application must disable interrupts that have
//! interrupt routines in flash. This function calls a ROM function which handles the
//! actual program operation.
//!
//! The \c pui8DataBuffer pointer can not point to flash.
//!
//! \param pui8DataBuffer is a pointer to the data to be programmed.
//! \param ui32Address is the starting address in flash to be programmed.
//! \param ui32Count is the number of bytes to be programmed.
//!
//! \return Returns status of the flash programming:
//! - \ref FAPI_STATUS_SUCCESS                     : Success.
//! - \ref FAPI_STATUS_INCORRECT_DATABUFFER_LENGTH : Too many bytes were requested.
//! - \ref FAPI_STATUS_FSM_ERROR                   : A programming error is encountered.
//
//*****************************************************************************
extern uint32_t FlashProgram(uint8_t *pui8DataBuffer,
                             uint32_t ui32Address, uint32_t ui32Count);

//*****************************************************************************
//
//! \brief Reads efuse data from specified row.
//!
//! This function will read one efuse row.
//! It is assumed that any previous efuse operation has finished.
//!
//! \param pui32EfuseData is pointer to variable to be updated with efuse data.
//! \param ui32RowAddress is the efuse row number to be read. First row is row
//! number 0.
//!
//! \return Returns the status of the efuse read operation.
//! - \c false : OK status.
//! - \c true  : Error status
//
//*****************************************************************************
extern bool FlashEfuseReadRow(uint32_t *pui32EfuseData,
                              uint32_t ui32RowAddress);

//*****************************************************************************
//
//! \brief Disables all sectors for erase and programming on the active bank.
//!
//! This function disables all sectors for erase and programming on the active
//! bank and enables the Idle Reading Power reduction mode if no low power
//! mode is configured. Furthermore, an additional level of protection from
//! erase is enabled.
//!
//! \note Please note that code can not execute in flash while any part of the flash
//! is being programmed or erased.
//!
//! \return None
//
//*****************************************************************************
extern void FlashDisableSectorsForWrite(void);


//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_FlashPowerModeSet
        #undef  FlashPowerModeSet
        #define FlashPowerModeSet               ROM_FlashPowerModeSet
    #endif
    #ifdef ROM_FlashPowerModeGet
        #undef  FlashPowerModeGet
        #define FlashPowerModeGet               ROM_FlashPowerModeGet
    #endif
    #ifdef ROM_FlashProtectionSet
        #undef  FlashProtectionSet
        #define FlashProtectionSet              ROM_FlashProtectionSet
    #endif
    #ifdef ROM_FlashProtectionGet
        #undef  FlashProtectionGet
        #define FlashProtectionGet              ROM_FlashProtectionGet
    #endif
    #ifdef ROM_FlashProtectionSave
        #undef  FlashProtectionSave
        #define FlashProtectionSave             ROM_FlashProtectionSave
    #endif
    #ifdef ROM_FlashSectorErase
        #undef  FlashSectorErase
        #define FlashSectorErase                ROM_FlashSectorErase
    #endif
    #ifdef ROM_FlashProgram
        #undef  FlashProgram
        #define FlashProgram                    ROM_FlashProgram
    #endif
    #ifdef ROM_FlashEfuseReadRow
        #undef  FlashEfuseReadRow
        #define FlashEfuseReadRow               ROM_FlashEfuseReadRow
    #endif
    #ifdef ROM_FlashDisableSectorsForWrite
        #undef  FlashDisableSectorsForWrite
        #define FlashDisableSectorsForWrite     ROM_FlashDisableSectorsForWrite
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __FLASH_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
