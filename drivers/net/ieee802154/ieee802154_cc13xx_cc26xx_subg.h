/*
 * Copyright (c) 2019 Brett Witherspoon
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC13XX_CC26XX_SUBG_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC13XX_CC26XX_SUBG_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_radio.h>

#include <ti/drivers/rf/RF.h>

#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_ieee_cmd.h>
#include <driverlib/rf_prop_cmd.h>
#include <driverlib/rf_mailbox.h>

#define CC13XX_CC26XX_NUM_RX_BUF \
	CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_NUM_RX_BUF

/* Three additional bytes for length, RSSI and status values from CPE */
#define CC13XX_CC26XX_RX_BUF_SIZE (IEEE802154_MAX_PHY_PACKET_SIZE + 3)

#define CC13XX_CC26XX_TX_BUF_SIZE (IEEE802154_PHY_SUN_FSK_PHR_LEN + IEEE802154_MAX_PHY_PACKET_SIZE)

#define CC13XX_CC26XX_INVALID_RSSI INT8_MIN

struct ieee802154_cc13xx_cc26xx_subg_data {
	/* protects writable data and serializes access to the API */
	struct k_sem lock;

	RF_Handle rf_handle;
	RF_Object rf_object;

	struct net_if *iface;
	uint8_t mac[8]; /* in big endian */

	bool is_up;

	dataQueue_t rx_queue;
	rfc_dataEntryPointer_t rx_entry[CC13XX_CC26XX_NUM_RX_BUF];
	uint8_t rx_data[CC13XX_CC26XX_NUM_RX_BUF][CC13XX_CC26XX_RX_BUF_SIZE];
	uint8_t tx_data[CC13XX_CC26XX_TX_BUF_SIZE];

	/* Common Radio Commands */
	volatile rfc_CMD_FS_t cmd_fs;

	/* Sub-GHz Radio Commands */
	volatile rfc_CMD_PROP_RX_ADV_t cmd_prop_rx_adv;
	volatile rfc_CMD_PROP_TX_ADV_t cmd_prop_tx_adv;
	volatile rfc_propRxOutput_t cmd_prop_rx_adv_output;
	volatile rfc_CMD_PROP_CS_t cmd_prop_cs;

	RF_CmdHandle rx_cmd_handle;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC13XX_CC26XX_SUBG_H_ */
