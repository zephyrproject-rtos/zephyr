/*
 * Copyright (c) 2022 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_OBJ_LPWAN_H
#define LWM2M_OBJ_LPWAN_H

#define LWM2M_OBJECT_LPWAN_ID 3412
/* Mandatory resource: ID 6 - IEEE MAC address of the device (up to 64 bits) */
#define MAC_ADDRESS_LEN	      17 /* "xxxxxxxxxxxxxxxx" */

/* Device resource IDs */
#define LPWAN_NETWORK_TYPE_ID	       1
#define LPWAN_IPV4_ADDRESS_ID	       2
#define LPWAN_IPV6_ADDRESS_ID	       3
#define LPWAN_NETWORK_ADDRESS_ID       4
#define LPWAN_SECONDARY_ADDRESS_ID     5
#define LPWAN_MAC_ADDRESS_ID	       6
#define LPWAN_PEER_ADDRESS_ID	       7
#define LPWAN_MULTICAST_GRP_ADDRESS_ID 8
#define LPWAN_MULTICAST_GRP_KEY_ID     9
#define LPWAN_DATA_RATE_ID	       10
#define LPWAN_TRANSMIT_POWER_ID	       11
#define LPWAN_FREQUENCY_ID	       12
#define LPWAN_SESSION_TIME_ID	       13
#define LPWAN_SESSION_DURATION_ID      14
#define LPWAN_MESH_NODE_ID	       15
#define LPWAN_MAX_REPEAT_TIME_ID       16
#define LPWAN_NUMBER_REPEATS_ID	       17
#define LPWAN_SIGNAL_NOISE_RATIO_ID    18
#define LPWAN_COMM_FAILURE_ID	       19
#define LPWAN_RSSI_ID		       20
#define LPWAN_IMSI_ID		       21
#define LPWAN_IMEI_ID		       22
#define LPWAN_CURRENT_COMM_OPERATOR_ID 23
#define LPWAN_IC_CARD_IDENTIFIER_ID    24

#define LPWAN_MAX_ID LPWAN_IC_CARD_IDENTIFIER_ID

#endif /* LWM2M_OBJ_LPWAN_H */
