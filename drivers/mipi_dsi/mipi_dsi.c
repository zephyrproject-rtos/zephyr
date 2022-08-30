/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mipi_dsi.h>

ssize_t mipi_dsi_generic_read(const struct device *dev, uint8_t channel,
			      const void *params, size_t nparams,
			      void *buf, size_t len)
{
	struct mipi_dsi_msg msg = {
		.tx_len = nparams,
		.tx_buf = params,
		.rx_len = len,
		.rx_buf = buf
	};

	switch (nparams) {
	case 0U:
		msg.type = MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM;
		break;

	case 1U:
		msg.type = MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM;
		break;

	case 2U:
		msg.type = MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM;
		break;

	default:
		return -EINVAL;
	}

	return mipi_dsi_transfer(dev, channel, &msg);
}

ssize_t mipi_dsi_generic_write(const struct device *dev, uint8_t channel,
			       const void *buf, size_t len)
{
	struct mipi_dsi_msg msg = {
		.tx_buf = buf,
		.tx_len = len
	};

	switch (len) {
	case 0U:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM;
		break;

	case 1U:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM;
		break;

	case 2U:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM;
		break;

	default:
		msg.type = MIPI_DSI_GENERIC_LONG_WRITE;
		break;
	}

	return mipi_dsi_transfer(dev, channel, &msg);
}

ssize_t mipi_dsi_dcs_read(const struct device *dev, uint8_t channel,
			  uint8_t cmd, void *buf, size_t len)
{
	struct mipi_dsi_msg msg = {
		.type = MIPI_DSI_DCS_READ,
		.cmd = cmd,
		.rx_buf = buf,
		.rx_len = len
	};

	return mipi_dsi_transfer(dev, channel, &msg);
}

ssize_t mipi_dsi_dcs_write(const struct device *dev, uint8_t channel,
			   uint8_t cmd, const void *buf, size_t len)
{
	struct mipi_dsi_msg msg = {
		.cmd = cmd,
		.tx_buf = buf,
		.tx_len = len
	};

	switch (len) {
	case 0U:
		msg.type = MIPI_DSI_DCS_SHORT_WRITE;
		break;

	case 1U:
		msg.type = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		break;

	default:
		msg.type = MIPI_DSI_DCS_LONG_WRITE;
		break;
	}

	return mipi_dsi_transfer(dev, channel, &msg);
}
