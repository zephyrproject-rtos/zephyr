/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX loopback L1 driver — for unit testing without real hardware.
 *
 * Selected by CONFIG_KNX_LOOPBACK=y (mutually exclusive with CONFIG_NCN5130).
 *
 * Behaviour:
 *   send_frame() feeds the frame back into data_ind as if it had arrived from
 *   the bus, then returns 0 (no real bus arbitration to wait on).  This lets
 *   the L2..L7 RX path be exercised without any physical TP1 interface.
 *
 *   reset_req() immediately calls the registered reset_con callback with
 *   KNX_PH_STATUS_OK.
 *   set_address_req() is a no-op.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/knx/knx_l1.h>

LOG_MODULE_REGISTER(knx_loopback, CONFIG_KNX_STACK_LOG_LEVEL);

struct knx_loopback_data {
	const struct knx_phy_callbacks *callbacks;
};

/* -------- knx_phy_driver_api implementation -------- */

static int loopback_send_frame(const struct device *dev, const uint8_t *frame, uint16_t len)
{
	struct knx_loopback_data *data = dev->data;

	if (len < 7) {
		return -EMSGSIZE;
	}

	LOG_HEXDUMP_DBG(frame, len, "LOOPBACK TX:");

	/* Replay the frame back into the RX path (minus the FCS byte), then
	 * confirm as positive — there is no real bus arbitration to wait on.
	 */
	data->callbacks->data_ind(dev, KNX_PH_IND_START_OF_FRAME, frame[0]);
	for (uint16_t i = 1; i < len - 1u; i++) {
		data->callbacks->data_ind(dev, KNX_PH_IND_INNER_FRAME_CHAR, frame[i]);
	}
	data->callbacks->data_ind(dev, KNX_PH_IND_END_OF_FRAME, 0);

	return 0;
}

static void loopback_reset_req(const struct device *dev)
{
	struct knx_loopback_data *data = dev->data;

	/* Loopback has no transceiver to sync to — immediately confirm OK. */
	data->callbacks->reset_con(dev, KNX_PH_STATUS_OK);
}

static void loopback_set_address_req(const struct device *dev, uint8_t addr_low,
				      uint8_t addr_high)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(addr_low);
	ARG_UNUSED(addr_high);
}

static void loopback_set_callbacks(const struct device *dev,
				    const struct knx_phy_callbacks *callbacks)
{
	struct knx_loopback_data *data = dev->data;

	data->callbacks = callbacks;
}

static DEVICE_API(knx_phy, knx_loopback_api) = {
	.reset_req = loopback_reset_req,
	.set_address_req = loopback_set_address_req,
	.send_frame = loopback_send_frame,
	.set_callbacks = loopback_set_callbacks,
};

/* -------- Zephyr driver init -------- */

static struct knx_loopback_data knx_loopback_data_0;

static int knx_loopback_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_INF("KNX loopback driver initialised");
	return 0;
}

DEVICE_DEFINE(knx_loopback, "knx_loopback", knx_loopback_init, NULL, &knx_loopback_data_0, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &knx_loopback_api);
