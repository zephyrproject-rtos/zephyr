/******************************************************************************
*  Filename:       hw_udma_h
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

#ifndef __HW_UDMA_H__
#define __HW_UDMA_H__

//*****************************************************************************
//
// This section defines the register offsets of
// UDMA component
//
//*****************************************************************************
// Status
#define UDMA_O_STATUS                                               0x00000000

// Configuration
#define UDMA_O_CFG                                                  0x00000004

// Channel Control Data Base Pointer
#define UDMA_O_CTRL                                                 0x00000008

// Channel Alternate Control Data Base Pointer
#define UDMA_O_ALTCTRL                                              0x0000000C

// Channel Wait On Request Status
#define UDMA_O_WAITONREQ                                            0x00000010

// Channel Software Request
#define UDMA_O_SOFTREQ                                              0x00000014

// Channel Set UseBurst
#define UDMA_O_SETBURST                                             0x00000018

// Channel Clear UseBurst
#define UDMA_O_CLEARBURST                                           0x0000001C

// Channel Set Request Mask
#define UDMA_O_SETREQMASK                                           0x00000020

// Clear Channel Request Mask
#define UDMA_O_CLEARREQMASK                                         0x00000024

// Set Channel Enable
#define UDMA_O_SETCHANNELEN                                         0x00000028

// Clear Channel Enable
#define UDMA_O_CLEARCHANNELEN                                       0x0000002C

// Channel Set Primary-Alternate
#define UDMA_O_SETCHNLPRIALT                                        0x00000030

// Channel Clear Primary-Alternate
#define UDMA_O_CLEARCHNLPRIALT                                      0x00000034

// Set Channel Priority
#define UDMA_O_SETCHNLPRIORITY                                      0x00000038

// Clear Channel Priority
#define UDMA_O_CLEARCHNLPRIORITY                                    0x0000003C

// Error Status and Clear
#define UDMA_O_ERROR                                                0x0000004C

// Channel Request Done
#define UDMA_O_REQDONE                                              0x00000504

// Channel Request Done Mask
#define UDMA_O_DONEMASK                                             0x00000520

//*****************************************************************************
//
// Register: UDMA_O_STATUS
//
//*****************************************************************************
// Field: [31:28] TEST
//
//
// 0x0: Controller does not include the integration test logic
// 0x1: Controller includes the integration test logic
// 0x2: Undefined
// ...
// 0xF: Undefined
#define UDMA_STATUS_TEST_W                                                   4
#define UDMA_STATUS_TEST_M                                          0xF0000000
#define UDMA_STATUS_TEST_S                                                  28

// Field: [20:16] TOTALCHANNELS
//
// Register value returns number of available uDMA channels minus one. For
// example a read out value of:
//
// 0x00: Show that the controller is configured to use 1 uDMA channel
// 0x01: Shows that the controller is configured to use 2 uDMA channels
// ...
// 0x1F: Shows that the controller is configured to use 32 uDMA channels
// (32-1=31=0x1F)
#define UDMA_STATUS_TOTALCHANNELS_W                                          5
#define UDMA_STATUS_TOTALCHANNELS_M                                 0x001F0000
#define UDMA_STATUS_TOTALCHANNELS_S                                         16

// Field:   [7:4] STATE
//
// Current state of the control state machine. State can be one of the
// following:
//
// 0x0: Idle
// 0x1: Reading channel controller data
// 0x2: Reading source data end pointer
// 0x3: Reading destination data end pointer
// 0x4: Reading source data
// 0x5: Writing destination data
// 0x6: Waiting for uDMA request to clear
// 0x7: Writing channel controller data
// 0x8: Stalled
// 0x9: Done
// 0xA: Peripheral scatter-gather transition
// 0xB: Undefined
// ...
// 0xF: Undefined.
#define UDMA_STATUS_STATE_W                                                  4
#define UDMA_STATUS_STATE_M                                         0x000000F0
#define UDMA_STATUS_STATE_S                                                  4

// Field:     [0] MASTERENABLE
//
// Shows the enable status of the controller as configured by CFG.MASTERENABLE:
//
// 0: Controller is disabled
// 1: Controller is enabled
#define UDMA_STATUS_MASTERENABLE                                    0x00000001
#define UDMA_STATUS_MASTERENABLE_BITN                                        0
#define UDMA_STATUS_MASTERENABLE_M                                  0x00000001
#define UDMA_STATUS_MASTERENABLE_S                                           0

//*****************************************************************************
//
// Register: UDMA_O_CFG
//
//*****************************************************************************
// Field:   [7:5] PRTOCTRL
//
// Sets the AHB-Lite bus protocol protection state by controlling the AHB
// signal HProt[3:1] as follows:
//
// Bit [7] Controls HProt[3] to indicate if a cacheable access is occurring.
// Bit [6] Controls HProt[2] to indicate if a bufferable access is occurring.
// Bit [5] Controls HProt[1] to indicate if a privileged access is occurring.
//
// When bit [n] = 1 then the corresponding HProt bit is high.
// When bit [n] = 0 then the corresponding HProt bit is low.
//
// This field controls HProt[3:1] signal for all transactions initiated by uDMA
// except two transactions below:
// - the read from the address indicated by source address pointer
// - the write to the address indicated by destination address pointer
// HProt[3:1] for these two exceptions can be controlled by dedicated fields in
// the channel configutation descriptor.
#define UDMA_CFG_PRTOCTRL_W                                                  3
#define UDMA_CFG_PRTOCTRL_M                                         0x000000E0
#define UDMA_CFG_PRTOCTRL_S                                                  5

// Field:     [0] MASTERENABLE
//
// Enables the controller:
//
// 0: Disables the controller
// 1: Enables the controller
#define UDMA_CFG_MASTERENABLE                                       0x00000001
#define UDMA_CFG_MASTERENABLE_BITN                                           0
#define UDMA_CFG_MASTERENABLE_M                                     0x00000001
#define UDMA_CFG_MASTERENABLE_S                                              0

//*****************************************************************************
//
// Register: UDMA_O_CTRL
//
//*****************************************************************************
// Field: [31:10] BASEPTR
//
// This register point to the base address for the primary data structures of
// each DMA channel. This is not stored in module, but in system memory, thus
// space must be allocated for this usage when DMA is in usage
#define UDMA_CTRL_BASEPTR_W                                                 22
#define UDMA_CTRL_BASEPTR_M                                         0xFFFFFC00
#define UDMA_CTRL_BASEPTR_S                                                 10

//*****************************************************************************
//
// Register: UDMA_O_ALTCTRL
//
//*****************************************************************************
// Field:  [31:0] BASEPTR
//
// This register shows the base address for the alternate data structures and
// is calculated by module, thus read only
#define UDMA_ALTCTRL_BASEPTR_W                                              32
#define UDMA_ALTCTRL_BASEPTR_M                                      0xFFFFFFFF
#define UDMA_ALTCTRL_BASEPTR_S                                               0

//*****************************************************************************
//
// Register: UDMA_O_WAITONREQ
//
//*****************************************************************************
// Field:  [31:0] CHNLSTATUS
//
// Channel wait on request status:
//
// Bit [Ch] = 0: Once uDMA receives a single or burst request on channel Ch,
// this channel may come out of active state even if request is still present.
// Bit [Ch] = 1: Once uDMA receives a single or burst request on channel Ch, it
// keeps channel Ch in active state until the requests are deasserted. This
// handshake is necessary for channels where the requester is in an
// asynchronous domain or can run at slower clock speed than uDMA
#define UDMA_WAITONREQ_CHNLSTATUS_W                                         32
#define UDMA_WAITONREQ_CHNLSTATUS_M                                 0xFFFFFFFF
#define UDMA_WAITONREQ_CHNLSTATUS_S                                          0

//*****************************************************************************
//
// Register: UDMA_O_SOFTREQ
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Set the appropriate bit to generate a software uDMA request on the
// corresponding uDMA channel
//
// Bit [Ch] = 0: Does not create a uDMA request for channel Ch
// Bit [Ch] = 1: Creates a uDMA request for channel Ch
//
// Writing to a bit where a uDMA channel is not implemented does not create a
// uDMA request for that channel
#define UDMA_SOFTREQ_CHNLS_W                                                32
#define UDMA_SOFTREQ_CHNLS_M                                        0xFFFFFFFF
#define UDMA_SOFTREQ_CHNLS_S                                                 0

//*****************************************************************************
//
// Register: UDMA_O_SETBURST
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Returns the useburst status, or disables individual channels from generating
// single uDMA requests. The value R is the arbitration rate and stored in the
// controller data structure.
//
// Read as:
//
// Bit [Ch] = 0: uDMA channel Ch responds to both burst and single requests on
// channel C. The controller performs 2^R, or single, bus transfers.
//
// Bit [Ch] = 1: uDMA channel Ch does not respond to single transfer requests.
// The controller only responds to burst transfer requests and performs 2^R
// transfers.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the CLEARBURST.CHNLS to set bit [Ch] to 0.
// Bit [Ch] = 1: Disables single transfer requests on channel Ch. The
// controller performs 2^R transfers for burst requests.
//
// Writing to a bit where a uDMA channel is not implemented has no effect
#define UDMA_SETBURST_CHNLS_W                                               32
#define UDMA_SETBURST_CHNLS_M                                       0xFFFFFFFF
#define UDMA_SETBURST_CHNLS_S                                                0

//*****************************************************************************
//
// Register: UDMA_O_CLEARBURST
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Set the appropriate bit to enable single transfer requests.
//
// Write as:
//
// Bit [Ch] = 0: No effect. Use the SETBURST.CHNLS to disable single transfer
// requests.
//
// Bit [Ch] = 1: Enables single transfer requests on channel Ch.
//
// Writing to a bit where a DMA channel is not implemented has no effect.
#define UDMA_CLEARBURST_CHNLS_W                                             32
#define UDMA_CLEARBURST_CHNLS_M                                     0xFFFFFFFF
#define UDMA_CLEARBURST_CHNLS_S                                              0

//*****************************************************************************
//
// Register: UDMA_O_SETREQMASK
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Returns the burst and single request mask status, or disables the
// corresponding channel from generating uDMA requests.
//
// Read as:
// Bit [Ch] = 0: External requests are enabled for channel Ch.
// Bit [Ch] = 1: External requests are disabled for channel Ch.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the CLEARREQMASK.CHNLS to enable uDMA requests.
// Bit [Ch] = 1: Disables uDMA burst request channel [C] and uDMA single
// request channel [C] input from generating uDMA requests.
//
// Writing to a bit where a uDMA channel is not implemented has no effect
#define UDMA_SETREQMASK_CHNLS_W                                             32
#define UDMA_SETREQMASK_CHNLS_M                                     0xFFFFFFFF
#define UDMA_SETREQMASK_CHNLS_S                                              0

//*****************************************************************************
//
// Register: UDMA_O_CLEARREQMASK
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Set the appropriate bit to enable DMA request for the channel.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the SETREQMASK.CHNLS to disable channel C from
// generating requests.
// Bit [Ch] = 1: Enables channel [C] to generate DMA requests.
//
// Writing to a bit where a DMA channel is not implemented has no effect.
#define UDMA_CLEARREQMASK_CHNLS_W                                           32
#define UDMA_CLEARREQMASK_CHNLS_M                                   0xFFFFFFFF
#define UDMA_CLEARREQMASK_CHNLS_S                                            0

//*****************************************************************************
//
// Register: UDMA_O_SETCHANNELEN
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Returns the enable status of the channels, or enables the corresponding
// channels.
//
// Read as:
// Bit [Ch] = 0: Channel Ch is disabled.
// Bit [Ch] = 1: Channel Ch is enabled.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the CLEARCHANNELEN.CHNLS to disable a channel
// Bit [Ch] = 1: Enables channel Ch
//
// Writing to a bit where a DMA channel is not implemented has no effect
#define UDMA_SETCHANNELEN_CHNLS_W                                           32
#define UDMA_SETCHANNELEN_CHNLS_M                                   0xFFFFFFFF
#define UDMA_SETCHANNELEN_CHNLS_S                                            0

//*****************************************************************************
//
// Register: UDMA_O_CLEARCHANNELEN
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Set the appropriate bit to disable the corresponding uDMA channel.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the SETCHANNELEN.CHNLS to enable uDMA channels.
// Bit [Ch] = 1: Disables channel Ch
//
// Writing to a bit where a uDMA channel is not implemented has no effect
#define UDMA_CLEARCHANNELEN_CHNLS_W                                         32
#define UDMA_CLEARCHANNELEN_CHNLS_M                                 0xFFFFFFFF
#define UDMA_CLEARCHANNELEN_CHNLS_S                                          0

//*****************************************************************************
//
// Register: UDMA_O_SETCHNLPRIALT
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Returns the channel control data structure status, or selects the alternate
// data structure for the corresponding uDMA channel.
//
// Read as:
// Bit [Ch] = 0: uDMA channel Ch is using the primary data structure.
// Bit [Ch] = 1: uDMA channel Ch is using the alternate data structure.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the CLEARCHNLPRIALT.CHNLS to disable a channel
// Bit [Ch] = 1: Selects the alternate data structure for channel Ch
//
// Writing to a bit where a uDMA channel is not implemented has no effect
#define UDMA_SETCHNLPRIALT_CHNLS_W                                          32
#define UDMA_SETCHNLPRIALT_CHNLS_M                                  0xFFFFFFFF
#define UDMA_SETCHNLPRIALT_CHNLS_S                                           0

//*****************************************************************************
//
// Register: UDMA_O_CLEARCHNLPRIALT
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Clears the appropriate bit to select the primary data structure for the
// corresponding uDMA channel.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the SETCHNLPRIALT.CHNLS to select the alternate
// data structure.
// Bit [Ch] = 1: Selects the primary data structure for channel Ch.
//
// Writing to a bit where a uDMA channel is not implemented has no effect
#define UDMA_CLEARCHNLPRIALT_CHNLS_W                                        32
#define UDMA_CLEARCHNLPRIALT_CHNLS_M                                0xFFFFFFFF
#define UDMA_CLEARCHNLPRIALT_CHNLS_S                                         0

//*****************************************************************************
//
// Register: UDMA_O_SETCHNLPRIORITY
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Returns the channel priority mask status, or sets the channel priority to
// high.
//
// Read as:
// Bit [Ch] = 0: uDMA channel Ch is using the default priority level.
// Bit [Ch] = 1: uDMA channel Ch is using a high priority level.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the CLEARCHNLPRIORITY.CHNLS to set channel Ch
// to the default priority level.
// Bit [Ch] = 1: Channel Ch uses the high priority level.
//
// Writing to a bit where a uDMA channel is not implemented has no effect
#define UDMA_SETCHNLPRIORITY_CHNLS_W                                        32
#define UDMA_SETCHNLPRIORITY_CHNLS_M                                0xFFFFFFFF
#define UDMA_SETCHNLPRIORITY_CHNLS_S                                         0

//*****************************************************************************
//
// Register: UDMA_O_CLEARCHNLPRIORITY
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Clear the appropriate bit to select the default priority level for the
// specified uDMA channel.
//
// Write as:
// Bit [Ch] = 0: No effect. Use the SETCHNLPRIORITY.CHNLS to set channel Ch to
// the high priority level.
// Bit [Ch] = 1: Channel Ch uses the default priority level.
//
// Writing to a bit where a uDMA channel is not implemented has no effect
#define UDMA_CLEARCHNLPRIORITY_CHNLS_W                                      32
#define UDMA_CLEARCHNLPRIORITY_CHNLS_M                              0xFFFFFFFF
#define UDMA_CLEARCHNLPRIORITY_CHNLS_S                                       0

//*****************************************************************************
//
// Register: UDMA_O_ERROR
//
//*****************************************************************************
// Field:     [0] STATUS
//
// Returns the status of bus error flag in uDMA, or clears this bit
//
// Read as:
//
// 0: No bus error detected
// 1: Bus error detected
//
// Write as:
//
// 0: No effect, status of bus error flag is unchanged.
// 1: Clears the bus error flag.
#define UDMA_ERROR_STATUS                                           0x00000001
#define UDMA_ERROR_STATUS_BITN                                               0
#define UDMA_ERROR_STATUS_M                                         0x00000001
#define UDMA_ERROR_STATUS_S                                                  0

//*****************************************************************************
//
// Register: UDMA_O_REQDONE
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Reflects the uDMA done status for the given channel, channel [Ch]. It's a
// sticky done bit. Unless cleared by writing a 1, it holds the value of 1.
//
// Read as:
// Bit [Ch] = 0: Request has not completed for channel Ch
// Bit [Ch] = 1: Request has completed for the channel Ch
//
// Writing a 1 to individual bits would clear the corresponding bit.
//
// Write as:
// Bit [Ch] = 0: No effect.
// Bit [Ch] = 1: The corresponding [Ch] bit is cleared  and is set to 0
#define UDMA_REQDONE_CHNLS_W                                                32
#define UDMA_REQDONE_CHNLS_M                                        0xFFFFFFFF
#define UDMA_REQDONE_CHNLS_S                                                 0

//*****************************************************************************
//
// Register: UDMA_O_DONEMASK
//
//*****************************************************************************
// Field:  [31:0] CHNLS
//
// Controls the propagation of the uDMA done and active state to the assigned
// peripheral. Specifically used for software channels.
//
// Read as:
// Bit [Ch] = 0: uDMA done and active state for channel Ch is not blocked from
// reaching to the peripherals.
// Note that the uDMA done state for channel [Ch] is blocked from contributing
// to generation of combined uDMA done signal
//
// Bit [Ch] = 1: uDMA done and active state for channel Ch is blocked from
// reaching to the peripherals.
// Note that the uDMA done state for channel [Ch] is not blocked from
// contributing to generation of combined uDMA done signal
//
// Write as:
// Bit [Ch] = 0: Allows uDMA done and active stat to propagate to the
// peripherals.
// Note that this disables uDMA done state for channel [Ch] from contributing
// to generation of combined uDMA done signal
//
// Bit [Ch] = 1: Blocks uDMA done and active state to propagate to the
// peripherals.
// Note that this enables uDMA done for  channel [Ch] to contribute to
// generation of combined uDMA done signal.
#define UDMA_DONEMASK_CHNLS_W                                               32
#define UDMA_DONEMASK_CHNLS_M                                       0xFFFFFFFF
#define UDMA_DONEMASK_CHNLS_S                                                0


#endif // __UDMA__
