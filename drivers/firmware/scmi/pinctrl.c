/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/pinctrl.h>
#include <zephyr/kernel.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, arm_scmi_pinctrl), NULL,
		SCMI_PIN_CONTROL_PROTOCOL_SUPPORTED_VERSION);

enum scmi_pinctrl_message {
	PINCTRL_ATTRIBUTES = 0x3,
	PINCTRL_LIST_ASSOCIATIONS = 0x4,
	PINCTRL_SETTINGS_GET = 0x5,
	PINCTRL_SETTINGS_CONFIGURE = 0x6,
	PINCTRL_REQUEST = 0x7,
	PINCTRL_RELEASE = 0x8,
	PINCTRL_NAME_GET = 0x9,
	PINCTRL_SET_PERMISSIONS = 0xa,
};

int scmi_pinctrl_settings_configure(struct scmi_pinctrl_settings *settings)
{
	struct scmi_protocol *proto;
	struct scmi_xfer xfer;
	uint32_t config_num;
	int32_t ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_PINCTRL);

	/* input validation */
	if (!settings) {
		return -EINVAL;
	}

	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_PINCTRL) {
		return -EINVAL;
	}

	config_num = SCMI_PINCTRL_ATTRIBUTES_CONFIG_NUM(settings->attributes);

	if (!config_num) {
		return -EINVAL;
	}

	if ((config_num * 2) > ARM_SCMI_PINCTRL_MAX_CONFIG_SIZE) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, PINCTRL_SETTINGS_CONFIGURE,
			     settings,
			     sizeof(*settings) -
			     (ARM_SCMI_PINCTRL_MAX_CONFIG_SIZE - config_num * 2) * 4,
			     NULL, 0x0);
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}
