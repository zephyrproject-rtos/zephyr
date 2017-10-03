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
//  flash.c
//
//  Driver for programming the on-chip flash.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup flash_api
//! @{
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_flash_ctrl.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_gprcm.h"
#include "inc/hw_hib1p2.h"
#include "inc/hw_hib3p3.h"
#include "inc/hw_common_reg.h"
#include "inc/hw_stack_die_ctrl.h"
#include "debug.h"
#include "flash.h"
#include "utils.h"
#include "interrupt.h"

#define HAVE_WRITE_BUFFER       1



//*****************************************************************************
//
// An array that maps the specified memory bank to the appropriate Flash
// Memory Protection Program Enable (FMPPE) register.
//
//*****************************************************************************
static const unsigned long g_pulFMPPERegs[] =
{
    FLASH_FMPPE0,
    FLASH_FMPPE1,
    FLASH_FMPPE2,
    FLASH_FMPPE3,
    FLASH_FMPPE4,
    FLASH_FMPPE5,
    FLASH_FMPPE6,
    FLASH_FMPPE7,
    FLASH_FMPPE8,
    FLASH_FMPPE9,
    FLASH_FMPPE10,
    FLASH_FMPPE11,
    FLASH_FMPPE12,
    FLASH_FMPPE13,
    FLASH_FMPPE14,
    FLASH_FMPPE15


};

//*****************************************************************************
//
// An array that maps the specified memory bank to the appropriate Flash
// Memory Protection Read Enable (FMPRE) register.
//
//*****************************************************************************
static const unsigned long g_pulFMPRERegs[] =
{
    FLASH_FMPRE0,
    FLASH_FMPRE1,
    FLASH_FMPRE2,
    FLASH_FMPRE3,
    FLASH_FMPRE4,
    FLASH_FMPRE5,
    FLASH_FMPRE6,
    FLASH_FMPRE7,
    FLASH_FMPRE8,
    FLASH_FMPRE9,
    FLASH_FMPRE10,
    FLASH_FMPRE11,
    FLASH_FMPRE12,
    FLASH_FMPRE13,
    FLASH_FMPRE14,
    FLASH_FMPRE15,
};

//*****************************************************************************
//
//! Flash Disable
//!
//! This function Disables the internal Flash.
//!
//! \return None.
//
//*****************************************************************************
void
FlashDisable()
{

  //
  // Wait for Flash Busy to get cleared
  //
  while((HWREG(GPRCM_BASE + GPRCM_O_TOP_DIE_ENABLE)
          & GPRCM_TOP_DIE_ENABLE_FLASH_BUSY))
  {

  }

  //
  // Assert reset
  //
  HWREG(HIB1P2_BASE + HIB1P2_O_PORPOL_SPARE) = 0xFFFF0000;

  //
  // 50 usec Delay Loop
  //
  UtilsDelay((50*80)/3);

  //
  // Disable TDFlash
  //
  HWREG(GPRCM_BASE + GPRCM_O_TOP_DIE_ENABLE) = 0x0;

  //
  // 50 usec Delay Loop
  //
  UtilsDelay((50*80)/3);

  HWREG(HIB1P2_BASE + HIB1P2_O_BGAP_DUTY_CYCLING_EXIT_CFG) = 0x1;

  //
  // 50 usec Delay Loop
  //
  UtilsDelay((50*80)/3);
}


//*****************************************************************************
//
//! Erases a block of flash.
//!
//! \param ulAddress is the start address of the flash block to be erased.
//!
//! This function will erase a 2 kB block of the on-chip flash.  After erasing,
//! the block will be filled with 0xFF bytes.  Read-only and execute-only
//! blocks cannot be erased.
//!
//! This function will not return until the block has been erased.
//!
//! \return Returns 0 on success, or -1 if an invalid block address was
//! specified or the block is write-protected.
//
//*****************************************************************************
long
FlashErase(unsigned long ulAddress)
{
    //
    // Check the arguments.
    //
    ASSERT(!(ulAddress & (FLASH_CTRL_ERASE_SIZE - 1)));

    //
    // Clear the flash access and error interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC)
      = (FLASH_CTRL_FCMISC_AMISC | FLASH_CTRL_FCMISC_VOLTMISC |
                           FLASH_CTRL_FCMISC_ERMISC);

    // Erase the block.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMA) = ulAddress;
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC)
                                = FLASH_CTRL_FMC_WRKEY | FLASH_CTRL_FMC_ERASE;

    //
    // Wait until the block has been erased.
    //
    while(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) & FLASH_CTRL_FMC_ERASE)
    {
    }

    //
    // Return an error if an access violation or erase error occurred.
    //
    if(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCRIS)
       & (FLASH_CTRL_FCRIS_ARIS | FLASH_CTRL_FCRIS_VOLTRIS |
                             FLASH_CTRL_FCRIS_ERRIS))


    {
        return(-1);
    }

    //
    // Success.
    //
    return(0);
}


//*****************************************************************************
//
//! Erases a block of flash but does not wait for completion.
//!
//! \param ulAddress is the start address of the flash block to be erased.
//!
//! This function will erase a 2 kB block of the on-chip flash.  After erasing,
//! the block will be filled with 0xFF bytes.  Read-only and execute-only
//! blocks cannot be erased.
//!
//! This function will return immediately after commanding the erase operation.
//! Applications making use of the function can determine completion state by
//! using a flash interrupt handler or by polling FlashIntStatus.
//!
//! \return None.
//
//*****************************************************************************
void
FlashEraseNonBlocking(unsigned long ulAddress)
{
    //
    // Check the arguments.
    //
    ASSERT(!(ulAddress & (FLASH_CTRL_ERASE_SIZE - 1)));

    //
    // Clear the flash access and error interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC) =
      (FLASH_CTRL_FCMISC_AMISC | FLASH_CTRL_FCMISC_VOLTMISC |
                           FLASH_CTRL_FCMISC_ERMISC);

    //
    // Command the flash controller to erase the block.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMA) = ulAddress;
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) = FLASH_CTRL_FMC_WRKEY | FLASH_CTRL_FMC_ERASE;
}


//*****************************************************************************
//
//! Erases a complele flash at shot.
//!
//! This function erases a complele flash at shot
//!
//! \return Returns 0 on success, or -1 if the block is write-protected.
//
//*****************************************************************************
long
FlashMassErase()
{
    //
    // Clear the flash access and error interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC) =
      (FLASH_CTRL_FCMISC_AMISC | FLASH_CTRL_FCMISC_VOLTMISC |
                           FLASH_CTRL_FCMISC_ERMISC);

    //
    // Command the flash controller for mass erase.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) =
      FLASH_CTRL_FMC_WRKEY | FLASH_CTRL_FMC_MERASE1;

    //
    // Wait until mass erase completes.
    //
    while(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) & FLASH_CTRL_FMC_MERASE1)
    {

    }

    //
    // Return an error if an access violation or erase error occurred.
    //
    if(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCRIS)
       & (FLASH_CTRL_FCRIS_ARIS | FLASH_CTRL_FCRIS_VOLTRIS |
                             FLASH_CTRL_FCRIS_ERRIS))
    {
        return -1;
    }

    //
    // Success.
    //
    return 0;
}

//*****************************************************************************
//
//! Erases a complele flash at shot but does not wait for completion.
//!
//!
//! This function will not return until the Flash has been erased.
//!
//! \return None.
//
//*****************************************************************************
void
FlashMassEraseNonBlocking()
{
    //
    // Clear the flash access and error interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC) =
      (FLASH_CTRL_FCMISC_AMISC | FLASH_CTRL_FCMISC_VOLTMISC |
                           FLASH_CTRL_FCMISC_ERMISC);

    //
    // Command the flash controller for mass erase.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) =
      FLASH_CTRL_FMC_WRKEY | FLASH_CTRL_FMC_MERASE1;

}

//*****************************************************************************
//
//! Programs flash.
//!
//! \param pulData is a pointer to the data to be programmed.
//! \param ulAddress is the starting address in flash to be programmed.  Must
//! be a multiple of four.
//! \param ulCount is the number of bytes to be programmed.  Must be a multiple
//! of four.
//!
//! This function will program a sequence of words into the on-chip flash.
//! Each word in a page of flash can only be programmed one time between an
//! erase of that page; programming a word multiple times will result in an
//! unpredictable value in that word of flash.
//!
//! Since the flash is programmed one word at a time, the starting address and
//! byte count must both be multiples of four.  It is up to the caller to
//! verify the programmed contents, if such verification is required.
//!
//! This function will not return until the data has been programmed.
//!
//! \return Returns 0 on success, or -1 if a programming error is encountered.
//
//*****************************************************************************
long
FlashProgram(unsigned long *pulData, unsigned long ulAddress,
             unsigned long ulCount)
{
    //
    // Check the arguments.
    //
    ASSERT(!(ulAddress & 3));
    ASSERT(!(ulCount & 3));

    //
    // Clear the flash access and error interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC)
      = (FLASH_CTRL_FCMISC_AMISC | FLASH_CTRL_FCMISC_VOLTMISC |
                           FLASH_CTRL_FCMISC_INVDMISC | FLASH_CTRL_FCMISC_PROGMISC);


    //
    // See if this device has a write buffer.
    //

#if HAVE_WRITE_BUFFER
    {
        //
        // Loop over the words to be programmed.
        //
        while(ulCount)
        {
            //
            // Set the address of this block of words. for 1 MB
            //
            HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMA) = ulAddress & ~(0x7F);

            //
            // Loop over the words in this 32-word block.
            //
            while(((ulAddress & 0x7C) ||
                   (HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FWBVAL) == 0)) &&
                  (ulCount != 0))
            {
                //
                // Write this word into the write buffer.
                //
                HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FWBN
                      + (ulAddress & 0x7C)) = *pulData++;
                ulAddress += 4;
                ulCount -= 4;
            }

            //
            // Program the contents of the write buffer into flash.
            //
            HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC2)
              = FLASH_CTRL_FMC2_WRKEY | FLASH_CTRL_FMC2_WRBUF;

            //
            // Wait until the write buffer has been programmed.
            //
            while(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC2) & FLASH_CTRL_FMC2_WRBUF)
            {
            }
        }
    }
#else
    {
        //
        // Loop over the words to be programmed.
        //
        while(ulCount)
        {
            //
            // Program the next word.
            //
            HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMA) = ulAddress;
            HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMD) = *pulData;
            HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) = FLASH_CTRL_FMC_WRKEY | FLASH_CTRL_FMC_WRITE;

            //
            // Wait until the word has been programmed.
            //
            while(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) & FLASH_CTRL_FMC_WRITE)
            {
            }

            //
            // Increment to the next word.
            //
            pulData++;
            ulAddress += 4;
            ulCount -= 4;
        }
    }
#endif
    //
    // Return an error if an access violation occurred.
    //

    if(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCRIS) & (FLASH_CTRL_FCRIS_ARIS | FLASH_CTRL_FCRIS_VOLTRIS |
                             FLASH_CTRL_FCRIS_INVDRIS | FLASH_CTRL_FCRIS_PROGRIS))

    {
        return(-1);
    }

    //
    // Success.
    //
    return(0);
}


//*****************************************************************************
//
//! Programs flash but does not poll for completion.
//!
//! \param pulData is a pointer to the data to be programmed.
//! \param ulAddress is the starting address in flash to be programmed.  Must
//! be a multiple of four.
//! \param ulCount is the number of bytes to be programmed.  Must be a multiple
//! of four.
//!
//! This function will start programming one or more words into the on-chip
//! flash and return immediately.  The number of words that can be programmed
//! in a single call depends the part on which the function is running.  For
//! parts without support for a flash write buffer, only a single word may be
//! programmed on each call to this function (\e ulCount must be 1).  If a
//! write buffer is present, up to 32 words may be programmed on condition
//! that the block being programmed does not straddle a 32 word address
//! boundary.  For example, wherease 32 words can be programmed if the address
//! passed is 0x100 (a multiple of 128 bytes or 32 words), only 31 words could
//! be programmed at 0x104 since attempting to write 32 would cross the 32
//! word boundary at 0x180.
//!
//! Since the flash is programmed one word at a time, the starting address and
//! byte count must both be multiples of four.  It is up to the caller to
//! verify the programmed contents, if such verification is required.
//!
//! This function will return immediately after commanding the erase operation.
//! Applications making use of the function can determine completion state by
//! using a flash interrupt handler or by polling FlashIntStatus.
//!
//! \return 0 if the write was started successfully, -1 if there was an error.
//
//*****************************************************************************
long
FlashProgramNonBlocking(unsigned long *pulData, unsigned long ulAddress,
                        unsigned long ulCount)
{
    //
    // Check the arguments.
    //
    ASSERT(!(ulAddress & 3));
    ASSERT(!(ulCount & 3));

    //
    // Clear the flash access and error interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC)
      = (FLASH_CTRL_FCMISC_AMISC | FLASH_CTRL_FCMISC_VOLTMISC |
                           FLASH_CTRL_FCMISC_INVDMISC | FLASH_CTRL_FCMISC_PROGMISC);

    //
    // See if this device has a write buffer.
    //

#if HAVE_WRITE_BUFFER
    {
        //
        // Make sure the address/count specified doesn't straddle a 32 word
        // boundary.
        //
        if(((ulAddress + (ulCount - 1)) & ~0x7F) != (ulAddress & ~0x7F))
        {
            return(-1);
        }

        //
        // Loop over the words to be programmed.
        //
        while(ulCount)
        {
            //
            // Set the address of this block of words.
            //
            HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMA) = ulAddress & ~(0x7F);

            //
            // Loop over the words in this 32-word block.
            //
            while(((ulAddress & 0x7C) || (HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FWBVAL) == 0)) &&
                  (ulCount != 0))
            {
                //
                // Write this word into the write buffer.
                //
                HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FWBN + (ulAddress & 0x7C)) = *pulData++;
                ulAddress += 4;
                ulCount -= 4;
            }

            //
            // Program the contents of the write buffer into flash.
            //
            HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC2) = FLASH_CTRL_FMC2_WRKEY | FLASH_CTRL_FMC2_WRBUF;
        }
    }
#else
    {
        //
        // We don't have a write buffer so we can only write a single word.
        //
        if(ulCount > 1)
        {
            return(-1);
        }

        //
        // Write a single word.
        //
        HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMA) = ulAddress;
        HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMD) = *pulData;
        HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FMC) = FLASH_CTRL_FMC_WRKEY | FLASH_CTRL_FMC_WRITE;
    }
#endif
    //
    // Success.
    //
    return(0);
}


//*****************************************************************************
//
//! Gets the protection setting for a block of flash.
//!
//! \param ulAddress is the start address of the flash block to be queried.
//!
//! This function gets the current protection for the specified 2-kB block
//! of flash.  Each block can be read/write, read-only, or execute-only.
//! Read/write blocks can be read, executed, erased, and programmed.  Read-only
//! blocks can be read and executed.  Execute-only blocks can only be executed;
//! processor and debugger data reads are not allowed.
//!
//! \return Returns the protection setting for this block.  See
//! FlashProtectSet() for possible values.
//
//*****************************************************************************
tFlashProtection
FlashProtectGet(unsigned long ulAddress)
{
    unsigned long ulFMPRE, ulFMPPE;
    unsigned long ulBank;

    //
    // Check the argument.
    //
    ASSERT(!(ulAddress & (FLASH_PROTECT_SIZE - 1)));

    //
    // Calculate the Flash Bank from Base Address, and mask off the Bank
    // from ulAddress for subsequent reference.
    //
    ulBank = (((ulAddress / FLASH_PROTECT_SIZE) / 32) % 16);
    ulAddress &= ((FLASH_PROTECT_SIZE * 32) - 1);

    //
    // Read the appropriate flash protection registers for the specified
    // flash bank.
    //
    ulFMPRE = HWREG(g_pulFMPRERegs[ulBank]);
    ulFMPPE = HWREG(g_pulFMPPERegs[ulBank]);

    //
    // Check the appropriate protection bits for the block of memory that
    // is specified by the address.
    //
    switch((((ulFMPRE >> (ulAddress / FLASH_PROTECT_SIZE)) &
             FLASH_FMP_BLOCK_0) << 1) |
           ((ulFMPPE >> (ulAddress / FLASH_PROTECT_SIZE)) & FLASH_FMP_BLOCK_0))
    {
        //
        // This block is marked as execute only (that is, it can not be erased
        // or programmed, and the only reads allowed are via the instruction
        // fetch interface).
        //
        case 0:
        case 1:
        {
            return(FlashExecuteOnly);
        }

        //
        // This block is marked as read only (that is, it can not be erased or
        // programmed).
        //
        case 2:
        {
            return(FlashReadOnly);
        }

        //
        // This block is read/write; it can be read, erased, and programmed.
        //
        case 3:
        default:
        {
            return(FlashReadWrite);
        }
    }
}

//*****************************************************************************
//
//! Registers an interrupt handler for the flash interrupt.
//!
//! \param pfnHandler is a pointer to the function to be called when the flash
//! interrupt occurs.
//!
//! This sets the handler to be called when the flash interrupt occurs.  The
//! flash controller can generate an interrupt when an invalid flash access
//! occurs, such as trying to program or erase a read-only block, or trying to
//! read from an execute-only block.  It can also generate an interrupt when a
//! program or erase operation has completed.  The interrupt will be
//! automatically enabled when the handler is registered.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
FlashIntRegister(void (*pfnHandler)(void))
{
    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    IntRegister(INT_FLASH, pfnHandler);

    //
    // Enable the flash interrupt.
    //
    IntEnable(INT_FLASH);
}

//*****************************************************************************
//
//! Unregisters the interrupt handler for the flash interrupt.
//!
//! This function will clear the handler to be called when the flash interrupt
//! occurs.  This will also mask off the interrupt in the interrupt controller
//! so that the interrupt handler is no longer called.
//!
//! \sa IntRegister() for important information about registering interrupt
//! handlers.
//!
//! \return None.
//
//*****************************************************************************
void
FlashIntUnregister(void)
{
    //
    // Disable the interrupt.
    //
    IntDisable(INT_FLASH);

    //
    // Unregister the interrupt handler.
    //
    IntUnregister(INT_FLASH);
}

//*****************************************************************************
//
//! Enables individual flash controller interrupt sources.
//!
//! \param ulIntFlags is a bit mask of the interrupt sources to be enabled.
//! Can be any of the \b FLASH_CTRL_PROGRAM or \b FLASH_CTRL_ACCESS values.
//!
//! Enables the indicated flash controller interrupt sources.  Only the sources
//! that are enabled can be reflected to the processor interrupt; disabled
//! sources have no effect on the processor.
//!
//! \return None.
//
//*****************************************************************************
void
FlashIntEnable(unsigned long ulIntFlags)
{
    //
    // Enable the specified interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCIM) |= ulIntFlags;
}

//*****************************************************************************
//
//! Disables individual flash controller interrupt sources.
//!
//! \param ulIntFlags is a bit mask of the interrupt sources to be disabled.
//! Can be any of the \b FLASH_CTRL_PROGRAM or \b FLASH_CTRL_ACCESS values.
//!
//! Disables the indicated flash controller interrupt sources.  Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \return None.
//
//*****************************************************************************
void
FlashIntDisable(unsigned long ulIntFlags)
{
    //
    // Disable the specified interrupts.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCIM) &= ~(ulIntFlags);
}

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \param bMasked is false if the raw interrupt status is required and true if
//! the masked interrupt status is required.
//!
//! This returns the interrupt status for the flash controller.  Either the raw
//! interrupt status or the status of interrupts that are allowed to reflect to
//! the processor can be returned.
//!
//! \return The current interrupt status, enumerated as a bit field of
//! \b FLASH_CTRL_PROGRAM and \b FLASH_CTRL_ACCESS.
//
//*****************************************************************************
unsigned long
FlashIntStatus(tBoolean bMasked)
{
    //
    // Return either the interrupt status or the raw interrupt status as
    // requested.
    //
    if(bMasked)
    {
        return(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC));
    }
    else
    {
        return(HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCRIS));
    }
}

//*****************************************************************************
//
//! Clears flash controller interrupt sources.
//!
//! \param ulIntFlags is the bit mask of the interrupt sources to be cleared.
//! Can be any of the \b FLASH_CTRL_PROGRAM or \b FLASH_CTRL_AMISC values.
//!
//! The specified flash controller interrupt sources are cleared, so that they
//! no longer assert.  This must be done in the interrupt handler to keep it
//! from being called again immediately upon exit.
//!
//! \note Because there is a write buffer in the Cortex-M3 processor, it may
//! take several clock cycles before the interrupt source is actually cleared.
//! Therefore, it is recommended that the interrupt source be cleared early in
//! the interrupt handler (as opposed to the very last action) to avoid
//! returning from the interrupt handler before the interrupt source is
//! actually cleared.  Failure to do so may result in the interrupt handler
//! being immediately reentered (because the interrupt controller still sees
//! the interrupt source asserted).
//!
//! \return None.
//
//*****************************************************************************
void
FlashIntClear(unsigned long ulIntFlags)
{
    //
    // Clear the flash interrupt.
    //
    HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCMISC) = ulIntFlags;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
