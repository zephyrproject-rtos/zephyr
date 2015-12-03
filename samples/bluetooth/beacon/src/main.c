/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

/* Set Advertisement data */
static const struct bt_eir ad[] = {
	{
		.len = 3,
		.type = BT_EIR_UUID16_ALL,
		.data = { 0xd8, 0xfe },
	},
	{
		.len = 9,
		.type = BT_EIR_SVC_DATA16,
		.data = { 0xd8, 0xfe, 0x00, 0x20, 0x02, '0', '1',
			  0x08 },
	},
	{ }
};

/* Set Scan Response data */
static const struct bt_eir sd[] = {
	{
		.len = 12,
		.type = BT_EIR_NAME_COMPLETE,
		.data = "Test beacon",
	},
	{ }
};

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	int err;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_SCAN_IND, ad, sd);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Beacon started\n");
}
