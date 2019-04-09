/******************************************************************************
*  Filename:       chipinfo.c
*  Revised:        2018-08-17 09:28:06 +0200 (Fri, 17 Aug 2018)
*  Revision:       52354
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

#include "chipinfo.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  ChipInfo_GetSupportedProtocol_BV
    #define ChipInfo_GetSupportedProtocol_BV NOROM_ChipInfo_GetSupportedProtocol_BV
    #undef  ChipInfo_GetPackageType
    #define ChipInfo_GetPackageType         NOROM_ChipInfo_GetPackageType
    #undef  ChipInfo_GetChipType
    #define ChipInfo_GetChipType            NOROM_ChipInfo_GetChipType
    #undef  ChipInfo_GetChipFamily
    #define ChipInfo_GetChipFamily          NOROM_ChipInfo_GetChipFamily
    #undef  ChipInfo_GetHwRevision
    #define ChipInfo_GetHwRevision          NOROM_ChipInfo_GetHwRevision
    #undef  ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated
    #define ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated NOROM_ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated
#endif

//*****************************************************************************
//
// ChipInfo_GetSupportedProtocol_BV()
//
//*****************************************************************************
ProtocolBitVector_t
ChipInfo_GetSupportedProtocol_BV( void )
{
   return ((ProtocolBitVector_t)( HWREG( PRCM_BASE + 0x1D4 ) & 0x0E ));
}

//*****************************************************************************
//
// ChipInfo_GetPackageType()
//
//*****************************************************************************
PackageType_t
ChipInfo_GetPackageType( void )
{
   PackageType_t packType = (PackageType_t)((
      HWREG( FCFG1_BASE + FCFG1_O_USER_ID     ) &
                          FCFG1_USER_ID_PKG_M ) >>
                          FCFG1_USER_ID_PKG_S ) ;

   if (( packType < PACKAGE_4x4    ) ||
       ( packType > PACKAGE_7x7_Q1 )    )
   {
      packType = PACKAGE_Unknown;
   }

   return ( packType );
}

//*****************************************************************************
//
// ChipInfo_GetChipFamily()
//
//*****************************************************************************
ChipFamily_t
ChipInfo_GetChipFamily( void )
{
   uint32_t       waferId                    ;
   ChipFamily_t   chipFam = FAMILY_Unknown   ;

   waferId = (( HWREG( FCFG1_BASE + FCFG1_O_ICEPICK_DEVICE_ID ) &
                                      FCFG1_ICEPICK_DEVICE_ID_WAFER_ID_M ) >>
                                      FCFG1_ICEPICK_DEVICE_ID_WAFER_ID_S ) ;

   if ( waferId == 0xBB41 ) {
      chipFam = FAMILY_CC13x2_CC26x2 ;
   }

   return ( chipFam );
}

//*****************************************************************************
//
// ChipInfo_GetChipType()
//
//*****************************************************************************
ChipType_t
ChipInfo_GetChipType( void )
{
   ChipType_t     chipType       = CHIP_TYPE_Unknown        ;
   ChipFamily_t   chipFam        = ChipInfo_GetChipFamily() ;
   uint32_t       fcfg1UserId    = ChipInfo_GetUserId()     ;
   uint32_t       fcfg1Protocol  = (( fcfg1UserId & FCFG1_USER_ID_PROTOCOL_M ) >>
                                                    FCFG1_USER_ID_PROTOCOL_S ) ;
   uint32_t       fcfg1Cc13      = (( fcfg1UserId & FCFG1_USER_ID_CC13_M ) >>
                                                    FCFG1_USER_ID_CC13_S ) ;
   uint32_t       fcfg1Pa        = (( fcfg1UserId & FCFG1_USER_ID_PA_M ) >>
                                                    FCFG1_USER_ID_PA_S ) ;

   if ( chipFam == FAMILY_CC13x2_CC26x2 ) {
      switch ( fcfg1Protocol ) {
      case 0xF :
         if( fcfg1Cc13 ) {
            if ( fcfg1Pa ) {
               chipType = CHIP_TYPE_CC1352P  ;
            } else {
               chipType = CHIP_TYPE_CC1352   ;
            }
         } else {
            chipType = CHIP_TYPE_CC2652      ;
         }
         break;
      case 0x9 :
         if( fcfg1Pa ) {
            chipType = CHIP_TYPE_unused      ;
         } else {
            chipType = CHIP_TYPE_CC2642      ;
         }
         break;
      case 0x8 :
         chipType = CHIP_TYPE_CC1312         ;
         break;
      }
   }

   return ( chipType );
}

//*****************************************************************************
//
// ChipInfo_GetHwRevision()
//
//*****************************************************************************
HwRevision_t
ChipInfo_GetHwRevision( void )
{
   HwRevision_t   hwRev       = HWREV_Unknown                     ;
   uint32_t       fcfg1Rev    = ChipInfo_GetDeviceIdHwRevCode()   ;
   uint32_t       minorHwRev  = ChipInfo_GetMinorHwRev()          ;
   ChipFamily_t   chipFam     = ChipInfo_GetChipFamily()          ;

   if ( chipFam == FAMILY_CC13x2_CC26x2 ) {
      switch ( fcfg1Rev ) {
      case 0 : // CC13x2, CC26x2 - PG1.0
      case 1 : // CC13x2, CC26x2 - PG1.01 (will also show up as PG1.0)
         hwRev = (HwRevision_t)((uint32_t)HWREV_1_0 );
         break;
      case 2 : // CC13x2, CC26x2 - PG1.1 (or later)
         hwRev = (HwRevision_t)(((uint32_t)HWREV_1_1 ) + minorHwRev );
         break;
      case 3 : // CC13x2, CC26x2 - PG2.1 (or later)
         hwRev = (HwRevision_t)(((uint32_t)HWREV_2_1 ) + minorHwRev );
         break;
      }
   }

   return ( hwRev );
}

//*****************************************************************************
// ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated()
//*****************************************************************************
void
ThisLibraryIsFor_CC13x2_CC26x2_HwRev20AndLater_HaltIfViolated( void )
{
   if (( ! ChipInfo_ChipFamilyIs_CC13x2_CC26x2() ) ||
       ( ! ChipInfo_HwRevisionIs_GTEQ_2_0()      )    )
   {
      while(1)
      {
         // This driverlib version is for the CC13x2/CC26x2 PG2.0 and later chips.
         // Do nothing - stay here forever
      }
   }
}
