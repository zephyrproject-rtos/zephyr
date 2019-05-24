/******************************************************************************
*  Filename:       chipinfo.h
*  Revised:        2018-06-18 10:26:12 +0200 (Mon, 18 Jun 2018)
*  Revision:       52189
*
*  Description:    Collection of functions returning chip information.
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
//! \addtogroup ChipInfo
//! @{
//
//*****************************************************************************

#ifndef __CHIP_INFO_H__
#define __CHIP_INFO_H__

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
#include <stdbool.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_fcfg1.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define ChipInfo_GetSupportedProtocol_BV NOROM_ChipInfo_GetSupportedProtocol_BV
    #define ChipInfo_GetPackageType         NOROM_ChipInfo_GetPackageType
    #define ChipInfo_GetChipType            NOROM_ChipInfo_GetChipType
    #define ChipInfo_GetChipFamily          NOROM_ChipInfo_GetChipFamily
    #define ChipInfo_GetHwRevision          NOROM_ChipInfo_GetHwRevision
    #define ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated NOROM_ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated
#endif

//*****************************************************************************
//
//! \brief Enumeration identifying the protocols supported.
//!
//! \note
//! This is a bit vector enumeration that indicates supported protocols.
//! E.g: 0x06 means that the chip supports both BLE and IEEE 802.15.4
//
//*****************************************************************************
typedef enum {
   PROTOCOL_Unknown          = 0   , //!< None of the known protocols are supported.
   PROTOCOLBIT_BLE           = 0x02, //!< Bit[1] set, indicates that Bluetooth Low Energy is supported.
   PROTOCOLBIT_IEEE_802_15_4 = 0x04, //!< Bit[2] set, indicates that IEEE 802.15.4 is supported.
   PROTOCOLBIT_Proprietary   = 0x08  //!< Bit[3] set, indicates that proprietary protocols are supported.
} ProtocolBitVector_t;

//*****************************************************************************
//
//! \brief Returns bit vector showing supported protocols.
//!
//! \return
//! Returns \ref ProtocolBitVector_t which is a bit vector indicating supported protocols.
//
//*****************************************************************************
extern ProtocolBitVector_t ChipInfo_GetSupportedProtocol_BV( void );

//*****************************************************************************
//
//! \brief Returns true if the chip supports the BLE protocol.
//!
//! \return
//! Returns \c true if supporting the BLE protocol, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_SupportsBLE( void )
{
   return (( ChipInfo_GetSupportedProtocol_BV() & PROTOCOLBIT_BLE ) != 0 );
}

//*****************************************************************************
//
//! \brief Returns true if the chip supports the IEEE 802.15.4 protocol.
//!
//! \return
//! Returns \c true if supporting the IEEE 802.15.4 protocol, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_SupportsIEEE_802_15_4( void )
{
   return (( ChipInfo_GetSupportedProtocol_BV() & PROTOCOLBIT_IEEE_802_15_4 ) != 0 );
}

//*****************************************************************************
//
//! \brief Returns true if the chip supports proprietary protocols.
//!
//! \return
//! Returns \c true if supporting proprietary protocols, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_SupportsPROPRIETARY( void )
{
   return (( ChipInfo_GetSupportedProtocol_BV() & PROTOCOLBIT_Proprietary ) != 0 );
}

//*****************************************************************************
//
//! \brief Package type enumeration
//!
//! \note
//! Packages available for a specific device are shown in the device datasheet.
//
//*****************************************************************************
typedef enum {
   PACKAGE_Unknown   = -1, //!< -1 means that current package type is unknown.
   PACKAGE_4x4       =  0, //!<  0 means that this is a 4x4 mm QFN (RHB) package.
   PACKAGE_5x5       =  1, //!<  1 means that this is a 5x5 mm QFN (RSM) package.
   PACKAGE_7x7       =  2, //!<  2 means that this is a 7x7 mm QFN (RGZ) package.
   PACKAGE_WAFER     =  3, //!<  3 means that this is a wafer sale package (naked die).
   PACKAGE_WCSP      =  4, //!<  4 means that this is a 2.7x2.7 mm WCSP (YFV).
   PACKAGE_7x7_Q1    =  5  //!<  5 means that this is a 7x7 mm QFN package with Wettable Flanks.
} PackageType_t;

//*****************************************************************************
//
//! \brief Returns package type.
//!
//! \return
//! Returns \ref PackageType_t
//
//*****************************************************************************
extern PackageType_t ChipInfo_GetPackageType( void );

//*****************************************************************************
//
//! \brief Returns true if this is a 4x4mm chip.
//!
//! \return
//! Returns \c true if this is a 4x4mm chip, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_PackageTypeIs4x4( void )
{
   return ( ChipInfo_GetPackageType() == PACKAGE_4x4 );
}

//*****************************************************************************
//
//! \brief Returns true if this is a 5x5mm chip.
//!
//! \return
//! Returns \c true if this is a 5x5mm chip, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_PackageTypeIs5x5( void )
{
   return ( ChipInfo_GetPackageType() == PACKAGE_5x5 );
}

//*****************************************************************************
//
//! \brief Returns true if this is a 7x7mm chip.
//!
//! \return
//! Returns \c true if this is a 7x7mm chip, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_PackageTypeIs7x7( void )
{
   return ( ChipInfo_GetPackageType() == PACKAGE_7x7 );
}

//*****************************************************************************
//
//! \brief Returns true if this is a wafer sale chip (naked die).
//!
//! \return
//! Returns \c true if this is a wafer sale chip, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_PackageTypeIsWAFER( void )
{
   return ( ChipInfo_GetPackageType() == PACKAGE_WAFER );
}

//*****************************************************************************
//
//! \brief Returns true if this is a WCSP chip (flip chip).
//!
//! \return
//! Returns \c true if this is a WCSP chip, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_PackageTypeIsWCSP( void )
{
   return ( ChipInfo_GetPackageType() == PACKAGE_WCSP );
}

//*****************************************************************************
//
//! \brief Returns true if this is a 7x7 Q1 chip.
//!
//! \return
//! Returns \c true if this is a 7x7 Q1 chip, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_PackageTypeIs7x7Q1( void )
{
   return ( ChipInfo_GetPackageType() == PACKAGE_7x7_Q1 );
}

//*****************************************************************************
//
//! \brief Returns the internal chip HW revision code.
//!
//! \return
//! Returns the internal chip HW revision code (in range 0-15)
//*****************************************************************************
__STATIC_INLINE uint32_t
ChipInfo_GetDeviceIdHwRevCode( void )
{
   // Returns HwRevCode = FCFG1_O_ICEPICK_DEVICE_ID[31:28]
   return ( HWREG( FCFG1_BASE + FCFG1_O_ICEPICK_DEVICE_ID ) >> 28 );
}

//*****************************************************************************
//
//! \brief Returns minor hardware revision number
//!
//! The minor revision number is set to 0 for the first market released chip
//! and thereafter incremented by 1 for each minor hardware change.
//!
//! \return
//! Returns the minor hardware revision number (in range 0-127)
//
//*****************************************************************************
__STATIC_INLINE uint32_t
ChipInfo_GetMinorHwRev( void )
{
   uint32_t minorRev = (( HWREG( FCFG1_BASE + FCFG1_O_MISC_CONF_1 ) &
                             FCFG1_MISC_CONF_1_DEVICE_MINOR_REV_M ) >>
                             FCFG1_MISC_CONF_1_DEVICE_MINOR_REV_S ) ;

   if ( minorRev >= 0x80 ) {
      minorRev = 0;
   }

   return( minorRev );
}

//*****************************************************************************
//
//! \brief Returns the 32 bits USER_ID field
//!
//! How to decode the USER_ID filed is described in the Technical Reference Manual (TRM)
//!
//! \return
//! Returns the 32 bits USER_ID field
//
//*****************************************************************************
__STATIC_INLINE uint32_t
ChipInfo_GetUserId( void )
{
   return ( HWREG( FCFG1_BASE + FCFG1_O_USER_ID ));
}

//*****************************************************************************
//
//! \brief Chip type enumeration
//
//*****************************************************************************
typedef enum {
   CHIP_TYPE_Unknown       = -1, //!< -1 means that the chip type is unknown.
   CHIP_TYPE_CC1310        =  0, //!<  0 means that this is a CC1310 chip.
   CHIP_TYPE_CC1350        =  1, //!<  1 means that this is a CC1350 chip.
   CHIP_TYPE_CC2620        =  2, //!<  2 means that this is a CC2620 chip.
   CHIP_TYPE_CC2630        =  3, //!<  3 means that this is a CC2630 chip.
   CHIP_TYPE_CC2640        =  4, //!<  4 means that this is a CC2640 chip.
   CHIP_TYPE_CC2650        =  5, //!<  5 means that this is a CC2650 chip.
   CHIP_TYPE_CUSTOM_0      =  6, //!<  6 means that this is a CUSTOM_0 chip.
   CHIP_TYPE_CUSTOM_1      =  7, //!<  7 means that this is a CUSTOM_1 chip.
   CHIP_TYPE_CC2640R2      =  8, //!<  8 means that this is a CC2640R2 chip.
   CHIP_TYPE_CC2642        =  9, //!<  9 means that this is a CC2642 chip.
   CHIP_TYPE_unused        =  10,//!< 10 unused value
   CHIP_TYPE_CC2652        =  11,//!< 11 means that this is a CC2652 chip.
   CHIP_TYPE_CC1312        =  12,//!< 12 means that this is a CC1312 chip.
   CHIP_TYPE_CC1352        =  13,//!< 13 means that this is a CC1352 chip.
   CHIP_TYPE_CC1352P       =  14 //!< 14 means that this is a CC1352P chip.
} ChipType_t;

//*****************************************************************************
//
//! \brief Returns chip type.
//!
//! \return
//! Returns \ref ChipType_t
//
//*****************************************************************************
extern ChipType_t ChipInfo_GetChipType( void );

//*****************************************************************************
//
//! \brief Chip family enumeration
//
//*****************************************************************************
typedef enum {
   FAMILY_Unknown          = -1, //!< -1 means that the chip's family member is unknown.
   FAMILY_CC26x0           =  0, //!<  0 means that the chip is a CC26x0 family member.
   FAMILY_CC13x0           =  1, //!<  1 means that the chip is a CC13x0 family member.
   FAMILY_CC26x1           =  2, //!<  2 means that the chip is a CC26x1 family member.
   FAMILY_CC26x0R2         =  3, //!<  3 means that the chip is a CC26x0R2 family (new ROM contents).
   FAMILY_CC13x2_CC26x2    =  4  //!<  4 means that the chip is a CC13x2, CC26x2 family member.
} ChipFamily_t;

//*****************************************************************************
//
//! \brief Returns chip family member.
//!
//! \return
//! Returns \ref ChipFamily_t
//
//*****************************************************************************
extern ChipFamily_t ChipInfo_GetChipFamily( void );

//*****************************************************************************
//
// Options for the define THIS_DRIVERLIB_BUILD
//
//*****************************************************************************
#define DRIVERLIB_BUILD_CC26X0        0 //!< 0 is the driverlib build ID for the cc26x0 driverlib.
#define DRIVERLIB_BUILD_CC13X0        1 //!< 1 is the driverlib build ID for the cc13x0 driverlib.
#define DRIVERLIB_BUILD_CC26X1        2 //!< 2 is the driverlib build ID for the cc26x1 driverlib.
#define DRIVERLIB_BUILD_CC26X0R2      3 //!< 3 is the driverlib build ID for the cc26x0r2 driverlib.
#define DRIVERLIB_BUILD_CC13X2_CC26X2 4 //!< 4 is the driverlib build ID for the cc13x2_cc26x2 driverlib.

//*****************************************************************************
//
//! \brief Define THIS_DRIVERLIB_BUILD, identifying current driverlib build ID.
//!
//! This driverlib build identifier can be useful for compile time checking/optimization (supporting C preprocessor expressions).
//
//*****************************************************************************
#define THIS_DRIVERLIB_BUILD   DRIVERLIB_BUILD_CC13X2_CC26X2

//*****************************************************************************
//
//! \brief Returns true if this chip is member of the CC13x0 family.
//!
//! \return
//! Returns \c true if this chip is member of the CC13x0 family, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_ChipFamilyIs_CC13x0( void )
{
   return ( ChipInfo_GetChipFamily() == FAMILY_CC13x0 );
}

//*****************************************************************************
//
//! \brief Returns true if this chip is member of the CC26x0 family.
//!
//! \return
//! Returns \c true if this chip is member of the CC26x0 family, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_ChipFamilyIs_CC26x0( void )
{
   return ( ChipInfo_GetChipFamily() == FAMILY_CC26x0 );
}

//*****************************************************************************
//
//! \brief Returns true if this chip is member of the CC26x0R2 family.
//!
//! \return
//! Returns \c true if this chip is member of the CC26x0R2 family, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_ChipFamilyIs_CC26x0R2( void )
{
   return ( ChipInfo_GetChipFamily() == FAMILY_CC26x0R2 );
}

//*****************************************************************************
//
//! \brief Returns true if this chip is member of the CC26x1 family.
//!
//! \return
//! Returns \c true if this chip is member of the CC26x1 family, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_ChipFamilyIs_CC26x1( void )
{
   return ( ChipInfo_GetChipFamily() == FAMILY_CC26x1 );
}

//*****************************************************************************
//
//! \brief Returns true if this chip is member of the CC13x2, CC26x2 family.
//!
//! \return
//! Returns \c true if this chip is member of the CC13x2, CC26x2 family, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_ChipFamilyIs_CC13x2_CC26x2( void )
{
   return ( ChipInfo_GetChipFamily() == FAMILY_CC13x2_CC26x2 );
}

//*****************************************************************************
//
//! \brief HW revision enumeration.
//
//*****************************************************************************
typedef enum {
   HWREV_Unknown     = -1, //!< -1 means that the chip's HW revision is unknown.
   HWREV_1_0         = 10, //!< 10 means that the chip's HW revision is 1.0
   HWREV_1_1         = 11, //!< 11 means that the chip's HW revision is 1.1
   HWREV_2_0         = 20, //!< 20 means that the chip's HW revision is 2.0
   HWREV_2_1         = 21, //!< 21 means that the chip's HW revision is 2.1
   HWREV_2_2         = 22, //!< 22 means that the chip's HW revision is 2.2
   HWREV_2_3         = 23, //!< 23 means that the chip's HW revision is 2.3
   HWREV_2_4         = 24  //!< 24 means that the chip's HW revision is 2.4
} HwRevision_t;

//*****************************************************************************
//
//! \brief Returns chip HW revision.
//!
//! \return
//! Returns \ref HwRevision_t
//
//*****************************************************************************
extern HwRevision_t ChipInfo_GetHwRevision( void );

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 1.0.
//!
//! \return
//! Returns \c true if HW revision for this chip is 1.0, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_1_0( void )
{
   return ( ChipInfo_GetHwRevision() == HWREV_1_0 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.0.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.0, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_2_0( void )
{
   return ( ChipInfo_GetHwRevision() == HWREV_2_0 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.0 or greater.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.0 or greater, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_GTEQ_2_0( void )
{
   return ( ChipInfo_GetHwRevision() >= HWREV_2_0 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.1.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.1, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_2_1( void )
{
   return ( ChipInfo_GetHwRevision() == HWREV_2_1 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.1 or greater.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.1 or greater, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_GTEQ_2_1( void )
{
   return ( ChipInfo_GetHwRevision() >= HWREV_2_1 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.2.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.2, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_2_2( void )
{
   return ( ChipInfo_GetHwRevision() == HWREV_2_2 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.2 or greater.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.2 or greater, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_GTEQ_2_2( void )
{
   return ( ChipInfo_GetHwRevision() >= HWREV_2_2 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.3 or greater.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.3 or greater, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_GTEQ_2_3( void )
{
   return ( ChipInfo_GetHwRevision() >= HWREV_2_3 );
}

//*****************************************************************************
//
//! \brief Returns true if HW revision for this chip is 2.4 or greater.
//!
//! \return
//! Returns \c true if HW revision for this chip is 2.4 or greater, \c false otherwise.
//
//*****************************************************************************
__STATIC_INLINE bool
ChipInfo_HwRevisionIs_GTEQ_2_4( void )
{
   return ( ChipInfo_GetHwRevision() >= HWREV_2_4 );
}

//*****************************************************************************
//
//! \brief Verifies that current chip is CC13x2 or CC26x2 PG2.0 or later and never returns if violated.
//!
//! \return None
//
//*****************************************************************************
extern void ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated( void );

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_ChipInfo_GetSupportedProtocol_BV
        #undef  ChipInfo_GetSupportedProtocol_BV
        #define ChipInfo_GetSupportedProtocol_BV ROM_ChipInfo_GetSupportedProtocol_BV
    #endif
    #ifdef ROM_ChipInfo_GetPackageType
        #undef  ChipInfo_GetPackageType
        #define ChipInfo_GetPackageType         ROM_ChipInfo_GetPackageType
    #endif
    #ifdef ROM_ChipInfo_GetChipType
        #undef  ChipInfo_GetChipType
        #define ChipInfo_GetChipType            ROM_ChipInfo_GetChipType
    #endif
    #ifdef ROM_ChipInfo_GetChipFamily
        #undef  ChipInfo_GetChipFamily
        #define ChipInfo_GetChipFamily          ROM_ChipInfo_GetChipFamily
    #endif
    #ifdef ROM_ChipInfo_GetHwRevision
        #undef  ChipInfo_GetHwRevision
        #define ChipInfo_GetHwRevision          ROM_ChipInfo_GetHwRevision
    #endif
    #ifdef ROM_ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated
        #undef  ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated
        #define ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated ROM_ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __CHIP_INFO_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
