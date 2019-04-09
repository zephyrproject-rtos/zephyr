/******************************************************************************
*  Filename:       rf_ieee_mailbox.h
*  Revised:        2018-01-23 19:51:42 +0100 (Tue, 23 Jan 2018)
*  Revision:       18189
*
*  Description:    Definitions for IEEE 802.15.4 interface
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

#ifndef _IEEE_MAILBOX_H
#define _IEEE_MAILBOX_H

#include "rf_mailbox.h"

/// \name Radio operation status
///@{
/// \name Operation not finished
///@{
#define IEEE_SUSPENDED          0x2001  ///< Operation suspended
///@}
/// \name Operation finished normally
///@{
#define IEEE_DONE_OK            0x2400  ///< Operation ended normally
#define IEEE_DONE_BUSY          0x2401  ///< CSMA-CA operation ended with failure
#define IEEE_DONE_STOPPED       0x2402  ///< Operation stopped after stop command
#define IEEE_DONE_ACK           0x2403  ///< ACK packet received with pending data bit cleared
#define IEEE_DONE_ACKPEND       0x2404  ///< ACK packet received with pending data bit set
#define IEEE_DONE_TIMEOUT       0x2405  ///< Operation ended due to timeout
#define IEEE_DONE_BGEND         0x2406  ///< FG operation ended because necessary background level
                                        ///< operation ended
#define IEEE_DONE_ABORT         0x2407  ///< Operation aborted by command
///@}
/// \name Operation finished with error
///@{
#define IEEE_ERROR_PAR          0x2800  ///< Illegal parameter
#define IEEE_ERROR_NO_SETUP     0x2801  ///< Operation using Rx or Tx attempted when not in 15.4 mode
#define IEEE_ERROR_NO_FS        0x2802  ///< Operation using Rx or Tx attempted without frequency synth configured
#define IEEE_ERROR_SYNTH_PROG   0x2803  ///< Synthesizer programming failed to complete on time
#define IEEE_ERROR_RXOVF        0x2804  ///< Receiver overflowed during operation
#define IEEE_ERROR_TXUNF        0x2805  ///< Transmitter underflowed during operation
///@}
///@}

#endif
