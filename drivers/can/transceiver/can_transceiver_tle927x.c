/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Deere & Company
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle927x_can

#include <zephyr/device.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/tle927x.h>

struct can_transceiver_tle927x_config {
	const struct device *parent;
};

static int can_transceiver_tle927x_write(const struct device *dev, uint8_t can_mode)
{
	const struct can_transceiver_tle927x_config *config = dev->config;

	return mfd_tle927x_update_reg(config->parent, TLE927X_REG_BUS_CTRL_1,
				      TLE927X_BUS_CTRL_1_CAN_MASK,
				      FIELD_PREP(TLE927X_BUS_CTRL_1_CAN_MASK, can_mode));
}

static int can_transceiver_tle927x_enable(const struct device *dev, can_mode_t mode)
{
	uint8_t can_mode = TLE927X_CAN_NORMAL;

	if ((mode & CAN_MODE_LISTENONLY) != 0U) {
		can_mode = TLE927X_CAN_RECEIVE_ONLY;
	}

	return can_transceiver_tle927x_write(dev, can_mode);
}

static int can_transceiver_tle927x_disable(const struct device *dev)
{
	return can_transceiver_tle927x_write(dev, TLE927X_CAN_OFF);
}

static DEVICE_API(can_transceiver, can_transceiver_tle927x_api) = {
	.enable = can_transceiver_tle927x_enable,
	.disable = can_transceiver_tle927x_disable,
};

static int can_transceiver_tle927x_init(const struct device *dev)
{
	const struct can_transceiver_tle927x_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	return 0;
}

#define CAN_TRANSCEIVER_TLE927X_DEFINE(inst)                                                       \
	static const struct can_transceiver_tle927x_config can_tle927x_config_##inst = {           \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, can_transceiver_tle927x_init, NULL, NULL,                      \
			      &can_tle927x_config_##inst, POST_KERNEL,                             \
			      CONFIG_CAN_TRANSCEIVER_TLE927X_INIT_PRIORITY,                        \
			      &can_transceiver_tle927x_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_TRANSCEIVER_TLE927X_DEFINE)
