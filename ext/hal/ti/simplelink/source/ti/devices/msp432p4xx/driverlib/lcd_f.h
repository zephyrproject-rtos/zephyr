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
#ifndef LCD_F_H_
#define LCD_F_H_

#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

/* Define to ensure that our current MSP432 has the LCD_F module. This
    definition is included in the device specific header file */
#ifdef __MCU_HAS_LCD_F__

//*****************************************************************************
//
//! \addtogroup lcd_f_api
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

typedef struct LCD_F_initParam
{
    //! Selects the clock that will be used by the LCD_F_E.
    //! \n Valid values are:
    //! - \b LCD_F_CLOCKSOURCE_XTCLK [Default]
    //! - \b LCD_F_CLOCKSOURCE_ACLK [Default]
    //! - \b LCD_F_CLOCKSOURCE_VLOCLK
    //! - \b LCD_F_CLOCKSOURCE_LFXT
    uint32_t clockSource;
    //! Selects the divider for LCD frequency.
    //! \n Valid values are:
    //! - \b LCD_F_CLOCKDIVIDER_1 [Default]
    //! - \b LCD_F_CLOCKDIVIDER_2
    //! - \b LCD_F_CLOCKDIVIDER_3
    //! - \b LCD_F_CLOCKDIVIDER_4
    //! - \b LCD_F_CLOCKDIVIDER_5
    //! - \b LCD_F_CLOCKDIVIDER_6
    //! - \b LCD_F_CLOCKDIVIDER_7
    //! - \b LCD_F_CLOCKDIVIDER_8
    //! - \b LCD_F_CLOCKDIVIDER_9
    //! - \b LCD_F_CLOCKDIVIDER_10
    //! - \b LCD_F_CLOCKDIVIDER_11
    //! - \b LCD_F_CLOCKDIVIDER_12
    //! - \b LCD_F_CLOCKDIVIDER_13
    //! - \b LCD_F_CLOCKDIVIDER_14
    //! - \b LCD_F_CLOCKDIVIDER_15
    //! - \b LCD_F_CLOCKDIVIDER_16
    //! - \b LCD_F_CLOCKDIVIDER_17
    //! - \b LCD_F_CLOCKDIVIDER_18
    //! - \b LCD_F_CLOCKDIVIDER_19
    //! - \b LCD_F_CLOCKDIVIDER_20
    //! - \b LCD_F_CLOCKDIVIDER_21
    //! - \b LCD_F_CLOCKDIVIDER_22
    //! - \b LCD_F_CLOCKDIVIDER_23
    //! - \b LCD_F_CLOCKDIVIDER_24
    //! - \b LCD_F_CLOCKDIVIDER_25
    //! - \b LCD_F_CLOCKDIVIDER_26
    //! - \b LCD_F_CLOCKDIVIDER_27
    //! - \b LCD_F_CLOCKDIVIDER_28
    //! - \b LCD_F_CLOCKDIVIDER_29
    //! - \b LCD_F_CLOCKDIVIDER_30
    //! - \b LCD_F_CLOCKDIVIDER_31
    //! - \b LCD_F_CLOCKDIVIDER_32
    uint32_t clockDivider;
    //! Selects the prescaler
    //! \n Valid values are:
    //! - \b LCD_F_CLOCKPRESCALER_1 [Default]
    //! - \b LCD_F_CLOCKPRESCALER_2
    //! - \b LCD_F_CLOCKPRESCALER_4
    //! - \b LCD_F_CLOCKPRESCALER_8
    //! - \b LCD_F_CLOCKPRESCALER_16
    //! - \b LCD_F_CLOCKPRESCALER_32
    uint32_t clockPrescaler;
    //! Selects LCD_F_E mux rate.
    //! \n Valid values are:
    //! - \b LCD_F_STATIC [Default]
    //! - \b LCD_F_2_MUX
    //! - \b LCD_F_3_MUX
    //! - \b LCD_F_4_MUX
    //! - \b LCD_F_5_MUX
    //! - \b LCD_F_6_MUX
    //! - \b LCD_F_7_MUX
    //! - \b LCD_F_8_MUX
    uint32_t muxRate;
    //! Selects LCD waveform mode.
    //! \n Valid values are:
    //! - \b LCD_F_STANDARD_WAVEFORMS [Default]
    //! - \b LCD_F_LOW_POWER_WAVEFORMS
    uint32_t waveforms;
    //! Sets LCD segment on/off.
    //! \n Valid values are:
    //! - \b LCD_F_SEGMENTS_DISABLED [Default]
    //! - \b LCD_F_SEGMENTS_ENABLED
    uint32_t segments;
} LCD_F_Config;

//*****************************************************************************
//
// The following are values that can be passed to the initParams parameter for
// functions: LCD_F_initModule().
//
//*****************************************************************************
#define LCD_F_CLOCKSOURCE_ACLK                               (LCD_F_CTL_SSEL_0)
#define LCD_F_CLOCKSOURCE_VLOCLK                             (LCD_F_CTL_SSEL_1)
#define LCD_F_CLOCKSOURCE_REFOCLK                            (LCD_F_CTL_SSEL_2)
#define LCD_F_CLOCKSOURCE_LFXT                               (LCD_F_CTL_SSEL_3)


//*****************************************************************************
//
// The following are values that can be passed to the initParams parameter for
// functions: LCD_F_initModule().
//
//*****************************************************************************
#define LCD_F_CLOCKDIVIDER_1 0x0
#define LCD_F_CLOCKDIVIDER_2 0x800
#define LCD_F_CLOCKDIVIDER_3 0x1000
#define LCD_F_CLOCKDIVIDER_4 0x1800
#define LCD_F_CLOCKDIVIDER_5 0x2000
#define LCD_F_CLOCKDIVIDER_6 0x2800
#define LCD_F_CLOCKDIVIDER_7 0x3000
#define LCD_F_CLOCKDIVIDER_8 0x3800
#define LCD_F_CLOCKDIVIDER_9 0x4000
#define LCD_F_CLOCKDIVIDER_10 0x4800
#define LCD_F_CLOCKDIVIDER_11 0x5000
#define LCD_F_CLOCKDIVIDER_12 0x5800
#define LCD_F_CLOCKDIVIDER_13 0x6000
#define LCD_F_CLOCKDIVIDER_14 0x6800
#define LCD_F_CLOCKDIVIDER_15 0x7000
#define LCD_F_CLOCKDIVIDER_16 0x7800
#define LCD_F_CLOCKDIVIDER_17 0x8000
#define LCD_F_CLOCKDIVIDER_18 0x8800
#define LCD_F_CLOCKDIVIDER_19 0x9000
#define LCD_F_CLOCKDIVIDER_20 0x9800
#define LCD_F_CLOCKDIVIDER_21 0xa000
#define LCD_F_CLOCKDIVIDER_22 0xa800
#define LCD_F_CLOCKDIVIDER_23 0xb000
#define LCD_F_CLOCKDIVIDER_24 0xb800
#define LCD_F_CLOCKDIVIDER_25 0xc000
#define LCD_F_CLOCKDIVIDER_26 0xc800
#define LCD_F_CLOCKDIVIDER_27 0xd000
#define LCD_F_CLOCKDIVIDER_28 0xd800
#define LCD_F_CLOCKDIVIDER_29 0xe000
#define LCD_F_CLOCKDIVIDER_30 0xe800
#define LCD_F_CLOCKDIVIDER_31 0xf000
#define LCD_F_CLOCKDIVIDER_32 0xf800

//*****************************************************************************
//
// The following are values that can be passed to the initParams parameter for
// functions: LCD_F_initModule().
//
//*****************************************************************************
#define LCD_F_CLOCKPRESCALER_1                                 (LCD_F_CTL_PRE_0)
#define LCD_F_CLOCKPRESCALER_2                                 (LCD_F_CTL_PRE_1)
#define LCD_F_CLOCKPRESCALER_4                                 (LCD_F_CTL_PRE_2)
#define LCD_F_CLOCKPRESCALER_8                                 (LCD_F_CTL_PRE_3)
#define LCD_F_CLOCKPRESCALER_16                                (LCD_F_CTL_PRE_4)
#define LCD_F_CLOCKPRESCALER_32                                (LCD_F_CTL_PRE_5)

//*****************************************************************************
//
// The following are values that can be passed to the initParams parameter for
// functions: LCD_F_initModule().
//
//*****************************************************************************
#define LCD_F_STATIC                                            (LCD_F_CTL_MX_0)
#define LCD_F_2_MUX                                             (LCD_F_CTL_MX_1)
#define LCD_F_3_MUX                                             (LCD_F_CTL_MX_2)
#define LCD_F_4_MUX                                             (LCD_F_CTL_MX_3)
#define LCD_F_5_MUX                                             (LCD_F_CTL_MX_4)
#define LCD_F_6_MUX                                             (LCD_F_CTL_MX_5)
#define LCD_F_7_MUX                                             (LCD_F_CTL_MX_6)
#define LCD_F_8_MUX                                             (LCD_F_CTL_MX_7)

//*****************************************************************************
//
// The following are values that can be passed to the v2v3v4Source parameter
// for functions: LCD_F_setVLCDSource().
//
//*****************************************************************************
#define LCD_F_V2V3V4_GENERATED_INTERNALLY_NOT_SWITCHED_TO_PINS            (0x0)
#define LCD_F_V2V3V4_GENERATED_INTERNALLY_SWITCHED_TO_PINS  (LCD_F_VCTL_REXT)
#define LCD_F_V2V3V4_SOURCED_EXTERNALLY                  (LCD_F_VCTL_EXTBIAS)

//*****************************************************************************
//
// The following are values that can be passed to the v5Source parameter for
// functions: LCD_F_setVLCDSource().
//
//*****************************************************************************
#define LCD_F_V5_VSS                                                      (0x0)
#define LCD_F_V5_SOURCED_FROM_R03                         (LCD_F_VCTL_R03EXT)

//*****************************************************************************
//
// The following are values that can be passed to the initParams parameter for
// functions: LCD_F_initModule().
//
//*****************************************************************************
#define LCD_F_STANDARD_WAVEFORMS                                          (0x0)
#define LCD_F_LOW_POWER_WAVEFORMS                                 (LCD_F_CTL_LP)

//*****************************************************************************
//
// The following are values that can be passed to the initParams parameter for
// functions: LCD_F_initModule().
//
//*****************************************************************************
#define LCD_F_SEGMENTS_DISABLED                                           (0x0)
#define LCD_F_SEGMENTS_ENABLED                                   (LCD_F_CTL_SON)

//*****************************************************************************
//
// The following are values that can be passed to the mask parameter for
// functions: LCD_F_clearInterrupt(), LCD_F_getInterruptStatus(),
// LCD_F_enableInterrupt(), and LCD_F_disableInterrupt() as well as returned by
// the LCD_F_getInterruptStatus() function.
//
//*****************************************************************************
#define LCD_F_BLINKING_SEGMENTS_ON_INTERRUPT                  (LCD_F_IE_BLKONIE)
#define LCD_F_BLINKING_SEGMENTS_OFF_INTERRUPT                (LCD_F_IE_BLKOFFIE)
#define LCD_F_FRAME_INTERRUPT                                   (LCD_F_IE_FRMIE)
#define LCD_F_ANIMATION_LOOP_INTERRUPT                      (LCD_F_IE_ANMLOOPIE)
#define LCD_F_ANIMATION_STEP_INTERRUPT                       (LCD_F_IE_ANMSTPIE)

//*****************************************************************************
//
// The following are values that can be passed to the displayMemory parameter
// for functions: LCD_F_selectDisplayMemory().
//
//*****************************************************************************
#define LCD_F_DISPLAYSOURCE_MEMORY                                        (0x0)
#define LCD_F_DISPLAYSOURCE_BLINKINGMEMORY                                (0x1)

//*****************************************************************************
//
// The following are values that can be passed to the clockPrescalar parameter
// for functions: LCD_F_setBlinkingControl().
//
//*****************************************************************************
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_512                LCD_F_BMCTL_BLKPRE_0
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_1024               LCD_F_BMCTL_BLKPRE_1
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_2048               LCD_F_BMCTL_BLKPRE_2
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_4096               LCD_F_BMCTL_BLKPRE_3
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_8162               LCD_F_BMCTL_BLKPRE_4
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_16384              LCD_F_BMCTL_BLKPRE_5
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_32768              LCD_F_BMCTL_BLKPRE_6
#define LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_65536              LCD_F_BMCTL_BLKPRE_7

//*****************************************************************************
//
// The following are values that can be passed to the clockPrescalar parameter
// for functions: LCD_F_setBlinkingControl().
//
//*****************************************************************************
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_1                   LCD_F_BMCTL_BLKDIV_0
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_2                   LCD_F_BMCTL_BLKDIV_1
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_3                   LCD_F_BMCTL_BLKDIV_2
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_4                   LCD_F_BMCTL_BLKDIV_3
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_5                   LCD_F_BMCTL_BLKDIV_4
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_6                   LCD_F_BMCTL_BLKDIV_5
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_7                   LCD_F_BMCTL_BLKDIV_6
#define LCD_F_BLINK_FREQ_CLOCK_DIVIDER_8                   LCD_F_BMCTL_BLKDIV_7

//*****************************************************************************
//
// The following are values that can be passed to the mode parameter for
// functions: LCD_F_setBlinkingControl().
//
//*****************************************************************************
#define LCD_F_BLINK_MODE_DISABLED                         (LCD_F_BMCTL_BLKMOD_0)
#define LCD_F_BLINK_MODE_INDIVIDUAL_SEGMENTS              (LCD_F_BMCTL_BLKMOD_1)
#define LCD_F_BLINK_MODE_ALL_SEGMENTS                     (LCD_F_BMCTL_BLKMOD_2)
#define LCD_F_BLINK_MODE_SWITCHING_BETWEEN_DISPLAY_CONTENTS \
                                                          (LCD_F_BMCTL_BLKMOD_3)

//*****************************************************************************
//
// The following are values that can be passed to the clockPrescalar parameter
// for functions: LCD_F_setAnimationControl().
//
//*****************************************************************************
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_512          LCD_F_ANMCTL_ANMPRE_0
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_1024         LCD_F_ANMCTL_ANMPRE_1
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_2048         LCD_F_ANMCTL_ANMPRE_2
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_4096         LCD_F_ANMCTL_ANMPRE_3
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_8162         LCD_F_ANMCTL_ANMPRE_4
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_16384        LCD_F_ANMCTL_ANMPRE_5
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_32768        LCD_F_ANMCTL_ANMPRE_6
#define LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_65536        LCD_F_ANMCTL_ANMPRE_7

//*****************************************************************************
//
// The following are values that can be passed to the clockPrescalar parameter
// for functions: LCD_F_setAnimationControl().
//
//*****************************************************************************
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_1              LCD_F_ANMCTL_ANMDIV_0
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_2              LCD_F_ANMCTL_ANMDIV_1
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_3              LCD_F_ANMCTL_ANMDIV_2
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_4              LCD_F_ANMCTL_ANMDIV_3
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_5              LCD_F_ANMCTL_ANMDIV_4
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_6              LCD_F_ANMCTL_ANMDIV_5
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_7              LCD_F_ANMCTL_ANMDIV_6
#define LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_8              LCD_F_ANMCTL_ANMDIV_7

//*****************************************************************************
//
// The following are values that can be passed to the mode parameter for
// functions: LCD_F_setAnimationControl().
//
//*****************************************************************************
#define LCD_F_ANIMATION_FRAMES_T0_TO_T7                   LCD_F_ANMCTL_ANMSTP_7
#define LCD_F_ANIMATION_FRAMES_T0_TO_T6                   LCD_F_ANMCTL_ANMSTP_6
#define LCD_F_ANIMATION_FRAMES_T0_TO_T5                   LCD_F_ANMCTL_ANMSTP_5
#define LCD_F_ANIMATION_FRAMES_T0_TO_T4                   LCD_F_ANMCTL_ANMSTP_4
#define LCD_F_ANIMATION_FRAMES_T0_TO_T3                   LCD_F_ANMCTL_ANMSTP_3
#define LCD_F_ANIMATION_FRAMES_T0_TO_T2                   LCD_F_ANMCTL_ANMSTP_2
#define LCD_F_ANIMATION_FRAMES_T0_TO_T1                   LCD_F_ANMCTL_ANMSTP_1
#define LCD_F_ANIMATION_FRAMES_T0                         LCD_F_ANMCTL_ANMSTP_0

//*****************************************************************************
//
// The following are values that can be passed to the startPin parameter for
// functions: LCD_F_setPinsAsLCDFunction(); the endPin parameter for
// functions: LCD_F_setPinsAsLCDFunction(); the pin parameter for functions:
// LCD_F_setPinAsLCDFunction(), LCD_F_setPinAsPortFunction(),
// LCD_F_setPinAsCOM(), and LCD_F_setPinAsSEG().
//
//*****************************************************************************
#define LCD_F_SEGMENT_LINE_0                                                (0)
#define LCD_F_SEGMENT_LINE_1                                                (1)
#define LCD_F_SEGMENT_LINE_2                                                (2)
#define LCD_F_SEGMENT_LINE_3                                                (3)
#define LCD_F_SEGMENT_LINE_4                                                (4)
#define LCD_F_SEGMENT_LINE_5                                                (5)
#define LCD_F_SEGMENT_LINE_6                                                (6)
#define LCD_F_SEGMENT_LINE_7                                                (7)
#define LCD_F_SEGMENT_LINE_8                                                (8)
#define LCD_F_SEGMENT_LINE_9                                                (9)
#define LCD_F_SEGMENT_LINE_10                                              (10)
#define LCD_F_SEGMENT_LINE_11                                              (11)
#define LCD_F_SEGMENT_LINE_12                                              (12)
#define LCD_F_SEGMENT_LINE_13                                              (13)
#define LCD_F_SEGMENT_LINE_14                                              (14)
#define LCD_F_SEGMENT_LINE_15                                              (15)
#define LCD_F_SEGMENT_LINE_16                                              (16)
#define LCD_F_SEGMENT_LINE_17                                              (17)
#define LCD_F_SEGMENT_LINE_18                                              (18)
#define LCD_F_SEGMENT_LINE_19                                              (19)
#define LCD_F_SEGMENT_LINE_20                                              (20)
#define LCD_F_SEGMENT_LINE_21                                              (21)
#define LCD_F_SEGMENT_LINE_22                                              (22)
#define LCD_F_SEGMENT_LINE_23                                              (23)
#define LCD_F_SEGMENT_LINE_24                                              (24)
#define LCD_F_SEGMENT_LINE_25                                              (25)
#define LCD_F_SEGMENT_LINE_26                                              (26)
#define LCD_F_SEGMENT_LINE_27                                              (27)
#define LCD_F_SEGMENT_LINE_28                                              (28)
#define LCD_F_SEGMENT_LINE_29                                              (29)
#define LCD_F_SEGMENT_LINE_30                                              (30)
#define LCD_F_SEGMENT_LINE_31                                              (31)
#define LCD_F_SEGMENT_LINE_32                                              (32)
#define LCD_F_SEGMENT_LINE_33                                              (33)
#define LCD_F_SEGMENT_LINE_34                                              (34)
#define LCD_F_SEGMENT_LINE_35                                              (35)
#define LCD_F_SEGMENT_LINE_36                                              (36)
#define LCD_F_SEGMENT_LINE_37                                              (37)
#define LCD_F_SEGMENT_LINE_38                                              (38)
#define LCD_F_SEGMENT_LINE_39                                              (39)
#define LCD_F_SEGMENT_LINE_40                                              (40)
#define LCD_F_SEGMENT_LINE_41                                              (41)
#define LCD_F_SEGMENT_LINE_42                                              (42)
#define LCD_F_SEGMENT_LINE_43                                              (43)
#define LCD_F_SEGMENT_LINE_44                                              (44)
#define LCD_F_SEGMENT_LINE_45                                              (45)
#define LCD_F_SEGMENT_LINE_46                                              (46)
#define LCD_F_SEGMENT_LINE_47                                              (47)


//*****************************************************************************
//
// The following are values that can be passed to the com parameter for
// functions: LCD_F_setPinAsCOM().
//
//*****************************************************************************
#define LCD_F_MEMORY_COM0                                                (0x01)
#define LCD_F_MEMORY_COM1                                                (0x02)
#define LCD_F_MEMORY_COM2                                                (0x04)
#define LCD_F_MEMORY_COM3                                                (0x08)
#define LCD_F_MEMORY_COM4                                                (0x10)
#define LCD_F_MEMORY_COM5                                                (0x20)
#define LCD_F_MEMORY_COM6                                                (0x40)
#define LCD_F_MEMORY_COM7                                                (0x80)

//*****************************************************************************
//
// The following are values that can be passed to the bias parameter for
// functions: LCD_F_selectBias().
//
//*****************************************************************************
#define LCD_F_BIAS_1_3                                                    (0x0)
#define LCD_F_BIAS_1_2                                     (LCD_F_VCTL_LCD2B)
#define LCD_F_BIAS_1_4                                     (LCD_F_VCTL_LCD2B)

/* Internal Macros for indexing */
#define OFS_LCDM0W                                                        0x120
#define OFS_LCDBM0W                                                       0x160

//*****************************************************************************
//
//! \brief Initializes the LCD_F Module.
//!
//! This function initializes the LCD_F but without turning on. It bascially
//! setup the clock source, clock divider, mux rate, low-power waveform and
//! segments on/off. After calling this function, user can enable/disable
//! internal reference voltage or pin SEG/COM configurations.
//!
//! \param initParams is the pointer to  LCD_F_Config structure.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_initModule(LCD_F_Config *initParams);

//*****************************************************************************
//
//! \brief Turns on the LCD_F module.
//!
//! This function turns the LCD_F on.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_turnOn(void);

//*****************************************************************************
//
//! \brief Turns the LCD_F off.
//!
//! This function turns the LCD_F off.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_turnOff(void);

/* Memory management functions */

//*****************************************************************************
//
//! \brief Clears all LCD_F memory registers.
//!
//! This function clears all LCD_F memory registers.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_clearAllMemory(void);

//*****************************************************************************
//
//! \brief Clears all LCD_F blinking memory registers.
//!
//! This function clears all LCD_F blinking memory registers.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_clearAllBlinkingMemory(void);

//*****************************************************************************
//
//! \brief Selects display memory.
//!
//! This function selects display memory either from memory or blinking memory.
//! Please note if the blinking mode is selected as
//! LCD_F_BLINKMODE_INDIVIDUALSEGMENTS or LCD_F_BLINKMODE_ALLSEGMENTS or mux
//! rate >=5, display memory can not be changed. If
//! LCD_F_BLINKMODE_SWITCHDISPLAYCONTENTS is selected, display memory bit
//! reflects current displayed memory.
//!
//! \param displayMemory is the desired displayed memory.
//!        Valid values are:
//!        - \b LCD_F_DISPLAYSOURCE_MEMORY [Default]
//!        - \b LCD_F_DISPLAYSOURCE_BLINKINGMEMORY
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_selectDisplayMemory(uint_fast16_t displayMemory);

//*****************************************************************************
//
//! \brief Sets the blinking control register.
//!
//! This function sets the blink control related parameter, including blink
//! clock frequency prescalar and blink mode.
//!
//! \param clockPrescalar is the clock pre-scalar for blinking frequency.
//!        Valid values are:
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_512 [Default]
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_1024
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_2048
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_4096
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_8162
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_16384
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_32768
//!        - \b LCD_F_BLINK_FREQ_CLOCK_PRESCALAR_65536
//! \param clockDivider is the clock divider for blinking frequency.
//!        Valid values are:
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_1 [Default]
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_2
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_3
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_4
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_5
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_6
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_7
//!        - \b LCD_F_BLINK_FREQ_CLOCK_DIVIDER_8
//! \param mode is the select for blinking mode.
//!        Valid values are:
//!        - \b LCD_F_BLINK_MODE_DISABLED [Default]
//!        - \b LCD_F_BLINK_MODE_INDIVIDUAL_SEGMENTS
//!        - \b LCD_F_BLINK_MODE_ALL_SEGMENTS
//!        - \b LCD_F_BLINK_MODE_SWITCHING_BETWEEN_DISPLAY_CONTENTS
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setBlinkingControl(uint_fast16_t clockPrescalar,
                                     uint_fast16_t divider, uint_fast16_t mode);

//*****************************************************************************
//
//! \brief Sets the animation control register.
//!
//! This function sets the animation control related parameter, including
//! animation clock frequency prescalar, divider, and frame settings
//!
//! \param clockPrescalar is the clock pre-scalar for animation frequency.
//!        Valid values are:
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_512 [Default]
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_1024
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_2048
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_4096
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_8162
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_16384
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_32768
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_PRESCALAR_65536
//! \param clockDivider is the clock divider for animation frequency.
//!        Valid values are:
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_1 [Default]
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_2
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_3
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_4
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_5
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_6
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_7
//!        - \b LCD_F_ANIMATION_FREQ_CLOCK_DIVIDER_8
//! \param frames is number of animations frames to be repeated
//!        Valid values are:
//!        - \b LCD_F_ANIMATION_FRAMES_T0_TO_T7
//!        - \b LCD_F_ANIMATION_FRAMES_T0_TO_T6
//!        - \b LCD_F_ANIMATION_FRAMES_T0_TO_T5
//!        - \b LCD_F_ANIMATION_FRAMES_T0_TO_T4
//!        - \b LCD_F_ANIMATION_FRAMES_T0_TO_T3
//!        - \b LCD_F_ANIMATION_FRAMES_T0_TO_T2
//!        - \b LCD_F_ANIMATION_FRAMES_T0_TO_T1
//!        - \b LCD_F_ANIMATION_FRAMES_T0
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setAnimationControl(uint_fast16_t clockPrescalar,
                                      uint_fast16_t divider,
                                      uint_fast16_t frames);

//*****************************************************************************
//
//! \brief Enables animation on the LCD_F controller
//!
//! This function turns on animation for the LCD_F controller.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_enableAnimation(void);


//*****************************************************************************
//
//! \brief Enables animation on the LCD_F controller
//!
//! This function turns on animation for the LCD_F controller.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_disableAnimation(void);

//*****************************************************************************
//
//! \brief Clears all LCD_F animation memory registers.
//!
//! This function clears all LCD_F animation memory registers.
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_clearAllAnimationMemory(void);

//*****************************************************************************
//
//! \brief Sets the LCD_F pins as LCD function pin.
//!
//! This function sets the LCD_F pins as LCD function pin.
//!
//! \param pin is the select pin set as LCD function.
//!        Valid values are:
//!        - \b LCD_F_SEGMENT_LINE_0
//!        - \b LCD_F_SEGMENT_LINE_1
//!        - \b LCD_F_SEGMENT_LINE_2
//!        - \b LCD_F_SEGMENT_LINE_3
//!        - \b LCD_F_SEGMENT_LINE_4
//!        - \b LCD_F_SEGMENT_LINE_5
//!        - \b LCD_F_SEGMENT_LINE_6
//!        - \b LCD_F_SEGMENT_LINE_7
//!        - \b LCD_F_SEGMENT_LINE_8
//!        - \b LCD_F_SEGMENT_LINE_9
//!        - \b LCD_F_SEGMENT_LINE_10
//!        - \b LCD_F_SEGMENT_LINE_11
//!        - \b LCD_F_SEGMENT_LINE_12
//!        - \b LCD_F_SEGMENT_LINE_13
//!        - \b LCD_F_SEGMENT_LINE_14
//!        - \b LCD_F_SEGMENT_LINE_15
//!        - \b LCD_F_SEGMENT_LINE_16
//!        - \b LCD_F_SEGMENT_LINE_17
//!        - \b LCD_F_SEGMENT_LINE_18
//!        - \b LCD_F_SEGMENT_LINE_19
//!        - \b LCD_F_SEGMENT_LINE_20
//!        - \b LCD_F_SEGMENT_LINE_21
//!        - \b LCD_F_SEGMENT_LINE_22
//!        - \b LCD_F_SEGMENT_LINE_23
//!        - \b LCD_F_SEGMENT_LINE_24
//!        - \b LCD_F_SEGMENT_LINE_25
//!        - \b LCD_F_SEGMENT_LINE_26
//!        - \b LCD_F_SEGMENT_LINE_27
//!        - \b LCD_F_SEGMENT_LINE_28
//!        - \b LCD_F_SEGMENT_LINE_29
//!        - \b LCD_F_SEGMENT_LINE_30
//!        - \b LCD_F_SEGMENT_LINE_31
//!        - \b LCD_F_SEGMENT_LINE_32
//!        - \b LCD_F_SEGMENT_LINE_33
//!        - \b LCD_F_SEGMENT_LINE_34
//!        - \b LCD_F_SEGMENT_LINE_35
//!        - \b LCD_F_SEGMENT_LINE_36
//!        - \b LCD_F_SEGMENT_LINE_37
//!        - \b LCD_F_SEGMENT_LINE_38
//!        - \b LCD_F_SEGMENT_LINE_39
//!        - \b LCD_F_SEGMENT_LINE_40
//!        - \b LCD_F_SEGMENT_LINE_41
//!        - \b LCD_F_SEGMENT_LINE_42
//!        - \b LCD_F_SEGMENT_LINE_43
//!        - \b LCD_F_SEGMENT_LINE_44
//!        - \b LCD_F_SEGMENT_LINE_45
//!        - \b LCD_F_SEGMENT_LINE_46
//!        - \b LCD_F_SEGMENT_LINE_47
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setPinAsLCDFunction(uint_fast8_t pin);

//*****************************************************************************
//
//! \brief Sets the LCD_F pins as port function pin.
//!
//! \param baseAddress is the base address of the LCD_F module.
//! \param pin is the select pin set as Port function.
//!        Valid values are:
//!        - \b LCD_F_SEGMENT_LINE_0
//!        - \b LCD_F_SEGMENT_LINE_1
//!        - \b LCD_F_SEGMENT_LINE_2
//!        - \b LCD_F_SEGMENT_LINE_3
//!        - \b LCD_F_SEGMENT_LINE_4
//!        - \b LCD_F_SEGMENT_LINE_5
//!        - \b LCD_F_SEGMENT_LINE_6
//!        - \b LCD_F_SEGMENT_LINE_7
//!        - \b LCD_F_SEGMENT_LINE_8
//!        - \b LCD_F_SEGMENT_LINE_9
//!        - \b LCD_F_SEGMENT_LINE_10
//!        - \b LCD_F_SEGMENT_LINE_11
//!        - \b LCD_F_SEGMENT_LINE_12
//!        - \b LCD_F_SEGMENT_LINE_13
//!        - \b LCD_F_SEGMENT_LINE_14
//!        - \b LCD_F_SEGMENT_LINE_15
//!        - \b LCD_F_SEGMENT_LINE_16
//!        - \b LCD_F_SEGMENT_LINE_17
//!        - \b LCD_F_SEGMENT_LINE_18
//!        - \b LCD_F_SEGMENT_LINE_19
//!        - \b LCD_F_SEGMENT_LINE_20
//!        - \b LCD_F_SEGMENT_LINE_21
//!        - \b LCD_F_SEGMENT_LINE_22
//!        - \b LCD_F_SEGMENT_LINE_23
//!        - \b LCD_F_SEGMENT_LINE_24
//!        - \b LCD_F_SEGMENT_LINE_25
//!        - \b LCD_F_SEGMENT_LINE_26
//!        - \b LCD_F_SEGMENT_LINE_27
//!        - \b LCD_F_SEGMENT_LINE_28
//!        - \b LCD_F_SEGMENT_LINE_29
//!        - \b LCD_F_SEGMENT_LINE_30
//!        - \b LCD_F_SEGMENT_LINE_31
//!        - \b LCD_F_SEGMENT_LINE_32
//!        - \b LCD_F_SEGMENT_LINE_33
//!        - \b LCD_F_SEGMENT_LINE_34
//!        - \b LCD_F_SEGMENT_LINE_35
//!        - \b LCD_F_SEGMENT_LINE_36
//!        - \b LCD_F_SEGMENT_LINE_37
//!        - \b LCD_F_SEGMENT_LINE_38
//!        - \b LCD_F_SEGMENT_LINE_39
//!        - \b LCD_F_SEGMENT_LINE_40
//!        - \b LCD_F_SEGMENT_LINE_41
//!        - \b LCD_F_SEGMENT_LINE_42
//!        - \b LCD_F_SEGMENT_LINE_43
//!        - \b LCD_F_SEGMENT_LINE_44
//!        - \b LCD_F_SEGMENT_LINE_45
//!        - \b LCD_F_SEGMENT_LINE_46
//!        - \b LCD_F_SEGMENT_LINE_47
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setPinAsPortFunction(uint_fast8_t pin);

//*****************************************************************************
//
//! \brief Sets the LCD_F pins as LCD function pin.
//!
//! This function sets the LCD_F pins as LCD function pin. Instead of passing
//! the all the possible pins, it just requires the start pin and the end pin.
//!
//! \param startPin is the starting pin to be configured as LCD function pin.
//!        Valid values are:
//!        - \b LCD_F_SEGMENT_LINE_0
//!        - \b LCD_F_SEGMENT_LINE_1
//!        - \b LCD_F_SEGMENT_LINE_2
//!        - \b LCD_F_SEGMENT_LINE_3
//!        - \b LCD_F_SEGMENT_LINE_4
//!        - \b LCD_F_SEGMENT_LINE_5
//!        - \b LCD_F_SEGMENT_LINE_6
//!        - \b LCD_F_SEGMENT_LINE_7
//!        - \b LCD_F_SEGMENT_LINE_8
//!        - \b LCD_F_SEGMENT_LINE_9
//!        - \b LCD_F_SEGMENT_LINE_10
//!        - \b LCD_F_SEGMENT_LINE_11
//!        - \b LCD_F_SEGMENT_LINE_12
//!        - \b LCD_F_SEGMENT_LINE_13
//!        - \b LCD_F_SEGMENT_LINE_14
//!        - \b LCD_F_SEGMENT_LINE_15
//!        - \b LCD_F_SEGMENT_LINE_16
//!        - \b LCD_F_SEGMENT_LINE_17
//!        - \b LCD_F_SEGMENT_LINE_18
//!        - \b LCD_F_SEGMENT_LINE_19
//!        - \b LCD_F_SEGMENT_LINE_20
//!        - \b LCD_F_SEGMENT_LINE_21
//!        - \b LCD_F_SEGMENT_LINE_22
//!        - \b LCD_F_SEGMENT_LINE_23
//!        - \b LCD_F_SEGMENT_LINE_24
//!        - \b LCD_F_SEGMENT_LINE_25
//!        - \b LCD_F_SEGMENT_LINE_26
//!        - \b LCD_F_SEGMENT_LINE_27
//!        - \b LCD_F_SEGMENT_LINE_28
//!        - \b LCD_F_SEGMENT_LINE_29
//!        - \b LCD_F_SEGMENT_LINE_30
//!        - \b LCD_F_SEGMENT_LINE_31
//!        - \b LCD_F_SEGMENT_LINE_32
//!        - \b LCD_F_SEGMENT_LINE_33
//!        - \b LCD_F_SEGMENT_LINE_34
//!        - \b LCD_F_SEGMENT_LINE_35
//!        - \b LCD_F_SEGMENT_LINE_36
//!        - \b LCD_F_SEGMENT_LINE_37
//!        - \b LCD_F_SEGMENT_LINE_38
//!        - \b LCD_F_SEGMENT_LINE_39
//!        - \b LCD_F_SEGMENT_LINE_40
//!        - \b LCD_F_SEGMENT_LINE_41
//!        - \b LCD_F_SEGMENT_LINE_42
//!        - \b LCD_F_SEGMENT_LINE_43
//!        - \b LCD_F_SEGMENT_LINE_44
//!        - \b LCD_F_SEGMENT_LINE_45
//!        - \b LCD_F_SEGMENT_LINE_46
//!        - \b LCD_F_SEGMENT_LINE_47
//! \param endPin is the ending pin to be configured as LCD function pin.
//!        Valid values are:
//!        - \b LCD_F_SEGMENT_LINE_0
//!        - \b LCD_F_SEGMENT_LINE_1
//!        - \b LCD_F_SEGMENT_LINE_2
//!        - \b LCD_F_SEGMENT_LINE_3
//!        - \b LCD_F_SEGMENT_LINE_4
//!        - \b LCD_F_SEGMENT_LINE_5
//!        - \b LCD_F_SEGMENT_LINE_6
//!        - \b LCD_F_SEGMENT_LINE_7
//!        - \b LCD_F_SEGMENT_LINE_8
//!        - \b LCD_F_SEGMENT_LINE_9
//!        - \b LCD_F_SEGMENT_LINE_10
//!        - \b LCD_F_SEGMENT_LINE_11
//!        - \b LCD_F_SEGMENT_LINE_12
//!        - \b LCD_F_SEGMENT_LINE_13
//!        - \b LCD_F_SEGMENT_LINE_14
//!        - \b LCD_F_SEGMENT_LINE_15
//!        - \b LCD_F_SEGMENT_LINE_16
//!        - \b LCD_F_SEGMENT_LINE_17
//!        - \b LCD_F_SEGMENT_LINE_18
//!        - \b LCD_F_SEGMENT_LINE_19
//!        - \b LCD_F_SEGMENT_LINE_20
//!        - \b LCD_F_SEGMENT_LINE_21
//!        - \b LCD_F_SEGMENT_LINE_22
//!        - \b LCD_F_SEGMENT_LINE_23
//!        - \b LCD_F_SEGMENT_LINE_24
//!        - \b LCD_F_SEGMENT_LINE_25
//!        - \b LCD_F_SEGMENT_LINE_26
//!        - \b LCD_F_SEGMENT_LINE_27
//!        - \b LCD_F_SEGMENT_LINE_28
//!        - \b LCD_F_SEGMENT_LINE_29
//!        - \b LCD_F_SEGMENT_LINE_30
//!        - \b LCD_F_SEGMENT_LINE_31
//!        - \b LCD_F_SEGMENT_LINE_32
//!        - \b LCD_F_SEGMENT_LINE_33
//!        - \b LCD_F_SEGMENT_LINE_34
//!        - \b LCD_F_SEGMENT_LINE_35
//!        - \b LCD_F_SEGMENT_LINE_36
//!        - \b LCD_F_SEGMENT_LINE_37
//!        - \b LCD_F_SEGMENT_LINE_38
//!        - \b LCD_F_SEGMENT_LINE_39
//!        - \b LCD_F_SEGMENT_LINE_40
//!        - \b LCD_F_SEGMENT_LINE_41
//!        - \b LCD_F_SEGMENT_LINE_42
//!        - \b LCD_F_SEGMENT_LINE_43
//!        - \b LCD_F_SEGMENT_LINE_44
//!        - \b LCD_F_SEGMENT_LINE_45
//!        - \b LCD_F_SEGMENT_LINE_46
//!        - \b LCD_F_SEGMENT_LINE_47
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setPinsAsLCDFunction(uint_fast8_t startPin, uint8_t endPin);

//*****************************************************************************
//
//! \brief Sets the LCD_F pin as a common line.
//!
//! This function sets the LCD_F pin as a common line and assigns the
//! corresponding memory pin to a specific COM line.
//!
//! \param pin is the selected pin to be configured as common line.
//!        Valid values are:
//!        - \b LCD_F_SEGMENT_LINE_0
//!        - \b LCD_F_SEGMENT_LINE_1
//!        - \b LCD_F_SEGMENT_LINE_2
//!        - \b LCD_F_SEGMENT_LINE_3
//!        - \b LCD_F_SEGMENT_LINE_4
//!        - \b LCD_F_SEGMENT_LINE_5
//!        - \b LCD_F_SEGMENT_LINE_6
//!        - \b LCD_F_SEGMENT_LINE_7
//!        - \b LCD_F_SEGMENT_LINE_8
//!        - \b LCD_F_SEGMENT_LINE_9
//!        - \b LCD_F_SEGMENT_LINE_10
//!        - \b LCD_F_SEGMENT_LINE_11
//!        - \b LCD_F_SEGMENT_LINE_12
//!        - \b LCD_F_SEGMENT_LINE_13
//!        - \b LCD_F_SEGMENT_LINE_14
//!        - \b LCD_F_SEGMENT_LINE_15
//!        - \b LCD_F_SEGMENT_LINE_16
//!        - \b LCD_F_SEGMENT_LINE_17
//!        - \b LCD_F_SEGMENT_LINE_18
//!        - \b LCD_F_SEGMENT_LINE_19
//!        - \b LCD_F_SEGMENT_LINE_20
//!        - \b LCD_F_SEGMENT_LINE_21
//!        - \b LCD_F_SEGMENT_LINE_22
//!        - \b LCD_F_SEGMENT_LINE_23
//!        - \b LCD_F_SEGMENT_LINE_24
//!        - \b LCD_F_SEGMENT_LINE_25
//!        - \b LCD_F_SEGMENT_LINE_26
//!        - \b LCD_F_SEGMENT_LINE_27
//!        - \b LCD_F_SEGMENT_LINE_28
//!        - \b LCD_F_SEGMENT_LINE_29
//!        - \b LCD_F_SEGMENT_LINE_30
//!        - \b LCD_F_SEGMENT_LINE_31
//!        - \b LCD_F_SEGMENT_LINE_32
//!        - \b LCD_F_SEGMENT_LINE_33
//!        - \b LCD_F_SEGMENT_LINE_34
//!        - \b LCD_F_SEGMENT_LINE_35
//!        - \b LCD_F_SEGMENT_LINE_36
//!        - \b LCD_F_SEGMENT_LINE_37
//!        - \b LCD_F_SEGMENT_LINE_38
//!        - \b LCD_F_SEGMENT_LINE_39
//!        - \b LCD_F_SEGMENT_LINE_40
//!        - \b LCD_F_SEGMENT_LINE_41
//!        - \b LCD_F_SEGMENT_LINE_42
//!        - \b LCD_F_SEGMENT_LINE_43
//!        - \b LCD_F_SEGMENT_LINE_44
//!        - \b LCD_F_SEGMENT_LINE_45
//!        - \b LCD_F_SEGMENT_LINE_46
//!        - \b LCD_F_SEGMENT_LINE_47
//! \param com is the selected COM number for the common line.
//!        Valid values are:
//!        - \b LCD_F_MEMORY_COM0
//!        - \b LCD_F_MEMORY_COM1
//!        - \b LCD_F_MEMORY_COM2
//!        - \b LCD_F_MEMORY_COM3
//!        - \b LCD_F_MEMORY_COM4 - only for 5-Mux/6-Mux/7-Mux/8-Mux
//!        - \b LCD_F_MEMORY_COM5 - only for 5-Mux/6-Mux/7-Mux/8-Mux
//!        - \b LCD_F_MEMORY_COM6 - only for 5-Mux/6-Mux/7-Mux/8-Mux
//!        - \b LCD_F_MEMORY_COM7 - only for 5-Mux/6-Mux/7-Mux/8-Mux
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setPinAsCOM(uint8_t pin, uint_fast8_t com);

//*****************************************************************************
//
//! \brief Sets the LCD_F pin as a segment line.
//!
//! This function sets the LCD_F pin as segment line.
//!
//! \param pin is the selected pin to be configed as segment line.
//!        Valid values are:
//!        - \b LCD_F_SEGMENT_LINE_0
//!        - \b LCD_F_SEGMENT_LINE_1
//!        - \b LCD_F_SEGMENT_LINE_2
//!        - \b LCD_F_SEGMENT_LINE_3
//!        - \b LCD_F_SEGMENT_LINE_4
//!        - \b LCD_F_SEGMENT_LINE_5
//!        - \b LCD_F_SEGMENT_LINE_6
//!        - \b LCD_F_SEGMENT_LINE_7
//!        - \b LCD_F_SEGMENT_LINE_8
//!        - \b LCD_F_SEGMENT_LINE_9
//!        - \b LCD_F_SEGMENT_LINE_10
//!        - \b LCD_F_SEGMENT_LINE_11
//!        - \b LCD_F_SEGMENT_LINE_12
//!        - \b LCD_F_SEGMENT_LINE_13
//!        - \b LCD_F_SEGMENT_LINE_14
//!        - \b LCD_F_SEGMENT_LINE_15
//!        - \b LCD_F_SEGMENT_LINE_16
//!        - \b LCD_F_SEGMENT_LINE_17
//!        - \b LCD_F_SEGMENT_LINE_18
//!        - \b LCD_F_SEGMENT_LINE_19
//!        - \b LCD_F_SEGMENT_LINE_20
//!        - \b LCD_F_SEGMENT_LINE_21
//!        - \b LCD_F_SEGMENT_LINE_22
//!        - \b LCD_F_SEGMENT_LINE_23
//!        - \b LCD_F_SEGMENT_LINE_24
//!        - \b LCD_F_SEGMENT_LINE_25
//!        - \b LCD_F_SEGMENT_LINE_26
//!        - \b LCD_F_SEGMENT_LINE_27
//!        - \b LCD_F_SEGMENT_LINE_28
//!        - \b LCD_F_SEGMENT_LINE_29
//!        - \b LCD_F_SEGMENT_LINE_30
//!        - \b LCD_F_SEGMENT_LINE_31
//!        - \b LCD_F_SEGMENT_LINE_32
//!        - \b LCD_F_SEGMENT_LINE_33
//!        - \b LCD_F_SEGMENT_LINE_34
//!        - \b LCD_F_SEGMENT_LINE_35
//!        - \b LCD_F_SEGMENT_LINE_36
//!        - \b LCD_F_SEGMENT_LINE_37
//!        - \b LCD_F_SEGMENT_LINE_38
//!        - \b LCD_F_SEGMENT_LINE_39
//!        - \b LCD_F_SEGMENT_LINE_40
//!        - \b LCD_F_SEGMENT_LINE_41
//!        - \b LCD_F_SEGMENT_LINE_42
//!        - \b LCD_F_SEGMENT_LINE_43
//!        - \b LCD_F_SEGMENT_LINE_44
//!        - \b LCD_F_SEGMENT_LINE_45
//!        - \b LCD_F_SEGMENT_LINE_46
//!        - \b LCD_F_SEGMENT_LINE_47
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setPinAsSEG(uint_fast8_t pin);

//*****************************************************************************
//
//! \brief Selects the bias level.
//!
//! \param bias is the select for bias level.
//!        Valid values are:
//!        - \b LCD_F_BIAS_1_3 [Default] - 1/3 bias
//!        - \b LCD_F_BIAS_1_4 - 1/4 bias
//!        - \b LCD_F_BIAS_1_2 - 1/2 bias
//!
//! \note Quarter (1/4) BIAS mode is only available in 5-mux to 8-mux. In
//!     2-mux to 4-mux modes, this value will result in a third BIAS (1/3)
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_selectBias(uint_fast16_t bias);

//*****************************************************************************
//
//! \brief Sets the voltage source for V2/V3/V4 and V5.
//!
//! \param v2v3v4Source is the V2/V3/V4 source select.
//!        Valid values are:
//!        - \b LCD_F_V2V3V4_GENERATED_INTERNALLY_NOT_SWITCHED_TO_PINS
//!           [Default]
//!        - \b LCD_F_V2V3V4_GENERATED_INTERNALLY_SWITCHED_TO_PINS
//!        - \b LCD_F_V2V3V4_SOURCED_EXTERNALLY
//! \param v5Source is the V5 source select.
//!        Valid values are:
//!        - \b LCD_F_V5_VSS [Default]
//!        - \b LCD_F_V5_SOURCED_FROM_R03
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_setVLCDSource(uint_fast16_t v2v3v4Source,
                                     uint_fast16_t v5Source);

//*****************************************************************************
//
//! \brief Clears the LCD_F selected interrupt flags.
//!
//! This function clears the specified interrupt flags.
//!
//! \param mask is the masked interrupt flag to be cleared.
//!        Mask value is the logical OR of any of the following:
//!        - \b LCD_F_BLINKING_SEGMENTS_ON_INTERRUPT
//!        - \b LCD_F_BLINKING_SEGMENTS_OFF_INTERRUPT
//!        - \b LCD_F_FRAME_INTERRUPT
//!        - \b LCD_F_ANIMATION_LOOP_INTERRUPT
//!        - \b LCD_F_ANIMATION_STEP_INTERRUPT
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_clearInterrupt(uint32_t mask);

//*****************************************************************************
//
//! \brief Returns the status of the selected interrupt flags.
//!
//! This function returns the status of the selected interrupt flags.
//!
//! \return The current interrupt flag status. Can be a logical OR of:
//!        - \b LCD_F_BLINKING_SEGMENTS_ON_INTERRUPT
//!        - \b LCD_F_BLINKING_SEGMENTS_OFF_INTERRUPT
//!        - \b LCD_F_FRAME_INTERRUPT
//!        - \b LCD_F_ANIMATION_LOOP_INTERRUPT
//!        - \b LCD_F_ANIMATION_STEP_INTERRUPT
//
//*****************************************************************************
extern uint32_t LCD_F_getInterruptStatus(void);

//*****************************************************************************
//
//! \brief Returns the status of the selected interrupt flags masked with the
//!  currently enabled interrupts.
//!
//! \return The current interrupt flag status. Can be a logical OR of:
//!        - \b LCD_F_BLINKING_SEGMENTS_ON_INTERRUPT
//!        - \b LCD_F_BLINKING_SEGMENTS_OFF_INTERRUPT
//!        - \b LCD_F_FRAME_INTERRUPT
//!        - \b LCD_F_ANIMATION_LOOP_INTERRUPT
//!        - \b LCD_F_ANIMATION_STEP_INTERRUPT
//
//*****************************************************************************
extern uint32_t LCD_F_getEnabledInterruptStatus(void);

//*****************************************************************************
//
//! \brief Enables the LCD_F selected interrupts
//!
//! This function enables the specified interrupts
//!
//! \param mask is the variable containing interrupt flags to be enabled.
//!        Mask value is the logical OR of any of the following:
//!        - \b LCD_F_BLINKING_SEGMENTS_ON_INTERRUPT
//!        - \b LCD_F_BLINKING_SEGMENTS_OFF_INTERRUPT
//!        - \b LCD_F_FRAME_INTERRUPT
//!        - \b LCD_F_ANIMATION_LOOP_INTERRUPT
//!        - \b LCD_F_ANIMATION_STEP_INTERRUPT
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_enableInterrupt(uint32_t mask);

//*****************************************************************************
//
//! \brief Disables the LCD_F selected interrupts.
//!
//! This function disables the specified interrupts.
//!
//! \param mask is the variable containing interrupt flags to be disabled
//!        Mask value is the logical OR of any of the following:
//!        - \b LCD_F_BLINKING_SEGMENTS_ON_INTERRUPT
//!        - \b LCD_F_BLINKING_SEGMENTS_OFF_INTERRUPT
//!        - \b LCD_F_FRAME_INTERRUPT
//!        - \b LCD_F_ANIMATION_LOOP_INTERRUPT
//!        - \b LCD_F_ANIMATION_STEP_INTERRUPT
//!
//! \return None
//
//*****************************************************************************
extern void LCD_F_disableInterrupt(uint32_t mask);

//*****************************************************************************
//
//! Registers an interrupt handler for LCD_F interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the
//! LCD_F interrupt occurs.
//!
//! This function registers the handler to be called when a LCD_F
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific LCD_F interrupts must be enabled
//! via LCD_F_enableInterrupt().  It is the interrupt handler's responsibility
//! to clear the interrupt source through LCD_F_clearInterruptFlag().
//!
//! \return None.
//
//*****************************************************************************
extern void LCD_F_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the LCD_F interrupt
//!
//! This function unregisters the handler to be called when LCD_F
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void LCD_F_unregisterInterrupt(void);

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

#endif /* __MCU_HAS_LCD_F__ */

#endif /* LCD_F_H_ */
