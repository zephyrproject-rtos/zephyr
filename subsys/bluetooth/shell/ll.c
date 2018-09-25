/** @file
 * @brief Bluetooth Link Layer functions
 *
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include <shell/shell.h>

#include "../controller/include/ll.h"

#include "bt.h"

int cmd_ll_addr_get(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t addr_type;
	const char *str_type;
	bt_addr_t addr;
	char str_addr[BT_ADDR_STR_LEN];

	if (argc < 2) {
		return -EINVAL;
	}

	str_type = argv[1];
	if (!strcmp(str_type, "random")) {
		addr_type = 1;
	} else if (!strcmp(str_type, "public")) {
		addr_type = 0;
	} else {
		return -EINVAL;
	}

	(void)ll_addr_get(addr_type, addr.val);
	bt_addr_to_str(&addr, str_addr, sizeof(str_addr));

	print(shell, "Current %s address: %s\n", str_type, str_addr);

	return 0;
}

#if defined(CONFIG_BT_CTLR_DTM)
#include "../controller/ll_sw/ll_test.h"

int cmd_test_tx(const struct shell *shell, size_t  argc, char *argv[])
{
	u8_t chan, len, type, phy;
	u32_t err;

	if (argc < 5) {
		return -EINVAL;
	}

	chan = strtoul(argv[1], NULL, 16);
	len  = strtoul(argv[2], NULL, 16);
	type = strtoul(argv[3], NULL, 16);
	phy  = strtoul(argv[4], NULL, 16);

	err = ll_test_tx(chan, len, type, phy);
	if (err) {
		return -EINVAL;
	}

	print(shell, "test_tx...");

	return 0;
}

int cmd_test_rx(const struct shell *shell, size_t  argc, char *argv[])
{
	u8_t chan, phy, mod_idx;
	u32_t err;

	if (argc < 4) {
		return -EINVAL;
	}

	chan    = strtoul(argv[1], NULL, 16);
	phy     = strtoul(argv[2], NULL, 16);
	mod_idx = strtoul(argv[3], NULL, 16);

	err = ll_test_rx(chan, phy, mod_idx);
	if (err) {
		return -EINVAL;
	}

	print(shell, "test_rx...");

	return 0;
}

int cmd_test_end(const struct shell *shell, size_t  argc, char *argv[])
{
	u16_t num_rx;
	u32_t err;

	err = ll_test_end(&num_rx);
	if (err) {
		return -EINVAL;
	}

	print(shell, "num_rx= %u.", num_rx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_DTM */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
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

int cmd_advx(const struct shell *shell, size_t argc, char *argv[])
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

	print(shell, "adv param set...");
	err = ll_adv_params_set(0x00, evt_prop, ADV_INTERVAL, ADV_TYPE,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, ADV_PHY_S, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}

disable:
	print(shell, "adv enable (%u)...", enable);
	err = ll_adv_enable(enable);
	if (err) {
		goto exit;
	}

exit:
	print(shell, "done (err= %d).", err);

	return 0;
}

int cmd_scanx(const struct shell *shell, size_t  argc, char *argv[])
{
	u8_t type = 0;
	u8_t enable;
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

	print(shell, "scan param set...");
	err = ll_scan_params_set(type, SCAN_INTERVAL, SCAN_WINDOW,
				 SCAN_OWN_ADDR_TYPE, SCAN_FILTER_POLICY);
	if (err) {
		goto exit;
	}

disable:
	print(shell, "scan enable (%u)...", enable);
	err = ll_scan_enable(enable);
	if (err) {
		goto exit;
	}

exit:
	print(shell, "done (err= %d).", err);

	return err;
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
