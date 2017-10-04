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
#ifndef ADC14_H_
#define ADC14_H_

//*****************************************************************************
//
//!
//! \addtogroup adc14_api
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

#include <stdint.h>
#include <stdbool.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

//*****************************************************************************
//
// Control specific variables
//
//*****************************************************************************

//*****************************************************************************
//
//The following are values that can be passed to ADC14_initModule
//
//*****************************************************************************
#define ADC_CLOCKSOURCE_ADCOSC   (ADC14_CTL0_SSEL_0)
#define ADC_CLOCKSOURCE_SYSOSC   (ADC14_CTL0_SSEL_1)
#define ADC_CLOCKSOURCE_ACLK     (ADC14_CTL0_SSEL_2)
#define ADC_CLOCKSOURCE_MCLK     (ADC14_CTL0_SSEL_3)
#define ADC_CLOCKSOURCE_SMCLK    (ADC14_CTL0_SSEL_4)
#define ADC_CLOCKSOURCE_HSMCLK   (ADC14_CTL0_SSEL_5)

#define ADC_PREDIVIDER_1         (ADC14_CTL0_PDIV_0)
#define ADC_PREDIVIDER_4         (ADC14_CTL0_PDIV_1)
#define ADC_PREDIVIDER_32        (ADC14_CTL0_PDIV_2)
#define ADC_PREDIVIDER_64        (ADC14_CTL0_PDIV_3)

#define ADC_DIVIDER_1            (ADC14_CTL0_DIV_0)
#define ADC_DIVIDER_2            (ADC14_CTL0_DIV_1)
#define ADC_DIVIDER_3            (ADC14_CTL0_DIV_2)
#define ADC_DIVIDER_4            (ADC14_CTL0_DIV_3)
#define ADC_DIVIDER_5            (ADC14_CTL0_DIV_4)
#define ADC_DIVIDER_6            (ADC14_CTL0_DIV_5)
#define ADC_DIVIDER_7            (ADC14_CTL0_DIV_6)
#define ADC_DIVIDER_8            (ADC14_CTL0_DIV_7)

#define ADC_MAPINTCH3            (ADC14_CTL1_CH3MAP)
#define ADC_MAPINTCH2            (ADC14_CTL1_CH2MAP)
#define ADC_MAPINTCH1            (ADC14_CTL1_CH1MAP)
#define ADC_MAPINTCH0            (ADC14_CTL1_CH0MAP)
#define ADC_TEMPSENSEMAP         (ADC14_CTL1_TCMAP)
#define ADC_BATTMAP              (ADC14_CTL1_BATMAP)
#define ADC_NOROUTE               0

#define ADC_8BIT                 ADC14_CTL1_RES_0
#define ADC_10BIT                ADC14_CTL1_RES_1
#define ADC_12BIT                ADC14_CTL1_RES_2
#define ADC_14BIT                ADC14_CTL1_RES_3

#define ADC_TRIGGER_ADCSC        ADC14_CTL0_SHS_0
#define ADC_TRIGGER_SOURCE1      ADC14_CTL0_SHS_1
#define ADC_TRIGGER_SOURCE2      ADC14_CTL0_SHS_2
#define ADC_TRIGGER_SOURCE3      ADC14_CTL0_SHS_3
#define ADC_TRIGGER_SOURCE4      ADC14_CTL0_SHS_4
#define ADC_TRIGGER_SOURCE5      ADC14_CTL0_SHS_5
#define ADC_TRIGGER_SOURCE6      ADC14_CTL0_SHS_6
#define ADC_TRIGGER_SOURCE7      ADC14_CTL0_SHS_7

#define ADC_PULSE_WIDTH_4        ADC14_CTL0_SHT1_0
#define ADC_PULSE_WIDTH_8        ADC14_CTL0_SHT1_1
#define ADC_PULSE_WIDTH_16       ADC14_CTL0_SHT1_2
#define ADC_PULSE_WIDTH_32       ADC14_CTL0_SHT1_3
#define ADC_PULSE_WIDTH_64       ADC14_CTL0_SHT1_4
#define ADC_PULSE_WIDTH_96       ADC14_CTL0_SHT1_5
#define ADC_PULSE_WIDTH_128      ADC14_CTL0_SHT1_6
#define ADC_PULSE_WIDTH_192      ADC14_CTL0_SHT1_7

#define ADC_NONDIFFERENTIAL_INPUTS false
#define ADC_DIFFERENTIAL_INPUTS true

#define ADC_MEM0                 0x00000001
#define ADC_MEM1                 0x00000002
#define ADC_MEM2                 0x00000004
#define ADC_MEM3                 0x00000008
#define ADC_MEM4                 0x00000010
#define ADC_MEM5                 0x00000020
#define ADC_MEM6                 0x00000040
#define ADC_MEM7                 0x00000080
#define ADC_MEM8                 0x00000100
#define ADC_MEM9                 0x00000200
#define ADC_MEM10                0x00000400
#define ADC_MEM11                0x00000800
#define ADC_MEM12                0x00001000
#define ADC_MEM13                0x00002000
#define ADC_MEM14                0x00004000
#define ADC_MEM15                0x00008000
#define ADC_MEM16                0x00010000
#define ADC_MEM17                0x00020000
#define ADC_MEM18                0x00040000
#define ADC_MEM19                0x00080000
#define ADC_MEM20                0x00100000
#define ADC_MEM21                0x00200000
#define ADC_MEM22                0x00400000
#define ADC_MEM23                0x00800000
#define ADC_MEM24                0x01000000
#define ADC_MEM25                0x02000000
#define ADC_MEM26                0x04000000
#define ADC_MEM27                0x08000000
#define ADC_MEM28                0x10000000
#define ADC_MEM29                0x20000000
#define ADC_MEM30                0x40000000
#define ADC_MEM31                0x80000000

#define ADC_VREFPOS_AVCC_VREFNEG_VSS       (ADC14_MCTLN_VRSEL_0)
#define ADC_VREFPOS_INTBUF_VREFNEG_VSS     (ADC14_MCTLN_VRSEL_1)
#define ADC_VREFPOS_EXTPOS_VREFNEG_EXTNEG  (ADC14_MCTLN_VRSEL_14)
#define ADC_VREFPOS_EXTBUF_VREFNEG_EXTNEG  (ADC14_MCTLN_VRSEL_15)

#define ADC_INPUT_A0              (ADC14_MCTLN_INCH_0)
#define ADC_INPUT_A1              (ADC14_MCTLN_INCH_1)
#define ADC_INPUT_A2              (ADC14_MCTLN_INCH_2)
#define ADC_INPUT_A3              (ADC14_MCTLN_INCH_3)
#define ADC_INPUT_A4              (ADC14_MCTLN_INCH_4)
#define ADC_INPUT_A5              (ADC14_MCTLN_INCH_5)
#define ADC_INPUT_A6              (ADC14_MCTLN_INCH_6)
#define ADC_INPUT_A7              (ADC14_MCTLN_INCH_7)
#define ADC_INPUT_A8              (ADC14_MCTLN_INCH_8)
#define ADC_INPUT_A9              (ADC14_MCTLN_INCH_9)
#define ADC_INPUT_A10             (ADC14_MCTLN_INCH_10)
#define ADC_INPUT_A11             (ADC14_MCTLN_INCH_11)
#define ADC_INPUT_A12             (ADC14_MCTLN_INCH_12)
#define ADC_INPUT_A13             (ADC14_MCTLN_INCH_13)
#define ADC_INPUT_A14             (ADC14_MCTLN_INCH_14)
#define ADC_INPUT_A15             (ADC14_MCTLN_INCH_15)
#define ADC_INPUT_A16             (ADC14_MCTLN_INCH_16)
#define ADC_INPUT_A17             (ADC14_MCTLN_INCH_17)
#define ADC_INPUT_A18             (ADC14_MCTLN_INCH_18)
#define ADC_INPUT_A19             (ADC14_MCTLN_INCH_19)
#define ADC_INPUT_A20             (ADC14_MCTLN_INCH_20)
#define ADC_INPUT_A21             (ADC14_MCTLN_INCH_21)
#define ADC_INPUT_A22             (ADC14_MCTLN_INCH_22)
#define ADC_INPUT_A23             (ADC14_MCTLN_INCH_23)
#define ADC_INPUT_A24             (ADC14_MCTLN_INCH_24)
#define ADC_INPUT_A25             (ADC14_MCTLN_INCH_25)
#define ADC_INPUT_A26             (ADC14_MCTLN_INCH_26)
#define ADC_INPUT_A27             (ADC14_MCTLN_INCH_27)
#define ADC_INPUT_A28             (ADC14_MCTLN_INCH_28)
#define ADC_INPUT_A29             (ADC14_MCTLN_INCH_29)
#define ADC_INPUT_A30             (ADC14_MCTLN_INCH_30)
#define ADC_INPUT_A31             (ADC14_MCTLN_INCH_31)

#define ADC_COMP_WINDOW0          0x00
#define ADC_COMP_WINDOW1          0x01

#define ADC_SIGNED_BINARY         0x00
#define ADC_UNSIGNED_BINARY       0x01

#define ADC_MANUAL_ITERATION       0x00
#define ADC_AUTOMATIC_ITERATION    ADC14_CTL0_MSC

#define ADC_UNRESTRICTED_POWER_MODE   ADC14_CTL1_PWRMD_0
#define ADC_ULTRA_LOW_POWER_MODE      ADC14_CTL1_PWRMD_2


#define ADC_INT0 ADC14_IER0_IE0
#define ADC_INT1 ADC14_IER0_IE1
#define ADC_INT2 ADC14_IER0_IE2
#define ADC_INT3 ADC14_IER0_IE3
#define ADC_INT4 ADC14_IER0_IE4
#define ADC_INT5 ADC14_IER0_IE5
#define ADC_INT6 ADC14_IER0_IE6
#define ADC_INT7 ADC14_IER0_IE7
#define ADC_INT8 ADC14_IER0_IE8
#define ADC_INT9 ADC14_IER0_IE9
#define ADC_INT10 ADC14_IER0_IE10
#define ADC_INT11 ADC14_IER0_IE11
#define ADC_INT12 ADC14_IER0_IE12
#define ADC_INT13 ADC14_IER0_IE13
#define ADC_INT14 ADC14_IER0_IE14
#define ADC_INT15 ADC14_IER0_IE15
#define ADC_INT16 ADC14_IER0_IE16
#define ADC_INT17 ADC14_IER0_IE17
#define ADC_INT18 ADC14_IER0_IE18
#define ADC_INT19 ADC14_IER0_IE19
#define ADC_INT20 ADC14_IER0_IE20
#define ADC_INT21 ADC14_IER0_IE21
#define ADC_INT22 ADC14_IER0_IE22
#define ADC_INT23 ADC14_IER0_IE23
#define ADC_INT24 ADC14_IER0_IE24
#define ADC_INT25 ADC14_IER0_IE25
#define ADC_INT26 ADC14_IER0_IE26
#define ADC_INT27 ADC14_IER0_IE27
#define ADC_INT28 ADC14_IER0_IE28
#define ADC_INT29 ADC14_IER0_IE29
#define ADC_INT30 ADC14_IER0_IE30
#define ADC_INT31 ADC14_IER0_IE31
#define ADC_IN_INT 0x0000000200000000
#define ADC_LO_INT 0x0000000400000000
#define ADC_HI_INT 0x0000000800000000
#define ADC_OV_INT 0x0000001000000000
#define ADC_TOV_INT 0x0000002000000000
#define ADC_RDY_INT 0x0000004000000000

#define ADC_INVALID_MEM   32

//*****************************************************************************
//
//Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//!
//! Enables the ADC block.
//!
//! This will enable operation of the ADC block.
//!
//! \return none.
//
//*****************************************************************************
extern void ADC14_enableModule(void);

//*****************************************************************************
//
//!
//! Disables the ADC block.
//!
//! This will disable operation of the ADC block.
//!
//! \return false if user is trying to disable during active conversion
//
//*****************************************************************************
extern bool ADC14_disableModule(void);

//*****************************************************************************
//
//!
//! Initializes the ADC module and sets up the clock system divider/pre-divider.
//! This initialization function will also configure the internal/external
//! signal mapping.
//!
//! \note A call to this function while active ADC conversion is happening
//! is an invalid case and will result in a false value being returned.
//!
//! \param clockSource The clock source to use for the ADC module.
//!                 - \b ADC_CLOCKSOURCE_ADCOSC [DEFAULT]
//!                 - \b ADC_CLOCKSOURCE_SYSOSC
//!                 - \b ADC_CLOCKSOURCE_ACLK
//!                 - \b ADC_CLOCKSOURCE_MCLK
//!                 - \b ADC_CLOCKSOURCE_SMCLK
//!                 - \b ADC_CLOCKSOURCE_HSMCLK
//!
//! \param clockPredivider Divides the given clock source before feeding it
//!         into the main clock divider.
//!             Valid values are:
//!                 - \b ADC_PREDIVIDER_1 [DEFAULT]
//!                 - \b ADC_PREDIVIDER_4
//!                 - \b ADC_PREDIVIDER_32
//!                 - \b ADC_PREDIVIDER_64
//!
//! \param clockDivider Divides the pre-divided clock source
//!         Valid values are
//!             - \b ADC_DIVIDER_1 [Default value]
//!             - \b ADC_DIVIDER_2
//!             - \b ADC_DIVIDER_3
//!             - \b ADC_DIVIDER_4
//!             - \b ADC_DIVIDER_5
//!             - \b ADC_DIVIDER_6
//!             - \b ADC_DIVIDER_7
//!             - \b ADC_DIVIDER_8
//!
//! \param internalChannelMask
//!  Configures the internal/external pin mappings
//!  for the ADC modules. This setting determines if the given ADC channel or
//!  component is mapped to an external pin (default), or routed to an internal
//!  component. This parameter is a bit mask where a logical high value will
//!  switch the component to the internal routing. For a list of internal
//!  routings, please refer to the device specific data sheet.
//!  Valid values are a logical OR of the following values:
//!         - \b ADC_MAPINTCH3
//!         - \b ADC_MAPINTCH2
//!         - \b ADC_MAPINTCH1
//!         - \b ADC_MAPINTCH0
//!         - \b ADC_TEMPSENSEMAP
//!         - \b ADC_BATTMAP
//!         - \n ADC_NOROUTE
//! If internalChannelMask is not desired, pass ADC_NOROUTE in lieu of this
//!  parameter.
//!
//! \return false if the initialization fails due to an in progress conversion
//!
//!
//
//*****************************************************************************
extern bool ADC14_initModule(uint32_t clockSource, uint32_t clockPredivider,
        uint32_t clockDivider, uint32_t internalChannelMask);

//*****************************************************************************
//
//!
//! Sets the resolution of the ADC module. The default resolution is 12-bit,
//! however for power consumption concerns this can be limited to a lower
//! resolution
//!
//! \param resolution Resolution of the ADC module
//!         - \b ADC_8BIT (10 clock cycle conversion time)
//!         - \b ADC_10BIT (12 clock cycle conversion time)
//!         - \b ADC_12BIT (14 clock cycle conversion time)
//!         - \b ADC_14BIT (16 clock cycle conversion time)[DEFAULT]
//!
//! \return none
//
//*****************************************************************************
extern void ADC14_setResolution(uint32_t resolution);

//*****************************************************************************
//
//!
//! Gets the resolution of the ADC module.
//!
//! \return Resolution of the ADC module
//!         - \b ADC_8BIT (10 clock cycle conversion time)
//!         - \b ADC_10BIT (12 clock cycle conversion time)
//!         - \b ADC_12BIT (14 clock cycle conversion time)
//!         - \b ADC_14BIT (16 clock cycle conversion time)
//
//*****************************************************************************
extern uint_fast32_t ADC14_getResolution(void);

//*****************************************************************************
//
//!
//! Sets the source for the trigger of the ADC module. By default, this value
//! is configured to a software source (the ADCSC bit), however depending on
//! the specific device the trigger can be set to different sources (for
//! example, a timer output). These sources vary from part to part and the
//! user should refer to the device specific datasheet.
//!
//! \param source Trigger source for sampling. Possible values include:
//!         - \b ADC_TRIGGER_ADCSC [DEFAULT]
//!         - \b ADC_TRIGGER_SOURCE1
//!         - \b ADC_TRIGGER_SOURCE2
//!         - \b ADC_TRIGGER_SOURCE3
//!         - \b ADC_TRIGGER_SOURCE4
//!         - \b ADC_TRIGGER_SOURCE5
//!         - \b ADC_TRIGGER_SOURCE6
//!         - \b ADC_TRIGGER_SOURCE7
//! \param invertSignal When set to true, will invert the trigger signal to a
//!         falling edge. When false, will use a rising edge.
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_setSampleHoldTrigger(uint32_t source, bool invertSignal);

//*****************************************************************************
//
//!
//! Sets the sample/hold time for the specified memory register range. The
//! duration of time required for a sample differs depending on the user's
//! hardware configuration.
//!
//! There are two values in the ADCC module. The first value controls
//! ADC memory locations ADC_MEMORY_0 through ADC_MEMORY_7 and
//! ADC_MEMORY_24 through ADC_MEMORY_31, while the second value
//! controls memory locations ADC_MEMORY_8 through ADC_MEMORY_23.
//!
//! \param firstPulseWidth Pulse width of the first pulse in ADCCLK cycles
//!     Possible values must be one of the following:
//!         - \b ADC_PULSE_WIDTH_4 [DEFAULT]
//!         - \b ADC_PULSE_WIDTH_8
//!         - \b ADC_PULSE_WIDTH_16
//!         - \b ADC_PULSE_WIDTH_32
//!         - \b ADC_PULSE_WIDTH_64
//!         - \b ADC_PULSE_WIDTH_96
//!         - \b ADC_PULSE_WIDTH_128
//!         - \b ADC_PULSE_WIDTH_192
//! \param secondPulseWidth Pulse width of the second pulse in ADCCLK
//!     cycles. Possible values must be one of the following:
//!         - \b ADC_PULSE_WIDTH_4 [DEFAULT]
//!         - \b ADC_PULSE_WIDTH_8
//!         - \b ADC_PULSE_WIDTH_16
//!         - \b ADC_PULSE_WIDTH_32
//!         - \b ADC_PULSE_WIDTH_64
//!         - \b ADC_PULSE_WIDTH_96
//!         - \b ADC_PULSE_WIDTH_128
//!         - \b ADC_PULSE_WIDTH_192
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_setSampleHoldTime(uint32_t firstPulseWidth,
        uint32_t secondPulseWidth);

//*****************************************************************************
//
//!
//! Configures the ADC module to use a multiple memory sample scheme. This
//! means that multiple samples will consecutively take place and be stored in
//! multiple memory locations. The first sample/conversion will be placed in
//! memoryStart, while the last sample will be stored in memoryEnd.
//! Each memory location should be configured individually using the
//! ADC14_configureConversionMemory function.
//!
//! The ADC module can be started in "repeat" mode which will cause the
//! ADC module to resume sampling once the initial sample/conversion set is
//! executed.  For multi-sample mode, this means that the sampling of the
//! entire memory provided.
//!
//! \param memoryStart Memory location to store first sample/conversion
//!         value. Possible values include:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//! \param memoryEnd Memory location to store last sample.
//!     Possible values include:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//! \param repeatMode Specifies whether or not to repeat the conversion/sample
//!         cycle after the first round of sample/conversions. Valid values
//!         are true or false.
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_configureMultiSequenceMode(uint32_t memoryStart,
        uint32_t memoryEnd, bool repeatMode);

//*****************************************************************************
//
//!
//! Configures the ADC module to use a a single ADC memory location for
//! sampling/conversion. This is used when only one channel might be needed for
//! conversion, or where using a multiple sampling scheme is not important.
//!
//! The ADC module can be started in "repeat" mode which will cause the
//! ADC module to resume sampling once the initial sample/conversion set is
//! executed. In single sample mode, this will cause the ADC module to
//! continuously sample into the memory destination provided.

//! \param memoryDestination Memory location to store sample/conversion
//!         value. Possible values include:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//!
//! \param repeatMode Specifies whether or not to repeat the conversion/sample
//!         cycle after the first round of sample/conversions
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_configureSingleSampleMode(uint32_t memoryDestination,
        bool repeatMode);

//*****************************************************************************
//
//!
//! Enables conversion of ADC data. Note that this only enables conversion.
//! To trigger the conversion, you will have to call the
//! ADC14_toggleConversionTrigger or use the source trigger configured in
//! ADC14_setSampleHoldTrigger.
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_enableConversion(void);

//*****************************************************************************
//
//!
//! Halts conversion conversion of the ADC module. Note that the software bit
//! for triggering conversions will also be cleared with this function.
//!
//! If multi-sequence conversion mode was enabled, the position of the last
//! completed conversion can be retrieved using ADCLastConversionMemoryGet
//!
//! \return none
//
//*****************************************************************************
extern void ADC14_disableConversion(void);

//*****************************************************************************
//
//!
//! Toggles the trigger for conversion of the ADC module by toggling the
//! trigger software bit. Note that this will cause the ADC to start
//! conversion regardless if the software bit was set as the trigger using
//! ADC14_setSampleHoldTrigger.
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_toggleConversionTrigger(void);

//*****************************************************************************
//
//!
//! Returns a boolean value that tells if a conversion/sample is in progress
//!
//! \return true if conversion is active, false otherwise
//
//*****************************************************************************
extern bool ADC14_isBusy(void);

//*****************************************************************************
//
//!
//! Configures an individual memory location for the ADC module.
//!
//! \param memorySelect is the individual ADC memory location to
//!     configure. If multiple memory locations want to be configured with the
//!     same configuration, this value can be logically ORed together with other
//!     values.
//!         - \b ADC_MEM0 through \b ADC_MEM31
//! \param refSelect is the voltage reference to use for the selected
//!         memory spot. Possible values include:
//!         - \b ADC_VREFPOS_AVCC_VREFNEG_VSS [DEFAULT]
//!         - \b ADC_VREFPOS_INTBUF_VREFNEG_VSS
//!         - \b ADC_VREFPOS_EXTPOS_VREFNEG_EXTNEG
//!         - \b ADC_VREFPOS_EXTBUF_VREFNEG_EXTNEG
//! \param channelSelect selects the channel to be used for ADC sampling.
//!         Note if differential mode is enabled, the value sampled will be
//!         equal to the difference between the corresponding even/odd memory
//!         locations. Possible values are:
//!         - \b ADC_INPUT_A0 through \b ADC_INPUT_A31
//!
//! \param differntialMode selects if the channel selected by the
//!     channelSelect will be configured in differential mode. If this
//!     parameter is given as true, the configured channel will be paired
//!     with its neighbor in differential mode. for example, if channel A0 or A1
//!     is selected, the channel configured will be the difference between A0
//!     and A1. If A2 or A3 are selected, the channel configured will be the
//!     difference between A2 and A3 (and so on). Users can enter true or false,
//!     or one of the following values:
//!         - ADC_NONDIFFERENTIAL_INPUTS
//!         - ADC_DIFFERENTIAL_INPUTS
//!
//!
//! \return false if setting fails due to an in progress conversion
//!
//
//*****************************************************************************
extern bool ADC14_configureConversionMemory(uint32_t memorySelect,
        uint32_t refSelect, uint32_t channelSelect, bool differntialMode);

//*****************************************************************************
//
//!
//! Enables the specified mask of memory channels to use the specified
//! comparator window. THe ADCC module has two different comparator windows
//! that can be set with this function.
//!
//! \param memorySelect is the mask of memory locations to enable the
//!         comparator window for. This can be a bitwise OR of the following
//!         values:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//! \param windowSelect Memory location to store sample/conversion
//!         value. Possible values include:
//!         \b ADCOMP_WINDOW0 [DEFAULT]
//!         \b ADCOMP_WINDOW1
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_enableComparatorWindow(uint32_t memorySelect,
        uint32_t windowSelect);

//*****************************************************************************
//
//!
//! Disables the comparator window on the specified memory channels
//!
//! \param memorySelect is the mask of memory locations to disable the
//!         comparator window for. This can be a bitwise OR of the following
//!         values:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//!
//! \return false if setting fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_disableComparatorWindow(uint32_t memorySelect);

//*****************************************************************************
//
//!
//! Sets the lower and upper limits of the specified window comparator. Note
//! that this function will truncate values based of the resolution/data
//! format configured. If the ADC is operating in 10-bit mode, and a 12-bit
//! value is passed into this function the most significant 2 bits will be
//! truncated.
//!
//! The parameters provided to this function for the upper and lower threshold
//! depend on the current resolution for the ADC. For example, if configured
//! in 12-bit mode, a 12-bit resolution is the maximum that can be provided
//! for the window. If in 2's complement mode, Bit 15 is used as the MSB.
//!
//! \param window Memory location to store sample/conversion
//!         value. Possible values include:
//!         \b ADC_COMP_WINDOW0 [DEFAULT]
//!         \b ADC_COMP_WINDOW1
//! \param low is the lower limit of the window comparator
//! \param high is the upper limit of the window comparator
//!
//! \return false if setting fails due to an in progress conversion
//!
//
//*****************************************************************************
extern bool ADC14_setComparatorWindowValue(uint32_t window, int16_t low,
        int16_t high);

//*****************************************************************************
//
//!
//! Switches between a binary unsigned data format and a signed 2's complement
//! data format.
//!
//! \param resultFormat Format for result to conversion results.
//!         Possible values include:
//!         \b ADC_UNSIGNED_BINARY [DEFAULT]
//!         \b ADC_SIGNED_BINARY
//!
//! \return false if setting fails due to an in progress conversion
//!
//
//*****************************************************************************
extern bool ADC14_setResultFormat(uint32_t resultFormat);

//*****************************************************************************
//
//!
//! Returns the conversion result for the specified memory channel in the format
//! assigned by the ADC14_setResultFormat (unsigned binary by default) function.
//!
//! \param memorySelect is the memory location to get the conversion result.
//!     Valid values are:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//! \return conversion result of specified memory channel
//!
//
//*****************************************************************************
extern uint_fast16_t ADC14_getResult(uint32_t memorySelect);

//*****************************************************************************
//
//!
//! Returns the conversion results of the currently configured multi-sequence
//! conversion. If a multi-sequence conversion has not happened, this value
//! is unreliable. Note that it is up to the user to verify the integrity of
//! and proper size of the array being passed. If there are 16 multi-sequence
//! results, and an array with only 4 elements allocated is passed, invalid
//! memory settings will occur
//!
//! \param res conversion result of the last multi-sequence sample
//! in an array of unsigned 16-bit integers
//!
//! \return None
//!
//
//*****************************************************************************
extern void ADC14_getMultiSequenceResult(uint16_t* res);

//*****************************************************************************
//
//!
//! Returns the conversion results of the specified ADC memory locations.
//! Note that it is up to the user to verify the integrity of
//! and proper size of the array being passed. If there are 16 multi-sequence
//! results, and an array with only 4 elements allocated is passed, invalid
//! memory settings will occur. This function is inclusive.
//!
//! \param memoryStart is the memory location to get the conversion result.
//!     Valid values are:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//!
//! \param memoryEnd is the memory location to get the conversion result.
//!     Valid values are:
//!         - \b ADC_MEM0 through \b ADC_MEM31
//! \param res conversion result of the last multi-sequence sample
//! in an array of unsigned 16-bit integers
//!
//! \return None
//!
//
//*****************************************************************************
extern void ADC14_getResultArray(uint32_t memoryStart, uint32_t memoryEnd,
        uint16_t* res);

//*****************************************************************************
//
//!
//! Enables the "on-demand" activity of the voltage reference register. If this
//! setting is enabled, the internal voltage reference buffer will only be
//! updated during a sample or conversion cycle. This is used to optimize
//! power consumption.
//!
//! \return false if setting fails due to an in progress conversion
//!
//
//*****************************************************************************
extern bool ADC14_enableReferenceBurst(void);

//*****************************************************************************
//
//!
//! Disables the "on-demand" activity of the voltage reference register.
//!
//! \return false if setting fails due to an in progress conversion
//!
//
//*****************************************************************************
extern bool ADC14_disableReferenceBurst(void);

//*****************************************************************************
//
//!
//! Sets the power mode of the ADC module. A more aggressive power mode will
//! restrict the number of samples per second for sampling while optimizing
//! power consumption. Ideally, if power consumption is a concern, this value
//! should be set to the most restrictive setting that satisfies your sampling
//! requirement.
//!
//! \param adcPowerMode is the power mode to set. Valid values are:
//!         - \b ADC_UNRESTRICTED_POWER_MODE (no restriction)
//!         - \b ADC_LOW_POWER_MODE (500ksps restriction)
//!         - \b ADC_ULTRA_LOW_POWER_MODE (200ksps restriction)
//!         - \b ADC_EXTREME_LOW_POWER_MODE (50ksps restriction)
//!
//! \return false if setting fails due to an in progress conversion
//!
//
//*****************************************************************************
extern bool ADC14_setPowerMode(uint32_t powerMode);

//*****************************************************************************
//
//!
//! Enables SAMPCON to be sourced from the sampling timer and to configures
//! multi sample and conversion mode.
//! \param multiSampleConvert - Switches between manual and automatic
//!         iteration when using the sample timer. Valid values are:
//! - \b ADC_MANUAL_ITERATION The user will have to manually set the SHI signal
//!         ( usually by  ADC14_toggleConversionTrigger ) at the end
//!         of each sample/conversion cycle.
//! - \b ADC_AUTOMATIC_ITERATION  After one sample/convert is finished, the ADC
//!         module will automatically continue on to the next sample
//!
//! \return false if the initialization fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_enableSampleTimer(uint32_t multiSampleConvert);

//*****************************************************************************
//
//!
//! Disables SAMPCON from being sourced from the sample timer.
//!
//! \return false if the initialization fails due to an in progress conversion
//
//*****************************************************************************
extern bool ADC14_disableSampleTimer(void);

//*****************************************************************************
//
//!
//! Enables the indicated ADCC interrupt sources. The ADC_INT0
//! through ADC_INT31 parameters correspond to a completion event of the
//! corresponding memory location. For example, when the ADC_MEM0 location
//! finishes a conversion cycle, the ADC_INT0 interrupt will be set.
//!
//! \param mask is the bit mask of interrupts to enable.
//!        Valid values are a bitwise OR of the following values:
//!        - \b ADC_INT0 through ADC_INT31
//!        - \b ADC_IN_INT - Interrupt enable for a conversion in the result
//!                          register is either greater than the ADCLO or
//!                          lower than the ADCHI threshold.
//!        - \b ADC_LO_INT - Interrupt enable for the falling short of the
//!                          lower limit interrupt of the window comparator for
//!                          the result register.
//!        - \b ADC_HI_INT - Interrupt enable for the exceeding the upper
//!                          limit of the window comparator for the result
//!                          register.
//!        - \b ADC_OV_INT - Interrupt enable for a conversion that is about
//!                          to save to a memory buffer that has not been read
//!                          out yet.
//!        - \b ADC_TOV_INT -Interrupt enable for a conversion that is about
//!                          to start before the previous conversion has been
//!                          completed.
//!        - \b ADC_RDY_INT -Interrupt enable for the local buffered reference
//!                          ready signal.
//!
//!
//! \return NONE
//
//*****************************************************************************
extern void ADC14_enableInterrupt(uint_fast64_t mask);

//*****************************************************************************
//
//!
//! Disables the indicated ADCC interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor. The ADC_INT0 through ADC_INT31
//! parameters correspond to a completion event of the corresponding memory
//! location. For example, when the ADC_MEM0 location finishes a conversion
//! cycle, the ADC_INT0 interrupt will be set.
//!
//! \param mask is the bit mask of interrupts to disable.
//!        Valid values are a bitwise OR of the following values:
//!        - \b ADC_INT0 through ADC_INT31
//!        - \b ADC_IN_INT - Interrupt enable for a conversion in the result
//!                          register is either greater than the ADCLO or
//!                          lower than the ADCHI threshold.
//!        - \b ADC_LO_INT - Interrupt enable for the falling short of the
//!                          lower limit interrupt of the window comparator for
//!                          the result register.
//!        - \b ADC_HI_INT - Interrupt enable for the exceeding the upper
//!                          limit of the window comparator for the result
//!                          register.
//!        - \b ADC_OV_INT - Interrupt enable for a conversion that is about
//!                          to save to a memory buffer that has not been read
//!                          out yet.
//!        - \b ADC_TOV_INT -Interrupt enable for a conversion that is about
//!                          to start before the previous conversion has been
//!                          completed.
//!        - \b ADC_RDY_INT -Interrupt enable for the local buffered reference
//!                          ready signal.
//!
//!
//! \return NONE
//
//*****************************************************************************
extern void ADC14_disableInterrupt(uint_fast64_t mask);

//*****************************************************************************
//
//!
//! Returns the status of a the ADC interrupt register. The ADC_INT0
//! through ADC_INT31 parameters correspond to a completion event of the
//! corresponding memory location. For example, when the ADC_MEM0 location
//! finishes a conversion cycle, the ADC_INT0 interrupt will be set.
//!
//! \return The interrupt status. Value is a bitwise OR of the following values:
//!        - \b ADC_INT0 through ADC_INT31
//!        - \b ADC_IN_INT - Interrupt enable for a conversion in the result
//!                          register is either greater than the ADCLO or
//!                          lower than the ADCHI threshold.
//!        - \b ADC_LO_INT - Interrupt enable for the falling short of the
//!                          lower limit interrupt of the window comparator for
//!                          the result register.
//!        - \b ADC_HI_INT - Interrupt enable for the exceeding the upper
//!                          limit of the window comparator for the result
//!                          register.
//!        - \b ADC_OV_INT - Interrupt enable for a conversion that is about
//!                          to save to a memory buffer that has not been read
//!                          out yet.
//!        - \b ADC_TOV_INT -Interrupt enable for a conversion that is about
//!                          to start before the previous conversion has been
//!                          completed.
//!        - \b ADC_RDY_INT -Interrupt enable for the local buffered reference
//!                          ready signal.
//!
//!
//
//*****************************************************************************
extern uint_fast64_t ADC14_getInterruptStatus(void);

//*****************************************************************************
//
//!
//! Returns the status of a the ADC interrupt register masked with the
//! enabled interrupts. This function is useful to call in ISRs to get a list
//! of pending interrupts that are actually enabled and could have caused the
//! ISR. The ADC_INT0 through ADC_INT31 parameters correspond to a
//! completion event of the corresponding memory location. For example,
//! when the ADC_MEM0 location finishes a conversion cycle, the ADC_INT0
// !interrupt will be set.
//!
//! \return The interrupt status. Value is a bitwise OR of the following values:
//!        - \b ADC_INT0 through ADC_INT31
//!        - \b ADC_IN_INT - Interrupt enable for a conversion in the result
//!                          register is either greater than the ADCLO or
//!                          lower than the ADCHI threshold.
//!        - \b ADC_LO_INT - Interrupt enable for the falling short of the
//!                          lower limit interrupt of the window comparator for
//!                          the result register.
//!        - \b ADC_HI_INT - Interrupt enable for the exceeding the upper
//!                          limit of the window comparator for the result
//!                          register.
//!        - \b ADC_OV_INT - Interrupt enable for a conversion that is about
//!                          to save to a memory buffer that has not been read
//!                          out yet.
//!        - \b ADC_TOV_INT -Interrupt enable for a conversion that is about
//!                          to start before the previous conversion has been
//!                          completed.
//!        - \b ADC_RDY_INT -Interrupt enable for the local buffered reference
//!                          ready signal.
//!
//!
//
//*****************************************************************************
extern uint_fast64_t ADC14_getEnabledInterruptStatus(void);

//*****************************************************************************
//
//!
//! Clears the indicated ADCC interrupt sources.
//!
//! \param mask is the bit mask of interrupts to clear. The ADC_INT0
//! through ADC_INT31 parameters correspond to a completion event of the
//! corresponding memory location. For example, when the ADC_MEM0 location
//! finishes a conversion cycle, the ADC_INT0 interrupt will be set.
//!        Valid values are a bitwise OR of the following values:
//!        - \b ADC_INT0 through ADC_INT31
//!        - \b ADC_IN_INT - Interrupt enable for a conversion in the result
//!                          register is either greater than the ADCLO or
//!                          lower than the ADCHI threshold.
//!        - \b ADC_LO_INT - Interrupt enable for the falling short of the
//!                          lower limit interrupt of the window comparator for
//!                          the result register.
//!        - \b ADC_HI_INT - Interrupt enable for the exceeding the upper
//!                          limit of the window comparator for the result
//!                          register.
//!        - \b ADC_OV_INT - Interrupt enable for a conversion that is about
//!                          to save to a memory buffer that has not been read
//!                          out yet.
//!        - \b ADC_TOV_INT -Interrupt enable for a conversion that is about
//!                          to start before the previous conversion has been
//!                          completed.
//!        - \b ADC_RDY_INT -Interrupt enable for the local buffered reference
//!                          ready signal.
//!
//!
//! \return NONE
//
//*****************************************************************************
extern void ADC14_clearInterruptFlag(uint_fast64_t mask);

//*****************************************************************************
//
//!
//! Registers an interrupt handler for the ADC interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the ADC
//!  interrupt occurs.
//!
//! This function registers the handler to be called when an ADC
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific ADC14 interrupts must be enabled
//! via ADC14_enableInterrupt().  It is the interrupt handler's responsibility
//! to clear the interrupt source via ADC14_clearInterruptFlag().
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void ADC14_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//!
//! Unregisters the interrupt handler for the ADCC module.
//!
//! This function unregisters the handler to be called when an ADCC
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void ADC14_unregisterInterrupt(void);

/* Defines for future devices that might have multiple instances */
#define ADC14_enableModuleMultipleInstance(a) ADC14_enableModule()
#define ADC14_disableModuleMultipleInstance(a) ADC14_disableModule()
#define ADC14_initModuleMultipleInstance(a,b,c,d,e) ADC14_initModule(b,c,d,e)
#define ADC14_setResolutionMultipleInstance(a,b) ADC14_setResolution(b)
#define ADC14_getResolutionMultipleInstance(a) ADC14_getResolution()
#define ADC14_setSampleHoldTriggerMultipleInstance(a,b,c) ADC14_setSampleHoldTrigger(b,c)
#define ADC14_setSampleHoldTimeMultipleInstance(a,b,c) ADC14_setSampleHoldTime(b,c)
#define ADC14_configureMultiSequenceModeMultipleInstance(a,b,c,d) ADC14_configureMultiSequenceMode(b,c,d)
#define ADC14_configureSingleSampleModeMultipleInstance(a,b,c) ADC14_configureSingleSampleMode(b,c)
#define ADC14_enableConversionMultipleInstance(a,b) ADC14_enableConversion(b)
#define ADC14_disableConversionMultipleInstance(a) ADC14_disableConversion()
#define ADC14_toggleConversionTriggerMultipleInstance(a) ADC14_toggleConversionTrigger()
#define ADC14_isBusyMultipleInstance(a) ADC14_isBusy()
#define ADC14_configureConversionMemoryMultipleInstance(a,b,c,d,e) ADC14_configureConversionMemory(b,c,d,e)
#define ADC14_enableComparatorWindowMultipleInstance(a,b,c) ADC14_enableComparatorWindow(b,c)
#define ADC14_disableComparatorWindowMultipleInstance(a,b) ADC14_disableComparatorWindow(b)
#define ADC14_setComparatorWindowValueMultipleInstance(a,b,c,d) ADC14_setComparatorWindowValue(b,c,d)
#define ADC14_setResultFormatMultipleInstance(a,b) ADC14_setResultFormat(b)
#define ADC14_getResultMultipleInstance(a,b) ADC14_getResult(b)
#define ADC14_getMultiSequenceResultMultipleInstance(a,b) ADC14_getMultiSequenceResult(b)
#define ADC14_getResultArrayMultipleInstance(a,b,c,d) ADC14_getResultArray(b,c,d)
#define ADC14_enableReferenceBurstMultipleInstance(a) ADC14_enableReferenceBurst()
#define ADC14_disableReferenceBurstMultipleInstance(a) ADC14_disableReferenceBurst()
#define ADC14_setPowerModeMultipleInstance(a,b) ADC14_setPowerMode(b)
#define ADC14_enableSampleTimerMultipleInstance(a,b) ADC14_enableSampleTimer(b)
#define ADC14_disableSampleTimerMultipleInstance(a) ADC14_disableSampleTimer()
#define ADC14_enableInterruptMultipleInstance(a,b) ADC14_enableInterrupt(b)
#define ADC14_disableInterruptMultipleInstance(a,b) ADC14_disableInterrupt(b)
#define ADC14_getInterruptStatusMultipleInstance(a) ADC14_getInterruptStatus()
#define ADC14_getEnabledInterruptStatusMultipleInstance(a) ADC14_getEnabledInterruptStatus()
#define ADC14_clearInterruptFlagMultipleInstance(a,b) ADC14_clearInterruptFlag(b)
#define ADC14_registerInterruptMultipleInstance(a,b) ADC14_registerInterrupt(b)
#define ADC14_unregisterInterruptMultipleInstance(a) ADC14_unregisterInterrupt()

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

#endif /* ADC14_H_ */
