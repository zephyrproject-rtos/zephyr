/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/canbus.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socketcan.h>
#include <zephyr/net/socketcan_utils.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(socket_can, LOG_LEVEL_ERR);

/* Regression tests below need the CAN network interface's ifindex to
 * bind() a raw CAN socket -- native_sim wires up &can_loopback0 as
 * zephyr,canbus by default (see boards/native/native_sim/native_sim.dts),
 * so there's always exactly one CANBUS_RAW L2 interface to find here.
 */
static int can_test_ifindex(void)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(CANBUS_RAW));

	zassert_not_null(iface, "No CAN network interface found");

	return net_if_get_by_iface(iface);
}

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

/**
 * @brief A CAN socket that never registers a filter must still release its
 * net_context on close() -- can_close_socket() used to gate that release on
 * "was a receivers[] entry found for this ctx", which is never true for a
 * filter-less socket, permanently leaking one CONFIG_NET_MAX_CONTEXTS slot
 * per such socket. Looping well past the pool size (4, see prj.conf) proves
 * every close() actually frees its slot -- if it didn't, socket() would
 * start failing with -ENOMEM/-ENOENT long before this loop finishes.
 */
ZTEST(socket_can, test_close_without_filter_does_not_leak_context)
{
	for (int i = 0; i < CONFIG_NET_MAX_CONTEXTS * 3; i++) {
		int fd = zsock_socket(NET_AF_CAN, NET_SOCK_RAW, NET_CAN_RAW);

		zassert_true(fd >= 0, "socket() failed on iteration %d (%d)", i, fd);
		zassert_ok(zsock_close(fd), "close() failed on iteration %d", i);
	}
}

/**
 * @brief A socket with more than one registered filter must have every
 * filter's receivers[] entry released on close(), not just the first one --
 * the original can_close_socket() returned as soon as it found one match,
 * leaking every filter slot after the first. Looping well past the
 * filter-slot pool size (4, see prj.conf) with 2 filters per socket proves
 * both are released each time -- if only the first were, this pool would
 * exhaust roughly twice as fast as the loop count below.
 */
ZTEST(socket_can, test_close_with_multiple_filters_releases_all_slots)
{
	struct can_filter filters[] = {
		{ .id = 0x100, .mask = CAN_STD_ID_MASK },
		{ .id = 0x200, .mask = CAN_STD_ID_MASK },
	};
	struct net_sockaddr_can addr = {
		.can_family = NET_AF_CAN,
		.can_ifindex = can_test_ifindex(),
	};

	for (int i = 0; i < CONFIG_NET_SOCKETS_CAN_RECEIVERS * 2; i++) {
		int fd = zsock_socket(NET_AF_CAN, NET_SOCK_RAW, NET_CAN_RAW);

		zassert_true(fd >= 0, "socket() failed on iteration %d (%d)", i, fd);
		zassert_ok(zsock_bind(fd, (struct net_sockaddr *)&addr, sizeof(addr)),
			   "bind() failed on iteration %d", i);

		for (int f = 0; f < ARRAY_SIZE(filters); f++) {
			struct socketcan_filter sfilter;

			socketcan_from_can_filter(&filters[f], &sfilter);
			zassert_ok(zsock_setsockopt(fd, NET_SOL_CAN_RAW, NET_CAN_RAW_FILTER,
						    &sfilter, sizeof(sfilter)),
				   "setsockopt() filter %d failed on iteration %d", f, i);
		}

		zassert_ok(zsock_close(fd), "close() failed on iteration %d", i);
	}
}

/**
 * @brief Any CAN packets sitting unread in a socket's recv_q at close() time
 * must be drained and freed, not left allocated forever -- nothing did that
 * before this fix. Each cycle below sends frames that match the socket's own
 * filter (the loopback driver delivers a transmitted frame straight back to
 * any matching RX filter on the same instance) but never reads them via
 * recv() before close(). Looping well past the packet-buffer pool size (4,
 * see prj.conf) proves every close() actually frees its unread packets -- if
 * it didn't, send() would start failing once the pool was exhausted.
 */
ZTEST(socket_can, test_close_with_unread_packets_drains_recv_q)
{
	struct can_filter filter = { .id = 0x321, .mask = CAN_STD_ID_MASK };
	struct can_frame frame = { .id = 0x321, .dlc = 1 };
	struct socketcan_filter sfilter;
	struct socketcan_frame sframe;
	struct net_sockaddr_can addr = {
		.can_family = NET_AF_CAN,
		.can_ifindex = can_test_ifindex(),
	};

	socketcan_from_can_filter(&filter, &sfilter);
	socketcan_from_can_frame(&frame, &sframe);

	for (int i = 0; i < CONFIG_NET_PKT_RX_COUNT * 3; i++) {
		int fd = zsock_socket(NET_AF_CAN, NET_SOCK_RAW, NET_CAN_RAW);

		zassert_true(fd >= 0, "socket() failed on iteration %d (%d)", i, fd);
		zassert_ok(zsock_bind(fd, (struct net_sockaddr *)&addr, sizeof(addr)),
			   "bind() failed on iteration %d", i);
		zassert_ok(zsock_setsockopt(fd, NET_SOL_CAN_RAW, NET_CAN_RAW_FILTER,
					    &sfilter, sizeof(sfilter)),
			   "setsockopt() failed on iteration %d", i);

		for (int f = 0; f < 2; f++) {
			zassert_true(zsock_send(fd, &sframe, sizeof(sframe), 0) >= 0,
				     "send() failed on iteration %d frame %d", i, f);
		}

		/* Give the loopback driver's RX thread a chance to deliver
		 * the frames into this socket's recv_q before closing it
		 * without ever having called recv().
		 */
		k_msleep(10);

		zassert_ok(zsock_close(fd), "close() failed on iteration %d", i);
	}
}

ZTEST_SUITE(socket_can, NULL, NULL, NULL, NULL, NULL);
