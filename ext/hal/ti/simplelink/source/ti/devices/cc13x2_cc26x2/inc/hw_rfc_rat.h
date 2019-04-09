/******************************************************************************
*  Filename:       hw_rfc_rat_h
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

#ifndef __HW_RFC_RAT_H__
#define __HW_RFC_RAT_H__

//*****************************************************************************
//
// This section defines the register offsets of
// RFC_RAT component
//
//*****************************************************************************
// Radio Timer Counter Value
#define RFC_RAT_O_RATCNT                                            0x00000004

// Timer Channel 0 Capture/Compare Register
#define RFC_RAT_O_RATCH0VAL                                         0x00000080

// Timer Channel 1 Capture/Compare Register
#define RFC_RAT_O_RATCH1VAL                                         0x00000084

// Timer Channel 2 Capture/Compare Register
#define RFC_RAT_O_RATCH2VAL                                         0x00000088

// Timer Channel 3 Capture/Compare Register
#define RFC_RAT_O_RATCH3VAL                                         0x0000008C

// Timer Channel 4 Capture/Compare Register
#define RFC_RAT_O_RATCH4VAL                                         0x00000090

// Timer Channel 5 Capture/Compare Register
#define RFC_RAT_O_RATCH5VAL                                         0x00000094

// Timer Channel 6 Capture/Compare Register
#define RFC_RAT_O_RATCH6VAL                                         0x00000098

// Timer Channel 7 Capture/Compare Register
#define RFC_RAT_O_RATCH7VAL                                         0x0000009C

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCNT
//
//*****************************************************************************
// Field:  [31:0] CNT
//
// Counter value. This is not writable while radio timer counter is enabled.
#define RFC_RAT_RATCNT_CNT_W                                                32
#define RFC_RAT_RATCNT_CNT_M                                        0xFFFFFFFF
#define RFC_RAT_RATCNT_CNT_S                                                 0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH0VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH0VAL_VAL_W                                             32
#define RFC_RAT_RATCH0VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH0VAL_VAL_S                                              0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH1VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH1VAL_VAL_W                                             32
#define RFC_RAT_RATCH1VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH1VAL_VAL_S                                              0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH2VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH2VAL_VAL_W                                             32
#define RFC_RAT_RATCH2VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH2VAL_VAL_S                                              0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH3VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH3VAL_VAL_W                                             32
#define RFC_RAT_RATCH3VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH3VAL_VAL_S                                              0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH4VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH4VAL_VAL_W                                             32
#define RFC_RAT_RATCH4VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH4VAL_VAL_S                                              0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH5VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH5VAL_VAL_W                                             32
#define RFC_RAT_RATCH5VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH5VAL_VAL_S                                              0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH6VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH6VAL_VAL_W                                             32
#define RFC_RAT_RATCH6VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH6VAL_VAL_S                                              0

//*****************************************************************************
//
// Register: RFC_RAT_O_RATCH7VAL
//
//*****************************************************************************
// Field:  [31:0] VAL
//
// Capture/compare value. Only writable when the channel is configured for
// compare mode. In compare mode, a write to this register will auto-arm the
// channel.
#define RFC_RAT_RATCH7VAL_VAL_W                                             32
#define RFC_RAT_RATCH7VAL_VAL_M                                     0xFFFFFFFF
#define RFC_RAT_RATCH7VAL_VAL_S                                              0


#endif // __RFC_RAT__
