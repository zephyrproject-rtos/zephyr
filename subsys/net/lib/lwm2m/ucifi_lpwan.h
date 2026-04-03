/*
 * Copyright (c) 2022 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LWM2M_UCIFI_LPWAN__
#define __LWM2M_UCIFI_LPWAN__

#define LWM2M_UCIFI_LPWAN_ID 3412
/* Mandatory resource: ID 6 - IEEE MAC address of the device (up to 64 bits) */
#define MAC_ADDRESS_SIZE 17 /* 16 hex digits, eg. "01a2b3c4d5e6f708\0" */

/* Device resource IDs */
/* clang-format off */
#define UCIFI_LPWAN_NETWORK_TYPE_RID           1
#define UCIFI_LPWAN_IPV4_ADDRESS_RID           2
#define UCIFI_LPWAN_IPV6_ADDRESS_RID           3
#define UCIFI_LPWAN_NETWORK_ADDRESS_RID        4
#define UCIFI_LPWAN_SECONDARY_ADDRESS_RID      5
#define UCIFI_LPWAN_MAC_ADDRESS_RID            6
#define UCIFI_LPWAN_PEER_ADDRESS_RID           7
#define UCIFI_LPWAN_MULTICAST_GRP_ADDRESS_RID  8
#define UCIFI_LPWAN_MULTICAST_GRP_KEY_RID      9
#define UCIFI_LPWAN_DATA_RATE_RID              10
#define UCIFI_LPWAN_TRANSMIT_POWER_RID         11
#define UCIFI_LPWAN_FREQUENCY_RID              12
#define UCIFI_LPWAN_SESSION_TIME_RID           13
#define UCIFI_LPWAN_SESSION_DURATION_RID       14
#define UCIFI_LPWAN_MESH_NODE_RID              15
#define UCIFI_LPWAN_MAX_REPEAT_TIME_RID        16
#define UCIFI_LPWAN_NUMBER_REPEATS_RID         17
#define UCIFI_LPWAN_SIGNAL_NOISE_RATIO_RID     18
#define UCIFI_LPWAN_COMM_FAILURE_RID           19
#define UCIFI_LPWAN_RSSI_RID                   20
#define UCIFI_LPWAN_IMSI_RID                   21
#define UCIFI_LPWAN_IMEI_RID                   22
#define UCIFI_LPWAN_COMM_OPERATOR_RID          23
#define UCIFI_LPWAN_IC_CARD_IDENTIFIER_RID     24
/* clang-format on */

#define UCIFI_LPWAN_MAX_RID UCIFI_LPWAN_IC_CARD_IDENTIFIER_RID

#endif /* __LWM2M_UCIFI_LPWAN__ */
