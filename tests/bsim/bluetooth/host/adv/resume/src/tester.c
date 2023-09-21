/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

#define TESTER_CENTRAL_ID    1
#define TESTER_PERIPHERAL_ID 2

void tester_central_procedure(void)
{
	bs_bt_utils_setup();
	backchannel_init(TESTER_PERIPHERAL_ID);
	printk("central tester start\n");

	/* connect to DUT as central */
	scan_connect_to_first_result();
	wait_connected();
	backchannel_sync_send();
	/* DUT is peripheral */

	/* wait until DUT connects to peripheral, and that it has disconnected
	 * from it afterwards.
	 */
	backchannel_sync_wait();

	printk("disconnect central\n");
	disconnect();

	/* DUT starts advertising again (with scanner's NRPA)
	 * should give wrong address
	 */
	scan_expect_same_address();

	/* PASS/FAIL is called in the `device_found` callback. */
}

void tester_peripheral_procedure(void)
{
	bs_bt_utils_setup();
	backchannel_init(TESTER_CENTRAL_ID);
	printk("peripheral tester start\n");

	/* wait for central to connect to DUT */
	backchannel_sync_wait();

	/* connect to DUT as peripheral */
	advertise_connectable(0, 0);
	wait_connected();
	/* DUT is central & peripheral */

	printk("disconnect peripheral\n");
	disconnect();
	/* DUT starts scanning again (using NRPA) */
	backchannel_sync_send();

	PASS("PASS\n");
}

void tester_procedure_2(void)
{
	bs_bt_utils_setup();

	printk("Tester start\n");

	scan_connect_to_first_result();
	wait_connected();

	/* Verify DUT advertiser was able to resume after the connection was
	 * established.
	 */
	scan_expect_same_address();

	WAIT_FOR_FLAG(flag_test_end);
}
