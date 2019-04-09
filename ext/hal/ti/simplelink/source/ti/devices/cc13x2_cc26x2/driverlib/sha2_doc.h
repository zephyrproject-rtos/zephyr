/******************************************************************************
*  Filename:       sha2_doc.h
*  Revised:        2017-11-01 10:33:37 +0100 (Wed, 01 Nov 2017)
*  Revision:       50125
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
//! \addtogroup sha2_api
//! @{
//! \section sec_sha2 Introduction
//!
//! The SHA-2 (Secure Hash Algorithm) API provides access to the SHA-2
//!	functionality of the crypto core. The AES accelerator and keystore are
//! also contained  within the crypto core. Hence, only one of SHA-2 and AES
//! may be used at the same time.
//! This module offers hardware acceleration for the SHA-2 family of hash
//! algorithms. The following output digest sizes are supported:
//! - 224 bits
//! - 256 bits
//! - 384 bits
//! - 512 bits
//!
//! Messages are hashed in one go or in multiple steps. Stepwise hashing
//! consists of an initial hash, multiple intermediate hashes, and a
//! finalization hash.
//!
//! The crypto core does not have retention and all configuration settings
//! are lost when going into standby or shutdown. If you wish to continue
//!	a hash operation after going into standby or shutdown, you must load
//! the intermediate hash into system RAM before entering standby or shutdown
//! and load the intermediate hash back into the crypto module after resuming
//! operation.
//!
//! @}
