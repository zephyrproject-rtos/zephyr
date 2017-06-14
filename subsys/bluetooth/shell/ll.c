/** @file
 * @brief Bluetooth Link Layer functions
 *
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <misc/printk.h>

#include <bluetooth/hci.h>

#include "../controller/include/ll.h"

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT)
#define ADV_INTERVAL 0x000020
#define ADV_TYPE 0x05 /* Adv. Ext. */
#define OWN_ADDR_TYPE 1
#define PEER_ADDR_TYPE 0
#define PEER_ADDR NULL
#define ADV_CHAN_MAP 0x07
#define FILTER_POLICY 0x00
#define ADV_TX_PWR NULL
#define ADV_SEC_SKIP 0
#define ADV_PHY_S 0x01
#define ADV_SID 0
#define SCAN_REQ_NOT 0

#define SCAN_INTERVAL 0x0004
#define SCAN_WINDOW 0x0004
#define SCAN_OWN_ADDR_TYPE 1
#define SCAN_FILTER_POLICY 0

int cmd_advx(int argc, char *argv[])
{
	u16_t evt_prop;
	u8_t enable;
	u8_t phy_p;
	s32_t err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			evt_prop = 0;
			enable = 1;
		} else if (!strcmp(argv[1], "off")) {
			enable = 0;
			goto disable;
		} else {
			return -EINVAL;
		}
	}

	phy_p = BIT(0);

	if (argc > 2) {
		if (!strcmp(argv[2], "coded")) {
			phy_p = BIT(2);
		} else if (!strcmp(argv[2], "anon")) {
			evt_prop |= BIT(5);
		} else if (!strcmp(argv[2], "txp")) {
			evt_prop |= BIT(6);
		} else {
			return -EINVAL;
		}
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "anon")) {
			evt_prop |= BIT(5);
		} else if (!strcmp(argv[3], "txp")) {
			evt_prop |= BIT(6);
		} else {
			return -EINVAL;
		}
	}

	if (argc > 4) {
		if (!strcmp(argv[4], "txp")) {
			evt_prop |= BIT(6);
		} else {
			return -EINVAL;
		}
	}

	printk("adv param set...");
	err = ll_adv_params_set(0x00, evt_prop, ADV_INTERVAL, ADV_TYPE,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, ADV_PHY_S, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}

disable:
	printk("adv enable (%u)...", enable);
	err = ll_adv_enable(enable);
	if (err) {
		goto exit;
	}

exit:
	printk("done (err= %d).\n", err);

	return 0;
}

int cmd_scanx(int argc, char *argv[])
{
	u8_t enable;
	u8_t type;
	s32_t err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			enable = 1;
			type = 1;
		} else if (!strcmp(argv[1], "passive")) {
			enable = 1;
			type = 0;
		} else if (!strcmp(argv[1], "off")) {
			enable = 0;
			goto disable;
		} else {
			return -EINVAL;
		}
	}

	type |= BIT(1);

	if (argc > 2) {
		if (!strcmp(argv[2], "coded")) {
			type &= BIT(0);
			type |= BIT(3);
		} else {
			return -EINVAL;
		}
	}

	printk("scan param set...");
	err = ll_scan_params_set(type, SCAN_INTERVAL, SCAN_WINDOW,
				 SCAN_OWN_ADDR_TYPE, SCAN_FILTER_POLICY);
	if (err) {
		goto exit;
	}

disable:
	printk("scan enable (%u)...", enable);
	err = ll_scan_enable(enable);
	if (err) {
		goto exit;
	}

exit:
	printk("done (err= %d).\n", err);

	return 0;
}
#endif /* CONFIG_BLUETOOTH_CONTROLLER_ADV_EXT */
