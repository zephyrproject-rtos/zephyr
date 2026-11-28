/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX loopback L1 driver: send_frame() must replay the COMPLETE wire frame,
 * FCS included, exactly as the NCN5130 would deliver it — L2 validates the
 * length (8 + LG) and the FCS against the last octet.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/knx/knx_l1.h>
#include <string.h>

static uint8_t rx[64];
static uint16_t rx_len;
static bool rx_eof;
static bool rx_started;

static void data_ind(const struct device *dev, enum knx_ph_data_ind_class c, uint8_t b)
{
	ARG_UNUSED(dev);
	switch (c) {
	case KNX_PH_IND_START_OF_FRAME:
		rx_started = true;
		rx_len = 0;
		rx[rx_len++] = b;
		break;
	case KNX_PH_IND_INNER_FRAME_CHAR:
		rx[rx_len++] = b;
		break;
	case KNX_PH_IND_END_OF_FRAME:
		rx_eof = true;
		break;
	default:
		break;
	}
}

static void reset_con(const struct device *dev, enum knx_ph_status s)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(s);
}

static const struct knx_phy_callbacks cbs = {.data_ind = data_ind, .reset_con = reset_con};

/* The knx_l1_*() wrappers assert that dev->api lies in the linker's
 * knx_phy_driver_api section, which only exists in a CONFIG_KNX_STACK build; go
 * through the vtable directly.
 */
#define API(dev) ((const struct knx_phy_driver_api *)(dev)->api)

ZTEST_SUITE(knx_loopback, NULL, NULL, NULL, NULL, NULL);

ZTEST(knx_loopback, test_send_frame_replays_the_fcs)
{
	const struct device *dev = device_get_binding("knx_loopback");
	/* Standard frame: 8 + LG octets, last one is the FCS. */
	const uint8_t frame[] = {0xBC, 0x11, 0x01, 0x08, 0x01, 0xE1, 0x00, 0x81, 0x4D};

	rx_len = 0;
	rx_eof = rx_started = false;
	API(dev)->set_callbacks(dev, &cbs);

	zassert_equal(API(dev)->send_frame(dev, frame, sizeof(frame)), 0);
	zassert_true(rx_started && rx_eof);
	zassert_equal(rx_len, sizeof(frame), "replayed %u of %u octets — FCS dropped?", rx_len,
		      (unsigned int)sizeof(frame));
	zassert_mem_equal(rx, frame, sizeof(frame));
}

ZTEST(knx_loopback, test_send_frame_without_callbacks_does_not_crash)
{
	const struct device *dev = device_get_binding("knx_loopback");
	const uint8_t frame[] = {0xBC, 0x11, 0x01, 0x08, 0x01, 0xE1, 0x00, 0x81, 0x4D};

	API(dev)->set_callbacks(dev, NULL);
	zassert_not_equal(API(dev)->send_frame(dev, frame, sizeof(frame)), 0,
			  "no registered receiver: refuse instead of calling NULL");
}
