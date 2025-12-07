/*
 * Copyright (c) 2025 by Markus Becker <markushx@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dali.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali_logger, LOG_LEVEL_INF);

/*
 * Values of the <length> field of the serial output format, see
 * https://github.com/SvenHaedrich/dali_usb_lpc1114/blob/main/doc/messages.md
 * DALI_LOG_STATUS_CORRUPT has no equivalent in the DALI USB interface: the
 * driver does not report whether a start bit or a data bit was corrupt.
 */
#define DALI_LOG_LENGTH_BACKWARD  0x08U
#define DALI_LOG_LENGTH_GEAR      0x10U
#define DALI_LOG_LENGTH_DEVICE    0x18U
#define DALI_LOG_LENGTH_FIRMWARE  0x20U
#define DALI_LOG_STATUS_CORRUPT   0x88U
#define DALI_LOG_STATUS_FAILURE   0x91U
#define DALI_LOG_STATUS_RECOVERED 0x92U

static void dali_rx_callback(const struct device *dev, struct dali_frame frame, void *user_data)
{
	uint8_t length;

	switch (frame.event_type) {
	case DALI_FRAME_BACKWARD:
		length = DALI_LOG_LENGTH_BACKWARD;
		break;
	case DALI_FRAME_GEAR:
		length = DALI_LOG_LENGTH_GEAR;
		break;
	case DALI_FRAME_DEVICE:
		length = DALI_LOG_LENGTH_DEVICE;
		break;
	case DALI_FRAME_FIRMWARE:
		length = DALI_LOG_LENGTH_FIRMWARE;
		break;
	case DALI_EVENT_BUS_FAILURE:
		length = DALI_LOG_STATUS_FAILURE;
		break;
	case DALI_EVENT_BUS_IDLE:
		length = DALI_LOG_STATUS_RECOVERED;
		break;
	case DALI_FRAME_CORRUPT:
	default:
		length = DALI_LOG_STATUS_CORRUPT;
		break;
	}

	/* fixed width fields: 8 digit timestamp in ms, 2 digit length, 8 digit data */
	LOG_INF("{%08x:%02x %08x}", k_uptime_get_32(), length, frame.data);
}

int main(void)
{
	const struct device *dali_dev = DEVICE_DT_GET(DT_ALIAS(dali));
	int ret;

	if (!device_is_ready(dali_dev)) {
		LOG_ERR("DALI device not ready");
		return 0;
	}

	ret = dali_set_receive_callback(dali_dev, dali_rx_callback, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to set DALI receive callback (%d)", ret);
		return 0;
	}

	LOG_INF("DALI logger started");

	return 0;
}
