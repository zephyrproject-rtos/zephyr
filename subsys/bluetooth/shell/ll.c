/** @file
 * @brief Bluetooth Link Layer functions
 *
 */

/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/shell/shell.h>

#include "../controller/util/memq.h"
#include "../controller/include/ll.h"

#include "bt.h"

int cmd_ll_addr_read(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t addr_type;
	const char *str_type;
	bt_addr_t addr;
	char str_addr[BT_ADDR_STR_LEN];

	if (argc < 2) {
		return -EINVAL;
	}

	str_type = argv[1];
	if (!strcmp(str_type, "random")) {
		addr_type = 1U;
	} else if (!strcmp(str_type, "public")) {
		addr_type = 0U;
	} else {
		return -EINVAL;
	}

	(void)ll_addr_read(addr_type, addr.val);
	bt_addr_to_str(&addr, str_addr, sizeof(str_addr));

	shell_print(sh, "Current %s address: %s", str_type, str_addr);

	return 0;
}

#if defined(CONFIG_BT_CTLR_DTM)
#include "../controller/ll_sw/ll_test.h"

int cmd_test_tx(const struct shell *sh, size_t  argc, char *argv[])
{
	uint8_t chan, len, type, phy;
	uint8_t err;

	if (argc < 5) {
		return -EINVAL;
	}

	chan = strtoul(argv[1], NULL, 16);
	len  = strtoul(argv[2], NULL, 16);
	type = strtoul(argv[3], NULL, 16);
	phy  = strtoul(argv[4], NULL, 16);

	err = ll_test_tx(chan, len, type, phy, BT_HCI_LE_TEST_CTE_DISABLED,
			 BT_HCI_LE_TEST_CTE_TYPE_ANY, BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY,
			 NULL, BT_HCI_TX_TEST_POWER_MAX_SET);
	if (err) {
		return -EINVAL;
	}

	shell_print(sh, "test_tx...");

	return 0;
}

int cmd_test_rx(const struct shell *sh, size_t  argc, char *argv[])
{
	uint8_t chan, phy, mod_idx;
	uint8_t err;

	if (argc < 4) {
		return -EINVAL;
	}

	chan    = strtoul(argv[1], NULL, 16);
	phy     = strtoul(argv[2], NULL, 16);
	mod_idx = strtoul(argv[3], NULL, 16);

	err = ll_test_rx(chan, phy, mod_idx, BT_HCI_LE_TEST_CTE_DISABLED,
			 BT_HCI_LE_TEST_CTE_TYPE_ANY, BT_HCI_LE_TEST_SLOT_DURATION_ANY,
			 BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY, NULL);
	if (err) {
		return -EINVAL;
	}

	shell_print(sh, "test_rx...");

	return 0;
}

int cmd_test_end(const struct shell *sh, size_t  argc, char *argv[])
{
	uint16_t num_rx;
	uint8_t err;

	err = ll_test_end(&num_rx);
	if (err) {
		return -EINVAL;
	}

	shell_print(sh, "num_rx= %u.", num_rx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_DTM */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#include "../controller/ll_sw/lll.h"

#if defined(CONFIG_BT_BROADCASTER)
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

#define AD_OP 0x03
#define AD_FRAG_PREF 0x00

static const struct bt_data adv_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	};

int cmd_advx(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t adv_interval = 0x20;
	uint16_t handle = 0U;
	uint16_t evt_prop = 0U;
	uint8_t adv_type;
	uint8_t enable;
	uint8_t ad = 0;
	uint8_t phy_p;
	int32_t err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			adv_type = 0x05; /* Adv. Ext. */
			enable = 1U;
		} else if (!strcmp(argv[1], "hdcd")) {
			adv_type = 0x01; /* Directed */
			adv_interval = 0U; /* High Duty Cycle */
			phy_p = BIT(0);
			enable = 1U;
			goto do_enable;
		} else if (!strcmp(argv[1], "ldcd")) {
			adv_type = 0x04; /* Directed */
			enable = 1U;
		} else if (!strcmp(argv[1], "off")) {
			enable = 0U;
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
		} else if (!strcmp(argv[2], "ad")) {
			ad = 1;
		} else {
			handle = strtoul(argv[2], NULL, 16);
			if (handle >= CONFIG_BT_CTLR_ADV_SET) {
				return -EINVAL;
			}
		}
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "anon")) {
			evt_prop |= BIT(5);
		} else if (!strcmp(argv[3], "txp")) {
			evt_prop |= BIT(6);
		} else if (!strcmp(argv[3], "ad")) {
			ad = 1;
		} else {
			handle = strtoul(argv[3], NULL, 16);
			if (handle >= CONFIG_BT_CTLR_ADV_SET) {
				return -EINVAL;
			}
		}
	}

	if (argc > 4) {
		if (!strcmp(argv[4], "txp")) {
			evt_prop |= BIT(6);
		} else if (!strcmp(argv[4], "ad")) {
			ad = 1;
		} else {
			handle = strtoul(argv[4], NULL, 16);
			if (handle >= CONFIG_BT_CTLR_ADV_SET) {
				return -EINVAL;
			}
		}
	}

	if (argc > 5) {
		if (!strcmp(argv[5], "ad")) {
			ad = 1;
		} else {
			handle = strtoul(argv[5], NULL, 16);
			if (handle >= CONFIG_BT_CTLR_ADV_SET) {
				return -EINVAL;
			}
		}
	}

	if (argc > 6) {
		handle = strtoul(argv[6], NULL, 16);
		if (handle >= CONFIG_BT_CTLR_ADV_SET) {
			return -EINVAL;
		}
	}

	if (!enable) {
		goto disable;
	}

do_enable:
	shell_print(sh, "adv param set...");
	err = ll_adv_params_set(handle, evt_prop, adv_interval, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, ADV_PHY_S, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}

	if (ad) {
		shell_print(sh, "ad data set...");
		err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
					     ARRAY_SIZE(adv_data),
					     (void *)adv_data);
		if (err) {
			goto exit;
		}
	}

disable:
	shell_print(sh, "adv enable (%u)...", enable);
#if defined(CONFIG_BT_HCI_MESH_EXT)
	err = ll_adv_enable(handle, enable, 0, 0, 0, 0, 0);
#else /* !CONFIG_BT_HCI_MESH_EXT */
	err = ll_adv_enable(handle, enable, 0, 0);
#endif /* !CONFIG_BT_HCI_MESH_EXT */
	if (err) {
		goto exit;
	}

	if (!enable) {
		err = ll_adv_aux_set_remove(handle);
		if (err) {
			goto exit;
		}
	}

exit:
	shell_print(sh, "done (err= %d).", err);

	return 0;
}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
#define SCAN_INTERVAL 0x0004
#define SCAN_WINDOW 0x0004
#define SCAN_OWN_ADDR_TYPE 1
#define SCAN_FILTER_POLICY 0

int cmd_scanx(const struct shell *sh, size_t  argc, char *argv[])
{
	uint8_t type = 0U;
	uint8_t enable;
	int32_t err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			enable = 1U;
			type = 1U;
		} else if (!strcmp(argv[1], "passive")) {
			enable = 1U;
			type = 0U;
		} else if (!strcmp(argv[1], "off")) {
			enable = 0U;
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

	shell_print(sh, "scan param set...");
	err = ll_scan_params_set(type, SCAN_INTERVAL, SCAN_WINDOW,
				 SCAN_OWN_ADDR_TYPE, SCAN_FILTER_POLICY);
	if (err) {
		goto exit;
	}

disable:
	shell_print(sh, "scan enable (%u)...", enable);
	err = ll_scan_enable(enable, 0, 0);
	if (err) {
		goto exit;
	}

exit:
	shell_print(sh, "done (err= %d).", err);

	return err;
}
#endif /* CONFIG_BT_OBSERVER */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
