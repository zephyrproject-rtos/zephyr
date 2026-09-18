/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_KNX_LAYER7_APPLICATION_H_
#define ZEPHYR_SUBSYS_KNX_LAYER7_APPLICATION_H_

#include <stdbool.h>
#include <zephyr/knx/knx_pkt.h>

/* APCI values — encoded in pkt->buf[0:1] */
#define KNX_A_GROUP_VALUE_READ                          (0x000)
#define KNX_A_GROUP_VALUE_RESPONSE                      (0x040)
#define KNX_A_GROUP_VALUE_WRITE                         (0x080)
#define KNX_A_INDIVIDUAL_ADDRESS_WRITE                  (0x0C0)
#define KNX_A_INDIVIDUAL_ADDRESS_READ                   (0x100)
#define KNX_A_INDIVIDUAL_ADDRESS_RESPONSE               (0x140)
#define KNX_A_ADC_READ                                  (0x180)
#define KNX_A_ADC_RESPONSE                              (0x1C0)
#define KNX_A_SYSTEM_NETWORK_PARAMETER_READ             (0x1C8)
#define KNX_A_SYSTEM_NETWORK_PARAMETER_RESPONSE         (0x1C9)
#define KNX_A_SYSTEM_NETWORK_PARAMETER_WRITE            (0x1CA)
#define KNX_A_MEMORY_READ                               (0x200)
#define KNX_A_MEMORY_RESPONSE                           (0x240)
#define KNX_A_MEMORY_WRITE                              (0x280)
#define KNX_A_USER_MEMORY_READ                          (0x2C0)
#define KNX_A_USER_MEMORY_RESPONSE                      (0x2C1)
#define KNX_A_USER_MEMORY_WRITE                         (0x2C2)
#define KNX_A_USER_MEMORY_BIT_WRITE                     (0x2C4)
#define KNX_A_USER_MANUFACTURER_INFO_READ               (0x2C5)
#define KNX_A_USER_MANUFACTURER_INFO_RESPONSE           (0x2C6)
#define KNX_A_FUNCTION_PROPERTY_COMMAND                 (0x2C7)
#define KNX_A_FUNCTION_PROPERTY_STATE_READ              (0x2C8)
#define KNX_A_FUNCTION_PROPERTY_STATE_RESPONSE          (0x2C9)
#define KNX_A_DEVICE_DESCRIPTOR_READ                    (0x300)
#define KNX_A_DEVICE_DESCRIPTOR_RESPONSE                (0x340)
#define KNX_A_RESTART                                   (0x380)
#define KNX_A_AUTHORIZE_REQUEST                         (0x3D1)
#define KNX_A_AUTHORIZE_RESPONSE                        (0x3D2)
#define KNX_A_KEY_WRITE                                 (0x3D3)
#define KNX_A_KEY_RESPONSE                              (0x3D4)
#define KNX_A_PROPERTY_VALUE_READ                       (0x3D5)
#define KNX_A_PROPERTY_VALUE_RESPONSE                   (0x3D6)
#define KNX_A_PROPERTY_VALUE_WRITE                      (0x3D7)
#define KNX_A_PROPERTY_DESCRIPTION_READ                 (0x3D8)
#define KNX_A_PROPERTY_DESCRIPTION_RESPONSE             (0x3D9)
#define KNX_A_NETWORK_PARAMETER_READ                    (0x3DA)
#define KNX_A_NETWORK_PARAMETER_RESPONSE                (0x3DB)
#define KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_READ     (0x3DC)
#define KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_RESPONSE (0x3DD)
#define KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_WRITE    (0x3DE)
#define KNX_A_DOMAIN_ADDRESS_WRITE                      (0x3E0)
#define KNX_A_DOMAIN_ADDRESS_READ                       (0x3E1)
#define KNX_A_DOMAIN_ADDRESS_RESPONSE                   (0x3E2)
#define KNX_A_DOMAIN_ADDRESS_SELECTIVE_READ             (0x3E3)
#define KNX_A_NETWORK_PARAMETER_WRITE                   (0x3E4)
#define KNX_A_LINK_READ                                 (0x3E5)
#define KNX_A_LINK_RESPONSE                             (0x3E6)
#define KNX_A_LINK_WRITE                                (0x3E7)
#define KNX_A_FILE_STREAM_INFO_REPORT                   (0x3F0)

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

/* 3.1.2 KNX_A_GROUP_VALUE_READ */
void knx_a_group_value_read_req(struct knx_pkt *pkt);
void knx_a_group_value_read_lcon(struct knx_pkt *pkt);
/*
 * Returns true when this ASAP actually generated the KNX_A_GROUP_VALUE_RESPONSE.
 *
 * The odd one out among the __ind primitives, because 03_03_07 §3.1.1 (p.12)
 * requires that a group read be answered ONCE even when several ASAPs are
 * associated with the group address: the Application Layer informs all of
 * them, but only one generates the response.  The caller stops iterating the
 * association table on the first true — see knx_t_data_group_ind().  Only L7
 * knows the R/C flags, so the decision has to be reported from here rather
 * than re-derived in L4.
 */
bool knx_a_group_value_read_ind(struct knx_pkt *pkt);
void knx_a_group_value_read_res(struct knx_pkt *pkt);
void knx_a_group_value_read_rcon(struct knx_pkt *pkt);
void knx_a_group_value_read_acon(struct knx_pkt *pkt);

/* 3.1.3 KNX_A_GROUP_VALUE_WRITE */
void knx_a_group_value_write_req(struct knx_pkt *pkt);
void knx_a_group_value_write_lcon(struct knx_pkt *pkt);
void knx_a_group_value_write_ind(struct knx_pkt *pkt);
void knx_a_group_value_write_rcon(struct knx_pkt *pkt);

/* 3.2.2 KNX_A_INDIVIDUAL_ADDRESS_WRITE */
void knx_a_individual_address_write_req(struct knx_pkt *pkt);
void knx_a_individual_address_write_lcon(struct knx_pkt *pkt);
void knx_a_individual_address_write_ind(struct knx_pkt *pkt);

/* 3.2.3 KNX_A_INDIVIDUAL_ADDRESS_READ */
void knx_a_individual_address_read_req(struct knx_pkt *pkt);
void knx_a_individual_address_read_lcon(struct knx_pkt *pkt);
void knx_a_individual_address_read_ind(struct knx_pkt *pkt);
void knx_a_individual_address_read_res(struct knx_pkt *pkt);
void knx_a_individual_address_read_rcon(struct knx_pkt *pkt);
void knx_a_individual_address_read_acon(struct knx_pkt *pkt);

/* 3.2.4 KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_READ */
void knx_a_individual_address_serial_number_read_req(struct knx_pkt *pkt);
void knx_a_individual_address_serial_number_read_lcon(struct knx_pkt *pkt);
void knx_a_individual_address_serial_number_read_ind(struct knx_pkt *pkt);
void knx_a_individual_address_serial_number_read_res(struct knx_pkt *pkt);
void knx_a_individual_address_serial_number_read_rcon(struct knx_pkt *pkt);
void knx_a_individual_address_serial_number_read_acon(struct knx_pkt *pkt);

/* 3.2.5 KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_WRITE */
void knx_a_individual_address_serial_number_write_req(struct knx_pkt *pkt);
void knx_a_individual_address_serial_number_write_lcon(struct knx_pkt *pkt);
void knx_a_individual_address_serial_number_write_ind(struct knx_pkt *pkt);

/* 3.3.2 KNX_A_DEVICE_DESCRIPTOR_READ */
void knx_a_device_descriptor_read_ind(struct knx_pkt *pkt);
void knx_a_device_descriptor_read_res(struct knx_pkt *pkt);

/* 3.3.3 KNX_A_RESTART */
void knx_a_restart_ind(struct knx_pkt *pkt);

/* 3.3.4 A_Authorize */
void knx_a_authorize_request_ind(struct knx_pkt *pkt);

/* 3.5.8 KNX_A_KEY_WRITE */
void knx_a_key_write_ind(struct knx_pkt *pkt);

/* 3.5.2 KNX_A_ADC_READ */
void knx_a_adc_read_ind(struct knx_pkt *pkt);

/* 3.4.2 KNX_A_PROPERTY_VALUE_READ */
void knx_a_property_value_read_ind(struct knx_pkt *pkt);
void knx_a_property_value_read_res(struct knx_pkt *pkt);

/* 3.4.3 KNX_A_PROPERTY_VALUE_WRITE */
void knx_a_property_value_write_ind(struct knx_pkt *pkt);

/* 3.4.4 KNX_A_PROPERTY_DESCRIPTION_READ */
void knx_a_property_description_read_ind(struct knx_pkt *pkt);
void knx_a_property_description_read_res(struct knx_pkt *pkt);

/* 3.4.5 KNX_A_FUNCTION_PROPERTY_COMMAND / KNX_A_FUNCTION_PROPERTY_STATE_READ */
void knx_a_function_property_command_ind(struct knx_pkt *pkt);
void knx_a_function_property_state_read_ind(struct knx_pkt *pkt);

/* 3.5.2 KNX_A_MEMORY_READ */
void knx_a_memory_read_req(struct knx_pkt *pkt);
void knx_a_memory_read_lcon(struct knx_pkt *pkt);
void knx_a_memory_read_ind(struct knx_pkt *pkt);
void knx_a_memory_read_res(struct knx_pkt *pkt);

/* 3.5.3 KNX_A_MEMORY_WRITE */
void knx_a_memory_write_req(struct knx_pkt *pkt);
void knx_a_memory_write_lcon(struct knx_pkt *pkt);
void knx_a_memory_write_ind(struct knx_pkt *pkt);
void knx_a_memory_write_res(struct knx_pkt *pkt);
void knx_a_memory_write_acon(struct knx_pkt *pkt);

/* 3.5.4 KNX_A_USER_MEMORY_WRITE / KNX_A_USER_MEMORY_READ (mandatory
 * for 07B0h, "DMA on User Memory", Volume 6 §4.2 row 2 p.37)
 */
void knx_a_user_memory_write_ind(struct knx_pkt *pkt);
void knx_a_user_memory_write_res(struct knx_pkt *pkt);
void knx_a_user_memory_read_ind(struct knx_pkt *pkt);
void knx_a_user_memory_read_res(struct knx_pkt *pkt);

/* KNX_A_USER_MEMORY_BIT_WRITE — same mandatory feature as above */
void knx_a_user_memory_bit_write_ind(struct knx_pkt *pkt);
void knx_a_user_memory_bit_write_res(struct knx_pkt *pkt);

/* 3.5.6 KNX_A_USER_MANUFACTURER_INFO_READ */
void knx_a_user_manufacturer_info_read_ind(struct knx_pkt *pkt);
void knx_a_user_manufacturer_info_read_res(struct knx_pkt *pkt);

#endif /* ZEPHYR_SUBSYS_KNX_LAYER7_APPLICATION_H_ */
