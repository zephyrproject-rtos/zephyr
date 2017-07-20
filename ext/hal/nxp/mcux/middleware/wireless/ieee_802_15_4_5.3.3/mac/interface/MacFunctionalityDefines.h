/*! *********************************************************************************
* \defgroup MacFunctionality Mac Functionality Definitions
* @{
********************************************************************************** */
/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _MAC_FUNCTIONALITY_DEFINES_H_
#define _MAC_FUNCTIONALITY_DEFINES_H_

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
************************************************************************************/

/*! 2.4 GHz MAC 2006 for CM4 */
#define gMacFeatureSet_06M4_d        0
/*! 2.4 GHz MAC 2011 for CM4 */
#define gMacFeatureSet_11M4_d        1
/*! 2.4 GHz ZigBee PRO MAC for CM4 */
#define gMacFeatureSet_ZPM4_d        2
/*! 2.4 GHz Beacon Enabled MAC 2006 for CM4 */
#define gMacFeatureSet_06BEM4_d      3
/*! 2.4 GHz Beacon Enabled MAC 2006 with GTS for CM4 */
#define gMacFeatureSet_06BEGTSM4_d   4

/*! Sub-GHZ MAC 2006 for CM0 */
#define gMacFeatureSet_06gM0_d       5
/*! Sub-GHZ MAC 2011 for CM0 */
#define gMacFeatureSet_11gM0_d       6
/*! Sub-GHZ MAC 2006 with CLS and RIT for CM0 */
#define gMacFeatureSet_06eLEgM0_d    7
/*! Sub-GHZ MAC 2011 with CLS and RIT for CM0 */
#define gMacFeatureSet_11eLEgM0_d    8
/*! Sub-GHZ MAC 2006 with TSCH for CM0 */
#define gMacFeatureSet_06eTSCHgM0_d  9
/*! Sub-GHZ MAC 2011 with TSCH for CM0 */
#define gMacFeatureSet_11eTSCHgM0_d  10

/*! 2.4 GHz MAC 2006 for CM0 */
#define gMacFeatureSet_06M0_d        11
/*! 2.4 GHz MAC 2011 for CM0 */
#define gMacFeatureSet_11M0_d        12
/*! 2.4 GHz ZigBee PRO MAC for CM0 */
#define gMacFeatureSet_ZPM0_d        13
/*! 2.4 GHz Beacon Enabled MAC 2006 for CM0 */
#define gMacFeatureSet_06BEM0_d      14
/*! 2.4 GHz Beacon Enabled MAC 2006 with GTS for CM0 */
#define gMacFeatureSet_06BEGTSM0_d   15

/*! 2.4 GHz Thread MAC for CM4 */
#define gMacFeatureSet_THR_M4_d      16
/*! 2.4 GHz Thread MAC for CM0 */
#define gMacFeatureSet_THR_M0_d      17
/*! 2.4 GHz RFD Thread MAC for CM4 */
#define gMacFeatureSet_THRRFD_M4_d   18
/*! 2.4 GHz RFD Thread MAC for CM0 */
#define gMacFeatureSet_THRRFD_M0_d   19

/*! 2.4 GHz RFD MAC 2006 for CM4 */
#define gMacFeatureSet_06RFD_M4_d    20
/*! 2.4 GHz RFD MAC 2006 for CM0 */
#define gMacFeatureSet_06RFD_M0_d    21



/*! *********************************************************************************
  MAC feature  | Description
  -------------|-------------
     06        | 2006 security
     11        | 2011 security
     g         | Used with g PHY
     e         | features followed by listing (LE/TSCH/DSME)
     LE        | 4e low energy features (CSL, RIT)
     TSCH      | 4e Time Slotted Channel Hopping
     DSME      | 4e Deterministic and Synchronous Multi-channel Extension
     BE        | Beacon order !=15 (for code size reduction purposes)
     GTS       | GTS support (for code size reduction purposes)
     ZP        | ZigBee PRO customizations
     M0        | Cortex M0+
     M4        | Cortex M4
 
********************************************************************************** */
#ifndef gMacFeatureSet_d
#define gMacFeatureSet_d gMacFeatureSet_06M4_d
#endif

/*! \cond DOXY_SKIP_TAG */
#if (gMacFeatureSet_d == gMacFeatureSet_06M4_d) || (gMacFeatureSet_d == gMacFeatureSet_06M0_d)
  #define gMacInternalDataSize_c      360 /* [bytes] */
  #define gMacSecurityEnable_d        (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_11M4_d) || (gMacFeatureSet_d == gMacFeatureSet_11M0_d)
  #define gMacInternalDataSize_c      336 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gMAC2011_d                  (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_06RFD_M4_d) || (gMacFeatureSet_d == gMacFeatureSet_06RFD_M0_d)
  #define gMacInternalDataSize_c      360 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gMacUseAssociation_d        (1)
  #define gMacUseOrphanScan_d         (0)
  #define gMacUsePromiscuous_d        (0)
  #define gMacUseRxEnableRequest_d    (0)
  #define gMacUseMlmeStart_d          (0)
  #define gMacPanIdConflictDetect_d   (0)
  #define gMacCoordinatorCapability_d (0)

#elif (gMacFeatureSet_d == gMacFeatureSet_ZPM4_d) || (gMacFeatureSet_d == gMacFeatureSet_ZPM0_d)
  #define gMacInternalDataSize_c      376 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gZPRO_d                     (1)
  #define gMacUsePackedStructs_d      (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_THR_M4_d) || (gMacFeatureSet_d == gMacFeatureSet_THR_M0_d)
  #define gMacInternalDataSize_c      344 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gMacThread_d                (1)
  #define gMacUseAssociation_d        (0)
  #define gMacUseOrphanScan_d         (0)
  #define gMacUsePromiscuous_d        (0)
  #define gMacUseRxEnableRequest_d    (0)
  #define gMacPanIdConflictDetect_d   (0)

#elif (gMacFeatureSet_d == gMacFeatureSet_THRRFD_M4_d) || (gMacFeatureSet_d == gMacFeatureSet_THRRFD_M0_d)
  #define gMacInternalDataSize_c      264 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gMacThread_d                (1)
  #define gMacUseAssociation_d        (0)
  #define gMacUseOrphanScan_d         (0)
  #define gMacUsePromiscuous_d        (0)
  #define gMacUseRxEnableRequest_d    (0)
  #define gMacUseMlmeStart_d          (0)
  #define gMacPanIdConflictDetect_d   (0)
  #define gMacCoordinatorCapability_d (0)

#elif (gMacFeatureSet_d == gMacFeatureSet_06BEM4_d) || (gMacFeatureSet_d == gMacFeatureSet_06BEM0_d)
  #define gMacInternalDataSize_c      560 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gBeaconEnabledSupport_d     (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_06BEGTSM4_d) || (gMacFeatureSet_d == gMacFeatureSet_06BEGTSM0_d)
  #define gMacInternalDataSize_c      600 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gBeaconEnabledSupport_d     (1)
  #define gGtsSupport_d               (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_06gM0_d)
  #define gMacInternalDataSize_c      464 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  
#elif (gMacFeatureSet_d == gMacFeatureSet_06eLEgM0_d)
  #define gMacInternalDataSize_c      568 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gCslSupport_d               (1)
  #define gRitSupport_d               (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_06eTSCHgM0_d)
  #define gMacInternalDataSize_c      552 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gTschSupport_d              (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_11gM0_d)
  #define gMacInternalDataSize_c      448 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gMAC2011_d                  (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_11eLEgM0_d)
  #define gMacInternalDataSize_c      560 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gMAC2011_d                  (1)
  #define gCslSupport_d               (1)
  #define gRitSupport_d               (1)

#elif (gMacFeatureSet_d == gMacFeatureSet_11eTSCHgM0_d)
  #define gMacInternalDataSize_c      544 /* [bytes] */
  #define gMacSecurityEnable_d        (1)
  #define gMAC2011_d                  (1)
  #define gTschSupport_d              (1)

#else
  #error Unsupported MAC Feature Set

#endif
/*! \endcond */

/*! Beaconed Network support */
#ifndef gBeaconEnabledSupport_d
#define gBeaconEnabledSupport_d     (0)
#endif

/*! Guaranteed Time Slot support in Beaconed Networks */
#ifndef gGtsSupport_d
#define gGtsSupport_d               (0)
#endif

/*! Coordinated Sampled Listening support (sub-GHz only) */
#ifndef gCslSupport_d
#define gCslSupport_d               (0)
#endif

/*! Receiver Initiated Transmission support (sub-GHz only) */
#ifndef gRitSupport_d
#define gRitSupport_d               (0)
#endif

/*! Timeslotted Channel Hopping support (sub-GHz only) */
#ifndef gTschSupport_d
#define gTschSupport_d              (0)
#endif

/*! MAC encryption/decryption support */
#ifndef gMacSecurityEnable_d
#define gMacSecurityEnable_d        (0)
#endif

/*! Channel Page support */
#ifndef gMacUseChannelPage_d
#define gMacUseChannelPage_d        (0)
#endif

/*! PACK all data structures from MAC interface */
#ifndef gMacUsePackedStructs_d
#define gMacUsePackedStructs_d      (0)
#endif

/*! MAC Association/Disassociation support */
#ifndef gMacUseAssociation_d
#define gMacUseAssociation_d        (1)
#endif

/*! MAC Orphan Scan support */
#ifndef gMacUseOrphanScan_d
#define gMacUseOrphanScan_d         (1)
#endif

/*! MAC Rx Enable support */
#ifndef gMacUseRxEnableRequest_d
#define gMacUseRxEnableRequest_d    (1)
#endif

/*! Promiscuous mode support */
#ifndef gMacUsePromiscuous_d
#define gMacUsePromiscuous_d        (1)
#endif 

/*! PAN creation support */
#ifndef gMacUseMlmeStart_d
#define gMacUseMlmeStart_d          (1)
#endif

/*! PAN Id conflict detection support */
#ifndef gMacPanIdConflictDetect_d
#define gMacPanIdConflictDetect_d   (1)
#endif

/*! MAC Coordinator functionality */
#ifndef gMacCoordinatorCapability_d
#define gMacCoordinatorCapability_d (1)
#endif

/*! Optimize MAC for ZigBee stack */
#ifndef gZPRO_d
#define gZPRO_d                     (0)
#endif

/*! Optimize MAC for Thread stack */
#ifndef gMacThread_d
#define gMacThread_d                (0)
#endif


#endif /* _MAC_FUNCTIONALITY_DEFINES_H_ */

/*! *********************************************************************************
* @}
********************************************************************************** */
