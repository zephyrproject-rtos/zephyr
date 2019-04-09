/******************************************************************************
*  Filename:       rf_data_entry.h
*  Revised:        2018-01-15 06:15:14 +0100 (Mon, 15 Jan 2018)
*  Revision:       18170
*
*  Description:    Definition of API for data exchange
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

#ifndef __DATA_ENTRY_H
#define __DATA_ENTRY_H

#ifndef __RFC_STRUCT
#define __RFC_STRUCT
#endif

#ifndef __RFC_STRUCT_ATTR
#if defined(__GNUC__)
#define __RFC_STRUCT_ATTR __attribute__ ((aligned (4)))
#elif defined(__TI_ARM__)
#define __RFC_STRUCT_ATTR __attribute__ ((__packed__,aligned (4)))
#else
#define __RFC_STRUCT_ATTR
#endif
#endif

//! \addtogroup rfc
//! @{

//! \addtogroup data_entry
//! @{

#include <stdint.h>
#include "rf_mailbox.h"

typedef struct __RFC_STRUCT rfc_dataEntry_s rfc_dataEntry_t;
typedef struct __RFC_STRUCT rfc_dataEntryGeneral_s rfc_dataEntryGeneral_t;
typedef struct __RFC_STRUCT rfc_dataEntryMulti_s rfc_dataEntryMulti_t;
typedef struct __RFC_STRUCT rfc_dataEntryPointer_s rfc_dataEntryPointer_t;
typedef struct __RFC_STRUCT rfc_dataEntryPartial_s rfc_dataEntryPartial_t;

//! \addtogroup dataEntry
//! @{
struct __RFC_STRUCT rfc_dataEntry_s {
   uint8_t* pNextEntry;                 //!<        Pointer to next entry in the queue, NULL if this is the last entry
   uint8_t status;                      //!<        Indicates status of entry, including whether it is free for the system CPU to write to
   struct {
      uint8_t type:2;                   //!< \brief Type of data entry structure<br>
                                        //!<        0: General data entry <br>
                                        //!<        1: Multi-element Rx entry<br>
                                        //!<        2: Pointer entry<br>
                                        //!<        3: Partial read Rx entry
      uint8_t lenSz:2;                  //!< \brief Size of length word in start of each Rx entry element<br>
                                        //!<        0: No length indicator<br>
                                        //!<        1: One byte length indicator<br>
                                        //!<        2: Two bytes length indicator<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t irqIntv:4;                //!< \brief For partial read Rx entry only: The number of bytes between interrupt generated
                                        //!<        by the radio CPU (0: 16 bytes)
   } config;
   uint16_t length;                     //!< \brief For pointer entries: Number of bytes in the data buffer pointed to<br>
                                        //!<        For other entries: Number of bytes following this length field
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup dataEntryGeneral
//! @{
//! General data entry structure (type = 0)

struct __RFC_STRUCT rfc_dataEntryGeneral_s {
   uint8_t* pNextEntry;                 //!<        Pointer to next entry in the queue, NULL if this is the last entry
   uint8_t status;                      //!<        Indicates status of entry, including whether it is free for the system CPU to write to
   struct {
      uint8_t type:2;                   //!< \brief Type of data entry structure<br>
                                        //!<        0: General data entry <br>
                                        //!<        1: Multi-element Rx entry<br>
                                        //!<        2: Pointer entry<br>
                                        //!<        3: Partial read Rx entry
      uint8_t lenSz:2;                  //!< \brief Size of length word in start of each Rx entry element<br>
                                        //!<        0: No length indicator<br>
                                        //!<        1: One byte length indicator<br>
                                        //!<        2: Two bytes length indicator<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t irqIntv:4;                //!< \brief For partial read Rx entry only: The number of bytes between interrupt generated
                                        //!<        by the radio CPU (0: 16 bytes)
   } config;
   uint16_t length;                     //!< \brief For pointer entries: Number of bytes in the data buffer pointed to<br>
                                        //!<        For other entries: Number of bytes following this length field
   uint8_t data;                        //!<        First byte of the data array to be received or transmitted
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup dataEntryMulti
//! @{
//! Multi-element data entry structure (type = 1)

struct __RFC_STRUCT rfc_dataEntryMulti_s {
   uint8_t* pNextEntry;                 //!<        Pointer to next entry in the queue, NULL if this is the last entry
   uint8_t status;                      //!<        Indicates status of entry, including whether it is free for the system CPU to write to
   struct {
      uint8_t type:2;                   //!< \brief Type of data entry structure<br>
                                        //!<        0: General data entry <br>
                                        //!<        1: Multi-element Rx entry<br>
                                        //!<        2: Pointer entry<br>
                                        //!<        3: Partial read Rx entry
      uint8_t lenSz:2;                  //!< \brief Size of length word in start of each Rx entry element<br>
                                        //!<        0: No length indicator<br>
                                        //!<        1: One byte length indicator<br>
                                        //!<        2: Two bytes length indicator<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t irqIntv:4;                //!< \brief For partial read Rx entry only: The number of bytes between interrupt generated
                                        //!<        by the radio CPU (0: 16 bytes)
   } config;
   uint16_t length;                     //!< \brief For pointer entries: Number of bytes in the data buffer pointed to<br>
                                        //!<        For other entries: Number of bytes following this length field
   uint16_t numElements;                //!<        Number of entry elements committed in the entry
   uint16_t nextIndex;                  //!<        Index to the byte after the last byte of the last entry element committed by the radio CPU
   uint8_t rxData;                      //!<        First byte of the data array of received data entry elements
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup dataEntryPointer
//! @{
//! Pointer data entry structure (type = 2)

struct __RFC_STRUCT rfc_dataEntryPointer_s {
   uint8_t* pNextEntry;                 //!<        Pointer to next entry in the queue, NULL if this is the last entry
   uint8_t status;                      //!<        Indicates status of entry, including whether it is free for the system CPU to write to
   struct {
      uint8_t type:2;                   //!< \brief Type of data entry structure<br>
                                        //!<        0: General data entry <br>
                                        //!<        1: Multi-element Rx entry<br>
                                        //!<        2: Pointer entry<br>
                                        //!<        3: Partial read Rx entry
      uint8_t lenSz:2;                  //!< \brief Size of length word in start of each Rx entry element<br>
                                        //!<        0: No length indicator<br>
                                        //!<        1: One byte length indicator<br>
                                        //!<        2: Two bytes length indicator<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t irqIntv:4;                //!< \brief For partial read Rx entry only: The number of bytes between interrupt generated
                                        //!<        by the radio CPU (0: 16 bytes)
   } config;
   uint16_t length;                     //!< \brief For pointer entries: Number of bytes in the data buffer pointed to<br>
                                        //!<        For other entries: Number of bytes following this length field
   uint8_t* pData;                      //!<        Pointer to data buffer of data to be received ro transmitted
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup dataEntryPartial
//! @{
//! Partial read data entry structure (type = 3)

struct __RFC_STRUCT rfc_dataEntryPartial_s {
   uint8_t* pNextEntry;                 //!<        Pointer to next entry in the queue, NULL if this is the last entry
   uint8_t status;                      //!<        Indicates status of entry, including whether it is free for the system CPU to write to
   struct {
      uint8_t type:2;                   //!< \brief Type of data entry structure<br>
                                        //!<        0: General data entry <br>
                                        //!<        1: Multi-element Rx entry<br>
                                        //!<        2: Pointer entry<br>
                                        //!<        3: Partial read Rx entry
      uint8_t lenSz:2;                  //!< \brief Size of length word in start of each Rx entry element<br>
                                        //!<        0: No length indicator<br>
                                        //!<        1: One byte length indicator<br>
                                        //!<        2: Two bytes length indicator<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t irqIntv:4;                //!< \brief For partial read Rx entry only: The number of bytes between interrupt generated
                                        //!<        by the radio CPU (0: 16 bytes)
   } config;
   uint16_t length;                     //!< \brief For pointer entries: Number of bytes in the data buffer pointed to<br>
                                        //!<        For other entries: Number of bytes following this length field
   struct {
      uint16_t numElements:13;          //!<        Number of entry elements committed in the entry
      uint16_t bEntryOpen:1;            //!<        1 if the entry contains an element that is still open for appending data
      uint16_t bFirstCont:1;            //!<        1 if the first element is a continuation of the last packet from the previous entry
      uint16_t bLastCont:1;             //!<        1 if the packet in the last element continues in the next entry
   } pktStatus;
   uint16_t nextIndex;                  //!<        Index to the byte after the last byte of the last entry element committed by the radio CPU
   uint8_t rxData;                      //!<        First byte of the data array of received data entry elements
} __RFC_STRUCT_ATTR;

//! @}

//! @}
//! @}
#endif
