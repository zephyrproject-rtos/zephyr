/******************************************************************************
*  Filename:       hw_aon_ioc_h
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

#ifndef __HW_AON_IOC_H__
#define __HW_AON_IOC_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AON_IOC component
//
//*****************************************************************************
// Internal
#define AON_IOC_O_IOSTRMIN                                          0x00000000

// Internal
#define AON_IOC_O_IOSTRMED                                          0x00000004

// Internal
#define AON_IOC_O_IOSTRMAX                                          0x00000008

// IO Latch Control
#define AON_IOC_O_IOCLATCH                                          0x0000000C

// SCLK_LF External Output Control
#define AON_IOC_O_CLK32KCTL                                         0x00000010

// TCK IO Pin Control
#define AON_IOC_O_TCKCTL                                            0x00000014

//*****************************************************************************
//
// Register: AON_IOC_O_IOSTRMIN
//
//*****************************************************************************
// Field:   [2:0] GRAY_CODE
//
// Internal. Only to be used through TI provided API.
#define AON_IOC_IOSTRMIN_GRAY_CODE_W                                         3
#define AON_IOC_IOSTRMIN_GRAY_CODE_M                                0x00000007
#define AON_IOC_IOSTRMIN_GRAY_CODE_S                                         0

//*****************************************************************************
//
// Register: AON_IOC_O_IOSTRMED
//
//*****************************************************************************
// Field:   [2:0] GRAY_CODE
//
// Internal. Only to be used through TI provided API.
#define AON_IOC_IOSTRMED_GRAY_CODE_W                                         3
#define AON_IOC_IOSTRMED_GRAY_CODE_M                                0x00000007
#define AON_IOC_IOSTRMED_GRAY_CODE_S                                         0

//*****************************************************************************
//
// Register: AON_IOC_O_IOSTRMAX
//
//*****************************************************************************
// Field:   [2:0] GRAY_CODE
//
// Internal. Only to be used through TI provided API.
#define AON_IOC_IOSTRMAX_GRAY_CODE_W                                         3
#define AON_IOC_IOSTRMAX_GRAY_CODE_M                                0x00000007
#define AON_IOC_IOSTRMAX_GRAY_CODE_S                                         0

//*****************************************************************************
//
// Register: AON_IOC_O_IOCLATCH
//
//*****************************************************************************
// Field:     [0] EN
//
// Controls latches between MCU IOC and AON_IOC.
//
// The latches are transparent by default.
//
// They must be closed prior to power off the domain(s) controlling the IOs in
// order to preserve IO values on external pins.
// ENUMs:
// TRANSP                   Latches are transparent, meaning the value of the
//                          IO is directly controlled by the GPIO or
//                          peripheral value
// STATIC                   Latches are static, meaning the current value on
//                          the IO pin is frozen by latches and kept even
//                          if GPIO module or a peripheral module is turned
//                          off
#define AON_IOC_IOCLATCH_EN                                         0x00000001
#define AON_IOC_IOCLATCH_EN_BITN                                             0
#define AON_IOC_IOCLATCH_EN_M                                       0x00000001
#define AON_IOC_IOCLATCH_EN_S                                                0
#define AON_IOC_IOCLATCH_EN_TRANSP                                  0x00000001
#define AON_IOC_IOCLATCH_EN_STATIC                                  0x00000000

//*****************************************************************************
//
// Register: AON_IOC_O_CLK32KCTL
//
//*****************************************************************************
// Field:     [0] OE_N
//
// 0: Output enable active. SCLK_LF output on IO pin that has PORT_ID (for
// example IOC:IOCFG0.PORT_ID) set to AON_CLK32K.
// 1: Output enable not active
#define AON_IOC_CLK32KCTL_OE_N                                      0x00000001
#define AON_IOC_CLK32KCTL_OE_N_BITN                                          0
#define AON_IOC_CLK32KCTL_OE_N_M                                    0x00000001
#define AON_IOC_CLK32KCTL_OE_N_S                                             0

//*****************************************************************************
//
// Register: AON_IOC_O_TCKCTL
//
//*****************************************************************************
// Field:     [0] EN
//
// 0: Input driver for TCK disabled.
// 1: Input driver for TCK enabled.
#define AON_IOC_TCKCTL_EN                                           0x00000001
#define AON_IOC_TCKCTL_EN_BITN                                               0
#define AON_IOC_TCKCTL_EN_M                                         0x00000001
#define AON_IOC_TCKCTL_EN_S                                                  0


#endif // __AON_IOC__
