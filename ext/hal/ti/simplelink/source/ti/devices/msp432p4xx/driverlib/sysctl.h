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
#ifndef __SYSCTL_H__
#define __SYSCTL_H__

#include <stdint.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

/* Define to ensure that our current MSP432 has the SYSCTL module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_SYSCTL__

//*****************************************************************************
//
//! \addtogroup sysctl_api
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
#define SYSCTL_SRAM_BANK7 SYSCTL_SRAM_BANKEN_BNK7_EN
#define SYSCTL_SRAM_BANK6 SYSCTL_SRAM_BANKEN_BNK6_EN
#define SYSCTL_SRAM_BANK5 SYSCTL_SRAM_BANKEN_BNK5_EN
#define SYSCTL_SRAM_BANK4 SYSCTL_SRAM_BANKEN_BNK4_EN
#define SYSCTL_SRAM_BANK3 SYSCTL_SRAM_BANKEN_BNK3_EN
#define SYSCTL_SRAM_BANK2 SYSCTL_SRAM_BANKEN_BNK2_EN
#define SYSCTL_SRAM_BANK1 SYSCTL_SRAM_BANKEN_BNK1_EN

#define SYSCTL_HARD_RESET 1
#define SYSCTL_SOFT_RESET 0

#define SYSCTL_PERIPH_DMA SYSCTL_PERIHALT_CTL_HALT_DMA
#define SYSCTL_PERIPH_WDT SYSCTL_PERIHALT_CTL_HALT_WDT
#define SYSCTL_PERIPH_ADC SYSCTL_PERIHALT_CTL_HALT_ADC
#define SYSCTL_PERIPH_EUSCIB3 SYSCTL_PERIHALT_CTL_HALT_EUB3
#define SYSCTL_PERIPH_EUSCIB2 SYSCTL_PERIHALT_CTL_HALT_EUB2
#define SYSCTL_PERIPH_EUSCIB1 SYSCTL_PERIHALT_CTL_HALT_EUB1
#define SYSCTL_PERIPH_EUSCIB0 SYSCTL_PERIHALT_CTL_HALT_EUB0
#define SYSCTL_PERIPH_EUSCIA3 SYSCTL_PERIHALT_CTL_HALT_EUA3
#define SYSCTL_PERIPH_EUSCIA2 SYSCTL_PERIHALT_CTL_HALT_EUA2
#define SYSCTL_PERIPH_EUSCIA1 SYSCTL_PERIHALT_CTL_HALT_EUA1
#define SYSCTL_PERIPH_EUSCIA0 SYSCTL_PERIHALT_CTL_HALT_EUA0
#define SYSCTL_PERIPH_TIMER32_0_MODULE SYSCTL_PERIHALT_CTL_HALT_T32_0
#define SYSCTL_PERIPH_TIMER16_3 SYSCTL_PERIHALT_CTL_HALT_T16_3
#define SYSCTL_PERIPH_TIMER16_2 SYSCTL_PERIHALT_CTL_HALT_T16_2
#define SYSCTL_PERIPH_TIMER16_1 SYSCTL_PERIHALT_CTL_HALT_T16_1
#define SYSCTL_PERIPH_TIMER16_0 SYSCTL_PERIHALT_CTL_HALT_T16_0

#define SYSCTL_NMIPIN_SRC SYSCTL_NMI_CTLSTAT_PIN_SRC
#define SYSCTL_PCM_SRC SYSCTL_NMI_CTLSTAT_PCM_SRC
#define SYSCTL_PSS_SRC SYSCTL_NMI_CTLSTAT_PSS_SRC
#define SYSCTL_CS_SRC SYSCTL_NMI_CTLSTAT_CS_SRC

#define SYSCTL_REBOOT_KEY   0x6900

#define SYSCTL_1_2V_REF        (uint32_t)&TLV->ADC14_REF1P2V_TS30C - (uint32_t)TLV_BASE
#define SYSCTL_1_45V_REF       (uint32_t)&TLV->ADC14_REF1P45V_TS30C - (uint32_t)TLV_BASE
#define SYSCTL_2_5V_REF        (uint32_t)&TLV->ADC14_REF2P5V_TS30C - (uint32_t)TLV_BASE

#define SYSCTL_85_DEGREES_C    4
#define SYSCTL_30_DEGREES_C    0


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
} SysCtl_FlashTLV_Info;

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

} SysCtl_CSCalTLV_Info;

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
extern uint_least32_t SysCtl_getSRAMSize(void);

//*****************************************************************************
//
//! Gets the size of the flash.
//!
//! \return The total number of bytes of flash.
//
//*****************************************************************************
extern uint_least32_t SysCtl_getFlashSize(void);

//*****************************************************************************
//
//! Reboots the device and causes the device to re-initialize itself.
//!
//! \return This function does not return.
//
//*****************************************************************************
extern void SysCtl_rebootDevice(void);

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
extern void SysCtl_getTLVInfo(uint_fast8_t tag, uint_fast8_t instance,
        uint_fast8_t *length, uint32_t **data_address);

//*****************************************************************************
//
//! Enables a set of banks in the SRAM. This can be used to optimize power
//! consumption when every SRAM bank isn't needed. It is important to note
//! that when a  higher bank is enabled, all of the SRAM banks below that bank
//! are also enabled. For example, if the user enables SYSCTL_SRAM_BANK7,
//! the banks SYSCTL_SRAM_BANK1 through SYSCTL_SRAM_BANK7 will be enabled
//! (SRAM_BANK0 is reserved and always enabled).
//!
//! \param sramBank The SRAM bank tier to enable.
//!        Must be only one of the following values:
//!                 - \b SYSCTL_SRAM_BANK1,
//!                 - \b SYSCTL_SRAM_BANK2,
//!                 - \b SYSCTL_SRAM_BANK3,
//!                 - \b SYSCTL_SRAM_BANK4,
//!                 - \b SYSCTL_SRAM_BANK5,
//!                 - \b SYSCTL_SRAM_BANK6,
//!                 - \b SYSCTL_SRAM_BANK7
//!
//! \note \b SYSCTL_SRAM_BANK0 is reserved and always enabled.
//!
//! \return None.
//
//*****************************************************************************
extern void SysCtl_enableSRAMBank(uint_fast8_t sramBank);

//*****************************************************************************
//
//! Disables a set of banks in the SRAM. This can be used to optimize power
//! consumption when every SRAM bank isn't needed. It is important to note
//! that when a  higher bank is disabled, all of the SRAM banks above that bank
//! are also disabled. For example, if the user disables SYSCTL_SRAM_BANK5,
//! the banks SYSCTL_SRAM_BANK6 through SYSCTL_SRAM_BANK7 will be disabled.
//!
//! \param sramBank The SRAM bank tier to disable.
//!        Must be only one of the following values:
//!                 - \b SYSCTL_SRAM_BANK1,
//!                 - \b SYSCTL_SRAM_BANK2,
//!                 - \b SYSCTL_SRAM_BANK3,
//!                 - \b SYSCTL_SRAM_BANK4,
//!                 - \b SYSCTL_SRAM_BANK5,
//!                 - \b SYSCTL_SRAM_BANK6,
//!                 - \b SYSCTL_SRAM_BANK7
//!
//! \note \b SYSCTL_SRAM_BANK0 is reserved and always enabled.
//!
//! \return None.
//
//*****************************************************************************
extern void SysCtl_disableSRAMBank(uint_fast8_t sramBank);

//*****************************************************************************
//
//! Enables retention of the specified SRAM bank register when the device goes
//! into LPM3 mode. When the system is placed in LPM3 mode, the SRAM
//! banks specified with this function will be placed into retention mode. By
//! default, retention of every SRAM bank except SYSCTL_SRAM_BANK0 (reserved) is
//! disabled. Retention of individual banks can be set without the restrictions
//! of the enable/disable functions.
//!
//! \param sramBank The SRAM banks to enable retention
//!        Can be a bitwise OR of the following values:
//!                 - \b SYSCTL_SRAM_BANK1,
//!                 - \b SYSCTL_SRAM_BANK2,
//!                 - \b SYSCTL_SRAM_BANK3,
//!                 - \b SYSCTL_SRAM_BANK4,
//!                 - \b SYSCTL_SRAM_BANK5,
//!                 - \b SYSCTL_SRAM_BANK6,
//!                 - \b SYSCTL_SRAM_BANK7
//! \note  \b SYSCTL_SRAM_BANK0 is reserved and retention is always enabled.
//!
//!
//! \return None.
//
//*****************************************************************************
extern void SysCtl_enableSRAMBankRetention(uint_fast8_t sramBank);

//*****************************************************************************
//
//! Disables retention of the specified SRAM bank register when the device goes
//! into LPM3 mode. When the system is placed in LPM3 mode, the SRAM
//! banks specified with this function will not be placed into retention mode.
//! By default, retention of every SRAM bank except SYSCTL_SRAM_BANK0 (reserved)
//! is disabled. Retention of individual banks can be set without the
//! restrictions of the enable/disable SRAM bank functions.
//!
//! \param sramBank The SRAM banks to disable retention
//!        Can be a bitwise OR of the following values:
//!                 - \b SYSCTL_SRAM_BANK1,
//!                 - \b SYSCTL_SRAM_BANK2,
//!                 - \b SYSCTL_SRAM_BANK3,
//!                 - \b SYSCTL_SRAM_BANK4,
//!                 - \b SYSCTL_SRAM_BANK5,
//!                 - \b SYSCTL_SRAM_BANK6,
//!                 - \b SYSCTL_SRAM_BANK7
//! \note  \b SYSCTL_SRAM_BANK0 is reserved and retention is always enabled.
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_disableSRAMBankRetention(uint_fast8_t sramBank);

//*****************************************************************************
//
//! Makes it so that the provided peripherals will either halt execution after
//! a CPU HALT. Parameters in this function can be combined to account for
//! multiple peripherals. By default, all peripherals keep running after a
//! CPU HALT.
//!
//! \param devices The peripherals to continue running after a CPU HALT
//!         This can be a bitwise OR of the following values:
//!                 - \b SYSCTL_PERIPH_DMA,
//!                 - \b SYSCTL_PERIPH_WDT,
//!                 - \b SYSCTL_PERIPH_ADC,
//!                 - \b SYSCTL_PERIPH_EUSCIB3,
//!                 - \b SYSCTL_PERIPH_EUSCIB2,
//!                 - \b SYSCTL_PERIPH_EUSCIB1
//!                 - \b SYSCTL_PERIPH_EUSCIB0,
//!                 - \b SYSCTL_PERIPH_EUSCIA3,
//!                 - \b SYSCTL_PERIPH_EUSCIA2
//!                 - \b SYSCTL_PERIPH_EUSCIA1,
//!                 - \b SYSCTL_PERIPH_EUSCIA0,
//!                 - \b SYSCTL_PERIPH_TIMER32_0_MODULE,
//!                 - \b SYSCTL_PERIPH_TIMER16_3,
//!                 - \b SYSCTL_PERIPH_TIMER16_2,
//!                 - \b SYSCTL_PERIPH_TIMER16_1,
//!                 - \b SYSCTL_PERIPH_TIMER16_0
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_enablePeripheralAtCPUHalt(uint_fast16_t devices);

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
//!                 - \b SYSCTL_PERIPH_DMA,
//!                 - \b SYSCTL_PERIPH_WDT,
//!                 - \b SYSCTL_PERIPH_ADC,
//!                 - \b SYSCTL_PERIPH_EUSCIB3,
//!                 - \b SYSCTL_PERIPH_EUSCIB2,
//!                 - \b SYSCTL_PERIPH_EUSCIB1
//!                 - \b SYSCTL_PERIPH_EUSCIB0,
//!                 - \b SYSCTL_PERIPH_EUSCIA3,
//!                 - \b SYSCTL_PERIPH_EUSCIA2
//!                 - \b SYSCTL_PERIPH_EUSCIA1,
//!                 - \b SYSCTL_PERIPH_EUSCIA0,
//!                 - \b SYSCTL_PERIPH_TIMER32_0_MODULE,
//!                 - \b SYSCTL_PERIPH_TIMER16_3,
//!                 - \b SYSCTL_PERIPH_TIMER16_2,
//!                 - \b SYSCTL_PERIPH_TIMER16_1,
//!                 - \b SYSCTL_PERIPH_TIMER16_0
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_disablePeripheralAtCPUHalt(uint_fast16_t devices);

//*****************************************************************************
//
//! Sets the type of RESET that happens when a watchdog timeout occurs.
//!
//! \param resetType The type of reset to set
//!
//! The \e resetType parameter must be only one of the following values:
//!         - \b SYSCTL_HARD_RESET,
//!         - \b SYSCTL_SOFT_RESET
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_setWDTTimeoutResetType(uint_fast8_t resetType);

//*****************************************************************************
//
//! Sets the type of RESET that happens when a watchdog password violation
//! occurs.
//!
//! \param resetType The type of reset to set
//!
//! The \e resetType parameter must be only one of the following values:
//!         - \b SYSCTL_HARD_RESET,
//!         - \b SYSCTL_SOFT_RESET
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_setWDTPasswordViolationResetType(uint_fast8_t resetType);

//*****************************************************************************
//
//! Disables NMIs for the provided modules. When disabled, a NMI flag will not
//! occur when a fault condition comes from the corresponding modules.
//!
//! \param flags The NMI sources to disable
//! Can be a bitwise OR of the following parameters:
//!         - \b SYSCTL_NMIPIN_SRC,
//!         - \b SYSCTL_PCM_SRC,
//!         - \b SYSCTL_PSS_SRC,
//!         - \b SYSCTL_CS_SRC
//!
//
//*****************************************************************************
extern void SysCtl_disableNMISource(uint_fast8_t flags);

//*****************************************************************************
//
//! Enables NMIs for the provided modules. When enabled, a NMI flag will
//! occur when a fault condition comes from the corresponding modules.
//!
//! \param flags The NMI sources to enable
//! Can be a bitwise OR of the following parameters:
//!         - \b SYSCTL_NMIPIN_SRC,
//!         - \b SYSCTL_PCM_SRC,
//!         - \b SYSCTL_PSS_SRC,
//!         - \b SYSCTL_CS_SRC
//!
//
//*****************************************************************************
extern void SysCtl_enableNMISource(uint_fast8_t flags);

//*****************************************************************************
//
//! Returns the current sources of NMIs that are enabled
//!
//! \return Bitwise OR of NMI flags that are enabled
//
//*****************************************************************************
extern uint_fast8_t SysCtl_getNMISourceStatus(void);

//*****************************************************************************
//
//! Enables glitch suppression on the reset pin of the device. Refer to the
//! device data sheet for specific information about glitch suppression
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_enableGlitchFilter(void);

//*****************************************************************************
//
//! Disables glitch suppression on the reset pin of the device. Refer to the
//! device data sheet for specific information about glitch suppression
//!
//! \return None.
//
//
//*****************************************************************************
extern void SysCtl_disableGlitchFilter(void);

//*****************************************************************************
//
//! Retrieves the calibration constant of the temperature sensor to be used
//! in temperature calculation.
//!
//! \param refVoltage Reference voltage being used.
//!
//! The \e refVoltage parameter must be only one of the following values:
//!         - \b SYSCTL_1_2V_REF
//!         - \b SYSCTL_1_45V_REF
//!         - \b SYSCTL_2_5V_REF
//!
//! \param temperature is the calibration temperature that the user wants to be
//!     returned.
//!
//! The \e temperature parameter must be only one of the following values:
//!         - \b SYSCTL_30_DEGREES_C
//!         - \b SYSCTL_85_DEGREES_C
//!
//! \return None.
//
//
//*****************************************************************************
extern uint_fast16_t SysCtl_getTempCalibrationConstant(uint32_t refVoltage,
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

#endif /* __MCU_HAS_SYSCTL__ */

#endif // __SYSCTL_H__
