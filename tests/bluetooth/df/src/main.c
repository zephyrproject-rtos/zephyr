/*
 * Copyright (c) 2020 Nordicsemiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <host/direction.h>
#include "hci_driver_mock.h"
#include "hci_mock.h"

/* @brief Storage for expected antenna information.
 *
 * Provided values are allowed by BT Core Spec. 5.1.
 */
static const struct bt_le_df_ant_info expected_ant_info = {
	.switch_sample_rates = BIT(2),
	.num_ant = 12,
	.max_switch_pattern_len = 40,
	.max_CTE_len = 20,
};

/* @brief Common setup function that registers virtual
 * test driver and enables bluetooth stack.
 */
static void setup_bt_host(void)
{
	zassert_true((hci_init_test_driver() == 0),
		     "HCI driver registration failed");

	zassert_true((bt_enable(NULL) == 0),
		     "bt_enable failed");

}

/* @brief Function is a mock of a Controller handler for
 * BT_HCI_OP_LE_READ_ANT_INF command.
 *
 * @param[in] opcode	Opcode of request command
 * @param[in] cmd	Pointer to requect command
 * @param[out] evt	Pointer to memory where to store response event
 * @param[in] rp_len	Length of response event
 */
static void le_df_read_ant_info(uint16_t opcode, struct net_buf *buf,
				struct net_buf **evt, uint8_t rp_len)
{
	struct bt_hci_rp_le_read_ant_info *rp;

	rp = hci_cmd_complete(BT_HCI_OP_LE_READ_ANT_INFO, evt, sizeof(*rp));

	rp->switch_sample_rates = expected_ant_info.switch_sample_rates;
	rp->num_ant = expected_ant_info.num_ant;
	rp->max_switch_pattern_len = expected_ant_info.max_switch_pattern_len;
	rp->max_CTE_len = expected_ant_info.max_CTE_len;
	rp->status = 0x00;
}

/* @brief Storage for Direction Finding related command handlers */
static const struct hci_cmd_handler df_handlers[] = {
	{ BT_HCI_OP_LE_READ_ANT_INFO,
	  sizeof(struct bt_hci_rp_le_read_ant_info),
	  le_df_read_ant_info },
};

/* @brief Test of BT_HCI_OP_LE_READ_ANT_INF command */
static void test_read_ant_info(void)
{
	struct bt_le_df_ant_info ant_info;

	hci_set_handlers(df_handlers, ARRAY_SIZE(df_handlers));

	zassert_true((hci_df_read_ant_info(&ant_info) == 0),
		     "Error while read DF ant. inf.");

	int rp_correct = memcmp(&ant_info, &expected_ant_info,
				 sizeof(expected_ant_info));

	zassert_true((rp_correct == 0), "Error, wrong ant. info. received");
}

void test_main(void)
{
	ztest_test_suite(read_ant_info_testsuite,
			 ztest_unit_test_setup_teardown(test_read_ant_info,
							setup_bt_host,
							unit_test_noop)
	);

	ztest_run_test_suite(read_ant_info_testsuite);
}
