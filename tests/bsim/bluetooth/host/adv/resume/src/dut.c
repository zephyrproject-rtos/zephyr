/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "zephyr/bluetooth/addr.h"
#include "zephyr/bluetooth/bluetooth.h"
#include "zephyr/bluetooth/conn.h"
#include "zephyr/toolchain/gcc.h"

#include <stdint.h>
#include <string.h>

void dut_procedure(void)
{
	bs_bt_utils_setup();

	printk("DUT start\n");

	/* start scanning (using NRPA) */
	scan_connect_to_first_result();

	advertise_connectable(0, 0);
	wait_connected();
	printk("DUT is peripheral\n");

	/* tester advertises using a new identity
	 * -> will get detected and connected to by DUT
	 */
	wait_connected();
	printk("DUT is central & peripheral\n");

	/* restart advertiser: it will fail because we have run out of contexts.
	 * But since we pass the `persist` flag, it will start up as soon as a
	 * peripheral role is disconnected.
	 *
	 * We can't start it with the `persist` flag the first time, because adv
	 * will resume straight after the peripheral's connection completes,
	 * 'stealing' the last conn context and preventing the scanner from
	 * establishing a connection.
	 */
	advertise_connectable(0, 1);

	wait_disconnected();
	printk("DUT is central\n");
	scan_connect_to_first_result();

	wait_disconnected();
	printk("DUT has no connections\n");

	PASS("PASS\n");
}

void dut_procedure_2(void)
{
	bs_bt_utils_setup();

	printk("DUT start\n");

	/* Start a resumable advertiser. */
	advertise_connectable(0, true);

	PASS("DUT done\n");
}
