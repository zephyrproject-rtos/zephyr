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
#include <zephyr.h>
#include <shell/shell.h>
#include <misc/printk.h>

#include <bluetooth/hci.h>

#include "../controller/util/memq.h"
#include "../controller/include/ll.h"

#if defined(CONFIG_BT_CTLR_DTM)
#include "../controller/ll_sw/ll_test.h"

int cmd_test_tx(int argc, char *argv[])
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

	printk("test_tx...\n");

	return 0;
}

int cmd_test_rx(int argc, char *argv[])
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

	printk("test_rx...\n");

	return 0;
}

int cmd_test_end(int argc, char *argv[])
{
	u16_t num_rx;
	u32_t err;

	err = ll_test_end(&num_rx);
	if (err) {
		return -EINVAL;
	}

	printk("num_rx= %u.\n", num_rx);

	return 0;
}
#endif /* CONFIG_BT_CTLR_DTM */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#include "../controller/ll_sw/ull_adv_aux.h"

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
#define AD_LEN 0x00
#define AD_DATA NULL

#define SCAN_INTERVAL 0x0004
#define SCAN_WINDOW 0x0004
#define SCAN_OWN_ADDR_TYPE 1
#define SCAN_FILTER_POLICY 0

#if defined(CONFIG_BT_BROADCASTER)
int cmd_advx(int argc, char *argv[])
{
	u16_t adv_interval = 0x20;
	u16_t evt_prop;
	u8_t adv_type;
	u8_t enable;
	u8_t phy_p;
	s32_t err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			evt_prop = 0;
			adv_type = 0x05; /* Adv. Ext. */
			enable = 1;
		} else if (!strcmp(argv[1], "hdcd")) {
			evt_prop = 0;
			adv_type = 0x01; /* Directed */
			adv_interval = 0; /* High Duty Cycle */
			enable = 1;
		} else if (!strcmp(argv[1], "ldcd")) {
			evt_prop = 0;
			adv_type = 0x01; /* Directed */
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
		} else if (!strcmp(argv[2], "ad")) {
		} else {
			return -EINVAL;
		}
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "anon")) {
			evt_prop |= BIT(5);
		} else if (!strcmp(argv[3], "txp")) {
			evt_prop |= BIT(6);
		} else if (!strcmp(argv[3], "ad")) {
		} else {
			return -EINVAL;
		}
	}

	if (argc > 4) {
		if (!strcmp(argv[4], "txp")) {
			evt_prop |= BIT(6);
		} else if (!strcmp(argv[4], "ad")) {
		} else {
			return -EINVAL;
		}
	}

	if (argc > 5) {
		if (!strcmp(argv[5], "ad")) {
		} else {
			return -EINVAL;
		}
	}

	printk("adv param set...");
	err = ll_adv_params_set(0x01, evt_prop, adv_interval, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, ADV_PHY_S, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}

	printk("ad data set...");
	err = ll_adv_aux_ad_data_set(0x01, AD_OP, AD_FRAG_PREF, AD_LEN,
				     AD_DATA);
	if (err) {
		goto exit;
	}

disable:
	printk("adv enable (%u)...", enable);
#if defined(CONFIG_BT_HCI_MESH_EXT)
	err = ll_adv_enable(0x01, enable, 0, 0, 0, 0, 0);
#else /* !CONFIG_BT_HCI_MESH_EXT */
	err = ll_adv_enable(0x01, enable);
#endif /* !CONFIG_BT_HCI_MESH_EXT */
	if (err) {
		goto exit;
	}

exit:
	printk("done (err= %d).\n", err);

	return 0;
}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
int cmd_scanx(int argc, char *argv[])
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
#endif /* CONFIG_BT_OBSERVER */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_LL_SW_SPLIT)
#include "../controller/ll_sw/ull.h"

int cmd_ull_reset(int argc, char *argv[])
{
	ll_reset();

	return 0;
}

#if defined(CONFIG_BT_TMP)
#include "../controller/ll_sw/ull_tmp.h"

int cmd_ull_tmp_enable(int argc, char *argv[])
{
	u16_t handle = 0;
	int enable;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "on")) {
			enable = 1;
		} else if (!strcmp(argv[1], "off")) {
			enable = 0;
		} else {
			return -EINVAL;
		}
	}

	if (argc > 2) {
		handle = strtoul(argv[2], NULL, 16);
	}

	if (enable) {
		err = ull_tmp_enable(handle);
	} else {
		err = ull_tmp_disable(handle);
	}

	printk("Done (%d).\n", err);

	return err;
}

int cmd_ull_tmp_send(int argc, char *argv[])
{
	u16_t handle = 0;
	int err;

	if (argc > 1) {
		handle = strtoul(argv[1], NULL, 16);
	}

	err = ull_tmp_data_send(handle, 0, NULL);

	printk("Done (%d).\n", err);

	return err;
}
#endif /* CONFIG_BT_TMP */
#endif /* CONFIG_BT_LL_SW_SPLIT */
