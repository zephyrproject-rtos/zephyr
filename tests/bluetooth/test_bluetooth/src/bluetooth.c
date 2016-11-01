/* bluetooth.c - Bluetooth smoke test */

/*
 * Copyright (c) 2015-2016 Intel Corporation.
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

#include <zephyr.h>

#include <errno.h>
#include <tc_util.h>

#include <bluetooth/bluetooth.h>
#include <drivers/bluetooth/hci_driver.h>

#define EXPECTED_ERROR -ENOSYS

static int driver_open(void)
{
	TC_PRINT("driver: %s\n", __func__);

	/* Indicate that there is no real Bluetooth device */
	return EXPECTED_ERROR;
}

static int driver_send(struct net_buf *buf)
{
	return 0;
}

static struct bt_hci_driver drv = {
	.name         = "test",
	.bus          = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open         = driver_open,
	.send         = driver_send,
};

static void driver_init(void)
{
	bt_hci_driver_register(&drv);
}

void main(void)
{
	int ret, ret_code;

	driver_init();

	ret = bt_enable(NULL);
	if (ret == EXPECTED_ERROR) {
		ret_code = TC_PASS;
	} else {
		ret_code = TC_FAIL;
	}

	TC_END(ret_code, "%s - %s.\n", ret_code == TC_PASS ? PASS : FAIL,
		   __func__);

	TC_END_REPORT(ret_code);
}
