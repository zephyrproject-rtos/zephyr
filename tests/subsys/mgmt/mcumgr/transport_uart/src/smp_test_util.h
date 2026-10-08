/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SMP_TEST_UTIL_
#define H_SMP_TEST_UTIL_

#include <stddef.h>
#include <stdint.h>

#define TEST_ECHO_STRING "tEsTiNg eChO dAtA"

/**
 * Builds an os_mgmt echo request, header included, and returns its length.
 */
size_t smp_test_build_echo_request(uint8_t *buffer, size_t size, const char *data, uint8_t seq);

/**
 * Builds an os_mgmt echo response, header included, and returns its length.
 */
size_t smp_test_build_echo_response(uint8_t *buffer, size_t size, const char *data, uint8_t seq);

/**
 * Builds a transport_mgmt connect request to the given transport, header included, and returns
 * its length.
 */
size_t smp_test_build_connect_request(uint8_t *buffer, size_t size, uint32_t transport,
				      uint8_t seq);

/**
 * Checks that a packet, header included, is the os_mgmt echo response for the given data and
 * sequence number.
 */
void smp_test_check_echo_response(const uint8_t *packet, size_t len, const char *data, uint8_t seq);

#endif
