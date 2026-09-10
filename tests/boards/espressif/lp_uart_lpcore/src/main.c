/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * HP-core side of the LP UART loopback test.
 *
 * The LP core runs an LP-UART internal-loopback self-test and reports the
 * result word over the mailbox. This side waits for that word and asserts on
 * it, so the verdict is produced on the HP console without any external wiring.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/mbox.h>
#include <string.h>

#define LP_RESULT_PASS 0xA5A5A5A5U

static K_SEM_DEFINE(rx_sem, 0, 1);
static volatile uint32_t lp_result;

static void mbox_cb(const struct device *dev, mbox_channel_id_t channel_id, void *user_data,
		    struct mbox_msg *data)
{
	uint32_t value = 0;

	memcpy(&value, data->data, MIN(data->size, sizeof(value)));
	lp_result = value;
	k_sem_give(&rx_sem);
}

ZTEST(lp_uart_lpcore, test_lp_loopback_result)
{
	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	int ret;

	ret = mbox_register_callback_dt(&rx_channel, mbox_cb, NULL);
	zassert_ok(ret, "mbox_register_callback failed: %d", ret);

	ret = mbox_set_enabled_dt(&rx_channel, 1);
	zassert_ok(ret, "mbox_set_enabled failed: %d", ret);

	ret = k_sem_take(&rx_sem, K_SECONDS(5));
	zassert_ok(ret, "timed out waiting for LP core result");

	zassert_equal(lp_result, LP_RESULT_PASS,
		      "LP core reported LP-UART loopback failure (0x%08x)", lp_result);
}

ZTEST_SUITE(lp_uart_lpcore, NULL, NULL, NULL, NULL, NULL);
