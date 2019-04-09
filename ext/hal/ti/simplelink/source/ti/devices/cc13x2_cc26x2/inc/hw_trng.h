/******************************************************************************
*  Filename:       hw_trng_h
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

#ifndef __HW_TRNG_H__
#define __HW_TRNG_H__

//*****************************************************************************
//
// This section defines the register offsets of
// TRNG component
//
//*****************************************************************************
// Random Number Lower Word Readout Value
#define TRNG_O_OUT0                                                 0x00000000

// Random Number Upper Word Readout Value
#define TRNG_O_OUT1                                                 0x00000004

// Interrupt Status
#define TRNG_O_IRQFLAGSTAT                                          0x00000008

// Interrupt Mask
#define TRNG_O_IRQFLAGMASK                                          0x0000000C

// Interrupt Flag Clear
#define TRNG_O_IRQFLAGCLR                                           0x00000010

// Control
#define TRNG_O_CTL                                                  0x00000014

// Configuration 0
#define TRNG_O_CFG0                                                 0x00000018

// Alarm Control
#define TRNG_O_ALARMCNT                                             0x0000001C

// FRO Enable
#define TRNG_O_FROEN                                                0x00000020

// FRO De-tune Bit
#define TRNG_O_FRODETUNE                                            0x00000024

// Alarm Event
#define TRNG_O_ALARMMASK                                            0x00000028

// Alarm Shutdown
#define TRNG_O_ALARMSTOP                                            0x0000002C

// LFSR Readout Value
#define TRNG_O_LFSR0                                                0x00000030

// LFSR Readout Value
#define TRNG_O_LFSR1                                                0x00000034

// LFSR Readout Value
#define TRNG_O_LFSR2                                                0x00000038

// TRNG Engine Options Information
#define TRNG_O_HWOPT                                                0x00000078

// HW Version 0
#define TRNG_O_HWVER0                                               0x0000007C

// Interrupt Status After Masking
#define TRNG_O_IRQSTATMASK                                          0x00001FD8

// HW Version 1
#define TRNG_O_HWVER1                                               0x00001FE0

// Interrupt Set
#define TRNG_O_IRQSET                                               0x00001FEC

// SW Reset Control
#define TRNG_O_SWRESET                                              0x00001FF0

// Interrupt Status
#define TRNG_O_IRQSTAT                                              0x00001FF8

//*****************************************************************************
//
// Register: TRNG_O_OUT0
//
//*****************************************************************************
// Field:  [31:0] VALUE_31_0
//
// LSW of 64- bit random value. New value ready when IRQFLAGSTAT.RDY = 1.
#define TRNG_OUT0_VALUE_31_0_W                                              32
#define TRNG_OUT0_VALUE_31_0_M                                      0xFFFFFFFF
#define TRNG_OUT0_VALUE_31_0_S                                               0

//*****************************************************************************
//
// Register: TRNG_O_OUT1
//
//*****************************************************************************
// Field:  [31:0] VALUE_63_32
//
// MSW of 64-bit random value. New value ready when IRQFLAGSTAT.RDY = 1.
#define TRNG_OUT1_VALUE_63_32_W                                             32
#define TRNG_OUT1_VALUE_63_32_M                                     0xFFFFFFFF
#define TRNG_OUT1_VALUE_63_32_S                                              0

//*****************************************************************************
//
// Register: TRNG_O_IRQFLAGSTAT
//
//*****************************************************************************
// Field:    [31] NEED_CLOCK
//
// 1: Indicates that the TRNG is busy generating entropy or is in one of its
// test modes - clocks may not be turned off and the power supply voltage must
// be kept stable.
// 0: TRNG is idle and can be shut down
#define TRNG_IRQFLAGSTAT_NEED_CLOCK                                 0x80000000
#define TRNG_IRQFLAGSTAT_NEED_CLOCK_BITN                                    31
#define TRNG_IRQFLAGSTAT_NEED_CLOCK_M                               0x80000000
#define TRNG_IRQFLAGSTAT_NEED_CLOCK_S                                       31

// Field:     [1] SHUTDOWN_OVF
//
// 1: The number of FROs shut down (i.e. the number of '1' bits in the
// ALARMSTOP register) has exceeded the threshold set by ALARMCNT.SHUTDOWN_THR
//
// Writing '1' to IRQFLAGCLR.SHUTDOWN_OVF clears this bit to '0' again.
#define TRNG_IRQFLAGSTAT_SHUTDOWN_OVF                               0x00000002
#define TRNG_IRQFLAGSTAT_SHUTDOWN_OVF_BITN                                   1
#define TRNG_IRQFLAGSTAT_SHUTDOWN_OVF_M                             0x00000002
#define TRNG_IRQFLAGSTAT_SHUTDOWN_OVF_S                                      1

// Field:     [0] RDY
//
// 1: Data are available in OUT0 and OUT1.
//
// Acknowledging this state by writing '1' to IRQFLAGCLR.RDY clears this bit to
// '0'.
// If a new number is already available in the internal register of the TRNG,
// the number is directly clocked into the result register. In this case the
// status bit is asserted again, after one clock cycle.
#define TRNG_IRQFLAGSTAT_RDY                                        0x00000001
#define TRNG_IRQFLAGSTAT_RDY_BITN                                            0
#define TRNG_IRQFLAGSTAT_RDY_M                                      0x00000001
#define TRNG_IRQFLAGSTAT_RDY_S                                               0

//*****************************************************************************
//
// Register: TRNG_O_IRQFLAGMASK
//
//*****************************************************************************
// Field:     [1] SHUTDOWN_OVF
//
// 1: Allow IRQFLAGSTAT.SHUTDOWN_OVF to activate the interrupt from this
// module.
#define TRNG_IRQFLAGMASK_SHUTDOWN_OVF                               0x00000002
#define TRNG_IRQFLAGMASK_SHUTDOWN_OVF_BITN                                   1
#define TRNG_IRQFLAGMASK_SHUTDOWN_OVF_M                             0x00000002
#define TRNG_IRQFLAGMASK_SHUTDOWN_OVF_S                                      1

// Field:     [0] RDY
//
// 1: Allow IRQFLAGSTAT.RDY to activate the interrupt from this module.
#define TRNG_IRQFLAGMASK_RDY                                        0x00000001
#define TRNG_IRQFLAGMASK_RDY_BITN                                            0
#define TRNG_IRQFLAGMASK_RDY_M                                      0x00000001
#define TRNG_IRQFLAGMASK_RDY_S                                               0

//*****************************************************************************
//
// Register: TRNG_O_IRQFLAGCLR
//
//*****************************************************************************
// Field:     [1] SHUTDOWN_OVF
//
// 1: Clear IRQFLAGSTAT.SHUTDOWN_OVF.
#define TRNG_IRQFLAGCLR_SHUTDOWN_OVF                                0x00000002
#define TRNG_IRQFLAGCLR_SHUTDOWN_OVF_BITN                                    1
#define TRNG_IRQFLAGCLR_SHUTDOWN_OVF_M                              0x00000002
#define TRNG_IRQFLAGCLR_SHUTDOWN_OVF_S                                       1

// Field:     [0] RDY
//
// 1: Clear IRQFLAGSTAT.RDY.
#define TRNG_IRQFLAGCLR_RDY                                         0x00000001
#define TRNG_IRQFLAGCLR_RDY_BITN                                             0
#define TRNG_IRQFLAGCLR_RDY_M                                       0x00000001
#define TRNG_IRQFLAGCLR_RDY_S                                                0

//*****************************************************************************
//
// Register: TRNG_O_CTL
//
//*****************************************************************************
// Field: [31:16] STARTUP_CYCLES
//
// This field determines the number of samples (between 2^8 and 2^24) taken to
// gather entropy from the FROs during startup. If the written value of this
// field is zero, the number of samples is 2^24, otherwise the number of
// samples equals the written value times 2^8.
//
// 0x0000: 2^24 samples
// 0x0001: 1*2^8 samples
// 0x0002: 2*2^8 samples
// 0x0003: 3*2^8 samples
// ...
// 0x8000: 32768*2^8 samples
// 0xC000: 49152*2^8 samples
// ...
// 0xFFFF: 65535*2^8 samples
//
// This field can only be modified while TRNG_EN is 0. If 1 an update will be
// ignored.
#define TRNG_CTL_STARTUP_CYCLES_W                                           16
#define TRNG_CTL_STARTUP_CYCLES_M                                   0xFFFF0000
#define TRNG_CTL_STARTUP_CYCLES_S                                           16

// Field:    [10] TRNG_EN
//
// 0: Forces all TRNG logic back into the idle state immediately.
// 1: Starts TRNG, gathering entropy from the FROs for the number of samples
// determined by STARTUP_CYCLES.
#define TRNG_CTL_TRNG_EN                                            0x00000400
#define TRNG_CTL_TRNG_EN_BITN                                               10
#define TRNG_CTL_TRNG_EN_M                                          0x00000400
#define TRNG_CTL_TRNG_EN_S                                                  10

// Field:     [2] NO_LFSR_FB
//
// 1: Remove XNOR feedback from the main LFSR, converting it into a normal
// shift register for the XOR-ed outputs of the FROs (shifting data in on the
// LSB side). A '1' also forces the LFSR to sample continuously.
//
// This bit can only be set to '1' when TEST_MODE is also set to '1' and should
// not be used for other than test purposes
#define TRNG_CTL_NO_LFSR_FB                                         0x00000004
#define TRNG_CTL_NO_LFSR_FB_BITN                                             2
#define TRNG_CTL_NO_LFSR_FB_M                                       0x00000004
#define TRNG_CTL_NO_LFSR_FB_S                                                2

// Field:     [1] TEST_MODE
//
// 1: Enables access to the TESTCNT and LFSR0/LFSR1/LFSR2 registers (the latter
// are automatically cleared before enabling access)  and keeps
// IRQFLAGSTAT.NEED_CLOCK at '1'.
//
// This bit shall not be used unless you need to change the LFSR seed prior to
// creating a new random value. All other testing is done external to register
// control.
#define TRNG_CTL_TEST_MODE                                          0x00000002
#define TRNG_CTL_TEST_MODE_BITN                                              1
#define TRNG_CTL_TEST_MODE_M                                        0x00000002
#define TRNG_CTL_TEST_MODE_S                                                 1

//*****************************************************************************
//
// Register: TRNG_O_CFG0
//
//*****************************************************************************
// Field: [31:16] MAX_REFILL_CYCLES
//
// This field determines the maximum number of samples (between 2^8 and 2^24)
// taken to re-generate entropy from the FROs after reading out a 64 bits
// random number. If the written value of this field is zero, the number of
// samples is 2^24, otherwise the number of samples equals the written value
// times 2^8.
//
// 0x0000: 2^24 samples
// 0x0001: 1*2^8 samples
// 0x0002: 2*2^8 samples
// 0x0003: 3*2^8 samples
// ...
// 0x8000: 32768*2^8 samples
// 0xC000: 49152*2^8 samples
// ...
// 0xFFFF: 65535*2^8 samples
//
// This field can only be modified while CTL.TRNG_EN is 0.
#define TRNG_CFG0_MAX_REFILL_CYCLES_W                                       16
#define TRNG_CFG0_MAX_REFILL_CYCLES_M                               0xFFFF0000
#define TRNG_CFG0_MAX_REFILL_CYCLES_S                                       16

// Field:  [11:8] SMPL_DIV
//
// This field directly controls the number of clock cycles between samples
// taken from the FROs. Default value 0 indicates that samples are taken every
// clock cycle,
// maximum value 0xF takes one sample every 16 clock cycles.
// This field must be set to a value such that the slowest FRO (even under
// worst-case
// conditions) has a cycle time less than twice the sample period.
//
// This field can only be modified while CTL.TRNG_EN is '0'.
#define TRNG_CFG0_SMPL_DIV_W                                                 4
#define TRNG_CFG0_SMPL_DIV_M                                        0x00000F00
#define TRNG_CFG0_SMPL_DIV_S                                                 8

// Field:   [7:0] MIN_REFILL_CYCLES
//
// This field determines the minimum number of samples (between 2^6 and 2^14)
// taken to re-generate entropy from the FROs after reading out a 64 bits
// random number. If the value of this field is zero, the number of samples is
// fixed to the value determined by the MAX_REFILL_CYCLES field, otherwise the
// minimum number of samples equals the written value times 64 (which can be up
// to 2^14). To ensure same entropy in all generated random numbers the value 0
// should be used. Then MAX_REFILL_CYCLES controls the minimum refill interval.
// The number of samples defined here cannot be higher than the number defined
// by the 'max_refill_cycles' field (i.e. that field takes precedence). No
// random value will be created if min refill > max refill.
//
// This field can only be modified while CTL.TRNG_EN = 0.
//
// 0x00: Minimum samples = MAX_REFILL_CYCLES (all numbers have same entropy)
// 0x01: 1*2^6 samples
// 0x02: 2*2^6 samples
// ...
// 0xFF: 255*2^6 samples
#define TRNG_CFG0_MIN_REFILL_CYCLES_W                                        8
#define TRNG_CFG0_MIN_REFILL_CYCLES_M                               0x000000FF
#define TRNG_CFG0_MIN_REFILL_CYCLES_S                                        0

//*****************************************************************************
//
// Register: TRNG_O_ALARMCNT
//
//*****************************************************************************
// Field: [29:24] SHUTDOWN_CNT
//
// Read-only, indicates the number of '1' bits in ALARMSTOP register.
// The maximum value equals the number of FROs.
#define TRNG_ALARMCNT_SHUTDOWN_CNT_W                                         6
#define TRNG_ALARMCNT_SHUTDOWN_CNT_M                                0x3F000000
#define TRNG_ALARMCNT_SHUTDOWN_CNT_S                                        24

// Field: [20:16] SHUTDOWN_THR
//
// Threshold setting for generating IRQFLAGSTAT.SHUTDOWN_OVF interrupt. The
// interrupt is triggered when SHUTDOWN_CNT value exceeds this bit field.
#define TRNG_ALARMCNT_SHUTDOWN_THR_W                                         5
#define TRNG_ALARMCNT_SHUTDOWN_THR_M                                0x001F0000
#define TRNG_ALARMCNT_SHUTDOWN_THR_S                                        16

// Field:   [7:0] ALARM_THR
//
// Alarm detection threshold for the repeating pattern detectors on each FRO.
// An FRO 'alarm event' is declared when a repeating pattern (of up to four
// samples length) is detected continuously for the number of samples defined
// by this field's value. Reset value 0xFF should keep the number of 'alarm
// events' to a manageable level.
#define TRNG_ALARMCNT_ALARM_THR_W                                            8
#define TRNG_ALARMCNT_ALARM_THR_M                                   0x000000FF
#define TRNG_ALARMCNT_ALARM_THR_S                                            0

//*****************************************************************************
//
// Register: TRNG_O_FROEN
//
//*****************************************************************************
// Field:  [23:0] FRO_MASK
//
// Enable bits for the individual FROs. A '1' in bit [n] enables FRO 'n'.
// Default state is all '1's to enable all FROs after power-up. Note that they
// are not actually started up before the CTL.TRNG_EN bit  is set to '1'.
//
// Bits are automatically forced to '0' here (and cannot be written to '1')
// while the corresponding bit in  ALARMSTOP.FRO_FLAGS has value '1'.
#define TRNG_FROEN_FRO_MASK_W                                               24
#define TRNG_FROEN_FRO_MASK_M                                       0x00FFFFFF
#define TRNG_FROEN_FRO_MASK_S                                                0

//*****************************************************************************
//
// Register: TRNG_O_FRODETUNE
//
//*****************************************************************************
// Field:  [23:0] FRO_MASK
//
// De-tune bits for the individual FROs. A '1' in bit [n] lets FRO 'n' run
// approximately 5% faster. The value of one of these bits may only be changed
// while the corresponding FRO is turned off (by temporarily writing a '0' in
// the corresponding
// bit of the FROEN.FRO_MASK register).
#define TRNG_FRODETUNE_FRO_MASK_W                                           24
#define TRNG_FRODETUNE_FRO_MASK_M                                   0x00FFFFFF
#define TRNG_FRODETUNE_FRO_MASK_S                                            0

//*****************************************************************************
//
// Register: TRNG_O_ALARMMASK
//
//*****************************************************************************
// Field:  [23:0] FRO_MASK
//
// Logging bits for the 'alarm events' of individual FROs. A '1' in bit [n]
// indicates FRO 'n' experienced an 'alarm event'.
#define TRNG_ALARMMASK_FRO_MASK_W                                           24
#define TRNG_ALARMMASK_FRO_MASK_M                                   0x00FFFFFF
#define TRNG_ALARMMASK_FRO_MASK_S                                            0

//*****************************************************************************
//
// Register: TRNG_O_ALARMSTOP
//
//*****************************************************************************
// Field:  [23:0] FRO_FLAGS
//
// Logging bits for the 'alarm events' of individual FROs. A '1' in bit [n]
// indicates FRO 'n' experienced more than one 'alarm event' in quick
// succession and has been turned off. A '1' in this field forces the
// corresponding bit in FROEN.FRO_MASK to '0'.
#define TRNG_ALARMSTOP_FRO_FLAGS_W                                          24
#define TRNG_ALARMSTOP_FRO_FLAGS_M                                  0x00FFFFFF
#define TRNG_ALARMSTOP_FRO_FLAGS_S                                           0

//*****************************************************************************
//
// Register: TRNG_O_LFSR0
//
//*****************************************************************************
// Field:  [31:0] LFSR_31_0
//
// Bits [31:0] of the main entropy accumulation LFSR. Register can only be
// accessed when CTL.TEST_MODE  = 1.
// Register contents will be cleared to zero before access is enabled.
#define TRNG_LFSR0_LFSR_31_0_W                                              32
#define TRNG_LFSR0_LFSR_31_0_M                                      0xFFFFFFFF
#define TRNG_LFSR0_LFSR_31_0_S                                               0

//*****************************************************************************
//
// Register: TRNG_O_LFSR1
//
//*****************************************************************************
// Field:  [31:0] LFSR_63_32
//
// Bits [63:32] of the main entropy accumulation LFSR. Register can only be
// accessed when CTL.TEST_MODE = 1.
// Register contents will be cleared to zero before access is enabled.
#define TRNG_LFSR1_LFSR_63_32_W                                             32
#define TRNG_LFSR1_LFSR_63_32_M                                     0xFFFFFFFF
#define TRNG_LFSR1_LFSR_63_32_S                                              0

//*****************************************************************************
//
// Register: TRNG_O_LFSR2
//
//*****************************************************************************
// Field:  [16:0] LFSR_80_64
//
// Bits [80:64] of the main entropy accumulation LFSR. Register can only be
// accessed when CTL.TEST_MODE = 1.
// Register contents will be cleared to zero before access is enabled.
#define TRNG_LFSR2_LFSR_80_64_W                                             17
#define TRNG_LFSR2_LFSR_80_64_M                                     0x0001FFFF
#define TRNG_LFSR2_LFSR_80_64_S                                              0

//*****************************************************************************
//
// Register: TRNG_O_HWOPT
//
//*****************************************************************************
// Field:  [11:6] NR_OF_FROS
//
// Number of FROs implemented in this TRNG, value 24 (decimal).
#define TRNG_HWOPT_NR_OF_FROS_W                                              6
#define TRNG_HWOPT_NR_OF_FROS_M                                     0x00000FC0
#define TRNG_HWOPT_NR_OF_FROS_S                                              6

//*****************************************************************************
//
// Register: TRNG_O_HWVER0
//
//*****************************************************************************
// Field: [27:24] HW_MAJOR_VER
//
// 4 bits binary encoding of the major hardware revision number.
#define TRNG_HWVER0_HW_MAJOR_VER_W                                           4
#define TRNG_HWVER0_HW_MAJOR_VER_M                                  0x0F000000
#define TRNG_HWVER0_HW_MAJOR_VER_S                                          24

// Field: [23:20] HW_MINOR_VER
//
// 4 bits binary encoding of the minor hardware revision number.
#define TRNG_HWVER0_HW_MINOR_VER_W                                           4
#define TRNG_HWVER0_HW_MINOR_VER_M                                  0x00F00000
#define TRNG_HWVER0_HW_MINOR_VER_S                                          20

// Field: [19:16] HW_PATCH_LVL
//
// 4 bits binary encoding of the hardware patch level, initial release will
// carry value zero.
#define TRNG_HWVER0_HW_PATCH_LVL_W                                           4
#define TRNG_HWVER0_HW_PATCH_LVL_M                                  0x000F0000
#define TRNG_HWVER0_HW_PATCH_LVL_S                                          16

// Field:  [15:8] EIP_NUM_COMPL
//
// Bit-by-bit logic complement of bits [7:0]. This TRNG gives 0xB4.
#define TRNG_HWVER0_EIP_NUM_COMPL_W                                          8
#define TRNG_HWVER0_EIP_NUM_COMPL_M                                 0x0000FF00
#define TRNG_HWVER0_EIP_NUM_COMPL_S                                          8

// Field:   [7:0] EIP_NUM
//
// 8 bits binary encoding of the module number. This TRNG gives 0x4B.
#define TRNG_HWVER0_EIP_NUM_W                                                8
#define TRNG_HWVER0_EIP_NUM_M                                       0x000000FF
#define TRNG_HWVER0_EIP_NUM_S                                                0

//*****************************************************************************
//
// Register: TRNG_O_IRQSTATMASK
//
//*****************************************************************************
// Field:     [1] SHUTDOWN_OVF
//
// Shutdown Overflow (result of IRQFLAGSTAT.SHUTDOWN_OVF AND'ed with
// IRQFLAGMASK.SHUTDOWN_OVF)
#define TRNG_IRQSTATMASK_SHUTDOWN_OVF                               0x00000002
#define TRNG_IRQSTATMASK_SHUTDOWN_OVF_BITN                                   1
#define TRNG_IRQSTATMASK_SHUTDOWN_OVF_M                             0x00000002
#define TRNG_IRQSTATMASK_SHUTDOWN_OVF_S                                      1

// Field:     [0] RDY
//
// New random value available (result of IRQFLAGSTAT.RDY AND'ed with
// IRQFLAGMASK.RDY)
#define TRNG_IRQSTATMASK_RDY                                        0x00000001
#define TRNG_IRQSTATMASK_RDY_BITN                                            0
#define TRNG_IRQSTATMASK_RDY_M                                      0x00000001
#define TRNG_IRQSTATMASK_RDY_S                                               0

//*****************************************************************************
//
// Register: TRNG_O_HWVER1
//
//*****************************************************************************
// Field:   [7:0] REV
//
// The revision number of this module is Rev 2.0.
#define TRNG_HWVER1_REV_W                                                    8
#define TRNG_HWVER1_REV_M                                           0x000000FF
#define TRNG_HWVER1_REV_S                                                    0

//*****************************************************************************
//
// Register: TRNG_O_IRQSET
//
//*****************************************************************************
//*****************************************************************************
//
// Register: TRNG_O_SWRESET
//
//*****************************************************************************
// Field:     [0] RESET
//
// Write '1' to soft reset , reset will be low for 4-5 clock cycles. Poll to 0
// for reset to be completed.
#define TRNG_SWRESET_RESET                                          0x00000001
#define TRNG_SWRESET_RESET_BITN                                              0
#define TRNG_SWRESET_RESET_M                                        0x00000001
#define TRNG_SWRESET_RESET_S                                                 0

//*****************************************************************************
//
// Register: TRNG_O_IRQSTAT
//
//*****************************************************************************
// Field:     [0] STAT
//
// TRNG Interrupt status. OR'ed version of IRQFLAGSTAT.SHUTDOWN_OVF and
// IRQFLAGSTAT.RDY
#define TRNG_IRQSTAT_STAT                                           0x00000001
#define TRNG_IRQSTAT_STAT_BITN                                               0
#define TRNG_IRQSTAT_STAT_M                                         0x00000001
#define TRNG_IRQSTAT_STAT_S                                                  0


#endif // __TRNG__
