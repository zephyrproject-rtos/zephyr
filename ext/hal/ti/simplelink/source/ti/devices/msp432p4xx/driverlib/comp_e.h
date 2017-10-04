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
#ifndef COMP_E_H_
#define COMP_E_H_

//*****************************************************************************
//
//! \addtogroup comp_api
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

/* Module defines for Comp */
#define COMP_E_CMSIS(x) ((COMP_E_Type *) x)

#define COMP_E_FILTEROUTPUT_OFF                                            0x00
#define COMP_E_FILTEROUTPUT_DLYLVL1         (COMP_E_CTL1_F + COMP_E_CTL1_FDLY_0)
#define COMP_E_FILTEROUTPUT_DLYLVL2         (COMP_E_CTL1_F + COMP_E_CTL1_FDLY_1)
#define COMP_E_FILTEROUTPUT_DLYLVL3         (COMP_E_CTL1_F + COMP_E_CTL1_FDLY_2)
#define COMP_E_FILTEROUTPUT_DLYLVL4         (COMP_E_CTL1_F + COMP_E_CTL1_FDLY_3)

#define COMP_E_INPUT0                                                    (0x01)
#define COMP_E_INPUT1                                                    (0x02)
#define COMP_E_INPUT2                                                    (0x04)
#define COMP_E_INPUT3                                                    (0x08)
#define COMP_E_INPUT4                                                    (0x10)
#define COMP_E_INPUT5                                                    (0x20)
#define COMP_E_INPUT6                                                    (0x40)
#define COMP_E_INPUT7                                                    (0x80)
#define COMP_E_INPUT8                                                   (0x100)
#define COMP_E_INPUT9                                                   (0x200)
#define COMP_E_INPUT10                                                  (0x400)
#define COMP_E_INPUT11                                                  (0x800)
#define COMP_E_INPUT12                                                 (0x1000)
#define COMP_E_INPUT13                                                 (0x2000)
#define COMP_E_INPUT14                                                 (0x4000)
#define COMP_E_INPUT15                                                 (0x8000)
#define COMP_E_VREF                                                      (0x9F)

#define COMP_E_NORMALOUTPUTPOLARITY                      (!(COMP_E_CTL1_OUTPOL))
#define COMP_E_INVERTEDOUTPUTPOLARITY                       (COMP_E_CTL1_OUTPOL)

#define COMP_E_REFERENCE_AMPLIFIER_DISABLED               (COMP_E_CTL2_CEREFL_0)
#define COMP_E_VREFBASE1_2V                               (COMP_E_CTL2_CEREFL_1)
#define COMP_E_VREFBASE2_0V                               (COMP_E_CTL2_CEREFL_2)
#define COMP_E_VREFBASE2_5V                               (COMP_E_CTL2_CEREFL_3)

#define COMP_E_ACCURACY_STATIC                             (!COMP_E_CTL2_REFACC)
#define COMP_E_ACCURACY_CLOCKED                             (COMP_E_CTL2_REFACC)

#define COMP_E_HIGH_SPEED_MODE                             (COMP_E_CTL1_PWRMD_0)
#define COMP_E_NORMAL_MODE                                 (COMP_E_CTL1_PWRMD_1)
#define COMP_E_ULTRA_LOW_POWER_MODE                        (COMP_E_CTL1_PWRMD_2)

#define COMP_E_OUTPUT_INTERRUPT                                  (COMP_E_INT_IE)
#define COMP_E_INVERTED_POLARITY_INTERRUPT                      (COMP_E_INT_IIE)
#define COMP_E_READY_INTERRUPT                                (COMP_E_INT_RDYIE)

#define COMP_E_OUTPUT_INTERRUPT_FLAG                            (COMP_E_INT_IFG)
#define COMP_E_INTERRUPT_FLAG_INVERTED_POLARITY                (COMP_E_INT_IIFG)
#define COMP_E_INTERRUPT_FLAG_READY                          (COMP_E_INT_RDYIFG)

#define COMP_E_FALLINGEDGE                                  (!(COMP_E_CTL1_IES))
#define COMP_E_RISINGEDGE                                      (COMP_E_CTL1_IES)

#define COMP_E_LOW                                                         (0x0)
#define COMP_E_HIGH                                            (COMP_E_CTL1_OUT)

//*****************************************************************************
//
//!     ypedef COMP_E_Config
//! \brief Type definition for \link _COMP_E_Config \endlink structure
//!
//! \struct _COMP_E_Config
//! \brief Configuration structure for Comparator module. See
//!        \link COMP_E_initModule \endlink for parameter documentation.
//
//*****************************************************************************
typedef struct _COMP_E_Config
{
    uint_fast16_t positiveTerminalInput;
    uint_fast16_t negativeTerminalInput;
    uint_fast8_t outputFilterEnableAndDelayLevel;
    uint_fast8_t invertedOutputPolarity;
    uint_fast16_t powerMode;
} COMP_E_Config;

//*****************************************************************************
//
//! Initializes the Comparator Module.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param config Configuration structure for the Comparator module
//!
//! <hr>
//! <b>Configuration options for \link COMP_E_Config \endlink structure.</b>
//! <hr>
//!
//! \param positiveTerminalInput selects the input to the positive terminal.
//!        Valid values are
//!        - \b COMP_E_INPUT0 [Default]
//!        - \b COMP_E_INPUT1
//!        - \b COMP_E_INPUT2
//!        - \b COMP_E_INPUT3
//!        - \b COMP_E_INPUT4
//!        - \b COMP_E_INPUT5
//!        - \b COMP_E_INPUT6
//!        - \b COMP_E_INPUT7
//!        - \b COMP_E_INPUT8
//!        - \b COMP_E_INPUT9
//!        - \b COMP_E_INPUT10
//!        - \b COMP_E_INPUT11
//!        - \b COMP_E_INPUT12
//!        - \b COMP_E_INPUT13
//!        - \b COMP_E_INPUT14
//!        - \b COMP_E_INPUT15
//!        - \b COMP_E_VREF
//!        \n Modified bits are \b CEIPSEL and \b CEIPEN of \b CECTL0 register,
//!        \b CERSEL of \b CECTL2 register, and CEPDx of \b CECTL3 register.
//! \param negativeTerminalInput selects the input to the negative terminal.
//!        \n Valid values are:
//!        - \b COMP_E_INPUT0 [Default]
//!        - \b COMP_E_INPUT1
//!        - \b COMP_E_INPUT2
//!        - \b COMP_E_INPUT3
//!        - \b COMP_E_INPUT4
//!        - \b COMP_E_INPUT5
//!        - \b COMP_E_INPUT6
//!        - \b COMP_E_INPUT7
//!        - \b COMP_E_INPUT8
//!        - \b COMP_E_INPUT9
//!        - \b COMP_E_INPUT10
//!        - \b COMP_E_INPUT11
//!        - \b COMP_E_INPUT12
//!        - \b COMP_E_INPUT13
//!        - \b COMP_E_INPUT14
//!        - \b COMP_E_INPUT15
//!        - \b COMP_E_VREF
//!        \n Modified bits are \b CEIMSEL and \b CEIMEN of \b CECTL0 register,
//!        \b CERSEL of \b CECTL2 register, and CEPDx of \b CECTL3 register.
//! \param outputFilterEnableAndDelayLevel controls the output filter delay
//!       state, which is either off or enabled with a specified delay level.
//!        \n Valid values are
//!        - \b COMP_E_FILTEROUTPUT_OFF [Default]
//!        - \b COMP_E_FILTEROUTPUT_DLYLVL1
//!        - \b COMP_E_FILTEROUTPUT_DLYLVL2
//!        - \b COMP_E_FILTEROUTPUT_DLYLVL3
//!        - \b COMP_E_FILTEROUTPUT_DLYLVL4
//!        \n This parameter is device specific and delay levels should be found
//!        in the device's datasheet.
//!        \n Modified bits are \b CEF and \b CEFDLY of \b CECTL1 register.
//! \param invertedOutputPolarity controls if the output will be inverted or
//!        not. Valid values are
//!        - \b COMP_E_NORMALOUTPUTPOLARITY - indicates the output should be
//!             normal. [Default]
//!        - \b COMP_E_INVERTEDOUTPUTPOLARITY -  the output should be inverted.
//!        \n Modified bits are \b CEOUTPOL of \b CECTL1 register.
//! \param powerMode controls the power mode of the module
//!        - \b COMP_E_HIGH_SPEED_MODE [default]
//!        - \b COMP_E_NORMAL_MODE
//!        - \b COMP_E_ULTRA_LOW_POWER_MODE
//! Upon successful initialization of the Comparator module, this function will
//! have reset all necessary register bits and set the given options in the
//! registers. To actually use the comparator module, the COMP_E_enableModule()
//! function must be explicitly called before use.
//! If a Reference Voltage is set to a terminal, the Voltage should be set
//! using the COMP_E_setReferenceVoltage() function.
//!
//! \return true or false of the initialization process.
//
//*****************************************************************************
extern bool COMP_E_initModule(uint32_t comparator, const COMP_E_Config *config);

//*****************************************************************************
//
//! Generates a Reference Voltage to the terminal selected during
//! initialization.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param supplyVoltageReferenceBase decides the source and max amount of
//!       Voltage that can be used as a reference.
//!        Valid values are
//!        - \b COMP_E_REFERENCE_AMPLIFIER_DISABLED
//!        - \b COMP_E_VREFBASE1_2V
//!        - \b COMP_E_VREFBASE2_0V
//!        - \b COMP_E_VREFBASE2_5V
//! \param upperLimitSupplyVoltageFractionOf32 is the numerator of the
//!       equation to generate the reference voltage for the upper limit
//!       reference voltage. Valid values are between 1 and 32.
//! \param lowerLimitSupplyVoltageFractionOf32 is the numerator of the
//!       equation to generate the reference voltage for the lower limit
//!       reference voltage. Valid values are between 1 and 32.
//!  <br>Modified bits are \b CEREF0 of \b CECTL2 register.
//!
//! Use this function to generate a voltage to serve as a reference to the
//! terminal selected at initialization. The voltage is determined by the
//! equation: Vbase * (Numerator / 32). If the upper and lower limit voltage
//! numerators are equal, then a static reference is defined, whereas they are
//! different then a hysteresis effect is generated.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_setReferenceVoltage(uint32_t comparator,
        uint_fast16_t supplyVoltageReferenceBase,
        uint_fast16_t lowerLimitSupplyVoltageFractionOf32,
        uint_fast16_t upperLimitSupplyVoltageFractionOf32);

//*****************************************************************************
//
//! Sets the reference accuracy
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param referenceAccuracy is the reference accuracy setting of the
//!      comparator. Clocked is for low power/low accuracy.
//!      Valid values are
//!      - \b COMP_E_ACCURACY_STATIC
//!      - \b COMP_E_ACCURACY_CLOCKED
//!      <br>Modified bits are \b CEREFACC of \b CECTL2 register.
//!
//! The reference accuracy is set to the desired setting. Clocked is better for
//!  low power operations but has a lower accuracy.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_setReferenceAccuracy(uint32_t comparator,
        uint_fast16_t referenceAccuracy);

//*****************************************************************************
//
//! Sets the power mode
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param powerMode decides the power mode
//!        Valid values are
//!        - \b COMP_E_HIGH_SPEED_MODE
//!        - \b COMP_E_NORMAL_MODE
//!        - \b COMP_E_ULTRA_LOW_POWER_MODE
//!        <br>Modified bits are \b CEPWRMD of \b CECTL1 register.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_setPowerMode(uint32_t comparator, uint_fast16_t powerMode);

//*****************************************************************************
//
//! Turns on the Comparator module.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This function sets the bit that enables the operation of the
//! Comparator module.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_enableModule(uint32_t comparator);

//*****************************************************************************
//
//! Turns off the Comparator module.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This function clears the CEON bit disabling the operation of the Comparator
//! module, saving from excess power consumption.
//!
//! Modified bits are \b CEON of \b CECTL1 register.
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_disableModule(uint32_t comparator);

//*****************************************************************************
//
//! Shorts the two input pins chosen during initialization.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This function sets the bit that shorts the devices attached to the input
//! pins chosen from the initialization of the comparator.
//!
//! Modified bits are \b CESHORT of \b CECTL1 register.
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_shortInputs(uint32_t comparator);

//*****************************************************************************
//
//! Disables the short of the two input pins chosen during initialization.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This function clears the bit that shorts the devices attached to the input
//! pins chosen from the initialization of the comparator.
//!
//! Modified bits are \b CESHORT of \b CECTL1 register.
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_unshortInputs(uint32_t comparator);

//*****************************************************************************
//
//! Disables the input buffer of the selected input port to effectively allow
//! for analog signals.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param inputPort is the port in which the input buffer will be disabled.
//!        Valid values are a logical OR of the following:
//!        - \b COMP_E_INPUT0 [Default]
//!        - \b COMP_E_INPUT1
//!        - \b COMP_E_INPUT2
//!        - \b COMP_E_INPUT3
//!        - \b COMP_E_INPUT4
//!        - \b COMP_E_INPUT5
//!        - \b COMP_E_INPUT6
//!        - \b COMP_E_INPUT7
//!        - \b COMP_E_INPUT8
//!        - \b COMP_E_INPUT9
//!        - \b COMP_E_INPUT10
//!        - \b COMP_E_INPUT11
//!        - \b COMP_E_INPUT12
//!        - \b COMP_E_INPUT13
//!        - \b COMP_E_INPUT14
//!        - \b COMP_E_INPUT15
//!       <br> Modified bits are \b CEPDx of \b CECTL3 register.
//!
//! This function sets the bit to disable the buffer for the specified input
//! port to allow for analog signals from any of the comparator input pins. This
//! bit is automatically set when the input is initialized to be used with the
//! comparator module. This function should be used whenever an analog input is
//! connected to one of these pins to prevent parasitic voltage from causing
//! unexpected results.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_disableInputBuffer(uint32_t comparator,
        uint_fast16_t inputPort);

//*****************************************************************************
//
//! Enables the input buffer of the selected input port to allow for digital
//! signals.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param inputPort is the port in which the input buffer will be enabled.
//!        Valid values are a logical OR of the following:
//!        - \b COMP_E_INPUT0 [Default]
//!        - \b COMP_E_INPUT1
//!        - \b COMP_E_INPUT2
//!        - \b COMP_E_INPUT3
//!        - \b COMP_E_INPUT4
//!        - \b COMP_E_INPUT5
//!        - \b COMP_E_INPUT6
//!        - \b COMP_E_INPUT7
//!        - \b COMP_E_INPUT8
//!        - \b COMP_E_INPUT9
//!        - \b COMP_E_INPUT10
//!        - \b COMP_E_INPUT11
//!        - \b COMP_E_INPUT12
//!        - \b COMP_E_INPUT13
//!        - \b COMP_E_INPUT14
//!        - \b COMP_E_INPUT15
//!      <br> Modified bits are \b CEPDx of \b CECTL3 register.
//!
//! This function clears the bit to enable the buffer for the specified input
//! port to allow for digital signals from any of the comparator input pins.
//! This should not be reset if there is an analog signal connected to the
//! specified input pin to prevent from unexpected results.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_enableInputBuffer(uint32_t comparator,
        uint_fast16_t inputPort);

//*****************************************************************************
//
//! Toggles the bit that swaps which terminals the inputs go to, while also
//! inverting the output of the comparator.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \ bCOMP_E0
//!         - \ bCOMP_E1
//!
//! This function toggles the bit that controls which input goes to which
//! terminal. After initialization, this bit is set to 0, after toggling it once
//! the inputs are routed to the opposite terminal and the output is inverted.
//!
//! Modified bits are \b CEEX of \b CECTL1 register.
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_swapIO(uint32_t comparator);

//*****************************************************************************
//
//! Returns the output value of the Comparator module.
//!
//! \param comparator is the instance of the Comparator module. Valid parameters
//! vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! Returns the output value of the Comparator module.
//!
//! \return COMP_E_HIGH or COMP_E_LOW as the output value of the Comparator
//!          module.
//
//*****************************************************************************
extern uint8_t COMP_E_outputValue(uint32_t comparator);

//*****************************************************************************
//
//! Enables selected Comparator interrupt sources.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param mask is the bit mask of the interrupt sources to be enabled.
//!        Mask value is the logical OR of any of the following
//!        - \b COMP_E_OUTPUT_INTERRUPT - Output interrupt
//!        - \b COMP_E_INVERTED_POLARITY_INTERRUPT - Output interrupt inverted
//!                                                 polarity
//!        - \b COMP_E_READY_INTERRUPT - Ready interrupt
//!
//! Enables the indicated Comparator interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor. The default trigger for the non-inverted
//! interrupt is a rising edge of the output, this can be changed with the
//! interruptSetEdgeDirection() function.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_enableInterrupt(uint32_t comparator, uint_fast16_t mask);

//*****************************************************************************
//
//! Disables selected Comparator interrupt sources.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param mask is the bit mask of the interrupt sources to be disabled.
//!        Mask value is the logical OR of any of the following
//!        - \b COMP_E_OUTPUT_INTERRUPT - Output interrupt
//!        - \b COMP_E_INVERTED_POLARITY_INTERRUPT - Output interrupt inverted
//!                                                 polarity
//!        - \b COMP_E_READY_INTERRUPT - Ready interrupt
//!
//! Disables the indicated Comparator interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_disableInterrupt(uint32_t comparator, uint_fast16_t mask);

//*****************************************************************************
//
//! Clears Comparator interrupt flags.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param mask is a bit mask of the interrupt sources to be cleared.
//!        Mask value is the logical OR of any of the following
//!        - \b COMP_E_INTERRUPT_FLAG - Output interrupt flag
//!        - \b COMP_E_INTERRUPT_FLAG_INVERTED_POLARITY - Output interrupt flag
//!                                                     inverted polarity
//!        - \b COMP_E_INTERRUPT_FLAG_READY - Ready interrupt flag
//!
//! The Comparator interrupt source is cleared, so that it no longer asserts.
//! The highest interrupt flag is automatically cleared when an interrupt vector
//! generator is used.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_clearInterruptFlag(uint32_t comparator, uint_fast16_t mask);

//*****************************************************************************
//
//! Gets the current Comparator interrupt status.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This returns the interrupt status for the Comparator module based on which
//! flag is passed.
//!
//! \return The current interrupt flag status for the corresponding mask.
//
//*****************************************************************************
extern uint_fast16_t COMP_E_getInterruptStatus(uint32_t comparator);

//*****************************************************************************
//
//! Enables selected Comparator interrupt sources masked with the enabled
//! interrupts. This function is useful to call in ISRs to get a list
//! of pending interrupts that are actually enabled and could have caused the
//! ISR.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! Enables the indicated Comparator interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor. The default trigger for the non-inverted
//! interrupt is a rising edge of the output, this can be changed with the
//! COMP_E_setInterruptEdgeDirection() function.
//!
//! \return NONE
//
//*****************************************************************************
extern uint_fast16_t COMP_E_getEnabledInterruptStatus(uint32_t comparator);

//*****************************************************************************
//
//! Explicitly sets the edge direction that would trigger an interrupt.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//! \param edgeDirection determines which direction the edge would have to go
//!       to generate an interrupt based on the non-inverted interrupt flag.
//!        Valid values are
//!        - \b COMP_E_FALLINGEDGE - sets the bit to generate an interrupt when
//!             the output of the comparator falls from HIGH to LOW if the
//!             normal interrupt bit is set(and LOW to HIGH if the inverted
//!             interrupt enable bit is set). [Default]
//!        - \b COMP_E_RISINGEDGE - sets the bit to generate an interrupt when the
//!             output of the comparator rises from LOW to HIGH if the normal
//!             interrupt bit is set(and HIGH to LOW if the inverted interrupt
//!             enable bit is set).
//!        <br>Modified bits are \b CEIES of \b CECTL1 register.
//!
//! This function will set which direction the output will have to go, whether
//! rising or falling, to generate an interrupt based on a non-inverted
//! interrupt.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_setInterruptEdgeDirection(uint32_t comparator,
        uint_fast8_t edgeDirection);

//*****************************************************************************
//
//! Toggles the edge direction that would trigger an interrupt.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This function will toggle which direction the output will have to go,
//! whether rising or falling, to generate an interrupt based on a non-inverted
//! interrupt. If the direction was rising, it is now falling, if it was
//! falling, it is now rising.
//!
//! Modified bits are \b CEIES of \b CECTL1 register.
//!
//! \return NONE
//
//*****************************************************************************
extern void COMP_E_toggleInterruptEdgeDirection(uint32_t comparator);

//*****************************************************************************
//
//! Registers an interrupt handler for the Comparator E interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the
//! Comparator interrupt occurs.
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This function registers the handler to be called when a Comparator
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific Comparator interrupts must be enabled
//! via COMP_E_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via COMP_E_clearInterruptFlag().
//!
//! \return None.
//
//*****************************************************************************
extern void COMP_E_registerInterrupt(uint32_t comparator,
        void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the Comparator E interrupt
//!
//! \param comparator is the instance of the Comparator module. Valid
//! parameters vary from part to part, but can include:
//!         - \b COMP_E0_BASE
//!         - \b COMP_E1_BASE
//!
//! This function unregisters the handler to be called when Comparator E
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void COMP_E_unregisterInterrupt(uint32_t comparator);

/* Backwards Compatibility Layer */
#define COMP_E_enable(a) COMP_E_enableModule(a)
#define COMP_E_disable(a) COMP_E_disableModule(a)
#define COMP_E_IOSwap(a) COMP_E_swapIO(a)
#define COMP_E_interruptToggleEdgeDirection(a) COMP_E_toggleInterruptEdgeDirection(a)
#define COMP_E_clearInterrupt(a,b) COMP_E_clearInterruptFlag(a,b)

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


#endif /* COMP_E_H_ */
