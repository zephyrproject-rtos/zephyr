/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smbus_utils.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

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

uint8_t smbus_pec_num_msgs(uint32_t flags, uint8_t num_msgs)
{
	__ASSERT_NO_MSG(num_msgs != 0);

	if ((flags & SMBUS_MODE_PEC) == 0) {
		return num_msgs - 1;
	}

	return num_msgs;
}

uint8_t smbus_pec(uint16_t addr, const struct i2c_msg *msgs, uint8_t num_msgs)
{
	uint8_t pec = 0;
	uint8_t prior_direction = 0;
	uint8_t addr8 = addr & BIT_MASK(7);

	for (uint8_t i = 0; i < num_msgs; i++) {
		/* When direction changes, there is a repeated start byte. */
		uint8_t start_byte;
		uint8_t direction = msgs[i].flags & I2C_MSG_RW_MASK;

		if ((i == 0) || (direction != prior_direction)) {
			prior_direction = direction;
			start_byte = (addr8 << 1) | direction;
			pec = crc8_ccitt(pec, &start_byte, sizeof(start_byte));
		}

		pec = crc8_ccitt(pec, msgs[i].buf, msgs[i].len);
	}

	return pec;
}

void smbus_write_prepare_pec(uint32_t flags, uint16_t addr, struct i2c_msg *msgs, uint8_t num_msgs)
{
	if ((flags & SMBUS_MODE_PEC) == 0) {
		return;
	}

	__ASSERT_NO_MSG(msgs != NULL);
	__ASSERT_NO_MSG(num_msgs != 0);
	__ASSERT_NO_MSG(msgs[num_msgs - 1].buf != NULL);

	msgs[num_msgs - 1].buf[0] = smbus_pec(addr, msgs, num_msgs - 1);
}

int smbus_read_check_pec(uint32_t flags, uint16_t addr, const struct i2c_msg *msgs,
			 uint8_t num_msgs)
{
	if ((flags & SMBUS_MODE_PEC) == 0) {
		return 0;
	}

	__ASSERT_NO_MSG(num_msgs != 0);
	__ASSERT_NO_MSG(msgs != NULL);
	__ASSERT_NO_MSG(msgs[num_msgs - 1].buf != NULL);

	uint8_t reported_pec = msgs[num_msgs - 1].buf[0];
	uint8_t computed_pec = smbus_pec(addr, msgs, num_msgs - 1);

	if (reported_pec != computed_pec) {
		return -EIO;
	}

	return 0;
}
