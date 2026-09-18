/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LAYER7_APPLICATION__
#define __LAYER7_APPLICATION__

#include <stdbool.h>
#include <zephyr/knx/knx_pkt.h>

/* APCI values — encoded in pkt->buf[0:1] */
#define A_GroupValue_Read                        (0x000)
#define A_GroupValue_Response                    (0x040)
#define A_GroupValue_Write                       (0x080)
#define A_IndividualAddress_Write                (0x0C0)
#define A_IndividualAddress_Read                 (0x100)
#define A_IndividualAddress_Response             (0x140)
#define A_ADC_Read                               (0x180)
#define A_ADC_Response                           (0x1C0)
#define A_SystemNetworkParameter_Read            (0x1C8)
#define A_SystemNetworkParameter_Response        (0x1C9)
#define A_SystemNetworkParameter_Write           (0x1CA)
#define A_Memory_Read                            (0x200)
#define A_Memory_Response                        (0x240)
#define A_Memory_Write                           (0x280)
#define A_UserMemory_Read                        (0x2C0)
#define A_UserMemory_Response                    (0x2C1)
#define A_UserMemory_Write                       (0x2C2)
#define A_UserMemoryBit_Write                    (0x2C4)
#define A_UserManufacturerInfo_Read              (0x2C5)
#define A_UserManufacturerInfo_Response          (0x2C6)
#define A_FunctionPropertyCommand                (0x2C7)
#define A_FunctionPropertyState_Read             (0x2C8)
#define A_FunctionPropertyState_Response         (0x2C9)
#define A_DeviceDescriptor_Read                  (0x300)
#define A_DeviceDescriptor_Response              (0x340)
#define A_Restart                                (0x380)
#define A_Authorize_Request                      (0x3D1)
#define A_Authorize_Response                     (0x3D2)
#define A_Key_Write                              (0x3D3)
#define A_Key_Response                           (0x3D4)
#define A_PropertyValue_Read                     (0x3D5)
#define A_PropertyValue_Response                 (0x3D6)
#define A_PropertyValue_Write                    (0x3D7)
#define A_PropertyDescription_Read               (0x3D8)
#define A_PropertyDescription_Response           (0x3D9)
#define A_NetworkParameter_Read                  (0x3DA)
#define A_NetworkParameter_Response              (0x3DB)
#define A_IndividualAddressSerialNumber_Read     (0x3DC)
#define A_IndividualAddressSerialNumber_Response (0x3DD)
#define A_IndividualAddressSerialNumber_Write    (0x3DE)
#define A_DomainAddress_Write                    (0x3E0)
#define A_DomainAddress_Read                     (0x3E1)
#define A_DomainAddress_Response                 (0x3E2)
#define A_DomainAddressSelective_Read            (0x3E3)
#define A_NetworkParameter_Write                 (0x3E4)
#define A_Link_Read                              (0x3E5)
#define A_Link_Response                          (0x3E6)
#define A_Link_Write                             (0x3E7)
#define A_FileStream_InfoReport                  (0x3F0)

/*
 * All A_* functions take a single struct knx_pkt * argument.
 *
 * Field conventions:
 *   pkt->asap       ASAP (Application Service Access Point)
 *   pkt->priority   Transmission priority
 *   pkt->hop_count  Hop count (6 = standard, 7 = unlimited)
 *   pkt->ack_request ACK requested flag
 *   pkt->buf        APDU payload (encoded per KNX spec for .req/.res)
 *   pkt->lsdu_len   LSDU/APDU payload length (use this, not pkt->len which was removed)
 *   pkt->status     Transmission status for .Lcon/.Rcon/.Acon
 *   pkt->src        Source individual address (for .ind from remote)
 *   pkt->dst        Destination address (individual address for .req)
 */

/* 3.1.2 A_GroupValue_Read */
void A_GroupValue_Read__req(struct knx_pkt *pkt);
void A_GroupValue_Read__Lcon(struct knx_pkt *pkt);
/*
 * Returns true when this ASAP actually generated the A_GroupValue_Response.
 *
 * The odd one out among the __ind primitives, because 03_03_07 §3.1.1 (p.12)
 * requires that a group read be answered ONCE even when several ASAPs are
 * associated with the group address: the Application Layer informs all of
 * them, but only one generates the response.  The caller stops iterating the
 * association table on the first true — see T_Data_Group__ind().  Only L7
 * knows the R/C flags, so the decision has to be reported from here rather
 * than re-derived in L4.
 */
bool A_GroupValue_Read__ind(struct knx_pkt *pkt);
void A_GroupValue_Read__res(struct knx_pkt *pkt);
void A_GroupValue_Read__Rcon(struct knx_pkt *pkt);
void A_GroupValue_Read__Acon(struct knx_pkt *pkt);

/* 3.1.3 A_GroupValue_Write */
void A_GroupValue_Write__req(struct knx_pkt *pkt);
void A_GroupValue_Write__Lcon(struct knx_pkt *pkt);
void A_GroupValue_Write__ind(struct knx_pkt *pkt);
void A_GroupValue_Write__Rcon(struct knx_pkt *pkt);

/* 3.2.2 A_IndividualAddress_Write */
void A_IndividualAddress_Write__req(struct knx_pkt *pkt);
void A_IndividualAddress_Write__Lcon(struct knx_pkt *pkt);
void A_IndividualAddress_Write__ind(struct knx_pkt *pkt);

/* 3.2.3 A_IndividualAddress_Read */
void A_IndividualAddress_Read__req(struct knx_pkt *pkt);
void A_IndividualAddress_Read__Lcon(struct knx_pkt *pkt);
void A_IndividualAddress_Read__ind(struct knx_pkt *pkt);
void A_IndividualAddress_Read__res(struct knx_pkt *pkt);
void A_IndividualAddress_Read__Rcon(struct knx_pkt *pkt);
void A_IndividualAddress_Read__Acon(struct knx_pkt *pkt);

/* 3.2.4 A_IndividualAddressSerialNumber_Read */
void A_IndividualAddressSerialNumber_Read__req(struct knx_pkt *pkt);
void A_IndividualAddressSerialNumber_Read__Lcon(struct knx_pkt *pkt);
void A_IndividualAddressSerialNumber_Read__ind(struct knx_pkt *pkt);
void A_IndividualAddressSerialNumber_Read__res(struct knx_pkt *pkt);
void A_IndividualAddressSerialNumber_Read__Rcon(struct knx_pkt *pkt);
void A_IndividualAddressSerialNumber_Read__Acon(struct knx_pkt *pkt);

/* 3.2.5 A_IndividualAddressSerialNumber_Write */
void A_IndividualAddressSerialNumber_Write__req(struct knx_pkt *pkt);
void A_IndividualAddressSerialNumber_Write__Lcon(struct knx_pkt *pkt);
void A_IndividualAddressSerialNumber_Write__ind(struct knx_pkt *pkt);

/* 3.3.2 A_DeviceDescriptor_Read */
void A_DeviceDescriptor_Read__ind(struct knx_pkt *pkt);
void A_DeviceDescriptor_Read__res(struct knx_pkt *pkt);

/* 3.3.3 A_Restart */
void A_Restart__ind(struct knx_pkt *pkt);

/* 3.3.4 A_Authorize */
void A_Authorize_Request__ind(struct knx_pkt *pkt);

/* 3.5.8 A_Key_Write */
void A_Key_Write__ind(struct knx_pkt *pkt);

/* 3.5.2 A_ADC_Read */
void A_ADC_Read__ind(struct knx_pkt *pkt);

/* 3.4.2 A_PropertyValue_Read */
void A_PropertyValue_Read__ind(struct knx_pkt *pkt);
void A_PropertyValue_Read__res(struct knx_pkt *pkt);

/* 3.4.3 A_PropertyValue_Write */
void A_PropertyValue_Write__ind(struct knx_pkt *pkt);

/* 3.4.4 A_PropertyDescription_Read */
void A_PropertyDescription_Read__ind(struct knx_pkt *pkt);
void A_PropertyDescription_Read__res(struct knx_pkt *pkt);

/* 3.4.5 A_FunctionPropertyCommand / A_FunctionPropertyState_Read */
void A_FunctionPropertyCommand__ind(struct knx_pkt *pkt);
void A_FunctionPropertyState_Read__ind(struct knx_pkt *pkt);

/* 3.5.2 A_Memory_Read */
void A_Memory_Read__req(struct knx_pkt *pkt);
void A_Memory_Read__Lcon(struct knx_pkt *pkt);
void A_Memory_Read__ind(struct knx_pkt *pkt);
void A_Memory_Read__res(struct knx_pkt *pkt);

/* 3.5.3 A_Memory_Write */
void A_Memory_Write__req(struct knx_pkt *pkt);
void A_Memory_Write__Lcon(struct knx_pkt *pkt);
void A_Memory_Write__ind(struct knx_pkt *pkt);
void A_Memory_Write__res(struct knx_pkt *pkt);
void A_Memory_Write__Acon(struct knx_pkt *pkt);

/* 3.5.4 A_UserMemory_Write / A_UserMemory_Read (mandatory
 * for 07B0h, "DMA on User Memory", Volume 6 §4.2 row 2 p.37)
 */
void A_UserMemory_Write__ind(struct knx_pkt *pkt);
void A_UserMemory_Write__res(struct knx_pkt *pkt);
void A_UserMemory_Read__ind(struct knx_pkt *pkt);
void A_UserMemory_Read__res(struct knx_pkt *pkt);

/* A_UserMemoryBit_Write — same mandatory feature as above */
void A_UserMemoryBit_Write__ind(struct knx_pkt *pkt);
void A_UserMemoryBit_Write__res(struct knx_pkt *pkt);

/* 3.5.6 A_UserManufacturerInfo_Read */
void A_UserManufacturerInfo_Read__ind(struct knx_pkt *pkt);
void A_UserManufacturerInfo_Read__res(struct knx_pkt *pkt);

#endif
