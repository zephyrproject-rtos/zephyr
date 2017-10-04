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
#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdbool.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

/* Define to ensure that our current MSP432 has the FLCTL module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_FLCTL__

//*****************************************************************************
//
//! \addtogroup flash_api
//! @{
//
//*****************************************************************************

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

//*****************************************************************************
//
// Control specific variables
//
//*****************************************************************************
#define FLASH_BURST_PRG_BIT 0x03

/* Interrupts */
#define FLASH_PROGRAM_ERROR         FLCTL_IFG_PRG_ERR
#define FLASH_BENCHMARK_INT         FLCTL_IFG_BMRK
#define FLASH_ERASE_COMPLETE        FLCTL_IFG_ERASE
#define FLASH_BRSTPRGM_COMPLETE     FLCTL_IFG_PRGB
#define FLASH_WRDPRGM_COMPLETE      FLCTL_IFG_PRG
#define FLASH_POSTVERIFY_FAILED     FLCTL_IFG_AVPST
#define FLASH_PREVERIFY_FAILED      FLCTL_IFG_AVPRE
#define FLASH_BRSTRDCMP_COMPLETE    FLCTL_IFG_RDBRST

#define FLASH_NORMAL_READ_MODE            FLCTL_BANK0_RDCTL_RD_MODE_0
#define FLASH_MARGIN0_READ_MODE           FLCTL_BANK0_RDCTL_RD_MODE_1
#define FLASH_MARGIN1_READ_MODE           FLCTL_BANK0_RDCTL_RD_MODE_2
#define FLASH_PROGRAM_VERIFY_READ_MODE    FLCTL_BANK0_RDCTL_RD_MODE_3
#define FLASH_ERASE_VERIFY_READ_MODE      FLCTL_BANK0_RDCTL_RD_MODE_4
#define FLASH_LEAKAGE_VERIFY_READ_MODE    FLCTL_BANK0_RDCTL_RD_MODE_5
#define FLASH_MARGIN0B_READ_MODE          FLCTL_BANK0_RDCTL_RD_MODE_9
#define FLASH_MARGIN1B_READ_MODE          FLCTL_BANK0_RDCTL_RD_MODE_10

#define FLASH_PRGBRSTCTLSTAT_BURSTSTATUS_COMPLETE FLCTL_PRGBRST_CTLSTAT_BURST_STATUS_7

#define FLASH_BANK0                 0x00
#define FLASH_BANK1                 0x01
#define FLASH_DATA_READ             0x00
#define FLASH_INSTRUCTION_FETCH     0x01

#define FLASH_MAIN_MEMORY_SPACE_BANK0     0x01
#define FLASH_MAIN_MEMORY_SPACE_BANK1     0x02
#define FLASH_INFO_MEMORY_SPACE_BANK0      0x03
#define FLASH_INFO_MEMORY_SPACE_BANK1      0x04

#define FLASH_MAIN_SPACE    FLCTL_RDBRST_CTLSTAT_MEM_TYPE_0
#define FLASH_INFO_SPACE    FLCTL_RDBRST_CTLSTAT_MEM_TYPE_1
#define FLASH_1_PATTERN FLCTL_RDBRST_CTLSTAT_DATA_CMP
#define FLASH_0_PATTERN 0x00

#define FLASH_SECTOR0               FLCTL_BANK0_MAIN_WEPROT_PROT0
#define FLASH_SECTOR1               FLCTL_BANK0_MAIN_WEPROT_PROT1
#define FLASH_SECTOR2               FLCTL_BANK0_MAIN_WEPROT_PROT2
#define FLASH_SECTOR3               FLCTL_BANK0_MAIN_WEPROT_PROT3
#define FLASH_SECTOR4               FLCTL_BANK0_MAIN_WEPROT_PROT4
#define FLASH_SECTOR5               FLCTL_BANK0_MAIN_WEPROT_PROT5
#define FLASH_SECTOR6               FLCTL_BANK0_MAIN_WEPROT_PROT6
#define FLASH_SECTOR7               FLCTL_BANK0_MAIN_WEPROT_PROT7
#define FLASH_SECTOR8               FLCTL_BANK0_MAIN_WEPROT_PROT8
#define FLASH_SECTOR9               FLCTL_BANK0_MAIN_WEPROT_PROT9
#define FLASH_SECTOR10              FLCTL_BANK0_MAIN_WEPROT_PROT10
#define FLASH_SECTOR11              FLCTL_BANK0_MAIN_WEPROT_PROT11
#define FLASH_SECTOR12              FLCTL_BANK0_MAIN_WEPROT_PROT12
#define FLASH_SECTOR13              FLCTL_BANK0_MAIN_WEPROT_PROT13
#define FLASH_SECTOR14              FLCTL_BANK0_MAIN_WEPROT_PROT14
#define FLASH_SECTOR15              FLCTL_BANK0_MAIN_WEPROT_PROT15
#define FLASH_SECTOR16              FLCTL_BANK0_MAIN_WEPROT_PROT16
#define FLASH_SECTOR17              FLCTL_BANK0_MAIN_WEPROT_PROT17
#define FLASH_SECTOR18              FLCTL_BANK0_MAIN_WEPROT_PROT18
#define FLASH_SECTOR19              FLCTL_BANK0_MAIN_WEPROT_PROT19
#define FLASH_SECTOR20              FLCTL_BANK0_MAIN_WEPROT_PROT20
#define FLASH_SECTOR21              FLCTL_BANK0_MAIN_WEPROT_PROT21
#define FLASH_SECTOR22              FLCTL_BANK0_MAIN_WEPROT_PROT22
#define FLASH_SECTOR23              FLCTL_BANK0_MAIN_WEPROT_PROT23
#define FLASH_SECTOR24              FLCTL_BANK0_MAIN_WEPROT_PROT24
#define FLASH_SECTOR25              FLCTL_BANK0_MAIN_WEPROT_PROT25
#define FLASH_SECTOR26              FLCTL_BANK0_MAIN_WEPROT_PROT26
#define FLASH_SECTOR27              FLCTL_BANK0_MAIN_WEPROT_PROT27
#define FLASH_SECTOR28              FLCTL_BANK0_MAIN_WEPROT_PROT28
#define FLASH_SECTOR29              FLCTL_BANK0_MAIN_WEPROT_PROT29
#define FLASH_SECTOR30              FLCTL_BANK0_MAIN_WEPROT_PROT30
#define FLASH_SECTOR31              FLCTL_BANK0_MAIN_WEPROT_PROT31

#define FLASH_NOVER     0
#define FLASH_BURSTPOST FLCTL_PRGBRST_CTLSTAT_AUTO_PST
#define FLASH_BURSTPRE FLCTL_PRGBRST_CTLSTAT_AUTO_PRE
#define FLASH_REGPRE FLCTL_PRG_CTLSTAT_VER_PRE
#define FLASH_REGPOST FLCTL_PRG_CTLSTAT_VER_PST
#define FLASH_FULLVER (FLCTL_PRGBRST_CTLSTAT_AUTO_PST |                        \
                    FLCTL_PRGBRST_CTLSTAT_AUTO_PRE | FLCTL_PRG_CTLSTAT_VER_PRE \
                    | FLCTL_PRG_CTLSTAT_VER_PST)

#define FLASH_COLLATED_WRITE_MODE  0x01
#define FLASH_IMMEDIATE_WRITE_MODE 0x02

#define __INFO_FLASH_TECH_START__  0x00200000
#define __INFO_FLASH_TECH_MIDDLE__ 0x00202000


//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! Calculates the flash bank and sector number given an address. Stores the
//! results into the two pointers given as parameters. The user must provide
//! a valid memory address (an address in SRAM for example will give an invalid
//! result).
//!
//! \param addr Address to calculate the bank/sector information for
//!
//! \param bankNum The bank number will be stored in here after the function
//!                     completes.
//!
//! \param sectorNum The sector number will be stored in here after the function
//!                     completes.
//!
//! \note For simplicity, this API only works with address in MAIN flash memory.
//!        For calculating the sector/bank number of an address in info memory,
//!         please refer to your device datasheet/
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_getMemoryInfo(uint32_t addr, uint32_t *bankNum,
                                   uint32_t *sectorNum);

//*****************************************************************************
//
//! Enables read buffering on accesses to a specified bank of flash memory
//!
//! \param memoryBank is the value of the memory bank to enable read
//!  buffering. Must be only one of the following values:
//!  - \b FLASH_BANK0,
//!  - \b FLASH_BANK1
//!
//! \param accessMethod is the value of the access type to enable read
//!  buffering. Must be only one of the following values:
//!  - \b FLASH_DATA_READ,
//!  - \b FLASH_INSTRUCTION_FETCH
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_enableReadBuffering(uint_fast8_t memoryBank,
        uint_fast8_t accessMethod);

//*****************************************************************************
//
//! Disables read buffering on accesses to a specified bank of flash memory
//!
//! \param memoryBank is the value of the memory bank to disable read
//!  buffering. Must be only one of the following values:
//!  - \b FLASH_BANK0,
//!  - \b FLASH_BANK1
//!
//! \param accessMethod is the value of the access type to disable read
//!  buffering. Must ne only one of the following values:
//!  - \b FLASH_DATA_READ,
//!  - \b FLASH_INSTRUCTION_FETCH
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_disableReadBuffering(uint_fast8_t memoryBank,
        uint_fast8_t accessMethod);

//*****************************************************************************
//
//! Disables program protection on the given sector mask. This setting can be
//! applied on a sector-wise bases on a given memory space (INFO or MAIN).
//!
//! \param memorySpace is the value of the memory bank to disable program
//!  protection. Must be only one of the following values:
//!  - \b FLASH_MAIN_MEMORY_SPACE_BANK0,
//!  - \b FLASH_MAIN_MEMORY_SPACE_BANK1,
//!  - \b FLASH_INFO_MEMORY_SPACE_BANK0,
//!  - \b FLASH_INFO_MEMORY_SPACE_BANK1
//!
//! \param sectorMask is a bit mask of the sectors to disable program
//!  protection. Must be a bitfield of the following values:
//!  - \b FLASH_SECTOR0,
//!  - \b FLASH_SECTOR1,
//!  - \b FLASH_SECTOR2,
//!  - \b FLASH_SECTOR3,
//!  - \b FLASH_SECTOR4,
//!  - \b FLASH_SECTOR5,
//!  - \b FLASH_SECTOR6,
//!  - \b FLASH_SECTOR7,
//!  - \b FLASH_SECTOR8,
//!  - \b FLASH_SECTOR9,
//!  - \b FLASH_SECTOR10,
//!  - \b FLASH_SECTOR11,
//!  - \b FLASH_SECTOR12,
//!  - \b FLASH_SECTOR13,
//!  - \b FLASH_SECTOR14,
//!  - \b FLASH_SECTOR15,
//!  - \b FLASH_SECTOR16,
//!  - \b FLASH_SECTOR17,
//!  - \b FLASH_SECTOR18,
//!  - \b FLASH_SECTOR19,
//!  - \b FLASH_SECTOR20,
//!  - \b FLASH_SECTOR21,
//!  - \b FLASH_SECTOR22,
//!  - \b FLASH_SECTOR23,
//!  - \b FLASH_SECTOR24,
//!  - \b FLASH_SECTOR25,
//!  - \b FLASH_SECTOR26,
//!  - \b FLASH_SECTOR27,
//!  - \b FLASH_SECTOR28,
//!  - \b FLASH_SECTOR29,
//!  - \b FLASH_SECTOR30,
//!  - \b FLASH_SECTOR31
//!
//!  \note Flash sector sizes are 4KB and the number of sectors may vary
//!  depending on the specific device. Also, for INFO memory space, only sectors
//!  \b FLASH_SECTOR0 and \b FLASH_SECTOR1 will exist.
//!
//! \note Not all devices will contain a dedicated INFO memory. Please check the
//!  device datasheet to see if your device has INFO memory available for use.
//!  For devices without INFO memory, any operation related to the INFO memory
//!  will be ignored by the hardware.
//!
//! \return true if sector protection disabled false otherwise.
//
//*****************************************************************************
extern bool FlashCtl_unprotectSector(uint_fast8_t memorySpace,
        uint32_t sectorMask);

//*****************************************************************************
//
//! Enables program protection on the given sector mask. This setting can be
//! applied on a sector-wise bases on a given memory space (INFO or MAIN).
//!
//! \param memorySpace is the value of the memory bank to enable program
//!  protection. Must be only one of the following values:
//!  - \b FLASH_MAIN_MEMORY_SPACE_BANK0,
//!  - \b FLASH_MAIN_MEMORY_SPACE_BANK1,
//!  - \b FLASH_INFO_MEMORY_SPACE_BANK0,
//!  - \b FLASH_INFO_MEMORY_SPACE_BANK1
//!
//! \param sectorMask is a bit mask of the sectors to enable program
//!  protection. Must be a bitfield of the following values:
//!  - \b FLASH_SECTOR0,
//!  - \b FLASH_SECTOR1,
//!  - \b FLASH_SECTOR2,
//!  - \b FLASH_SECTOR3,
//!  - \b FLASH_SECTOR4,
//!  - \b FLASH_SECTOR5,
//!  - \b FLASH_SECTOR6,
//!  - \b FLASH_SECTOR7,
//!  - \b FLASH_SECTOR8,
//!  - \b FLASH_SECTOR9,
//!  - \b FLASH_SECTOR10,
//!  - \b FLASH_SECTOR11,
//!  - \b FLASH_SECTOR12,
//!  - \b FLASH_SECTOR13,
//!  - \b FLASH_SECTOR14,
//!  - \b FLASH_SECTOR15,
//!  - \b FLASH_SECTOR16,
//!  - \b FLASH_SECTOR17,
//!  - \b FLASH_SECTOR18,
//!  - \b FLASH_SECTOR19,
//!  - \b FLASH_SECTOR20,
//!  - \b FLASH_SECTOR21,
//!  - \b FLASH_SECTOR22,
//!  - \b FLASH_SECTOR23,
//!  - \b FLASH_SECTOR24,
//!  - \b FLASH_SECTOR25,
//!  - \b FLASH_SECTOR26,
//!  - \b FLASH_SECTOR27,
//!  - \b FLASH_SECTOR28,
//!  - \b FLASH_SECTOR29,
//!  - \b FLASH_SECTOR30,
//!  - \b FLASH_SECTOR31
//!
//!  \note Flash sector sizes are 4KB and the number of sectors may vary
//!  depending on the specific device. Also, for INFO memory space, only sectors
//!  \b FLASH_SECTOR0 and \b FLASH_SECTOR1 will exist.
//!
//! \note Not all devices will contain a dedicated INFO memory. Please check the
//!  device datasheet to see if your device has INFO memory available for use.
//!  For devices without INFO memory, any operation related to the INFO memory
//!  will be ignored by the hardware.
//!
//! \return true if sector protection enabled false otherwise.
//
//*****************************************************************************
extern bool FlashCtl_protectSector(uint_fast8_t memorySpace,
        uint32_t sectorMask);

//*****************************************************************************
//
//! Returns the sector protection for given sector mask and memory space
//!
//! \param memorySpace is the value of the memory bank to check for program
//!  protection. Must be only one of the following values:
//!  - \b FLASH_MAIN_MEMORY_SPACE_BANK0,
//!  - \b FLASH_MAIN_MEMORY_SPACE_BANK1,
//!  - \b FLASH_INFO_MEMORY_SPACE_BANK0,
//!  - \b FLASH_INFO_MEMORY_SPACE_BANK1
//!
//! \param sector is the sector to check for program protection.
//!  Must be one of the following values:
//!  - \b FLASH_SECTOR0,
//!  - \b FLASH_SECTOR1,
//!  - \b FLASH_SECTOR2,
//!  - \b FLASH_SECTOR3,
//!  - \b FLASH_SECTOR4,
//!  - \b FLASH_SECTOR5,
//!  - \b FLASH_SECTOR6,
//!  - \b FLASH_SECTOR7,
//!  - \b FLASH_SECTOR8,
//!  - \b FLASH_SECTOR9,
//!  - \b FLASH_SECTOR10,
//!  - \b FLASH_SECTOR11,
//!  - \b FLASH_SECTOR12,
//!  - \b FLASH_SECTOR13,
//!  - \b FLASH_SECTOR14,
//!  - \b FLASH_SECTOR15,
//!  - \b FLASH_SECTOR16,
//!  - \b FLASH_SECTOR17,
//!  - \b FLASH_SECTOR18,
//!  - \b FLASH_SECTOR19,
//!  - \b FLASH_SECTOR20,
//!  - \b FLASH_SECTOR21,
//!  - \b FLASH_SECTOR22,
//!  - \b FLASH_SECTOR23,
//!  - \b FLASH_SECTOR24,
//!  - \b FLASH_SECTOR25,
//!  - \b FLASH_SECTOR26,
//!  - \b FLASH_SECTOR27,
//!  - \b FLASH_SECTOR28,
//!  - \b FLASH_SECTOR29,
//!  - \b FLASH_SECTOR30,
//!  - \b FLASH_SECTOR31
//!
//!  Note that flash sector sizes are 4KB and the number of sectors may vary
//!  depending on the specific device. Also, for INFO memory space, only sectors
//!  FLASH_SECTOR0 and FLASH_SECTOR1 will exist.
//!
//! \note Not all devices will contain a dedicated INFO memory. Please check the
//!  device datasheet to see if your device has INFO memory available for use.
//!  For devices without INFO memory, any operation related to the INFO memory
//!  will be ignored by the hardware.
//!
//! \return true if sector protection enabled false otherwise.
//
//*****************************************************************************
extern bool FlashCtl_isSectorProtected(uint_fast8_t memorySpace,
        uint32_t sector);

//*****************************************************************************
//
//! Verifies a given segment of memory based off either a high (1) or low (0)
//! state.
//!
//! \param verifyAddr Start address where verification will begin
//!
//! \param length Length in bytes to verify based off the pattern
//!
//! \param pattern The pattern which verification will check versus. This can
//!  either be a low pattern (each register will be checked versus a pattern
//!  of 32 zeros, or a high pattern (each register will be checked versus a
//!  pattern of 32 ones). Valid values are: FLASH_0_PATTERN, FLASH_1_PATTERN
//!
//!  \note There are no sector/boundary restrictions for this function,
//!  however it is encouraged to proved a start address aligned on 32-bit
//!  boundaries.  Providing an unaligned address will result in unaligned data
//!  accesses and detriment efficiency.
//!
//!  \note This function is blocking and will not exit until operation has
//!  either completed or failed due to an error. Furthermore, given the
//!  complex verification requirements of the flash controller, master
//!  interrupts are disabled throughout execution of this function. The original
//!  interrupt context is saved at the start of execution and restored prior
//!  to exit of the API.
//!
//!  \note Due to the hardware limitations of the flash controller, this
//!  function cannot verify a memory address in the same flash bank that it
//!  is executing from. If using the ROM version of this API (by using the
//!  (ROM_ or MAP_ prefixes) this is a don't care, however if this API resides
//!  in flash then special care needs to be taken to ensure no code execution
//!  or reads happen in the flash bank being programmed while this API is
//!  being executed.
//!
//! \return true if memory verification is successful, false otherwise.
//
//*****************************************************************************
extern bool FlashCtl_verifyMemory(void* verifyAddr, uint32_t length,
        uint_fast8_t pattern);

//*****************************************************************************
//
//!  Performs a mass erase on all unprotected flash sectors. Protected sectors
//!  are ignored.
//!
//!  \note This function is blocking and will not exit until operation has
//!  either completed or failed due to an error. Furthermore, given the
//!  complex verification requirements of the flash controller, master
//!  interrupts are disabled throughout execution of this function. The original
//!  interrupt context is saved at the start of execution and restored prior
//!  to exit of the API.
//!
//!  \note Due to the hardware limitations of the flash controller, this
//!  function cannot erase a memory address in the same flash bank that it
//!  is executing from. If using the ROM version of this API (by using the
//!  (ROM_ or MAP_ prefixes) this is a don't care, however if this API resides
//!  in flash then special care needs to be taken to ensure no code execution
//!  or reads happen in the flash bank being programmed while this API is
//!  being executed.
//!
//! \return true if mass erase completes successfully, false otherwise
//
//*****************************************************************************
extern bool FlashCtl_performMassErase(void);

//*****************************************************************************
//
//!  Initiates a mass erase and returns control back to the program. This is a
//!  non-blocking function, however it is the user's responsibility to perform
//!  the necessary verification requirements after the interrupt is set to
//!  signify completion.
//!
//! \return None
//
//*****************************************************************************
extern void FlashCtl_initiateMassErase(void);

//*****************************************************************************
//
//! Erases a sector of MAIN or INFO flash memory.
//!
//! \param addr The start of the sector to erase. Note that with flash,
//!         the minimum allowed size that can be erased is a flash sector
//!         (which is 4KB on the MSP432 family). If an address is provided to
//!         this function which is not on a 4KB boundary, the entire sector
//!         will still be erased.
//!
//!  \note This function is blocking and will not exit until operation has
//!  either completed or failed due to an error. Furthermore, given the
//!  complex verification requirements of the flash controller, master
//!  interrupts are disabled throughout execution of this function. The original
//!  interrupt context is saved at the start of execution and restored prior
//!  to exit of the API.
//!
//!  \note Due to the hardware limitations of the flash controller, this
//!  function cannot erase a memory addressin the same flash bank that it
//!  is executing from. If using the ROM version of this API (by using the
//!  (ROM_ or MAP_ prefixes) this is a don't care, however if this API resides
//!  in flash then special care needs to be taken to ensure no code execution
//!  or reads happen in the flash bank being programmed while this API is
//!  being executed.
//!
//! \return true if sector erase is successful, false otherwise.
//
//*****************************************************************************
extern bool FlashCtl_eraseSector(uint32_t addr);

//*****************************************************************************
//
//! Program a portion of flash memory with the provided data
//!
//! \param src Pointer to the data source to program into flash
//!
//! \param dest Pointer to the destination in flash to program
//!
//! \param length Length in bytes to program
//!
//!  \note There are no sector/boundary restrictions for this function,
//!  however it is encouraged to proved a start address aligned on 32-bit
//!  boundaries.  Providing an unaligned address will result in unaligned data
//!  accesses and detriment efficiency.
//!
//!  \note This function is blocking and will not exit until operation has
//!  either completed or failed due to an error. Furthermore, given the
//!  complex verification requirements of the flash controller, master
//!  interrupts are disabled throughout execution of this function. The original
//!  interrupt context is saved at the start of execution and restored prior
//!  to exit of the API.
//!
//!  \note Due to the hardware limitations of the flash controller, this
//!  function cannot program a memory addressin the same flash bank that it
//!  is executing from. If using the ROM version of this API (by using the
//!  (ROM_ or MAP_ prefixes) this is a don't care, however if this API resides
//!  in flash then special care needs to be taken to ensure no code execution
//!  or reads happen in the flash bank being programmed while this API is
//!  being executed.
//!
//! \return Whether or not the program succeeded
//
//*****************************************************************************
extern bool FlashCtl_programMemory(void* src, void* dest, uint32_t length);

//*****************************************************************************
//
//! Setups pre/post verification of burst and regular flash programming
//! instructions. Note that this API is for advanced users that are programming
//! their own flash drivers. The program/erase APIs are not affected by this
//! setting and take care of the verification requirements.
//!
//! \param verificationSetting Verification setting to set. This value can
//!  be a bitwise OR of the following values:
//!     - \b FLASH_BURSTPOST,
//!     - \b FLASH_BURSTPRE,
//!     - \b FLASH_REGPRE,
//!     - \b FLASH_REGPOST
//!     - \b FLASH_NOVER No verification enabled
//!     - \b FLASH_FULLVER Full verification enabled
//!
//! \return none
//
//*****************************************************************************
extern void FlashCtl_setProgramVerification(uint32_t verificationSetting);

//*****************************************************************************
//
//! Clears pre/post verification of burst and regular flash programming
//! instructions. Note that this API is for advanced users that are programming
//! their own flash drivers. The program/erase APIs are not affected by this
//! setting and take care of the verification requirements.
//!
//! \param verificationSetting Verification setting to clear. This value can
//!  be a bitwise OR of the following values:
//!     - \b FLASH_BURSTPOST,
//!     - \b FLASH_BURSTPRE,
//!     - \b FLASH_REGPRE,
//!     - \b FLASH_REGPOST
//!     - \b FLASH_NOVER No verification enabled
//!     - \b FLASH_FULLVER Full verification enabled
//!
//! \return none
//
//*****************************************************************************
extern void FlashCtl_clearProgramVerification(uint32_t verificationSetting);

//*****************************************************************************
//
//! Enables  word programming of flash memory.
//!
//! This function will enable word programming of the flash memory and set the
//! mode of behavior when the flash write occurs.
//!
//! \param mode The mode specifies the behavior of the flash controller when
//!        programming words to flash. In \b FLASH_IMMEDIATE_WRITE_MODE, the
//!        program operation happens immediately on the write to flash while
//!        in \b FLASH_COLLATED_WRITE_MODE the write will be delayed until a full
//!        128-bits have been collated. Possible values include:
//!             - \b FLASH_IMMEDIATE_WRITE_MODE
//!             - \b FLASH_COLLATED_WRITE_MODE
//!
//!
//! Refer to the user's guide for further documentation.
//!
//! \return none
//
//*****************************************************************************
extern void FlashCtl_enableWordProgramming(uint32_t mode);

//*****************************************************************************
//
//! Disables  word programming of flash memory.
//!
//! Refer to FlashCtl_enableWordProgramming and the user's guide for description
//! on the difference between full word and immediate programming
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_disableWordProgramming(void);

//*****************************************************************************
//
//! Returns if word programming mode is enabled (and if it is, the specific mode)
//!
//! Refer to FlashCtl_enableWordProgramming and the user's guide for description
//! on the difference between full word and immediate programming
//!
//! \return a zero value if word programming is disabled,
//!             - \b FLASH_IMMEDIATE_WRITE_MODE
//!             - \b FLASH_COLLATED_WRITE_MODE
//!
//
//*****************************************************************************
extern uint32_t FlashCtl_isWordProgrammingEnabled(void);

//*****************************************************************************
//
//!  Sets the flash read mode to be used by default flash read operations.
//!  Note that the proper wait states must be set prior to entering this
//!   function.
//!
//! \param flashBank Flash bank to set read mode for. Valid values are:
//!         - \b FLASH_BANK0
//!         - \b FLASH_BANK1
//!
//! \param readMode The read mode to set. Valid values are:
//!  - \b FLASH_NORMAL_READ_MODE,
//!  - \b FLASH_MARGIN0_READ_MODE,
//!  - \b FLASH_MARGIN1_READ_MODE,
//!  - \b FLASH_PROGRAM_VERIFY_READ_MODE,
//!  - \b FLASH_ERASE_VERIFY_READ_MODE,
//!  - \b FLASH_LEAKAGE_VERIFY_READ_MODE,
//!  - \b FLASH_MARGIN0B_READ_MODE,
//!  - \b FLASH_MARGIN1B_READ_MODE
//!
//! \return None.
//
//*****************************************************************************
extern bool FlashCtl_setReadMode(uint32_t flashBank, uint32_t readMode);

//*****************************************************************************
//
//!  Gets the flash read mode to be used by default flash read operations.
//!
//! \param flashBank Flash bank to set read mode for. Valid values are:
//!         - \b FLASH_BANK0
//!         - \b FLASH_BANK1
//!
//! \return Returns the read mode to set. Valid values are:
//!  - \b FLASH_NORMAL_READ_MODE,
//!  - \b FLASH_MARGIN0_READ_MODE,
//!  - \b FLASH_MARGIN1_READ_MODE,
//!  - \b FLASH_PROGRAM_VERIFY_READ_MODE,
//!  - \b FLASH_ERASE_VERIFY_READ_MODE,
//!  - \b FLASH_LEAKAGE_VERIFY_READ_MODE,
//!  - \b FLASH_MARGIN0B_READ_MODE,
//!  - \b FLASH_MARGIN1B_READ_MODE
//!
//
//*****************************************************************************
extern uint32_t FlashCtl_getReadMode(uint32_t flashBank);

//*****************************************************************************
//
//! Changes the number of wait states that are used by the flash controller
//! for read operations. When changing frequency ranges of the clock, this
//! functions must be used in order to allow for readable flash memory.
//!
//! \param waitState The number of wait states to set. Note that only
//!     bits 0-3 are used.
//!
//! \param flashBank Flash bank to set wait state for. Valid values are:
//!         - \b FLASH_BANK0
//!         - \b FLASH_BANK1
//!
//
//*****************************************************************************
extern void FlashCtl_setWaitState(uint32_t bank, uint32_t waitState);

//*****************************************************************************
//
//! Returns the set number of flash wait states for the given flash bank.
//!
//! \param flashBank Flash bank to set wait state for. Valid values are:
//!         - \b FLASH_BANK0
//!         - \b FLASH_BANK1
//!
//! \return The wait state setting for the specified flash bank
//
//*****************************************************************************
extern uint32_t FlashCtl_getWaitState(uint32_t bank);

//*****************************************************************************
//
//! Enables individual flash control interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be enabled.  Must
//! be a logical OR of:
//!         - \b FLASH_PROGRAM_ERROR,
//!         - \b FLASH_BENCHMARK_INT,
//!         - \b FLASH_ERASE_COMPLETE,
//!         - \b FLASH_BRSTPRGM_COMPLETE,
//!         - \b FLASH_WRDPRGM_COMPLETE,
//!         - \b FLASH_POSTVERIFY_FAILED,
//!         - \b FLASH_PREVERIFY_FAILED,
//!         - \b FLASH_BRSTRDCMP_COMPLETE
//!
//! This function enables the indicated flash system interrupt sources.  Only
//! the sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_enableInterrupt(uint32_t flags);

//*****************************************************************************
//
//! Disables individual flash system interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be disabled.  Must
//! be a logical OR of:
//!         - \b FLASH_PROGRAM_ERROR,
//!         - \b FLASH_BENCHMARK_INT,
//!         - \b FLASH_ERASE_COMPLETE,
//!         - \b FLASH_BRSTPRGM_COMPLETE,
//!         - \b FLASH_WRDPRGM_COMPLETE,
//!         - \b FLASH_POSTVERIFY_FAILED,
//!         - \b FLASH_PREVERIFY_FAILED,
//!         - \b FLASH_BRSTRDCMP_COMPLETE
//!
//! This function disables the indicated flash system interrupt sources.
//! Only the sources that are enabled can be reflected to the processor
//! interrupt; disabled sources have no effect on the processor.
//!
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_disableInterrupt(uint32_t flags);

//*****************************************************************************
//
//! Gets the current interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list
//! of pending interrupts that are actually enabled and could have caused the
//! ISR.
//!
//! \return The current interrupt status, enumerated as a bit field of
//!         - \b FLASH_PROGRAM_ERROR,
//!         - \b FLASH_BENCHMARK_INT,
//!         - \b FLASH_ERASE_COMPLETE,
//!         - \b FLASH_BRSTPRGM_COMPLETE,
//!         - \b FLASH_WRDPRGM_COMPLETE,
//!         - \b FLASH_POSTVERIFY_FAILED,
//!         - \b FLASH_PREVERIFY_FAILED,
//!         - \b FLASH_BRSTRDCMP_COMPLETE
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//
//*****************************************************************************
extern uint32_t FlashCtl_getEnabledInterruptStatus(void);

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \return The current interrupt status, enumerated as a bit field of:
//!         - \b FLASH_PROGRAM_ERROR,
//!         - \b FLASH_BENCHMARK_INT,
//!         - \b FLASH_ERASE_COMPLETE,
//!         - \b FLASH_BRSTPRGM_COMPLETE,
//!         - \b FLASH_WRDPRGM_COMPLETE,
//!         - \b FLASH_POSTVERIFY_FAILED,
//!         - \b FLASH_PREVERIFY_FAILED,
//!         - \b FLASH_BRSTRDCMP_COMPLETE
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//
//*****************************************************************************
extern uint32_t FlashCtl_getInterruptStatus(void);

//*****************************************************************************
//
//! Clears flash system interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be cleared.  Must
//! be a logical OR of:
//!         - \b FLASH_PROGRAM_ERROR,
//!         - \b FLASH_BENCHMARK_INT,
//!         - \b FLASH_ERASE_COMPLETE,
//!         - \b FLASH_BRSTPRGM_COMPLETE,
//!         - \b FLASH_WRDPRGM_COMPLETE,
//!         - \b FLASH_POSTVERIFY_FAILED,
//!         - \b FLASH_PREVERIFY_FAILED,
//!         - \b FLASH_BRSTRDCMP_COMPLETE
//!
//! The specified flash system interrupt sources are cleared, so that they no
//! longer assert.  This function must be called in the interrupt handler to
//! keep it from being called again immediately upon exit.
//!
//! \note Because there is a write buffer in the Cortex-M processor, it may
//! take several clock cycles before the interrupt source is actually cleared.
//! Therefore, it is recommended that the interrupt source be cleared early in
//! the interrupt handler (as opposed to the very last action) to avoid
//! returning from the interrupt handler before the interrupt source is
//! actually cleared.  Failure to do so may result in the interrupt handler
//! being immediately reentered (because the interrupt controller still sees
//! the interrupt source asserted).
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_clearInterruptFlag(uint32_t flags);

//*****************************************************************************
//
//! Registers an interrupt handler for flash clock system interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the clock
//! system interrupt occurs.
//!
//! This function registers the handler to be called when a clock system
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific flash controller interrupts must be enabled
//! via FlashCtl_enableInterrupt().  It is the interrupt handler's
//! responsibility to clear the interrupt source via
//! FlashCtl_clearInterruptFlag().
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the flash system.
//!
//! This function unregisters the handler to be called when a clock system
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void FlashCtl_unregisterInterrupt(void);


//*****************************************************************************
//
//! Initiates a sector erase of MAIN or INFO flash memory. Note that this
//! function simply initaites the sector erase, but does no verification
//! which is required by the flash controller. The user must manually set
//! and enable interrupts on the flash controller to fire on erase completion
//! and then use the FlashCtl_verifyMemory function to verify that the sector
//! was actually erased
//!
//! \param addr The start of the sector to erase. Note that with flash,
//!         the minimum allowed size that can be erased is a flash sector
//!         (which is 4KB on the MSP432 family). If an address is provided to
//!         this function which is not on a 4KB boundary, the entire sector
//!         will still be erased.
//!
//! \return None
//
//*****************************************************************************
extern void FlashCtl_initiateSectorErase(uint32_t addr);


/* The following functions are advanced functions that are used by the flash
 * driver to remask a failed bit in the event of a post or pre verification
 * failure. They are meant to be advanced functions and should not be used
 * by the majority of users (unless you are writing your own flash driver).
 */
extern uint8_t __FlashCtl_remaskData8Post(uint8_t data, uint32_t addr);
extern uint8_t __FlashCtl_remaskData8Pre(uint8_t data, uint32_t addr);
extern uint32_t __FlashCtl_remaskData32Post(uint32_t data, uint32_t addr);
extern uint32_t __FlashCtl_remaskData32Pre(uint32_t data, uint32_t addr);
extern void __FlashCtl_remaskBurstDataPost(uint32_t addr, uint32_t size);
extern void __FlashCtl_remaskBurstDataPre(uint32_t addr, uint32_t size);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif /* __MCU_HAS_FLCTL__ */

#endif // __FLASH_H__
