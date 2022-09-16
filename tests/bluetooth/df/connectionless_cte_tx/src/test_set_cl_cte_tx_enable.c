/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <host/hci_core.h>

#include "common.h"
#include "test_set_cl_cte_tx_enable.h"

/* Macros delivering common values for unit tests */
#define ADV_HANDLE_INVALID              (CONFIG_BT_CTLR_ADV_AUX_SET + 1)

/* @brief Function sends HCI_LE_Set_Connectionless_CTE_Transmit_Enable
 *        to controller.
 *
 * @param[in]adv_handle                 Handle of advertising set.
 * @param[in]adv_flags                  Flags related with advertising set.
 * @param[in] enable                    Enable or disable CTE TX
 *
 * @return Zero if success, non-zero value in case of failure.
 */
int send_set_cl_cte_tx_enable(uint8_t adv_handle, atomic_t *adv_flags,
			      bool enable)
{
	struct bt_hci_cp_le_set_cl_cte_tx_enable *cp;
	struct bt_hci_cmd_state_set state;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CL_CTE_TX_ENABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->handle = adv_handle;
	cp->cte_enable = enable ? 1 : 0;

	bt_hci_cmd_state_set_init(buf, &state, adv_flags, BT_PER_ADV_CTE_ENABLED,
				  enable);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CL_CTE_TX_ENABLE,
				    buf, NULL);
}

void test_set_cl_cte_tx_enable_invalid_adv_set_handle(void)
{
	int err;

	err = send_set_cl_cte_tx_enable(ADV_HANDLE_INVALID, g_adv->flags, true);
	zassert_equal(err, -EIO, "Unexpected error value for enable CTE with "
		      "wrong advertising set handle");
}

void test_set_cl_cte_tx_enable_cte_params_not_set(void)
{
	int err;

	/* setup */
	common_create_adv_set();

	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err, -EIO, "Unexpected error value for enable CTE before "
		      "CTE params set");

	/* clean up */
	common_delete_adv_set();
}

void test_set_cl_cte_tx_enable_per_adv_coded_phy(void)
{
	int err;

	/* setup */
	g_param.options =  g_param.options | BT_LE_ADV_OPT_CODED;

	common_create_adv_set();
	common_set_cl_cte_tx_params();

	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err, -EIO, "Unexpected error value for enable CTE for "
		      "coded PHY");

	/* clean up */
	common_delete_adv_set();

	g_param.options =  g_param.options & (~BT_LE_ADV_OPT_CODED);
}

void test_set_cl_cte_tx_enable(void)
{
	int err;

	/* setup */
	common_create_adv_set();
	common_set_cl_cte_tx_params();
	common_set_adv_params();

	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err, 0, "Unexpected error value for enable CTE");

	/* clean up */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, false);
	zassert_equal(err,  0, "Unexpected error value for disable CTE ");

	common_delete_adv_set();
}

void test_set_cl_cte_tx_enable_after_per_adv_enabled(void)
{
	int err;

	/* setup */
	common_create_adv_set();
	common_set_cl_cte_tx_params();
	common_set_adv_params();
	common_per_adv_enable();

	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err,  0, "Unexpected error value for enable CTE after"
		      " per. adv. is enabled");

	/* clean up */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, false);
	zassert_equal(err,  0, "Unexpected error value for disable CTE ");

	common_per_adv_disable();
	common_delete_adv_set();
}

void test_set_cl_cte_tx_disable_when_no_CTE_enabled(void)
{
	int err;

	/* setup */
	common_create_adv_set();

	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, false);
	zassert_equal(err,  -EIO, "Unexpected error value for disable CTE "
		      "before CTE enable");

	/* clean up */
	common_delete_adv_set();
}

void test_set_cl_cte_tx_disable_before_per_adv_enable(void)
{
	int err;

	/* setup */
	common_create_adv_set();
	common_set_cl_cte_tx_params();
	common_set_adv_params();

	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err,  0, "Unexpected error value for enable");

	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, false);
	zassert_equal(err,  0, "Unexpected error value for disable CTE ");

	/* clean up */
	common_delete_adv_set();
}

void test_set_cl_cte_tx_disable_during_per_adv_enable(void)
{
	int err;

	/* setup */
	common_create_adv_set();
	common_set_cl_cte_tx_params();
	common_set_adv_params();

	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err,  0, "Unexpected error value for enable");

	common_per_adv_enable();

	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, false);
	zassert_equal(err,  0, "Unexpected error value for disable CTE ");

	/* clean up */
	common_per_adv_disable();
	common_delete_adv_set();
}

void test_set_cl_cte_tx_enable_and_update_cte_params(void)
{
	uint8_t cte_len_prev;
	int err;

	/* setup */
	common_create_adv_set();
	common_set_cl_cte_tx_params();
	common_set_adv_params();

	printk("en cte\n");
	/* test logic */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err,  0, "Unexpected error value for enable CTE after"
		      " per. adv. is enabled");

	printk("en adv\n");
	common_per_adv_enable();

	printk("disable cte\n");
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, false);
	zassert_equal(err,  0, "Unexpected error value for disable CTE ");

	printk("params to update\n");
	cte_len_prev = g_cte_len;
	g_cte_len = 0x5U;
	common_set_cl_cte_tx_params();

	printk("params updated\n");
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, true);
	zassert_equal(err,  0, "Unexpected error value for enable CTE after"
		      " per. adv. is enabled");

	printk("cleanup\n");
	/* clean up */
	err = send_set_cl_cte_tx_enable(g_adv->handle, g_adv->flags, false);
	zassert_equal(err,  0, "Unexpected error value for disable CTE ");

	g_cte_len = cte_len_prev;
	common_per_adv_disable();
	common_delete_adv_set();
}

static bool test_set_cl_cte_tx_enable_pragma(const void *state)
{
	const struct bt_test_state *s = state;

	return s->is_setup && !s->is_adv_set_created;
}

ztest_register_test_suite(test_set_cl_cte_tx_enable, test_set_cl_cte_tx_enable_pragma,
			  ztest_unit_test(test_set_cl_cte_tx_enable_invalid_adv_set_handle),
			  ztest_unit_test(test_set_cl_cte_tx_enable_cte_params_not_set),
			  ztest_unit_test(test_set_cl_cte_tx_enable_per_adv_coded_phy),
			  ztest_unit_test(test_set_cl_cte_tx_enable),
			  ztest_unit_test(test_set_cl_cte_tx_enable_after_per_adv_enabled),
			  ztest_unit_test(test_set_cl_cte_tx_disable_before_per_adv_enable),
			  ztest_unit_test(test_set_cl_cte_tx_enable_and_update_cte_params),
			  ztest_unit_test(test_set_cl_cte_tx_disable_during_per_adv_enable));
