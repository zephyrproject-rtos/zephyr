/******************************************************************************
*  Filename:       hw_rfc_pwr_h
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

#ifndef __HW_RFC_PWR_H__
#define __HW_RFC_PWR_H__

//*****************************************************************************
//
// This section defines the register offsets of
// RFC_PWR component
//
//*****************************************************************************
// RF Core Power Management and Clock Enable
#define RFC_PWR_O_PWMCLKEN                                          0x00000000

//*****************************************************************************
//
// Register: RFC_PWR_O_PWMCLKEN
//
//*****************************************************************************
// Field:    [10] RFCTRC
//
// Enable clock to the RF Core Tracer (RFCTRC) module.
#define RFC_PWR_PWMCLKEN_RFCTRC                                     0x00000400
#define RFC_PWR_PWMCLKEN_RFCTRC_BITN                                        10
#define RFC_PWR_PWMCLKEN_RFCTRC_M                                   0x00000400
#define RFC_PWR_PWMCLKEN_RFCTRC_S                                           10

// Field:     [9] FSCA
//
// Enable clock to the Frequency Synthesizer Calibration Accelerator (FSCA)
// module.
#define RFC_PWR_PWMCLKEN_FSCA                                       0x00000200
#define RFC_PWR_PWMCLKEN_FSCA_BITN                                           9
#define RFC_PWR_PWMCLKEN_FSCA_M                                     0x00000200
#define RFC_PWR_PWMCLKEN_FSCA_S                                              9

// Field:     [8] PHA
//
// Enable clock to the Packet Handling Accelerator (PHA) module.
#define RFC_PWR_PWMCLKEN_PHA                                        0x00000100
#define RFC_PWR_PWMCLKEN_PHA_BITN                                            8
#define RFC_PWR_PWMCLKEN_PHA_M                                      0x00000100
#define RFC_PWR_PWMCLKEN_PHA_S                                               8

// Field:     [7] RAT
//
// Enable clock to the Radio Timer (RAT) module.
#define RFC_PWR_PWMCLKEN_RAT                                        0x00000080
#define RFC_PWR_PWMCLKEN_RAT_BITN                                            7
#define RFC_PWR_PWMCLKEN_RAT_M                                      0x00000080
#define RFC_PWR_PWMCLKEN_RAT_S                                               7

// Field:     [6] RFERAM
//
// Enable clock to the RF Engine RAM module.
#define RFC_PWR_PWMCLKEN_RFERAM                                     0x00000040
#define RFC_PWR_PWMCLKEN_RFERAM_BITN                                         6
#define RFC_PWR_PWMCLKEN_RFERAM_M                                   0x00000040
#define RFC_PWR_PWMCLKEN_RFERAM_S                                            6

// Field:     [5] RFE
//
// Enable clock to the RF Engine (RFE) module.
#define RFC_PWR_PWMCLKEN_RFE                                        0x00000020
#define RFC_PWR_PWMCLKEN_RFE_BITN                                            5
#define RFC_PWR_PWMCLKEN_RFE_M                                      0x00000020
#define RFC_PWR_PWMCLKEN_RFE_S                                               5

// Field:     [4] MDMRAM
//
// Enable clock to the Modem RAM module.
#define RFC_PWR_PWMCLKEN_MDMRAM                                     0x00000010
#define RFC_PWR_PWMCLKEN_MDMRAM_BITN                                         4
#define RFC_PWR_PWMCLKEN_MDMRAM_M                                   0x00000010
#define RFC_PWR_PWMCLKEN_MDMRAM_S                                            4

// Field:     [3] MDM
//
// Enable clock to the Modem (MDM) module.
#define RFC_PWR_PWMCLKEN_MDM                                        0x00000008
#define RFC_PWR_PWMCLKEN_MDM_BITN                                            3
#define RFC_PWR_PWMCLKEN_MDM_M                                      0x00000008
#define RFC_PWR_PWMCLKEN_MDM_S                                               3

// Field:     [2] CPERAM
//
// Enable clock to the Command and Packet Engine (CPE) RAM module. As part of
// RF Core initialization, set this bit together with CPE bit to enable CPE to
// boot.
#define RFC_PWR_PWMCLKEN_CPERAM                                     0x00000004
#define RFC_PWR_PWMCLKEN_CPERAM_BITN                                         2
#define RFC_PWR_PWMCLKEN_CPERAM_M                                   0x00000004
#define RFC_PWR_PWMCLKEN_CPERAM_S                                            2

// Field:     [1] CPE
//
// Enable processor clock (hclk) to the Command and Packet Engine (CPE). As
// part of RF Core initialization, set this bit together with CPERAM bit to
// enable CPE to boot.
#define RFC_PWR_PWMCLKEN_CPE                                        0x00000002
#define RFC_PWR_PWMCLKEN_CPE_BITN                                            1
#define RFC_PWR_PWMCLKEN_CPE_M                                      0x00000002
#define RFC_PWR_PWMCLKEN_CPE_S                                               1

// Field:     [0] RFC
//
// Enable essential clocks for the RF Core interface. This includes the
// interconnect, the radio doorbell DBELL command interface, the power
// management (PWR) clock control module, and bus clock (sclk) for the CPE. To
// remove possibility of locking yourself out from the RF Core, this bit can
// not be cleared. If you need to disable all clocks to the RF Core, see the
// PRCM:RFCCLKG.CLK_EN register.
#define RFC_PWR_PWMCLKEN_RFC                                        0x00000001
#define RFC_PWR_PWMCLKEN_RFC_BITN                                            0
#define RFC_PWR_PWMCLKEN_RFC_M                                      0x00000001
#define RFC_PWR_PWMCLKEN_RFC_S                                               0


#endif // __RFC_PWR__
