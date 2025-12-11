/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/i2c_nrfx_twim.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define I2C_CONTROLLER_NODE              DT_ALIAS(i2c_controller)
#define I2C_CONTROLLER_TARGET_NODE       DT_ALIAS(i2c_controller_target)
#define I2C_CONTROLLER_DEVICE_GET        DEVICE_DT_GET(I2C_CONTROLLER_NODE)
#define I2C_CONTROLLER_TARGET_DEVICE_GET DEVICE_DT_GET(I2C_CONTROLLER_TARGET_NODE)
#define I2C_TARGET_ADDR                  0x0A

static const struct device *sample_i2c_controller = I2C_CONTROLLER_DEVICE_GET;
static const struct device *sample_i2c_controller_target = I2C_CONTROLLER_TARGET_DEVICE_GET;

#define TEST_TRANSFER_BUF_SIZE           16
#define TARGET_RECEIVED_BUFFERS_CAPACITY 4

typedef struct {
	uint32_t len;
	uint8_t buf[TEST_TRANSFER_BUF_SIZE];
} test_transfer_buf_t;

static uint32_t target_received_buffers_count;
static test_transfer_buf_t target_received_buffers[TARGET_RECEIVED_BUFFERS_CAPACITY];

static void test_target_received_buffers_reset(void)
{
	target_received_buffers_count = 0;
	memset(target_received_buffers, 0, sizeof(target_received_buffers));
}

BUILD_ASSERT(IS_ENABLED(CONFIG_I2C_TARGET_BUFFER_MODE),
	     "CONFIG_I2C_TARGET_BUFFER_MODE must be enabled");

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
static void sample_i2c_controller_target_buf_write_received_cb(struct i2c_target_config *config,
							       uint8_t *ptr, uint32_t len)
{
	if (target_received_buffers_count >= TARGET_RECEIVED_BUFFERS_CAPACITY) {
		return;
	}

	test_transfer_buf_t *transfer_buf = &target_received_buffers[target_received_buffers_count];

	transfer_buf->len = len;
	memcpy(transfer_buf->buf, ptr, MIN(len, TEST_TRANSFER_BUF_SIZE));
	target_received_buffers_count++;
}

static int sample_i2c_controller_target_buf_read_requested_cb(struct i2c_target_config *config,
							      uint8_t **ptr, uint32_t *len)
{
	zassert_true(false, "Call to target_buf_read_requested_cb was unexpected");
	return -EIO;
}
#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */

static const struct i2c_target_callbacks sample_i2c_controller_target_callbacks = {
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	.buf_write_received = sample_i2c_controller_target_buf_write_received_cb,
	.buf_read_requested = sample_i2c_controller_target_buf_read_requested_cb,
#endif
};

static struct i2c_target_config sample_i2c_controller_target_config = {
	.address = I2C_TARGET_ADDR, .callbacks = &sample_i2c_controller_target_callbacks};

static void test_prepare(void)
{
	int ret;
	bool ret_bool;

	ret_bool = device_is_ready(sample_i2c_controller_target);
	zassert_true(ret_bool, "sample_i2c_controller_target device is not ready");

	ret = i2c_target_register(sample_i2c_controller_target,
				  &sample_i2c_controller_target_config);
	zassert_equal(ret, 0, "sample_i2c_controller_target can't register target");

	ret_bool = device_is_ready(sample_i2c_controller);
	zassert_true(ret_bool, "sample_i2c_controller device is not ready");

	test_target_received_buffers_reset();
}

#define SOME_OTHER_I2C_ACCESSING_THREAD_STACK_SIZE 1024
static K_THREAD_STACK_DEFINE(some_other_i2c_accessing_thread_stack,
			     SOME_OTHER_I2C_ACCESSING_THREAD_STACK_SIZE);
static struct k_thread some_other_i2c_accessing_thread_data;
static K_SEM_DEFINE(some_other_i2c_accessing_thread_execute_sem, 0, 1);

static void some_other_i2c_accessing_thread(void *param1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (true) {
		if (k_sem_take(&some_other_i2c_accessing_thread_execute_sem, K_FOREVER) == 0) {
			int ret;
			test_transfer_buf_t *tx_buf = (test_transfer_buf_t *)param1;

			ret = i2c_write(sample_i2c_controller, tx_buf->buf, tx_buf->len,
					I2C_TARGET_ADDR);
			zassert_equal(ret, 0, "i2c_write failed");
		}
	}
}

static void some_other_i2c_accessing_thread_start(test_transfer_buf_t *tx_buf)
{
	(void)k_thread_create(&some_other_i2c_accessing_thread_data,
			      some_other_i2c_accessing_thread_stack,
			      K_THREAD_STACK_SIZEOF(some_other_i2c_accessing_thread_stack),
			      some_other_i2c_accessing_thread, tx_buf, NULL, NULL,
			      CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);
}

ZTEST(i2c_nrfx_twim_async, test_01_i2c_nrfx_twim_exclusive_access)
{
	int ret;

	test_prepare();

	static test_transfer_buf_t some_other_thread_tx_buf = {
		.len = 3,
		.buf = {0xE1, 0xE2, 0xE3},
	};

	some_other_i2c_accessing_thread_start(&some_other_thread_tx_buf);

	ret = i2c_nrfx_twim_exclusive_access_acquire(sample_i2c_controller, K_FOREVER);
	zassert_equal(ret, 0, "i2c_nrfx_twim_exclusive_access_acquire failed");

	/* While we are holding exclusive access to the sample_i2c_controller,
	 * let the some_other_i2c_accessing_thread attempt to perform an i2c_write.
	 */

	k_sem_give(&some_other_i2c_accessing_thread_execute_sem);

	/* Let the some_other_i2c_accessing_thread run for a while. */
	k_sleep(K_MSEC(100));

	/* We are still holding the exclusive access so the some_other_i2c_accessing_thread
	 * waits on semaphore. No i2c transfer should occur.
	 */
	zassert_equal(target_received_buffers_count, 0);

	i2c_nrfx_twim_exclusive_access_release(sample_i2c_controller);

	/* Let the some_other_i2c_accessing_thread finally perform the i2c_write. */
	k_sleep(K_MSEC(100));

	zassert_equal(target_received_buffers_count, 1);
	zassert_equal(target_received_buffers[0].len, some_other_thread_tx_buf.len);
	zassert_mem_equal(some_other_thread_tx_buf.buf, target_received_buffers[0].buf,
			  some_other_thread_tx_buf.len);
}

ZTEST_SUITE(i2c_nrfx_twim_async, NULL, NULL, NULL, NULL, NULL);
