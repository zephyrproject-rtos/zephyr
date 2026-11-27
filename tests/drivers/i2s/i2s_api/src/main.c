/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include "i2s_api_test.h"

ZTEST_DMEM const struct device *dev_i2s_rx =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
ZTEST_DMEM const struct device *dev_i2s_tx =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_TX);
ZTEST_DMEM const struct device *dev_i2s =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
ZTEST_DMEM bool dir_both_supported;

static const struct i2s_test_frame frames[] = {
	{"I2S", I2S_FMT_DATA_FORMAT_I2S, 2U},
#ifdef CONFIG_I2S_TEST_FORMAT_PCM_SHORT
	{"PCM short frame", I2S_FMT_DATA_FORMAT_PCM_SHORT, 1U},
#endif
#ifdef CONFIG_I2S_TEST_FORMAT_PCM_LONG
	{"PCM long frame", I2S_FMT_DATA_FORMAT_PCM_LONG, 1U},
#endif
};

ZTEST_DMEM const struct i2s_test_frame *test_frame = &frames[0];

static void *setup(void)
{
	k_thread_access_grant(k_current_get(),
			      &rx_mem_slab, &tx_mem_slab);
	k_object_access_grant(dev_i2s_rx, k_current_get());
	k_object_access_grant(dev_i2s_tx, k_current_get());

	return NULL;
}

static void print_frame_format(void)
{
	TC_PRINT("Frame format: %s, %u channel(s)\n", test_frame->name, test_frame->channels);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	int ret;

	print_frame_format();

	zassert_not_null(dev_i2s_rx, "RX device not found");
	zassert_true(device_is_ready(dev_i2s_rx),
		     "device %s is not ready", dev_i2s_rx->name);
	zassert_not_null(dev_i2s_tx, "TX device not found");
	zassert_true(device_is_ready(dev_i2s_tx),
		     "device %s is not ready", dev_i2s_tx->name);

	ret = configure_stream(dev_i2s_rx, I2S_DIR_RX);
	zassert_equal(ret, TC_PASS);

	ret = configure_stream(dev_i2s_tx, I2S_DIR_TX);
	zassert_equal(ret, TC_PASS);
}

static void before_dir_both(void *fixture)
{
	ARG_UNUSED(fixture);

	int ret;

	print_frame_format();

	zassert_not_null(dev_i2s, "TX/RX device not found");
	zassert_true(device_is_ready(dev_i2s),
		     "device %s is not ready", dev_i2s->name);

	ret = configure_stream(dev_i2s, I2S_DIR_BOTH);
	zassert_equal(ret, TC_PASS);

	/* Check if the tested driver supports the I2S_DIR_BOTH value.
	 * Use the DROP trigger for this, as in the current state of the driver
	 * (READY, both TX and RX queues empty) it is actually a no-op.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	dir_both_supported = (ret == 0);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		zassert_true(dir_both_supported,
			     "I2S_DIR_BOTH value is supposed to be supported.");
	}
}

ZTEST_SUITE(i2s_loopback, NULL, setup, before, NULL, NULL);
ZTEST_SUITE(i2s_states, NULL, setup, before, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_states, NULL, setup, before_dir_both, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_loopback, NULL, setup, before_dir_both, NULL, NULL);
ZTEST_SUITE(i2s_errors, NULL, setup, before, NULL, NULL);

static void slab_reset(struct k_mem_slab *slab)
{
	(void)k_mem_slab_init(slab, slab->buffer, slab->info.block_size, slab->info.num_blocks);
}

void test_main(void)
{
	ARRAY_FOR_EACH(frames, i) {
		test_frame = &frames[i];
		ztest_run_all(NULL, false, 1, 1);

		/* Start the next format with idle streams and all blocks free. A
		 * block that a stream held when its clock stopped does not return.
		 */
		k_sleep(K_MSEC(200));
		(void)i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
		(void)i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
		slab_reset(&tx_mem_slab);
		slab_reset(&rx_mem_slab);
	}

	ztest_verify_all_test_suites_ran();
}
