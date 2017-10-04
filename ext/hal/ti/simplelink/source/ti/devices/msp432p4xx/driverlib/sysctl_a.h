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
#ifndef __SYSCTL_A_H__
#define __SYSCTL_A_H__

#include <stdint.h>
#include <stdbool.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

/* Define to ensure that our current MSP432 has the SYSCTL_A module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_SYSCTL_A__

//*****************************************************************************
//
//! \addtogroup sysctl_a_api
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
#define SYSCTL_A_HARD_RESET 1
#define SYSCTL_A_SOFT_RESET 0

#define SYSCTL_A_PERIPH_LCD SYSCTL_A_PERIHALT_CTL_HALT_LCD
#define SYSCTL_A_PERIPH_DMA SYSCTL_A_PERIHALT_CTL_HALT_DMA
#define SYSCTL_A_PERIPH_WDT SYSCTL_A_PERIHALT_CTL_HALT_WDT
#define SYSCTL_A_PERIPH_ADC SYSCTL_A_PERIHALT_CTL_HALT_ADC
#define SYSCTL_A_PERIPH_EUSCIB3 SYSCTL_A_PERIHALT_CTL_HALT_EUB3
#define SYSCTL_A_PERIPH_EUSCIB2 SYSCTL_A_PERIHALT_CTL_HALT_EUB2
#define SYSCTL_A_PERIPH_EUSCIB1 SYSCTL_A_PERIHALT_CTL_HALT_EUB1
#define SYSCTL_A_PERIPH_EUSCIB0 SYSCTL_A_PERIHALT_CTL_HALT_EUB0
#define SYSCTL_A_PERIPH_EUSCIA3 SYSCTL_A_PERIHALT_CTL_HALT_EUA3
#define SYSCTL_A_PERIPH_EUSCIA2 SYSCTL_A_PERIHALT_CTL_HALT_EUA2
#define SYSCTL_A_PERIPH_EUSCIA1 SYSCTL_A_PERIHALT_CTL_HALT_EUA1
#define SYSCTL_A_PERIPH_EUSCIA0 SYSCTL_A_PERIHALT_CTL_HALT_EUA0
#define SYSCTL_A_PERIPH_TIMER32_0_MODULE SYSCTL_A_PERIHALT_CTL_HALT_T32_0
#define SYSCTL_A_PERIPH_TIMER16_3 SYSCTL_A_PERIHALT_CTL_HALT_T16_3
#define SYSCTL_A_PERIPH_TIMER16_2 SYSCTL_A_PERIHALT_CTL_HALT_T16_2
#define SYSCTL_A_PERIPH_TIMER16_1 SYSCTL_A_PERIHALT_CTL_HALT_T16_1
#define SYSCTL_A_PERIPH_TIMER16_0 SYSCTL_A_PERIHALT_CTL_HALT_T16_0

#define SYSCTL_A_NMIPIN_SRC SYSCTL_A_NMI_CTLSTAT_PIN_SRC
#define SYSCTL_A_PCM_SRC SYSCTL_A_NMI_CTLSTAT_PCM_SRC
#define SYSCTL_A_PSS_SRC SYSCTL_A_NMI_CTLSTAT_PSS_SRC
#define SYSCTL_A_CS_SRC SYSCTL_A_NMI_CTLSTAT_CS_SRC

#define SYSCTL_A_REBOOT_KEY   0x6900

#define SYSCTL_A_1_2V_REF        (uint32_t)&TLV->ADC14_REF1P2V_TS30C - (uint32_t)TLV_BASE
#define SYSCTL_A_1_45V_REF       (uint32_t)&TLV->ADC14_REF1P45V_TS30C - (uint32_t)TLV_BASE
#define SYSCTL_A_2_5V_REF        (uint32_t)&TLV->ADC14_REF2P5V_TS30C - (uint32_t)TLV_BASE

#define SYSCTL_A_85_DEGREES_C    4
#define SYSCTL_A_30_DEGREES_C    0

#define SYSCTL_A_BANKMASK       0x80000000
#define SRAMCTL_CTL0_BANK       0x10000000
#define SRAMCTL_CTL1_BANK       0x20000000
#define SRAMCTL_CTL2_BANK       0x30000000
#define SRAMCTL_CTL3_BANK       0x40000000


#define TLV_START               0x00201004
#define TLV_TAG_RESERVED1      1
#define TLV_TAG_RESERVED2      2
#define TLV_TAG_CS             3
#define TLV_TAG_FLASHCTL       4
#define TLV_TAG_ADC14          5
#define TLV_TAG_RESERVED6      6
#define TLV_TAG_RESERVED7      7
#define TLV_TAG_REF            8
#define TLV_TAG_RESERVED9      9
#define TLV_TAG_RESERVED10     10
#define TLV_TAG_DEVINFO        11
#define TLV_TAG_DIEREC         12
#define TLV_TAG_RANDNUM        13
#define TLV_TAG_RESERVED14     14
#define TLV_TAG_BSL            15
#define TLV_TAGEND             0x0BD0E11D

//*****************************************************************************
//
// Structures for TLV definitions
//
//*****************************************************************************
typedef struct
{
    uint32_t    maxProgramPulses;
    uint32_t    maxErasePulses;
} SysCtl_A_FlashTLV_Info;

typedef struct
{
    uint32_t rDCOIR_FCAL_RSEL04;
    uint32_t rDCOIR_FCAL_RSEL5;
    uint32_t rDCOIR_MAXPOSTUNE_RSEL04;
    uint32_t rDCOIR_MAXNEGTUNE_RSEL04;
    uint32_t rDCOIR_MAXPOSTUNE_RSEL5;
    uint32_t rDCOIR_MAXNEGTUNE_RSEL5;
    uint32_t rDCOIR_CONSTK_RSEL04;
    uint32_t rDCOIR_CONSTK_RSEL5;
    uint32_t rDCOER_FCAL_RSEL04;
    uint32_t rDCOER_FCAL_RSEL5;
    uint32_t rDCOER_MAXPOSTUNE_RSEL04;
    uint32_t rDCOER_MAXNEGTUNE_RSEL04;
    uint32_t rDCOER_MAXPOSTUNE_RSEL5;
    uint32_t rDCOER_MAXNEGTUNE_RSEL5;
    uint32_t rDCOER_CONSTK_RSEL04;
    uint32_t rDCOER_CONSTK_RSEL5;

} SysCtl_A_CSCalTLV_Info;

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! Gets the size of the SRAM.
//!
//! \return The total number of bytes of SRAM.
//
//*****************************************************************************
extern uint_least32_t SysCtl_A_getSRAMSize(void);

//*****************************************************************************
//
//! Gets the size of the flash.
//!
//! \return The total number of bytes of main flash memory.
//!
//! \note This returns the total amount of main memory flash. To find how much
//!        INFO memory is available, use the \link SysCtl_A_getInfoFlashSize
//!         \endlink function.
//
//*****************************************************************************
extern uint_least32_t SysCtl_A_getFlashSize(void);

//*****************************************************************************
//
//! Gets the size of the flash.
//!
//! \return The total number of bytes of flash of INFO flash memory.
//!
//! \note This returns the total amount of INFO memory flash. To find how much
//!        main  memory is available, use the \link SysCtl_A_getFlashSize
//!         \endlink function.
//
//*****************************************************************************
extern uint_least32_t SysCtl_A_getInfoFlashSize(void);

//*****************************************************************************
//
//! Reboots the device and causes the device to re-initialize itself.
//!
//! \return This function does not return.
//
//*****************************************************************************
extern void SysCtl_A_rebootDevice(void);

//*****************************************************************************
//
//! The TLV structure uses a tag or base address to identify segments of the
//! table where information is stored. Some examples of TLV tags are Peripheral
//! Descriptor, Interrupts, Info Block and Die Record. This function retrieves
//! the value of a tag and the length of the tag.
//!
//! \param tag represents the tag for which the information needs to be
//!        retrieved.
//!        Valid values are:
//!        - \b TLV_TAG_RESERVED1
//!        - \b TLV_TAG_RESERVED2
//!        - \b TLV_TAG_CS
//!        - \b TLV_TAG_FLASHCTL
//!        - \b TLV_TAG_ADC14
//!        - \b TLV_TAG_RESERVED6
//!        - \b TLV_TAG_RESERVED7
//!        - \b TLV_TAG_REF
//!        - \b TLV_TAG_RESERVED9
//!        - \b TLV_TAG_RESERVED10
//!        - \b TLV_TAG_DEVINFO
//!        - \b TLV_TAG_DIEREC
//!        - \b TLV_TAG_RANDNUM
//!        - \b TLV_TAG_RESERVED14
//! \param instance In some cases a specific tag may have more than one
//!        instance. For example there may be multiple instances of timer
//!        calibration data present under a single Timer Cal tag. This variable
//!        specifies the instance for which information is to be retrieved (0,
//!        1, etc.). When only one instance exists; 0 is passed.
//! \param length Acts as a return through indirect reference. The function
//!        retrieves the value of the TLV tag length. This value is pointed to
//!        by *length and can be used by the application level once the
//!        function is called. If the specified tag is not found then the
//!        pointer is null 0.
//! \param data_address acts as a return through indirect reference. Once the
//!        function is called data_address points to the pointer that holds the
//!        value retrieved from the specified TLV tag. If the specified tag is
//!        not found then the pointer is null 0.
//!
//! \return None
//
//*****************************************************************************
extern void SysCtl_A_getTLVInfo(uint_fast8_t tag, uint_fast8_t instance,
        uint_fast8_t *length, uint32_t **data_address);

//*****************************************************************************
//
//! Enables areas of SRAM memory. This can be used to optimize power
//! consumption when every SRAM bank isn't needed.
//! This function takes in a 32-bit address to the area in SRAM to to enable.
//! It will convert this address into the corresponding register settings and
//! set them in the register accordingly. Note that passing an address to an
//! area other than SRAM will result in unreliable behavior. Addresses should
//! be given with reference to the SRAM_DATA area of SRAM (usually starting at
//! 0x20000000).
//!
//! \param addr Break address of SRAM to enable. All SRAM below this address
//!     will also be enabled. If an unaligned address is given the appropriate
//!     aligned address will be calculated.
//!
//! \note The first bank of SRAM is reserved and always enabled.
//!
//! \return true if banks were set, false otherwise. If the BNKEN_RDY bit is
//!               not set in the STAT register, this function will return false.
//
//*****************************************************************************
extern bool SysCtl_A_enableSRAM(uint32_t addr);

//*****************************************************************************
//
//! Disables areas of SRAM memory. This can be used to optimize power
//! consumption when every SRAM bank isn't needed. It is important to note
//! that when a  higher bank is disabled, all of the SRAM banks above that bank
//! are also disabled. For example, if the address of 0x2001FA0 is given, all
//! SRAM banks from 0x2001FA0 to the top of SRAM will be disabled.
//! This function takes in a 32-bit address to the area in SRAM to to disable.
//! It will convert this address into the corresponding register settings and
//! set them in the register accordingly. Note that passing an address to an
//! area other than SRAM will result in unreliable behavior. Addresses should
//! be given with reference to the SRAM_DATA area of SRAM (usually starting at
//! 0x20000000).
//!
//! \param addr Break address of SRAM to disable. All SRAM above this address
//!     will also be disabled. If an unaligned address is given the appropriate
//!     aligned address will be calculated.
//!
//! \note The first bank of SRAM is reserved and always enabled.
//!
//! \return true if banks were set, false otherwise. If the BNKEN_RDY bit is
//!               not set in the STAT register, this function will return false.
//
//*****************************************************************************
extern bool SysCtl_A_disableSRAM(uint32_t addr);

//*****************************************************************************
//
//! Enables retention of the specified SRAM block address range when the device
//! goes into LPM3 mode. When the system is placed in LPM3 mode, the SRAM
//! banks specified with this function will be placed into retention mode.
//! Retention of individual blocks can be set without the restrictions of the
//! enable/disable functions. Note that any memory range given outside of SRAM
//! will result in unreliable behavior. Also note that any unaligned addresses
//! will be truncated to the closest aligned address before the address given.
//! Addresses should be given with reference to the SRAM_DATA area of SRAM
//! (usually starting at 0x20000000).
//!
//! \param startAddr Start address to enable retention
//!
//! \param endtAddr End address to enable retention
//!
//! \note  Block 0 is reserved and retention is always enabled.
//!
//! \return true if banks were set, false otherwise. If the BLKEN_RDY bit is
//!               not set in the STAT register, this function will return false.
//
//*****************************************************************************
extern bool SysCtl_A_enableSRAMRetention(uint32_t startAddr,
        uint32_t endAddr);

//*****************************************************************************
//
//! Disables retention of the specified SRAM block address range when the device
//! goes into LPM3 mode. When the system is placed in LPM3 mode, the SRAM
//! banks specified with this function will be placed into retention mode.
//! Retention of individual blocks can be set without the restrictions of the
//! enable/disable functions. Note that any memory range given outside of SRAM
//! will result in unreliable behavior. Also note that any unaligned addresses
//! will be truncated to the closest aligned address before the address given.
//! Addresses should be given with reference to the SRAM_DATA area of SRAM
//! (usually starting at 0x20000000).
//!
//! \param startAddr Start address to disable retention
//!
//! \param endtAddr End address to disable retention
//!
//! \note  Block 0 is reserved and retention is always enabled.
//!
//! \return true if banks were set, false otherwise. If the BLKEN_RDY bit is
//!               not set in the STAT register, this function will return false.
//
//*****************************************************************************
extern bool SysCtl_A_disableSRAMRetention(uint32_t startAddr,
        uint32_t endAddr);

//*****************************************************************************
//
//! Makes it so that the provided peripherals will either halt execution after
//! a CPU HALT. Parameters in this function can be combined to account for
//! multiple peripherals. By default, all peripherals keep running after a
//! CPU HALT.
//!
//! \param devices The peripherals to continue running after a CPU HALT
//!         This can be a bitwise OR of the following values:
//!                 - \b SYSCTL_A_PERIPH_LCD,
//!                 - \b SYSCTL_A_PERIPH_DMA,
//!                 - \b SYSCTL_A_PERIPH_WDT,
//!                 - \b SYSCTL_A_PERIPH_ADC,
//!                 - \b SYSCTL_A_PERIPH_EUSCIB3,
//!                 - \b SYSCTL_A_PERIPH_EUSCIB2,
//!                 - \b SYSCTL_A_PERIPH_EUSCIB1
//!                 - \b SYSCTL_A_PERIPH_EUSCIB0,
//!                 - \b SYSCTL_A_PERIPH_EUSCIA3,
//!                 - \b SYSCTL_A_PERIPH_EUSCIA2
//!                 - \b SYSCTL_A_PERIPH_EUSCIA1,
//!                 - \b SYSCTL_A_PERIPH_EUSCIA0,
//!                 - \b SYSCTL_A_PERIPH_TIMER32_0_MODULE,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_3,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_2,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_1,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_0
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_A_enablePeripheralAtCPUHalt(uint_fast16_t devices);

//*****************************************************************************
//
//! Makes it so that the provided peripherals will either halt execution after
//! a CPU HALT. Parameters in this function can be combined to account for
//! multiple peripherals. By default, all peripherals keep running after a
//! CPU HALT.
//!
//! \param devices The peripherals to disable after a CPU HALT
//!
//! The \e devices parameter can be a bitwise OR of the following values:
//!         This can be a bitwise OR of the following values:
//!                 - \b SYSCTL_A_PERIPH_LCD,
//!                 - \b SYSCTL_A_PERIPH_DMA,
//!                 - \b SYSCTL_A_PERIPH_WDT,
//!                 - \b SYSCTL_A_PERIPH_ADC,
//!                 - \b SYSCTL_A_PERIPH_EUSCIB3,
//!                 - \b SYSCTL_A_PERIPH_EUSCIB2,
//!                 - \b SYSCTL_A_PERIPH_EUSCIB1
//!                 - \b SYSCTL_A_PERIPH_EUSCIB0,
//!                 - \b SYSCTL_A_PERIPH_EUSCIA3,
//!                 - \b SYSCTL_A_PERIPH_EUSCIA2
//!                 - \b SYSCTL_A_PERIPH_EUSCIA1,
//!                 - \b SYSCTL_A_PERIPH_EUSCIA0,
//!                 - \b SYSCTL_A_PERIPH_TIMER32_0_MODULE,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_3,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_2,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_1,
//!                 - \b SYSCTL_A_PERIPH_TIMER16_0
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_A_disablePeripheralAtCPUHalt(uint_fast16_t devices);

//*****************************************************************************
//
//! Sets the type of RESET that happens when a watchdog timeout occurs.
//!
//! \param resetType The type of reset to set
//!
//! The \e resetType parameter must be only one of the following values:
//!         - \b SYSCTL_A_HARD_RESET,
//!         - \b SYSCTL_A_SOFT_RESET
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_A_setWDTTimeoutResetType(uint_fast8_t resetType);

//*****************************************************************************
//
//! Sets the type of RESET that happens when a watchdog password violation
//! occurs.
//!
//! \param resetType The type of reset to set
//!
//! The \e resetType parameter must be only one of the following values:
//!         - \b SYSCTL_A_HARD_RESET,
//!         - \b SYSCTL_A_SOFT_RESET
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_A_setWDTPasswordViolationResetType(uint_fast8_t resetType);

//*****************************************************************************
//
//! Disables NMIs for the provided modules. When disabled, a NMI flag will not
//! occur when a fault condition comes from the corresponding modules.
//!
//! \param flags The NMI sources to disable
//! Can be a bitwise OR of the following parameters:
//!         - \b SYSCTL_A_NMIPIN_SRC,
//!         - \b SYSCTL_A_PCM_SRC,
//!         - \b SYSCTL_A_PSS_SRC,
//!         - \b SYSCTL_A_CS_SRC
//!
//
//*****************************************************************************
extern void SysCtl_A_disableNMISource(uint_fast8_t flags);

//*****************************************************************************
//
//! Enables NMIs for the provided modules. When enabled, a NMI flag will
//! occur when a fault condition comes from the corresponding modules.
//!
//! \param flags The NMI sources to enable
//! Can be a bitwise OR of the following parameters:
//!         - \b SYSCTL_A_NMIPIN_SRC,
//!         - \b SYSCTL_A_PCM_SRC,
//!         - \b SYSCTL_A_PSS_SRC,
//!         - \b SYSCTL_A_CS_SRC
//!
//
//*****************************************************************************
extern void SysCtl_A_enableNMISource(uint_fast8_t flags);

//*****************************************************************************
//
//! Returns the current sources of NMIs that are enabled
//!
//! \return NMI source status
//
//*****************************************************************************
extern uint_fast8_t SysCtl_A_getNMISourceStatus(void);

//*****************************************************************************
//
//! Enables glitch suppression on the reset pin of the device. Refer to the
//! device data sheet for specific information about glitch suppression
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_A_enableGlitchFilter(void);

//*****************************************************************************
//
//! Disables glitch suppression on the reset pin of the device. Refer to the
//! device data sheet for specific information about glitch suppression
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_A_disableGlitchFilter(void);

//*****************************************************************************
//
//! Retrieves the calibration constant of the temperature sensor to be used
//! in temperature calculation.
//!
//! \param refVoltage Reference voltage being used.
//!
//! The \e refVoltage parameter must be only one of the following values:
//!         - \b SYSCTL_A_1_2V_REF
//!         - \b SYSCTL_A_1_45V_REF
//!         - \b SYSCTL_A_2_5V_REF
//!
//! \param temperature is the calibration temperature that the user wants to be
//!     returned.
//!
//! The \e temperature parameter must be only one of the following values:
//!         - \b SYSCTL_A_30_DEGREES_C
//!         - \b SYSCTL_A_85_DEGREES_C
//!
//! \return None.
//
//
//*****************************************************************************
extern uint_fast16_t SysCtl_A_getTempCalibrationConstant(uint32_t refVoltage,
        uint32_t temperature);

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

#endif /* __MCU_HAS_SYSCTL_A__ */

#endif // __SYSCTL_A_H__
