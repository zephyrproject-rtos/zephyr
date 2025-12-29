/*
 * Copyright (c) 2023 Meta
 * Copyright (c) 2026 Paul Wedeck <paulwedeck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/drivers/can.h>
#include <zephyr/spinlock.h>

LOG_MODULE_REGISTER(test, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

#define TEST_DATA        "0123456789ABCDEF"
#define SAMPLE_DATA_SIZE (sizeof(TEST_DATA) + CAN_MAX_DLEN)
/* Assert that size of the test string (without '\0') fits in the SAMPLE_DATA_SIZE */
BUILD_ASSERT((sizeof(TEST_DATA) - 1) < SAMPLE_DATA_SIZE);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static uint8_t rx_buffer[SAMPLE_DATA_SIZE];
static size_t rx_len;
static struct k_spinlock rx_lck;

static void rx_func(const struct device *dev, struct can_frame *frame, void *user_data)
{
	uint8_t dlc_bytes = can_dlc_to_bytes(frame->dlc);

	K_SPINLOCK(&rx_lck) {
		zassert_true((rx_len + dlc_bytes) <= SAMPLE_DATA_SIZE);
		if (rx_len + dlc_bytes <= SAMPLE_DATA_SIZE && dlc_bytes <= sizeof(frame->data)) {
			memcpy(rx_buffer + rx_len, frame->data, dlc_bytes);
			rx_len += dlc_bytes;
		}
	}
}

static void *can_setup(void)
{
	struct can_filter filter = {
		.id = CONFIG_LOG_BACKEND_CAN_ID,
		.mask = IS_ENABLED(CONFIG_LOG_BACKEND_USE_EXTID) ? CAN_EXT_ID_MASK
								 : CAN_STD_ID_MASK,
		.flags = IS_ENABLED(CONFIG_LOG_BACKEND_USE_EXTID) ? CAN_FILTER_IDE : 0,
	};

	can_stop(can_dev);
	can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	can_start(can_dev);

	can_add_rx_filter(can_dev, rx_func, NULL, &filter);

	return NULL;
}

struct log_backend_can_fixture {
};

ZTEST_F(log_backend_can, test_log_backend_can_main)
{
	LOG_RAW(TEST_DATA);

	k_sleep(K_SECONDS(1));

	K_SPINLOCK(&rx_lck) {
		zassert_equal(rx_len, sizeof(TEST_DATA) - 1);
		zassert_mem_equal(rx_buffer, TEST_DATA, sizeof(TEST_DATA) - 1);
	}
}

ZTEST_SUITE(log_backend_can, NULL, can_setup, NULL, NULL, NULL);
