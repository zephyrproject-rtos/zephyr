/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/types.h>
#include <string.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


#include "peripherals.h"
#include "app_bt.h"
LOG_MODULE_REGISTER(app_main, LOG_LEVEL_DBG);


void main(void)
{
	int err = 0;

	err = app_bt_init();
	if (err) {
		return;
	}

	err = peripherals_init();
	if (err) {
		return;
	}

	while (1) {
		k_sleep(MSEC_PER_SEC);

	}

}
