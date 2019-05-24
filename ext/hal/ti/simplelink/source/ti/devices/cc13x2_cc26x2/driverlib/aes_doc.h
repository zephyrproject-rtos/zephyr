/******************************************************************************
*  Filename:       aes_doc.h
*  Revised:        2017-06-05 12:13:49 +0200 (Mon, 05 Jun 2017)
*  Revision:       49096
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
//! \addtogroup aes_api
//! @{
//! \section sec_aes Introduction
//!
//! The AES (advanced encryption standard) API provides access to the AES and key
//! store functionality of the crypto core. The SHA2 accelerator is also
//! contained within the crypto core. Hence, only one of SHA2 and AES may be
//! used at the same time.
//! This module offers hardware acceleration for several protocols using the
//! AES block cypher. The protocols below are supported by the hardware. The
//! driverlib documentation only explicitly references the most commonly used ones.
//! - ECB
//! - CBC
//! - CCM
//! - CBC-MAC
//! - GCM
//!
//! The key store is a section of crypto memory that is only accessible to the crypto module
//! and may be written to by the application via the crypto DMA. It is not possible to
//! read from the key store to main memory. Thereby, it is not possible to
//! compromise the key should the application be hacked if the original key in main
//! memory was overwritten already.
//!
//! The crypto core does not have retention and all configuration settings and
//! keys in the keystore are lost when going into standby or shutdown.
//! The typical security advantages a key store offers are not available in these
//! low power modes as the key must be saved in regular memory to reload
//! it after going into standby or shutdown.
//! Consequently, the keystore primarily serves as an interface to the AES accelerator.
//!
//! @}
