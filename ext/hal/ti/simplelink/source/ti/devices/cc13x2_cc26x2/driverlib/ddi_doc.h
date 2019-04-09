/******************************************************************************
*  Filename:       ddi_doc.h
*  Revised:        2016-08-30 14:34:13 +0200 (Tue, 30 Aug 2016)
*  Revision:       47080
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
//! \addtogroup ddi_api
//! @{
//! \section sec_ddi Introduction
//! \n
//!
//! \section sec_ddi_api API
//!
//! The API functions can be grouped like this:
//!
//! Write:
//! - Direct (all bits):
//!   - \ref DDI32RegWrite()
//! - Set individual bits:
//!   - \ref DDI32BitsSet()
//! - Clear individual bits:
//!   - \ref DDI32BitsClear()
//! - Masked:
//!   - \ref DDI8SetValBit()
//!   - \ref DDI16SetValBit()
//! - Special functions using masked write:
//!   - \ref DDI16BitWrite()
//!   - \ref DDI16BitfieldWrite()
//!
//! Read:
//! - Direct (all bits):
//!   - \ref DDI32RegRead()
//! - Special functions using masked read:
//!   - \ref DDI16BitRead()
//!   - \ref DDI16BitfieldRead()
//!
//!
//! @}
