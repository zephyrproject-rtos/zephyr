/******************************************************************************
*  Filename:       hw_aux_timer2_h
*  Revised:        2018-05-14 12:24:52 +0200 (Mon, 14 May 2018)
*  Revision:       51990
*
* Copyright (c) 2015 - 2017, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1) Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2) Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3) Neither the name of the ORGANIZATION nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef __HW_AUX_TIMER2_H__
#define __HW_AUX_TIMER2_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_TIMER2 component
//
//*****************************************************************************
// Timer Control
#define AUX_TIMER2_O_CTL                                            0x00000000

// Target
#define AUX_TIMER2_O_TARGET                                         0x00000004

// Shadow Target
#define AUX_TIMER2_O_SHDWTARGET                                     0x00000008

// Counter
#define AUX_TIMER2_O_CNTR                                           0x0000000C

// Clock Prescaler Configuration
#define AUX_TIMER2_O_PRECFG                                         0x00000010

// Event Control
#define AUX_TIMER2_O_EVCTL                                          0x00000014

// Pulse Trigger
#define AUX_TIMER2_O_PULSETRIG                                      0x00000018

// Channel 0 Event Configuration
#define AUX_TIMER2_O_CH0EVCFG                                       0x00000080

// Channel 0 Capture Configuration
#define AUX_TIMER2_O_CH0CCFG                                        0x00000084

// Channel 0 Pipeline Capture Compare
#define AUX_TIMER2_O_CH0PCC                                         0x00000088

// Channel 0 Capture Compare
#define AUX_TIMER2_O_CH0CC                                          0x0000008C

// Channel 1 Event Configuration
#define AUX_TIMER2_O_CH1EVCFG                                       0x00000090

// Channel 1 Capture Configuration
#define AUX_TIMER2_O_CH1CCFG                                        0x00000094

// Channel 1 Pipeline Capture Compare
#define AUX_TIMER2_O_CH1PCC                                         0x00000098

// Channel 1 Capture Compare
#define AUX_TIMER2_O_CH1CC                                          0x0000009C

// Channel 2 Event Configuration
#define AUX_TIMER2_O_CH2EVCFG                                       0x000000A0

// Channel 2 Capture Configuration
#define AUX_TIMER2_O_CH2CCFG                                        0x000000A4

// Channel 2 Pipeline Capture Compare
#define AUX_TIMER2_O_CH2PCC                                         0x000000A8

// Channel 2 Capture Compare
#define AUX_TIMER2_O_CH2CC                                          0x000000AC

// Channel 3 Event Configuration
#define AUX_TIMER2_O_CH3EVCFG                                       0x000000B0

// Channel 3 Capture Configuration
#define AUX_TIMER2_O_CH3CCFG                                        0x000000B4

// Channel 3 Pipeline Capture Compare
#define AUX_TIMER2_O_CH3PCC                                         0x000000B8

// Channel 3 Capture Compare
#define AUX_TIMER2_O_CH3CC                                          0x000000BC

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CTL
//
//*****************************************************************************
// Field:     [6] CH3_RESET
//
// Channel 3 reset.
//
// 0: No effect.
// 1: Reset CH3CC, CH3PCC, CH3EVCFG, and CH3CCFG.
//
// Read returns 0.
#define AUX_TIMER2_CTL_CH3_RESET                                    0x00000040
#define AUX_TIMER2_CTL_CH3_RESET_BITN                                        6
#define AUX_TIMER2_CTL_CH3_RESET_M                                  0x00000040
#define AUX_TIMER2_CTL_CH3_RESET_S                                           6

// Field:     [5] CH2_RESET
//
// Channel 2 reset.
//
// 0: No effect.
// 1: Reset CH2CC, CH2PCC, CH2EVCFG, and CH2CCFG.
//
// Read returns 0.
#define AUX_TIMER2_CTL_CH2_RESET                                    0x00000020
#define AUX_TIMER2_CTL_CH2_RESET_BITN                                        5
#define AUX_TIMER2_CTL_CH2_RESET_M                                  0x00000020
#define AUX_TIMER2_CTL_CH2_RESET_S                                           5

// Field:     [4] CH1_RESET
//
// Channel 1 reset.
//
// 0: No effect.
// 1: Reset CH1CC, CH1PCC, CH1EVCFG, and CH1CCFG.
//
// Read returns 0.
#define AUX_TIMER2_CTL_CH1_RESET                                    0x00000010
#define AUX_TIMER2_CTL_CH1_RESET_BITN                                        4
#define AUX_TIMER2_CTL_CH1_RESET_M                                  0x00000010
#define AUX_TIMER2_CTL_CH1_RESET_S                                           4

// Field:     [3] CH0_RESET
//
// Channel 0 reset.
//
// 0: No effect.
// 1: Reset CH0CC, CH0PCC, CH0EVCFG, and CH0CCFG.
//
// Read returns 0.
#define AUX_TIMER2_CTL_CH0_RESET                                    0x00000008
#define AUX_TIMER2_CTL_CH0_RESET_BITN                                        3
#define AUX_TIMER2_CTL_CH0_RESET_M                                  0x00000008
#define AUX_TIMER2_CTL_CH0_RESET_S                                           3

// Field:     [2] TARGET_EN
//
// Select counter target value.
//
// You must select TARGET to use shadow target functionality.
// ENUMs:
// TARGET                   TARGET.VALUE
// CNTR_MAX                 65535
#define AUX_TIMER2_CTL_TARGET_EN                                    0x00000004
#define AUX_TIMER2_CTL_TARGET_EN_BITN                                        2
#define AUX_TIMER2_CTL_TARGET_EN_M                                  0x00000004
#define AUX_TIMER2_CTL_TARGET_EN_S                                           2
#define AUX_TIMER2_CTL_TARGET_EN_TARGET                             0x00000004
#define AUX_TIMER2_CTL_TARGET_EN_CNTR_MAX                           0x00000000

// Field:   [1:0] MODE
//
// Timer mode control.
//
// The timer restarts from 0 when you set MODE to UP_ONCE, UP_PER, or
// UPDWN_PER.
//
// When you write MODE all internally queued updates to [CHnCC.*] and TARGET
// clear.
// ENUMs:
// UPDWN_PER                Count up and down periodically. The timer counts
//                          from 0 to target value and back to 0,
//                          repeatedly.
//
//                          Period =  (target value *
//                          2) * timer clock period
// UP_PER                   Count up periodically. The timer increments from 0
//                          to target value, repeatedly.
//
//                          Period = (target value +
//                          1) * timer clock period
// UP_ONCE                  Count up once. The timer increments from 0 to
//                          target value,  then stops and sets MODE to DIS.
// DIS                      Disable timer. Updates to counter, channels, and
//                          events stop.
#define AUX_TIMER2_CTL_MODE_W                                                2
#define AUX_TIMER2_CTL_MODE_M                                       0x00000003
#define AUX_TIMER2_CTL_MODE_S                                                0
#define AUX_TIMER2_CTL_MODE_UPDWN_PER                               0x00000003
#define AUX_TIMER2_CTL_MODE_UP_PER                                  0x00000002
#define AUX_TIMER2_CTL_MODE_UP_ONCE                                 0x00000001
#define AUX_TIMER2_CTL_MODE_DIS                                     0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_TARGET
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// 16 bit user defined counter target value, which is used when selected by
// CTL.TARGET_EN.
#define AUX_TIMER2_TARGET_VALUE_W                                           16
#define AUX_TIMER2_TARGET_VALUE_M                                   0x0000FFFF
#define AUX_TIMER2_TARGET_VALUE_S                                            0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_SHDWTARGET
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Target value for next counter period.
//
// The timer copies VALUE to TARGET.VALUE when CNTR.VALUE becomes 0. The copy
// does not happen when you restart the timer.
//
// This is useful to avoid period jitter in PWM applications with time-varying
// period, sometimes referenced as phase corrected PWM.
#define AUX_TIMER2_SHDWTARGET_VALUE_W                                       16
#define AUX_TIMER2_SHDWTARGET_VALUE_M                               0x0000FFFF
#define AUX_TIMER2_SHDWTARGET_VALUE_S                                        0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CNTR
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// 16 bit current counter value.
#define AUX_TIMER2_CNTR_VALUE_W                                             16
#define AUX_TIMER2_CNTR_VALUE_M                                     0x0000FFFF
#define AUX_TIMER2_CNTR_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_PRECFG
//
//*****************************************************************************
// Field:   [7:0] CLKDIV
//
// Clock division.
//
// CLKDIV determines the timer clock frequency for counter, synchronization,
// and timer event updates. The timer clock frequency is the clock selected by
// AUX_SYSIF:TIMER2CLKCTL.SRC divided by (CLKDIV + 1). This inverse is the
// timer clock period.
//
// 0x00: Divide by 1.
// 0x01: Divide by 2.
// ...
// 0xFF: Divide by 256.
#define AUX_TIMER2_PRECFG_CLKDIV_W                                           8
#define AUX_TIMER2_PRECFG_CLKDIV_M                                  0x000000FF
#define AUX_TIMER2_PRECFG_CLKDIV_S                                           0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_EVCTL
//
//*****************************************************************************
// Field:     [7] EV3_SET
//
// Set event 3.
//
// Write 1 to set event 3.
#define AUX_TIMER2_EVCTL_EV3_SET                                    0x00000080
#define AUX_TIMER2_EVCTL_EV3_SET_BITN                                        7
#define AUX_TIMER2_EVCTL_EV3_SET_M                                  0x00000080
#define AUX_TIMER2_EVCTL_EV3_SET_S                                           7

// Field:     [6] EV3_CLR
//
// Clear event 3.
//
// Write 1 to clear event 3.
#define AUX_TIMER2_EVCTL_EV3_CLR                                    0x00000040
#define AUX_TIMER2_EVCTL_EV3_CLR_BITN                                        6
#define AUX_TIMER2_EVCTL_EV3_CLR_M                                  0x00000040
#define AUX_TIMER2_EVCTL_EV3_CLR_S                                           6

// Field:     [5] EV2_SET
//
// Set event 2.
//
// Write 1 to set event 2.
#define AUX_TIMER2_EVCTL_EV2_SET                                    0x00000020
#define AUX_TIMER2_EVCTL_EV2_SET_BITN                                        5
#define AUX_TIMER2_EVCTL_EV2_SET_M                                  0x00000020
#define AUX_TIMER2_EVCTL_EV2_SET_S                                           5

// Field:     [4] EV2_CLR
//
// Clear event 2.
//
// Write 1 to clear event 2.
#define AUX_TIMER2_EVCTL_EV2_CLR                                    0x00000010
#define AUX_TIMER2_EVCTL_EV2_CLR_BITN                                        4
#define AUX_TIMER2_EVCTL_EV2_CLR_M                                  0x00000010
#define AUX_TIMER2_EVCTL_EV2_CLR_S                                           4

// Field:     [3] EV1_SET
//
// Set event 1.
//
// Write 1 to set event 1.
#define AUX_TIMER2_EVCTL_EV1_SET                                    0x00000008
#define AUX_TIMER2_EVCTL_EV1_SET_BITN                                        3
#define AUX_TIMER2_EVCTL_EV1_SET_M                                  0x00000008
#define AUX_TIMER2_EVCTL_EV1_SET_S                                           3

// Field:     [2] EV1_CLR
//
// Clear event 1.
//
// Write 1 to clear event 1.
#define AUX_TIMER2_EVCTL_EV1_CLR                                    0x00000004
#define AUX_TIMER2_EVCTL_EV1_CLR_BITN                                        2
#define AUX_TIMER2_EVCTL_EV1_CLR_M                                  0x00000004
#define AUX_TIMER2_EVCTL_EV1_CLR_S                                           2

// Field:     [1] EV0_SET
//
// Set event 0.
//
// Write 1 to set event 0.
#define AUX_TIMER2_EVCTL_EV0_SET                                    0x00000002
#define AUX_TIMER2_EVCTL_EV0_SET_BITN                                        1
#define AUX_TIMER2_EVCTL_EV0_SET_M                                  0x00000002
#define AUX_TIMER2_EVCTL_EV0_SET_S                                           1

// Field:     [0] EV0_CLR
//
// Clear event 0.
//
// Write 1 to clear event 0.
#define AUX_TIMER2_EVCTL_EV0_CLR                                    0x00000001
#define AUX_TIMER2_EVCTL_EV0_CLR_BITN                                        0
#define AUX_TIMER2_EVCTL_EV0_CLR_M                                  0x00000001
#define AUX_TIMER2_EVCTL_EV0_CLR_S                                           0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_PULSETRIG
//
//*****************************************************************************
// Field:     [0] TRIG
//
// Pulse trigger.
//
// Write 1 to generate a pulse to AUX_EVCTL:EVSTAT3.AUX_TIMER2_PULSE. Pulse
// width equals the duty cycle of AUX_SYSIF:TIMER2CLKCTL.SRC.
#define AUX_TIMER2_PULSETRIG_TRIG                                   0x00000001
#define AUX_TIMER2_PULSETRIG_TRIG_BITN                                       0
#define AUX_TIMER2_PULSETRIG_TRIG_M                                 0x00000001
#define AUX_TIMER2_PULSETRIG_TRIG_S                                          0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH0EVCFG
//
//*****************************************************************************
// Field:     [7] EV3_GEN
//
// Event 3 enable.
//
// 0: Channel 0 does not control event 3.
// 1: Channel 0 controls event 3.
//
//  When 0 < CCACT < 8, EV3_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH0EVCFG_EV3_GEN                                 0x00000080
#define AUX_TIMER2_CH0EVCFG_EV3_GEN_BITN                                     7
#define AUX_TIMER2_CH0EVCFG_EV3_GEN_M                               0x00000080
#define AUX_TIMER2_CH0EVCFG_EV3_GEN_S                                        7

// Field:     [6] EV2_GEN
//
// Event 2 enable.
//
// 0: Channel 0 does not control event 2.
// 1: Channel 0 controls event 2.
//
//  When 0 < CCACT < 8, EV2_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH0EVCFG_EV2_GEN                                 0x00000040
#define AUX_TIMER2_CH0EVCFG_EV2_GEN_BITN                                     6
#define AUX_TIMER2_CH0EVCFG_EV2_GEN_M                               0x00000040
#define AUX_TIMER2_CH0EVCFG_EV2_GEN_S                                        6

// Field:     [5] EV1_GEN
//
// Event 1 enable.
//
// 0: Channel 0 does not control event 1.
// 1: Channel 0 controls event 1.
//
//  When 0 < CCACT < 8, EV1_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH0EVCFG_EV1_GEN                                 0x00000020
#define AUX_TIMER2_CH0EVCFG_EV1_GEN_BITN                                     5
#define AUX_TIMER2_CH0EVCFG_EV1_GEN_M                               0x00000020
#define AUX_TIMER2_CH0EVCFG_EV1_GEN_S                                        5

// Field:     [4] EV0_GEN
//
// Event 0 enable.
//
// 0: Channel 0 does not control event 0.
// 1: Channel 0 controls event 0.
//
//  When 0 < CCACT < 8, EV0_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH0EVCFG_EV0_GEN                                 0x00000010
#define AUX_TIMER2_CH0EVCFG_EV0_GEN_BITN                                     4
#define AUX_TIMER2_CH0EVCFG_EV0_GEN_M                               0x00000010
#define AUX_TIMER2_CH0EVCFG_EV0_GEN_S                                        4

// Field:   [3:0] CCACT
//
// Capture-Compare action.
//
// Capture-Compare action defines 15 different channel functions that utilize
// capture, compare, and zero events.
// ENUMs:
// PULSE_ON_CMP             Pulse on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP               Toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
// SET_ON_CMP               Set on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
// CLR_ON_CMP               Clear on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
// SET_ON_0_TGL_ON_CMP      Set on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UP_PER
//                          for edge-aligned PWM generation. Duty cycle is
//                          given by:
//
//                          When CH0CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle =
//                          CH0CC.VALUE / ( TARGET.VALUE + 1 ).
//
//                          When CH0CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 1.
//
//                          Enabled events are
//                          cleared when CH0CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP      Clear on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UPDWN_PER
//                          for center-aligned PWM generation. Duty cycle
//                          is given by:
//
//                          When CH0CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle = 1 - (
//                          CH0CC.VALUE / TARGET.VALUE ).
//
//                          When CH0CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 0.
//
//                          Enabled events are set
//                          when CH0CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT              Set on capture repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH0CC.VALUE.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Select this function
//                          with no event enable.
//                           - Configure CH0CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          enable events.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// PER_PULSE_WIDTH_MEAS     Period and pulse width measurement.
//
//                          Continuously capture
//                          period and pulse width of the signal selected
//                          by CH0CCFG.CAPT_SRC relative to the signal edge
//                          given by CH0CCFG.EDGE.
//
//                          Set enabled events when
//                          CH0CC.VALUE contains signal period and
//                          CH0PCC.VALUE contains signal pulse width.
//
//                          Notes:
//                          - Make sure that you
//                          configure CH0CCFG.CAPT_SRC and CCACT when
//                          CTL.MODE equals DIS, then set CTL.MODE to
//                          UP_ONCE or UP_PER.
//                          - The counter restarts in
//                          the selected timer mode when CH0CC.VALUE
//                          contains the signal period.
//                          - If more than one
//                          channel uses this function, the channels will
//                          perform this function one at a time. The
//                          channel with lowest number has priority and
//                          performs the function first. Next measurement
//                          starts when current measurement completes
//                          successfully or times out. A timeout occurs
//                          when counter equals target.
//                          - If you want to observe
//                          a timeout event configure another channel to
//                          SET_ON_CAPT.
//
//                          Signal property
//                          requirements:
//                          - Signal Period >= 2 * (
//                          1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal Period <= 65535
//                          * (1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal low and high
//                          phase >= (1 + PRECFG.CLKDIV ) * timer clock
//                          period.
// PULSE_ON_CMP_DIS         Pulse on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP_DIS           Toggle on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_CMP_DIS           Set on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CH0CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// CLR_ON_CMP_DIS           Clear on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_0_TGL_ON_CMP_DIS  Set on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are
//                          cleared when CH0CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP_DIS  Clear on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH0CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are set
//                          when CH0CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT_DIS          Set on capture, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH0CC.VALUE.
//                          - Disable channel.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Set CCACT to
//                          SET_ON_CAPT with no event enable.
//                           - Configure CH0CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          set CCACT to SET_ON_CAPT_DIS. Event enable is
//                          optional.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// DIS                      Disable channel.
#define AUX_TIMER2_CH0EVCFG_CCACT_W                                          4
#define AUX_TIMER2_CH0EVCFG_CCACT_M                                 0x0000000F
#define AUX_TIMER2_CH0EVCFG_CCACT_S                                          0
#define AUX_TIMER2_CH0EVCFG_CCACT_PULSE_ON_CMP                      0x0000000F
#define AUX_TIMER2_CH0EVCFG_CCACT_TGL_ON_CMP                        0x0000000E
#define AUX_TIMER2_CH0EVCFG_CCACT_SET_ON_CMP                        0x0000000D
#define AUX_TIMER2_CH0EVCFG_CCACT_CLR_ON_CMP                        0x0000000C
#define AUX_TIMER2_CH0EVCFG_CCACT_SET_ON_0_TGL_ON_CMP               0x0000000B
#define AUX_TIMER2_CH0EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP               0x0000000A
#define AUX_TIMER2_CH0EVCFG_CCACT_SET_ON_CAPT                       0x00000009
#define AUX_TIMER2_CH0EVCFG_CCACT_PER_PULSE_WIDTH_MEAS              0x00000008
#define AUX_TIMER2_CH0EVCFG_CCACT_PULSE_ON_CMP_DIS                  0x00000007
#define AUX_TIMER2_CH0EVCFG_CCACT_TGL_ON_CMP_DIS                    0x00000006
#define AUX_TIMER2_CH0EVCFG_CCACT_SET_ON_CMP_DIS                    0x00000005
#define AUX_TIMER2_CH0EVCFG_CCACT_CLR_ON_CMP_DIS                    0x00000004
#define AUX_TIMER2_CH0EVCFG_CCACT_SET_ON_0_TGL_ON_CMP_DIS           0x00000003
#define AUX_TIMER2_CH0EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP_DIS           0x00000002
#define AUX_TIMER2_CH0EVCFG_CCACT_SET_ON_CAPT_DIS                   0x00000001
#define AUX_TIMER2_CH0EVCFG_CCACT_DIS                               0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH0CCFG
//
//*****************************************************************************
// Field:   [6:1] CAPT_SRC
//
// Select capture signal source from the asynchronous AUX event bus.
//
// The selected signal enters the edge-detection circuit. False capture events
// can occur when:
// - the edge-detection circuit contains expired signal samples and the circuit
// is enabled without flush as described in CH0EVCFG
// - this register is reconfigured while CTL.MODE is different from DIS.
//
// You can avoid false capture events. When wanted channel function is:
// - SET_ON_CAPT_DIS, see description for SET_ON_CAPT_DIS in CH0EVCFG.CCACT.
// - SET_ON_CAPT, see description for SET_ON_CAPT in CH0EVCFG.CCACT.
// - PER_PULSE_WIDTH_MEAS, see description for PER_PULSE_WIDTH_MEAS in
// CH0EVCFG.CCACT.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
// ENUMs:
// NO_EVENT                 No event.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
// AUX_TIMER1_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_EV3           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                AUX_EVCTL:EVSTAT2.AUX_COMPB
// AUX_COMPA                AUX_EVCTL:EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              AUX_EVCTL:EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              AUX_EVCTL:EVSTAT2.MCU_OBSMUX0
// MCU_EV                   AUX_EVCTL:EVSTAT2.MCU_EV
// ACLK_REF                 AUX_EVCTL:EVSTAT2.ACLK_REF
// VDDR_RECHARGE            AUX_EVCTL:EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               AUX_EVCTL:EVSTAT2.MCU_ACTIVE
// PWR_DWN                  AUX_EVCTL:EVSTAT2.PWR_DWN
// SCLK_LF                  AUX_EVCTL:EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      AUX_EVCTL:EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       AUX_EVCTL:EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             AUX_EVCTL:EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              AUX_EVCTL:EVSTAT2.AON_RTC_CH2
// MANUAL_EV                AUX_EVCTL:EVSTAT2.MANUAL_EV
// AUXIO31                  AUX_EVCTL:EVSTAT1.AUXIO31
// AUXIO30                  AUX_EVCTL:EVSTAT1.AUXIO30
// AUXIO29                  AUX_EVCTL:EVSTAT1.AUXIO29
// AUXIO28                  AUX_EVCTL:EVSTAT1.AUXIO28
// AUXIO27                  AUX_EVCTL:EVSTAT1.AUXIO27
// AUXIO26                  AUX_EVCTL:EVSTAT1.AUXIO26
// AUXIO25                  AUX_EVCTL:EVSTAT1.AUXIO25
// AUXIO24                  AUX_EVCTL:EVSTAT1.AUXIO24
// AUXIO23                  AUX_EVCTL:EVSTAT1.AUXIO23
// AUXIO22                  AUX_EVCTL:EVSTAT1.AUXIO22
// AUXIO21                  AUX_EVCTL:EVSTAT1.AUXIO21
// AUXIO20                  AUX_EVCTL:EVSTAT1.AUXIO20
// AUXIO19                  AUX_EVCTL:EVSTAT1.AUXIO19
// AUXIO18                  AUX_EVCTL:EVSTAT1.AUXIO18
// AUXIO17                  AUX_EVCTL:EVSTAT1.AUXIO17
// AUXIO16                  AUX_EVCTL:EVSTAT1.AUXIO16
// AUXIO15                  AUX_EVCTL:EVSTAT0.AUXIO15
// AUXIO14                  AUX_EVCTL:EVSTAT0.AUXIO14
// AUXIO13                  AUX_EVCTL:EVSTAT0.AUXIO13
// AUXIO12                  AUX_EVCTL:EVSTAT0.AUXIO12
// AUXIO11                  AUX_EVCTL:EVSTAT0.AUXIO11
// AUXIO10                  AUX_EVCTL:EVSTAT0.AUXIO10
// AUXIO9                   AUX_EVCTL:EVSTAT0.AUXIO9
// AUXIO8                   AUX_EVCTL:EVSTAT0.AUXIO8
// AUXIO7                   AUX_EVCTL:EVSTAT0.AUXIO7
// AUXIO6                   AUX_EVCTL:EVSTAT0.AUXIO6
// AUXIO5                   AUX_EVCTL:EVSTAT0.AUXIO5
// AUXIO4                   AUX_EVCTL:EVSTAT0.AUXIO4
// AUXIO3                   AUX_EVCTL:EVSTAT0.AUXIO3
// AUXIO2                   AUX_EVCTL:EVSTAT0.AUXIO2
// AUXIO1                   AUX_EVCTL:EVSTAT0.AUXIO1
// AUXIO0                   AUX_EVCTL:EVSTAT0.AUXIO0
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_W                                        6
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_M                               0x0000007E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_S                                        1
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_NO_EVENT                        0x0000007E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000007A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x00000078
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x00000076
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_ADC_IRQ                     0x00000074
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_ADC_DONE                    0x00000072
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_ISRC_RESET_N                0x00000070
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_TDC_DONE                    0x0000006E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_TIMER0_EV                   0x0000006C
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_TIMER1_EV                   0x0000006A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_TIMER2_EV3                  0x00000066
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_TIMER2_EV2                  0x00000064
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_TIMER2_EV1                  0x00000062
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_TIMER2_EV0                  0x00000060
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_COMPB                       0x0000005E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUX_COMPA                       0x0000005C
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_MCU_OBSMUX1                     0x0000005A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_MCU_OBSMUX0                     0x00000058
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_MCU_EV                          0x00000056
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_ACLK_REF                        0x00000054
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_VDDR_RECHARGE                   0x00000052
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_MCU_ACTIVE                      0x00000050
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_PWR_DWN                         0x0000004E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_SCLK_LF                         0x0000004C
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AON_BATMON_TEMP_UPD             0x0000004A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AON_BATMON_BAT_UPD              0x00000048
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AON_RTC_4KHZ                    0x00000046
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AON_RTC_CH2_DLY                 0x00000044
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AON_RTC_CH2                     0x00000042
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_MANUAL_EV                       0x00000040
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO31                         0x0000003E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO30                         0x0000003C
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO29                         0x0000003A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO28                         0x00000038
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO27                         0x00000036
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO26                         0x00000034
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO25                         0x00000032
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO24                         0x00000030
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO23                         0x0000002E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO22                         0x0000002C
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO21                         0x0000002A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO20                         0x00000028
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO19                         0x00000026
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO18                         0x00000024
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO17                         0x00000022
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO16                         0x00000020
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO15                         0x0000001E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO14                         0x0000001C
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO13                         0x0000001A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO12                         0x00000018
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO11                         0x00000016
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO10                         0x00000014
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO9                          0x00000012
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO8                          0x00000010
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO7                          0x0000000E
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO6                          0x0000000C
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO5                          0x0000000A
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO4                          0x00000008
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO3                          0x00000006
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO2                          0x00000004
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO1                          0x00000002
#define AUX_TIMER2_CH0CCFG_CAPT_SRC_AUXIO0                          0x00000000

// Field:     [0] EDGE
//
// Edge configuration.
//
// Channel captures counter value at selected edge on signal source selected by
// CAPT_SRC. See CH0EVCFG.CCACT.
// ENUMs:
// RISING                   Capture CNTR.VALUE at rising edge of CAPT_SRC.
// FALLING                  Capture CNTR.VALUE at falling edge of CAPT_SRC.
#define AUX_TIMER2_CH0CCFG_EDGE                                     0x00000001
#define AUX_TIMER2_CH0CCFG_EDGE_BITN                                         0
#define AUX_TIMER2_CH0CCFG_EDGE_M                                   0x00000001
#define AUX_TIMER2_CH0CCFG_EDGE_S                                            0
#define AUX_TIMER2_CH0CCFG_EDGE_RISING                              0x00000001
#define AUX_TIMER2_CH0CCFG_EDGE_FALLING                             0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH0PCC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Pipeline Capture Compare value.
//
// 16-bit user defined pipeline compare value or channel-updated capture value.
//
// Compare mode:
// An update of VALUE will be transferred to CH0CC.VALUE when the next
// CNTR.VALUE is zero and CTL.MODE is different from DIS. This is useful for
// PWM generation and prevents jitter on the edges of the generated signal.
//
// Capture mode:
// When CH0EVCFG.CCACT equals PER_PULSE_WIDTH_MEAS then VALUE contains the
// width of the low or high phase of the selected signal. This is specified by
// CH0CCFG.EDGE and CH0CCFG.CAPT_SRC.
#define AUX_TIMER2_CH0PCC_VALUE_W                                           16
#define AUX_TIMER2_CH0PCC_VALUE_M                                   0x0000FFFF
#define AUX_TIMER2_CH0PCC_VALUE_S                                            0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH0CC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Capture Compare value.
//
// 16-bit user defined compare value or channel-updated capture value.
//
// Compare mode:
// VALUE is compared against CNTR.VALUE and an event is generated as specified
// by CH0EVCFG.CCACT when these are equal.
//
// Capture mode:
// The current counter value is stored in VALUE when a capture event occurs.
// CH0EVCFG.CCACT determines if VALUE is a signal period or a regular capture
// value.
#define AUX_TIMER2_CH0CC_VALUE_W                                            16
#define AUX_TIMER2_CH0CC_VALUE_M                                    0x0000FFFF
#define AUX_TIMER2_CH0CC_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH1EVCFG
//
//*****************************************************************************
// Field:     [7] EV3_GEN
//
// Event 3 enable.
//
// 0: Channel 1 does not control event 3.
// 1: Channel 1 controls event 3.
//
//  When 0 < CCACT < 8, EV3_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH1EVCFG_EV3_GEN                                 0x00000080
#define AUX_TIMER2_CH1EVCFG_EV3_GEN_BITN                                     7
#define AUX_TIMER2_CH1EVCFG_EV3_GEN_M                               0x00000080
#define AUX_TIMER2_CH1EVCFG_EV3_GEN_S                                        7

// Field:     [6] EV2_GEN
//
// Event 2 enable.
//
// 0: Channel 1 does not control event 2.
// 1: Channel 1 controls event 2.
//
//  When 0 < CCACT < 8, EV2_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH1EVCFG_EV2_GEN                                 0x00000040
#define AUX_TIMER2_CH1EVCFG_EV2_GEN_BITN                                     6
#define AUX_TIMER2_CH1EVCFG_EV2_GEN_M                               0x00000040
#define AUX_TIMER2_CH1EVCFG_EV2_GEN_S                                        6

// Field:     [5] EV1_GEN
//
// Event 1 enable.
//
// 0: Channel 1 does not control event 1.
// 1: Channel 1 controls event 1.
//
//  When 0 < CCACT < 8, EV1_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH1EVCFG_EV1_GEN                                 0x00000020
#define AUX_TIMER2_CH1EVCFG_EV1_GEN_BITN                                     5
#define AUX_TIMER2_CH1EVCFG_EV1_GEN_M                               0x00000020
#define AUX_TIMER2_CH1EVCFG_EV1_GEN_S                                        5

// Field:     [4] EV0_GEN
//
// Event 0 enable.
//
// 0: Channel 1 does not control event 0.
// 1: Channel 1 controls event 0.
//
//  When 0 < CCACT < 8, EV0_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH1EVCFG_EV0_GEN                                 0x00000010
#define AUX_TIMER2_CH1EVCFG_EV0_GEN_BITN                                     4
#define AUX_TIMER2_CH1EVCFG_EV0_GEN_M                               0x00000010
#define AUX_TIMER2_CH1EVCFG_EV0_GEN_S                                        4

// Field:   [3:0] CCACT
//
// Capture-Compare action.
//
// Capture-Compare action defines 15 different channel functions that utilize
// capture, compare, and zero events.
// ENUMs:
// PULSE_ON_CMP             Pulse on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP               Toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
// SET_ON_CMP               Set on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
// CLR_ON_CMP               Clear on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
// SET_ON_0_TGL_ON_CMP      Set on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UP_PER
//                          for edge-aligned PWM generation. Duty cycle is
//                          given by:
//
//                          When CH1CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle =
//                          CH1CC.VALUE / ( TARGET.VALUE + 1 ).
//
//                          When CH1CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 1.
//
//                          Enabled events are
//                          cleared when CH1CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP      Clear on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UPDWN_PER
//                          for center-aligned PWM generation. Duty cycle
//                          is given by:
//
//                          When CH1CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle = 1 - (
//                          CH1CC.VALUE / TARGET.VALUE ).
//
//                          When CH1CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 0.
//
//                          Enabled events are set
//                          when CH1CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT              Set on capture repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH1CC.VALUE.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Select this function
//                          with no event enable.
//                           - Configure CH1CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          enable events.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// PER_PULSE_WIDTH_MEAS     Period and pulse width measurement.
//
//                          Continuously capture
//                          period and pulse width of the signal selected
//                          by CH1CCFG.CAPT_SRC relative to the signal edge
//                          given by CH1CCFG.EDGE.
//
//                          Set enabled events when
//                          CH1CC.VALUE contains signal period and
//                          CH1PCC.VALUE contains signal pulse width.
//
//                          Notes:
//                          - Make sure that you
//                          configure CH1CCFG.CAPT_SRC and CCACT when
//                          CTL.MODE equals DIS, then set CTL.MODE to
//                          UP_ONCE or UP_PER.
//                          - The counter restarts in
//                          the selected timer mode when CH1CC.VALUE
//                          contains the signal period.
//                          - If more than one
//                          channel uses this function, the channels will
//                          perform this function one at a time. The
//                          channel with lowest number has priority and
//                          performs the function first. Next measurement
//                          starts when current measurement completes
//                          successfully or times out. A timeout occurs
//                          when counter equals target.
//                          - If you want to observe
//                          a timeout event configure another channel to
//                          SET_ON_CAPT.
//
//                          Signal property
//                          requirements:
//                          - Signal Period >= 2 * (
//                          1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal Period <= 65535
//                          * (1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal low and high
//                          phase >= (1 + PRECFG.CLKDIV ) * timer clock
//                          period.
// PULSE_ON_CMP_DIS         Pulse on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP_DIS           Toggle on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_CMP_DIS           Set on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CH1CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// CLR_ON_CMP_DIS           Clear on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_0_TGL_ON_CMP_DIS  Set on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are
//                          cleared when CH1CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP_DIS  Clear on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH1CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are set
//                          when CH1CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT_DIS          Set on capture, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH1CC.VALUE.
//                          - Disable channel.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Set CCACT to
//                          SET_ON_CAPT with no event enable.
//                           - Configure CH1CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          set CCACT to SET_ON_CAPT_DIS. Event enable is
//                          optional.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// DIS                      Disable channel.
#define AUX_TIMER2_CH1EVCFG_CCACT_W                                          4
#define AUX_TIMER2_CH1EVCFG_CCACT_M                                 0x0000000F
#define AUX_TIMER2_CH1EVCFG_CCACT_S                                          0
#define AUX_TIMER2_CH1EVCFG_CCACT_PULSE_ON_CMP                      0x0000000F
#define AUX_TIMER2_CH1EVCFG_CCACT_TGL_ON_CMP                        0x0000000E
#define AUX_TIMER2_CH1EVCFG_CCACT_SET_ON_CMP                        0x0000000D
#define AUX_TIMER2_CH1EVCFG_CCACT_CLR_ON_CMP                        0x0000000C
#define AUX_TIMER2_CH1EVCFG_CCACT_SET_ON_0_TGL_ON_CMP               0x0000000B
#define AUX_TIMER2_CH1EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP               0x0000000A
#define AUX_TIMER2_CH1EVCFG_CCACT_SET_ON_CAPT                       0x00000009
#define AUX_TIMER2_CH1EVCFG_CCACT_PER_PULSE_WIDTH_MEAS              0x00000008
#define AUX_TIMER2_CH1EVCFG_CCACT_PULSE_ON_CMP_DIS                  0x00000007
#define AUX_TIMER2_CH1EVCFG_CCACT_TGL_ON_CMP_DIS                    0x00000006
#define AUX_TIMER2_CH1EVCFG_CCACT_SET_ON_CMP_DIS                    0x00000005
#define AUX_TIMER2_CH1EVCFG_CCACT_CLR_ON_CMP_DIS                    0x00000004
#define AUX_TIMER2_CH1EVCFG_CCACT_SET_ON_0_TGL_ON_CMP_DIS           0x00000003
#define AUX_TIMER2_CH1EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP_DIS           0x00000002
#define AUX_TIMER2_CH1EVCFG_CCACT_SET_ON_CAPT_DIS                   0x00000001
#define AUX_TIMER2_CH1EVCFG_CCACT_DIS                               0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH1CCFG
//
//*****************************************************************************
// Field:   [6:1] CAPT_SRC
//
// Select capture signal source from the asynchronous AUX event bus.
//
// The selected signal enters the edge-detection circuit. False capture events
// can occur when:
// - the edge-detection circuit contains expired signal samples and the circuit
// is enabled without flush as described in CH1EVCFG
// - this register is reconfigured while CTL.MODE is different from DIS.
//
// You can avoid false capture events. When wanted channel function is:
// - SET_ON_CAPT_DIS, see description for SET_ON_CAPT_DIS in CH1EVCFG.CCACT.
// - SET_ON_CAPT, see description for SET_ON_CAPT in CH1EVCFG.CCACT.
// - PER_PULSE_WIDTH_MEAS, see description for PER_PULSE_WIDTH_MEAS in
// CH1EVCFG.CCACT.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
// ENUMs:
// NO_EVENT                 No event.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
// AUX_TIMER1_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_EV3           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                AUX_EVCTL:EVSTAT2.AUX_COMPB
// AUX_COMPA                AUX_EVCTL:EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              AUX_EVCTL:EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              AUX_EVCTL:EVSTAT2.MCU_OBSMUX0
// MCU_EV                   AUX_EVCTL:EVSTAT2.MCU_EV
// ACLK_REF                 AUX_EVCTL:EVSTAT2.ACLK_REF
// VDDR_RECHARGE            AUX_EVCTL:EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               AUX_EVCTL:EVSTAT2.MCU_ACTIVE
// PWR_DWN                  AUX_EVCTL:EVSTAT2.PWR_DWN
// SCLK_LF                  AUX_EVCTL:EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      AUX_EVCTL:EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       AUX_EVCTL:EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             AUX_EVCTL:EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              AUX_EVCTL:EVSTAT2.AON_RTC_CH2
// MANUAL_EV                AUX_EVCTL:EVSTAT2.MANUAL_EV
// AUXIO31                  AUX_EVCTL:EVSTAT1.AUXIO31
// AUXIO30                  AUX_EVCTL:EVSTAT1.AUXIO30
// AUXIO29                  AUX_EVCTL:EVSTAT1.AUXIO29
// AUXIO28                  AUX_EVCTL:EVSTAT1.AUXIO28
// AUXIO27                  AUX_EVCTL:EVSTAT1.AUXIO27
// AUXIO26                  AUX_EVCTL:EVSTAT1.AUXIO26
// AUXIO25                  AUX_EVCTL:EVSTAT1.AUXIO25
// AUXIO24                  AUX_EVCTL:EVSTAT1.AUXIO24
// AUXIO23                  AUX_EVCTL:EVSTAT1.AUXIO23
// AUXIO22                  AUX_EVCTL:EVSTAT1.AUXIO22
// AUXIO21                  AUX_EVCTL:EVSTAT1.AUXIO21
// AUXIO20                  AUX_EVCTL:EVSTAT1.AUXIO20
// AUXIO19                  AUX_EVCTL:EVSTAT1.AUXIO19
// AUXIO18                  AUX_EVCTL:EVSTAT1.AUXIO18
// AUXIO17                  AUX_EVCTL:EVSTAT1.AUXIO17
// AUXIO16                  AUX_EVCTL:EVSTAT1.AUXIO16
// AUXIO15                  AUX_EVCTL:EVSTAT0.AUXIO15
// AUXIO14                  AUX_EVCTL:EVSTAT0.AUXIO14
// AUXIO13                  AUX_EVCTL:EVSTAT0.AUXIO13
// AUXIO12                  AUX_EVCTL:EVSTAT0.AUXIO12
// AUXIO11                  AUX_EVCTL:EVSTAT0.AUXIO11
// AUXIO10                  AUX_EVCTL:EVSTAT0.AUXIO10
// AUXIO9                   AUX_EVCTL:EVSTAT0.AUXIO9
// AUXIO8                   AUX_EVCTL:EVSTAT0.AUXIO8
// AUXIO7                   AUX_EVCTL:EVSTAT0.AUXIO7
// AUXIO6                   AUX_EVCTL:EVSTAT0.AUXIO6
// AUXIO5                   AUX_EVCTL:EVSTAT0.AUXIO5
// AUXIO4                   AUX_EVCTL:EVSTAT0.AUXIO4
// AUXIO3                   AUX_EVCTL:EVSTAT0.AUXIO3
// AUXIO2                   AUX_EVCTL:EVSTAT0.AUXIO2
// AUXIO1                   AUX_EVCTL:EVSTAT0.AUXIO1
// AUXIO0                   AUX_EVCTL:EVSTAT0.AUXIO0
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_W                                        6
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_M                               0x0000007E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_S                                        1
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_NO_EVENT                        0x0000007E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000007A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x00000078
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x00000076
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_ADC_IRQ                     0x00000074
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_ADC_DONE                    0x00000072
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_ISRC_RESET_N                0x00000070
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_TDC_DONE                    0x0000006E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_TIMER0_EV                   0x0000006C
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_TIMER1_EV                   0x0000006A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_TIMER2_EV3                  0x00000066
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_TIMER2_EV2                  0x00000064
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_TIMER2_EV1                  0x00000062
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_TIMER2_EV0                  0x00000060
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_COMPB                       0x0000005E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUX_COMPA                       0x0000005C
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_MCU_OBSMUX1                     0x0000005A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_MCU_OBSMUX0                     0x00000058
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_MCU_EV                          0x00000056
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_ACLK_REF                        0x00000054
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_VDDR_RECHARGE                   0x00000052
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_MCU_ACTIVE                      0x00000050
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_PWR_DWN                         0x0000004E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_SCLK_LF                         0x0000004C
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AON_BATMON_TEMP_UPD             0x0000004A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AON_BATMON_BAT_UPD              0x00000048
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AON_RTC_4KHZ                    0x00000046
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AON_RTC_CH2_DLY                 0x00000044
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AON_RTC_CH2                     0x00000042
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_MANUAL_EV                       0x00000040
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO31                         0x0000003E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO30                         0x0000003C
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO29                         0x0000003A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO28                         0x00000038
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO27                         0x00000036
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO26                         0x00000034
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO25                         0x00000032
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO24                         0x00000030
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO23                         0x0000002E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO22                         0x0000002C
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO21                         0x0000002A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO20                         0x00000028
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO19                         0x00000026
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO18                         0x00000024
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO17                         0x00000022
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO16                         0x00000020
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO15                         0x0000001E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO14                         0x0000001C
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO13                         0x0000001A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO12                         0x00000018
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO11                         0x00000016
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO10                         0x00000014
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO9                          0x00000012
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO8                          0x00000010
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO7                          0x0000000E
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO6                          0x0000000C
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO5                          0x0000000A
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO4                          0x00000008
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO3                          0x00000006
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO2                          0x00000004
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO1                          0x00000002
#define AUX_TIMER2_CH1CCFG_CAPT_SRC_AUXIO0                          0x00000000

// Field:     [0] EDGE
//
// Edge configuration.
//
// Channel captures counter value at selected edge on signal source selected by
// CAPT_SRC. See CH1EVCFG.CCACT.
// ENUMs:
// RISING                   Capture CNTR.VALUE at rising edge of CAPT_SRC.
// FALLING                  Capture CNTR.VALUE at falling edge of CAPT_SRC.
#define AUX_TIMER2_CH1CCFG_EDGE                                     0x00000001
#define AUX_TIMER2_CH1CCFG_EDGE_BITN                                         0
#define AUX_TIMER2_CH1CCFG_EDGE_M                                   0x00000001
#define AUX_TIMER2_CH1CCFG_EDGE_S                                            0
#define AUX_TIMER2_CH1CCFG_EDGE_RISING                              0x00000001
#define AUX_TIMER2_CH1CCFG_EDGE_FALLING                             0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH1PCC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Pipeline Capture Compare value.
//
// 16-bit user defined pipeline compare value or channel-updated capture value.
//
// Compare mode:
// An update of VALUE will be transferred to CH1CC.VALUE when the next
// CNTR.VALUE is zero and CTL.MODE is different from DIS. This is useful for
// PWM generation and prevents jitter on the edges of the generated signal.
//
// Capture mode:
// When CH1EVCFG.CCACT equals PER_PULSE_WIDTH_MEAS then VALUE contains the
// width of the low or high phase of the selected signal. This is specified by
// CH1CCFG.EDGE and CH1CCFG.CAPT_SRC.
#define AUX_TIMER2_CH1PCC_VALUE_W                                           16
#define AUX_TIMER2_CH1PCC_VALUE_M                                   0x0000FFFF
#define AUX_TIMER2_CH1PCC_VALUE_S                                            0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH1CC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Capture Compare value.
//
// 16-bit user defined compare value or channel-updated capture value.
//
// Compare mode:
// VALUE is compared against CNTR.VALUE and an event is generated as specified
// by CH1EVCFG.CCACT when these are equal.
//
// Capture mode:
// The current counter value is stored in VALUE when a capture event occurs.
// CH1EVCFG.CCACT determines if VALUE is a signal period or a regular capture
// value.
#define AUX_TIMER2_CH1CC_VALUE_W                                            16
#define AUX_TIMER2_CH1CC_VALUE_M                                    0x0000FFFF
#define AUX_TIMER2_CH1CC_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH2EVCFG
//
//*****************************************************************************
// Field:     [7] EV3_GEN
//
// Event 3 enable.
//
// 0: Channel 2 does not control event 3.
// 1: Channel 2 controls event 3.
//
//  When 0 < CCACT < 8, EV3_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH2EVCFG_EV3_GEN                                 0x00000080
#define AUX_TIMER2_CH2EVCFG_EV3_GEN_BITN                                     7
#define AUX_TIMER2_CH2EVCFG_EV3_GEN_M                               0x00000080
#define AUX_TIMER2_CH2EVCFG_EV3_GEN_S                                        7

// Field:     [6] EV2_GEN
//
// Event 2 enable.
//
// 0: Channel 2 does not control event 2.
// 1: Channel 2 controls event 2.
//
//  When 0 < CCACT < 8, EV2_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH2EVCFG_EV2_GEN                                 0x00000040
#define AUX_TIMER2_CH2EVCFG_EV2_GEN_BITN                                     6
#define AUX_TIMER2_CH2EVCFG_EV2_GEN_M                               0x00000040
#define AUX_TIMER2_CH2EVCFG_EV2_GEN_S                                        6

// Field:     [5] EV1_GEN
//
// Event 1 enable.
//
// 0: Channel 2 does not control event 1.
// 1: Channel 2 controls event 1.
//
//  When 0 < CCACT < 8, EV1_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH2EVCFG_EV1_GEN                                 0x00000020
#define AUX_TIMER2_CH2EVCFG_EV1_GEN_BITN                                     5
#define AUX_TIMER2_CH2EVCFG_EV1_GEN_M                               0x00000020
#define AUX_TIMER2_CH2EVCFG_EV1_GEN_S                                        5

// Field:     [4] EV0_GEN
//
// Event 0 enable.
//
// 0: Channel 2 does not control event 0.
// 1: Channel 2 controls event 0.
//
//  When 0 < CCACT < 8, EV0_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH2EVCFG_EV0_GEN                                 0x00000010
#define AUX_TIMER2_CH2EVCFG_EV0_GEN_BITN                                     4
#define AUX_TIMER2_CH2EVCFG_EV0_GEN_M                               0x00000010
#define AUX_TIMER2_CH2EVCFG_EV0_GEN_S                                        4

// Field:   [3:0] CCACT
//
// Capture-Compare action.
//
// Capture-Compare action defines 15 different channel functions that utilize
// capture, compare, and zero events.
// ENUMs:
// PULSE_ON_CMP             Pulse on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP               Toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
// SET_ON_CMP               Set on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
// CLR_ON_CMP               Clear on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
// SET_ON_0_TGL_ON_CMP      Set on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UP_PER
//                          for edge-aligned PWM generation. Duty cycle is
//                          given by:
//
//                          When CH2CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle =
//                          CH2CC.VALUE / ( TARGET.VALUE + 1 ).
//
//                          When CH2CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 1.
//
//                          Enabled events are
//                          cleared when CH2CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP      Clear on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UPDWN_PER
//                          for center-aligned PWM generation. Duty cycle
//                          is given by:
//
//                          When CH2CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle = 1 - (
//                          CH2CC.VALUE / TARGET.VALUE ).
//
//                          When CH2CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 0.
//
//                          Enabled events are set
//                          when CH2CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT              Set on capture repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH2CC.VALUE.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Select this function
//                          with no event enable.
//                           - Configure CH2CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          enable events.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// PER_PULSE_WIDTH_MEAS     Period and pulse width measurement.
//
//                          Continuously capture
//                          period and pulse width of the signal selected
//                          by CH2CCFG.CAPT_SRC relative to the signal edge
//                          given by CH2CCFG.EDGE.
//
//                          Set enabled events when
//                          CH2CC.VALUE contains signal period and
//                          CH2PCC.VALUE contains signal pulse width.
//
//                          Notes:
//                          - Make sure that you
//                          configure CH2CCFG.CAPT_SRC and CCACT when
//                          CTL.MODE equals DIS, then set CTL.MODE to
//                          UP_ONCE or UP_PER.
//                          - The counter restarts in
//                          the selected timer mode when CH2CC.VALUE
//                          contains the signal period.
//                          - If more than one
//                          channel uses this function, the channels will
//                          perform this function one at a time. The
//                          channel with lowest number has priority and
//                          performs the function first. Next measurement
//                          starts when current measurement completes
//                          successfully or times out. A timeout occurs
//                          when counter equals target.
//                          - If you want to observe
//                          a timeout event configure another channel to
//                          SET_ON_CAPT.
//
//                          Signal property
//                          requirements:
//                          - Signal Period >= 2 * (
//                          1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal Period <= 65535
//                          * (1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal low and high
//                          phase >= (1 + PRECFG.CLKDIV ) * timer clock
//                          period.
// PULSE_ON_CMP_DIS         Pulse on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP_DIS           Toggle on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_CMP_DIS           Set on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CH2CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// CLR_ON_CMP_DIS           Clear on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_0_TGL_ON_CMP_DIS  Set on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are
//                          cleared when CH2CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP_DIS  Clear on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH2CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are set
//                          when CH2CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT_DIS          Set on capture, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH2CC.VALUE.
//                          - Disable channel.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Set to SET_ON_CAPT
//                          with no event enable.
//                           - Configure CH2CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          set to SET_ON_CAPT_DIS. Event enable is
//                          optional.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// DIS                      Disable channel.
#define AUX_TIMER2_CH2EVCFG_CCACT_W                                          4
#define AUX_TIMER2_CH2EVCFG_CCACT_M                                 0x0000000F
#define AUX_TIMER2_CH2EVCFG_CCACT_S                                          0
#define AUX_TIMER2_CH2EVCFG_CCACT_PULSE_ON_CMP                      0x0000000F
#define AUX_TIMER2_CH2EVCFG_CCACT_TGL_ON_CMP                        0x0000000E
#define AUX_TIMER2_CH2EVCFG_CCACT_SET_ON_CMP                        0x0000000D
#define AUX_TIMER2_CH2EVCFG_CCACT_CLR_ON_CMP                        0x0000000C
#define AUX_TIMER2_CH2EVCFG_CCACT_SET_ON_0_TGL_ON_CMP               0x0000000B
#define AUX_TIMER2_CH2EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP               0x0000000A
#define AUX_TIMER2_CH2EVCFG_CCACT_SET_ON_CAPT                       0x00000009
#define AUX_TIMER2_CH2EVCFG_CCACT_PER_PULSE_WIDTH_MEAS              0x00000008
#define AUX_TIMER2_CH2EVCFG_CCACT_PULSE_ON_CMP_DIS                  0x00000007
#define AUX_TIMER2_CH2EVCFG_CCACT_TGL_ON_CMP_DIS                    0x00000006
#define AUX_TIMER2_CH2EVCFG_CCACT_SET_ON_CMP_DIS                    0x00000005
#define AUX_TIMER2_CH2EVCFG_CCACT_CLR_ON_CMP_DIS                    0x00000004
#define AUX_TIMER2_CH2EVCFG_CCACT_SET_ON_0_TGL_ON_CMP_DIS           0x00000003
#define AUX_TIMER2_CH2EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP_DIS           0x00000002
#define AUX_TIMER2_CH2EVCFG_CCACT_SET_ON_CAPT_DIS                   0x00000001
#define AUX_TIMER2_CH2EVCFG_CCACT_DIS                               0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH2CCFG
//
//*****************************************************************************
// Field:   [6:1] CAPT_SRC
//
// Select capture signal source from the asynchronous AUX event bus.
//
// The selected signal enters the edge-detection circuit. False capture events
// can occur when:
// - the edge-detection circuit contains expired signal samples and the circuit
// is enabled without flush as described in CH2EVCFG
// - this register is reconfigured while CTL.MODE is different from DIS.
//
// You can avoid false capture events. When wanted channel function is:
// - SET_ON_CAPT_DIS, see description for SET_ON_CAPT_DIS in CH2EVCFG.CCACT.
// - SET_ON_CAPT, see description for SET_ON_CAPT in CH2EVCFG.CCACT.
// - PER_PULSE_WIDTH_MEAS, see description for PER_PULSE_WIDTH_MEAS in
// CH2EVCFG.CCACT.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
// ENUMs:
// NO_EVENT                 No event.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
// AUX_TIMER1_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_EV3           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                AUX_EVCTL:EVSTAT2.AUX_COMPB
// AUX_COMPA                AUX_EVCTL:EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              AUX_EVCTL:EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              AUX_EVCTL:EVSTAT2.MCU_OBSMUX0
// MCU_EV                   AUX_EVCTL:EVSTAT2.MCU_EV
// ACLK_REF                 AUX_EVCTL:EVSTAT2.ACLK_REF
// VDDR_RECHARGE            AUX_EVCTL:EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               AUX_EVCTL:EVSTAT2.MCU_ACTIVE
// PWR_DWN                  AUX_EVCTL:EVSTAT2.PWR_DWN
// SCLK_LF                  AUX_EVCTL:EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      AUX_EVCTL:EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       AUX_EVCTL:EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             AUX_EVCTL:EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              AUX_EVCTL:EVSTAT2.AON_RTC_CH2
// MANUAL_EV                AUX_EVCTL:EVSTAT2.MANUAL_EV
// AUXIO31                  AUX_EVCTL:EVSTAT1.AUXIO31
// AUXIO30                  AUX_EVCTL:EVSTAT1.AUXIO30
// AUXIO29                  AUX_EVCTL:EVSTAT1.AUXIO29
// AUXIO28                  AUX_EVCTL:EVSTAT1.AUXIO28
// AUXIO27                  AUX_EVCTL:EVSTAT1.AUXIO27
// AUXIO26                  AUX_EVCTL:EVSTAT1.AUXIO26
// AUXIO25                  AUX_EVCTL:EVSTAT1.AUXIO25
// AUXIO24                  AUX_EVCTL:EVSTAT1.AUXIO24
// AUXIO23                  AUX_EVCTL:EVSTAT1.AUXIO23
// AUXIO22                  AUX_EVCTL:EVSTAT1.AUXIO22
// AUXIO21                  AUX_EVCTL:EVSTAT1.AUXIO21
// AUXIO20                  AUX_EVCTL:EVSTAT1.AUXIO20
// AUXIO19                  AUX_EVCTL:EVSTAT1.AUXIO19
// AUXIO18                  AUX_EVCTL:EVSTAT1.AUXIO18
// AUXIO17                  AUX_EVCTL:EVSTAT1.AUXIO17
// AUXIO16                  AUX_EVCTL:EVSTAT1.AUXIO16
// AUXIO15                  AUX_EVCTL:EVSTAT0.AUXIO15
// AUXIO14                  AUX_EVCTL:EVSTAT0.AUXIO14
// AUXIO13                  AUX_EVCTL:EVSTAT0.AUXIO13
// AUXIO12                  AUX_EVCTL:EVSTAT0.AUXIO12
// AUXIO11                  AUX_EVCTL:EVSTAT0.AUXIO11
// AUXIO10                  AUX_EVCTL:EVSTAT0.AUXIO10
// AUXIO9                   AUX_EVCTL:EVSTAT0.AUXIO9
// AUXIO8                   AUX_EVCTL:EVSTAT0.AUXIO8
// AUXIO7                   AUX_EVCTL:EVSTAT0.AUXIO7
// AUXIO6                   AUX_EVCTL:EVSTAT0.AUXIO6
// AUXIO5                   AUX_EVCTL:EVSTAT0.AUXIO5
// AUXIO4                   AUX_EVCTL:EVSTAT0.AUXIO4
// AUXIO3                   AUX_EVCTL:EVSTAT0.AUXIO3
// AUXIO2                   AUX_EVCTL:EVSTAT0.AUXIO2
// AUXIO1                   AUX_EVCTL:EVSTAT0.AUXIO1
// AUXIO0                   AUX_EVCTL:EVSTAT0.AUXIO0
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_W                                        6
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_M                               0x0000007E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_S                                        1
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_NO_EVENT                        0x0000007E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000007A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x00000078
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x00000076
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_ADC_IRQ                     0x00000074
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_ADC_DONE                    0x00000072
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_ISRC_RESET_N                0x00000070
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_TDC_DONE                    0x0000006E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_TIMER0_EV                   0x0000006C
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_TIMER1_EV                   0x0000006A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_TIMER2_EV3                  0x00000066
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_TIMER2_EV2                  0x00000064
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_TIMER2_EV1                  0x00000062
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_TIMER2_EV0                  0x00000060
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_COMPB                       0x0000005E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUX_COMPA                       0x0000005C
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_MCU_OBSMUX1                     0x0000005A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_MCU_OBSMUX0                     0x00000058
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_MCU_EV                          0x00000056
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_ACLK_REF                        0x00000054
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_VDDR_RECHARGE                   0x00000052
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_MCU_ACTIVE                      0x00000050
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_PWR_DWN                         0x0000004E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_SCLK_LF                         0x0000004C
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AON_BATMON_TEMP_UPD             0x0000004A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AON_BATMON_BAT_UPD              0x00000048
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AON_RTC_4KHZ                    0x00000046
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AON_RTC_CH2_DLY                 0x00000044
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AON_RTC_CH2                     0x00000042
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_MANUAL_EV                       0x00000040
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO31                         0x0000003E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO30                         0x0000003C
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO29                         0x0000003A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO28                         0x00000038
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO27                         0x00000036
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO26                         0x00000034
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO25                         0x00000032
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO24                         0x00000030
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO23                         0x0000002E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO22                         0x0000002C
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO21                         0x0000002A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO20                         0x00000028
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO19                         0x00000026
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO18                         0x00000024
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO17                         0x00000022
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO16                         0x00000020
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO15                         0x0000001E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO14                         0x0000001C
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO13                         0x0000001A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO12                         0x00000018
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO11                         0x00000016
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO10                         0x00000014
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO9                          0x00000012
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO8                          0x00000010
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO7                          0x0000000E
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO6                          0x0000000C
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO5                          0x0000000A
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO4                          0x00000008
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO3                          0x00000006
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO2                          0x00000004
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO1                          0x00000002
#define AUX_TIMER2_CH2CCFG_CAPT_SRC_AUXIO0                          0x00000000

// Field:     [0] EDGE
//
// Edge configuration.
//
// Channel captures counter value at selected edge on signal source selected by
// CAPT_SRC. See CH2EVCFG.CCACT.
// ENUMs:
// RISING                   Capture CNTR.VALUE at rising edge of CAPT_SRC.
// FALLING                  Capture CNTR.VALUE at falling edge of CAPT_SRC.
#define AUX_TIMER2_CH2CCFG_EDGE                                     0x00000001
#define AUX_TIMER2_CH2CCFG_EDGE_BITN                                         0
#define AUX_TIMER2_CH2CCFG_EDGE_M                                   0x00000001
#define AUX_TIMER2_CH2CCFG_EDGE_S                                            0
#define AUX_TIMER2_CH2CCFG_EDGE_RISING                              0x00000001
#define AUX_TIMER2_CH2CCFG_EDGE_FALLING                             0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH2PCC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Pipeline Capture Compare value.
//
// 16-bit user defined pipeline compare value or channel-updated capture value.
//
// Compare mode:
// An update of VALUE will be transferred to CH2CC.VALUE when the next
// CNTR.VALUE is zero and CTL.MODE is different from DIS. This is useful for
// PWM generation and prevents jitter on the edges of the generated signal.
//
// Capture mode:
// When CH2EVCFG.CCACT equals PER_PULSE_WIDTH_MEAS then VALUE contains the
// width of the low or high phase of the selected signal. This is specified by
// CH2CCFG.EDGE and CH2CCFG.CAPT_SRC.
#define AUX_TIMER2_CH2PCC_VALUE_W                                           16
#define AUX_TIMER2_CH2PCC_VALUE_M                                   0x0000FFFF
#define AUX_TIMER2_CH2PCC_VALUE_S                                            0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH2CC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Capture Compare value.
//
// 16-bit user defined compare value or channel-updated capture value.
//
// Compare mode:
// VALUE is compared against CNTR.VALUE and an event is generated as specified
// by CH2EVCFG.CCACT when these are equal.
//
// Capture mode:
// The current counter value is stored in VALUE when a capture event occurs.
// CH2EVCFG.CCACT determines if VALUE is a signal period or a regular capture
// value.
#define AUX_TIMER2_CH2CC_VALUE_W                                            16
#define AUX_TIMER2_CH2CC_VALUE_M                                    0x0000FFFF
#define AUX_TIMER2_CH2CC_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH3EVCFG
//
//*****************************************************************************
// Field:     [7] EV3_GEN
//
// Event 3 enable.
//
// 0: Channel 3 does not control event 3.
// 1: Channel 3 controls event 3.
//
//  When 0 < CCACT < 8, EV3_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH3EVCFG_EV3_GEN                                 0x00000080
#define AUX_TIMER2_CH3EVCFG_EV3_GEN_BITN                                     7
#define AUX_TIMER2_CH3EVCFG_EV3_GEN_M                               0x00000080
#define AUX_TIMER2_CH3EVCFG_EV3_GEN_S                                        7

// Field:     [6] EV2_GEN
//
// Event 2 enable.
//
// 0: Channel 3 does not control event 2.
// 1: Channel 3 controls event 2.
//
//  When 0 < CCACT < 8, EV2_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH3EVCFG_EV2_GEN                                 0x00000040
#define AUX_TIMER2_CH3EVCFG_EV2_GEN_BITN                                     6
#define AUX_TIMER2_CH3EVCFG_EV2_GEN_M                               0x00000040
#define AUX_TIMER2_CH3EVCFG_EV2_GEN_S                                        6

// Field:     [5] EV1_GEN
//
// Event 1 enable.
//
// 0: Channel 3 does not control event 1.
// 1: Channel 3 controls event 1.
//
//  When 0 < CCACT < 8, EV1_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH3EVCFG_EV1_GEN                                 0x00000020
#define AUX_TIMER2_CH3EVCFG_EV1_GEN_BITN                                     5
#define AUX_TIMER2_CH3EVCFG_EV1_GEN_M                               0x00000020
#define AUX_TIMER2_CH3EVCFG_EV1_GEN_S                                        5

// Field:     [4] EV0_GEN
//
// Event 0 enable.
//
// 0: Channel 3 does not control event 0.
// 1: Channel 3 controls event 0.
//
//  When 0 < CCACT < 8, EV0_GEN becomes zero after a capture or compare event.
#define AUX_TIMER2_CH3EVCFG_EV0_GEN                                 0x00000010
#define AUX_TIMER2_CH3EVCFG_EV0_GEN_BITN                                     4
#define AUX_TIMER2_CH3EVCFG_EV0_GEN_M                               0x00000010
#define AUX_TIMER2_CH3EVCFG_EV0_GEN_S                                        4

// Field:   [3:0] CCACT
//
// Capture-Compare action.
//
// Capture-Compare action defines 15 different channel functions that utilize
// capture, compare, and zero events.
// ENUMs:
// PULSE_ON_CMP             Pulse on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP               Toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
// SET_ON_CMP               Set on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
// CLR_ON_CMP               Clear on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
// SET_ON_0_TGL_ON_CMP      Set on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UP_PER
//                          for edge-aligned PWM generation. Duty cycle is
//                          given by:
//
//                          When CH3CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle =
//                          CH3CC.VALUE / ( TARGET.VALUE + 1 ).
//
//                          When CH3CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 1.
//
//                          Enabled events are
//                          cleared when CH3CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP      Clear on zero, toggle on compare repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//
//                          Set CTL.MODE to UPDWN_PER
//                          for center-aligned PWM generation. Duty cycle
//                          is given by:
//
//                          When CH3CC.VALUE <=
//                          TARGET.VALUE:
//                             Duty cycle = 1 - (
//                          CH3CC.VALUE / TARGET.VALUE ).
//
//                          When CH3CC.VALUE >
//                          TARGET.VALUE:
//                             Duty cycle = 0.
//
//                          Enabled events are set
//                          when CH3CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT              Set on capture repeatedly.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH3CC.VALUE.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Select this function
//                          with no event enable.
//                           - Configure CH3CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          enable events.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// PER_PULSE_WIDTH_MEAS     Period and pulse width measurement.
//
//                          Continuously capture
//                          period and pulse width of the signal selected
//                          by CH3CCFG.CAPT_SRC relative to the signal edge
//                          given by CH3CCFG.EDGE.
//
//                          Set enabled events when
//                          CH3CC.VALUE contains signal period and
//                          CH3PCC.VALUE contains signal pulse width.
//
//                          Notes:
//                          - Make sure that you
//                          configure CH3CCFG.CAPT_SRC and CCACT when
//                          CTL.MODE equals DIS, then set CTL.MODE to
//                          UP_ONCE or UP_PER.
//                          - The counter restarts in
//                          the selected timer mode when CH3CC.VALUE
//                          contains the signal period.
//                          - If more than one
//                          channel uses this function, the channels will
//                          perform this function one at a time. The
//                          channel with lowest number has priority and
//                          performs the function first. Next measurement
//                          starts when current measurement completes
//                          successfully or times out. A timeout occurs
//                          when counter equals target.
//                          - If you want to observe
//                          a timeout event configure another channel to
//                          SET_ON_CAPT.
//
//                          Signal property
//                          requirements:
//                          - Signal Period >= 2 * (
//                          1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal Period <= 65535
//                          * (1 + PRECFG.CLKDIV ) * timer clock period.
//                          - Signal low and high
//                          phase >= (1 + PRECFG.CLKDIV ) * timer clock
//                          period.
// PULSE_ON_CMP_DIS         Pulse on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Pulse enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                           The event is high for
//                          two timer clock periods.
// TGL_ON_CMP_DIS           Toggle on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Toggle enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_CMP_DIS           Set on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CH3CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// CLR_ON_CMP_DIS           Clear on compare, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
// SET_ON_0_TGL_ON_CMP_DIS  Set on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events when
//                          CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are
//                          cleared when CH3CC.VALUE = 0 and CNTR.VALUE =
//                          0.
// CLR_ON_0_TGL_ON_CMP_DIS  Clear on zero, toggle on compare, and then disable
//                          channel.
//
//                          Channel function
//                          sequence:
//                          - Clear enabled events
//                          when CNTR.VALUE = 0.
//                          - Toggle enabled events
//                          when CH3CC.VALUE = CNTR.VALUE.
//                          - Disable channel.
//
//                          Enabled events are set
//                          when CH3CC.VALUE = 0 and CNTR.VALUE = 0.
// SET_ON_CAPT_DIS          Set on capture, and then disable channel.
//
//                          Channel function
//                          sequence:
//                          - Set enabled events on
//                          capture event and copy CNTR.VALUE to
//                          CH3CC.VALUE.
//                          - Disable channel.
//
//                          Primary use scenario is
//                          to select this function before you start the
//                          timer.
//                          Follow these steps if you
//                          need to select this function while CTL.MODE is
//                          different from DIS:
//                           - Set CCACT to
//                          SET_ON_CAPT with no event enable.
//                           - Configure CH3CCFG
//                          (optional).
//                           - Wait for three timer
//                          clock periods as defined in PRECFG before you
//                          set CCACT to SET_ON_CAPT_DIS.  Event enable is
//                          optional.
//
//                          These steps prevent
//                          capture events caused by expired signal values
//                          in edge-detection circuit.
// DIS                      Disable channel.
#define AUX_TIMER2_CH3EVCFG_CCACT_W                                          4
#define AUX_TIMER2_CH3EVCFG_CCACT_M                                 0x0000000F
#define AUX_TIMER2_CH3EVCFG_CCACT_S                                          0
#define AUX_TIMER2_CH3EVCFG_CCACT_PULSE_ON_CMP                      0x0000000F
#define AUX_TIMER2_CH3EVCFG_CCACT_TGL_ON_CMP                        0x0000000E
#define AUX_TIMER2_CH3EVCFG_CCACT_SET_ON_CMP                        0x0000000D
#define AUX_TIMER2_CH3EVCFG_CCACT_CLR_ON_CMP                        0x0000000C
#define AUX_TIMER2_CH3EVCFG_CCACT_SET_ON_0_TGL_ON_CMP               0x0000000B
#define AUX_TIMER2_CH3EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP               0x0000000A
#define AUX_TIMER2_CH3EVCFG_CCACT_SET_ON_CAPT                       0x00000009
#define AUX_TIMER2_CH3EVCFG_CCACT_PER_PULSE_WIDTH_MEAS              0x00000008
#define AUX_TIMER2_CH3EVCFG_CCACT_PULSE_ON_CMP_DIS                  0x00000007
#define AUX_TIMER2_CH3EVCFG_CCACT_TGL_ON_CMP_DIS                    0x00000006
#define AUX_TIMER2_CH3EVCFG_CCACT_SET_ON_CMP_DIS                    0x00000005
#define AUX_TIMER2_CH3EVCFG_CCACT_CLR_ON_CMP_DIS                    0x00000004
#define AUX_TIMER2_CH3EVCFG_CCACT_SET_ON_0_TGL_ON_CMP_DIS           0x00000003
#define AUX_TIMER2_CH3EVCFG_CCACT_CLR_ON_0_TGL_ON_CMP_DIS           0x00000002
#define AUX_TIMER2_CH3EVCFG_CCACT_SET_ON_CAPT_DIS                   0x00000001
#define AUX_TIMER2_CH3EVCFG_CCACT_DIS                               0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH3CCFG
//
//*****************************************************************************
// Field:   [6:1] CAPT_SRC
//
// Select capture signal source from the asynchronous AUX event bus.
//
// The selected signal enters the edge-detection circuit. False capture events
// can occur when:
// - the edge-detection circuit contains expired signal samples and the circuit
// is enabled without flush as described in CH3EVCFG
// - this register is reconfigured while CTL.MODE is different from DIS.
//
// You can avoid false capture events. When wanted channel function:
// - SET_ON_CAPT_DIS, see description for SET_ON_CAPT_DIS in CH3EVCFG.CCACT.
// - SET_ON_CAPT, see description for SET_ON_CAPT in CH3EVCFG.CCACT.
// - PER_PULSE_WIDTH_MEAS, see description for PER_PULSE_WIDTH_MEAS in
// CH3EVCFG.CCACT.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
// ENUMs:
// NO_EVENT                 No event.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
// AUX_TIMER1_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_EV3           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                AUX_EVCTL:EVSTAT2.AUX_COMPB
// AUX_COMPA                AUX_EVCTL:EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              AUX_EVCTL:EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              AUX_EVCTL:EVSTAT2.MCU_OBSMUX0
// MCU_EV                   AUX_EVCTL:EVSTAT2.MCU_EV
// ACLK_REF                 AUX_EVCTL:EVSTAT2.ACLK_REF
// VDDR_RECHARGE            AUX_EVCTL:EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               AUX_EVCTL:EVSTAT2.MCU_ACTIVE
// PWR_DWN                  AUX_EVCTL:EVSTAT2.PWR_DWN
// SCLK_LF                  AUX_EVCTL:EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      AUX_EVCTL:EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       AUX_EVCTL:EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             AUX_EVCTL:EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              AUX_EVCTL:EVSTAT2.AON_RTC_CH2
// MANUAL_EV                AUX_EVCTL:EVSTAT2.MANUAL_EV
// AUXIO31                  AUX_EVCTL:EVSTAT1.AUXIO31
// AUXIO30                  AUX_EVCTL:EVSTAT1.AUXIO30
// AUXIO29                  AUX_EVCTL:EVSTAT1.AUXIO29
// AUXIO28                  AUX_EVCTL:EVSTAT1.AUXIO28
// AUXIO27                  AUX_EVCTL:EVSTAT1.AUXIO27
// AUXIO26                  AUX_EVCTL:EVSTAT1.AUXIO26
// AUXIO25                  AUX_EVCTL:EVSTAT1.AUXIO25
// AUXIO24                  AUX_EVCTL:EVSTAT1.AUXIO24
// AUXIO23                  AUX_EVCTL:EVSTAT1.AUXIO23
// AUXIO22                  AUX_EVCTL:EVSTAT1.AUXIO22
// AUXIO21                  AUX_EVCTL:EVSTAT1.AUXIO21
// AUXIO20                  AUX_EVCTL:EVSTAT1.AUXIO20
// AUXIO19                  AUX_EVCTL:EVSTAT1.AUXIO19
// AUXIO18                  AUX_EVCTL:EVSTAT1.AUXIO18
// AUXIO17                  AUX_EVCTL:EVSTAT1.AUXIO17
// AUXIO16                  AUX_EVCTL:EVSTAT1.AUXIO16
// AUXIO15                  AUX_EVCTL:EVSTAT0.AUXIO15
// AUXIO14                  AUX_EVCTL:EVSTAT0.AUXIO14
// AUXIO13                  AUX_EVCTL:EVSTAT0.AUXIO13
// AUXIO12                  AUX_EVCTL:EVSTAT0.AUXIO12
// AUXIO11                  AUX_EVCTL:EVSTAT0.AUXIO11
// AUXIO10                  AUX_EVCTL:EVSTAT0.AUXIO10
// AUXIO9                   AUX_EVCTL:EVSTAT0.AUXIO9
// AUXIO8                   AUX_EVCTL:EVSTAT0.AUXIO8
// AUXIO7                   AUX_EVCTL:EVSTAT0.AUXIO7
// AUXIO6                   AUX_EVCTL:EVSTAT0.AUXIO6
// AUXIO5                   AUX_EVCTL:EVSTAT0.AUXIO5
// AUXIO4                   AUX_EVCTL:EVSTAT0.AUXIO4
// AUXIO3                   AUX_EVCTL:EVSTAT0.AUXIO3
// AUXIO2                   AUX_EVCTL:EVSTAT0.AUXIO2
// AUXIO1                   AUX_EVCTL:EVSTAT0.AUXIO1
// AUXIO0                   AUX_EVCTL:EVSTAT0.AUXIO0
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_W                                        6
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_M                               0x0000007E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_S                                        1
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_NO_EVENT                        0x0000007E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000007A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x00000078
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x00000076
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_ADC_IRQ                     0x00000074
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_ADC_DONE                    0x00000072
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_ISRC_RESET_N                0x00000070
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_TDC_DONE                    0x0000006E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_TIMER0_EV                   0x0000006C
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_TIMER1_EV                   0x0000006A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_TIMER2_EV3                  0x00000066
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_TIMER2_EV2                  0x00000064
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_TIMER2_EV1                  0x00000062
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_TIMER2_EV0                  0x00000060
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_COMPB                       0x0000005E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUX_COMPA                       0x0000005C
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_MCU_OBSMUX1                     0x0000005A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_MCU_OBSMUX0                     0x00000058
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_MCU_EV                          0x00000056
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_ACLK_REF                        0x00000054
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_VDDR_RECHARGE                   0x00000052
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_MCU_ACTIVE                      0x00000050
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_PWR_DWN                         0x0000004E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_SCLK_LF                         0x0000004C
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AON_BATMON_TEMP_UPD             0x0000004A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AON_BATMON_BAT_UPD              0x00000048
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AON_RTC_4KHZ                    0x00000046
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AON_RTC_CH2_DLY                 0x00000044
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AON_RTC_CH2                     0x00000042
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_MANUAL_EV                       0x00000040
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO31                         0x0000003E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO30                         0x0000003C
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO29                         0x0000003A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO28                         0x00000038
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO27                         0x00000036
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO26                         0x00000034
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO25                         0x00000032
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO24                         0x00000030
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO23                         0x0000002E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO22                         0x0000002C
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO21                         0x0000002A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO20                         0x00000028
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO19                         0x00000026
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO18                         0x00000024
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO17                         0x00000022
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO16                         0x00000020
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO15                         0x0000001E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO14                         0x0000001C
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO13                         0x0000001A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO12                         0x00000018
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO11                         0x00000016
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO10                         0x00000014
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO9                          0x00000012
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO8                          0x00000010
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO7                          0x0000000E
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO6                          0x0000000C
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO5                          0x0000000A
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO4                          0x00000008
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO3                          0x00000006
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO2                          0x00000004
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO1                          0x00000002
#define AUX_TIMER2_CH3CCFG_CAPT_SRC_AUXIO0                          0x00000000

// Field:     [0] EDGE
//
// Edge configuration.
//
// Channel captures counter value at selected edge on signal source selected by
// CAPT_SRC. See CH3EVCFG.CCACT.
// ENUMs:
// RISING                   Capture CNTR.VALUE at rising edge of CAPT_SRC.
// FALLING                  Capture CNTR.VALUE at falling edge of CAPT_SRC.
#define AUX_TIMER2_CH3CCFG_EDGE                                     0x00000001
#define AUX_TIMER2_CH3CCFG_EDGE_BITN                                         0
#define AUX_TIMER2_CH3CCFG_EDGE_M                                   0x00000001
#define AUX_TIMER2_CH3CCFG_EDGE_S                                            0
#define AUX_TIMER2_CH3CCFG_EDGE_RISING                              0x00000001
#define AUX_TIMER2_CH3CCFG_EDGE_FALLING                             0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH3PCC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Pipeline Capture Compare value.
//
// 16-bit user defined pipeline compare value or channel-updated capture value.
//
// Compare mode:
// An update of VALUE will be transferred to CH3CC.VALUE when the next
// CNTR.VALUE is zero and CTL.MODE is different from DIS. This is useful for
// PWM generation and prevents jitter on the edges of the generated signal.
//
// Capture mode:
// When CH3EVCFG.CCACT equals PER_PULSE_WIDTH_MEAS then VALUE contains the
// width of the low or high phase of the selected signal. This is specified by
// CH3CCFG.EDGE and CH3CCFG.CAPT_SRC.
#define AUX_TIMER2_CH3PCC_VALUE_W                                           16
#define AUX_TIMER2_CH3PCC_VALUE_M                                   0x0000FFFF
#define AUX_TIMER2_CH3PCC_VALUE_S                                            0

//*****************************************************************************
//
// Register: AUX_TIMER2_O_CH3CC
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Capture Compare value.
//
// 16-bit user defined compare value or channel-updated capture value.
//
// Compare mode:
// VALUE is compared against CNTR.VALUE and an event is generated as specified
// by CH3EVCFG.CCACT when these are equal.
//
// Capture mode:
// The current counter value is stored in VALUE when a capture event occurs.
// CH3EVCFG.CCACT determines if VALUE is a signal period or a regular capture
// value.
#define AUX_TIMER2_CH3CC_VALUE_W                                            16
#define AUX_TIMER2_CH3CC_VALUE_M                                    0x0000FFFF
#define AUX_TIMER2_CH3CC_VALUE_S                                             0


#endif // __AUX_TIMER2__
