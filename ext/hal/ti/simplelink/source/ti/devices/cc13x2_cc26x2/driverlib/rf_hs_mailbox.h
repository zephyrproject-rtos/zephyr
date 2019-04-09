/******************************************************************************
*  Filename:       rf_hs_mailbox.h
*  Revised:        2018-01-15 15:58:36 +0100 (Mon, 15 Jan 2018)
*  Revision:       18171
*
*  Description:     Definitions for high-speed mode radio interface
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

#ifndef _HS_MAILBOX_H
#define _HS_MAILBOX_H

/// \name Radio operation status
///@{
/// \name Operation finished normally
///@{
#define HS_DONE_OK            0x3440  ///< Operation ended normally
#define HS_DONE_RXTIMEOUT     0x3441  ///< Operation stopped after end trigger while waiting for sync
#define HS_DONE_RXERR         0x3442  ///< Operation ended after CRC error
#define HS_DONE_TXBUF         0x3443  ///< Tx queue was empty at start of operation
#define HS_DONE_ENDED         0x3444  ///< Operation stopped after end trigger during reception
#define HS_DONE_STOPPED       0x3445  ///< Operation stopped after stop command
#define HS_DONE_ABORT         0x3446  ///< Operation aborted by abort command
///@}
/// \name Operation finished with error
///@{
#define HS_ERROR_PAR          0x3840  ///< Illegal parameter
#define HS_ERROR_RXBUF        0x3841  ///< No available Rx buffer at the start of a packet
#define HS_ERROR_NO_SETUP     0x3842  ///< Radio was not set up in a compatible mode
#define HS_ERROR_NO_FS        0x3843  ///< Synth was not programmed when running Rx or Tx
#define HS_ERROR_RXOVF        0x3844  ///< Rx overflow observed during operation
#define HS_ERROR_TXUNF        0x3845  ///< Tx underflow observed during operation
///@}
///@}

#endif
