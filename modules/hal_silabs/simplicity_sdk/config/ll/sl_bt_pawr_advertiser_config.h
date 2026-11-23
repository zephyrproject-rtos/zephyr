/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the Silicon Labs Bluetooth Controller init
 * functions. It is used to configure Periodic Advertising With Response options.
 */

#ifndef SL_BT_PAWR_ADVERTISER_CONFIG_H
#define SL_BT_PAWR_ADVERTISER_CONFIG_H

#define SL_BT_CONFIG_MAX_PAWR_ADVERTISERS CONFIG_BT_SILABS_EFR32_MAX_PAWR_ADVERTISERS

#define SL_BT_CONFIG_MAX_PAWR_ADVERTISED_DATA_LENGTH_HINT                                          \
	CONFIG_BT_SILABS_EFR32_MAX_PAWR_ADVERTISED_DATA_LENGTH_HINT

#define SL_BT_CONFIG_PAWR_PACKET_REQUEST_COUNT CONFIG_BT_SILABS_EFR32_PAWR_PACKET_REQUEST_COUNT

#define SL_BT_CONFIG_PAWR_PACKET_REQUEST_ADVANCE CONFIG_BT_SILABS_EFR32_PAWR_PACKET_REQUEST_ADVANCE

#endif
