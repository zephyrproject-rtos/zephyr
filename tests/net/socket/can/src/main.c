/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socketcan.h>
#include <zephyr/net/socketcan_utils.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(socket_can, LOG_LEVEL_ERR);

/**
 * @brief Test of @a socketcan_to_can_frame()
 */
ZTEST(socket_can, test_socketcan_frame_to_can_frame)
{
	struct socketcan_frame sframe = { 0 };
	struct can_frame expected = { 0 };
	struct can_frame zframe;
	const uint8_t data[SOCKETCAN_MAX_DLEN] = { 0x01, 0x02, 0x03, 0x04,
						   0x05, 0x06, 0x07, 0x08 };

	sframe.can_id = BIT(31) | 1234;
	sframe.len = sizeof(data);
	memcpy(sframe.data, data, sizeof(sframe.data));

	expected.flags = CAN_FRAME_IDE;
	expected.id = 1234U;
	expected.dlc = can_bytes_to_dlc(sizeof(data));
	memcpy(expected.data, data, sizeof(data));

	socketcan_to_can_frame(&sframe, &zframe);

	LOG_HEXDUMP_DBG((const uint8_t *)&sframe, sizeof(sframe), "sframe");
	LOG_HEXDUMP_DBG((const uint8_t *)&zframe, sizeof(zframe), "zframe");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(zframe.flags, expected.flags, "Flags not equal");
	zassert_equal(zframe.id, expected.id, "CAN id invalid");
	zassert_equal(zframe.dlc, expected.dlc, "Msg length invalid");
	zassert_mem_equal(&zframe.data, &expected.data, can_dlc_to_bytes(expected.dlc),
			  "CAN data not same");

	/* Test RTR flag conversion after comparing data payload */
	sframe.can_id |= BIT(30);
	expected.flags |= CAN_FRAME_RTR;

	socketcan_to_can_frame(&sframe, &zframe);

	zassert_equal(zframe.flags, expected.flags, "Flags not equal");
	zassert_equal(zframe.id, expected.id, "CAN id invalid");
}

/**
 * @brief Test of @a socketcan_from_can_frame()
 */
ZTEST(socket_can, test_can_frame_to_socketcan_frame)
{
	struct socketcan_frame sframe = { 0 };
	struct socketcan_frame expected = { 0 };
	struct can_frame zframe = { 0 };
	const uint8_t data[SOCKETCAN_MAX_DLEN] = { 0x01, 0x02, 0x03, 0x04,
						   0x05, 0x06, 0x07, 0x08 };

	expected.can_id = BIT(31) | 1234;
	expected.len = sizeof(data);
	memcpy(expected.data, data, sizeof(expected.data));

	zframe.flags = CAN_FRAME_IDE;
	zframe.id = 1234U;
	zframe.dlc = can_bytes_to_dlc(sizeof(data));
	memcpy(zframe.data, data, sizeof(data));

	socketcan_from_can_frame(&zframe, &sframe);

	LOG_HEXDUMP_DBG((const uint8_t *)&sframe, sizeof(sframe), "sframe");
	LOG_HEXDUMP_DBG((const uint8_t *)&zframe, sizeof(zframe), "zframe");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(sframe.can_id, expected.can_id, "CAN ID not same");
	zassert_equal(sframe.len, expected.len, "CAN msg length not same");
	zassert_mem_equal(&sframe.data, &expected.data, sizeof(sframe.data), "CAN data not same");

	/* Test RTR flag conversion after comparing data payload */
	expected.can_id |= BIT(30);
	zframe.flags |= CAN_FRAME_RTR;

	socketcan_from_can_frame(&zframe, &sframe);
	zassert_equal(sframe.can_id, expected.can_id, "CAN ID not same");
}

/**
 * @brief Test of @a socketcan_to_can_filter()
 */
ZTEST(socket_can, test_socketcan_filter_to_can_filter)
{
	struct socketcan_filter sfilter = { 0 };
	struct can_filter expected = { 0 };
	struct can_filter zfilter = { 0 };

	sfilter.can_id = BIT(31) | 1234;
	sfilter.can_mask = BIT(31) | 1234;

	expected.flags = CAN_FILTER_IDE;
	expected.id = 1234U;
	expected.mask = 1234U;

	socketcan_to_can_filter(&sfilter, &zfilter);

	LOG_HEXDUMP_DBG((const uint8_t *)&zfilter, sizeof(zfilter), "zfilter");
	LOG_HEXDUMP_DBG((const uint8_t *)&sfilter, sizeof(sfilter), "sfilter");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(zfilter.flags, expected.flags, "Flags not equal");
	zassert_equal(zfilter.id, expected.id, "CAN id invalid");
	zassert_equal(zfilter.mask, expected.mask, "id mask not set");
}

/**
 * @brief Test of @a socketcan_from_can_filter()
 */
ZTEST(socket_can, test_can_filter_to_socketcan_filter)
{
	struct socketcan_filter sfilter = { 0 };
	struct socketcan_filter expected = { 0 };
	struct can_filter zfilter = { 0 };

	expected.can_id = BIT(31) | 1234;
	expected.can_mask = BIT(31) | 1234;
#ifndef CONFIG_CAN_ACCEPT_RTR
	expected.can_mask |= BIT(30);
#endif /* !CONFIG_CAN_ACCEPT_RTR */

	zfilter.flags = CAN_FILTER_IDE;
	zfilter.id = 1234U;
	zfilter.mask = 1234U;

	socketcan_from_can_filter(&zfilter, &sfilter);

	LOG_HEXDUMP_DBG((const uint8_t *)&zfilter, sizeof(zfilter), "zfilter");
	LOG_HEXDUMP_DBG((const uint8_t *)&sfilter, sizeof(sfilter), "sfilter");
	LOG_HEXDUMP_DBG((const uint8_t *)&expected, sizeof(expected), "expected");

	zassert_equal(sfilter.can_id, expected.can_id, "CAN ID not same");
	zassert_equal(sfilter.can_mask, expected.can_mask, "CAN mask not same");
}

ZTEST_SUITE(socket_can, NULL, NULL, NULL, NULL, NULL);

#if defined(CONFIG_NET_SOCKETS_CAN)

static int fd = -1;

static void socket_can_unbound_before(void *fixture)
{
	ARG_UNUSED(fixture);

	fd = zsock_socket(NET_AF_CAN, NET_SOCK_RAW, NET_CAN_RAW);
	zassert_true(fd >= 0, "socket() failed (%d)", errno);
}

static void socket_can_unbound_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (fd >= 0) {
		(void)zsock_close(fd);
		fd = -1;
	}
}

/**
 * @brief Test that CAN_RAW_FILTER cannot be set before the socket is bound
 */
ZTEST(socket_can_unbound, test_setsockopt_filter)
{
	struct socketcan_filter sfilter = {
		.can_id = 0x1,
		.can_mask = 0x7ffU,
	};
	int ret;

	ret = zsock_setsockopt(fd, NET_SOL_CAN_RAW, NET_CAN_RAW_FILTER, &sfilter, sizeof(sfilter));
	zexpect_equal(ret, -1, "setsockopt() succeeded on unbound socket");
	zexpect_equal(errno, ENODEV, "Unexpected errno (%d)", errno);
}

/**
 * @brief Test that CAN_RAW_FILTER cannot be read before the socket is bound
 */
ZTEST(socket_can_unbound, test_getsockopt_filter)
{
	struct socketcan_filter sfilter;
	net_socklen_t optlen = sizeof(sfilter);
	int ret;

	ret = zsock_getsockopt(fd, NET_SOL_CAN_RAW, NET_CAN_RAW_FILTER, &sfilter, &optlen);
	zexpect_equal(ret, -1, "getsockopt() succeeded on unbound socket");
	zexpect_equal(errno, ENODEV, "Unexpected errno (%d)", errno);
}

ZTEST_SUITE(socket_can_unbound, NULL, NULL, socket_can_unbound_before, socket_can_unbound_after,
	    NULL);

#endif /* CONFIG_NET_SOCKETS_CAN */
