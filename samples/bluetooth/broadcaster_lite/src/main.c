/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>

#include "../../../subsys/bluetooth/controller/include/ll.h"

#define ADV_INTERVAL       0x0020
#define ADV_OWN_ADDR_TYPE  BT_HCI_OWN_ADDR_RANDOM
#define ADV_PEER_ADDR_TYPE BT_HCI_OWN_ADDR_RANDOM
#define ADV_PEER_ADDR      NULL
#define ADV_CHAN_MAP       0x07
#define ADV_FILTER_POLICY  0x00

static const uint8_t own_addr[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

static const uint8_t adv_data[] = {
	2, BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	7, BT_DATA_NAME_COMPLETE, 'Z', 'e', 'p', 'h', 'y', 'r',
};

int main(void)
{
	struct k_sem sem_rx;
	uint8_t ret;
	int err;

	printf("Initialize LL on %s...", CONFIG_BOARD_TARGET);
	err = ll_init(&sem_rx);
	if (err) {
		goto exit;
	}
	printf("success.\n");

	printf("Own address set...");
	ret = ll_addr_set(ADV_OWN_ADDR_TYPE, own_addr);
	if (ret) {
		goto exit;
	}
	printf("success.\n");

	printf("AD Data set...");
	ret = ll_adv_data_set(sizeof(adv_data), adv_data);
	if (ret) {
		goto exit;
	}
	printf("success.\n");

	printf("Non-connectable advertising, parameter set...");
	ret = ll_adv_params_set(ADV_INTERVAL, BT_HCI_ADV_NONCONN_IND,
				ADV_OWN_ADDR_TYPE, ADV_PEER_ADDR_TYPE, ADV_PEER_ADDR,
				ADV_CHAN_MAP, ADV_FILTER_POLICY);
	if (ret) {
		goto exit;
	}
	printf("success.\n");

	printf("Enabling...");
	ret = ll_adv_enable(1);
	if (ret) {
		goto exit;
	}
	printf("success.\n");

exit:
	return 0;
}
