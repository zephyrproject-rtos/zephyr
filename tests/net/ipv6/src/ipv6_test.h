/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Кирило Шипачов
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_NET_IPV6_SRC_IPV6_TEST_H_
#define ZEPHYR_TESTS_NET_IPV6_SRC_IPV6_TEST_H_

#include <stdbool.h>

#include <zephyr/net/net_pkt.h>

/**
 * @brief Called for every packet the test interface transmits.
 *
 * @param pkt Packet handed to the driver.
 * @param user_data Value the handler was registered with.
 *
 * @retval true The handler took the packet, the driver does nothing else
 *              with it.
 * @retval false The driver handles the packet as it normally would.
 */
typedef bool (*tx_callback)(struct net_pkt *pkt, void *user_data);

struct test_tx_handler {
	tx_callback fn;
	void *user_data;
};

/* Set by a test that needs to look at the frames the interface sends, and
 * cleared before every test. While it is NULL the driver answers ICMPv6 and
 * feeds everything else back into the receive path, which is what most tests
 * in this suite expect.
 */
extern struct test_tx_handler *tx_handler;

#endif /* ZEPHYR_TESTS_NET_IPV6_SRC_IPV6_TEST_H_ */
