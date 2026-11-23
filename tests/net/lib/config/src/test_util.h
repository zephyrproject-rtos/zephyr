/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright 2026 inovex GmbH
 */

/*
 * The pieces both scenarios of this test share: a minimal SNTP server on
 * loopback, and the realtime clock the tests read a query's outcome from.
 */

#ifndef ZEPHYR_TESTS_NET_LIB_CONFIG_TEST_UTIL_H_
#define ZEPHYR_TESTS_NET_LIB_CONFIG_TEST_UTIL_H_

#include <stdbool.h>
#include <time.h>

/* The time the test server serves, and a different one the tests write to
 * the clock beforehand, so "the clock was set" and "the clock was left
 * alone" tell each other apart.
 */
#define SERVED_TIME    1000000000
#define UNTOUCHED_TIME 500000000

/* Runs an SNTP server on 127.0.0.1 that answers whatever reaches it. */
void sntp_test_server_start(void);
void sntp_test_server_stop(void);

/* Whether the server answers at all. A server that takes a request and then
 * stays silent makes the client run into its timeout, which is how a test
 * arranges for a query to still be in flight when it needs one to be.
 */
void sntp_test_server_set_answering(bool answering);

void set_clock(time_t seconds);
time_t get_clock(void);

#endif /* ZEPHYR_TESTS_NET_LIB_CONFIG_TEST_UTIL_H_ */
