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
#define I2C_CONTROLLER_NODE_IRQN         DT_IRQN(I2C_CONTROLLER_NODE)
#define I2C_CONTROLLER_NODE_IRQ_PRIORITY DT_IRQ(I2C_CONTROLLER_NODE, priority)
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

ZTEST(i2c_nrfx_twim_async, test_01_ordinary_write)
{
	int ret;

	test_prepare();

	static const test_transfer_buf_t tx_buf = {
		.len = 3,
		.buf = {0x12, 0x34, 0x56},
	};

	ret = i2c_write(sample_i2c_controller, tx_buf.buf, tx_buf.len, I2C_TARGET_ADDR);

	zassert_equal(ret, 0, "i2c_write failed");
	zassert_equal(target_received_buffers_count, 1);
	zassert_equal(target_received_buffers[0].len, tx_buf.len);
	zassert_mem_equal(tx_buf.buf, target_received_buffers[0].buf, tx_buf.len);
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

ZTEST(i2c_nrfx_twim_async, test_02_i2c_nrfx_twim_exclusive_access)
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

static struct {
	const struct device *dev;
	int res;
	void *ctx;
} test_twim_async_handler_params;

static K_SEM_DEFINE(test_twim_async_handler_sem, 0, 1);

static void test_twim_async_handler_params_reset(void)
{
	memset(&test_twim_async_handler_params, 0, sizeof(test_twim_async_handler_params));
	k_sem_take(&test_twim_async_handler_sem, K_NO_WAIT);
}

static void test_twim_async_handler_that_gives_sem(const struct device *dev, int res, void *ctx)
{
	test_twim_async_handler_params.dev = dev;
	test_twim_async_handler_params.res = res;
	test_twim_async_handler_params.ctx = ctx;

	k_sem_give(&test_twim_async_handler_sem);
}

ZTEST(i2c_nrfx_twim_async, test_03_i2c_nrfx_twim_async_transfer)
{
	int ret;

	test_prepare();

	static test_transfer_buf_t tx_buf = {
		.len = 4,
		.buf = {0x78, 0x9a, 0xbc},
	};

	ret = i2c_nrfx_twim_exclusive_access_acquire(sample_i2c_controller, K_FOREVER);
	zassert_equal(ret, 0, "Can't acquire exclusive access to sample_i2c_controller");

	struct i2c_msg msg = {
		.buf = tx_buf.buf,
		.len = tx_buf.len,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
	};
	uint32_t test_twim_async_handler_dummy_ctx = 0xCC;

	test_twim_async_handler_params_reset();

	ret = i2c_nrfx_twim_async_transfer_begin(sample_i2c_controller, &msg, I2C_TARGET_ADDR,
						 test_twim_async_handler_that_gives_sem,
						 &test_twim_async_handler_dummy_ctx);

	zassert_equal(ret, 0, "i2c_nrfx_twim_async_transfer_begin failed");

	ret = k_sem_take(&test_twim_async_handler_sem, K_MSEC(1000));
	zassert_equal(ret, 0,
		      "Can't take test_twim_async_handler_sem, seem the callback "
		      "test_twim_async_handler_that_gives_sem was not called");

	zassert_equal_ptr(sample_i2c_controller, test_twim_async_handler_params.dev);
	zassert_equal(0, test_twim_async_handler_params.res);
	zassert_equal_ptr(&test_twim_async_handler_dummy_ctx, test_twim_async_handler_params.ctx);

	i2c_nrfx_twim_exclusive_access_release(sample_i2c_controller);

	zassert_equal(target_received_buffers_count, 1);
	zassert_equal(target_received_buffers[0].len, tx_buf.len);
	zassert_mem_equal(tx_buf.buf, target_received_buffers[0].buf, tx_buf.len);
}

static struct {
	test_transfer_buf_t *tx_buffers;
	uint32_t tx_buffers_count;
	uint32_t tx_buffer_curr_idx;
	uint32_t dummy_ctx;
	atomic_t finished;
} async_transfer_state;

static void async_transfer_state_process(void);

static void test_twim_async_handler_that_calls_process(const struct device *dev, int res, void *ctx)
{
	zassert_equal_ptr(dev, sample_i2c_controller);
	zassert_equal(res, 0);
	zassert_equal_ptr(ctx, &async_transfer_state.dummy_ctx);

	async_transfer_state.tx_buffer_curr_idx++;

	async_transfer_state_process();
}

static void async_transfer_state_process(void)
{
	if (async_transfer_state.tx_buffer_curr_idx < async_transfer_state.tx_buffers_count) {
		int ret;

		test_transfer_buf_t *tx_buffer =
			&async_transfer_state.tx_buffers[async_transfer_state.tx_buffer_curr_idx];

		struct i2c_msg msg = {
			.buf = tx_buffer->buf,
			.len = tx_buffer->len,
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		};

		ret = i2c_nrfx_twim_async_transfer_begin(sample_i2c_controller, &msg,
							 I2C_TARGET_ADDR,
							 test_twim_async_handler_that_calls_process,
							 &async_transfer_state.dummy_ctx);

		zassert_equal(ret, 0, "i2c_nrfx_twim_async_transfer_begin failed");
	} else {
		/* No more to send */
		atomic_set(&async_transfer_state.finished, 1);
	}
}

ZTEST(i2c_nrfx_twim_async, test_04_i2c_nrfx_twim_async_transfer_from_zli)
{
	int ret;

	test_prepare();

	/* Prepare the sequence of buffers to be written in consecutive calls to
	 * i2c_nrfx_twim_async_transfer_begin
	 */
	memset(&async_transfer_state, 0, sizeof(0));
	static test_transfer_buf_t tx_buffers[] = {
		{.len = 3, .buf = {0x51, 0x52, 0x53}},
		{.len = 4, .buf = {0x61, 0x62, 0x63, 0x64}},
		{.len = 5, .buf = {0x75, 0x76, 0x77, 0x78, 0x79}},
	};
	async_transfer_state.tx_buffers = tx_buffers;
	async_transfer_state.tx_buffers_count = ARRAY_SIZE(tx_buffers);
	async_transfer_state.tx_buffer_curr_idx = 0;
	atomic_set(&async_transfer_state.finished, 0);

	ret = i2c_nrfx_twim_exclusive_access_acquire(sample_i2c_controller, K_FOREVER);
	zassert_equal(ret, 0, "Can't acquire exclusive access to sample_i2c_controller");

	/* Let's boost the IRQ priority of the sample_i2c_controller to the highest, ZLI */
	z_arm_irq_priority_set(I2C_CONTROLLER_NODE_IRQN, 0, IRQ_ZERO_LATENCY);

	/* Let's start the multi transfer operation with IRQs happening at ZLI priority. */
	async_transfer_state_process();

	/* Let's wait for operation to finish. Note that the handler is called from ZLI,
	 * so no Zephyr semaphore API can be used.
	 */
	k_sleep(K_MSEC(1000));

	zassert_equal(atomic_get(&async_transfer_state.finished), 1);

	/* Let's restore the original IRQ priority for the sample_i2c_controller. */
	z_arm_irq_priority_set(I2C_CONTROLLER_NODE_IRQN, I2C_CONTROLLER_NODE_IRQ_PRIORITY, 0);

	i2c_nrfx_twim_exclusive_access_release(sample_i2c_controller);

	zassert_equal(async_transfer_state.tx_buffers_count,
		      async_transfer_state.tx_buffer_curr_idx);

	/* Let's check if target received all tx_buffers. */
	zassert_equal(target_received_buffers_count, async_transfer_state.tx_buffers_count);
	for (uint32_t i = 0; i < ARRAY_SIZE(tx_buffers); ++i) {
		zassert_equal(target_received_buffers[i].len, tx_buffers[i].len);
		zassert_mem_equal(target_received_buffers[i].buf, tx_buffers[i].buf,
				  tx_buffers[i].len);
	}

	/* Let's check if ordinary i2c write is still functional. */
	test_target_received_buffers_reset();

	static const test_transfer_buf_t tx_buf = {
		.len = 3,
		.buf = {0x12, 0x34, 0x56},
	};

	ret = i2c_write(sample_i2c_controller, tx_buf.buf, tx_buf.len, I2C_TARGET_ADDR);

	zassert_equal(ret, 0, "i2c_write failed");
	zassert_equal(target_received_buffers_count, 1);
	zassert_equal(target_received_buffers[0].len, tx_buf.len);
	zassert_mem_equal(tx_buf.buf, target_received_buffers[0].buf, tx_buf.len);
}

ZTEST_SUITE(i2c_nrfx_twim_async, NULL, NULL, NULL, NULL, NULL);
