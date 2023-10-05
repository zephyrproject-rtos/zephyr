/*
 * Copyright (c) 2021 Safran Passenger Innovations Germany GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_semailbox

 #include <zephyr/drivers/entropy.h>
 #include <soc.h>
 #include "em_cmu.h"
 #include "sl_se_manager.h"
 #include "sl_se_manager_entropy.h"


static int entropy_gecko_se_get_entropy(const struct device *dev,
					uint8_t *buffer,
					uint16_t length)
{
	ARG_UNUSED(dev);

	int err = 0;
	sl_status_t status;
	sl_se_command_context_t cmd_ctx;

	status = sl_se_init_command_context(&cmd_ctx);

	if (status == SL_STATUS_OK) {
		status = sl_se_get_random(&cmd_ctx, buffer, length);

		if (status != SL_STATUS_OK) {
			err = -EIO;
		}

		sl_se_deinit_command_context(&cmd_ctx);
	} else {
		err = -EIO;
	}

	return err;
}

static int entropy_gecko_se_init(const struct device *dev)
{
	if (sl_se_init()) {
		return -EIO;
	}

	return 0;
}

static const struct entropy_driver_api entropy_gecko_se_api_funcs = {
	.get_entropy = entropy_gecko_se_get_entropy,
};

#define GECKO_SE_INIT(n) \
	DEVICE_DT_INST_DEFINE(n, \
			      entropy_gecko_se_init, NULL, \
			      NULL, NULL, \
			      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, \
			      &entropy_gecko_se_api_funcs); \

DT_INST_FOREACH_STATUS_OKAY(GECKO_SE_INIT)
