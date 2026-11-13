/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network_server.h"
#include "test_common.h"
#include "region/test_region.h"

#include <zephyr/drivers/lora/lora_fake.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/ztest.h>

uint8_t app_payload[] = {0xde, 0xad, 0xbe, 0xef};

void lorawan_native_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(lorawan_set_region(test_region_get()->id));
	zassert_ok(lorawan_start());
}

void lorawan_native_after(void *fixture)
{
	ARG_UNUSED(fixture);

	lorawan_stop();
}

void run_successful_join(uint32_t rx_window)
{
	struct lorawan_join_config join_config = network_server_join_config();

	network_server_prepare_join(rx_window);
	zassert_ok(lorawan_join(&join_config));

	zassert_equal(fake_lora_send_async_fake.call_count, 1U);
	zassert_equal(fake_lora_recv_fake.call_count, rx_window);
	zassert_equal(fake_lora_config_fake.call_count, rx_window + 1U);
}

void prepare_send_test(void)
{
	zassert_ok(lorawan_set_conf_msg_tries(1U));
	run_successful_join(1U);
}

/*
 * After a confirmed uplink without ACK, LoRaWAN L2 1.0.4 section 4.3.1.3
 * requires RETRANSMIT_TIMEOUT to elapse after RECEIVE_DELAY2 before any
 * following uplink, whether or not RX2 was opened.
 */
void assert_retransmit_timeout(int64_t tx_time, int64_t next_time)
{
	int64_t elapsed = next_time - tx_time;

	zassert_true(elapsed >= TEST_RX2_DELAY_MS + TEST_RETRANSMIT_TIMEOUT_MIN_MS,
		     "Next uplink allowed %lld ms after TX", elapsed);
	zassert_true(elapsed <= TEST_RX2_DELAY_MS + TEST_RETRANSMIT_TIMEOUT_MAX_MS +
					k_ticks_to_ms_ceil64(1),
		     "Next uplink allowed %lld ms after TX", elapsed);
}

ZTEST(lorawan_native_common, test_errors_before_start)
{
	uint8_t dev_eui[8] = {0};
	uint8_t join_eui[8] = {0};
	uint8_t nwk_key[16] = {0};
	struct lorawan_join_config join_config = {
		.mode = LORAWAN_ACT_OTAA,
		.dev_eui = dev_eui,
		.otaa = {.join_eui = join_eui, .nwk_key = nwk_key},
	};
	uint16_t channels_mask = UINT16_MAX;

	zassert_equal(lorawan_join(&join_config), -EPERM);
	zassert_equal(lorawan_send(APP_PORT, NULL, 0U, LORAWAN_MSG_UNCONFIRMED), -EPERM);
	zassert_equal(lorawan_set_datarate(LORAWAN_DR_5), -EPERM);
	zassert_equal(lorawan_set_conf_msg_tries(1U), -EPERM);
	zassert_equal(lorawan_set_channels_mask(&channels_mask, 1U), -EPERM);
	zassert_equal(lorawan_request_link_check(false), -EPERM);
}

ZTEST_SUITE(lorawan_native_common, NULL, NULL, NULL, NULL, NULL);
