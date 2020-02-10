/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "ll.h"

#define HANDLE          0x0000
#define EVT_PROP_ANON   BIT(5)
#define EVT_PROP_TXP    BIT(6)
#define ADV_INTERVAL    0x20
#define OWN_ADDR_TYPE   1
#define PEER_ADDR_TYPE  0
#define PEER_ADDR       NULL
#define ADV_CHAN_MAP    0x07
#define FILTER_POLICY   0x00
#define ADV_TX_PWR      NULL
#define ADV_SEC_SKIP    0
#define ADV_PHY_1M      BIT(0)
#define ADV_PHY_CODED   BIT(2)
#define ADV_SID         0
#define SCAN_REQ_NOT    0

#define AD_OP           0x03
#define AD_FRAG_PREF    0x00

#define ADV_INTERVAL_PERIODIC 0x30

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	};

static u8_t adv_data[] = {
		2, BT_DATA_FLAGS, BT_LE_AD_NO_BREDR,
	};

static u8_t adv_data1[] = {
		2, BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
		7, BT_DATA_NAME_COMPLETE, 'Z', 'e', 'p', 'h', 'y', 'r',
	};

static u8_t adv_data2[] = {
		2, BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	};

void main(void)
{
	u16_t evt_prop;
	u8_t adv_type;
	u16_t handle;
	u8_t phy_p;
	u8_t phy_s;
	int err;

	printk("\n*Extended Advertising Demo*\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Starting non-connectable advertising...");
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");


	k_sleep(K_MSEC(400));

	printk("Stopping advertising...");
	err = bt_le_adv_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Starting non-conn non-scan without aux 1M advertising...");
	handle = 0x0000;
	evt_prop = EVT_PROP_TXP;
	adv_type = 0x05; /* Adv. Ext. */
	phy_p = ADV_PHY_1M;
	phy_s = ADV_PHY_1M;
	err = ll_adv_params_set(handle, evt_prop, ADV_INTERVAL, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, phy_s, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}

	printk("enabling...");
	err = ll_adv_enable(handle, 1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Starting non-conn non-scan with aux 1M advertising...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data), (void *)adv_data);
	if (err) {
		goto exit;
	}

	printk("enabling...");
	err = ll_adv_enable(handle, 1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 1...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data1), (void *)adv_data1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 2...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data2), (void *)adv_data2);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Starting periodic 1M advertising...");
	handle = 0x0000;
	err = ll_adv_sync_param_set(handle, ADV_INTERVAL_PERIODIC, 0);
	if (err) {
		goto exit;
	}

	printk("enabling periodic...");
	err = ll_adv_sync_enable(handle, 1);
	if (err) {
		goto exit;
	}

	printk("enabling extended...");
	err = ll_adv_enable(handle, 1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 1...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data1), (void *)adv_data1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 2...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data2), (void *)adv_data2);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");
	return;

exit:
	printk("failed (%d)\n", err);
}
