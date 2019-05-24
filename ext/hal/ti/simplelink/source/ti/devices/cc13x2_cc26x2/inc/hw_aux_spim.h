/******************************************************************************
*  Filename:       hw_aux_spim_h
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

#ifndef __HW_AUX_SPIM_H__
#define __HW_AUX_SPIM_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_SPIM component
//
//*****************************************************************************
// SPI Master Configuration
#define AUX_SPIM_O_SPIMCFG                                          0x00000000

// MISO Configuration
#define AUX_SPIM_O_MISOCFG                                          0x00000004

// MOSI Control
#define AUX_SPIM_O_MOSICTL                                          0x00000008

// Transmit 8 Bit
#define AUX_SPIM_O_TX8                                              0x0000000C

// Transmit 16 Bit
#define AUX_SPIM_O_TX16                                             0x00000010

// Receive 8 Bit
#define AUX_SPIM_O_RX8                                              0x00000014

// Receive 16 Bit
#define AUX_SPIM_O_RX16                                             0x00000018

// SCLK Idle
#define AUX_SPIM_O_SCLKIDLE                                         0x0000001C

// Data Idle
#define AUX_SPIM_O_DATAIDLE                                         0x00000020

//*****************************************************************************
//
// Register: AUX_SPIM_O_SPIMCFG
//
//*****************************************************************************
// Field:   [7:2] DIV
//
// SCLK divider.
//
// Peripheral clock frequency division gives the SCLK clock frequency. The
// division factor equals (2 * (DIV+1)):
//
// 0x00: Divide by 2.
// 0x01: Divide by 4.
// 0x02: Divide by 6.
// ...
// 0x3F: Divide by 128.
#define AUX_SPIM_SPIMCFG_DIV_W                                               6
#define AUX_SPIM_SPIMCFG_DIV_M                                      0x000000FC
#define AUX_SPIM_SPIMCFG_DIV_S                                               2

// Field:     [1] PHA
//
// Phase of the MOSI and MISO data signals.
//
// 0: Sample MISO at leading (odd) edges and shift MOSI at trailing (even)
// edges of SCLK.
// 1: Sample MISO at trailing (even) edges and shift MOSI at leading (odd)
// edges of SCLK.
#define AUX_SPIM_SPIMCFG_PHA                                        0x00000002
#define AUX_SPIM_SPIMCFG_PHA_BITN                                            1
#define AUX_SPIM_SPIMCFG_PHA_M                                      0x00000002
#define AUX_SPIM_SPIMCFG_PHA_S                                               1

// Field:     [0] POL
//
// Polarity of the SCLK signal.
//
// 0: SCLK is low when idle, first clock edge rises.
// 1: SCLK is high when idle, first clock edge falls.
#define AUX_SPIM_SPIMCFG_POL                                        0x00000001
#define AUX_SPIM_SPIMCFG_POL_BITN                                            0
#define AUX_SPIM_SPIMCFG_POL_M                                      0x00000001
#define AUX_SPIM_SPIMCFG_POL_S                                               0

//*****************************************************************************
//
// Register: AUX_SPIM_O_MISOCFG
//
//*****************************************************************************
// Field:   [4:0] AUXIO
//
// AUXIO to MISO mux.
//
// Select the AUXIO pin that connects to MISO.
#define AUX_SPIM_MISOCFG_AUXIO_W                                             5
#define AUX_SPIM_MISOCFG_AUXIO_M                                    0x0000001F
#define AUX_SPIM_MISOCFG_AUXIO_S                                             0

//*****************************************************************************
//
// Register: AUX_SPIM_O_MOSICTL
//
//*****************************************************************************
// Field:     [0] VALUE
//
// MOSI level control.
//
// 0: Set MOSI low.
// 1: Set MOSI high.
#define AUX_SPIM_MOSICTL_VALUE                                      0x00000001
#define AUX_SPIM_MOSICTL_VALUE_BITN                                          0
#define AUX_SPIM_MOSICTL_VALUE_M                                    0x00000001
#define AUX_SPIM_MOSICTL_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_SPIM_O_TX8
//
//*****************************************************************************
// Field:   [7:0] DATA
//
// 8 bit data transfer.
//
// Write DATA to start transfer, MSB first. When transfer completes, MOSI stays
// at the value of LSB.
#define AUX_SPIM_TX8_DATA_W                                                  8
#define AUX_SPIM_TX8_DATA_M                                         0x000000FF
#define AUX_SPIM_TX8_DATA_S                                                  0

//*****************************************************************************
//
// Register: AUX_SPIM_O_TX16
//
//*****************************************************************************
// Field:  [15:0] DATA
//
// 16 bit data transfer.
//
// Write DATA to start transfer, MSB first. When transfer completes, MOSI stays
// at the value of LSB.
#define AUX_SPIM_TX16_DATA_W                                                16
#define AUX_SPIM_TX16_DATA_M                                        0x0000FFFF
#define AUX_SPIM_TX16_DATA_S                                                 0

//*****************************************************************************
//
// Register: AUX_SPIM_O_RX8
//
//*****************************************************************************
// Field:   [7:0] DATA
//
// Latest 8 bits received on MISO.
#define AUX_SPIM_RX8_DATA_W                                                  8
#define AUX_SPIM_RX8_DATA_M                                         0x000000FF
#define AUX_SPIM_RX8_DATA_S                                                  0

//*****************************************************************************
//
// Register: AUX_SPIM_O_RX16
//
//*****************************************************************************
// Field:  [15:0] DATA
//
// Latest 16 bits received on MISO.
#define AUX_SPIM_RX16_DATA_W                                                16
#define AUX_SPIM_RX16_DATA_M                                        0x0000FFFF
#define AUX_SPIM_RX16_DATA_S                                                 0

//*****************************************************************************
//
// Register: AUX_SPIM_O_SCLKIDLE
//
//*****************************************************************************
// Field:     [0] STAT
//
// Wait for SCLK idle.
//
// Read operation stalls until SCLK is idle with no remaining clock edges. Read
// then returns 1.
//
// AUX_SCE can use this to control CS deassertion.
#define AUX_SPIM_SCLKIDLE_STAT                                      0x00000001
#define AUX_SPIM_SCLKIDLE_STAT_BITN                                          0
#define AUX_SPIM_SCLKIDLE_STAT_M                                    0x00000001
#define AUX_SPIM_SCLKIDLE_STAT_S                                             0

//*****************************************************************************
//
// Register: AUX_SPIM_O_DATAIDLE
//
//*****************************************************************************
// Field:     [0] STAT
//
// Wait for data idle.
//
// Read operation stalls until the SCLK period associated with LSB transmission
// completes. Read then returns 1.
//
// AUX_SCE can use this to control CS deassertion.
#define AUX_SPIM_DATAIDLE_STAT                                      0x00000001
#define AUX_SPIM_DATAIDLE_STAT_BITN                                          0
#define AUX_SPIM_DATAIDLE_STAT_M                                    0x00000001
#define AUX_SPIM_DATAIDLE_STAT_S                                             0


#endif // __AUX_SPIM__
