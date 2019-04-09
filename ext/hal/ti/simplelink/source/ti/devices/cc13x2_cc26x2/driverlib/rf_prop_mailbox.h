/******************************************************************************
*  Filename:       rf_prop_mailbox.h
*  Revised:        2018-01-15 15:58:36 +0100 (Mon, 15 Jan 2018)
*  Revision:       18171
*
*  Description:    Definitions for proprietary mode radio interface
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef _PROP_MAILBOX_H
#define _PROP_MAILBOX_H

/// \name Radio operation status
///@{
/// \name Operation finished normally
///@{
#define PROP_DONE_OK            0x3400  ///< Operation ended normally
#define PROP_DONE_RXTIMEOUT     0x3401  ///< Operation stopped after end trigger while waiting for sync
#define PROP_DONE_BREAK         0x3402  ///< Rx stopped due to timeout in the middle of a packet
#define PROP_DONE_ENDED         0x3403  ///< Operation stopped after end trigger during reception
#define PROP_DONE_STOPPED       0x3404  ///< Operation stopped after stop command
#define PROP_DONE_ABORT         0x3405  ///< Operation aborted by abort command
#define PROP_DONE_RXERR         0x3406  ///< Operation ended after receiving packet with CRC error
#define PROP_DONE_IDLE          0x3407  ///< Carrier sense operation ended because of idle channel
#define PROP_DONE_BUSY          0x3408  ///< Carrier sense operation ended because of busy channel
#define PROP_DONE_IDLETIMEOUT   0x3409  ///< Carrier sense operation ended because of timeout with csConf.timeoutRes = 1
#define PROP_DONE_BUSYTIMEOUT   0x340A  ///< Carrier sense operation ended because of timeout with csConf.timeoutRes = 0

///@}
/// \name Operation finished with error
///@{
#define PROP_ERROR_PAR          0x3800  ///< Illegal parameter
#define PROP_ERROR_RXBUF        0x3801  ///< No available Rx buffer at the start of a packet
#define PROP_ERROR_RXFULL       0x3802  ///< Out of Rx buffer during reception in a partial read buffer
#define PROP_ERROR_NO_SETUP     0x3803  ///< Radio was not set up in proprietary mode
#define PROP_ERROR_NO_FS        0x3804  ///< Synth was not programmed when running Rx or Tx
#define PROP_ERROR_RXOVF        0x3805  ///< Rx overflow observed during operation
#define PROP_ERROR_TXUNF        0x3806  ///< Tx underflow observed during operation
///@}
///@}

#endif
