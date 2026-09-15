/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tlv320aic3104_fault.h"

#include <errno.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#include "tlv320aic3104_bus.h"
#include "tlv320aic3104_priv.h"
#include "tlv320aic3104_regs.h"

int tlv320aic3104_fault_init(const struct device *dev)
{
	return tlv320aic3104_bus_write_reg(dev, 0, HP_OUTPUT_DRIVER_CTRL,
					   HP_OUTPUT_DRIVER_SHORT_CCT_PROTECT_EN |
						   HP_OUTPUT_DRIVER_SHORT_CCT_AUTO_PWRDN);
}

int tlv320aic3104_fault_register_callback(const struct device *dev, audio_codec_error_callback_t cb)
{
	struct tlv320aic3104_data *data = dev->data;

	data->fault_cb = cb;
	return 0;
}

int tlv320aic3104_fault_clear(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;

	data->fault_sticky_errors = 0;
	return 0;
}

int tlv320aic3104_fault_check(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;
	uint8_t flags;
	int ret;

	ret = tlv320aic3104_bus_read_reg(dev, 0, STICKY_INTERRUPT_FLAGS, &flags);
	if (ret < 0) {
		return ret;
	}

	if ((flags & STICKY_INTERRUPT_FLAGS_SHORT_CCT_MASK) == 0) {
		return 0;
	}

	data->fault_sticky_errors |= AUDIO_CODEC_ERROR_OVERCURRENT;

	if (data->fault_cb != NULL) {
		data->fault_cb(dev, data->fault_sticky_errors);
	}

	return 0;
}

int tlv320aic3104_fault_get_errors(const struct device *dev, uint32_t *out_errors)
{
	const struct tlv320aic3104_data *data = dev->data;

	if (out_errors == NULL) {
		return -EINVAL;
	}

	*out_errors = data->fault_sticky_errors;
	return 0;
}
