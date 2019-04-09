/******************************************************************************
*  Filename:       hw_pka_int_h
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

#ifndef __HW_PKA_INT_H__
#define __HW_PKA_INT_H__

//*****************************************************************************
//
// This section defines the register offsets of
// PKA_INT component
//
//*****************************************************************************
// PKA Options register
#define PKA_INT_O_OPTIONS                                           0x00000FF8

// PKA hardware revision register
#define PKA_INT_O_REVISION                                          0x00000FFC

//*****************************************************************************
//
// Register: PKA_INT_O_OPTIONS
//
//*****************************************************************************
// Field:    [10] AIC_PRESENT
//
// When set to '1', indicates that an EIP201 AIC  is included in the EIP150
#define PKA_INT_OPTIONS_AIC_PRESENT                                 0x00000400
#define PKA_INT_OPTIONS_AIC_PRESENT_BITN                                    10
#define PKA_INT_OPTIONS_AIC_PRESENT_M                               0x00000400
#define PKA_INT_OPTIONS_AIC_PRESENT_S                                       10

// Field:     [9] EIP76_PRESENT
//
// When set to '1', indicates that the EIP76 TRNG  is included in the EIP150
#define PKA_INT_OPTIONS_EIP76_PRESENT                               0x00000200
#define PKA_INT_OPTIONS_EIP76_PRESENT_BITN                                   9
#define PKA_INT_OPTIONS_EIP76_PRESENT_M                             0x00000200
#define PKA_INT_OPTIONS_EIP76_PRESENT_S                                      9

// Field:     [8] EIP28_PRESENT
//
// When set to '1', indicates that the EIP28 PKA is included in the EIP150
#define PKA_INT_OPTIONS_EIP28_PRESENT                               0x00000100
#define PKA_INT_OPTIONS_EIP28_PRESENT_BITN                                   8
#define PKA_INT_OPTIONS_EIP28_PRESENT_M                             0x00000100
#define PKA_INT_OPTIONS_EIP28_PRESENT_S                                      8

// Field:     [3] AXI_INTERFACE
//
// When set to '1', indicates that the EIP150 is equipped with a AXI interface
#define PKA_INT_OPTIONS_AXI_INTERFACE                               0x00000008
#define PKA_INT_OPTIONS_AXI_INTERFACE_BITN                                   3
#define PKA_INT_OPTIONS_AXI_INTERFACE_M                             0x00000008
#define PKA_INT_OPTIONS_AXI_INTERFACE_S                                      3

// Field:     [2] AHB_IS_ASYNC
//
// When set to '1', indicates that AHB interface is asynchronous  Only
// applicable when AHB_INTERFACE is 1
#define PKA_INT_OPTIONS_AHB_IS_ASYNC                                0x00000004
#define PKA_INT_OPTIONS_AHB_IS_ASYNC_BITN                                    2
#define PKA_INT_OPTIONS_AHB_IS_ASYNC_M                              0x00000004
#define PKA_INT_OPTIONS_AHB_IS_ASYNC_S                                       2

// Field:     [1] AHB_INTERFACE
//
// When set to '1', indicates that the EIP150 is equipped with a AHB interface
#define PKA_INT_OPTIONS_AHB_INTERFACE                               0x00000002
#define PKA_INT_OPTIONS_AHB_INTERFACE_BITN                                   1
#define PKA_INT_OPTIONS_AHB_INTERFACE_M                             0x00000002
#define PKA_INT_OPTIONS_AHB_INTERFACE_S                                      1

// Field:     [0] PLB_INTERFACE
//
// When set to '1', indicates that the EIP150 is equipped with a PLB interface
#define PKA_INT_OPTIONS_PLB_INTERFACE                               0x00000001
#define PKA_INT_OPTIONS_PLB_INTERFACE_BITN                                   0
#define PKA_INT_OPTIONS_PLB_INTERFACE_M                             0x00000001
#define PKA_INT_OPTIONS_PLB_INTERFACE_S                                      0

//*****************************************************************************
//
// Register: PKA_INT_O_REVISION
//
//*****************************************************************************
// Field: [27:24] MAJOR_REVISION
//
// These bits encode the major version number for this module
#define PKA_INT_REVISION_MAJOR_REVISION_W                                    4
#define PKA_INT_REVISION_MAJOR_REVISION_M                           0x0F000000
#define PKA_INT_REVISION_MAJOR_REVISION_S                                   24

// Field: [23:20] MINOR_REVISION
//
// These bits encode the minor version number for this module
#define PKA_INT_REVISION_MINOR_REVISION_W                                    4
#define PKA_INT_REVISION_MINOR_REVISION_M                           0x00F00000
#define PKA_INT_REVISION_MINOR_REVISION_S                                   20

// Field: [19:16] PATCH_LEVEL
//
// These bits encode the hardware patch level for this module they start at
// value 0 on the first release
#define PKA_INT_REVISION_PATCH_LEVEL_W                                       4
#define PKA_INT_REVISION_PATCH_LEVEL_M                              0x000F0000
#define PKA_INT_REVISION_PATCH_LEVEL_S                                      16

// Field:  [15:8] COMP_EIP_NUM
//
// These bits simply contain the complement of bits [7:0], used by a driver to
// ascertain that the EIP150 revision register is indeed read
#define PKA_INT_REVISION_COMP_EIP_NUM_W                                      8
#define PKA_INT_REVISION_COMP_EIP_NUM_M                             0x0000FF00
#define PKA_INT_REVISION_COMP_EIP_NUM_S                                      8

// Field:   [7:0] EIP_NUM
//
// These bits encode the AuthenTec EIP number for the EIP150
#define PKA_INT_REVISION_EIP_NUM_W                                           8
#define PKA_INT_REVISION_EIP_NUM_M                                  0x000000FF
#define PKA_INT_REVISION_EIP_NUM_S                                           0


#endif // __PKA_INT__
