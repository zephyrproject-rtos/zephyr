/******************************************************************************
*  Filename:       hw_gpio_h
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

#ifndef __HW_GPIO_H__
#define __HW_GPIO_H__

//*****************************************************************************
//
// This section defines the register offsets of
// GPIO component
//
//*****************************************************************************
// Data Out 0 to 3
#define GPIO_O_DOUT3_0                                              0x00000000

// Data Out 4 to 7
#define GPIO_O_DOUT7_4                                              0x00000004

// Data Out 8 to 11
#define GPIO_O_DOUT11_8                                             0x00000008

// Data Out 12 to 15
#define GPIO_O_DOUT15_12                                            0x0000000C

// Data Out 16 to 19
#define GPIO_O_DOUT19_16                                            0x00000010

// Data Out 20 to 23
#define GPIO_O_DOUT23_20                                            0x00000014

// Data Out 24 to 27
#define GPIO_O_DOUT27_24                                            0x00000018

// Data Out 28 to 31
#define GPIO_O_DOUT31_28                                            0x0000001C

// Data Output for DIO 0 to 31
#define GPIO_O_DOUT31_0                                             0x00000080

// Data Out Set
#define GPIO_O_DOUTSET31_0                                          0x00000090

// Data Out Clear
#define GPIO_O_DOUTCLR31_0                                          0x000000A0

// Data Out Toggle
#define GPIO_O_DOUTTGL31_0                                          0x000000B0

// Data Input from DIO 0 to 31
#define GPIO_O_DIN31_0                                              0x000000C0

// Data Output Enable for DIO 0 to 31
#define GPIO_O_DOE31_0                                              0x000000D0

// Event Register for DIO 0 to 31
#define GPIO_O_EVFLAGS31_0                                          0x000000E0

//*****************************************************************************
//
// Register: GPIO_O_DOUT3_0
//
//*****************************************************************************
// Field:    [24] DIO3
//
// Sets the state of the pin that is configured as DIO#3, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT3_0_DIO3                                           0x01000000
#define GPIO_DOUT3_0_DIO3_BITN                                              24
#define GPIO_DOUT3_0_DIO3_M                                         0x01000000
#define GPIO_DOUT3_0_DIO3_S                                                 24

// Field:    [16] DIO2
//
// Sets the state of the pin that is configured as DIO#2, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT3_0_DIO2                                           0x00010000
#define GPIO_DOUT3_0_DIO2_BITN                                              16
#define GPIO_DOUT3_0_DIO2_M                                         0x00010000
#define GPIO_DOUT3_0_DIO2_S                                                 16

// Field:     [8] DIO1
//
// Sets the state of the pin that is configured as DIO#1, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT3_0_DIO1                                           0x00000100
#define GPIO_DOUT3_0_DIO1_BITN                                               8
#define GPIO_DOUT3_0_DIO1_M                                         0x00000100
#define GPIO_DOUT3_0_DIO1_S                                                  8

// Field:     [0] DIO0
//
// Sets the state of the pin that is configured as DIO#0, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT3_0_DIO0                                           0x00000001
#define GPIO_DOUT3_0_DIO0_BITN                                               0
#define GPIO_DOUT3_0_DIO0_M                                         0x00000001
#define GPIO_DOUT3_0_DIO0_S                                                  0

//*****************************************************************************
//
// Register: GPIO_O_DOUT7_4
//
//*****************************************************************************
// Field:    [24] DIO7
//
// Sets the state of the pin that is configured as DIO#7, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT7_4_DIO7                                           0x01000000
#define GPIO_DOUT7_4_DIO7_BITN                                              24
#define GPIO_DOUT7_4_DIO7_M                                         0x01000000
#define GPIO_DOUT7_4_DIO7_S                                                 24

// Field:    [16] DIO6
//
// Sets the state of the pin that is configured as DIO#6, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT7_4_DIO6                                           0x00010000
#define GPIO_DOUT7_4_DIO6_BITN                                              16
#define GPIO_DOUT7_4_DIO6_M                                         0x00010000
#define GPIO_DOUT7_4_DIO6_S                                                 16

// Field:     [8] DIO5
//
// Sets the state of the pin that is configured as DIO#5, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT7_4_DIO5                                           0x00000100
#define GPIO_DOUT7_4_DIO5_BITN                                               8
#define GPIO_DOUT7_4_DIO5_M                                         0x00000100
#define GPIO_DOUT7_4_DIO5_S                                                  8

// Field:     [0] DIO4
//
// Sets the state of the pin that is configured as DIO#4, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT7_4_DIO4                                           0x00000001
#define GPIO_DOUT7_4_DIO4_BITN                                               0
#define GPIO_DOUT7_4_DIO4_M                                         0x00000001
#define GPIO_DOUT7_4_DIO4_S                                                  0

//*****************************************************************************
//
// Register: GPIO_O_DOUT11_8
//
//*****************************************************************************
// Field:    [24] DIO11
//
// Sets the state of the pin that is configured as DIO#11, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT11_8_DIO11                                         0x01000000
#define GPIO_DOUT11_8_DIO11_BITN                                            24
#define GPIO_DOUT11_8_DIO11_M                                       0x01000000
#define GPIO_DOUT11_8_DIO11_S                                               24

// Field:    [16] DIO10
//
// Sets the state of the pin that is configured as DIO#10, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT11_8_DIO10                                         0x00010000
#define GPIO_DOUT11_8_DIO10_BITN                                            16
#define GPIO_DOUT11_8_DIO10_M                                       0x00010000
#define GPIO_DOUT11_8_DIO10_S                                               16

// Field:     [8] DIO9
//
// Sets the state of the pin that is configured as DIO#9, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT11_8_DIO9                                          0x00000100
#define GPIO_DOUT11_8_DIO9_BITN                                              8
#define GPIO_DOUT11_8_DIO9_M                                        0x00000100
#define GPIO_DOUT11_8_DIO9_S                                                 8

// Field:     [0] DIO8
//
// Sets the state of the pin that is configured as DIO#8, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT11_8_DIO8                                          0x00000001
#define GPIO_DOUT11_8_DIO8_BITN                                              0
#define GPIO_DOUT11_8_DIO8_M                                        0x00000001
#define GPIO_DOUT11_8_DIO8_S                                                 0

//*****************************************************************************
//
// Register: GPIO_O_DOUT15_12
//
//*****************************************************************************
// Field:    [24] DIO15
//
// Sets the state of the pin that is configured as DIO#15, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT15_12_DIO15                                        0x01000000
#define GPIO_DOUT15_12_DIO15_BITN                                           24
#define GPIO_DOUT15_12_DIO15_M                                      0x01000000
#define GPIO_DOUT15_12_DIO15_S                                              24

// Field:    [16] DIO14
//
// Sets the state of the pin that is configured as DIO#14, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT15_12_DIO14                                        0x00010000
#define GPIO_DOUT15_12_DIO14_BITN                                           16
#define GPIO_DOUT15_12_DIO14_M                                      0x00010000
#define GPIO_DOUT15_12_DIO14_S                                              16

// Field:     [8] DIO13
//
// Sets the state of the pin that is configured as DIO#13, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT15_12_DIO13                                        0x00000100
#define GPIO_DOUT15_12_DIO13_BITN                                            8
#define GPIO_DOUT15_12_DIO13_M                                      0x00000100
#define GPIO_DOUT15_12_DIO13_S                                               8

// Field:     [0] DIO12
//
// Sets the state of the pin that is configured as DIO#12, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT15_12_DIO12                                        0x00000001
#define GPIO_DOUT15_12_DIO12_BITN                                            0
#define GPIO_DOUT15_12_DIO12_M                                      0x00000001
#define GPIO_DOUT15_12_DIO12_S                                               0

//*****************************************************************************
//
// Register: GPIO_O_DOUT19_16
//
//*****************************************************************************
// Field:    [24] DIO19
//
// Sets the state of the pin that is configured as DIO#19, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT19_16_DIO19                                        0x01000000
#define GPIO_DOUT19_16_DIO19_BITN                                           24
#define GPIO_DOUT19_16_DIO19_M                                      0x01000000
#define GPIO_DOUT19_16_DIO19_S                                              24

// Field:    [16] DIO18
//
// Sets the state of the pin that is configured as DIO#18, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT19_16_DIO18                                        0x00010000
#define GPIO_DOUT19_16_DIO18_BITN                                           16
#define GPIO_DOUT19_16_DIO18_M                                      0x00010000
#define GPIO_DOUT19_16_DIO18_S                                              16

// Field:     [8] DIO17
//
// Sets the state of the pin that is configured as DIO#17, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT19_16_DIO17                                        0x00000100
#define GPIO_DOUT19_16_DIO17_BITN                                            8
#define GPIO_DOUT19_16_DIO17_M                                      0x00000100
#define GPIO_DOUT19_16_DIO17_S                                               8

// Field:     [0] DIO16
//
// Sets the state of the pin that is configured as DIO#16, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT19_16_DIO16                                        0x00000001
#define GPIO_DOUT19_16_DIO16_BITN                                            0
#define GPIO_DOUT19_16_DIO16_M                                      0x00000001
#define GPIO_DOUT19_16_DIO16_S                                               0

//*****************************************************************************
//
// Register: GPIO_O_DOUT23_20
//
//*****************************************************************************
// Field:    [24] DIO23
//
// Sets the state of the pin that is configured as DIO#23, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT23_20_DIO23                                        0x01000000
#define GPIO_DOUT23_20_DIO23_BITN                                           24
#define GPIO_DOUT23_20_DIO23_M                                      0x01000000
#define GPIO_DOUT23_20_DIO23_S                                              24

// Field:    [16] DIO22
//
// Sets the state of the pin that is configured as DIO#22, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT23_20_DIO22                                        0x00010000
#define GPIO_DOUT23_20_DIO22_BITN                                           16
#define GPIO_DOUT23_20_DIO22_M                                      0x00010000
#define GPIO_DOUT23_20_DIO22_S                                              16

// Field:     [8] DIO21
//
// Sets the state of the pin that is configured as DIO#21, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT23_20_DIO21                                        0x00000100
#define GPIO_DOUT23_20_DIO21_BITN                                            8
#define GPIO_DOUT23_20_DIO21_M                                      0x00000100
#define GPIO_DOUT23_20_DIO21_S                                               8

// Field:     [0] DIO20
//
// Sets the state of the pin that is configured as DIO#20, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT23_20_DIO20                                        0x00000001
#define GPIO_DOUT23_20_DIO20_BITN                                            0
#define GPIO_DOUT23_20_DIO20_M                                      0x00000001
#define GPIO_DOUT23_20_DIO20_S                                               0

//*****************************************************************************
//
// Register: GPIO_O_DOUT27_24
//
//*****************************************************************************
// Field:    [24] DIO27
//
// Sets the state of the pin that is configured as DIO#27, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT27_24_DIO27                                        0x01000000
#define GPIO_DOUT27_24_DIO27_BITN                                           24
#define GPIO_DOUT27_24_DIO27_M                                      0x01000000
#define GPIO_DOUT27_24_DIO27_S                                              24

// Field:    [16] DIO26
//
// Sets the state of the pin that is configured as DIO#26, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT27_24_DIO26                                        0x00010000
#define GPIO_DOUT27_24_DIO26_BITN                                           16
#define GPIO_DOUT27_24_DIO26_M                                      0x00010000
#define GPIO_DOUT27_24_DIO26_S                                              16

// Field:     [8] DIO25
//
// Sets the state of the pin that is configured as DIO#25, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT27_24_DIO25                                        0x00000100
#define GPIO_DOUT27_24_DIO25_BITN                                            8
#define GPIO_DOUT27_24_DIO25_M                                      0x00000100
#define GPIO_DOUT27_24_DIO25_S                                               8

// Field:     [0] DIO24
//
// Sets the state of the pin that is configured as DIO#24, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT27_24_DIO24                                        0x00000001
#define GPIO_DOUT27_24_DIO24_BITN                                            0
#define GPIO_DOUT27_24_DIO24_M                                      0x00000001
#define GPIO_DOUT27_24_DIO24_S                                               0

//*****************************************************************************
//
// Register: GPIO_O_DOUT31_28
//
//*****************************************************************************
// Field:    [24] DIO31
//
// Sets the state of the pin that is configured as DIO#31, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT31_28_DIO31                                        0x01000000
#define GPIO_DOUT31_28_DIO31_BITN                                           24
#define GPIO_DOUT31_28_DIO31_M                                      0x01000000
#define GPIO_DOUT31_28_DIO31_S                                              24

// Field:    [16] DIO30
//
// Sets the state of the pin that is configured as DIO#30, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT31_28_DIO30                                        0x00010000
#define GPIO_DOUT31_28_DIO30_BITN                                           16
#define GPIO_DOUT31_28_DIO30_M                                      0x00010000
#define GPIO_DOUT31_28_DIO30_S                                              16

// Field:     [8] DIO29
//
// Sets the state of the pin that is configured as DIO#29, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT31_28_DIO29                                        0x00000100
#define GPIO_DOUT31_28_DIO29_BITN                                            8
#define GPIO_DOUT31_28_DIO29_M                                      0x00000100
#define GPIO_DOUT31_28_DIO29_S                                               8

// Field:     [0] DIO28
//
// Sets the state of the pin that is configured as DIO#28, if the corresponding
// DOE31_0 bitfield is set.
#define GPIO_DOUT31_28_DIO28                                        0x00000001
#define GPIO_DOUT31_28_DIO28_BITN                                            0
#define GPIO_DOUT31_28_DIO28_M                                      0x00000001
#define GPIO_DOUT31_28_DIO28_S                                               0

//*****************************************************************************
//
// Register: GPIO_O_DOUT31_0
//
//*****************************************************************************
// Field:    [31] DIO31
//
// Data output for DIO 31
#define GPIO_DOUT31_0_DIO31                                         0x80000000
#define GPIO_DOUT31_0_DIO31_BITN                                            31
#define GPIO_DOUT31_0_DIO31_M                                       0x80000000
#define GPIO_DOUT31_0_DIO31_S                                               31

// Field:    [30] DIO30
//
// Data output for DIO 30
#define GPIO_DOUT31_0_DIO30                                         0x40000000
#define GPIO_DOUT31_0_DIO30_BITN                                            30
#define GPIO_DOUT31_0_DIO30_M                                       0x40000000
#define GPIO_DOUT31_0_DIO30_S                                               30

// Field:    [29] DIO29
//
// Data output for DIO 29
#define GPIO_DOUT31_0_DIO29                                         0x20000000
#define GPIO_DOUT31_0_DIO29_BITN                                            29
#define GPIO_DOUT31_0_DIO29_M                                       0x20000000
#define GPIO_DOUT31_0_DIO29_S                                               29

// Field:    [28] DIO28
//
// Data output for DIO 28
#define GPIO_DOUT31_0_DIO28                                         0x10000000
#define GPIO_DOUT31_0_DIO28_BITN                                            28
#define GPIO_DOUT31_0_DIO28_M                                       0x10000000
#define GPIO_DOUT31_0_DIO28_S                                               28

// Field:    [27] DIO27
//
// Data output for DIO 27
#define GPIO_DOUT31_0_DIO27                                         0x08000000
#define GPIO_DOUT31_0_DIO27_BITN                                            27
#define GPIO_DOUT31_0_DIO27_M                                       0x08000000
#define GPIO_DOUT31_0_DIO27_S                                               27

// Field:    [26] DIO26
//
// Data output for DIO 26
#define GPIO_DOUT31_0_DIO26                                         0x04000000
#define GPIO_DOUT31_0_DIO26_BITN                                            26
#define GPIO_DOUT31_0_DIO26_M                                       0x04000000
#define GPIO_DOUT31_0_DIO26_S                                               26

// Field:    [25] DIO25
//
// Data output for DIO 25
#define GPIO_DOUT31_0_DIO25                                         0x02000000
#define GPIO_DOUT31_0_DIO25_BITN                                            25
#define GPIO_DOUT31_0_DIO25_M                                       0x02000000
#define GPIO_DOUT31_0_DIO25_S                                               25

// Field:    [24] DIO24
//
// Data output for DIO 24
#define GPIO_DOUT31_0_DIO24                                         0x01000000
#define GPIO_DOUT31_0_DIO24_BITN                                            24
#define GPIO_DOUT31_0_DIO24_M                                       0x01000000
#define GPIO_DOUT31_0_DIO24_S                                               24

// Field:    [23] DIO23
//
// Data output for DIO 23
#define GPIO_DOUT31_0_DIO23                                         0x00800000
#define GPIO_DOUT31_0_DIO23_BITN                                            23
#define GPIO_DOUT31_0_DIO23_M                                       0x00800000
#define GPIO_DOUT31_0_DIO23_S                                               23

// Field:    [22] DIO22
//
// Data output for DIO 22
#define GPIO_DOUT31_0_DIO22                                         0x00400000
#define GPIO_DOUT31_0_DIO22_BITN                                            22
#define GPIO_DOUT31_0_DIO22_M                                       0x00400000
#define GPIO_DOUT31_0_DIO22_S                                               22

// Field:    [21] DIO21
//
// Data output for DIO 21
#define GPIO_DOUT31_0_DIO21                                         0x00200000
#define GPIO_DOUT31_0_DIO21_BITN                                            21
#define GPIO_DOUT31_0_DIO21_M                                       0x00200000
#define GPIO_DOUT31_0_DIO21_S                                               21

// Field:    [20] DIO20
//
// Data output for DIO 20
#define GPIO_DOUT31_0_DIO20                                         0x00100000
#define GPIO_DOUT31_0_DIO20_BITN                                            20
#define GPIO_DOUT31_0_DIO20_M                                       0x00100000
#define GPIO_DOUT31_0_DIO20_S                                               20

// Field:    [19] DIO19
//
// Data output for DIO 19
#define GPIO_DOUT31_0_DIO19                                         0x00080000
#define GPIO_DOUT31_0_DIO19_BITN                                            19
#define GPIO_DOUT31_0_DIO19_M                                       0x00080000
#define GPIO_DOUT31_0_DIO19_S                                               19

// Field:    [18] DIO18
//
// Data output for DIO 18
#define GPIO_DOUT31_0_DIO18                                         0x00040000
#define GPIO_DOUT31_0_DIO18_BITN                                            18
#define GPIO_DOUT31_0_DIO18_M                                       0x00040000
#define GPIO_DOUT31_0_DIO18_S                                               18

// Field:    [17] DIO17
//
// Data output for DIO 17
#define GPIO_DOUT31_0_DIO17                                         0x00020000
#define GPIO_DOUT31_0_DIO17_BITN                                            17
#define GPIO_DOUT31_0_DIO17_M                                       0x00020000
#define GPIO_DOUT31_0_DIO17_S                                               17

// Field:    [16] DIO16
//
// Data output for DIO 16
#define GPIO_DOUT31_0_DIO16                                         0x00010000
#define GPIO_DOUT31_0_DIO16_BITN                                            16
#define GPIO_DOUT31_0_DIO16_M                                       0x00010000
#define GPIO_DOUT31_0_DIO16_S                                               16

// Field:    [15] DIO15
//
// Data output for DIO 15
#define GPIO_DOUT31_0_DIO15                                         0x00008000
#define GPIO_DOUT31_0_DIO15_BITN                                            15
#define GPIO_DOUT31_0_DIO15_M                                       0x00008000
#define GPIO_DOUT31_0_DIO15_S                                               15

// Field:    [14] DIO14
//
// Data output for DIO 14
#define GPIO_DOUT31_0_DIO14                                         0x00004000
#define GPIO_DOUT31_0_DIO14_BITN                                            14
#define GPIO_DOUT31_0_DIO14_M                                       0x00004000
#define GPIO_DOUT31_0_DIO14_S                                               14

// Field:    [13] DIO13
//
// Data output for DIO 13
#define GPIO_DOUT31_0_DIO13                                         0x00002000
#define GPIO_DOUT31_0_DIO13_BITN                                            13
#define GPIO_DOUT31_0_DIO13_M                                       0x00002000
#define GPIO_DOUT31_0_DIO13_S                                               13

// Field:    [12] DIO12
//
// Data output for DIO 12
#define GPIO_DOUT31_0_DIO12                                         0x00001000
#define GPIO_DOUT31_0_DIO12_BITN                                            12
#define GPIO_DOUT31_0_DIO12_M                                       0x00001000
#define GPIO_DOUT31_0_DIO12_S                                               12

// Field:    [11] DIO11
//
// Data output for DIO 11
#define GPIO_DOUT31_0_DIO11                                         0x00000800
#define GPIO_DOUT31_0_DIO11_BITN                                            11
#define GPIO_DOUT31_0_DIO11_M                                       0x00000800
#define GPIO_DOUT31_0_DIO11_S                                               11

// Field:    [10] DIO10
//
// Data output for DIO 10
#define GPIO_DOUT31_0_DIO10                                         0x00000400
#define GPIO_DOUT31_0_DIO10_BITN                                            10
#define GPIO_DOUT31_0_DIO10_M                                       0x00000400
#define GPIO_DOUT31_0_DIO10_S                                               10

// Field:     [9] DIO9
//
// Data output for DIO 9
#define GPIO_DOUT31_0_DIO9                                          0x00000200
#define GPIO_DOUT31_0_DIO9_BITN                                              9
#define GPIO_DOUT31_0_DIO9_M                                        0x00000200
#define GPIO_DOUT31_0_DIO9_S                                                 9

// Field:     [8] DIO8
//
// Data output for DIO 8
#define GPIO_DOUT31_0_DIO8                                          0x00000100
#define GPIO_DOUT31_0_DIO8_BITN                                              8
#define GPIO_DOUT31_0_DIO8_M                                        0x00000100
#define GPIO_DOUT31_0_DIO8_S                                                 8

// Field:     [7] DIO7
//
// Data output for DIO 7
#define GPIO_DOUT31_0_DIO7                                          0x00000080
#define GPIO_DOUT31_0_DIO7_BITN                                              7
#define GPIO_DOUT31_0_DIO7_M                                        0x00000080
#define GPIO_DOUT31_0_DIO7_S                                                 7

// Field:     [6] DIO6
//
// Data output for DIO 6
#define GPIO_DOUT31_0_DIO6                                          0x00000040
#define GPIO_DOUT31_0_DIO6_BITN                                              6
#define GPIO_DOUT31_0_DIO6_M                                        0x00000040
#define GPIO_DOUT31_0_DIO6_S                                                 6

// Field:     [5] DIO5
//
// Data output for DIO 5
#define GPIO_DOUT31_0_DIO5                                          0x00000020
#define GPIO_DOUT31_0_DIO5_BITN                                              5
#define GPIO_DOUT31_0_DIO5_M                                        0x00000020
#define GPIO_DOUT31_0_DIO5_S                                                 5

// Field:     [4] DIO4
//
// Data output for DIO 4
#define GPIO_DOUT31_0_DIO4                                          0x00000010
#define GPIO_DOUT31_0_DIO4_BITN                                              4
#define GPIO_DOUT31_0_DIO4_M                                        0x00000010
#define GPIO_DOUT31_0_DIO4_S                                                 4

// Field:     [3] DIO3
//
// Data output for DIO 3
#define GPIO_DOUT31_0_DIO3                                          0x00000008
#define GPIO_DOUT31_0_DIO3_BITN                                              3
#define GPIO_DOUT31_0_DIO3_M                                        0x00000008
#define GPIO_DOUT31_0_DIO3_S                                                 3

// Field:     [2] DIO2
//
// Data output for DIO 2
#define GPIO_DOUT31_0_DIO2                                          0x00000004
#define GPIO_DOUT31_0_DIO2_BITN                                              2
#define GPIO_DOUT31_0_DIO2_M                                        0x00000004
#define GPIO_DOUT31_0_DIO2_S                                                 2

// Field:     [1] DIO1
//
// Data output for DIO 1
#define GPIO_DOUT31_0_DIO1                                          0x00000002
#define GPIO_DOUT31_0_DIO1_BITN                                              1
#define GPIO_DOUT31_0_DIO1_M                                        0x00000002
#define GPIO_DOUT31_0_DIO1_S                                                 1

// Field:     [0] DIO0
//
// Data output for DIO 0
#define GPIO_DOUT31_0_DIO0                                          0x00000001
#define GPIO_DOUT31_0_DIO0_BITN                                              0
#define GPIO_DOUT31_0_DIO0_M                                        0x00000001
#define GPIO_DOUT31_0_DIO0_S                                                 0

//*****************************************************************************
//
// Register: GPIO_O_DOUTSET31_0
//
//*****************************************************************************
// Field:    [31] DIO31
//
// Set bit 31
#define GPIO_DOUTSET31_0_DIO31                                      0x80000000
#define GPIO_DOUTSET31_0_DIO31_BITN                                         31
#define GPIO_DOUTSET31_0_DIO31_M                                    0x80000000
#define GPIO_DOUTSET31_0_DIO31_S                                            31

// Field:    [30] DIO30
//
// Set bit 30
#define GPIO_DOUTSET31_0_DIO30                                      0x40000000
#define GPIO_DOUTSET31_0_DIO30_BITN                                         30
#define GPIO_DOUTSET31_0_DIO30_M                                    0x40000000
#define GPIO_DOUTSET31_0_DIO30_S                                            30

// Field:    [29] DIO29
//
// Set bit 29
#define GPIO_DOUTSET31_0_DIO29                                      0x20000000
#define GPIO_DOUTSET31_0_DIO29_BITN                                         29
#define GPIO_DOUTSET31_0_DIO29_M                                    0x20000000
#define GPIO_DOUTSET31_0_DIO29_S                                            29

// Field:    [28] DIO28
//
// Set bit 28
#define GPIO_DOUTSET31_0_DIO28                                      0x10000000
#define GPIO_DOUTSET31_0_DIO28_BITN                                         28
#define GPIO_DOUTSET31_0_DIO28_M                                    0x10000000
#define GPIO_DOUTSET31_0_DIO28_S                                            28

// Field:    [27] DIO27
//
// Set bit 27
#define GPIO_DOUTSET31_0_DIO27                                      0x08000000
#define GPIO_DOUTSET31_0_DIO27_BITN                                         27
#define GPIO_DOUTSET31_0_DIO27_M                                    0x08000000
#define GPIO_DOUTSET31_0_DIO27_S                                            27

// Field:    [26] DIO26
//
// Set bit 26
#define GPIO_DOUTSET31_0_DIO26                                      0x04000000
#define GPIO_DOUTSET31_0_DIO26_BITN                                         26
#define GPIO_DOUTSET31_0_DIO26_M                                    0x04000000
#define GPIO_DOUTSET31_0_DIO26_S                                            26

// Field:    [25] DIO25
//
// Set bit 25
#define GPIO_DOUTSET31_0_DIO25                                      0x02000000
#define GPIO_DOUTSET31_0_DIO25_BITN                                         25
#define GPIO_DOUTSET31_0_DIO25_M                                    0x02000000
#define GPIO_DOUTSET31_0_DIO25_S                                            25

// Field:    [24] DIO24
//
// Set bit 24
#define GPIO_DOUTSET31_0_DIO24                                      0x01000000
#define GPIO_DOUTSET31_0_DIO24_BITN                                         24
#define GPIO_DOUTSET31_0_DIO24_M                                    0x01000000
#define GPIO_DOUTSET31_0_DIO24_S                                            24

// Field:    [23] DIO23
//
// Set bit 23
#define GPIO_DOUTSET31_0_DIO23                                      0x00800000
#define GPIO_DOUTSET31_0_DIO23_BITN                                         23
#define GPIO_DOUTSET31_0_DIO23_M                                    0x00800000
#define GPIO_DOUTSET31_0_DIO23_S                                            23

// Field:    [22] DIO22
//
// Set bit 22
#define GPIO_DOUTSET31_0_DIO22                                      0x00400000
#define GPIO_DOUTSET31_0_DIO22_BITN                                         22
#define GPIO_DOUTSET31_0_DIO22_M                                    0x00400000
#define GPIO_DOUTSET31_0_DIO22_S                                            22

// Field:    [21] DIO21
//
// Set bit 21
#define GPIO_DOUTSET31_0_DIO21                                      0x00200000
#define GPIO_DOUTSET31_0_DIO21_BITN                                         21
#define GPIO_DOUTSET31_0_DIO21_M                                    0x00200000
#define GPIO_DOUTSET31_0_DIO21_S                                            21

// Field:    [20] DIO20
//
// Set bit 20
#define GPIO_DOUTSET31_0_DIO20                                      0x00100000
#define GPIO_DOUTSET31_0_DIO20_BITN                                         20
#define GPIO_DOUTSET31_0_DIO20_M                                    0x00100000
#define GPIO_DOUTSET31_0_DIO20_S                                            20

// Field:    [19] DIO19
//
// Set bit 19
#define GPIO_DOUTSET31_0_DIO19                                      0x00080000
#define GPIO_DOUTSET31_0_DIO19_BITN                                         19
#define GPIO_DOUTSET31_0_DIO19_M                                    0x00080000
#define GPIO_DOUTSET31_0_DIO19_S                                            19

// Field:    [18] DIO18
//
// Set bit 18
#define GPIO_DOUTSET31_0_DIO18                                      0x00040000
#define GPIO_DOUTSET31_0_DIO18_BITN                                         18
#define GPIO_DOUTSET31_0_DIO18_M                                    0x00040000
#define GPIO_DOUTSET31_0_DIO18_S                                            18

// Field:    [17] DIO17
//
// Set bit 17
#define GPIO_DOUTSET31_0_DIO17                                      0x00020000
#define GPIO_DOUTSET31_0_DIO17_BITN                                         17
#define GPIO_DOUTSET31_0_DIO17_M                                    0x00020000
#define GPIO_DOUTSET31_0_DIO17_S                                            17

// Field:    [16] DIO16
//
// Set bit 16
#define GPIO_DOUTSET31_0_DIO16                                      0x00010000
#define GPIO_DOUTSET31_0_DIO16_BITN                                         16
#define GPIO_DOUTSET31_0_DIO16_M                                    0x00010000
#define GPIO_DOUTSET31_0_DIO16_S                                            16

// Field:    [15] DIO15
//
// Set bit 15
#define GPIO_DOUTSET31_0_DIO15                                      0x00008000
#define GPIO_DOUTSET31_0_DIO15_BITN                                         15
#define GPIO_DOUTSET31_0_DIO15_M                                    0x00008000
#define GPIO_DOUTSET31_0_DIO15_S                                            15

// Field:    [14] DIO14
//
// Set bit 14
#define GPIO_DOUTSET31_0_DIO14                                      0x00004000
#define GPIO_DOUTSET31_0_DIO14_BITN                                         14
#define GPIO_DOUTSET31_0_DIO14_M                                    0x00004000
#define GPIO_DOUTSET31_0_DIO14_S                                            14

// Field:    [13] DIO13
//
// Set bit 13
#define GPIO_DOUTSET31_0_DIO13                                      0x00002000
#define GPIO_DOUTSET31_0_DIO13_BITN                                         13
#define GPIO_DOUTSET31_0_DIO13_M                                    0x00002000
#define GPIO_DOUTSET31_0_DIO13_S                                            13

// Field:    [12] DIO12
//
// Set bit 12
#define GPIO_DOUTSET31_0_DIO12                                      0x00001000
#define GPIO_DOUTSET31_0_DIO12_BITN                                         12
#define GPIO_DOUTSET31_0_DIO12_M                                    0x00001000
#define GPIO_DOUTSET31_0_DIO12_S                                            12

// Field:    [11] DIO11
//
// Set bit 11
#define GPIO_DOUTSET31_0_DIO11                                      0x00000800
#define GPIO_DOUTSET31_0_DIO11_BITN                                         11
#define GPIO_DOUTSET31_0_DIO11_M                                    0x00000800
#define GPIO_DOUTSET31_0_DIO11_S                                            11

// Field:    [10] DIO10
//
// Set bit 10
#define GPIO_DOUTSET31_0_DIO10                                      0x00000400
#define GPIO_DOUTSET31_0_DIO10_BITN                                         10
#define GPIO_DOUTSET31_0_DIO10_M                                    0x00000400
#define GPIO_DOUTSET31_0_DIO10_S                                            10

// Field:     [9] DIO9
//
// Set bit 9
#define GPIO_DOUTSET31_0_DIO9                                       0x00000200
#define GPIO_DOUTSET31_0_DIO9_BITN                                           9
#define GPIO_DOUTSET31_0_DIO9_M                                     0x00000200
#define GPIO_DOUTSET31_0_DIO9_S                                              9

// Field:     [8] DIO8
//
// Set bit 8
#define GPIO_DOUTSET31_0_DIO8                                       0x00000100
#define GPIO_DOUTSET31_0_DIO8_BITN                                           8
#define GPIO_DOUTSET31_0_DIO8_M                                     0x00000100
#define GPIO_DOUTSET31_0_DIO8_S                                              8

// Field:     [7] DIO7
//
// Set bit 7
#define GPIO_DOUTSET31_0_DIO7                                       0x00000080
#define GPIO_DOUTSET31_0_DIO7_BITN                                           7
#define GPIO_DOUTSET31_0_DIO7_M                                     0x00000080
#define GPIO_DOUTSET31_0_DIO7_S                                              7

// Field:     [6] DIO6
//
// Set bit 6
#define GPIO_DOUTSET31_0_DIO6                                       0x00000040
#define GPIO_DOUTSET31_0_DIO6_BITN                                           6
#define GPIO_DOUTSET31_0_DIO6_M                                     0x00000040
#define GPIO_DOUTSET31_0_DIO6_S                                              6

// Field:     [5] DIO5
//
// Set bit 5
#define GPIO_DOUTSET31_0_DIO5                                       0x00000020
#define GPIO_DOUTSET31_0_DIO5_BITN                                           5
#define GPIO_DOUTSET31_0_DIO5_M                                     0x00000020
#define GPIO_DOUTSET31_0_DIO5_S                                              5

// Field:     [4] DIO4
//
// Set bit 4
#define GPIO_DOUTSET31_0_DIO4                                       0x00000010
#define GPIO_DOUTSET31_0_DIO4_BITN                                           4
#define GPIO_DOUTSET31_0_DIO4_M                                     0x00000010
#define GPIO_DOUTSET31_0_DIO4_S                                              4

// Field:     [3] DIO3
//
// Set bit 3
#define GPIO_DOUTSET31_0_DIO3                                       0x00000008
#define GPIO_DOUTSET31_0_DIO3_BITN                                           3
#define GPIO_DOUTSET31_0_DIO3_M                                     0x00000008
#define GPIO_DOUTSET31_0_DIO3_S                                              3

// Field:     [2] DIO2
//
// Set bit 2
#define GPIO_DOUTSET31_0_DIO2                                       0x00000004
#define GPIO_DOUTSET31_0_DIO2_BITN                                           2
#define GPIO_DOUTSET31_0_DIO2_M                                     0x00000004
#define GPIO_DOUTSET31_0_DIO2_S                                              2

// Field:     [1] DIO1
//
// Set bit 1
#define GPIO_DOUTSET31_0_DIO1                                       0x00000002
#define GPIO_DOUTSET31_0_DIO1_BITN                                           1
#define GPIO_DOUTSET31_0_DIO1_M                                     0x00000002
#define GPIO_DOUTSET31_0_DIO1_S                                              1

// Field:     [0] DIO0
//
// Set bit 0
#define GPIO_DOUTSET31_0_DIO0                                       0x00000001
#define GPIO_DOUTSET31_0_DIO0_BITN                                           0
#define GPIO_DOUTSET31_0_DIO0_M                                     0x00000001
#define GPIO_DOUTSET31_0_DIO0_S                                              0

//*****************************************************************************
//
// Register: GPIO_O_DOUTCLR31_0
//
//*****************************************************************************
// Field:    [31] DIO31
//
// Clears bit 31
#define GPIO_DOUTCLR31_0_DIO31                                      0x80000000
#define GPIO_DOUTCLR31_0_DIO31_BITN                                         31
#define GPIO_DOUTCLR31_0_DIO31_M                                    0x80000000
#define GPIO_DOUTCLR31_0_DIO31_S                                            31

// Field:    [30] DIO30
//
// Clears bit 30
#define GPIO_DOUTCLR31_0_DIO30                                      0x40000000
#define GPIO_DOUTCLR31_0_DIO30_BITN                                         30
#define GPIO_DOUTCLR31_0_DIO30_M                                    0x40000000
#define GPIO_DOUTCLR31_0_DIO30_S                                            30

// Field:    [29] DIO29
//
// Clears bit 29
#define GPIO_DOUTCLR31_0_DIO29                                      0x20000000
#define GPIO_DOUTCLR31_0_DIO29_BITN                                         29
#define GPIO_DOUTCLR31_0_DIO29_M                                    0x20000000
#define GPIO_DOUTCLR31_0_DIO29_S                                            29

// Field:    [28] DIO28
//
// Clears bit 28
#define GPIO_DOUTCLR31_0_DIO28                                      0x10000000
#define GPIO_DOUTCLR31_0_DIO28_BITN                                         28
#define GPIO_DOUTCLR31_0_DIO28_M                                    0x10000000
#define GPIO_DOUTCLR31_0_DIO28_S                                            28

// Field:    [27] DIO27
//
// Clears bit 27
#define GPIO_DOUTCLR31_0_DIO27                                      0x08000000
#define GPIO_DOUTCLR31_0_DIO27_BITN                                         27
#define GPIO_DOUTCLR31_0_DIO27_M                                    0x08000000
#define GPIO_DOUTCLR31_0_DIO27_S                                            27

// Field:    [26] DIO26
//
// Clears bit 26
#define GPIO_DOUTCLR31_0_DIO26                                      0x04000000
#define GPIO_DOUTCLR31_0_DIO26_BITN                                         26
#define GPIO_DOUTCLR31_0_DIO26_M                                    0x04000000
#define GPIO_DOUTCLR31_0_DIO26_S                                            26

// Field:    [25] DIO25
//
// Clears bit 25
#define GPIO_DOUTCLR31_0_DIO25                                      0x02000000
#define GPIO_DOUTCLR31_0_DIO25_BITN                                         25
#define GPIO_DOUTCLR31_0_DIO25_M                                    0x02000000
#define GPIO_DOUTCLR31_0_DIO25_S                                            25

// Field:    [24] DIO24
//
// Clears bit 24
#define GPIO_DOUTCLR31_0_DIO24                                      0x01000000
#define GPIO_DOUTCLR31_0_DIO24_BITN                                         24
#define GPIO_DOUTCLR31_0_DIO24_M                                    0x01000000
#define GPIO_DOUTCLR31_0_DIO24_S                                            24

// Field:    [23] DIO23
//
// Clears bit 23
#define GPIO_DOUTCLR31_0_DIO23                                      0x00800000
#define GPIO_DOUTCLR31_0_DIO23_BITN                                         23
#define GPIO_DOUTCLR31_0_DIO23_M                                    0x00800000
#define GPIO_DOUTCLR31_0_DIO23_S                                            23

// Field:    [22] DIO22
//
// Clears bit 22
#define GPIO_DOUTCLR31_0_DIO22                                      0x00400000
#define GPIO_DOUTCLR31_0_DIO22_BITN                                         22
#define GPIO_DOUTCLR31_0_DIO22_M                                    0x00400000
#define GPIO_DOUTCLR31_0_DIO22_S                                            22

// Field:    [21] DIO21
//
// Clears bit 21
#define GPIO_DOUTCLR31_0_DIO21                                      0x00200000
#define GPIO_DOUTCLR31_0_DIO21_BITN                                         21
#define GPIO_DOUTCLR31_0_DIO21_M                                    0x00200000
#define GPIO_DOUTCLR31_0_DIO21_S                                            21

// Field:    [20] DIO20
//
// Clears bit 20
#define GPIO_DOUTCLR31_0_DIO20                                      0x00100000
#define GPIO_DOUTCLR31_0_DIO20_BITN                                         20
#define GPIO_DOUTCLR31_0_DIO20_M                                    0x00100000
#define GPIO_DOUTCLR31_0_DIO20_S                                            20

// Field:    [19] DIO19
//
// Clears bit 19
#define GPIO_DOUTCLR31_0_DIO19                                      0x00080000
#define GPIO_DOUTCLR31_0_DIO19_BITN                                         19
#define GPIO_DOUTCLR31_0_DIO19_M                                    0x00080000
#define GPIO_DOUTCLR31_0_DIO19_S                                            19

// Field:    [18] DIO18
//
// Clears bit 18
#define GPIO_DOUTCLR31_0_DIO18                                      0x00040000
#define GPIO_DOUTCLR31_0_DIO18_BITN                                         18
#define GPIO_DOUTCLR31_0_DIO18_M                                    0x00040000
#define GPIO_DOUTCLR31_0_DIO18_S                                            18

// Field:    [17] DIO17
//
// Clears bit 17
#define GPIO_DOUTCLR31_0_DIO17                                      0x00020000
#define GPIO_DOUTCLR31_0_DIO17_BITN                                         17
#define GPIO_DOUTCLR31_0_DIO17_M                                    0x00020000
#define GPIO_DOUTCLR31_0_DIO17_S                                            17

// Field:    [16] DIO16
//
// Clears bit 16
#define GPIO_DOUTCLR31_0_DIO16                                      0x00010000
#define GPIO_DOUTCLR31_0_DIO16_BITN                                         16
#define GPIO_DOUTCLR31_0_DIO16_M                                    0x00010000
#define GPIO_DOUTCLR31_0_DIO16_S                                            16

// Field:    [15] DIO15
//
// Clears bit 15
#define GPIO_DOUTCLR31_0_DIO15                                      0x00008000
#define GPIO_DOUTCLR31_0_DIO15_BITN                                         15
#define GPIO_DOUTCLR31_0_DIO15_M                                    0x00008000
#define GPIO_DOUTCLR31_0_DIO15_S                                            15

// Field:    [14] DIO14
//
// Clears bit 14
#define GPIO_DOUTCLR31_0_DIO14                                      0x00004000
#define GPIO_DOUTCLR31_0_DIO14_BITN                                         14
#define GPIO_DOUTCLR31_0_DIO14_M                                    0x00004000
#define GPIO_DOUTCLR31_0_DIO14_S                                            14

// Field:    [13] DIO13
//
// Clears bit 13
#define GPIO_DOUTCLR31_0_DIO13                                      0x00002000
#define GPIO_DOUTCLR31_0_DIO13_BITN                                         13
#define GPIO_DOUTCLR31_0_DIO13_M                                    0x00002000
#define GPIO_DOUTCLR31_0_DIO13_S                                            13

// Field:    [12] DIO12
//
// Clears bit 12
#define GPIO_DOUTCLR31_0_DIO12                                      0x00001000
#define GPIO_DOUTCLR31_0_DIO12_BITN                                         12
#define GPIO_DOUTCLR31_0_DIO12_M                                    0x00001000
#define GPIO_DOUTCLR31_0_DIO12_S                                            12

// Field:    [11] DIO11
//
// Clears bit 11
#define GPIO_DOUTCLR31_0_DIO11                                      0x00000800
#define GPIO_DOUTCLR31_0_DIO11_BITN                                         11
#define GPIO_DOUTCLR31_0_DIO11_M                                    0x00000800
#define GPIO_DOUTCLR31_0_DIO11_S                                            11

// Field:    [10] DIO10
//
// Clears bit 10
#define GPIO_DOUTCLR31_0_DIO10                                      0x00000400
#define GPIO_DOUTCLR31_0_DIO10_BITN                                         10
#define GPIO_DOUTCLR31_0_DIO10_M                                    0x00000400
#define GPIO_DOUTCLR31_0_DIO10_S                                            10

// Field:     [9] DIO9
//
// Clears bit 9
#define GPIO_DOUTCLR31_0_DIO9                                       0x00000200
#define GPIO_DOUTCLR31_0_DIO9_BITN                                           9
#define GPIO_DOUTCLR31_0_DIO9_M                                     0x00000200
#define GPIO_DOUTCLR31_0_DIO9_S                                              9

// Field:     [8] DIO8
//
// Clears bit 8
#define GPIO_DOUTCLR31_0_DIO8                                       0x00000100
#define GPIO_DOUTCLR31_0_DIO8_BITN                                           8
#define GPIO_DOUTCLR31_0_DIO8_M                                     0x00000100
#define GPIO_DOUTCLR31_0_DIO8_S                                              8

// Field:     [7] DIO7
//
// Clears bit 7
#define GPIO_DOUTCLR31_0_DIO7                                       0x00000080
#define GPIO_DOUTCLR31_0_DIO7_BITN                                           7
#define GPIO_DOUTCLR31_0_DIO7_M                                     0x00000080
#define GPIO_DOUTCLR31_0_DIO7_S                                              7

// Field:     [6] DIO6
//
// Clears bit 6
#define GPIO_DOUTCLR31_0_DIO6                                       0x00000040
#define GPIO_DOUTCLR31_0_DIO6_BITN                                           6
#define GPIO_DOUTCLR31_0_DIO6_M                                     0x00000040
#define GPIO_DOUTCLR31_0_DIO6_S                                              6

// Field:     [5] DIO5
//
// Clears bit 5
#define GPIO_DOUTCLR31_0_DIO5                                       0x00000020
#define GPIO_DOUTCLR31_0_DIO5_BITN                                           5
#define GPIO_DOUTCLR31_0_DIO5_M                                     0x00000020
#define GPIO_DOUTCLR31_0_DIO5_S                                              5

// Field:     [4] DIO4
//
// Clears bit 4
#define GPIO_DOUTCLR31_0_DIO4                                       0x00000010
#define GPIO_DOUTCLR31_0_DIO4_BITN                                           4
#define GPIO_DOUTCLR31_0_DIO4_M                                     0x00000010
#define GPIO_DOUTCLR31_0_DIO4_S                                              4

// Field:     [3] DIO3
//
// Clears bit 3
#define GPIO_DOUTCLR31_0_DIO3                                       0x00000008
#define GPIO_DOUTCLR31_0_DIO3_BITN                                           3
#define GPIO_DOUTCLR31_0_DIO3_M                                     0x00000008
#define GPIO_DOUTCLR31_0_DIO3_S                                              3

// Field:     [2] DIO2
//
// Clears bit 2
#define GPIO_DOUTCLR31_0_DIO2                                       0x00000004
#define GPIO_DOUTCLR31_0_DIO2_BITN                                           2
#define GPIO_DOUTCLR31_0_DIO2_M                                     0x00000004
#define GPIO_DOUTCLR31_0_DIO2_S                                              2

// Field:     [1] DIO1
//
// Clears bit 1
#define GPIO_DOUTCLR31_0_DIO1                                       0x00000002
#define GPIO_DOUTCLR31_0_DIO1_BITN                                           1
#define GPIO_DOUTCLR31_0_DIO1_M                                     0x00000002
#define GPIO_DOUTCLR31_0_DIO1_S                                              1

// Field:     [0] DIO0
//
// Clears bit 0
#define GPIO_DOUTCLR31_0_DIO0                                       0x00000001
#define GPIO_DOUTCLR31_0_DIO0_BITN                                           0
#define GPIO_DOUTCLR31_0_DIO0_M                                     0x00000001
#define GPIO_DOUTCLR31_0_DIO0_S                                              0

//*****************************************************************************
//
// Register: GPIO_O_DOUTTGL31_0
//
//*****************************************************************************
// Field:    [31] DIO31
//
// Toggles bit 31
#define GPIO_DOUTTGL31_0_DIO31                                      0x80000000
#define GPIO_DOUTTGL31_0_DIO31_BITN                                         31
#define GPIO_DOUTTGL31_0_DIO31_M                                    0x80000000
#define GPIO_DOUTTGL31_0_DIO31_S                                            31

// Field:    [30] DIO30
//
// Toggles bit 30
#define GPIO_DOUTTGL31_0_DIO30                                      0x40000000
#define GPIO_DOUTTGL31_0_DIO30_BITN                                         30
#define GPIO_DOUTTGL31_0_DIO30_M                                    0x40000000
#define GPIO_DOUTTGL31_0_DIO30_S                                            30

// Field:    [29] DIO29
//
// Toggles bit 29
#define GPIO_DOUTTGL31_0_DIO29                                      0x20000000
#define GPIO_DOUTTGL31_0_DIO29_BITN                                         29
#define GPIO_DOUTTGL31_0_DIO29_M                                    0x20000000
#define GPIO_DOUTTGL31_0_DIO29_S                                            29

// Field:    [28] DIO28
//
// Toggles bit 28
#define GPIO_DOUTTGL31_0_DIO28                                      0x10000000
#define GPIO_DOUTTGL31_0_DIO28_BITN                                         28
#define GPIO_DOUTTGL31_0_DIO28_M                                    0x10000000
#define GPIO_DOUTTGL31_0_DIO28_S                                            28

// Field:    [27] DIO27
//
// Toggles bit 27
#define GPIO_DOUTTGL31_0_DIO27                                      0x08000000
#define GPIO_DOUTTGL31_0_DIO27_BITN                                         27
#define GPIO_DOUTTGL31_0_DIO27_M                                    0x08000000
#define GPIO_DOUTTGL31_0_DIO27_S                                            27

// Field:    [26] DIO26
//
// Toggles bit 26
#define GPIO_DOUTTGL31_0_DIO26                                      0x04000000
#define GPIO_DOUTTGL31_0_DIO26_BITN                                         26
#define GPIO_DOUTTGL31_0_DIO26_M                                    0x04000000
#define GPIO_DOUTTGL31_0_DIO26_S                                            26

// Field:    [25] DIO25
//
// Toggles bit 25
#define GPIO_DOUTTGL31_0_DIO25                                      0x02000000
#define GPIO_DOUTTGL31_0_DIO25_BITN                                         25
#define GPIO_DOUTTGL31_0_DIO25_M                                    0x02000000
#define GPIO_DOUTTGL31_0_DIO25_S                                            25

// Field:    [24] DIO24
//
// Toggles bit 24
#define GPIO_DOUTTGL31_0_DIO24                                      0x01000000
#define GPIO_DOUTTGL31_0_DIO24_BITN                                         24
#define GPIO_DOUTTGL31_0_DIO24_M                                    0x01000000
#define GPIO_DOUTTGL31_0_DIO24_S                                            24

// Field:    [23] DIO23
//
// Toggles bit 23
#define GPIO_DOUTTGL31_0_DIO23                                      0x00800000
#define GPIO_DOUTTGL31_0_DIO23_BITN                                         23
#define GPIO_DOUTTGL31_0_DIO23_M                                    0x00800000
#define GPIO_DOUTTGL31_0_DIO23_S                                            23

// Field:    [22] DIO22
//
// Toggles bit 22
#define GPIO_DOUTTGL31_0_DIO22                                      0x00400000
#define GPIO_DOUTTGL31_0_DIO22_BITN                                         22
#define GPIO_DOUTTGL31_0_DIO22_M                                    0x00400000
#define GPIO_DOUTTGL31_0_DIO22_S                                            22

// Field:    [21] DIO21
//
// Toggles bit 21
#define GPIO_DOUTTGL31_0_DIO21                                      0x00200000
#define GPIO_DOUTTGL31_0_DIO21_BITN                                         21
#define GPIO_DOUTTGL31_0_DIO21_M                                    0x00200000
#define GPIO_DOUTTGL31_0_DIO21_S                                            21

// Field:    [20] DIO20
//
// Toggles bit 20
#define GPIO_DOUTTGL31_0_DIO20                                      0x00100000
#define GPIO_DOUTTGL31_0_DIO20_BITN                                         20
#define GPIO_DOUTTGL31_0_DIO20_M                                    0x00100000
#define GPIO_DOUTTGL31_0_DIO20_S                                            20

// Field:    [19] DIO19
//
// Toggles bit 19
#define GPIO_DOUTTGL31_0_DIO19                                      0x00080000
#define GPIO_DOUTTGL31_0_DIO19_BITN                                         19
#define GPIO_DOUTTGL31_0_DIO19_M                                    0x00080000
#define GPIO_DOUTTGL31_0_DIO19_S                                            19

// Field:    [18] DIO18
//
// Toggles bit 18
#define GPIO_DOUTTGL31_0_DIO18                                      0x00040000
#define GPIO_DOUTTGL31_0_DIO18_BITN                                         18
#define GPIO_DOUTTGL31_0_DIO18_M                                    0x00040000
#define GPIO_DOUTTGL31_0_DIO18_S                                            18

// Field:    [17] DIO17
//
// Toggles bit 17
#define GPIO_DOUTTGL31_0_DIO17                                      0x00020000
#define GPIO_DOUTTGL31_0_DIO17_BITN                                         17
#define GPIO_DOUTTGL31_0_DIO17_M                                    0x00020000
#define GPIO_DOUTTGL31_0_DIO17_S                                            17

// Field:    [16] DIO16
//
// Toggles bit 16
#define GPIO_DOUTTGL31_0_DIO16                                      0x00010000
#define GPIO_DOUTTGL31_0_DIO16_BITN                                         16
#define GPIO_DOUTTGL31_0_DIO16_M                                    0x00010000
#define GPIO_DOUTTGL31_0_DIO16_S                                            16

// Field:    [15] DIO15
//
// Toggles bit 15
#define GPIO_DOUTTGL31_0_DIO15                                      0x00008000
#define GPIO_DOUTTGL31_0_DIO15_BITN                                         15
#define GPIO_DOUTTGL31_0_DIO15_M                                    0x00008000
#define GPIO_DOUTTGL31_0_DIO15_S                                            15

// Field:    [14] DIO14
//
// Toggles bit 14
#define GPIO_DOUTTGL31_0_DIO14                                      0x00004000
#define GPIO_DOUTTGL31_0_DIO14_BITN                                         14
#define GPIO_DOUTTGL31_0_DIO14_M                                    0x00004000
#define GPIO_DOUTTGL31_0_DIO14_S                                            14

// Field:    [13] DIO13
//
// Toggles bit 13
#define GPIO_DOUTTGL31_0_DIO13                                      0x00002000
#define GPIO_DOUTTGL31_0_DIO13_BITN                                         13
#define GPIO_DOUTTGL31_0_DIO13_M                                    0x00002000
#define GPIO_DOUTTGL31_0_DIO13_S                                            13

// Field:    [12] DIO12
//
// Toggles bit 12
#define GPIO_DOUTTGL31_0_DIO12                                      0x00001000
#define GPIO_DOUTTGL31_0_DIO12_BITN                                         12
#define GPIO_DOUTTGL31_0_DIO12_M                                    0x00001000
#define GPIO_DOUTTGL31_0_DIO12_S                                            12

// Field:    [11] DIO11
//
// Toggles bit 11
#define GPIO_DOUTTGL31_0_DIO11                                      0x00000800
#define GPIO_DOUTTGL31_0_DIO11_BITN                                         11
#define GPIO_DOUTTGL31_0_DIO11_M                                    0x00000800
#define GPIO_DOUTTGL31_0_DIO11_S                                            11

// Field:    [10] DIO10
//
// Toggles bit 10
#define GPIO_DOUTTGL31_0_DIO10                                      0x00000400
#define GPIO_DOUTTGL31_0_DIO10_BITN                                         10
#define GPIO_DOUTTGL31_0_DIO10_M                                    0x00000400
#define GPIO_DOUTTGL31_0_DIO10_S                                            10

// Field:     [9] DIO9
//
// Toggles bit 9
#define GPIO_DOUTTGL31_0_DIO9                                       0x00000200
#define GPIO_DOUTTGL31_0_DIO9_BITN                                           9
#define GPIO_DOUTTGL31_0_DIO9_M                                     0x00000200
#define GPIO_DOUTTGL31_0_DIO9_S                                              9

// Field:     [8] DIO8
//
// Toggles bit 8
#define GPIO_DOUTTGL31_0_DIO8                                       0x00000100
#define GPIO_DOUTTGL31_0_DIO8_BITN                                           8
#define GPIO_DOUTTGL31_0_DIO8_M                                     0x00000100
#define GPIO_DOUTTGL31_0_DIO8_S                                              8

// Field:     [7] DIO7
//
// Toggles bit 7
#define GPIO_DOUTTGL31_0_DIO7                                       0x00000080
#define GPIO_DOUTTGL31_0_DIO7_BITN                                           7
#define GPIO_DOUTTGL31_0_DIO7_M                                     0x00000080
#define GPIO_DOUTTGL31_0_DIO7_S                                              7

// Field:     [6] DIO6
//
// Toggles bit 6
#define GPIO_DOUTTGL31_0_DIO6                                       0x00000040
#define GPIO_DOUTTGL31_0_DIO6_BITN                                           6
#define GPIO_DOUTTGL31_0_DIO6_M                                     0x00000040
#define GPIO_DOUTTGL31_0_DIO6_S                                              6

// Field:     [5] DIO5
//
// Toggles bit 5
#define GPIO_DOUTTGL31_0_DIO5                                       0x00000020
#define GPIO_DOUTTGL31_0_DIO5_BITN                                           5
#define GPIO_DOUTTGL31_0_DIO5_M                                     0x00000020
#define GPIO_DOUTTGL31_0_DIO5_S                                              5

// Field:     [4] DIO4
//
// Toggles bit 4
#define GPIO_DOUTTGL31_0_DIO4                                       0x00000010
#define GPIO_DOUTTGL31_0_DIO4_BITN                                           4
#define GPIO_DOUTTGL31_0_DIO4_M                                     0x00000010
#define GPIO_DOUTTGL31_0_DIO4_S                                              4

// Field:     [3] DIO3
//
// Toggles bit 3
#define GPIO_DOUTTGL31_0_DIO3                                       0x00000008
#define GPIO_DOUTTGL31_0_DIO3_BITN                                           3
#define GPIO_DOUTTGL31_0_DIO3_M                                     0x00000008
#define GPIO_DOUTTGL31_0_DIO3_S                                              3

// Field:     [2] DIO2
//
// Toggles bit 2
#define GPIO_DOUTTGL31_0_DIO2                                       0x00000004
#define GPIO_DOUTTGL31_0_DIO2_BITN                                           2
#define GPIO_DOUTTGL31_0_DIO2_M                                     0x00000004
#define GPIO_DOUTTGL31_0_DIO2_S                                              2

// Field:     [1] DIO1
//
// Toggles bit 1
#define GPIO_DOUTTGL31_0_DIO1                                       0x00000002
#define GPIO_DOUTTGL31_0_DIO1_BITN                                           1
#define GPIO_DOUTTGL31_0_DIO1_M                                     0x00000002
#define GPIO_DOUTTGL31_0_DIO1_S                                              1

// Field:     [0] DIO0
//
// Toggles bit 0
#define GPIO_DOUTTGL31_0_DIO0                                       0x00000001
#define GPIO_DOUTTGL31_0_DIO0_BITN                                           0
#define GPIO_DOUTTGL31_0_DIO0_M                                     0x00000001
#define GPIO_DOUTTGL31_0_DIO0_S                                              0

//*****************************************************************************
//
// Register: GPIO_O_DIN31_0
//
//*****************************************************************************
// Field:    [31] DIO31
//
// Data input from DIO 31
#define GPIO_DIN31_0_DIO31                                          0x80000000
#define GPIO_DIN31_0_DIO31_BITN                                             31
#define GPIO_DIN31_0_DIO31_M                                        0x80000000
#define GPIO_DIN31_0_DIO31_S                                                31

// Field:    [30] DIO30
//
// Data input from DIO 30
#define GPIO_DIN31_0_DIO30                                          0x40000000
#define GPIO_DIN31_0_DIO30_BITN                                             30
#define GPIO_DIN31_0_DIO30_M                                        0x40000000
#define GPIO_DIN31_0_DIO30_S                                                30

// Field:    [29] DIO29
//
// Data input from DIO 29
#define GPIO_DIN31_0_DIO29                                          0x20000000
#define GPIO_DIN31_0_DIO29_BITN                                             29
#define GPIO_DIN31_0_DIO29_M                                        0x20000000
#define GPIO_DIN31_0_DIO29_S                                                29

// Field:    [28] DIO28
//
// Data input from DIO 28
#define GPIO_DIN31_0_DIO28                                          0x10000000
#define GPIO_DIN31_0_DIO28_BITN                                             28
#define GPIO_DIN31_0_DIO28_M                                        0x10000000
#define GPIO_DIN31_0_DIO28_S                                                28

// Field:    [27] DIO27
//
// Data input from DIO 27
#define GPIO_DIN31_0_DIO27                                          0x08000000
#define GPIO_DIN31_0_DIO27_BITN                                             27
#define GPIO_DIN31_0_DIO27_M                                        0x08000000
#define GPIO_DIN31_0_DIO27_S                                                27

// Field:    [26] DIO26
//
// Data input from DIO 26
#define GPIO_DIN31_0_DIO26                                          0x04000000
#define GPIO_DIN31_0_DIO26_BITN                                             26
#define GPIO_DIN31_0_DIO26_M                                        0x04000000
#define GPIO_DIN31_0_DIO26_S                                                26

// Field:    [25] DIO25
//
// Data input from DIO 25
#define GPIO_DIN31_0_DIO25                                          0x02000000
#define GPIO_DIN31_0_DIO25_BITN                                             25
#define GPIO_DIN31_0_DIO25_M                                        0x02000000
#define GPIO_DIN31_0_DIO25_S                                                25

// Field:    [24] DIO24
//
// Data input from DIO 24
#define GPIO_DIN31_0_DIO24                                          0x01000000
#define GPIO_DIN31_0_DIO24_BITN                                             24
#define GPIO_DIN31_0_DIO24_M                                        0x01000000
#define GPIO_DIN31_0_DIO24_S                                                24

// Field:    [23] DIO23
//
// Data input from DIO 23
#define GPIO_DIN31_0_DIO23                                          0x00800000
#define GPIO_DIN31_0_DIO23_BITN                                             23
#define GPIO_DIN31_0_DIO23_M                                        0x00800000
#define GPIO_DIN31_0_DIO23_S                                                23

// Field:    [22] DIO22
//
// Data input from DIO 22
#define GPIO_DIN31_0_DIO22                                          0x00400000
#define GPIO_DIN31_0_DIO22_BITN                                             22
#define GPIO_DIN31_0_DIO22_M                                        0x00400000
#define GPIO_DIN31_0_DIO22_S                                                22

// Field:    [21] DIO21
//
// Data input from DIO 21
#define GPIO_DIN31_0_DIO21                                          0x00200000
#define GPIO_DIN31_0_DIO21_BITN                                             21
#define GPIO_DIN31_0_DIO21_M                                        0x00200000
#define GPIO_DIN31_0_DIO21_S                                                21

// Field:    [20] DIO20
//
// Data input from DIO 20
#define GPIO_DIN31_0_DIO20                                          0x00100000
#define GPIO_DIN31_0_DIO20_BITN                                             20
#define GPIO_DIN31_0_DIO20_M                                        0x00100000
#define GPIO_DIN31_0_DIO20_S                                                20

// Field:    [19] DIO19
//
// Data input from DIO 19
#define GPIO_DIN31_0_DIO19                                          0x00080000
#define GPIO_DIN31_0_DIO19_BITN                                             19
#define GPIO_DIN31_0_DIO19_M                                        0x00080000
#define GPIO_DIN31_0_DIO19_S                                                19

// Field:    [18] DIO18
//
// Data input from DIO 18
#define GPIO_DIN31_0_DIO18                                          0x00040000
#define GPIO_DIN31_0_DIO18_BITN                                             18
#define GPIO_DIN31_0_DIO18_M                                        0x00040000
#define GPIO_DIN31_0_DIO18_S                                                18

// Field:    [17] DIO17
//
// Data input from DIO 17
#define GPIO_DIN31_0_DIO17                                          0x00020000
#define GPIO_DIN31_0_DIO17_BITN                                             17
#define GPIO_DIN31_0_DIO17_M                                        0x00020000
#define GPIO_DIN31_0_DIO17_S                                                17

// Field:    [16] DIO16
//
// Data input from DIO 16
#define GPIO_DIN31_0_DIO16                                          0x00010000
#define GPIO_DIN31_0_DIO16_BITN                                             16
#define GPIO_DIN31_0_DIO16_M                                        0x00010000
#define GPIO_DIN31_0_DIO16_S                                                16

// Field:    [15] DIO15
//
// Data input from DIO 15
#define GPIO_DIN31_0_DIO15                                          0x00008000
#define GPIO_DIN31_0_DIO15_BITN                                             15
#define GPIO_DIN31_0_DIO15_M                                        0x00008000
#define GPIO_DIN31_0_DIO15_S                                                15

// Field:    [14] DIO14
//
// Data input from DIO 14
#define GPIO_DIN31_0_DIO14                                          0x00004000
#define GPIO_DIN31_0_DIO14_BITN                                             14
#define GPIO_DIN31_0_DIO14_M                                        0x00004000
#define GPIO_DIN31_0_DIO14_S                                                14

// Field:    [13] DIO13
//
// Data input from DIO 13
#define GPIO_DIN31_0_DIO13                                          0x00002000
#define GPIO_DIN31_0_DIO13_BITN                                             13
#define GPIO_DIN31_0_DIO13_M                                        0x00002000
#define GPIO_DIN31_0_DIO13_S                                                13

// Field:    [12] DIO12
//
// Data input from DIO 12
#define GPIO_DIN31_0_DIO12                                          0x00001000
#define GPIO_DIN31_0_DIO12_BITN                                             12
#define GPIO_DIN31_0_DIO12_M                                        0x00001000
#define GPIO_DIN31_0_DIO12_S                                                12

// Field:    [11] DIO11
//
// Data input from DIO 11
#define GPIO_DIN31_0_DIO11                                          0x00000800
#define GPIO_DIN31_0_DIO11_BITN                                             11
#define GPIO_DIN31_0_DIO11_M                                        0x00000800
#define GPIO_DIN31_0_DIO11_S                                                11

// Field:    [10] DIO10
//
// Data input from DIO 10
#define GPIO_DIN31_0_DIO10                                          0x00000400
#define GPIO_DIN31_0_DIO10_BITN                                             10
#define GPIO_DIN31_0_DIO10_M                                        0x00000400
#define GPIO_DIN31_0_DIO10_S                                                10

// Field:     [9] DIO9
//
// Data input from DIO 9
#define GPIO_DIN31_0_DIO9                                           0x00000200
#define GPIO_DIN31_0_DIO9_BITN                                               9
#define GPIO_DIN31_0_DIO9_M                                         0x00000200
#define GPIO_DIN31_0_DIO9_S                                                  9

// Field:     [8] DIO8
//
// Data input from DIO 8
#define GPIO_DIN31_0_DIO8                                           0x00000100
#define GPIO_DIN31_0_DIO8_BITN                                               8
#define GPIO_DIN31_0_DIO8_M                                         0x00000100
#define GPIO_DIN31_0_DIO8_S                                                  8

// Field:     [7] DIO7
//
// Data input from DIO 7
#define GPIO_DIN31_0_DIO7                                           0x00000080
#define GPIO_DIN31_0_DIO7_BITN                                               7
#define GPIO_DIN31_0_DIO7_M                                         0x00000080
#define GPIO_DIN31_0_DIO7_S                                                  7

// Field:     [6] DIO6
//
// Data input from DIO 6
#define GPIO_DIN31_0_DIO6                                           0x00000040
#define GPIO_DIN31_0_DIO6_BITN                                               6
#define GPIO_DIN31_0_DIO6_M                                         0x00000040
#define GPIO_DIN31_0_DIO6_S                                                  6

// Field:     [5] DIO5
//
// Data input from DIO 5
#define GPIO_DIN31_0_DIO5                                           0x00000020
#define GPIO_DIN31_0_DIO5_BITN                                               5
#define GPIO_DIN31_0_DIO5_M                                         0x00000020
#define GPIO_DIN31_0_DIO5_S                                                  5

// Field:     [4] DIO4
//
// Data input from DIO 4
#define GPIO_DIN31_0_DIO4                                           0x00000010
#define GPIO_DIN31_0_DIO4_BITN                                               4
#define GPIO_DIN31_0_DIO4_M                                         0x00000010
#define GPIO_DIN31_0_DIO4_S                                                  4

// Field:     [3] DIO3
//
// Data input from DIO 3
#define GPIO_DIN31_0_DIO3                                           0x00000008
#define GPIO_DIN31_0_DIO3_BITN                                               3
#define GPIO_DIN31_0_DIO3_M                                         0x00000008
#define GPIO_DIN31_0_DIO3_S                                                  3

// Field:     [2] DIO2
//
// Data input from DIO 2
#define GPIO_DIN31_0_DIO2                                           0x00000004
#define GPIO_DIN31_0_DIO2_BITN                                               2
#define GPIO_DIN31_0_DIO2_M                                         0x00000004
#define GPIO_DIN31_0_DIO2_S                                                  2

// Field:     [1] DIO1
//
// Data input from DIO 1
#define GPIO_DIN31_0_DIO1                                           0x00000002
#define GPIO_DIN31_0_DIO1_BITN                                               1
#define GPIO_DIN31_0_DIO1_M                                         0x00000002
#define GPIO_DIN31_0_DIO1_S                                                  1

// Field:     [0] DIO0
//
// Data input from DIO 0
#define GPIO_DIN31_0_DIO0                                           0x00000001
#define GPIO_DIN31_0_DIO0_BITN                                               0
#define GPIO_DIN31_0_DIO0_M                                         0x00000001
#define GPIO_DIN31_0_DIO0_S                                                  0

//*****************************************************************************
//
// Register: GPIO_O_DOE31_0
//
//*****************************************************************************
// Field:    [31] DIO31
//
// Data output enable for DIO 31
#define GPIO_DOE31_0_DIO31                                          0x80000000
#define GPIO_DOE31_0_DIO31_BITN                                             31
#define GPIO_DOE31_0_DIO31_M                                        0x80000000
#define GPIO_DOE31_0_DIO31_S                                                31

// Field:    [30] DIO30
//
// Data output enable for DIO 30
#define GPIO_DOE31_0_DIO30                                          0x40000000
#define GPIO_DOE31_0_DIO30_BITN                                             30
#define GPIO_DOE31_0_DIO30_M                                        0x40000000
#define GPIO_DOE31_0_DIO30_S                                                30

// Field:    [29] DIO29
//
// Data output enable for DIO 29
#define GPIO_DOE31_0_DIO29                                          0x20000000
#define GPIO_DOE31_0_DIO29_BITN                                             29
#define GPIO_DOE31_0_DIO29_M                                        0x20000000
#define GPIO_DOE31_0_DIO29_S                                                29

// Field:    [28] DIO28
//
// Data output enable for DIO 28
#define GPIO_DOE31_0_DIO28                                          0x10000000
#define GPIO_DOE31_0_DIO28_BITN                                             28
#define GPIO_DOE31_0_DIO28_M                                        0x10000000
#define GPIO_DOE31_0_DIO28_S                                                28

// Field:    [27] DIO27
//
// Data output enable for DIO 27
#define GPIO_DOE31_0_DIO27                                          0x08000000
#define GPIO_DOE31_0_DIO27_BITN                                             27
#define GPIO_DOE31_0_DIO27_M                                        0x08000000
#define GPIO_DOE31_0_DIO27_S                                                27

// Field:    [26] DIO26
//
// Data output enable for DIO 26
#define GPIO_DOE31_0_DIO26                                          0x04000000
#define GPIO_DOE31_0_DIO26_BITN                                             26
#define GPIO_DOE31_0_DIO26_M                                        0x04000000
#define GPIO_DOE31_0_DIO26_S                                                26

// Field:    [25] DIO25
//
// Data output enable for DIO 25
#define GPIO_DOE31_0_DIO25                                          0x02000000
#define GPIO_DOE31_0_DIO25_BITN                                             25
#define GPIO_DOE31_0_DIO25_M                                        0x02000000
#define GPIO_DOE31_0_DIO25_S                                                25

// Field:    [24] DIO24
//
// Data output enable for DIO 24
#define GPIO_DOE31_0_DIO24                                          0x01000000
#define GPIO_DOE31_0_DIO24_BITN                                             24
#define GPIO_DOE31_0_DIO24_M                                        0x01000000
#define GPIO_DOE31_0_DIO24_S                                                24

// Field:    [23] DIO23
//
// Data output enable for DIO 23
#define GPIO_DOE31_0_DIO23                                          0x00800000
#define GPIO_DOE31_0_DIO23_BITN                                             23
#define GPIO_DOE31_0_DIO23_M                                        0x00800000
#define GPIO_DOE31_0_DIO23_S                                                23

// Field:    [22] DIO22
//
// Data output enable for DIO 22
#define GPIO_DOE31_0_DIO22                                          0x00400000
#define GPIO_DOE31_0_DIO22_BITN                                             22
#define GPIO_DOE31_0_DIO22_M                                        0x00400000
#define GPIO_DOE31_0_DIO22_S                                                22

// Field:    [21] DIO21
//
// Data output enable for DIO 21
#define GPIO_DOE31_0_DIO21                                          0x00200000
#define GPIO_DOE31_0_DIO21_BITN                                             21
#define GPIO_DOE31_0_DIO21_M                                        0x00200000
#define GPIO_DOE31_0_DIO21_S                                                21

// Field:    [20] DIO20
//
// Data output enable for DIO 20
#define GPIO_DOE31_0_DIO20                                          0x00100000
#define GPIO_DOE31_0_DIO20_BITN                                             20
#define GPIO_DOE31_0_DIO20_M                                        0x00100000
#define GPIO_DOE31_0_DIO20_S                                                20

// Field:    [19] DIO19
//
// Data output enable for DIO 19
#define GPIO_DOE31_0_DIO19                                          0x00080000
#define GPIO_DOE31_0_DIO19_BITN                                             19
#define GPIO_DOE31_0_DIO19_M                                        0x00080000
#define GPIO_DOE31_0_DIO19_S                                                19

// Field:    [18] DIO18
//
// Data output enable for DIO 18
#define GPIO_DOE31_0_DIO18                                          0x00040000
#define GPIO_DOE31_0_DIO18_BITN                                             18
#define GPIO_DOE31_0_DIO18_M                                        0x00040000
#define GPIO_DOE31_0_DIO18_S                                                18

// Field:    [17] DIO17
//
// Data output enable for DIO 17
#define GPIO_DOE31_0_DIO17                                          0x00020000
#define GPIO_DOE31_0_DIO17_BITN                                             17
#define GPIO_DOE31_0_DIO17_M                                        0x00020000
#define GPIO_DOE31_0_DIO17_S                                                17

// Field:    [16] DIO16
//
// Data output enable for DIO 16
#define GPIO_DOE31_0_DIO16                                          0x00010000
#define GPIO_DOE31_0_DIO16_BITN                                             16
#define GPIO_DOE31_0_DIO16_M                                        0x00010000
#define GPIO_DOE31_0_DIO16_S                                                16

// Field:    [15] DIO15
//
// Data output enable for DIO 15
#define GPIO_DOE31_0_DIO15                                          0x00008000
#define GPIO_DOE31_0_DIO15_BITN                                             15
#define GPIO_DOE31_0_DIO15_M                                        0x00008000
#define GPIO_DOE31_0_DIO15_S                                                15

// Field:    [14] DIO14
//
// Data output enable for DIO 14
#define GPIO_DOE31_0_DIO14                                          0x00004000
#define GPIO_DOE31_0_DIO14_BITN                                             14
#define GPIO_DOE31_0_DIO14_M                                        0x00004000
#define GPIO_DOE31_0_DIO14_S                                                14

// Field:    [13] DIO13
//
// Data output enable for DIO 13
#define GPIO_DOE31_0_DIO13                                          0x00002000
#define GPIO_DOE31_0_DIO13_BITN                                             13
#define GPIO_DOE31_0_DIO13_M                                        0x00002000
#define GPIO_DOE31_0_DIO13_S                                                13

// Field:    [12] DIO12
//
// Data output enable for DIO 12
#define GPIO_DOE31_0_DIO12                                          0x00001000
#define GPIO_DOE31_0_DIO12_BITN                                             12
#define GPIO_DOE31_0_DIO12_M                                        0x00001000
#define GPIO_DOE31_0_DIO12_S                                                12

// Field:    [11] DIO11
//
// Data output enable for DIO 11
#define GPIO_DOE31_0_DIO11                                          0x00000800
#define GPIO_DOE31_0_DIO11_BITN                                             11
#define GPIO_DOE31_0_DIO11_M                                        0x00000800
#define GPIO_DOE31_0_DIO11_S                                                11

// Field:    [10] DIO10
//
// Data output enable for DIO 10
#define GPIO_DOE31_0_DIO10                                          0x00000400
#define GPIO_DOE31_0_DIO10_BITN                                             10
#define GPIO_DOE31_0_DIO10_M                                        0x00000400
#define GPIO_DOE31_0_DIO10_S                                                10

// Field:     [9] DIO9
//
// Data output enable for DIO 9
#define GPIO_DOE31_0_DIO9                                           0x00000200
#define GPIO_DOE31_0_DIO9_BITN                                               9
#define GPIO_DOE31_0_DIO9_M                                         0x00000200
#define GPIO_DOE31_0_DIO9_S                                                  9

// Field:     [8] DIO8
//
// Data output enable for DIO 8
#define GPIO_DOE31_0_DIO8                                           0x00000100
#define GPIO_DOE31_0_DIO8_BITN                                               8
#define GPIO_DOE31_0_DIO8_M                                         0x00000100
#define GPIO_DOE31_0_DIO8_S                                                  8

// Field:     [7] DIO7
//
// Data output enable for DIO 7
#define GPIO_DOE31_0_DIO7                                           0x00000080
#define GPIO_DOE31_0_DIO7_BITN                                               7
#define GPIO_DOE31_0_DIO7_M                                         0x00000080
#define GPIO_DOE31_0_DIO7_S                                                  7

// Field:     [6] DIO6
//
// Data output enable for DIO 6
#define GPIO_DOE31_0_DIO6                                           0x00000040
#define GPIO_DOE31_0_DIO6_BITN                                               6
#define GPIO_DOE31_0_DIO6_M                                         0x00000040
#define GPIO_DOE31_0_DIO6_S                                                  6

// Field:     [5] DIO5
//
// Data output enable for DIO 5
#define GPIO_DOE31_0_DIO5                                           0x00000020
#define GPIO_DOE31_0_DIO5_BITN                                               5
#define GPIO_DOE31_0_DIO5_M                                         0x00000020
#define GPIO_DOE31_0_DIO5_S                                                  5

// Field:     [4] DIO4
//
// Data output enable for DIO 4
#define GPIO_DOE31_0_DIO4                                           0x00000010
#define GPIO_DOE31_0_DIO4_BITN                                               4
#define GPIO_DOE31_0_DIO4_M                                         0x00000010
#define GPIO_DOE31_0_DIO4_S                                                  4

// Field:     [3] DIO3
//
// Data output enable for DIO 3
#define GPIO_DOE31_0_DIO3                                           0x00000008
#define GPIO_DOE31_0_DIO3_BITN                                               3
#define GPIO_DOE31_0_DIO3_M                                         0x00000008
#define GPIO_DOE31_0_DIO3_S                                                  3

// Field:     [2] DIO2
//
// Data output enable for DIO 2
#define GPIO_DOE31_0_DIO2                                           0x00000004
#define GPIO_DOE31_0_DIO2_BITN                                               2
#define GPIO_DOE31_0_DIO2_M                                         0x00000004
#define GPIO_DOE31_0_DIO2_S                                                  2

// Field:     [1] DIO1
//
// Data output enable for DIO 1
#define GPIO_DOE31_0_DIO1                                           0x00000002
#define GPIO_DOE31_0_DIO1_BITN                                               1
#define GPIO_DOE31_0_DIO1_M                                         0x00000002
#define GPIO_DOE31_0_DIO1_S                                                  1

// Field:     [0] DIO0
//
// Data output enable for DIO 0
#define GPIO_DOE31_0_DIO0                                           0x00000001
#define GPIO_DOE31_0_DIO0_BITN                                               0
#define GPIO_DOE31_0_DIO0_M                                         0x00000001
#define GPIO_DOE31_0_DIO0_S                                                  0

//*****************************************************************************
//
// Register: GPIO_O_EVFLAGS31_0
//
//*****************************************************************************
// Field:    [31] DIO31
//
// Event for DIO 31
#define GPIO_EVFLAGS31_0_DIO31                                      0x80000000
#define GPIO_EVFLAGS31_0_DIO31_BITN                                         31
#define GPIO_EVFLAGS31_0_DIO31_M                                    0x80000000
#define GPIO_EVFLAGS31_0_DIO31_S                                            31

// Field:    [30] DIO30
//
// Event for DIO 30
#define GPIO_EVFLAGS31_0_DIO30                                      0x40000000
#define GPIO_EVFLAGS31_0_DIO30_BITN                                         30
#define GPIO_EVFLAGS31_0_DIO30_M                                    0x40000000
#define GPIO_EVFLAGS31_0_DIO30_S                                            30

// Field:    [29] DIO29
//
// Event for DIO 29
#define GPIO_EVFLAGS31_0_DIO29                                      0x20000000
#define GPIO_EVFLAGS31_0_DIO29_BITN                                         29
#define GPIO_EVFLAGS31_0_DIO29_M                                    0x20000000
#define GPIO_EVFLAGS31_0_DIO29_S                                            29

// Field:    [28] DIO28
//
// Event for DIO 28
#define GPIO_EVFLAGS31_0_DIO28                                      0x10000000
#define GPIO_EVFLAGS31_0_DIO28_BITN                                         28
#define GPIO_EVFLAGS31_0_DIO28_M                                    0x10000000
#define GPIO_EVFLAGS31_0_DIO28_S                                            28

// Field:    [27] DIO27
//
// Event for DIO 27
#define GPIO_EVFLAGS31_0_DIO27                                      0x08000000
#define GPIO_EVFLAGS31_0_DIO27_BITN                                         27
#define GPIO_EVFLAGS31_0_DIO27_M                                    0x08000000
#define GPIO_EVFLAGS31_0_DIO27_S                                            27

// Field:    [26] DIO26
//
// Event for DIO 26
#define GPIO_EVFLAGS31_0_DIO26                                      0x04000000
#define GPIO_EVFLAGS31_0_DIO26_BITN                                         26
#define GPIO_EVFLAGS31_0_DIO26_M                                    0x04000000
#define GPIO_EVFLAGS31_0_DIO26_S                                            26

// Field:    [25] DIO25
//
// Event for DIO 25
#define GPIO_EVFLAGS31_0_DIO25                                      0x02000000
#define GPIO_EVFLAGS31_0_DIO25_BITN                                         25
#define GPIO_EVFLAGS31_0_DIO25_M                                    0x02000000
#define GPIO_EVFLAGS31_0_DIO25_S                                            25

// Field:    [24] DIO24
//
// Event for DIO 24
#define GPIO_EVFLAGS31_0_DIO24                                      0x01000000
#define GPIO_EVFLAGS31_0_DIO24_BITN                                         24
#define GPIO_EVFLAGS31_0_DIO24_M                                    0x01000000
#define GPIO_EVFLAGS31_0_DIO24_S                                            24

// Field:    [23] DIO23
//
// Event for DIO 23
#define GPIO_EVFLAGS31_0_DIO23                                      0x00800000
#define GPIO_EVFLAGS31_0_DIO23_BITN                                         23
#define GPIO_EVFLAGS31_0_DIO23_M                                    0x00800000
#define GPIO_EVFLAGS31_0_DIO23_S                                            23

// Field:    [22] DIO22
//
// Event for DIO 22
#define GPIO_EVFLAGS31_0_DIO22                                      0x00400000
#define GPIO_EVFLAGS31_0_DIO22_BITN                                         22
#define GPIO_EVFLAGS31_0_DIO22_M                                    0x00400000
#define GPIO_EVFLAGS31_0_DIO22_S                                            22

// Field:    [21] DIO21
//
// Event for DIO 21
#define GPIO_EVFLAGS31_0_DIO21                                      0x00200000
#define GPIO_EVFLAGS31_0_DIO21_BITN                                         21
#define GPIO_EVFLAGS31_0_DIO21_M                                    0x00200000
#define GPIO_EVFLAGS31_0_DIO21_S                                            21

// Field:    [20] DIO20
//
// Event for DIO 20
#define GPIO_EVFLAGS31_0_DIO20                                      0x00100000
#define GPIO_EVFLAGS31_0_DIO20_BITN                                         20
#define GPIO_EVFLAGS31_0_DIO20_M                                    0x00100000
#define GPIO_EVFLAGS31_0_DIO20_S                                            20

// Field:    [19] DIO19
//
// Event for DIO 19
#define GPIO_EVFLAGS31_0_DIO19                                      0x00080000
#define GPIO_EVFLAGS31_0_DIO19_BITN                                         19
#define GPIO_EVFLAGS31_0_DIO19_M                                    0x00080000
#define GPIO_EVFLAGS31_0_DIO19_S                                            19

// Field:    [18] DIO18
//
// Event for DIO 18
#define GPIO_EVFLAGS31_0_DIO18                                      0x00040000
#define GPIO_EVFLAGS31_0_DIO18_BITN                                         18
#define GPIO_EVFLAGS31_0_DIO18_M                                    0x00040000
#define GPIO_EVFLAGS31_0_DIO18_S                                            18

// Field:    [17] DIO17
//
// Event for DIO 17
#define GPIO_EVFLAGS31_0_DIO17                                      0x00020000
#define GPIO_EVFLAGS31_0_DIO17_BITN                                         17
#define GPIO_EVFLAGS31_0_DIO17_M                                    0x00020000
#define GPIO_EVFLAGS31_0_DIO17_S                                            17

// Field:    [16] DIO16
//
// Event for DIO 16
#define GPIO_EVFLAGS31_0_DIO16                                      0x00010000
#define GPIO_EVFLAGS31_0_DIO16_BITN                                         16
#define GPIO_EVFLAGS31_0_DIO16_M                                    0x00010000
#define GPIO_EVFLAGS31_0_DIO16_S                                            16

// Field:    [15] DIO15
//
// Event for DIO 15
#define GPIO_EVFLAGS31_0_DIO15                                      0x00008000
#define GPIO_EVFLAGS31_0_DIO15_BITN                                         15
#define GPIO_EVFLAGS31_0_DIO15_M                                    0x00008000
#define GPIO_EVFLAGS31_0_DIO15_S                                            15

// Field:    [14] DIO14
//
// Event for DIO 14
#define GPIO_EVFLAGS31_0_DIO14                                      0x00004000
#define GPIO_EVFLAGS31_0_DIO14_BITN                                         14
#define GPIO_EVFLAGS31_0_DIO14_M                                    0x00004000
#define GPIO_EVFLAGS31_0_DIO14_S                                            14

// Field:    [13] DIO13
//
// Event for DIO 13
#define GPIO_EVFLAGS31_0_DIO13                                      0x00002000
#define GPIO_EVFLAGS31_0_DIO13_BITN                                         13
#define GPIO_EVFLAGS31_0_DIO13_M                                    0x00002000
#define GPIO_EVFLAGS31_0_DIO13_S                                            13

// Field:    [12] DIO12
//
// Event for DIO 12
#define GPIO_EVFLAGS31_0_DIO12                                      0x00001000
#define GPIO_EVFLAGS31_0_DIO12_BITN                                         12
#define GPIO_EVFLAGS31_0_DIO12_M                                    0x00001000
#define GPIO_EVFLAGS31_0_DIO12_S                                            12

// Field:    [11] DIO11
//
// Event for DIO 11
#define GPIO_EVFLAGS31_0_DIO11                                      0x00000800
#define GPIO_EVFLAGS31_0_DIO11_BITN                                         11
#define GPIO_EVFLAGS31_0_DIO11_M                                    0x00000800
#define GPIO_EVFLAGS31_0_DIO11_S                                            11

// Field:    [10] DIO10
//
// Event for DIO 10
#define GPIO_EVFLAGS31_0_DIO10                                      0x00000400
#define GPIO_EVFLAGS31_0_DIO10_BITN                                         10
#define GPIO_EVFLAGS31_0_DIO10_M                                    0x00000400
#define GPIO_EVFLAGS31_0_DIO10_S                                            10

// Field:     [9] DIO9
//
// Event for DIO 9
#define GPIO_EVFLAGS31_0_DIO9                                       0x00000200
#define GPIO_EVFLAGS31_0_DIO9_BITN                                           9
#define GPIO_EVFLAGS31_0_DIO9_M                                     0x00000200
#define GPIO_EVFLAGS31_0_DIO9_S                                              9

// Field:     [8] DIO8
//
// Event for DIO 8
#define GPIO_EVFLAGS31_0_DIO8                                       0x00000100
#define GPIO_EVFLAGS31_0_DIO8_BITN                                           8
#define GPIO_EVFLAGS31_0_DIO8_M                                     0x00000100
#define GPIO_EVFLAGS31_0_DIO8_S                                              8

// Field:     [7] DIO7
//
// Event for DIO 7
#define GPIO_EVFLAGS31_0_DIO7                                       0x00000080
#define GPIO_EVFLAGS31_0_DIO7_BITN                                           7
#define GPIO_EVFLAGS31_0_DIO7_M                                     0x00000080
#define GPIO_EVFLAGS31_0_DIO7_S                                              7

// Field:     [6] DIO6
//
// Event for DIO 6
#define GPIO_EVFLAGS31_0_DIO6                                       0x00000040
#define GPIO_EVFLAGS31_0_DIO6_BITN                                           6
#define GPIO_EVFLAGS31_0_DIO6_M                                     0x00000040
#define GPIO_EVFLAGS31_0_DIO6_S                                              6

// Field:     [5] DIO5
//
// Event for DIO 5
#define GPIO_EVFLAGS31_0_DIO5                                       0x00000020
#define GPIO_EVFLAGS31_0_DIO5_BITN                                           5
#define GPIO_EVFLAGS31_0_DIO5_M                                     0x00000020
#define GPIO_EVFLAGS31_0_DIO5_S                                              5

// Field:     [4] DIO4
//
// Event for DIO 4
#define GPIO_EVFLAGS31_0_DIO4                                       0x00000010
#define GPIO_EVFLAGS31_0_DIO4_BITN                                           4
#define GPIO_EVFLAGS31_0_DIO4_M                                     0x00000010
#define GPIO_EVFLAGS31_0_DIO4_S                                              4

// Field:     [3] DIO3
//
// Event for DIO 3
#define GPIO_EVFLAGS31_0_DIO3                                       0x00000008
#define GPIO_EVFLAGS31_0_DIO3_BITN                                           3
#define GPIO_EVFLAGS31_0_DIO3_M                                     0x00000008
#define GPIO_EVFLAGS31_0_DIO3_S                                              3

// Field:     [2] DIO2
//
// Event for DIO 2
#define GPIO_EVFLAGS31_0_DIO2                                       0x00000004
#define GPIO_EVFLAGS31_0_DIO2_BITN                                           2
#define GPIO_EVFLAGS31_0_DIO2_M                                     0x00000004
#define GPIO_EVFLAGS31_0_DIO2_S                                              2

// Field:     [1] DIO1
//
// Event for DIO 1
#define GPIO_EVFLAGS31_0_DIO1                                       0x00000002
#define GPIO_EVFLAGS31_0_DIO1_BITN                                           1
#define GPIO_EVFLAGS31_0_DIO1_M                                     0x00000002
#define GPIO_EVFLAGS31_0_DIO1_S                                              1

// Field:     [0] DIO0
//
// Event for DIO 0
#define GPIO_EVFLAGS31_0_DIO0                                       0x00000001
#define GPIO_EVFLAGS31_0_DIO0_BITN                                           0
#define GPIO_EVFLAGS31_0_DIO0_M                                     0x00000001
#define GPIO_EVFLAGS31_0_DIO0_S                                              0


#endif // __GPIO__
