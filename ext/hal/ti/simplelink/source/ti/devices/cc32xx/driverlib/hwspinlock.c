/*
 *  Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/ 
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 */
//*****************************************************************************
//
//  hwspinlock.c
//
//  Driver for the Apps-NWP spinlock
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup HwSpinLock_api
//! @{
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_common_reg.h"
#include "hwspinlock.h"

//*****************************************************************************
// Global semaphore register list
//*****************************************************************************
static const uint32_t HwSpinLock_RegLst[]=
{
    COMMON_REG_BASE + COMMON_REG_O_SPI_Properties_Register
};

//*****************************************************************************
//
//! Acquire specified spin lock.
//!
//! \param ui32LockID is one of the valid spin lock.
//!
//! This function acquires specified spin lock and will not retun util the
//! specified lock is acquired.
//!
//! The parameter \e ui32LockID should \b HWSPINLOCK_MCSPIS0.
//!
//! return None.
//
//*****************************************************************************
void HwSpinLockAcquire(uint32_t ui32LockID)
{
  uint32_t ui32BitPos;
  uint32_t ui32SemVal;
  uint32_t ui32RegAddr;

  //
  // Extract the bit position from the
  // LockID
  //
  ui32BitPos  = ((ui32LockID >> 16) & 0x0FFF);
  ui32RegAddr = HwSpinLock_RegLst[ui32LockID & 0xF];

  //
  // Set the corresponding
  // ownership bits to 'b01
  //
  ui32SemVal = (0xFFFFFFFF ^ (0x2 << ui32BitPos));

  //
  // Retry untill we succeed
  //
  do
  {
    HWREG(ui32RegAddr) = ui32SemVal;
  }
  while( !(HWREG(ui32RegAddr) & (1 << ui32BitPos )) );

}

//*****************************************************************************
//
//! Try to acquire specified spin lock.
//!
//! \param ui32LockID is one of the valid spin lock.
//! \param ui32Retry is the number of reties.
//!
//! This function tries acquire specified spin lock in \e ui32Retry retries.
//!
//! The parameter \e ui32Retry can be any value between 0 and 2^32.
//!
//! return Returns 0 on success, -1 otherwise.
//
//*****************************************************************************
int32_t HwSpinLockTryAcquire(uint32_t ui32LockID, uint32_t ui32Retry)
{
  uint32_t ui32BitPos;
  uint32_t ui32SemVal;
  uint32_t ui32RegAddr;

  //
  // Extract the bit position from the
  // LockID
  //
  ui32BitPos  = ((ui32LockID >> 16) & 0x0FFF);
  ui32RegAddr = HwSpinLock_RegLst[ui32LockID & 0xF];

  //
  // Set the corresponding
  // ownership bits to 'b01
  //
  ui32SemVal = (0xFFFFFFFF ^ (0x2 << ui32BitPos));

  //
  // Check for 0 retry.
  //
  if(ui32Retry == 0)
  {
    ui32Retry = 1;
  }

  //
  // Retry the number of times specified
  //
  do
  {
    HWREG(ui32RegAddr) = ui32SemVal;
    ui32Retry--;
  }
  while( !(HWREG(ui32RegAddr) & (1 << ui32BitPos )) && ui32Retry );


  //
  // Check the semaphore status
  //
  if(HWREG(ui32RegAddr) & (1 << ui32BitPos ))
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

//*****************************************************************************
//
//! Release a previously owned spin lock
//!
//! \param ui32LockID is one of the valid spin lock.
//!
//! This function releases previously owned spin lock.
//!
//! \return None.
//
//*****************************************************************************
void HwSpinLockRelease(uint32_t ui32LockID)
{
  uint32_t ui32BitPos;
  uint32_t ui32SemVal;

  //
  // Extract the bit position from the
  // lock id.
  //
  ui32BitPos = ((ui32LockID >> 16) & 0x00FF);

  //
  // Release the spin lock, only if already owned
  //
  if(HWREG(HwSpinLock_RegLst[ui32LockID & 0xF]) & (1 << ui32BitPos ))
  {
    ui32SemVal = (0xFFFFFFFF & ~(0x3 << ui32BitPos));
    HWREG(HwSpinLock_RegLst[ui32LockID & 0xF]) = ui32SemVal;
  }
}

//*****************************************************************************
//
//! Get the current or previous ownership status.
//!
//! \param ui32LockID is one of the valid spin lock.
//! \param bCurrentStatus is \b true for current status, \b flase otherwise
//!
//! This function gets the current or previous ownership status of the
//! specified spin lock based on \e bCurrentStatus parameter.
//!
//! \return Returns \b HWSPINLOCK_OWNER_APPS, \b HWSPINLOCK_OWNER_NWP or
//! \b HWSPINLOCK_OWNER_NONE.
//
//*****************************************************************************
uint32_t HwSpinLockTest(uint32_t ui32LockID, bool bCurrentStatus)
{
  uint32_t ui32BitPos;
  uint32_t ui32SemVal;

  if(bCurrentStatus)
  {
    //
    // Extract the bit position from the
    // lock id.
    //
    ui32BitPos = ((ui32LockID >> 16) & 0x00FF);

    //
    // return semaphore
    //
    return((HWREG(HwSpinLock_RegLst[ui32LockID & 0xF]) >> ui32BitPos ) & 0x3 );
  }
  else
  {
    //
    // Extract the bit position
    //
    ui32BitPos = ((ui32LockID >> 24) & 0xFF);

    //
    // Identify which register to read
    //
    if(ui32LockID & 0xF > 4)
    {
      ui32SemVal = ((HWREG(COMMON_REG_BASE +
                     COMMON_REG_O_SEMAPHORE_PREV_OWNER1) >> ui32BitPos ) & 0x3);
    }
    else
    {
      ui32SemVal = ((HWREG(COMMON_REG_BASE +
                     COMMON_REG_O_SEMAPHORE_PREV_OWNER2) >> ui32BitPos ) & 0x3);
    }

    //
    // return the owner
    //
    return ui32SemVal;
  }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
