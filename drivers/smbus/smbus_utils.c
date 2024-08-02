/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smbus_utils.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(smbus_utils, CONFIG_SMBUS_LOG_LEVEL);

void smbus_loop_alert_devices(const struct device *dev, sys_slist_t *callbacks)
{
	int result;
	uint8_t address;

	/**
	 * There might be several peripheral devices which could have triggered the alert and
	 * the one with the highest priority (lowest address) device wins the arbitration. In
	 * any case, we will have to loop through all of them.
	 *
	 * The format of the transaction is:
	 *
	 *  0                   1                   2
	 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
	 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 *  |S|  Alert Addr |R|A|   Address   |X|N|P|
	 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */
	while (true) {
		result = smbus_byte_read(dev, SMBUS_ADDRESS_ARA, &address);
		if (result != 0) {
			LOG_DBG("%s: no more peripheral devices left which triggered an alert",
				dev->name);
			return;
		}

		LOG_DBG("%s: address 0x%02X triggered an alert", dev->name, address);

		smbus_fire_callbacks(callbacks, dev, address);
	}
}
