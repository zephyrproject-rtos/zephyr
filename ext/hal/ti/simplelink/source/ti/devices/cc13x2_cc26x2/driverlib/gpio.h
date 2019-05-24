/******************************************************************************
*  Filename:       gpio.h
*  Revised:        2018-05-02 11:11:40 +0200 (Wed, 02 May 2018)
*  Revision:       51951
*
*  Description:    Defines and prototypes for the GPIO.
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
//! \addtogroup peripheral_group
//! @{
//! \addtogroup gpio_api
//! @{
//
//*****************************************************************************

#ifndef __GPIO_H__
#define __GPIO_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_gpio.h"
#include "debug.h"

//*****************************************************************************
//
// Check for legal range of variable dioNumber
//
//*****************************************************************************
#ifdef DRIVERLIB_DEBUG
#include "../inc/hw_fcfg1.h"
#include "chipinfo.h"

static bool
dioNumberLegal( uint32_t dioNumber )
{
    uint32_t ioCount =
        (( HWREG( FCFG1_BASE + FCFG1_O_IOCONF ) &
            FCFG1_IOCONF_GPIO_CNT_M ) >>
            FCFG1_IOCONF_GPIO_CNT_S ) ;

    // CC13x2 + CC26x2
    if ( ChipInfo_ChipFamilyIs_CC13x2_CC26x2() )
    {
        return ( (dioNumber >= (31 - ioCount)) && (dioNumber < 31) )
    }
    // Special handling of CC13x0 7x7, where IO_CNT = 30 and legal range is 1..30
    // for all other chips legal range is 0..(dioNumber-1)
    else if (( ioCount == 30 ) && ChipInfo_ChipFamilyIs_CC13x0() )
    {
        return (( dioNumber > 0 ) && ( dioNumber <= ioCount ));
    }
    else
    {
        return ( dioNumber < ioCount );
    }

}
#endif

//*****************************************************************************
//
// The following values define the bit field for the GPIO DIOs.
//
//*****************************************************************************
#define GPIO_DIO_0_MASK         0x00000001  // GPIO DIO 0 mask
#define GPIO_DIO_1_MASK         0x00000002  // GPIO DIO 1 mask
#define GPIO_DIO_2_MASK         0x00000004  // GPIO DIO 2 mask
#define GPIO_DIO_3_MASK         0x00000008  // GPIO DIO 3 mask
#define GPIO_DIO_4_MASK         0x00000010  // GPIO DIO 4 mask
#define GPIO_DIO_5_MASK         0x00000020  // GPIO DIO 5 mask
#define GPIO_DIO_6_MASK         0x00000040  // GPIO DIO 6 mask
#define GPIO_DIO_7_MASK         0x00000080  // GPIO DIO 7 mask
#define GPIO_DIO_8_MASK         0x00000100  // GPIO DIO 8 mask
#define GPIO_DIO_9_MASK         0x00000200  // GPIO DIO 9 mask
#define GPIO_DIO_10_MASK        0x00000400  // GPIO DIO 10 mask
#define GPIO_DIO_11_MASK        0x00000800  // GPIO DIO 11 mask
#define GPIO_DIO_12_MASK        0x00001000  // GPIO DIO 12 mask
#define GPIO_DIO_13_MASK        0x00002000  // GPIO DIO 13 mask
#define GPIO_DIO_14_MASK        0x00004000  // GPIO DIO 14 mask
#define GPIO_DIO_15_MASK        0x00008000  // GPIO DIO 15 mask
#define GPIO_DIO_16_MASK        0x00010000  // GPIO DIO 16 mask
#define GPIO_DIO_17_MASK        0x00020000  // GPIO DIO 17 mask
#define GPIO_DIO_18_MASK        0x00040000  // GPIO DIO 18 mask
#define GPIO_DIO_19_MASK        0x00080000  // GPIO DIO 19 mask
#define GPIO_DIO_20_MASK        0x00100000  // GPIO DIO 20 mask
#define GPIO_DIO_21_MASK        0x00200000  // GPIO DIO 21 mask
#define GPIO_DIO_22_MASK        0x00400000  // GPIO DIO 22 mask
#define GPIO_DIO_23_MASK        0x00800000  // GPIO DIO 23 mask
#define GPIO_DIO_24_MASK        0x01000000  // GPIO DIO 24 mask
#define GPIO_DIO_25_MASK        0x02000000  // GPIO DIO 25 mask
#define GPIO_DIO_26_MASK        0x04000000  // GPIO DIO 26 mask
#define GPIO_DIO_27_MASK        0x08000000  // GPIO DIO 27 mask
#define GPIO_DIO_28_MASK        0x10000000  // GPIO DIO 28 mask
#define GPIO_DIO_29_MASK        0x20000000  // GPIO DIO 29 mask
#define GPIO_DIO_30_MASK        0x40000000  // GPIO DIO 30 mask
#define GPIO_DIO_31_MASK        0x80000000  // GPIO DIO 31 mask
#define GPIO_DIO_ALL_MASK       0xFFFFFFFF  // GPIO all DIOs mask

//*****************************************************************************
//
// Define constants that shall be passed as the outputEnableValue parameter to
// GPIO_setOutputEnableDio() and will be returned from the function
// GPIO_getOutputEnableDio().
//
//*****************************************************************************
#define GPIO_OUTPUT_DISABLE     0x00000000  // DIO output is disabled
#define GPIO_OUTPUT_ENABLE      0x00000001  // DIO output is enabled

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Reads a specific DIO.
//!
//! \param dioNumber specifies the DIO to read (0-31).
//!
//! \return Returns 0 or 1 reflecting the input value of the specified DIO.
//!
//! \sa \ref GPIO_readMultiDio(), \ref GPIO_writeDio(), \ref GPIO_writeMultiDio()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
GPIO_readDio( uint32_t dioNumber )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));

    // Return the input value from the specified DIO.
    return (( HWREG( GPIO_BASE + GPIO_O_DIN31_0 ) >> dioNumber ) & 1 );
}

//*****************************************************************************
//
//! \brief Reads the input value for the specified DIOs.
//!
//! This function returns the input value for multiple DIOs.
//! The value returned is not shifted and hence matches the corresponding dioMask bits.
//!
//! \param dioMask is the bit-mask representation of the DIOs to read.
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//!
//! \return Returns a bit vector reflecting the input value of the corresponding DIOs.
//! - 0 : Corresponding DIO is low.
//! - 1 : Corresponding DIO is high.
//!
//! \sa \ref GPIO_readDio(), \ref GPIO_writeDio(), \ref GPIO_writeMultiDio()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
GPIO_readMultiDio( uint32_t dioMask )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    // Return the input value from the specified DIOs.
    return ( HWREG( GPIO_BASE + GPIO_O_DIN31_0 ) & dioMask );
}

//*****************************************************************************
//
//! \brief Writes a value to a specific DIO.
//!
//! \param dioNumber specifies the DIO to update (0-31).
//! \param value specifies the value to write
//! - 0 : Logic zero (low)
//! - 1 : Logic one (high)
//!
//! \return None
//!
//! \sa \ref GPIO_writeMultiDio(), \ref GPIO_readDio(), \ref GPIO_readMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_writeDio( uint32_t dioNumber, uint32_t value )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));
    ASSERT(( value == 0 ) || ( value == 1 ));

    // Write 0 or 1 to the byte indexed DOUT map
    HWREGB( GPIO_BASE + dioNumber ) = value;
}

//*****************************************************************************
//
//! \brief Writes masked data to the specified DIOs.
//!
//! Enables for writing multiple bits simultaneously.
//! The value to write must be shifted so it matches the corresponding dioMask bits.
//!
//! \note Note that this is a read-modify-write operation and hence not atomic.
//!
//! \param dioMask is the bit-mask representation of the DIOs to write.
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//! \param bitVectoredValue holds the value to be written to the corresponding DIO-bits.
//!
//! \return None
//!
//! \sa \ref GPIO_writeDio(), \ref GPIO_readDio(), \ref GPIO_readMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_writeMultiDio( uint32_t dioMask, uint32_t bitVectoredValue )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    HWREG( GPIO_BASE + GPIO_O_DOUT31_0 ) =
        ( HWREG( GPIO_BASE + GPIO_O_DOUT31_0 ) & ~dioMask ) |
        ( bitVectoredValue & dioMask );
}

//*****************************************************************************
//
//! \brief Sets a specific DIO to 1 (high).
//!
//! \param dioNumber specifies the DIO to set (0-31).
//!
//! \return None
//!
//! \sa \ref GPIO_setMultiDio(), \ref GPIO_clearDio(), \ref GPIO_clearMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_setDio( uint32_t dioNumber )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));

    // Set the specified DIO.
    HWREG( GPIO_BASE + GPIO_O_DOUTSET31_0 ) = ( 1 << dioNumber );
}

//*****************************************************************************
//
//! \brief Sets the specified DIOs to 1 (high).
//!
//! \param dioMask is the bit-mask representation of the DIOs to set.
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//!
//! \return None
//!
//! \sa \ref GPIO_setDio(), \ref GPIO_clearDio(), \ref GPIO_clearMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_setMultiDio( uint32_t dioMask )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    // Set the DIOs.
    HWREG( GPIO_BASE + GPIO_O_DOUTSET31_0 ) = dioMask;
}

//*****************************************************************************
//
//! \brief Clears a specific DIO to 0 (low).
//!
//! \param dioNumber specifies the DIO to clear (0-31).
//!
//! \return None
//!
//! \sa \ref GPIO_clearMultiDio(), \ref GPIO_setDio(), \ref GPIO_setMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_clearDio( uint32_t dioNumber )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));

    // Clear the specified DIO.
    HWREG( GPIO_BASE + GPIO_O_DOUTCLR31_0 ) = ( 1 << dioNumber );
}

//*****************************************************************************
//
//! \brief Clears the specified DIOs to 0 (low).
//!
//! \param dioMask is the bit-mask representation of the DIOs to clear.
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//!
//! \return None
//!
//! \sa \ref GPIO_clearDio(), \ref GPIO_setDio(), \ref GPIO_setMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_clearMultiDio( uint32_t dioMask )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    // Clear the DIOs.
    HWREG( GPIO_BASE + GPIO_O_DOUTCLR31_0 ) = dioMask;
}

//*****************************************************************************
//
//! \brief Toggles a specific DIO.
//!
//! \param dioNumber specifies the DIO to toggle (0-31).
//!
//! \return None
//!
//! \sa \ref GPIO_toggleMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_toggleDio( uint32_t dioNumber )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));

    // Toggle the specified DIO.
    HWREG( GPIO_BASE + GPIO_O_DOUTTGL31_0 ) = ( 1 << dioNumber );
}

//*****************************************************************************
//
//! \brief Toggles the specified DIOs.
//!
//! \param dioMask is the bit-mask representation of the DIOs to toggle.
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//!
//! \return None
//!
//! \sa \ref GPIO_toggleDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_toggleMultiDio( uint32_t dioMask )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    // Toggle the DIOs.
    HWREG( GPIO_BASE + GPIO_O_DOUTTGL31_0 ) = dioMask;
}

//*****************************************************************************
//
//! \brief Gets the output enable status of a specific DIO.
//!
//! This function returns the output enable status for the specified DIO.
//! The DIO can be configured as either input or output under software control.
//!
//! \param dioNumber specifies the DIO to get the output enable setting from (0-31).
//!
//! \return Returns one of the enumerated data types (0 or 1):
//! - \ref GPIO_OUTPUT_DISABLE : DIO output is disabled.
//! - \ref GPIO_OUTPUT_ENABLE  : DIO output is enabled.
//!
//! \sa \ref GPIO_getOutputEnableMultiDio(), \ref GPIO_setOutputEnableDio(), \ref GPIO_setOutputEnableMultiDio()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
GPIO_getOutputEnableDio( uint32_t dioNumber )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));

    // Return the output enable status for the specified DIO.
    return (( HWREG( GPIO_BASE + GPIO_O_DOE31_0 ) >> dioNumber ) & 1 );
}

//*****************************************************************************
//
//! \brief Gets the output enable setting of the specified DIOs.
//!
//! This function returns the output enable setting for multiple DIOs.
//! The value returned is not shifted and hence matches the corresponding dioMask bits.
//!
//! \param dioMask is the bit-mask representation of the DIOs to return the output enable settings from.
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//!
//! \return Returns the output enable setting for multiple DIOs as a bit vector corresponding to the dioMask bits.
//! - 0 : Corresponding DIO is configured with output disabled.
//! - 1 : Corresponding DIO is configured with output enabled.
//!
//! \sa \ref GPIO_getOutputEnableDio(), \ref GPIO_setOutputEnableDio(), \ref GPIO_setOutputEnableMultiDio()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
GPIO_getOutputEnableMultiDio( uint32_t dioMask )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    // Return the output enable value for the specified DIOs.
    return ( HWREG( GPIO_BASE + GPIO_O_DOE31_0 ) & dioMask );
}

//*****************************************************************************
//
//! \brief Sets output enable of a specific DIO.
//!
//! This function sets the GPIO output enable bit for the specified DIO.
//! The DIO can be configured as either input or output under software control.
//!
//! \param dioNumber specifies the DIO to configure (0-31).
//! \param outputEnableValue specifies the output enable setting of the specified DIO:
//! - \ref GPIO_OUTPUT_DISABLE : DIO output is disabled.
//! - \ref GPIO_OUTPUT_ENABLE  : DIO output is enabled.
//!
//! \return None
//!
//! \sa \ref GPIO_setOutputEnableMultiDio(), \ref GPIO_getOutputEnableDio(), \ref GPIO_getOutputEnableMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_setOutputEnableDio( uint32_t dioNumber, uint32_t outputEnableValue )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));
    ASSERT(( outputEnableValue == GPIO_OUTPUT_DISABLE ) ||
           ( outputEnableValue == GPIO_OUTPUT_ENABLE  )    );

    // Update the output enable bit for the specified DIO.
    HWREGBITW( GPIO_BASE + GPIO_O_DOE31_0, dioNumber ) = outputEnableValue;
}

//*****************************************************************************
//
//! \brief Configures the output enable setting for all specified DIOs.
//!
//! This function configures the output enable setting for the specified DIOs.
//! The output enable setting must be shifted so it matches the corresponding dioMask bits.
//! The DIOs can be configured as either an input or output under software control.
//!
//! \note Note that this is a read-modify-write operation and hence not atomic.
//!
//! \param dioMask is the bit-mask representation of the DIOs on which to configure the
//! output enable setting. The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//! \param bitVectoredOutputEnable holds the output enable setting the corresponding DIO-bits:
//! - 0 : Corresponding DIO is configured with output disabled.
//! - 1 : Corresponding DIO is configured with output enabled.
//!
//! \return None
//!
//! \sa \ref GPIO_setOutputEnableDio(), \ref GPIO_getOutputEnableDio(), \ref GPIO_getOutputEnableMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_setOutputEnableMultiDio( uint32_t dioMask, uint32_t bitVectoredOutputEnable )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    HWREG( GPIO_BASE + GPIO_O_DOE31_0 ) =
        ( HWREG( GPIO_BASE + GPIO_O_DOE31_0 ) & ~dioMask ) |
        ( bitVectoredOutputEnable & dioMask );
}

//*****************************************************************************
//
//! \brief Gets the event status of a specific DIO.
//!
//! \param dioNumber specifies the DIO to get the event status from (0-31).
//!
//! \return Returns the current event status on the specified DIO.
//! - 0 : Non-triggered event.
//! - 1 : Triggered event.
//!
//! \sa \ref GPIO_getEventMultiDio(), \ref GPIO_clearEventDio(), \ref GPIO_clearEventMultiDio()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
GPIO_getEventDio( uint32_t dioNumber )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));

    // Return the event status for the specified DIO.
    return (( HWREG( GPIO_BASE + GPIO_O_EVFLAGS31_0 ) >> dioNumber ) & 1 );
}

//*****************************************************************************
//
//! \brief Gets the event status of the specified DIOs.
//!
//! This function returns the event status for multiple DIOs.
//! The value returned is not shifted and hence matches the corresponding dioMask bits.
//!
//! \param dioMask is the bit-mask representation of the DIOs to get the
//! event status from (0-31).
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//!
//! \return Returns a bit vector with the current event status corresponding to the specified DIOs.
//! - 0 : Corresponding DIO has no triggered event.
//! - 1 : Corresponding DIO has a triggered event.
//!
//! \sa \ref GPIO_getEventDio(), \ref GPIO_clearEventDio(), \ref GPIO_clearEventMultiDio()
//
//*****************************************************************************
__STATIC_INLINE uint32_t
GPIO_getEventMultiDio( uint32_t dioMask )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    // Return the event status for the specified DIO.
    return ( HWREG( GPIO_BASE + GPIO_O_EVFLAGS31_0 ) & dioMask );
}

//*****************************************************************************
//
//! \brief Clears the IO event status of a specific DIO.
//!
//! \param dioNumber specifies the DIO on which to clear the event status (0-31).
//!
//! \return None
//!
//! \sa \ref GPIO_clearEventMultiDio(), \ref GPIO_getEventDio(), \ref GPIO_getEventMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_clearEventDio( uint32_t dioNumber )
{
    // Check the arguments.
    ASSERT( dioNumberLegal( dioNumber ));

    // Clear the event status for the specified DIO.
    HWREG( GPIO_BASE + GPIO_O_EVFLAGS31_0 ) = ( 1 << dioNumber );
}

//*****************************************************************************
//
//! \brief Clears the IO event status on the specified DIOs.
//!
//! \param dioMask is the bit-mask representation of the DIOs on which to
//! clear the events status.
//! The parameter must be a bitwise OR'ed combination of the following:
//! - \ref GPIO_DIO_0_MASK
//! - ...
//! - \ref GPIO_DIO_31_MASK
//!
//! \return None
//!
//! \sa \ref GPIO_clearEventDio(), \ref GPIO_getEventDio(), \ref GPIO_getEventMultiDio()
//
//*****************************************************************************
__STATIC_INLINE void
GPIO_clearEventMultiDio( uint32_t dioMask )
{
    // Check the arguments.
    ASSERT( dioMask & GPIO_DIO_ALL_MASK );

    // Clear the event status for the specified DIOs.
    HWREG( GPIO_BASE + GPIO_O_EVFLAGS31_0 ) = dioMask;
}

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __GPIO_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
