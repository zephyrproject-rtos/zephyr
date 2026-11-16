/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_LORAWAN_NATIVE_REGION_TEST_REGION_H_
#define TESTS_SUBSYS_LORAWAN_NATIVE_REGION_TEST_REGION_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/lora.h>
#include <zephyr/lorawan/lorawan.h>

struct test_region_datarate {
	enum lora_datarate spreading_factor;
	enum lora_signal_bandwidth bandwidth;
	uint32_t rx_timeout_ms;
};

struct test_region {
	enum lorawan_region id;
	const struct test_region_datarate *datarates;
	size_t datarate_count;
	uint8_t rx1_dr_offset;
	enum lorawan_datarate rx2_datarate;
	enum lorawan_datarate test_datarate;
	uint32_t rx2_frequency;
	bool (*is_default_channel)(uint32_t frequency);
	bool has_1_0_x_vector;
	const uint32_t *cflist_frequencies;
	size_t cflist_frequency_count;
	uint16_t cflist_first_channel_idx;
};

/** @brief Get parameters for the region exercised by this test instance. */
const struct test_region *test_region_get(void);

#endif /* TESTS_SUBSYS_LORAWAN_NATIVE_REGION_TEST_REGION_H_ */
