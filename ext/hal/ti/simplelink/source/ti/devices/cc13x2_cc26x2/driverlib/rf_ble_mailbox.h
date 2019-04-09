/******************************************************************************
*  Filename:       rf_ble_mailbox.h
*  Revised:        2018-01-15 15:58:36 +0100 (Mon, 15 Jan 2018)
*  Revision:       18171
*
*  Description:    Definitions for BLE interface
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

#ifndef _BLE_MAILBOX_H
#define _BLE_MAILBOX_H

/// \name Radio operation status
///@{
/// \name Operation finished normally
///@{
#define BLE_DONE_OK             0x1400  ///< Operation ended normally
#define BLE_DONE_RXTIMEOUT      0x1401  ///< Timeout of first Rx of slave operation or end of scan window
#define BLE_DONE_NOSYNC         0x1402  ///< Timeout of subsequent Rx
#define BLE_DONE_RXERR          0x1403  ///< Operation ended because of receive error (CRC or other)
#define BLE_DONE_CONNECT        0x1404  ///< CONNECT_IND or AUX_CONNECT_RSP received or transmitted
#define BLE_DONE_MAXNACK        0x1405  ///< Maximum number of retransmissions exceeded
#define BLE_DONE_ENDED          0x1406  ///< Operation stopped after end trigger
#define BLE_DONE_ABORT          0x1407  ///< Operation aborted by command
#define BLE_DONE_STOPPED        0x1408  ///< Operation stopped after stop command
#define BLE_DONE_AUX            0x1409  ///< Operation ended after following aux pointer pointing far ahead
#define BLE_DONE_CONNECT_CHSEL0 0x140A  ///< CONNECT_IND received or transmitted; peer does not support channel selection algorithm #2
#define BLE_DONE_SCAN_RSP       0x140B  ///< SCAN_RSP or AUX_SCAN_RSP transmitted
///@}
/// \name Operation finished with error
///@{
#define BLE_ERROR_PAR           0x1800  ///< Illegal parameter
#define BLE_ERROR_RXBUF         0x1801  ///< No available Rx buffer (Advertiser, Scanner, Initiator)
#define BLE_ERROR_NO_SETUP      0x1802  ///< Operation using Rx or Tx attempted when not in BLE mode
#define BLE_ERROR_NO_FS         0x1803  ///< Operation using Rx or Tx attempted without frequency synth configured
#define BLE_ERROR_SYNTH_PROG    0x1804  ///< Synthesizer programming failed to complete on time
#define BLE_ERROR_RXOVF         0x1805  ///< Receiver overflowed during operation
#define BLE_ERROR_TXUNF         0x1806  ///< Transmitter underflowed during operation
#define BLE_ERROR_AUX           0x1807  ///< Calculated AUX pointer was too far into the future or in the past
///@}
///@}


/// Special trigger for BLE slave command
#define BLE_TRIG_REL_SYNC       15

#endif
