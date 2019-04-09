/******************************************************************************
*  Filename:       driverlib_release.h
*  Revised:        $Date: 2015-07-16 12:12:04 +0200 (Thu, 16 Jul 2015) $
*  Revision:       $Revision: 44151 $
*
*  Description:    Provides macros for ensuring that a specfic release of
*                  DriverLib is used.
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

//*****************************************************************************
//
//! \addtogroup system_control_group
//! @{
//! \addtogroup driverlib_release_api
//! @{
//
//*****************************************************************************

#ifndef __DRIVERLIB_RELEASE_H__
#define __DRIVERLIB_RELEASE_H__


#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>




/// DriverLib release group number
#define DRIVERLIB_RELEASE_GROUP   0
/// DriverLib release build number
#define DRIVERLIB_RELEASE_BUILD   54539




//*****************************************************************************
//
//! This macro is called internally from within DriverLib to declare the
//! DriverLib release locking object:
//! \param group is the DriverLib release group number.
//! \param build is the DriverLib release build number.
//!
//! This macro shall not be called in the application unless the intention is
//! to bypass the release locking (at own risk).
//
//*****************************************************************************
#define DRIVERLIB_DECLARE_RELEASE(group, build) \
    const volatile uint8_t driverlib_release_##group##_##build

/// External declaration of the DriverLib release locking object
extern DRIVERLIB_DECLARE_RELEASE(0, 54539);




//*****************************************************************************
//
//! This macro shall be called once from within a function of a precompiled
//! software deliverable to lock the deliverable to a specific DriverLib
//! release. It is essential that the call is made from code that is not
//! optimized away.
//!
//! This macro locks to a specific DriverLib release:
//! \param group is the DriverLib release group number.
//! \param build is the DriverLib release build number.
//!
//! If attempting to use the precompiled deliverable with a different release
//! of DriverLib, a linker error will be produced, stating that
//! "driverlib_release_xx_yyyyy is undefined" or similar.
//!
//! To override the check, for example when upgrading DriverLib but not the
//! precompiled deliverables, or when mixing precompiled deliverables,
//! application developers may (at own risk) declare the missing DriverLib
//! release using the \ref DRIVERLIB_DECLARE_RELEASE() macro.
//
//*****************************************************************************
#define DRIVERLIB_ASSERT_RELEASE(group, build) \
    (driverlib_release_##group##_##build)




//*****************************************************************************
//
//! This macro shall be called once from within a function of a precompiled
//! software deliverable to lock the deliverable to a specific DriverLib
//! release. It is essential that the call is made from code that is not
//! optimized away.
//!
//! This macro locks to the current DriverLib release used at compile-time.
//!
//! If attempting to use the precompiled deliverable with a different release
//! of DriverLib, a linker error will be produced, stating that
//! "driverlib_release_xx_yyyyy is undefined" or similar.
//!
//! To override the check, for example when upgrading DriverLib but not the
//! precompiled deliverables, or when mixing precompiled deliverables,
//! application developers may (at own risk) declare the missing DriverLib
//! release using the \ref DRIVERLIB_DECLARE_RELEASE() macro.
//
//*****************************************************************************
#define DRIVERLIB_ASSERT_CURR_RELEASE() \
    DRIVERLIB_ASSERT_RELEASE(0, 54539)




#ifdef __cplusplus
}
#endif

#endif // __DRIVERLIB_RELEASE_H__


//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
