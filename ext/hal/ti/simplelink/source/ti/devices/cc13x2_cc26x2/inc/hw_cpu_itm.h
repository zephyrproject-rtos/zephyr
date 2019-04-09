/******************************************************************************
*  Filename:       hw_cpu_itm_h
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

#ifndef __HW_CPU_ITM_H__
#define __HW_CPU_ITM_H__

//*****************************************************************************
//
// This section defines the register offsets of
// CPU_ITM component
//
//*****************************************************************************
// Stimulus Port 0
#define CPU_ITM_O_STIM0                                             0x00000000

// Stimulus Port 1
#define CPU_ITM_O_STIM1                                             0x00000004

// Stimulus Port 2
#define CPU_ITM_O_STIM2                                             0x00000008

// Stimulus Port 3
#define CPU_ITM_O_STIM3                                             0x0000000C

// Stimulus Port 4
#define CPU_ITM_O_STIM4                                             0x00000010

// Stimulus Port 5
#define CPU_ITM_O_STIM5                                             0x00000014

// Stimulus Port 6
#define CPU_ITM_O_STIM6                                             0x00000018

// Stimulus Port 7
#define CPU_ITM_O_STIM7                                             0x0000001C

// Stimulus Port 8
#define CPU_ITM_O_STIM8                                             0x00000020

// Stimulus Port 9
#define CPU_ITM_O_STIM9                                             0x00000024

// Stimulus Port 10
#define CPU_ITM_O_STIM10                                            0x00000028

// Stimulus Port 11
#define CPU_ITM_O_STIM11                                            0x0000002C

// Stimulus Port 12
#define CPU_ITM_O_STIM12                                            0x00000030

// Stimulus Port 13
#define CPU_ITM_O_STIM13                                            0x00000034

// Stimulus Port 14
#define CPU_ITM_O_STIM14                                            0x00000038

// Stimulus Port 15
#define CPU_ITM_O_STIM15                                            0x0000003C

// Stimulus Port 16
#define CPU_ITM_O_STIM16                                            0x00000040

// Stimulus Port 17
#define CPU_ITM_O_STIM17                                            0x00000044

// Stimulus Port 18
#define CPU_ITM_O_STIM18                                            0x00000048

// Stimulus Port 19
#define CPU_ITM_O_STIM19                                            0x0000004C

// Stimulus Port 20
#define CPU_ITM_O_STIM20                                            0x00000050

// Stimulus Port 21
#define CPU_ITM_O_STIM21                                            0x00000054

// Stimulus Port 22
#define CPU_ITM_O_STIM22                                            0x00000058

// Stimulus Port 23
#define CPU_ITM_O_STIM23                                            0x0000005C

// Stimulus Port 24
#define CPU_ITM_O_STIM24                                            0x00000060

// Stimulus Port 25
#define CPU_ITM_O_STIM25                                            0x00000064

// Stimulus Port 26
#define CPU_ITM_O_STIM26                                            0x00000068

// Stimulus Port 27
#define CPU_ITM_O_STIM27                                            0x0000006C

// Stimulus Port 28
#define CPU_ITM_O_STIM28                                            0x00000070

// Stimulus Port 29
#define CPU_ITM_O_STIM29                                            0x00000074

// Stimulus Port 30
#define CPU_ITM_O_STIM30                                            0x00000078

// Stimulus Port 31
#define CPU_ITM_O_STIM31                                            0x0000007C

// Trace Enable
#define CPU_ITM_O_TER                                               0x00000E00

// Trace Privilege
#define CPU_ITM_O_TPR                                               0x00000E40

// Trace Control
#define CPU_ITM_O_TCR                                               0x00000E80

// Lock Access
#define CPU_ITM_O_LAR                                               0x00000FB0

// Lock Status
#define CPU_ITM_O_LSR                                               0x00000FB4

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM0
//
//*****************************************************************************
// Field:  [31:0] STIM0
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA0 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM0_STIM0_W                                               32
#define CPU_ITM_STIM0_STIM0_M                                       0xFFFFFFFF
#define CPU_ITM_STIM0_STIM0_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM1
//
//*****************************************************************************
// Field:  [31:0] STIM1
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA1 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM1_STIM1_W                                               32
#define CPU_ITM_STIM1_STIM1_M                                       0xFFFFFFFF
#define CPU_ITM_STIM1_STIM1_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM2
//
//*****************************************************************************
// Field:  [31:0] STIM2
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA2 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM2_STIM2_W                                               32
#define CPU_ITM_STIM2_STIM2_M                                       0xFFFFFFFF
#define CPU_ITM_STIM2_STIM2_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM3
//
//*****************************************************************************
// Field:  [31:0] STIM3
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA3 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM3_STIM3_W                                               32
#define CPU_ITM_STIM3_STIM3_M                                       0xFFFFFFFF
#define CPU_ITM_STIM3_STIM3_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM4
//
//*****************************************************************************
// Field:  [31:0] STIM4
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA4 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM4_STIM4_W                                               32
#define CPU_ITM_STIM4_STIM4_M                                       0xFFFFFFFF
#define CPU_ITM_STIM4_STIM4_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM5
//
//*****************************************************************************
// Field:  [31:0] STIM5
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA5 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM5_STIM5_W                                               32
#define CPU_ITM_STIM5_STIM5_M                                       0xFFFFFFFF
#define CPU_ITM_STIM5_STIM5_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM6
//
//*****************************************************************************
// Field:  [31:0] STIM6
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA6 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM6_STIM6_W                                               32
#define CPU_ITM_STIM6_STIM6_M                                       0xFFFFFFFF
#define CPU_ITM_STIM6_STIM6_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM7
//
//*****************************************************************************
// Field:  [31:0] STIM7
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA7 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM7_STIM7_W                                               32
#define CPU_ITM_STIM7_STIM7_M                                       0xFFFFFFFF
#define CPU_ITM_STIM7_STIM7_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM8
//
//*****************************************************************************
// Field:  [31:0] STIM8
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA8 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM8_STIM8_W                                               32
#define CPU_ITM_STIM8_STIM8_M                                       0xFFFFFFFF
#define CPU_ITM_STIM8_STIM8_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM9
//
//*****************************************************************************
// Field:  [31:0] STIM9
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA9 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM9_STIM9_W                                               32
#define CPU_ITM_STIM9_STIM9_M                                       0xFFFFFFFF
#define CPU_ITM_STIM9_STIM9_S                                                0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM10
//
//*****************************************************************************
// Field:  [31:0] STIM10
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA10 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM10_STIM10_W                                             32
#define CPU_ITM_STIM10_STIM10_M                                     0xFFFFFFFF
#define CPU_ITM_STIM10_STIM10_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM11
//
//*****************************************************************************
// Field:  [31:0] STIM11
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA11 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM11_STIM11_W                                             32
#define CPU_ITM_STIM11_STIM11_M                                     0xFFFFFFFF
#define CPU_ITM_STIM11_STIM11_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM12
//
//*****************************************************************************
// Field:  [31:0] STIM12
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA12 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM12_STIM12_W                                             32
#define CPU_ITM_STIM12_STIM12_M                                     0xFFFFFFFF
#define CPU_ITM_STIM12_STIM12_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM13
//
//*****************************************************************************
// Field:  [31:0] STIM13
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA13 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM13_STIM13_W                                             32
#define CPU_ITM_STIM13_STIM13_M                                     0xFFFFFFFF
#define CPU_ITM_STIM13_STIM13_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM14
//
//*****************************************************************************
// Field:  [31:0] STIM14
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA14 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM14_STIM14_W                                             32
#define CPU_ITM_STIM14_STIM14_M                                     0xFFFFFFFF
#define CPU_ITM_STIM14_STIM14_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM15
//
//*****************************************************************************
// Field:  [31:0] STIM15
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA15 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM15_STIM15_W                                             32
#define CPU_ITM_STIM15_STIM15_M                                     0xFFFFFFFF
#define CPU_ITM_STIM15_STIM15_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM16
//
//*****************************************************************************
// Field:  [31:0] STIM16
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA16 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM16_STIM16_W                                             32
#define CPU_ITM_STIM16_STIM16_M                                     0xFFFFFFFF
#define CPU_ITM_STIM16_STIM16_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM17
//
//*****************************************************************************
// Field:  [31:0] STIM17
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA17 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM17_STIM17_W                                             32
#define CPU_ITM_STIM17_STIM17_M                                     0xFFFFFFFF
#define CPU_ITM_STIM17_STIM17_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM18
//
//*****************************************************************************
// Field:  [31:0] STIM18
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA18 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM18_STIM18_W                                             32
#define CPU_ITM_STIM18_STIM18_M                                     0xFFFFFFFF
#define CPU_ITM_STIM18_STIM18_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM19
//
//*****************************************************************************
// Field:  [31:0] STIM19
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA19 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM19_STIM19_W                                             32
#define CPU_ITM_STIM19_STIM19_M                                     0xFFFFFFFF
#define CPU_ITM_STIM19_STIM19_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM20
//
//*****************************************************************************
// Field:  [31:0] STIM20
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA20 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM20_STIM20_W                                             32
#define CPU_ITM_STIM20_STIM20_M                                     0xFFFFFFFF
#define CPU_ITM_STIM20_STIM20_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM21
//
//*****************************************************************************
// Field:  [31:0] STIM21
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA21 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM21_STIM21_W                                             32
#define CPU_ITM_STIM21_STIM21_M                                     0xFFFFFFFF
#define CPU_ITM_STIM21_STIM21_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM22
//
//*****************************************************************************
// Field:  [31:0] STIM22
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA22 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM22_STIM22_W                                             32
#define CPU_ITM_STIM22_STIM22_M                                     0xFFFFFFFF
#define CPU_ITM_STIM22_STIM22_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM23
//
//*****************************************************************************
// Field:  [31:0] STIM23
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA23 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM23_STIM23_W                                             32
#define CPU_ITM_STIM23_STIM23_M                                     0xFFFFFFFF
#define CPU_ITM_STIM23_STIM23_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM24
//
//*****************************************************************************
// Field:  [31:0] STIM24
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA24 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM24_STIM24_W                                             32
#define CPU_ITM_STIM24_STIM24_M                                     0xFFFFFFFF
#define CPU_ITM_STIM24_STIM24_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM25
//
//*****************************************************************************
// Field:  [31:0] STIM25
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA25 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM25_STIM25_W                                             32
#define CPU_ITM_STIM25_STIM25_M                                     0xFFFFFFFF
#define CPU_ITM_STIM25_STIM25_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM26
//
//*****************************************************************************
// Field:  [31:0] STIM26
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA26 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM26_STIM26_W                                             32
#define CPU_ITM_STIM26_STIM26_M                                     0xFFFFFFFF
#define CPU_ITM_STIM26_STIM26_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM27
//
//*****************************************************************************
// Field:  [31:0] STIM27
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA27 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM27_STIM27_W                                             32
#define CPU_ITM_STIM27_STIM27_M                                     0xFFFFFFFF
#define CPU_ITM_STIM27_STIM27_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM28
//
//*****************************************************************************
// Field:  [31:0] STIM28
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA28 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM28_STIM28_W                                             32
#define CPU_ITM_STIM28_STIM28_M                                     0xFFFFFFFF
#define CPU_ITM_STIM28_STIM28_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM29
//
//*****************************************************************************
// Field:  [31:0] STIM29
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA29 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM29_STIM29_W                                             32
#define CPU_ITM_STIM29_STIM29_M                                     0xFFFFFFFF
#define CPU_ITM_STIM29_STIM29_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM30
//
//*****************************************************************************
// Field:  [31:0] STIM30
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA30 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM30_STIM30_W                                             32
#define CPU_ITM_STIM30_STIM30_M                                     0xFFFFFFFF
#define CPU_ITM_STIM30_STIM30_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_STIM31
//
//*****************************************************************************
// Field:  [31:0] STIM31
//
// A write to this location causes data to be written into the FIFO if
// TER.STIMENA31 is set. Reading from the stimulus port returns the FIFO status
// in bit [0]: 0 = full, 1 = not full. The polled FIFO interface does not
// provide an atomic read-modify-write, so it's users responsibility to ensure
// exclusive read-modify-write if this ITM port is used concurrently by
// interrupts or other threads.
#define CPU_ITM_STIM31_STIM31_W                                             32
#define CPU_ITM_STIM31_STIM31_M                                     0xFFFFFFFF
#define CPU_ITM_STIM31_STIM31_S                                              0

//*****************************************************************************
//
// Register: CPU_ITM_O_TER
//
//*****************************************************************************
// Field:    [31] STIMENA31
//
// Bit mask to enable tracing on ITM stimulus port 31.
#define CPU_ITM_TER_STIMENA31                                       0x80000000
#define CPU_ITM_TER_STIMENA31_BITN                                          31
#define CPU_ITM_TER_STIMENA31_M                                     0x80000000
#define CPU_ITM_TER_STIMENA31_S                                             31

// Field:    [30] STIMENA30
//
// Bit mask to enable tracing on ITM stimulus port 30.
#define CPU_ITM_TER_STIMENA30                                       0x40000000
#define CPU_ITM_TER_STIMENA30_BITN                                          30
#define CPU_ITM_TER_STIMENA30_M                                     0x40000000
#define CPU_ITM_TER_STIMENA30_S                                             30

// Field:    [29] STIMENA29
//
// Bit mask to enable tracing on ITM stimulus port 29.
#define CPU_ITM_TER_STIMENA29                                       0x20000000
#define CPU_ITM_TER_STIMENA29_BITN                                          29
#define CPU_ITM_TER_STIMENA29_M                                     0x20000000
#define CPU_ITM_TER_STIMENA29_S                                             29

// Field:    [28] STIMENA28
//
// Bit mask to enable tracing on ITM stimulus port 28.
#define CPU_ITM_TER_STIMENA28                                       0x10000000
#define CPU_ITM_TER_STIMENA28_BITN                                          28
#define CPU_ITM_TER_STIMENA28_M                                     0x10000000
#define CPU_ITM_TER_STIMENA28_S                                             28

// Field:    [27] STIMENA27
//
// Bit mask to enable tracing on ITM stimulus port 27.
#define CPU_ITM_TER_STIMENA27                                       0x08000000
#define CPU_ITM_TER_STIMENA27_BITN                                          27
#define CPU_ITM_TER_STIMENA27_M                                     0x08000000
#define CPU_ITM_TER_STIMENA27_S                                             27

// Field:    [26] STIMENA26
//
// Bit mask to enable tracing on ITM stimulus port 26.
#define CPU_ITM_TER_STIMENA26                                       0x04000000
#define CPU_ITM_TER_STIMENA26_BITN                                          26
#define CPU_ITM_TER_STIMENA26_M                                     0x04000000
#define CPU_ITM_TER_STIMENA26_S                                             26

// Field:    [25] STIMENA25
//
// Bit mask to enable tracing on ITM stimulus port 25.
#define CPU_ITM_TER_STIMENA25                                       0x02000000
#define CPU_ITM_TER_STIMENA25_BITN                                          25
#define CPU_ITM_TER_STIMENA25_M                                     0x02000000
#define CPU_ITM_TER_STIMENA25_S                                             25

// Field:    [24] STIMENA24
//
// Bit mask to enable tracing on ITM stimulus port 24.
#define CPU_ITM_TER_STIMENA24                                       0x01000000
#define CPU_ITM_TER_STIMENA24_BITN                                          24
#define CPU_ITM_TER_STIMENA24_M                                     0x01000000
#define CPU_ITM_TER_STIMENA24_S                                             24

// Field:    [23] STIMENA23
//
// Bit mask to enable tracing on ITM stimulus port 23.
#define CPU_ITM_TER_STIMENA23                                       0x00800000
#define CPU_ITM_TER_STIMENA23_BITN                                          23
#define CPU_ITM_TER_STIMENA23_M                                     0x00800000
#define CPU_ITM_TER_STIMENA23_S                                             23

// Field:    [22] STIMENA22
//
// Bit mask to enable tracing on ITM stimulus port 22.
#define CPU_ITM_TER_STIMENA22                                       0x00400000
#define CPU_ITM_TER_STIMENA22_BITN                                          22
#define CPU_ITM_TER_STIMENA22_M                                     0x00400000
#define CPU_ITM_TER_STIMENA22_S                                             22

// Field:    [21] STIMENA21
//
// Bit mask to enable tracing on ITM stimulus port 21.
#define CPU_ITM_TER_STIMENA21                                       0x00200000
#define CPU_ITM_TER_STIMENA21_BITN                                          21
#define CPU_ITM_TER_STIMENA21_M                                     0x00200000
#define CPU_ITM_TER_STIMENA21_S                                             21

// Field:    [20] STIMENA20
//
// Bit mask to enable tracing on ITM stimulus port 20.
#define CPU_ITM_TER_STIMENA20                                       0x00100000
#define CPU_ITM_TER_STIMENA20_BITN                                          20
#define CPU_ITM_TER_STIMENA20_M                                     0x00100000
#define CPU_ITM_TER_STIMENA20_S                                             20

// Field:    [19] STIMENA19
//
// Bit mask to enable tracing on ITM stimulus port 19.
#define CPU_ITM_TER_STIMENA19                                       0x00080000
#define CPU_ITM_TER_STIMENA19_BITN                                          19
#define CPU_ITM_TER_STIMENA19_M                                     0x00080000
#define CPU_ITM_TER_STIMENA19_S                                             19

// Field:    [18] STIMENA18
//
// Bit mask to enable tracing on ITM stimulus port 18.
#define CPU_ITM_TER_STIMENA18                                       0x00040000
#define CPU_ITM_TER_STIMENA18_BITN                                          18
#define CPU_ITM_TER_STIMENA18_M                                     0x00040000
#define CPU_ITM_TER_STIMENA18_S                                             18

// Field:    [17] STIMENA17
//
// Bit mask to enable tracing on ITM stimulus port 17.
#define CPU_ITM_TER_STIMENA17                                       0x00020000
#define CPU_ITM_TER_STIMENA17_BITN                                          17
#define CPU_ITM_TER_STIMENA17_M                                     0x00020000
#define CPU_ITM_TER_STIMENA17_S                                             17

// Field:    [16] STIMENA16
//
// Bit mask to enable tracing on ITM stimulus port 16.
#define CPU_ITM_TER_STIMENA16                                       0x00010000
#define CPU_ITM_TER_STIMENA16_BITN                                          16
#define CPU_ITM_TER_STIMENA16_M                                     0x00010000
#define CPU_ITM_TER_STIMENA16_S                                             16

// Field:    [15] STIMENA15
//
// Bit mask to enable tracing on ITM stimulus port 15.
#define CPU_ITM_TER_STIMENA15                                       0x00008000
#define CPU_ITM_TER_STIMENA15_BITN                                          15
#define CPU_ITM_TER_STIMENA15_M                                     0x00008000
#define CPU_ITM_TER_STIMENA15_S                                             15

// Field:    [14] STIMENA14
//
// Bit mask to enable tracing on ITM stimulus port 14.
#define CPU_ITM_TER_STIMENA14                                       0x00004000
#define CPU_ITM_TER_STIMENA14_BITN                                          14
#define CPU_ITM_TER_STIMENA14_M                                     0x00004000
#define CPU_ITM_TER_STIMENA14_S                                             14

// Field:    [13] STIMENA13
//
// Bit mask to enable tracing on ITM stimulus port 13.
#define CPU_ITM_TER_STIMENA13                                       0x00002000
#define CPU_ITM_TER_STIMENA13_BITN                                          13
#define CPU_ITM_TER_STIMENA13_M                                     0x00002000
#define CPU_ITM_TER_STIMENA13_S                                             13

// Field:    [12] STIMENA12
//
// Bit mask to enable tracing on ITM stimulus port 12.
#define CPU_ITM_TER_STIMENA12                                       0x00001000
#define CPU_ITM_TER_STIMENA12_BITN                                          12
#define CPU_ITM_TER_STIMENA12_M                                     0x00001000
#define CPU_ITM_TER_STIMENA12_S                                             12

// Field:    [11] STIMENA11
//
// Bit mask to enable tracing on ITM stimulus port 11.
#define CPU_ITM_TER_STIMENA11                                       0x00000800
#define CPU_ITM_TER_STIMENA11_BITN                                          11
#define CPU_ITM_TER_STIMENA11_M                                     0x00000800
#define CPU_ITM_TER_STIMENA11_S                                             11

// Field:    [10] STIMENA10
//
// Bit mask to enable tracing on ITM stimulus port 10.
#define CPU_ITM_TER_STIMENA10                                       0x00000400
#define CPU_ITM_TER_STIMENA10_BITN                                          10
#define CPU_ITM_TER_STIMENA10_M                                     0x00000400
#define CPU_ITM_TER_STIMENA10_S                                             10

// Field:     [9] STIMENA9
//
// Bit mask to enable tracing on ITM stimulus port 9.
#define CPU_ITM_TER_STIMENA9                                        0x00000200
#define CPU_ITM_TER_STIMENA9_BITN                                            9
#define CPU_ITM_TER_STIMENA9_M                                      0x00000200
#define CPU_ITM_TER_STIMENA9_S                                               9

// Field:     [8] STIMENA8
//
// Bit mask to enable tracing on ITM stimulus port 8.
#define CPU_ITM_TER_STIMENA8                                        0x00000100
#define CPU_ITM_TER_STIMENA8_BITN                                            8
#define CPU_ITM_TER_STIMENA8_M                                      0x00000100
#define CPU_ITM_TER_STIMENA8_S                                               8

// Field:     [7] STIMENA7
//
// Bit mask to enable tracing on ITM stimulus port 7.
#define CPU_ITM_TER_STIMENA7                                        0x00000080
#define CPU_ITM_TER_STIMENA7_BITN                                            7
#define CPU_ITM_TER_STIMENA7_M                                      0x00000080
#define CPU_ITM_TER_STIMENA7_S                                               7

// Field:     [6] STIMENA6
//
// Bit mask to enable tracing on ITM stimulus port 6.
#define CPU_ITM_TER_STIMENA6                                        0x00000040
#define CPU_ITM_TER_STIMENA6_BITN                                            6
#define CPU_ITM_TER_STIMENA6_M                                      0x00000040
#define CPU_ITM_TER_STIMENA6_S                                               6

// Field:     [5] STIMENA5
//
// Bit mask to enable tracing on ITM stimulus port 5.
#define CPU_ITM_TER_STIMENA5                                        0x00000020
#define CPU_ITM_TER_STIMENA5_BITN                                            5
#define CPU_ITM_TER_STIMENA5_M                                      0x00000020
#define CPU_ITM_TER_STIMENA5_S                                               5

// Field:     [4] STIMENA4
//
// Bit mask to enable tracing on ITM stimulus port 4.
#define CPU_ITM_TER_STIMENA4                                        0x00000010
#define CPU_ITM_TER_STIMENA4_BITN                                            4
#define CPU_ITM_TER_STIMENA4_M                                      0x00000010
#define CPU_ITM_TER_STIMENA4_S                                               4

// Field:     [3] STIMENA3
//
// Bit mask to enable tracing on ITM stimulus port 3.
#define CPU_ITM_TER_STIMENA3                                        0x00000008
#define CPU_ITM_TER_STIMENA3_BITN                                            3
#define CPU_ITM_TER_STIMENA3_M                                      0x00000008
#define CPU_ITM_TER_STIMENA3_S                                               3

// Field:     [2] STIMENA2
//
// Bit mask to enable tracing on ITM stimulus port 2.
#define CPU_ITM_TER_STIMENA2                                        0x00000004
#define CPU_ITM_TER_STIMENA2_BITN                                            2
#define CPU_ITM_TER_STIMENA2_M                                      0x00000004
#define CPU_ITM_TER_STIMENA2_S                                               2

// Field:     [1] STIMENA1
//
// Bit mask to enable tracing on ITM stimulus port 1.
#define CPU_ITM_TER_STIMENA1                                        0x00000002
#define CPU_ITM_TER_STIMENA1_BITN                                            1
#define CPU_ITM_TER_STIMENA1_M                                      0x00000002
#define CPU_ITM_TER_STIMENA1_S                                               1

// Field:     [0] STIMENA0
//
// Bit mask to enable tracing on ITM stimulus port 0.
#define CPU_ITM_TER_STIMENA0                                        0x00000001
#define CPU_ITM_TER_STIMENA0_BITN                                            0
#define CPU_ITM_TER_STIMENA0_M                                      0x00000001
#define CPU_ITM_TER_STIMENA0_S                                               0

//*****************************************************************************
//
// Register: CPU_ITM_O_TPR
//
//*****************************************************************************
// Field:   [3:0] PRIVMASK
//
// Bit mask to enable unprivileged (User) access to ITM stimulus ports:
//
// Bit [0] enables stimulus ports 0, 1, ..., and 7.
// Bit [1] enables stimulus ports 8, 9, ..., and 15.
// Bit [2] enables stimulus ports 16, 17, ..., and 23.
// Bit [3] enables stimulus ports 24, 25, ..., and 31.
//
// 0: User access allowed to stimulus ports
// 1: Privileged access only to stimulus ports
#define CPU_ITM_TPR_PRIVMASK_W                                               4
#define CPU_ITM_TPR_PRIVMASK_M                                      0x0000000F
#define CPU_ITM_TPR_PRIVMASK_S                                               0

//*****************************************************************************
//
// Register: CPU_ITM_O_TCR
//
//*****************************************************************************
// Field:    [23] BUSY
//
// Set when ITM events present and being drained.
#define CPU_ITM_TCR_BUSY                                            0x00800000
#define CPU_ITM_TCR_BUSY_BITN                                               23
#define CPU_ITM_TCR_BUSY_M                                          0x00800000
#define CPU_ITM_TCR_BUSY_S                                                  23

// Field: [22:16] ATBID
//
// Trace Bus ID for CoreSight system. Optional identifier for multi-source
// trace stream formatting. If multi-source trace is in use, this field must be
// written with a non-zero value.
#define CPU_ITM_TCR_ATBID_W                                                  7
#define CPU_ITM_TCR_ATBID_M                                         0x007F0000
#define CPU_ITM_TCR_ATBID_S                                                 16

// Field:   [9:8] TSPRESCALE
//
// Timestamp prescaler
// ENUMs:
// DIV64                    Divide by 64
// DIV16                    Divide by 16
// DIV4                     Divide by 4
// NOPRESCALING             No prescaling
#define CPU_ITM_TCR_TSPRESCALE_W                                             2
#define CPU_ITM_TCR_TSPRESCALE_M                                    0x00000300
#define CPU_ITM_TCR_TSPRESCALE_S                                             8
#define CPU_ITM_TCR_TSPRESCALE_DIV64                                0x00000300
#define CPU_ITM_TCR_TSPRESCALE_DIV16                                0x00000200
#define CPU_ITM_TCR_TSPRESCALE_DIV4                                 0x00000100
#define CPU_ITM_TCR_TSPRESCALE_NOPRESCALING                         0x00000000

// Field:     [4] SWOENA
//
// Enables asynchronous clocking of the timestamp counter (when TSENA = 1). If
// TSENA = 0, writing this bit to 1 does not enable asynchronous clocking of
// the timestamp counter.
//
// 0x0: Mode disabled. Timestamp counter uses system clock from the core and
// counts continuously.
// 0x1: Timestamp counter uses lineout (data related) clock from TPIU
// interface. The timestamp counter is held in reset while the output line is
// idle.
#define CPU_ITM_TCR_SWOENA                                          0x00000010
#define CPU_ITM_TCR_SWOENA_BITN                                              4
#define CPU_ITM_TCR_SWOENA_M                                        0x00000010
#define CPU_ITM_TCR_SWOENA_S                                                 4

// Field:     [3] DWTENA
//
// Enables the DWT stimulus (hardware event packet emission to the TPIU from
// the DWT)
#define CPU_ITM_TCR_DWTENA                                          0x00000008
#define CPU_ITM_TCR_DWTENA_BITN                                              3
#define CPU_ITM_TCR_DWTENA_M                                        0x00000008
#define CPU_ITM_TCR_DWTENA_S                                                 3

// Field:     [2] SYNCENA
//
// Enables synchronization packet transmission for a synchronous TPIU.
// CPU_DWT:CTRL.SYNCTAP must be configured for the correct synchronization
// speed.
#define CPU_ITM_TCR_SYNCENA                                         0x00000004
#define CPU_ITM_TCR_SYNCENA_BITN                                             2
#define CPU_ITM_TCR_SYNCENA_M                                       0x00000004
#define CPU_ITM_TCR_SYNCENA_S                                                2

// Field:     [1] TSENA
//
// Enables differential timestamps. Differential timestamps are emitted when a
// packet is written to the FIFO with a non-zero timestamp counter, and when
// the timestamp counter overflows. Timestamps are emitted during idle times
// after a fixed number of two million cycles. This provides a time reference
// for packets and inter-packet gaps. If SWOENA (bit [4]) is set, timestamps
// are triggered by activity on the internal trace bus only. In this case there
// is no regular timestamp output when the ITM is idle.
#define CPU_ITM_TCR_TSENA                                           0x00000002
#define CPU_ITM_TCR_TSENA_BITN                                               1
#define CPU_ITM_TCR_TSENA_M                                         0x00000002
#define CPU_ITM_TCR_TSENA_S                                                  1

// Field:     [0] ITMENA
//
// Enables ITM. This is the master enable, and must be set before ITM Stimulus
// and Trace Enable registers can be written.
#define CPU_ITM_TCR_ITMENA                                          0x00000001
#define CPU_ITM_TCR_ITMENA_BITN                                              0
#define CPU_ITM_TCR_ITMENA_M                                        0x00000001
#define CPU_ITM_TCR_ITMENA_S                                                 0

//*****************************************************************************
//
// Register: CPU_ITM_O_LAR
//
//*****************************************************************************
// Field:  [31:0] LOCK_ACCESS
//
// A privileged write of 0xC5ACCE55 enables more write access to Control
// Registers TER, TPR and TCR. An invalid write removes write access.
#define CPU_ITM_LAR_LOCK_ACCESS_W                                           32
#define CPU_ITM_LAR_LOCK_ACCESS_M                                   0xFFFFFFFF
#define CPU_ITM_LAR_LOCK_ACCESS_S                                            0

//*****************************************************************************
//
// Register: CPU_ITM_O_LSR
//
//*****************************************************************************
// Field:     [2] BYTEACC
//
// Reads 0 which means 8-bit lock access is not be implemented.
#define CPU_ITM_LSR_BYTEACC                                         0x00000004
#define CPU_ITM_LSR_BYTEACC_BITN                                             2
#define CPU_ITM_LSR_BYTEACC_M                                       0x00000004
#define CPU_ITM_LSR_BYTEACC_S                                                2

// Field:     [1] ACCESS
//
// Write access to component is blocked. All writes are ignored, reads are
// permitted.
#define CPU_ITM_LSR_ACCESS                                          0x00000002
#define CPU_ITM_LSR_ACCESS_BITN                                              1
#define CPU_ITM_LSR_ACCESS_M                                        0x00000002
#define CPU_ITM_LSR_ACCESS_S                                                 1

// Field:     [0] PRESENT
//
// Indicates that a lock mechanism exists for this component.
#define CPU_ITM_LSR_PRESENT                                         0x00000001
#define CPU_ITM_LSR_PRESENT_BITN                                             0
#define CPU_ITM_LSR_PRESENT_M                                       0x00000001
#define CPU_ITM_LSR_PRESENT_S                                                0


#endif // __CPU_ITM__
