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
#ifndef __CS_H__
#define __CS_H__

//*****************************************************************************
//
//! \addtogroup cs_api
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
#define CS_CLOCK_DIVIDER_1 CS_CTL1_DIVS_0
#define CS_CLOCK_DIVIDER_2 CS_CTL1_DIVS_1
#define CS_CLOCK_DIVIDER_4 CS_CTL1_DIVS_2
#define CS_CLOCK_DIVIDER_8 CS_CTL1_DIVS_3
#define CS_CLOCK_DIVIDER_16 CS_CTL1_DIVS_4
#define CS_CLOCK_DIVIDER_32 CS_CTL1_DIVS_5
#define CS_CLOCK_DIVIDER_64 CS_CTL1_DIVS_6
#define CS_CLOCK_DIVIDER_128 CS_CTL1_DIVS_7

#define CS_LFXTCLK_SELECT   CS_CTL1_SELM_0
#define CS_HFXTCLK_SELECT   CS_CTL1_SELM_5
#define CS_VLOCLK_SELECT    CS_CTL1_SELM_1
#define CS_REFOCLK_SELECT   CS_CTL1_SELM_2
#define CS_DCOCLK_SELECT    CS_CTL1_SELM_3
#define CS_MODOSC_SELECT    CS_CTL1_SELM_4

#define CS_KEY 0x695A

/* Number of positions to shift for divider calculation */
#define CS_ACLK_DIV_BITPOS 0x04
#define CS_MCLK_DIV_BITPOS 0x0C
#define CS_SMCLK_DIV_BITPOS 0x00
#define CS_HSMCLK_DIV_BITPOS 0x08

/* Number of positions to shift for source calculation */
#define CS_ACLK_SRC_BITPOS 0x08
#define CS_MCLK_SRC_BITPOS 0x00
#define CS_SMCLK_SRC_BITPOS 0x04
#define CS_HSMCLK_SRC_BITPOS 0x04

/* REFO Clock Values */
#define CS_REFO_32KHZ 0x00
#define CS_REFO_128KHZ 0x01

/* Frequency Values */
#define CS_VLOCLK_FREQUENCY 10000
#define CS_MODCLK_FREQUENCY 24000000

/* Interrupts */
#define CS_LFXT_FAULT   CS_IE_LFXTIE
#define CS_HFXT_FAULT   CS_IE_HFXTIE
#define CS_DCO_OPEN_FAULT   CS_IE_DCOR_OPNIE
#define CS_STARTCOUNT_LFXT_FAULT   CS_IE_FCNTLFIE
#define CS_STARTCOUNT_HFXT_FAULT   CS_IE_FCNTHFIE
#define CS_DCO_SHORT_FAULT   CS_IFG_DCOR_SHTIFG

#define CS_HFXT_DRIVE CS_CTL2_HFXTDRIVE
#define CS_HFXT_BYPASS CS_CTL2_HFXTBYPASS

#define CS_LFXT_DRIVE0 CS_CTL2_LFXTDRIVE_0
#define CS_LFXT_DRIVE1 CS_CTL2_LFXTDRIVE_1
#define CS_LFXT_DRIVE2 CS_CTL2_LFXTDRIVE_2
#define CS_LFXT_DRIVE3 CS_CTL2_LFXTDRIVE_3
#define CS_LFXT_BYPASS CS_CTL2_LFXTBYPASS

#define CS_ACLK CS_CLKEN_ACLK_EN
#define CS_MCLK CS_CLKEN_MCLK_EN
#define CS_SMCLK CS_CLKEN_SMCLK_EN
#define CS_HSMCLK CS_CLKEN_HSMCLK_EN
#define CS_BCLK CS_STAT_BCLK_READY

#define CS_LFXTCLK  0x01

#define CS_1MHZ 1000000
#define CS_15MHZ 1500000
#define CS_3MHZ 3000000
#define CS_4MHZ 4000000
#define CS_6MHZ 6000000
#define CS_8MHZ 8000000
#define CS_12MHZ 12000000
#define CS_16MHZ 16000000
#define CS_24MHZ 24000000
#define CS_32MHZ 32000000
#define CS_40MHZ 40000000
#define CS_48MHZ 48000000

#define CS_DCO_FREQUENCY_1_5 CS_CTL0_DCORSEL_0
#define CS_DCO_FREQUENCY_3 CS_CTL0_DCORSEL_1
#define CS_DCO_FREQUENCY_6 CS_CTL0_DCORSEL_2
#define CS_DCO_FREQUENCY_12 CS_CTL0_DCORSEL_3
#define CS_DCO_FREQUENCY_24 CS_CTL0_DCORSEL_4
#define CS_DCO_FREQUENCY_48 CS_CTL0_DCORSEL_5

#define CS_HFXT_FAULT_COUNTER 0x01
#define CS_LFXT_FAULT_COUNTER 0x02

#define CS_FAULT_COUNTER_4096_CYCLES CS_CTL3_FCNTLF_0
#define CS_FAULT_COUNTER_8192_CYCLES CS_CTL3_FCNTLF_1
#define CS_FAULT_COUNTER_16384_CYCLES CS_CTL3_FCNTLF_2
#define CS_FAULT_COUNTER_32768_CYCLES CS_CTL3_FCNTLF_3

#define CS_OVER32MHZ    0x01
#define CS_UNDER32MHZ   0x02

//******************************************************************************
//
//! This function sets the external clock sources LFXT and HFXT crystal
//! oscillator frequency values. This function must be called if an external
//! crystal LFXT or HFXT is used and the user intends to call
//! CS_getSMCLK, CS_getMCLK, CS_getBCLK,  CS_getHSMCLK, CS_getACLK and
//! any of the HFXT oscillator control functions
//!
//! \param lfxt_XT_CLK_frequency is the LFXT crystal frequencies in Hz
//! \param hfxt_XT_CLK_frequency is the HFXT crystal frequencies in Hz
//!
//! \return None
//
//******************************************************************************
extern void CS_setExternalClockSourceFrequency(uint32_t lfxt_XT_CLK_frequency,
        uint32_t hfxt_XT_CLK_frequency);

//******************************************************************************
//
//! This function initializes each of the clock signals. The user must ensure
//! that this function is called for each clock signal. If not, the default
//! state is assumed for the particular clock signal. Refer to DriverLib
//! documentation for CS module or Device Family User's Guide for details of
//! default clock signal states.
//!
//! Note that this function is blocking and will wait on the appropriate bit
//! to be set in the CSSTAT READY register to be set before setting the clock
//! source.
//!
//! Also note that when HSMCLK and SMCLK share the same clock signal. If you
//! change the clock signal for HSMCLK, the clock signal for SMCLK will change
//! also (and vice-versa).
//!
//! HFXTCLK is not available for BCLK or ACLK.
//!
//! \param selectedClockSignal Clock signal to initialize.
//!           - \b CS_ACLK,
//!           - \b CS_MCLK,
//!           - \b CS_HSMCLK
//!           - \b CS_SMCLK
//!           - \b CS_BCLK  [clockSourceDivider is ignored for this parameter]
//! \param clockSource  Clock source for the selectedClockSignal signal.
//!            - \b CS_LFXTCLK_SELECT,
//!            - \b CS_HFXTCLK_SELECT,
//!            - \b CS_VLOCLK_SELECT,     [Not available for BCLK]
//!            - \b CS_DCOCLK_SELECT,     [Not available for ACLK, BCLK]
//!            - \b CS_REFOCLK_SELECT,
//!            - \b CS_MODOSC_SELECT      [Not available for ACLK, BCLK]
//! \param clockSourceDivider - selected the clock divider to calculate
//!         clock signal from clock source. This parameter is ignored when
//!         setting BLCK. Valid values are:
//!           - \b CS_CLOCK_DIVIDER_1,
//!           - \b CS_CLOCK_DIVIDER_2,
//!           - \b CS_CLOCK_DIVIDER_4,
//!           - \b CS_CLOCK_DIVIDER_8,
//!           - \b CS_CLOCK_DIVIDER_16,
//!           - \b CS_CLOCK_DIVIDER_32,
//!           - \b CS_CLOCK_DIVIDER_64,
//!           - \b CS_CLOCK_DIVIDER_128
//!
//! \return NONE
//
//******************************************************************************
extern void CS_initClockSignal(uint32_t selectedClockSignal,
        uint32_t clockSource, uint32_t clockSourceDivider);

//******************************************************************************
//
//! Initializes the HFXT crystal oscillator, which supports crystal frequencies
//! between 0 MHz and 48 MHz, depending on the selected drive strength. Loops
//! until all oscillator fault flags are cleared, with no timeout. See the
//! device-specific data sheet for appropriate drive settings. NOTE: User must
//! call CS_setExternalClockSourceFrequency to set frequency of external clocks
//! before calling this function.
//!
//! \param bypassMode When this variable is set, the oscillator will start
//! in bypass mode and the signal can be generated by a digital square wave.
//!
//! \return true if started correctly, false otherwise
//
//******************************************************************************
extern bool CS_startHFXT(bool bypassMode);

//******************************************************************************
//
//! Initializes the HFXT crystal oscillator, which supports crystal frequencies
//! between 0 MHz and 48 MHz, depending on the selected drive strength. Loops
//! until all oscillator fault flags are cleared, with no timeout. See the
//! device-specific data sheet for appropriate drive settings. NOTE: User must
//! call CS_setExternalClockSourceFrequency to set frequency of external clocks
//! before calling this function. This function has a timeout associated with
//! stabilizing the oscillator.
//!
//! \param bypassMode When this variable is set, the oscillator will start
//! in bypass mode and the signal can be generated by a digital square wave.
//!
//! \param timeout is the count value that gets decremented every time the loop
//!         that clears oscillator fault flags gets executed.
//!
//! \return true if started correctly, false otherwise
//
//******************************************************************************
extern bool CS_startHFXTWithTimeout(bool bypassMode, uint32_t timeout);

//******************************************************************************
//
//! Initializes the LFXT crystal oscillator, which supports crystal frequencies
//! up to 50kHz, depending on the selected drive strength. Loops
//! until all oscillator fault flags are cleared, with no timeout. See the
//! device-specific data sheet for appropriate drive settings. NOTE: User must
//! call CS_setExternalClockSourceFrequency to set frequency of external clocks
//! before calling this function.
//!
//! \param xtDrive is the target drive strength for the LFXT crystal
//!                  oscillator.
//!      Valid values are:
//!         - \b CS_LFXT_DRIVE0,
//!         - \b CS_LFXT_DRIVE1,
//!         - \b CS_LFXT_DRIVE2,
//!         - \b CS_LFXT_DRIVE3,  [Default Value]
//!         - \b CS_LFXT_BYPASS
//!
//! \note When CS_LFXT_BYPASS is passed as a parameter the oscillator will start
//! in bypass mode and the signal can be generated by a digital square wave.
//!
//! \return true if started correctly, false otherwise
//
//******************************************************************************
extern bool CS_startLFXT(uint32_t xtDrive);

//******************************************************************************
//
//! Initializes the LFXT crystal oscillator, which supports crystal frequencies
//! up to 50kHz, depending on the selected drive strength. Loops
//! until all oscillator fault flags are cleared. See the
//! device-specific data sheet for appropriate drive settings. NOTE: User must
//! call CS_setExternalClockSourceFrequency to set frequency of external clocks
//! before calling this function. This function has a timeout associated with
//! stabilizing the oscillator.
//!
//! \param xtDrive is the target drive strength for the LFXT crystal
//!                  oscillator.
//!      Valid values are:
//!         - \b CS_LFXT_DRIVE0,
//!         - \b CS_LFXT_DRIVE1,
//!         - \b CS_LFXT_DRIVE2,
//!         - \b CS_LFXT_DRIVE3,  [Default Value]
//!         - \b CS_LFXT_BYPASS
//!
//! \note When CS_LFXT_BYPASS is passed as a parameter the oscillator will
//! start in bypass mode and the signal can be generated by a digital square
//! wave.
//!
//! \param timeout is the count value that gets decremented every time the loop
//!         that clears oscillator fault flags gets executed.
//!
//! \return true if started correctly, false otherwise
//
//******************************************************************************
extern bool CS_startLFXTWithTimeout(uint32_t xtDrive, uint32_t timeout);

//******************************************************************************
//
//! Selects between the frequency of the internal REFO clock source
//!
//! \param referenceFrequency selects between the valid frequencies:
//!        - \b CS_REFO_32KHZ,
//!        - \b CS_REFO_128KHZ,
//!
//! \return NONE
//
//******************************************************************************
extern void CS_setReferenceOscillatorFrequency(uint8_t referenceFrequency);

//******************************************************************************
//
//! Enables conditional module requests
//!
//! \param selectClock selects specific request enables. Valid values are
//!        are a logical OR of the following values:
//!        - \b CS_ACLK,
//!        - \b CS_HSMCLK,
//!        - \b CS_SMCLK,
//!        - \b CS_MCLK
//!
//! \return NONE
//
//******************************************************************************
extern void CS_enableClockRequest(uint32_t selectClock);

//******************************************************************************
//
//! Disables conditional module requests
//!
//! \param selectClock selects specific request disables. Valid values are
//!        are a logical OR of the following values:
//!        - \b CS_ACLK,
//!        - \b CS_HSMCLK,
//!        - \b CS_SMCLK,
//!        - \b CS_MCLK
//!
//! \return NONE
//
//******************************************************************************
extern void CS_disableClockRequest(uint32_t selectClock);

//******************************************************************************
//
//! Get the current ACLK frequency.
//!
//! If a oscillator fault is set, the frequency returned will be based on the
//! fail safe mechanism of CS module. The user of this API must ensure that
//! \link CS_setExternalClockSourceFrequency() \endlink API was invoked before
//! in case LFXT is being used.
//!
//! \return Current ACLK frequency in Hz
//
//******************************************************************************
extern uint32_t CS_getACLK(void);

//******************************************************************************
//
//! Get the current SMCLK frequency.
//!
//! If a oscillator fault is set, the frequency returned will be based on the
//! fail safe mechanism of CS module. The user of this API must ensure that
//! CS_setExternalClockSourceFrequency API was invoked before in case LFXT or
//! HFXT is being used.
//!
//! \return Current SMCLK frequency in Hz
//
//******************************************************************************
extern uint32_t CS_getSMCLK(void);

//******************************************************************************
//
//! Get the current MCLK frequency.
//!
//! If a oscillator fault is set, the frequency returned will be based on the
//! fail safe mechanism of CS module. The user of this API must ensure that
//! CS_setExternalClockSourceFrequency API was invoked before in case LFXT or
//! HFXT is being used.
//!
//! \return Current MCLK frequency in Hz
//
//******************************************************************************
extern uint32_t CS_getMCLK(void);

//******************************************************************************
//
//! Get the current BCLK frequency.
//!
//! If a oscillator fault is set, the frequency returned will be based on the
//! fail safe mechanism of CS module. The user of this API must ensure that
//! \link CS_setExternalClockSourceFrequency \endlink API was invoked before in
//! case LFXT or HFXT is being used.
//!
//! \return Current BCLK frequency in Hz
//
//******************************************************************************
extern uint32_t CS_getBCLK(void);

//******************************************************************************
//
//! Get the current HSMCLK frequency.
//!
//! If a oscillator fault is set, the frequency returned will be based on the
//! fail safe mechanism of CS module. The user of this API must ensure that
//! \link CS_setExternalClockSourceFrequency \endlink API was invoked before in
//! case LFXT or HFXT is being used.
//!
//! \return Current HSMCLK frequency in Hz
//
//******************************************************************************
extern uint32_t CS_getHSMCLK(void);

//******************************************************************************
//
//! Sets the centered frequency of DCO operation.  Each frequency represents
//! the centred frequency of a particular frequency range. Further tuning can
//! be achieved by using the CS_tuneDCOFrequency function. Note that setting
//! the nominal frequency will reset the tuning parameters.
//!
//! \param dcoFreq selects between the valid frequencies:
//!        - \b CS_DCO_FREQUENCY_1_5, [1MHz to 2MHz]
//!        - \b CS_DCO_FREQUENCY_3,   [2MHz to 4MHz]
//!        - \b CS_DCO_FREQUENCY_6,   [4MHz to 8MHz]
//!        - \b CS_DCO_FREQUENCY_12,  [8MHz to 16MHz]
//!        - \b CS_DCO_FREQUENCY_24,  [16MHz to 32MHz]
//!        - \b CS_DCO_FREQUENCY_48   [32MHz to 64MHz]
//!
//! \return NONE
//
//******************************************************************************
extern void CS_setDCOCenteredFrequency(uint32_t dcoFreq);

//******************************************************************************
//
//! Automatically sets/tunes the DCO to the given frequency. Any valid value
//! up to max frequency in the spec can be given to this function and the API
//! will do its best to determine the correct tuning parameter.
//!
//! \note The frequency ranges that can be custom tuned on early release MSP432
//! devices is limited. For further details on supported tunable frequencies,
//! please refer to the device errata sheet or data sheet.
//!
//! \param dcoFrequency Frequency in Hz that the user wants to set the DCO to.
//!
//! \note This function uses floating point math to calculate the DCO tuning
//!         parameter. If efficiency is a concern, the user should use the
//!         \link FPU_enableModule \endlink function (if available) to enable
//!         the floating point co-processor.
//!
//! \return None
//
//******************************************************************************
extern void CS_setDCOFrequency(uint32_t dcoFrequency);

//******************************************************************************
//
//! Tunes the DCO to a specific frequency. Tuning of the DCO is based off of the
//! following equation in the user's guide:
//!
//! See the user's guide for more detailed information about DCO tuning.
//!
//! \note This function is not currently available on pre-release MSP432 devices.
//!  On early release versions of MSP432, the DCO calibration information has not been
//!  populated making the DCO only able to operate at the pre-calibrated centered
//!  frequencies accessible by the \link CS_setDCOCenteredFrequency \endlink
//!  function. While this function will be added on the final devices being released,
//!  for early silicon please default to the pre-calibrated DCO center frequencies.
//!
//! \param tuneParameter Tuning parameter in 2's Compliment representation.
//!  Can be negative or positive.
//!
//! \return NONE
//
//******************************************************************************
extern void CS_tuneDCOFrequency(int16_t tuneParameter);

//******************************************************************************
//
//! Enables the external resistor for DCO operation
//!
//! \return NONE
//
//******************************************************************************
extern void CS_enableDCOExternalResistor(void);

//******************************************************************************
//
//! Disables the external resistor for DCO operation
//!
//! \return NONE
//
//******************************************************************************
extern void CS_disableDCOExternalResistor(void);

//******************************************************************************
//
//! Sets the calibration value for the DCO when using the external resistor
//! mode. This value is used for tuning the DCO to custom frequencies. By
//! default, the value in the CS module is populated by the calibration
//! data of the suggested external resistor (see device datasheet).
//!
//! \param calData is the calibration data constant for the external resistor.
//!
//! \param freqRange is the range of the DCO to set the external calibration
//!             for. Frequencies above 32MHZ have a different calibration value
//!             than frequencies below 32MHZ.
//!
//! \return None
//
//******************************************************************************
extern void CS_setDCOExternalResistorCalibration(uint_fast8_t uiCalData,
                                                 uint_fast8_t freqRange);

//******************************************************************************
//
//! Gets the current tuned DCO frequency. If no tuning has been done, this
//! returns the nominal DCO frequency of the current DCO range. Note that this
//! function will grab any constant/calibration data from the DDDS table
//! without any user interaction needed.
//!
//! \note This function uses floating point math to calculate the DCO tuning
//!         parameter. If efficiency is a concern, the user should use the
//!         \link FPU_enableModule \endlink function (if available) to enable
//!         the floating point co-processor.
//!
//! \return Current DCO frequency in Hz
//
//******************************************************************************
extern uint32_t CS_getDCOFrequency(void);

//******************************************************************************
//
//! Automatically sets/tunes the DCO to the given frequency. Any valid value
//! up to (and including) 64Mhz can be given to this function and the API
//! will do its best to determine the correct tuning parameter.
//!
//!
//! \note This function is not currently available on pre-release MSP432 devices.
//!  On early release versions of MSP432, the DCO calibration information has not been
//!  populated making the DCO only able to operate at the pre-calibrated centered
//!  frequencies accessible by the \link CS_setDCOCenteredFrequency \endlink
//!  function. While this function will be added on the final devices being released,
//!  for early silicon please default to the pre-calibrated DCO center frequencies.
//!
//! \param dcoFrequency Frequency in Hz (1500000 - 64000000) that the user wants
//!         to set the DCO to.
//!
//! \note This function uses floating point math to calculate the DCO tuning
//!         parameter. If efficiency is a concern, the user should use the
//!         \link FPU_enableModule \endlink function (if available) to enable
//!         the floating point co-processor.
//!
//! \return None
//
//******************************************************************************
extern void CS_setDCOFrequency(uint32_t dcoFrequency);

//******************************************************************************
//
//! Enables the fault counter for the CS module. This function can enable
//! either the HFXT fault counter or the LFXT fault counter.
//!
//! \param counterSelect selects the fault counter to enable
//!        - \b CS_HFXT_FAULT_COUNTER
//!        - \b CS_LFXT_FAULT_COUNTER
//!
//! \return NONE
//
//******************************************************************************
extern void CS_enableFaultCounter(uint_fast8_t counterSelect);

//******************************************************************************
//
//! Disables the fault counter for the CS module. This function can disable
//! either the HFXT fault counter or the LFXT fault counter.
//!
//! \param counterSelect selects the fault counter to disable
//!        - \b CS_HFXT_FAULT_COUNTER
//!        - \b CS_LFXT_FAULT_COUNTER
//!
//! \return NONE
//
//******************************************************************************
extern void CS_disableFaultCounter(uint_fast8_t counterSelect);

//******************************************************************************
//
//! Resets the fault counter for the CS module. This function can reset
//! either the HFXT fault counter or the LFXT fault counter.
//!
//! \param counterSelect selects the fault counter to reset
//!        - \b CS_HFXT_FAULT_COUNTER
//!        - \b CS_LFXT_FAULT_COUNTER
//!
//! \return NONE
//
//******************************************************************************
extern void CS_resetFaultCounter(uint_fast8_t counterSelect);

//******************************************************************************
//
//! Sets the count for the start value of the fault counter. This function can
//! be used to set either the HFXT count or the LFXT count.
//!
//! \param counterSelect selects the fault counter to reset
//!        - \b CS_HFXT_FAULT_COUNTER
//!        - \b CS_LFXT_FAULT_COUNTER
//! \param countValue selects the cycles to set the fault counter to
//!        - \b CS_FAULT_COUNTER_4096_CYCLES
//!        - \b CS_FAULT_COUNTER_8192_CYCLES
//!        - \b CS_FAULT_COUNTER_16384_CYCLES
//!        - \b CS_FAULT_COUNTER_32768_CYCLES
//!
//! \return NONE
//
//******************************************************************************
extern void CS_startFaultCounter(uint_fast8_t counterSelect,
        uint_fast8_t countValue);

//*****************************************************************************
//
//! Enables individual clock control interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be enabled.  Must
//! be a logical OR of:
//!                     - \b CS_LFXT_FAULT,
//!                     - \b CS_HFXT_FAULT,
//!                     - \b CS_DCOMIN_FAULT,
//!                     - \b CS_DCOMAX_FAULT,
//!                     - \b CS_DCO_OPEN_FAULT,
//!                     - \b CS_STARTCOUNT_LFXT_FAULT,
//!                     - \b CS_STARTCOUNT_HFXT_FAULT,
//!
//! This function enables the indicated clock system interrupt sources.  Only
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
extern void CS_enableInterrupt(uint32_t flags);

//*****************************************************************************
//
//! Disables individual clock system interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be disabled.  Must
//! be a logical OR of:
//!                     - \b CS_LFXT_FAULT,
//!                     - \b CS_HFXT_FAULT,
//!                     - \b CS_DCOMIN_FAULT,
//!                     - \b CS_DCOMAX_FAULT,
//!                     - \b CS_DCO_OPEN_FAULT,
//!                     - \b CS_STARTCOUNT_LFXT_FAULT,
//!                     - \b CS_STARTCOUNT_HFXT_FAULT,
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//!
//! \return None.
//
//*****************************************************************************
extern void CS_disableInterrupt(uint32_t flags);

//*****************************************************************************
//
//! Gets the current interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list of pending interrupts
//! that are actually enabled and could have caused the ISR.
//!
//! \return The current interrupt status, enumerated as a bit field of
//!                     - \b CS_LFXT_FAULT,
//!                     - \b CS_HFXT_FAULT,
//!                     - \b CS_DCO_OPEN_FAULT,
//!                     - \b CS_DCO_SHORT_FAULT,
//!                     - \b CS_STARTCOUNT_LFXT_FAULT,
//!                     - \b CS_STARTCOUNT_HFXT_FAULT,
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//
//*****************************************************************************
extern uint32_t CS_getEnabledInterruptStatus(void);

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \return The current interrupt status, enumerated as a bit field of:
//!                     - \b CS_LFXT_FAULT,
//!                     - \b CS_HFXT_FAULT,
//!                     - \b CS_DCO_OPEN_FAULT,
//!                     - \b CS_DCO_SHORT_FAULT,
//!                     - \b CS_STARTCOUNT_LFXT_FAULT,
//!                     - \b CS_STARTCOUNT_HFXT_FAULT,
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//
//*****************************************************************************
extern uint32_t CS_getInterruptStatus(void);

//*****************************************************************************
//
//! Clears clock system interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be cleared.  Must
//! be a logical OR of:
//!                     - \b CS_LFXT_FAULT,
//!                     - \b CS_HFXT_FAULT,
//!                     - \b CS_DCO_OPEN_FAULT,
//!                     - \b CS_STARTCOUNT_LFXT_FAULT,
//!                     - \b CS_STARTCOUNT_HFXT_FAULT,
//!
//! The specified clock system interrupt sources are cleared, so that they no
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
extern void CS_clearInterruptFlag(uint32_t flags);

//*****************************************************************************
//
//! Registers an interrupt handler for the clock system interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the clock
//! system interrupt occurs.
//!
//! This function registers the handler to be called when a clock system
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific clock system interrupts must be enabled
//! via CS_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via CS_clearInterruptFlag().
//!
//! Clock System can generate interrupts when
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void CS_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the clock system.
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
extern void CS_unregisterInterrupt(void);

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

#endif
