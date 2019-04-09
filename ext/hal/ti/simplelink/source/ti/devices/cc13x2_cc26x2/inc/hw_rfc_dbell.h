/******************************************************************************
*  Filename:       hw_rfc_dbell_h
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

#ifndef __HW_RFC_DBELL_H__
#define __HW_RFC_DBELL_H__

//*****************************************************************************
//
// This section defines the register offsets of
// RFC_DBELL component
//
//*****************************************************************************
// Doorbell Command Register
#define RFC_DBELL_O_CMDR                                            0x00000000

// Doorbell Command Status Register
#define RFC_DBELL_O_CMDSTA                                          0x00000004

// Interrupt Flags From RF Hardware Modules
#define RFC_DBELL_O_RFHWIFG                                         0x00000008

// Interrupt Enable For RF Hardware Modules
#define RFC_DBELL_O_RFHWIEN                                         0x0000000C

// Interrupt Flags For Command and Packet Engine Generated Interrupts
#define RFC_DBELL_O_RFCPEIFG                                        0x00000010

// Interrupt Enable For Command and Packet Engine Generated Interrupts
#define RFC_DBELL_O_RFCPEIEN                                        0x00000014

// Interrupt Vector Selection For Command and Packet Engine Generated
// Interrupts
#define RFC_DBELL_O_RFCPEISL                                        0x00000018

// Doorbell Command Acknowledgement Interrupt Flag
#define RFC_DBELL_O_RFACKIFG                                        0x0000001C

// RF Core General Purpose Output Control
#define RFC_DBELL_O_SYSGPOCTL                                       0x00000020

//*****************************************************************************
//
// Register: RFC_DBELL_O_CMDR
//
//*****************************************************************************
// Field:  [31:0] CMD
//
// Command register. Raises an interrupt to the Command and packet engine (CPE)
// upon write.
#define RFC_DBELL_CMDR_CMD_W                                                32
#define RFC_DBELL_CMDR_CMD_M                                        0xFFFFFFFF
#define RFC_DBELL_CMDR_CMD_S                                                 0

//*****************************************************************************
//
// Register: RFC_DBELL_O_CMDSTA
//
//*****************************************************************************
// Field:  [31:0] STAT
//
// Status of the last command used
#define RFC_DBELL_CMDSTA_STAT_W                                             32
#define RFC_DBELL_CMDSTA_STAT_M                                     0xFFFFFFFF
#define RFC_DBELL_CMDSTA_STAT_S                                              0

//*****************************************************************************
//
// Register: RFC_DBELL_O_RFHWIFG
//
//*****************************************************************************
// Field:    [19] RATCH7
//
// Radio timer channel 7 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH7                                    0x00080000
#define RFC_DBELL_RFHWIFG_RATCH7_BITN                                       19
#define RFC_DBELL_RFHWIFG_RATCH7_M                                  0x00080000
#define RFC_DBELL_RFHWIFG_RATCH7_S                                          19

// Field:    [18] RATCH6
//
// Radio timer channel 6 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH6                                    0x00040000
#define RFC_DBELL_RFHWIFG_RATCH6_BITN                                       18
#define RFC_DBELL_RFHWIFG_RATCH6_M                                  0x00040000
#define RFC_DBELL_RFHWIFG_RATCH6_S                                          18

// Field:    [17] RATCH5
//
// Radio timer channel 5 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH5                                    0x00020000
#define RFC_DBELL_RFHWIFG_RATCH5_BITN                                       17
#define RFC_DBELL_RFHWIFG_RATCH5_M                                  0x00020000
#define RFC_DBELL_RFHWIFG_RATCH5_S                                          17

// Field:    [16] RATCH4
//
// Radio timer channel 4 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH4                                    0x00010000
#define RFC_DBELL_RFHWIFG_RATCH4_BITN                                       16
#define RFC_DBELL_RFHWIFG_RATCH4_M                                  0x00010000
#define RFC_DBELL_RFHWIFG_RATCH4_S                                          16

// Field:    [15] RATCH3
//
// Radio timer channel 3 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH3                                    0x00008000
#define RFC_DBELL_RFHWIFG_RATCH3_BITN                                       15
#define RFC_DBELL_RFHWIFG_RATCH3_M                                  0x00008000
#define RFC_DBELL_RFHWIFG_RATCH3_S                                          15

// Field:    [14] RATCH2
//
// Radio timer channel 2 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH2                                    0x00004000
#define RFC_DBELL_RFHWIFG_RATCH2_BITN                                       14
#define RFC_DBELL_RFHWIFG_RATCH2_M                                  0x00004000
#define RFC_DBELL_RFHWIFG_RATCH2_S                                          14

// Field:    [13] RATCH1
//
// Radio timer channel 1 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH1                                    0x00002000
#define RFC_DBELL_RFHWIFG_RATCH1_BITN                                       13
#define RFC_DBELL_RFHWIFG_RATCH1_M                                  0x00002000
#define RFC_DBELL_RFHWIFG_RATCH1_S                                          13

// Field:    [12] RATCH0
//
// Radio timer channel 0 interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_RATCH0                                    0x00001000
#define RFC_DBELL_RFHWIFG_RATCH0_BITN                                       12
#define RFC_DBELL_RFHWIFG_RATCH0_M                                  0x00001000
#define RFC_DBELL_RFHWIFG_RATCH0_S                                          12

// Field:    [11] RFESOFT2
//
// RF engine software defined interrupt 2 flag. Write zero to clear flag. Write
// to one has no effect.
#define RFC_DBELL_RFHWIFG_RFESOFT2                                  0x00000800
#define RFC_DBELL_RFHWIFG_RFESOFT2_BITN                                     11
#define RFC_DBELL_RFHWIFG_RFESOFT2_M                                0x00000800
#define RFC_DBELL_RFHWIFG_RFESOFT2_S                                        11

// Field:    [10] RFESOFT1
//
// RF engine software defined interrupt 1 flag. Write zero to clear flag. Write
// to one has no effect.
#define RFC_DBELL_RFHWIFG_RFESOFT1                                  0x00000400
#define RFC_DBELL_RFHWIFG_RFESOFT1_BITN                                     10
#define RFC_DBELL_RFHWIFG_RFESOFT1_M                                0x00000400
#define RFC_DBELL_RFHWIFG_RFESOFT1_S                                        10

// Field:     [9] RFESOFT0
//
// RF engine software defined interrupt 0 flag. Write zero to clear flag. Write
// to one has no effect.
#define RFC_DBELL_RFHWIFG_RFESOFT0                                  0x00000200
#define RFC_DBELL_RFHWIFG_RFESOFT0_BITN                                      9
#define RFC_DBELL_RFHWIFG_RFESOFT0_M                                0x00000200
#define RFC_DBELL_RFHWIFG_RFESOFT0_S                                         9

// Field:     [8] RFEDONE
//
// RF engine command done interrupt flag. Write zero to clear flag. Write to
// one has no effect.
#define RFC_DBELL_RFHWIFG_RFEDONE                                   0x00000100
#define RFC_DBELL_RFHWIFG_RFEDONE_BITN                                       8
#define RFC_DBELL_RFHWIFG_RFEDONE_M                                 0x00000100
#define RFC_DBELL_RFHWIFG_RFEDONE_S                                          8

// Field:     [6] TRCTK
//
// Debug tracer system tick interrupt flag. Write zero to clear flag. Write to
// one has no effect.
#define RFC_DBELL_RFHWIFG_TRCTK                                     0x00000040
#define RFC_DBELL_RFHWIFG_TRCTK_BITN                                         6
#define RFC_DBELL_RFHWIFG_TRCTK_M                                   0x00000040
#define RFC_DBELL_RFHWIFG_TRCTK_S                                            6

// Field:     [5] MDMSOFT
//
// Modem software defined interrupt flag. Write zero to clear flag. Write to
// one has no effect.
#define RFC_DBELL_RFHWIFG_MDMSOFT                                   0x00000020
#define RFC_DBELL_RFHWIFG_MDMSOFT_BITN                                       5
#define RFC_DBELL_RFHWIFG_MDMSOFT_M                                 0x00000020
#define RFC_DBELL_RFHWIFG_MDMSOFT_S                                          5

// Field:     [4] MDMOUT
//
// Modem FIFO output interrupt flag. Write zero to clear flag. Write to one has
// no effect.
#define RFC_DBELL_RFHWIFG_MDMOUT                                    0x00000010
#define RFC_DBELL_RFHWIFG_MDMOUT_BITN                                        4
#define RFC_DBELL_RFHWIFG_MDMOUT_M                                  0x00000010
#define RFC_DBELL_RFHWIFG_MDMOUT_S                                           4

// Field:     [3] MDMIN
//
// Modem FIFO input interrupt flag. Write zero to clear flag. Write to one has
// no effect.
#define RFC_DBELL_RFHWIFG_MDMIN                                     0x00000008
#define RFC_DBELL_RFHWIFG_MDMIN_BITN                                         3
#define RFC_DBELL_RFHWIFG_MDMIN_M                                   0x00000008
#define RFC_DBELL_RFHWIFG_MDMIN_S                                            3

// Field:     [2] MDMDONE
//
// Modem command done interrupt flag. Write zero to clear flag. Write to one
// has no effect.
#define RFC_DBELL_RFHWIFG_MDMDONE                                   0x00000004
#define RFC_DBELL_RFHWIFG_MDMDONE_BITN                                       2
#define RFC_DBELL_RFHWIFG_MDMDONE_M                                 0x00000004
#define RFC_DBELL_RFHWIFG_MDMDONE_S                                          2

// Field:     [1] FSCA
//
// Frequency synthesizer calibration accelerator interrupt flag. Write zero to
// clear flag. Write to one has no effect.
#define RFC_DBELL_RFHWIFG_FSCA                                      0x00000002
#define RFC_DBELL_RFHWIFG_FSCA_BITN                                          1
#define RFC_DBELL_RFHWIFG_FSCA_M                                    0x00000002
#define RFC_DBELL_RFHWIFG_FSCA_S                                             1

//*****************************************************************************
//
// Register: RFC_DBELL_O_RFHWIEN
//
//*****************************************************************************
// Field:    [19] RATCH7
//
// Interrupt enable for RFHWIFG.RATCH7.
#define RFC_DBELL_RFHWIEN_RATCH7                                    0x00080000
#define RFC_DBELL_RFHWIEN_RATCH7_BITN                                       19
#define RFC_DBELL_RFHWIEN_RATCH7_M                                  0x00080000
#define RFC_DBELL_RFHWIEN_RATCH7_S                                          19

// Field:    [18] RATCH6
//
// Interrupt enable for RFHWIFG.RATCH6.
#define RFC_DBELL_RFHWIEN_RATCH6                                    0x00040000
#define RFC_DBELL_RFHWIEN_RATCH6_BITN                                       18
#define RFC_DBELL_RFHWIEN_RATCH6_M                                  0x00040000
#define RFC_DBELL_RFHWIEN_RATCH6_S                                          18

// Field:    [17] RATCH5
//
// Interrupt enable for RFHWIFG.RATCH5.
#define RFC_DBELL_RFHWIEN_RATCH5                                    0x00020000
#define RFC_DBELL_RFHWIEN_RATCH5_BITN                                       17
#define RFC_DBELL_RFHWIEN_RATCH5_M                                  0x00020000
#define RFC_DBELL_RFHWIEN_RATCH5_S                                          17

// Field:    [16] RATCH4
//
// Interrupt enable for RFHWIFG.RATCH4.
#define RFC_DBELL_RFHWIEN_RATCH4                                    0x00010000
#define RFC_DBELL_RFHWIEN_RATCH4_BITN                                       16
#define RFC_DBELL_RFHWIEN_RATCH4_M                                  0x00010000
#define RFC_DBELL_RFHWIEN_RATCH4_S                                          16

// Field:    [15] RATCH3
//
// Interrupt enable for RFHWIFG.RATCH3.
#define RFC_DBELL_RFHWIEN_RATCH3                                    0x00008000
#define RFC_DBELL_RFHWIEN_RATCH3_BITN                                       15
#define RFC_DBELL_RFHWIEN_RATCH3_M                                  0x00008000
#define RFC_DBELL_RFHWIEN_RATCH3_S                                          15

// Field:    [14] RATCH2
//
// Interrupt enable for RFHWIFG.RATCH2.
#define RFC_DBELL_RFHWIEN_RATCH2                                    0x00004000
#define RFC_DBELL_RFHWIEN_RATCH2_BITN                                       14
#define RFC_DBELL_RFHWIEN_RATCH2_M                                  0x00004000
#define RFC_DBELL_RFHWIEN_RATCH2_S                                          14

// Field:    [13] RATCH1
//
// Interrupt enable for RFHWIFG.RATCH1.
#define RFC_DBELL_RFHWIEN_RATCH1                                    0x00002000
#define RFC_DBELL_RFHWIEN_RATCH1_BITN                                       13
#define RFC_DBELL_RFHWIEN_RATCH1_M                                  0x00002000
#define RFC_DBELL_RFHWIEN_RATCH1_S                                          13

// Field:    [12] RATCH0
//
// Interrupt enable for RFHWIFG.RATCH0.
#define RFC_DBELL_RFHWIEN_RATCH0                                    0x00001000
#define RFC_DBELL_RFHWIEN_RATCH0_BITN                                       12
#define RFC_DBELL_RFHWIEN_RATCH0_M                                  0x00001000
#define RFC_DBELL_RFHWIEN_RATCH0_S                                          12

// Field:    [11] RFESOFT2
//
// Interrupt enable for RFHWIFG.RFESOFT2.
#define RFC_DBELL_RFHWIEN_RFESOFT2                                  0x00000800
#define RFC_DBELL_RFHWIEN_RFESOFT2_BITN                                     11
#define RFC_DBELL_RFHWIEN_RFESOFT2_M                                0x00000800
#define RFC_DBELL_RFHWIEN_RFESOFT2_S                                        11

// Field:    [10] RFESOFT1
//
// Interrupt enable for RFHWIFG.RFESOFT1.
#define RFC_DBELL_RFHWIEN_RFESOFT1                                  0x00000400
#define RFC_DBELL_RFHWIEN_RFESOFT1_BITN                                     10
#define RFC_DBELL_RFHWIEN_RFESOFT1_M                                0x00000400
#define RFC_DBELL_RFHWIEN_RFESOFT1_S                                        10

// Field:     [9] RFESOFT0
//
// Interrupt enable for RFHWIFG.RFESOFT0.
#define RFC_DBELL_RFHWIEN_RFESOFT0                                  0x00000200
#define RFC_DBELL_RFHWIEN_RFESOFT0_BITN                                      9
#define RFC_DBELL_RFHWIEN_RFESOFT0_M                                0x00000200
#define RFC_DBELL_RFHWIEN_RFESOFT0_S                                         9

// Field:     [8] RFEDONE
//
// Interrupt enable for RFHWIFG.RFEDONE.
#define RFC_DBELL_RFHWIEN_RFEDONE                                   0x00000100
#define RFC_DBELL_RFHWIEN_RFEDONE_BITN                                       8
#define RFC_DBELL_RFHWIEN_RFEDONE_M                                 0x00000100
#define RFC_DBELL_RFHWIEN_RFEDONE_S                                          8

// Field:     [6] TRCTK
//
// Interrupt enable for RFHWIFG.TRCTK.
#define RFC_DBELL_RFHWIEN_TRCTK                                     0x00000040
#define RFC_DBELL_RFHWIEN_TRCTK_BITN                                         6
#define RFC_DBELL_RFHWIEN_TRCTK_M                                   0x00000040
#define RFC_DBELL_RFHWIEN_TRCTK_S                                            6

// Field:     [5] MDMSOFT
//
// Interrupt enable for RFHWIFG.MDMSOFT.
#define RFC_DBELL_RFHWIEN_MDMSOFT                                   0x00000020
#define RFC_DBELL_RFHWIEN_MDMSOFT_BITN                                       5
#define RFC_DBELL_RFHWIEN_MDMSOFT_M                                 0x00000020
#define RFC_DBELL_RFHWIEN_MDMSOFT_S                                          5

// Field:     [4] MDMOUT
//
// Interrupt enable for RFHWIFG.MDMOUT.
#define RFC_DBELL_RFHWIEN_MDMOUT                                    0x00000010
#define RFC_DBELL_RFHWIEN_MDMOUT_BITN                                        4
#define RFC_DBELL_RFHWIEN_MDMOUT_M                                  0x00000010
#define RFC_DBELL_RFHWIEN_MDMOUT_S                                           4

// Field:     [3] MDMIN
//
// Interrupt enable for RFHWIFG.MDMIN.
#define RFC_DBELL_RFHWIEN_MDMIN                                     0x00000008
#define RFC_DBELL_RFHWIEN_MDMIN_BITN                                         3
#define RFC_DBELL_RFHWIEN_MDMIN_M                                   0x00000008
#define RFC_DBELL_RFHWIEN_MDMIN_S                                            3

// Field:     [2] MDMDONE
//
// Interrupt enable for RFHWIFG.MDMDONE.
#define RFC_DBELL_RFHWIEN_MDMDONE                                   0x00000004
#define RFC_DBELL_RFHWIEN_MDMDONE_BITN                                       2
#define RFC_DBELL_RFHWIEN_MDMDONE_M                                 0x00000004
#define RFC_DBELL_RFHWIEN_MDMDONE_S                                          2

// Field:     [1] FSCA
//
// Interrupt enable for RFHWIFG.FSCA.
#define RFC_DBELL_RFHWIEN_FSCA                                      0x00000002
#define RFC_DBELL_RFHWIEN_FSCA_BITN                                          1
#define RFC_DBELL_RFHWIEN_FSCA_M                                    0x00000002
#define RFC_DBELL_RFHWIEN_FSCA_S                                             1

//*****************************************************************************
//
// Register: RFC_DBELL_O_RFCPEIFG
//
//*****************************************************************************
// Field:    [31] INTERNAL_ERROR
//
// Interrupt flag 31. The command and packet engine (CPE) has observed an
// unexpected error. A reset of the CPE is needed. This can be done by
// switching the RF Core power domain off and on in PRCM:PDCTL1RFC. Write zero
// to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_INTERNAL_ERROR                           0x80000000
#define RFC_DBELL_RFCPEIFG_INTERNAL_ERROR_BITN                              31
#define RFC_DBELL_RFCPEIFG_INTERNAL_ERROR_M                         0x80000000
#define RFC_DBELL_RFCPEIFG_INTERNAL_ERROR_S                                 31

// Field:    [30] BOOT_DONE
//
// Interrupt flag 30. The command and packet engine (CPE) boot is finished.
// Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_BOOT_DONE                                0x40000000
#define RFC_DBELL_RFCPEIFG_BOOT_DONE_BITN                                   30
#define RFC_DBELL_RFCPEIFG_BOOT_DONE_M                              0x40000000
#define RFC_DBELL_RFCPEIFG_BOOT_DONE_S                                      30

// Field:    [29] MODULES_UNLOCKED
//
// Interrupt flag 29. As part of command and packet engine (CPE) boot process,
// it has opened access to RF Core modules and memories. Write zero to clear
// flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_MODULES_UNLOCKED                         0x20000000
#define RFC_DBELL_RFCPEIFG_MODULES_UNLOCKED_BITN                            29
#define RFC_DBELL_RFCPEIFG_MODULES_UNLOCKED_M                       0x20000000
#define RFC_DBELL_RFCPEIFG_MODULES_UNLOCKED_S                               29

// Field:    [28] SYNTH_NO_LOCK
//
// Interrupt flag 28. The phase-locked loop in frequency synthesizer has
// reported loss of lock. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_SYNTH_NO_LOCK                            0x10000000
#define RFC_DBELL_RFCPEIFG_SYNTH_NO_LOCK_BITN                               28
#define RFC_DBELL_RFCPEIFG_SYNTH_NO_LOCK_M                          0x10000000
#define RFC_DBELL_RFCPEIFG_SYNTH_NO_LOCK_S                                  28

// Field:    [27] IRQ27
//
// Interrupt flag 27. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_IRQ27                                    0x08000000
#define RFC_DBELL_RFCPEIFG_IRQ27_BITN                                       27
#define RFC_DBELL_RFCPEIFG_IRQ27_M                                  0x08000000
#define RFC_DBELL_RFCPEIFG_IRQ27_S                                          27

// Field:    [26] RX_ABORTED
//
// Interrupt flag 26. Packet reception stopped before packet was done. Write
// zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_ABORTED                               0x04000000
#define RFC_DBELL_RFCPEIFG_RX_ABORTED_BITN                                  26
#define RFC_DBELL_RFCPEIFG_RX_ABORTED_M                             0x04000000
#define RFC_DBELL_RFCPEIFG_RX_ABORTED_S                                     26

// Field:    [25] RX_N_DATA_WRITTEN
//
// Interrupt flag 25. Specified number of bytes written to partial read Rx
// buffer. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_N_DATA_WRITTEN                        0x02000000
#define RFC_DBELL_RFCPEIFG_RX_N_DATA_WRITTEN_BITN                           25
#define RFC_DBELL_RFCPEIFG_RX_N_DATA_WRITTEN_M                      0x02000000
#define RFC_DBELL_RFCPEIFG_RX_N_DATA_WRITTEN_S                              25

// Field:    [24] RX_DATA_WRITTEN
//
// Interrupt flag 24. Data written to partial read Rx buffer. Write zero to
// clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_DATA_WRITTEN                          0x01000000
#define RFC_DBELL_RFCPEIFG_RX_DATA_WRITTEN_BITN                             24
#define RFC_DBELL_RFCPEIFG_RX_DATA_WRITTEN_M                        0x01000000
#define RFC_DBELL_RFCPEIFG_RX_DATA_WRITTEN_S                                24

// Field:    [23] RX_ENTRY_DONE
//
// Interrupt flag 23. Rx queue data entry changing state to finished.  Write
// zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_ENTRY_DONE                            0x00800000
#define RFC_DBELL_RFCPEIFG_RX_ENTRY_DONE_BITN                               23
#define RFC_DBELL_RFCPEIFG_RX_ENTRY_DONE_M                          0x00800000
#define RFC_DBELL_RFCPEIFG_RX_ENTRY_DONE_S                                  23

// Field:    [22] RX_BUF_FULL
//
// Interrupt flag 22. Packet received that did not fit in Rx queue. BLE mode:
// Packet received that did not fit in the Rx queue. IEEE 802.15.4 mode: Frame
// received that did not fit in the Rx queue. Write zero to clear flag. Write
// to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_BUF_FULL                              0x00400000
#define RFC_DBELL_RFCPEIFG_RX_BUF_FULL_BITN                                 22
#define RFC_DBELL_RFCPEIFG_RX_BUF_FULL_M                            0x00400000
#define RFC_DBELL_RFCPEIFG_RX_BUF_FULL_S                                    22

// Field:    [21] RX_CTRL_ACK
//
// Interrupt flag 21. BLE mode only: LL control packet received with CRC OK,
// not to be ignored, then acknowledgement sent. Write zero to clear flag.
// Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_CTRL_ACK                              0x00200000
#define RFC_DBELL_RFCPEIFG_RX_CTRL_ACK_BITN                                 21
#define RFC_DBELL_RFCPEIFG_RX_CTRL_ACK_M                            0x00200000
#define RFC_DBELL_RFCPEIFG_RX_CTRL_ACK_S                                    21

// Field:    [20] RX_CTRL
//
// Interrupt flag 20. BLE mode only: LL control packet received with CRC OK,
// not to be ignored. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_CTRL                                  0x00100000
#define RFC_DBELL_RFCPEIFG_RX_CTRL_BITN                                     20
#define RFC_DBELL_RFCPEIFG_RX_CTRL_M                                0x00100000
#define RFC_DBELL_RFCPEIFG_RX_CTRL_S                                        20

// Field:    [19] RX_EMPTY
//
// Interrupt flag 19. BLE mode only: Packet received with CRC OK, not to be
// ignored, no payload. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_EMPTY                                 0x00080000
#define RFC_DBELL_RFCPEIFG_RX_EMPTY_BITN                                    19
#define RFC_DBELL_RFCPEIFG_RX_EMPTY_M                               0x00080000
#define RFC_DBELL_RFCPEIFG_RX_EMPTY_S                                       19

// Field:    [18] RX_IGNORED
//
// Interrupt flag 18. Packet received, but can be ignored. BLE mode: Packet
// received with CRC OK, but to be ignored. IEEE 802.15.4 mode: Frame received
// with ignore flag set. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_IGNORED                               0x00040000
#define RFC_DBELL_RFCPEIFG_RX_IGNORED_BITN                                  18
#define RFC_DBELL_RFCPEIFG_RX_IGNORED_M                             0x00040000
#define RFC_DBELL_RFCPEIFG_RX_IGNORED_S                                     18

// Field:    [17] RX_NOK
//
// Interrupt flag 17. Packet received with CRC error. BLE mode: Packet received
// with CRC error. IEEE 802.15.4 mode: Frame received with CRC error. Write
// zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_NOK                                   0x00020000
#define RFC_DBELL_RFCPEIFG_RX_NOK_BITN                                      17
#define RFC_DBELL_RFCPEIFG_RX_NOK_M                                 0x00020000
#define RFC_DBELL_RFCPEIFG_RX_NOK_S                                         17

// Field:    [16] RX_OK
//
// Interrupt flag 16. Packet received correctly. BLE mode: Packet received with
// CRC OK, payload, and not to be ignored. IEEE 802.15.4 mode: Frame received
// with CRC OK. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_RX_OK                                    0x00010000
#define RFC_DBELL_RFCPEIFG_RX_OK_BITN                                       16
#define RFC_DBELL_RFCPEIFG_RX_OK_M                                  0x00010000
#define RFC_DBELL_RFCPEIFG_RX_OK_S                                          16

// Field:    [15] IRQ15
//
// Interrupt flag 15. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_IRQ15                                    0x00008000
#define RFC_DBELL_RFCPEIFG_IRQ15_BITN                                       15
#define RFC_DBELL_RFCPEIFG_IRQ15_M                                  0x00008000
#define RFC_DBELL_RFCPEIFG_IRQ15_S                                          15

// Field:    [14] IRQ14
//
// Interrupt flag 14. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_IRQ14                                    0x00004000
#define RFC_DBELL_RFCPEIFG_IRQ14_BITN                                       14
#define RFC_DBELL_RFCPEIFG_IRQ14_M                                  0x00004000
#define RFC_DBELL_RFCPEIFG_IRQ14_S                                          14

// Field:    [13] FG_COMMAND_STARTED
//
// Interrupt flag 13. IEEE 802.15.4 mode only: A foreground radio operation
// command has gone into active state.
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_STARTED                       0x00002000
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_STARTED_BITN                          13
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_STARTED_M                     0x00002000
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_STARTED_S                             13

// Field:    [12] COMMAND_STARTED
//
// Interrupt flag 12. A radio operation command has gone into active state.
#define RFC_DBELL_RFCPEIFG_COMMAND_STARTED                          0x00001000
#define RFC_DBELL_RFCPEIFG_COMMAND_STARTED_BITN                             12
#define RFC_DBELL_RFCPEIFG_COMMAND_STARTED_M                        0x00001000
#define RFC_DBELL_RFCPEIFG_COMMAND_STARTED_S                                12

// Field:    [11] TX_BUFFER_CHANGED
//
// Interrupt flag 11. BLE mode only: A buffer change is complete after
// CMD_BLE_ADV_PAYLOAD. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_BUFFER_CHANGED                        0x00000800
#define RFC_DBELL_RFCPEIFG_TX_BUFFER_CHANGED_BITN                           11
#define RFC_DBELL_RFCPEIFG_TX_BUFFER_CHANGED_M                      0x00000800
#define RFC_DBELL_RFCPEIFG_TX_BUFFER_CHANGED_S                              11

// Field:    [10] TX_ENTRY_DONE
//
// Interrupt flag 10. Tx queue data entry state changed to finished. Write zero
// to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_ENTRY_DONE                            0x00000400
#define RFC_DBELL_RFCPEIFG_TX_ENTRY_DONE_BITN                               10
#define RFC_DBELL_RFCPEIFG_TX_ENTRY_DONE_M                          0x00000400
#define RFC_DBELL_RFCPEIFG_TX_ENTRY_DONE_S                                  10

// Field:     [9] TX_RETRANS
//
// Interrupt flag 9. BLE mode only: Packet retransmitted. Write zero to clear
// flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_RETRANS                               0x00000200
#define RFC_DBELL_RFCPEIFG_TX_RETRANS_BITN                                   9
#define RFC_DBELL_RFCPEIFG_TX_RETRANS_M                             0x00000200
#define RFC_DBELL_RFCPEIFG_TX_RETRANS_S                                      9

// Field:     [8] TX_CTRL_ACK_ACK
//
// Interrupt flag 8. BLE mode only: Acknowledgement received on a transmitted
// LL control packet, and acknowledgement transmitted for that packet. Write
// zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK_ACK                          0x00000100
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK_ACK_BITN                              8
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK_ACK_M                        0x00000100
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK_ACK_S                                 8

// Field:     [7] TX_CTRL_ACK
//
// Interrupt flag 7. BLE mode: Acknowledgement received on a transmitted LL
// control packet. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK                              0x00000080
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK_BITN                                  7
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK_M                            0x00000080
#define RFC_DBELL_RFCPEIFG_TX_CTRL_ACK_S                                     7

// Field:     [6] TX_CTRL
//
// Interrupt flag 6. BLE mode: Transmitted LL control packet. Write zero to
// clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_CTRL                                  0x00000040
#define RFC_DBELL_RFCPEIFG_TX_CTRL_BITN                                      6
#define RFC_DBELL_RFCPEIFG_TX_CTRL_M                                0x00000040
#define RFC_DBELL_RFCPEIFG_TX_CTRL_S                                         6

// Field:     [5] TX_ACK
//
// Interrupt flag 5. BLE mode: Acknowledgement received on a transmitted
// packet. IEEE 802.15.4 mode: Transmitted automatic ACK frame. Write zero to
// clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_ACK                                   0x00000020
#define RFC_DBELL_RFCPEIFG_TX_ACK_BITN                                       5
#define RFC_DBELL_RFCPEIFG_TX_ACK_M                                 0x00000020
#define RFC_DBELL_RFCPEIFG_TX_ACK_S                                          5

// Field:     [4] TX_DONE
//
// Interrupt flag 4. Packet transmitted. (BLE mode: A packet has been
// transmitted.) (IEEE 802.15.4 mode: A frame has been transmitted). Write zero
// to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_TX_DONE                                  0x00000010
#define RFC_DBELL_RFCPEIFG_TX_DONE_BITN                                      4
#define RFC_DBELL_RFCPEIFG_TX_DONE_M                                0x00000010
#define RFC_DBELL_RFCPEIFG_TX_DONE_S                                         4

// Field:     [3] LAST_FG_COMMAND_DONE
//
// Interrupt flag 3. IEEE 802.15.4 mode only: The last foreground radio
// operation command in a chain of commands has finished. Write zero to clear
// flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_LAST_FG_COMMAND_DONE                     0x00000008
#define RFC_DBELL_RFCPEIFG_LAST_FG_COMMAND_DONE_BITN                         3
#define RFC_DBELL_RFCPEIFG_LAST_FG_COMMAND_DONE_M                   0x00000008
#define RFC_DBELL_RFCPEIFG_LAST_FG_COMMAND_DONE_S                            3

// Field:     [2] FG_COMMAND_DONE
//
// Interrupt flag 2. IEEE 802.15.4 mode only: A foreground radio operation
// command has finished. Write zero to clear flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_DONE                          0x00000004
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_DONE_BITN                              2
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_DONE_M                        0x00000004
#define RFC_DBELL_RFCPEIFG_FG_COMMAND_DONE_S                                 2

// Field:     [1] LAST_COMMAND_DONE
//
// Interrupt flag 1. The last radio operation command in a chain of commands
// has finished. (IEEE 802.15.4 mode: The last background level radio operation
// command in a chain of commands has finished.) Write zero to clear flag.
// Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE                        0x00000002
#define RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE_BITN                            1
#define RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE_M                      0x00000002
#define RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE_S                               1

// Field:     [0] COMMAND_DONE
//
// Interrupt flag 0. A radio operation has finished. (IEEE 802.15.4 mode: A
// background level radio operation command has finished.) Write zero to clear
// flag. Write to one has no effect.
#define RFC_DBELL_RFCPEIFG_COMMAND_DONE                             0x00000001
#define RFC_DBELL_RFCPEIFG_COMMAND_DONE_BITN                                 0
#define RFC_DBELL_RFCPEIFG_COMMAND_DONE_M                           0x00000001
#define RFC_DBELL_RFCPEIFG_COMMAND_DONE_S                                    0

//*****************************************************************************
//
// Register: RFC_DBELL_O_RFCPEIEN
//
//*****************************************************************************
// Field:    [31] INTERNAL_ERROR
//
// Interrupt enable for RFCPEIFG.INTERNAL_ERROR.
#define RFC_DBELL_RFCPEIEN_INTERNAL_ERROR                           0x80000000
#define RFC_DBELL_RFCPEIEN_INTERNAL_ERROR_BITN                              31
#define RFC_DBELL_RFCPEIEN_INTERNAL_ERROR_M                         0x80000000
#define RFC_DBELL_RFCPEIEN_INTERNAL_ERROR_S                                 31

// Field:    [30] BOOT_DONE
//
// Interrupt enable for RFCPEIFG.BOOT_DONE.
#define RFC_DBELL_RFCPEIEN_BOOT_DONE                                0x40000000
#define RFC_DBELL_RFCPEIEN_BOOT_DONE_BITN                                   30
#define RFC_DBELL_RFCPEIEN_BOOT_DONE_M                              0x40000000
#define RFC_DBELL_RFCPEIEN_BOOT_DONE_S                                      30

// Field:    [29] MODULES_UNLOCKED
//
// Interrupt enable for RFCPEIFG.MODULES_UNLOCKED.
#define RFC_DBELL_RFCPEIEN_MODULES_UNLOCKED                         0x20000000
#define RFC_DBELL_RFCPEIEN_MODULES_UNLOCKED_BITN                            29
#define RFC_DBELL_RFCPEIEN_MODULES_UNLOCKED_M                       0x20000000
#define RFC_DBELL_RFCPEIEN_MODULES_UNLOCKED_S                               29

// Field:    [28] SYNTH_NO_LOCK
//
// Interrupt enable for RFCPEIFG.SYNTH_NO_LOCK.
#define RFC_DBELL_RFCPEIEN_SYNTH_NO_LOCK                            0x10000000
#define RFC_DBELL_RFCPEIEN_SYNTH_NO_LOCK_BITN                               28
#define RFC_DBELL_RFCPEIEN_SYNTH_NO_LOCK_M                          0x10000000
#define RFC_DBELL_RFCPEIEN_SYNTH_NO_LOCK_S                                  28

// Field:    [27] IRQ27
//
// Interrupt enable for RFCPEIFG.IRQ27.
#define RFC_DBELL_RFCPEIEN_IRQ27                                    0x08000000
#define RFC_DBELL_RFCPEIEN_IRQ27_BITN                                       27
#define RFC_DBELL_RFCPEIEN_IRQ27_M                                  0x08000000
#define RFC_DBELL_RFCPEIEN_IRQ27_S                                          27

// Field:    [26] RX_ABORTED
//
// Interrupt enable for RFCPEIFG.RX_ABORTED.
#define RFC_DBELL_RFCPEIEN_RX_ABORTED                               0x04000000
#define RFC_DBELL_RFCPEIEN_RX_ABORTED_BITN                                  26
#define RFC_DBELL_RFCPEIEN_RX_ABORTED_M                             0x04000000
#define RFC_DBELL_RFCPEIEN_RX_ABORTED_S                                     26

// Field:    [25] RX_N_DATA_WRITTEN
//
// Interrupt enable for RFCPEIFG.RX_N_DATA_WRITTEN.
#define RFC_DBELL_RFCPEIEN_RX_N_DATA_WRITTEN                        0x02000000
#define RFC_DBELL_RFCPEIEN_RX_N_DATA_WRITTEN_BITN                           25
#define RFC_DBELL_RFCPEIEN_RX_N_DATA_WRITTEN_M                      0x02000000
#define RFC_DBELL_RFCPEIEN_RX_N_DATA_WRITTEN_S                              25

// Field:    [24] RX_DATA_WRITTEN
//
// Interrupt enable for RFCPEIFG.RX_DATA_WRITTEN.
#define RFC_DBELL_RFCPEIEN_RX_DATA_WRITTEN                          0x01000000
#define RFC_DBELL_RFCPEIEN_RX_DATA_WRITTEN_BITN                             24
#define RFC_DBELL_RFCPEIEN_RX_DATA_WRITTEN_M                        0x01000000
#define RFC_DBELL_RFCPEIEN_RX_DATA_WRITTEN_S                                24

// Field:    [23] RX_ENTRY_DONE
//
// Interrupt enable for RFCPEIFG.RX_ENTRY_DONE.
#define RFC_DBELL_RFCPEIEN_RX_ENTRY_DONE                            0x00800000
#define RFC_DBELL_RFCPEIEN_RX_ENTRY_DONE_BITN                               23
#define RFC_DBELL_RFCPEIEN_RX_ENTRY_DONE_M                          0x00800000
#define RFC_DBELL_RFCPEIEN_RX_ENTRY_DONE_S                                  23

// Field:    [22] RX_BUF_FULL
//
// Interrupt enable for RFCPEIFG.RX_BUF_FULL.
#define RFC_DBELL_RFCPEIEN_RX_BUF_FULL                              0x00400000
#define RFC_DBELL_RFCPEIEN_RX_BUF_FULL_BITN                                 22
#define RFC_DBELL_RFCPEIEN_RX_BUF_FULL_M                            0x00400000
#define RFC_DBELL_RFCPEIEN_RX_BUF_FULL_S                                    22

// Field:    [21] RX_CTRL_ACK
//
// Interrupt enable for RFCPEIFG.RX_CTRL_ACK.
#define RFC_DBELL_RFCPEIEN_RX_CTRL_ACK                              0x00200000
#define RFC_DBELL_RFCPEIEN_RX_CTRL_ACK_BITN                                 21
#define RFC_DBELL_RFCPEIEN_RX_CTRL_ACK_M                            0x00200000
#define RFC_DBELL_RFCPEIEN_RX_CTRL_ACK_S                                    21

// Field:    [20] RX_CTRL
//
// Interrupt enable for RFCPEIFG.RX_CTRL.
#define RFC_DBELL_RFCPEIEN_RX_CTRL                                  0x00100000
#define RFC_DBELL_RFCPEIEN_RX_CTRL_BITN                                     20
#define RFC_DBELL_RFCPEIEN_RX_CTRL_M                                0x00100000
#define RFC_DBELL_RFCPEIEN_RX_CTRL_S                                        20

// Field:    [19] RX_EMPTY
//
// Interrupt enable for RFCPEIFG.RX_EMPTY.
#define RFC_DBELL_RFCPEIEN_RX_EMPTY                                 0x00080000
#define RFC_DBELL_RFCPEIEN_RX_EMPTY_BITN                                    19
#define RFC_DBELL_RFCPEIEN_RX_EMPTY_M                               0x00080000
#define RFC_DBELL_RFCPEIEN_RX_EMPTY_S                                       19

// Field:    [18] RX_IGNORED
//
// Interrupt enable for RFCPEIFG.RX_IGNORED.
#define RFC_DBELL_RFCPEIEN_RX_IGNORED                               0x00040000
#define RFC_DBELL_RFCPEIEN_RX_IGNORED_BITN                                  18
#define RFC_DBELL_RFCPEIEN_RX_IGNORED_M                             0x00040000
#define RFC_DBELL_RFCPEIEN_RX_IGNORED_S                                     18

// Field:    [17] RX_NOK
//
// Interrupt enable for RFCPEIFG.RX_NOK.
#define RFC_DBELL_RFCPEIEN_RX_NOK                                   0x00020000
#define RFC_DBELL_RFCPEIEN_RX_NOK_BITN                                      17
#define RFC_DBELL_RFCPEIEN_RX_NOK_M                                 0x00020000
#define RFC_DBELL_RFCPEIEN_RX_NOK_S                                         17

// Field:    [16] RX_OK
//
// Interrupt enable for RFCPEIFG.RX_OK.
#define RFC_DBELL_RFCPEIEN_RX_OK                                    0x00010000
#define RFC_DBELL_RFCPEIEN_RX_OK_BITN                                       16
#define RFC_DBELL_RFCPEIEN_RX_OK_M                                  0x00010000
#define RFC_DBELL_RFCPEIEN_RX_OK_S                                          16

// Field:    [15] IRQ15
//
// Interrupt enable for RFCPEIFG.IRQ15.
#define RFC_DBELL_RFCPEIEN_IRQ15                                    0x00008000
#define RFC_DBELL_RFCPEIEN_IRQ15_BITN                                       15
#define RFC_DBELL_RFCPEIEN_IRQ15_M                                  0x00008000
#define RFC_DBELL_RFCPEIEN_IRQ15_S                                          15

// Field:    [14] IRQ14
//
// Interrupt enable for RFCPEIFG.IRQ14.
#define RFC_DBELL_RFCPEIEN_IRQ14                                    0x00004000
#define RFC_DBELL_RFCPEIEN_IRQ14_BITN                                       14
#define RFC_DBELL_RFCPEIEN_IRQ14_M                                  0x00004000
#define RFC_DBELL_RFCPEIEN_IRQ14_S                                          14

// Field:    [13] FG_COMMAND_STARTED
//
// Interrupt enable for RFCPEIFG.FG_COMMAND_STARTED.
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_STARTED                       0x00002000
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_STARTED_BITN                          13
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_STARTED_M                     0x00002000
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_STARTED_S                             13

// Field:    [12] COMMAND_STARTED
//
// Interrupt enable for RFCPEIFG.COMMAND_STARTED.
#define RFC_DBELL_RFCPEIEN_COMMAND_STARTED                          0x00001000
#define RFC_DBELL_RFCPEIEN_COMMAND_STARTED_BITN                             12
#define RFC_DBELL_RFCPEIEN_COMMAND_STARTED_M                        0x00001000
#define RFC_DBELL_RFCPEIEN_COMMAND_STARTED_S                                12

// Field:    [11] TX_BUFFER_CHANGED
//
// Interrupt enable for RFCPEIFG.TX_BUFFER_CHANGED.
#define RFC_DBELL_RFCPEIEN_TX_BUFFER_CHANGED                        0x00000800
#define RFC_DBELL_RFCPEIEN_TX_BUFFER_CHANGED_BITN                           11
#define RFC_DBELL_RFCPEIEN_TX_BUFFER_CHANGED_M                      0x00000800
#define RFC_DBELL_RFCPEIEN_TX_BUFFER_CHANGED_S                              11

// Field:    [10] TX_ENTRY_DONE
//
// Interrupt enable for RFCPEIFG.TX_ENTRY_DONE.
#define RFC_DBELL_RFCPEIEN_TX_ENTRY_DONE                            0x00000400
#define RFC_DBELL_RFCPEIEN_TX_ENTRY_DONE_BITN                               10
#define RFC_DBELL_RFCPEIEN_TX_ENTRY_DONE_M                          0x00000400
#define RFC_DBELL_RFCPEIEN_TX_ENTRY_DONE_S                                  10

// Field:     [9] TX_RETRANS
//
// Interrupt enable for RFCPEIFG.TX_RETRANS.
#define RFC_DBELL_RFCPEIEN_TX_RETRANS                               0x00000200
#define RFC_DBELL_RFCPEIEN_TX_RETRANS_BITN                                   9
#define RFC_DBELL_RFCPEIEN_TX_RETRANS_M                             0x00000200
#define RFC_DBELL_RFCPEIEN_TX_RETRANS_S                                      9

// Field:     [8] TX_CTRL_ACK_ACK
//
// Interrupt enable for RFCPEIFG.TX_CTRL_ACK_ACK.
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK_ACK                          0x00000100
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK_ACK_BITN                              8
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK_ACK_M                        0x00000100
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK_ACK_S                                 8

// Field:     [7] TX_CTRL_ACK
//
// Interrupt enable for RFCPEIFG.TX_CTRL_ACK.
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK                              0x00000080
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK_BITN                                  7
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK_M                            0x00000080
#define RFC_DBELL_RFCPEIEN_TX_CTRL_ACK_S                                     7

// Field:     [6] TX_CTRL
//
// Interrupt enable for RFCPEIFG.TX_CTRL.
#define RFC_DBELL_RFCPEIEN_TX_CTRL                                  0x00000040
#define RFC_DBELL_RFCPEIEN_TX_CTRL_BITN                                      6
#define RFC_DBELL_RFCPEIEN_TX_CTRL_M                                0x00000040
#define RFC_DBELL_RFCPEIEN_TX_CTRL_S                                         6

// Field:     [5] TX_ACK
//
// Interrupt enable for RFCPEIFG.TX_ACK.
#define RFC_DBELL_RFCPEIEN_TX_ACK                                   0x00000020
#define RFC_DBELL_RFCPEIEN_TX_ACK_BITN                                       5
#define RFC_DBELL_RFCPEIEN_TX_ACK_M                                 0x00000020
#define RFC_DBELL_RFCPEIEN_TX_ACK_S                                          5

// Field:     [4] TX_DONE
//
// Interrupt enable for RFCPEIFG.TX_DONE.
#define RFC_DBELL_RFCPEIEN_TX_DONE                                  0x00000010
#define RFC_DBELL_RFCPEIEN_TX_DONE_BITN                                      4
#define RFC_DBELL_RFCPEIEN_TX_DONE_M                                0x00000010
#define RFC_DBELL_RFCPEIEN_TX_DONE_S                                         4

// Field:     [3] LAST_FG_COMMAND_DONE
//
// Interrupt enable for RFCPEIFG.LAST_FG_COMMAND_DONE.
#define RFC_DBELL_RFCPEIEN_LAST_FG_COMMAND_DONE                     0x00000008
#define RFC_DBELL_RFCPEIEN_LAST_FG_COMMAND_DONE_BITN                         3
#define RFC_DBELL_RFCPEIEN_LAST_FG_COMMAND_DONE_M                   0x00000008
#define RFC_DBELL_RFCPEIEN_LAST_FG_COMMAND_DONE_S                            3

// Field:     [2] FG_COMMAND_DONE
//
// Interrupt enable for RFCPEIFG.FG_COMMAND_DONE.
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_DONE                          0x00000004
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_DONE_BITN                              2
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_DONE_M                        0x00000004
#define RFC_DBELL_RFCPEIEN_FG_COMMAND_DONE_S                                 2

// Field:     [1] LAST_COMMAND_DONE
//
// Interrupt enable for RFCPEIFG.LAST_COMMAND_DONE.
#define RFC_DBELL_RFCPEIEN_LAST_COMMAND_DONE                        0x00000002
#define RFC_DBELL_RFCPEIEN_LAST_COMMAND_DONE_BITN                            1
#define RFC_DBELL_RFCPEIEN_LAST_COMMAND_DONE_M                      0x00000002
#define RFC_DBELL_RFCPEIEN_LAST_COMMAND_DONE_S                               1

// Field:     [0] COMMAND_DONE
//
// Interrupt enable for RFCPEIFG.COMMAND_DONE.
#define RFC_DBELL_RFCPEIEN_COMMAND_DONE                             0x00000001
#define RFC_DBELL_RFCPEIEN_COMMAND_DONE_BITN                                 0
#define RFC_DBELL_RFCPEIEN_COMMAND_DONE_M                           0x00000001
#define RFC_DBELL_RFCPEIEN_COMMAND_DONE_S                                    0

//*****************************************************************************
//
// Register: RFC_DBELL_O_RFCPEISL
//
//*****************************************************************************
// Field:    [31] INTERNAL_ERROR
//
// Select which CPU interrupt vector the RFCPEIFG.INTERNAL_ERROR interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_INTERNAL_ERROR                           0x80000000
#define RFC_DBELL_RFCPEISL_INTERNAL_ERROR_BITN                              31
#define RFC_DBELL_RFCPEISL_INTERNAL_ERROR_M                         0x80000000
#define RFC_DBELL_RFCPEISL_INTERNAL_ERROR_S                                 31
#define RFC_DBELL_RFCPEISL_INTERNAL_ERROR_CPE1                      0x80000000
#define RFC_DBELL_RFCPEISL_INTERNAL_ERROR_CPE0                      0x00000000

// Field:    [30] BOOT_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.BOOT_DONE interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_BOOT_DONE                                0x40000000
#define RFC_DBELL_RFCPEISL_BOOT_DONE_BITN                                   30
#define RFC_DBELL_RFCPEISL_BOOT_DONE_M                              0x40000000
#define RFC_DBELL_RFCPEISL_BOOT_DONE_S                                      30
#define RFC_DBELL_RFCPEISL_BOOT_DONE_CPE1                           0x40000000
#define RFC_DBELL_RFCPEISL_BOOT_DONE_CPE0                           0x00000000

// Field:    [29] MODULES_UNLOCKED
//
// Select which CPU interrupt vector the RFCPEIFG.MODULES_UNLOCKED interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_MODULES_UNLOCKED                         0x20000000
#define RFC_DBELL_RFCPEISL_MODULES_UNLOCKED_BITN                            29
#define RFC_DBELL_RFCPEISL_MODULES_UNLOCKED_M                       0x20000000
#define RFC_DBELL_RFCPEISL_MODULES_UNLOCKED_S                               29
#define RFC_DBELL_RFCPEISL_MODULES_UNLOCKED_CPE1                    0x20000000
#define RFC_DBELL_RFCPEISL_MODULES_UNLOCKED_CPE0                    0x00000000

// Field:    [28] SYNTH_NO_LOCK
//
// Select which CPU interrupt vector the RFCPEIFG.SYNTH_NO_LOCK interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_SYNTH_NO_LOCK                            0x10000000
#define RFC_DBELL_RFCPEISL_SYNTH_NO_LOCK_BITN                               28
#define RFC_DBELL_RFCPEISL_SYNTH_NO_LOCK_M                          0x10000000
#define RFC_DBELL_RFCPEISL_SYNTH_NO_LOCK_S                                  28
#define RFC_DBELL_RFCPEISL_SYNTH_NO_LOCK_CPE1                       0x10000000
#define RFC_DBELL_RFCPEISL_SYNTH_NO_LOCK_CPE0                       0x00000000

// Field:    [27] IRQ27
//
// Select which CPU interrupt vector the RFCPEIFG.IRQ27 interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_IRQ27                                    0x08000000
#define RFC_DBELL_RFCPEISL_IRQ27_BITN                                       27
#define RFC_DBELL_RFCPEISL_IRQ27_M                                  0x08000000
#define RFC_DBELL_RFCPEISL_IRQ27_S                                          27
#define RFC_DBELL_RFCPEISL_IRQ27_CPE1                               0x08000000
#define RFC_DBELL_RFCPEISL_IRQ27_CPE0                               0x00000000

// Field:    [26] RX_ABORTED
//
// Select which CPU interrupt vector the RFCPEIFG.RX_ABORTED interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_ABORTED                               0x04000000
#define RFC_DBELL_RFCPEISL_RX_ABORTED_BITN                                  26
#define RFC_DBELL_RFCPEISL_RX_ABORTED_M                             0x04000000
#define RFC_DBELL_RFCPEISL_RX_ABORTED_S                                     26
#define RFC_DBELL_RFCPEISL_RX_ABORTED_CPE1                          0x04000000
#define RFC_DBELL_RFCPEISL_RX_ABORTED_CPE0                          0x00000000

// Field:    [25] RX_N_DATA_WRITTEN
//
// Select which CPU interrupt vector the RFCPEIFG.RX_N_DATA_WRITTEN interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_N_DATA_WRITTEN                        0x02000000
#define RFC_DBELL_RFCPEISL_RX_N_DATA_WRITTEN_BITN                           25
#define RFC_DBELL_RFCPEISL_RX_N_DATA_WRITTEN_M                      0x02000000
#define RFC_DBELL_RFCPEISL_RX_N_DATA_WRITTEN_S                              25
#define RFC_DBELL_RFCPEISL_RX_N_DATA_WRITTEN_CPE1                   0x02000000
#define RFC_DBELL_RFCPEISL_RX_N_DATA_WRITTEN_CPE0                   0x00000000

// Field:    [24] RX_DATA_WRITTEN
//
// Select which CPU interrupt vector the RFCPEIFG.RX_DATA_WRITTEN interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_DATA_WRITTEN                          0x01000000
#define RFC_DBELL_RFCPEISL_RX_DATA_WRITTEN_BITN                             24
#define RFC_DBELL_RFCPEISL_RX_DATA_WRITTEN_M                        0x01000000
#define RFC_DBELL_RFCPEISL_RX_DATA_WRITTEN_S                                24
#define RFC_DBELL_RFCPEISL_RX_DATA_WRITTEN_CPE1                     0x01000000
#define RFC_DBELL_RFCPEISL_RX_DATA_WRITTEN_CPE0                     0x00000000

// Field:    [23] RX_ENTRY_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.RX_ENTRY_DONE interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_ENTRY_DONE                            0x00800000
#define RFC_DBELL_RFCPEISL_RX_ENTRY_DONE_BITN                               23
#define RFC_DBELL_RFCPEISL_RX_ENTRY_DONE_M                          0x00800000
#define RFC_DBELL_RFCPEISL_RX_ENTRY_DONE_S                                  23
#define RFC_DBELL_RFCPEISL_RX_ENTRY_DONE_CPE1                       0x00800000
#define RFC_DBELL_RFCPEISL_RX_ENTRY_DONE_CPE0                       0x00000000

// Field:    [22] RX_BUF_FULL
//
// Select which CPU interrupt vector the RFCPEIFG.RX_BUF_FULL interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_BUF_FULL                              0x00400000
#define RFC_DBELL_RFCPEISL_RX_BUF_FULL_BITN                                 22
#define RFC_DBELL_RFCPEISL_RX_BUF_FULL_M                            0x00400000
#define RFC_DBELL_RFCPEISL_RX_BUF_FULL_S                                    22
#define RFC_DBELL_RFCPEISL_RX_BUF_FULL_CPE1                         0x00400000
#define RFC_DBELL_RFCPEISL_RX_BUF_FULL_CPE0                         0x00000000

// Field:    [21] RX_CTRL_ACK
//
// Select which CPU interrupt vector the RFCPEIFG.RX_CTRL_ACK interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_CTRL_ACK                              0x00200000
#define RFC_DBELL_RFCPEISL_RX_CTRL_ACK_BITN                                 21
#define RFC_DBELL_RFCPEISL_RX_CTRL_ACK_M                            0x00200000
#define RFC_DBELL_RFCPEISL_RX_CTRL_ACK_S                                    21
#define RFC_DBELL_RFCPEISL_RX_CTRL_ACK_CPE1                         0x00200000
#define RFC_DBELL_RFCPEISL_RX_CTRL_ACK_CPE0                         0x00000000

// Field:    [20] RX_CTRL
//
// Select which CPU interrupt vector the RFCPEIFG.RX_CTRL interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_CTRL                                  0x00100000
#define RFC_DBELL_RFCPEISL_RX_CTRL_BITN                                     20
#define RFC_DBELL_RFCPEISL_RX_CTRL_M                                0x00100000
#define RFC_DBELL_RFCPEISL_RX_CTRL_S                                        20
#define RFC_DBELL_RFCPEISL_RX_CTRL_CPE1                             0x00100000
#define RFC_DBELL_RFCPEISL_RX_CTRL_CPE0                             0x00000000

// Field:    [19] RX_EMPTY
//
// Select which CPU interrupt vector the RFCPEIFG.RX_EMPTY interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_EMPTY                                 0x00080000
#define RFC_DBELL_RFCPEISL_RX_EMPTY_BITN                                    19
#define RFC_DBELL_RFCPEISL_RX_EMPTY_M                               0x00080000
#define RFC_DBELL_RFCPEISL_RX_EMPTY_S                                       19
#define RFC_DBELL_RFCPEISL_RX_EMPTY_CPE1                            0x00080000
#define RFC_DBELL_RFCPEISL_RX_EMPTY_CPE0                            0x00000000

// Field:    [18] RX_IGNORED
//
// Select which CPU interrupt vector the RFCPEIFG.RX_IGNORED interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_IGNORED                               0x00040000
#define RFC_DBELL_RFCPEISL_RX_IGNORED_BITN                                  18
#define RFC_DBELL_RFCPEISL_RX_IGNORED_M                             0x00040000
#define RFC_DBELL_RFCPEISL_RX_IGNORED_S                                     18
#define RFC_DBELL_RFCPEISL_RX_IGNORED_CPE1                          0x00040000
#define RFC_DBELL_RFCPEISL_RX_IGNORED_CPE0                          0x00000000

// Field:    [17] RX_NOK
//
// Select which CPU interrupt vector the RFCPEIFG.RX_NOK interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_NOK                                   0x00020000
#define RFC_DBELL_RFCPEISL_RX_NOK_BITN                                      17
#define RFC_DBELL_RFCPEISL_RX_NOK_M                                 0x00020000
#define RFC_DBELL_RFCPEISL_RX_NOK_S                                         17
#define RFC_DBELL_RFCPEISL_RX_NOK_CPE1                              0x00020000
#define RFC_DBELL_RFCPEISL_RX_NOK_CPE0                              0x00000000

// Field:    [16] RX_OK
//
// Select which CPU interrupt vector the RFCPEIFG.RX_OK interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_RX_OK                                    0x00010000
#define RFC_DBELL_RFCPEISL_RX_OK_BITN                                       16
#define RFC_DBELL_RFCPEISL_RX_OK_M                                  0x00010000
#define RFC_DBELL_RFCPEISL_RX_OK_S                                          16
#define RFC_DBELL_RFCPEISL_RX_OK_CPE1                               0x00010000
#define RFC_DBELL_RFCPEISL_RX_OK_CPE0                               0x00000000

// Field:    [15] IRQ15
//
// Select which CPU interrupt vector the RFCPEIFG.IRQ15 interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_IRQ15                                    0x00008000
#define RFC_DBELL_RFCPEISL_IRQ15_BITN                                       15
#define RFC_DBELL_RFCPEISL_IRQ15_M                                  0x00008000
#define RFC_DBELL_RFCPEISL_IRQ15_S                                          15
#define RFC_DBELL_RFCPEISL_IRQ15_CPE1                               0x00008000
#define RFC_DBELL_RFCPEISL_IRQ15_CPE0                               0x00000000

// Field:    [14] IRQ14
//
// Select which CPU interrupt vector the RFCPEIFG.IRQ14 interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_IRQ14                                    0x00004000
#define RFC_DBELL_RFCPEISL_IRQ14_BITN                                       14
#define RFC_DBELL_RFCPEISL_IRQ14_M                                  0x00004000
#define RFC_DBELL_RFCPEISL_IRQ14_S                                          14
#define RFC_DBELL_RFCPEISL_IRQ14_CPE1                               0x00004000
#define RFC_DBELL_RFCPEISL_IRQ14_CPE0                               0x00000000

// Field:    [13] FG_COMMAND_STARTED
//
// Select which CPU interrupt vector the RFCPEIFG.FG_COMMAND_STARTED interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_FG_COMMAND_STARTED                       0x00002000
#define RFC_DBELL_RFCPEISL_FG_COMMAND_STARTED_BITN                          13
#define RFC_DBELL_RFCPEISL_FG_COMMAND_STARTED_M                     0x00002000
#define RFC_DBELL_RFCPEISL_FG_COMMAND_STARTED_S                             13
#define RFC_DBELL_RFCPEISL_FG_COMMAND_STARTED_CPE1                  0x00002000
#define RFC_DBELL_RFCPEISL_FG_COMMAND_STARTED_CPE0                  0x00000000

// Field:    [12] COMMAND_STARTED
//
// Select which CPU interrupt vector the RFCPEIFG.COMMAND_STARTED interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_COMMAND_STARTED                          0x00001000
#define RFC_DBELL_RFCPEISL_COMMAND_STARTED_BITN                             12
#define RFC_DBELL_RFCPEISL_COMMAND_STARTED_M                        0x00001000
#define RFC_DBELL_RFCPEISL_COMMAND_STARTED_S                                12
#define RFC_DBELL_RFCPEISL_COMMAND_STARTED_CPE1                     0x00001000
#define RFC_DBELL_RFCPEISL_COMMAND_STARTED_CPE0                     0x00000000

// Field:    [11] TX_BUFFER_CHANGED
//
// Select which CPU interrupt vector the RFCPEIFG.TX_BUFFER_CHANGED interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_BUFFER_CHANGED                        0x00000800
#define RFC_DBELL_RFCPEISL_TX_BUFFER_CHANGED_BITN                           11
#define RFC_DBELL_RFCPEISL_TX_BUFFER_CHANGED_M                      0x00000800
#define RFC_DBELL_RFCPEISL_TX_BUFFER_CHANGED_S                              11
#define RFC_DBELL_RFCPEISL_TX_BUFFER_CHANGED_CPE1                   0x00000800
#define RFC_DBELL_RFCPEISL_TX_BUFFER_CHANGED_CPE0                   0x00000000

// Field:    [10] TX_ENTRY_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.TX_ENTRY_DONE interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_ENTRY_DONE                            0x00000400
#define RFC_DBELL_RFCPEISL_TX_ENTRY_DONE_BITN                               10
#define RFC_DBELL_RFCPEISL_TX_ENTRY_DONE_M                          0x00000400
#define RFC_DBELL_RFCPEISL_TX_ENTRY_DONE_S                                  10
#define RFC_DBELL_RFCPEISL_TX_ENTRY_DONE_CPE1                       0x00000400
#define RFC_DBELL_RFCPEISL_TX_ENTRY_DONE_CPE0                       0x00000000

// Field:     [9] TX_RETRANS
//
// Select which CPU interrupt vector the RFCPEIFG.TX_RETRANS interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_RETRANS                               0x00000200
#define RFC_DBELL_RFCPEISL_TX_RETRANS_BITN                                   9
#define RFC_DBELL_RFCPEISL_TX_RETRANS_M                             0x00000200
#define RFC_DBELL_RFCPEISL_TX_RETRANS_S                                      9
#define RFC_DBELL_RFCPEISL_TX_RETRANS_CPE1                          0x00000200
#define RFC_DBELL_RFCPEISL_TX_RETRANS_CPE0                          0x00000000

// Field:     [8] TX_CTRL_ACK_ACK
//
// Select which CPU interrupt vector the RFCPEIFG.TX_CTRL_ACK_ACK interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_ACK                          0x00000100
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_ACK_BITN                              8
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_ACK_M                        0x00000100
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_ACK_S                                 8
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_ACK_CPE1                     0x00000100
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_ACK_CPE0                     0x00000000

// Field:     [7] TX_CTRL_ACK
//
// Select which CPU interrupt vector the RFCPEIFG.TX_CTRL_ACK interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK                              0x00000080
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_BITN                                  7
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_M                            0x00000080
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_S                                     7
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_CPE1                         0x00000080
#define RFC_DBELL_RFCPEISL_TX_CTRL_ACK_CPE0                         0x00000000

// Field:     [6] TX_CTRL
//
// Select which CPU interrupt vector the RFCPEIFG.TX_CTRL interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_CTRL                                  0x00000040
#define RFC_DBELL_RFCPEISL_TX_CTRL_BITN                                      6
#define RFC_DBELL_RFCPEISL_TX_CTRL_M                                0x00000040
#define RFC_DBELL_RFCPEISL_TX_CTRL_S                                         6
#define RFC_DBELL_RFCPEISL_TX_CTRL_CPE1                             0x00000040
#define RFC_DBELL_RFCPEISL_TX_CTRL_CPE0                             0x00000000

// Field:     [5] TX_ACK
//
// Select which CPU interrupt vector the RFCPEIFG.TX_ACK interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_ACK                                   0x00000020
#define RFC_DBELL_RFCPEISL_TX_ACK_BITN                                       5
#define RFC_DBELL_RFCPEISL_TX_ACK_M                                 0x00000020
#define RFC_DBELL_RFCPEISL_TX_ACK_S                                          5
#define RFC_DBELL_RFCPEISL_TX_ACK_CPE1                              0x00000020
#define RFC_DBELL_RFCPEISL_TX_ACK_CPE0                              0x00000000

// Field:     [4] TX_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.TX_DONE interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_TX_DONE                                  0x00000010
#define RFC_DBELL_RFCPEISL_TX_DONE_BITN                                      4
#define RFC_DBELL_RFCPEISL_TX_DONE_M                                0x00000010
#define RFC_DBELL_RFCPEISL_TX_DONE_S                                         4
#define RFC_DBELL_RFCPEISL_TX_DONE_CPE1                             0x00000010
#define RFC_DBELL_RFCPEISL_TX_DONE_CPE0                             0x00000000

// Field:     [3] LAST_FG_COMMAND_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.LAST_FG_COMMAND_DONE
// interrupt should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_LAST_FG_COMMAND_DONE                     0x00000008
#define RFC_DBELL_RFCPEISL_LAST_FG_COMMAND_DONE_BITN                         3
#define RFC_DBELL_RFCPEISL_LAST_FG_COMMAND_DONE_M                   0x00000008
#define RFC_DBELL_RFCPEISL_LAST_FG_COMMAND_DONE_S                            3
#define RFC_DBELL_RFCPEISL_LAST_FG_COMMAND_DONE_CPE1                0x00000008
#define RFC_DBELL_RFCPEISL_LAST_FG_COMMAND_DONE_CPE0                0x00000000

// Field:     [2] FG_COMMAND_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.FG_COMMAND_DONE interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_FG_COMMAND_DONE                          0x00000004
#define RFC_DBELL_RFCPEISL_FG_COMMAND_DONE_BITN                              2
#define RFC_DBELL_RFCPEISL_FG_COMMAND_DONE_M                        0x00000004
#define RFC_DBELL_RFCPEISL_FG_COMMAND_DONE_S                                 2
#define RFC_DBELL_RFCPEISL_FG_COMMAND_DONE_CPE1                     0x00000004
#define RFC_DBELL_RFCPEISL_FG_COMMAND_DONE_CPE0                     0x00000000

// Field:     [1] LAST_COMMAND_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.LAST_COMMAND_DONE interrupt
// should use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE                        0x00000002
#define RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE_BITN                            1
#define RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE_M                      0x00000002
#define RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE_S                               1
#define RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE_CPE1                   0x00000002
#define RFC_DBELL_RFCPEISL_LAST_COMMAND_DONE_CPE0                   0x00000000

// Field:     [0] COMMAND_DONE
//
// Select which CPU interrupt vector the RFCPEIFG.COMMAND_DONE interrupt should
// use.
// ENUMs:
// CPE1                     Associate this interrupt line with INT_RF_CPE1
//                          interrupt vector
// CPE0                     Associate this interrupt line with INT_RF_CPE0
//                          interrupt vector
#define RFC_DBELL_RFCPEISL_COMMAND_DONE                             0x00000001
#define RFC_DBELL_RFCPEISL_COMMAND_DONE_BITN                                 0
#define RFC_DBELL_RFCPEISL_COMMAND_DONE_M                           0x00000001
#define RFC_DBELL_RFCPEISL_COMMAND_DONE_S                                    0
#define RFC_DBELL_RFCPEISL_COMMAND_DONE_CPE1                        0x00000001
#define RFC_DBELL_RFCPEISL_COMMAND_DONE_CPE0                        0x00000000

//*****************************************************************************
//
// Register: RFC_DBELL_O_RFACKIFG
//
//*****************************************************************************
// Field:     [0] ACKFLAG
//
// Interrupt flag for Command ACK
#define RFC_DBELL_RFACKIFG_ACKFLAG                                  0x00000001
#define RFC_DBELL_RFACKIFG_ACKFLAG_BITN                                      0
#define RFC_DBELL_RFACKIFG_ACKFLAG_M                                0x00000001
#define RFC_DBELL_RFACKIFG_ACKFLAG_S                                         0

//*****************************************************************************
//
// Register: RFC_DBELL_O_SYSGPOCTL
//
//*****************************************************************************
// Field: [15:12] GPOCTL3
//
// RF Core GPO control bit 3. Selects which signal to output on the RF Core GPO
// line 3.
// ENUMs:
// RATGPO3                  RAT GPO line 3
// RATGPO2                  RAT GPO line 2
// RATGPO1                  RAT GPO line 1
// RATGPO0                  RAT GPO line 0
// RFEGPO3                  RFE GPO line 3
// RFEGPO2                  RFE GPO line 2
// RFEGPO1                  RFE GPO line 1
// RFEGPO0                  RFE GPO line 0
// MCEGPO3                  MCE GPO line 3
// MCEGPO2                  MCE GPO line 2
// MCEGPO1                  MCE GPO line 1
// MCEGPO0                  MCE GPO line 0
// CPEGPO3                  CPE GPO line 3
// CPEGPO2                  CPE GPO line 2
// CPEGPO1                  CPE GPO line 1
// CPEGPO0                  CPE GPO line 0
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_W                                        4
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_M                               0x0000F000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_S                                       12
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO3                         0x0000F000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO2                         0x0000E000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO1                         0x0000D000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO0                         0x0000C000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RFEGPO3                         0x0000B000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RFEGPO2                         0x0000A000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RFEGPO1                         0x00009000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_RFEGPO0                         0x00008000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_MCEGPO3                         0x00007000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_MCEGPO2                         0x00006000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_MCEGPO1                         0x00005000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_MCEGPO0                         0x00004000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_CPEGPO3                         0x00003000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_CPEGPO2                         0x00002000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_CPEGPO1                         0x00001000
#define RFC_DBELL_SYSGPOCTL_GPOCTL3_CPEGPO0                         0x00000000

// Field:  [11:8] GPOCTL2
//
// RF Core GPO control bit 2. Selects which signal to output on the RF Core GPO
// line 2.
// ENUMs:
// RATGPO3                  RAT GPO line 3
// RATGPO2                  RAT GPO line 2
// RATGPO1                  RAT GPO line 1
// RATGPO0                  RAT GPO line 0
// RFEGPO3                  RFE GPO line 3
// RFEGPO2                  RFE GPO line 2
// RFEGPO1                  RFE GPO line 1
// RFEGPO0                  RFE GPO line 0
// MCEGPO3                  MCE GPO line 3
// MCEGPO2                  MCE GPO line 2
// MCEGPO1                  MCE GPO line 1
// MCEGPO0                  MCE GPO line 0
// CPEGPO3                  CPE GPO line 3
// CPEGPO2                  CPE GPO line 2
// CPEGPO1                  CPE GPO line 1
// CPEGPO0                  CPE GPO line 0
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_W                                        4
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_M                               0x00000F00
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_S                                        8
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RATGPO3                         0x00000F00
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RATGPO2                         0x00000E00
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RATGPO1                         0x00000D00
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RATGPO0                         0x00000C00
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RFEGPO3                         0x00000B00
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RFEGPO2                         0x00000A00
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RFEGPO1                         0x00000900
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_RFEGPO0                         0x00000800
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO3                         0x00000700
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO2                         0x00000600
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO1                         0x00000500
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO0                         0x00000400
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_CPEGPO3                         0x00000300
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_CPEGPO2                         0x00000200
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_CPEGPO1                         0x00000100
#define RFC_DBELL_SYSGPOCTL_GPOCTL2_CPEGPO0                         0x00000000

// Field:   [7:4] GPOCTL1
//
// RF Core GPO control bit 1. Selects which signal to output on the RF Core GPO
// line 1.
// ENUMs:
// RATGPO3                  RAT GPO line 3
// RATGPO2                  RAT GPO line 2
// RATGPO1                  RAT GPO line 1
// RATGPO0                  RAT GPO line 0
// RFEGPO3                  RFE GPO line 3
// RFEGPO2                  RFE GPO line 2
// RFEGPO1                  RFE GPO line 1
// RFEGPO0                  RFE GPO line 0
// MCEGPO3                  MCE GPO line 3
// MCEGPO2                  MCE GPO line 2
// MCEGPO1                  MCE GPO line 1
// MCEGPO0                  MCE GPO line 0
// CPEGPO3                  CPE GPO line 3
// CPEGPO2                  CPE GPO line 2
// CPEGPO1                  CPE GPO line 1
// CPEGPO0                  CPE GPO line 0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_W                                        4
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_M                               0x000000F0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_S                                        4
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO3                         0x000000F0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO2                         0x000000E0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO1                         0x000000D0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO0                         0x000000C0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RFEGPO3                         0x000000B0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RFEGPO2                         0x000000A0
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RFEGPO1                         0x00000090
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_RFEGPO0                         0x00000080
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_MCEGPO3                         0x00000070
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_MCEGPO2                         0x00000060
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_MCEGPO1                         0x00000050
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_MCEGPO0                         0x00000040
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_CPEGPO3                         0x00000030
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_CPEGPO2                         0x00000020
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_CPEGPO1                         0x00000010
#define RFC_DBELL_SYSGPOCTL_GPOCTL1_CPEGPO0                         0x00000000

// Field:   [3:0] GPOCTL0
//
// RF Core GPO control bit 0. Selects which signal to output on the RF Core GPO
// line 0.
// ENUMs:
// RATGPO3                  RAT GPO line 3
// RATGPO2                  RAT GPO line 2
// RATGPO1                  RAT GPO line 1
// RATGPO0                  RAT GPO line 0
// RFEGPO3                  RFE GPO line 3
// RFEGPO2                  RFE GPO line 2
// RFEGPO1                  RFE GPO line 1
// RFEGPO0                  RFE GPO line 0
// MCEGPO3                  MCE GPO line 3
// MCEGPO2                  MCE GPO line 2
// MCEGPO1                  MCE GPO line 1
// MCEGPO0                  MCE GPO line 0
// CPEGPO3                  CPE GPO line 3
// CPEGPO2                  CPE GPO line 2
// CPEGPO1                  CPE GPO line 1
// CPEGPO0                  CPE GPO line 0
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_W                                        4
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_M                               0x0000000F
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_S                                        0
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RATGPO3                         0x0000000F
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RATGPO2                         0x0000000E
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RATGPO1                         0x0000000D
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RATGPO0                         0x0000000C
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RFEGPO3                         0x0000000B
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RFEGPO2                         0x0000000A
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RFEGPO1                         0x00000009
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_RFEGPO0                         0x00000008
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_MCEGPO3                         0x00000007
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_MCEGPO2                         0x00000006
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_MCEGPO1                         0x00000005
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_MCEGPO0                         0x00000004
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO3                         0x00000003
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO2                         0x00000002
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO1                         0x00000001
#define RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO0                         0x00000000


#endif // __RFC_DBELL__
