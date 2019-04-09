/******************************************************************************
*  Filename:       pka_doc.h
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
//! \addtogroup pka_api
//! @{
//! \section sec_pka Introduction
//!
//! The PKA (Public Key Accelerator) API provides access to the Large Number
//! Engine (LNME). The LNME allows for efficient math operations on numbers
//! larger than those that fit within the ALU of the system CPU. It is significantly faster
//! to perform these operations using the LNME than implementing the same
//! functionality in software using regular word-wise math operations. While the
//! LNME runs in the background, the system CPU may perform other operations
//! or be turned off.
//!
//! The LNME supports both primitive math operations and serialized primitive
//! operations (sequencer operations).
//!     - Addition
//!     - Multiplication
//!     - Comparison
//!     - Modulo
//!     - Inverse Modulo
//!     - ECC Point Addition (including point doubling)
//!     - ECC Scalar Multiplication
//!
//! These primitives and sequencer operations can be used to implement various
//! public key encryption schemes.
//! It is possible to implement the following schemes using the operations mentioned above:
//!     - RSA encryption and decryption
//!     - RSA sign and verify
//!     - DHE (Diffie-Hellman Key Exchange)
//!     - ECDH (Elliptic Curve Diffie-Hellman Key Exchange)
//!     - ECDSA (Elliptic Curve Digital Signature Algorithm)
//!     - ECIES (Elliptic Curve Integrated Encryption Scheme)
//!
//! The DriverLib PKA functions copy the relevant parameters into the dedicated
//! PKA RAM. The LNME requires these parameters be present and correctly
//! formatted in the PKA RAM and not system RAM. They are copied word-wise as
//! the PKA RAM does not support byte-wise access. The CPU handles the alignment differences
//! during the memory copy operation. Forcing buffer alignment in system RAM results
//! in a significant speedup of the copy operation compared to unaligned buffers.
//!
//! When the operation completes, the result is copied back into
//! a buffer in system RAM specified by the application. The PKA RAM is then cleared
//! to prevent sensitive keying material from remaining in PKA RAM.
//!
//!
//! @}
