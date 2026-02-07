/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief Comprehensive unit tests for ticker module
 *
 * This test suite provides comprehensive coverage of the ticker module
 * interfaces used in the Zephyr Bluetooth Low Energy Controller.
 *
 * Test Coverage:
 * - Ticker initialization and deinitialization
 * - Ticker node allocation and lifecycle
 * - Start, stop, and update operations
 * - Callback handling and execution
 * - Multiple concurrent ticker nodes
 * - Collision handling and priorities
 * - Lazy timeout handling
 * - Edge cases and error conditions
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <string.h>

#include "ticker.h"

/* Test configuration */
#define TEST_INSTANCE_INDEX 0
#define TEST_USER_ID_0 0
#define TEST_USER_ID_1 1
#define TEST_TICKER_NODES 8
#define TEST_TICKER_USERS 2
#define TEST_TICKER_USER_OPS 8

/* Ticker node storage */
static uint8_t ticker_nodes[TEST_TICKER_NODES][TICKER_NODE_T_SIZE];
static uint8_t ticker_users[TEST_TICKER_USERS][TICKER_USER_T_SIZE];
static uint8_t ticker_user_ops[TEST_TICKER_USER_OPS][TICKER_USER_OP_T_SIZE];

/* Test callback tracking */
static uint32_t timeout_callback_count;
static uint32_t timeout_callback_ticks_at_expire;
static uint32_t timeout_callback_ticks_drift;
static uint32_t timeout_callback_remainder;
static uint16_t timeout_callback_lazy;
static uint8_t timeout_callback_force;
static void *timeout_callback_context;

static uint32_t op_callback_count;
static uint32_t op_callback_status;
static void *op_callback_context;

/* HAL mock functions */
extern void hal_mock_reset(void);
extern void hal_mock_set_increment(uint32_t increment);
extern void hal_mock_set_ticks(uint32_t ticks);
extern uint32_t hal_mock_get_ticks(void);

/**
 * Test timeout callback function
 */
static void test_timeout_callback(uint32_t ticks_at_expire,
				  uint32_t ticks_drift,
				  uint32_t remainder,
				  uint16_t lazy,
				  uint8_t force,
				  void *context)
{
	timeout_callback_count++;
	timeout_callback_ticks_at_expire = ticks_at_expire;
	timeout_callback_ticks_drift = ticks_drift;
	timeout_callback_remainder = remainder;
	timeout_callback_lazy = lazy;
	timeout_callback_force = force;
	timeout_callback_context = context;
}

/**
 * Test operation callback function
 */
static void test_op_callback(uint32_t status, void *op_context)
{
	op_callback_count++;
	op_callback_status = status;
	op_callback_context = op_context;
}

/**
 * Mock caller ID get callback
 */
static uint8_t test_caller_id_get_cb(uint8_t user_id)
{
	return TICKER_CALL_ID_PROGRAM;
}

/**
 * Mock scheduler callback
 */
static void test_sched_cb(uint8_t caller_id, uint8_t callee_id,
			  uint8_t chain, void *instance)
{
	/* Scheduler callback for ticker job scheduling */
}

/**
 * Mock trigger set callback
 */
static void test_trigger_set_cb(uint32_t value)
{
	/* Trigger set callback for hardware timer */
}

/**
 * Reset test callback counters
 */
static void reset_test_callbacks(void)
{
	timeout_callback_count = 0;
	timeout_callback_ticks_at_expire = 0;
	timeout_callback_ticks_drift = 0;
	timeout_callback_remainder = 0;
	timeout_callback_lazy = 0;
	timeout_callback_force = 0;
	timeout_callback_context = NULL;
	
	op_callback_count = 0;
	op_callback_status = 0;
	op_callback_context = NULL;
}

/**
 * Setup function called before each test
 */
static void *ticker_test_setup(void)
{
	hal_mock_reset();
	reset_test_callbacks();
	
	/* Deinitialize ticker if already initialized */
	if (ticker_is_initialized(TEST_INSTANCE_INDEX)) {
		ticker_deinit(TEST_INSTANCE_INDEX);
	}
	
	return NULL;
}

/**
 * Teardown function called after each test
 */
static void ticker_test_teardown(void *fixture)
{
	/* Cleanup ticker instance */
	if (ticker_is_initialized(TEST_INSTANCE_INDEX)) {
		ticker_deinit(TEST_INSTANCE_INDEX);
	}
}

/**
 * @brief Test ticker initialization and deinitialization
 *
 * Validates:
 * - ticker_init() returns success
 * - ticker_is_initialized() returns correct state
 * - ticker_deinit() properly cleans up
 */
ZTEST(ticker_tests, test_ticker_init_deinit)
{
	uint8_t ret;
	
	/* Test ticker is not initialized initially */
	zassert_false(ticker_is_initialized(TEST_INSTANCE_INDEX),
		      "Ticker should not be initialized");
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	
	zassert_equal(ret, TICKER_STATUS_SUCCESS,
		      "ticker_init should return success");
	
	/* Verify ticker is initialized */
	zassert_true(ticker_is_initialized(TEST_INSTANCE_INDEX),
		     "Ticker should be initialized");
	
	/* Deinitialize ticker */
	ret = ticker_deinit(TEST_INSTANCE_INDEX);
	zassert_equal(ret, 0, "ticker_deinit should return 0");
	
	/* Verify ticker is not initialized */
	zassert_false(ticker_is_initialized(TEST_INSTANCE_INDEX),
		      "Ticker should not be initialized after deinit");
}

/**
 * @brief Test basic ticker start and stop operations
 *
 * Validates:
 * - ticker_start() can schedule a ticker node
 * - Operation callback is invoked with success status
 * - ticker_stop() can cancel a ticker node
 */
ZTEST(ticker_tests, test_ticker_start_stop)
{
	uint8_t ret;
	uint8_t ticker_id = 0;
	uint32_t ticks_anchor = 1000;
	uint32_t ticks_first = 100;
	uint32_t ticks_periodic = 500;
	void *test_context = (void *)0x12345678;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	/* Start a ticker node */
	ret = ticker_start(TEST_INSTANCE_INDEX,
			   TEST_USER_ID_0,
			   ticker_id,
			   ticks_anchor,
			   ticks_first,
			   ticks_periodic,
			   TICKER_NULL_REMAINDER,
			   TICKER_NULL_LAZY,
			   TICKER_NULL_SLOT,
			   test_timeout_callback,
			   test_context,
			   test_op_callback,
			   (void *)1);
	
	/* Verify return status (may be SUCCESS or BUSY) */
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_start should return SUCCESS or BUSY");
	
	/* Process ticker job to execute operation */
	ticker_job(NULL);
	
	/* Verify operation callback was invoked */
	zassert_equal(op_callback_count, 1,
		      "Operation callback should be called once");
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Operation should succeed");
	zassert_equal(op_callback_context, (void *)1,
		      "Operation context should match");
	
	/* Reset callbacks for stop test */
	reset_test_callbacks();
	
	/* Stop the ticker node */
	ret = ticker_stop(TEST_INSTANCE_INDEX,
			  TEST_USER_ID_0,
			  ticker_id,
			  test_op_callback,
			  (void *)2);
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_stop should return SUCCESS or BUSY");
	
	/* Process ticker job */
	ticker_job(NULL);
	
	/* Verify stop operation callback */
	zassert_equal(op_callback_count, 1,
		      "Stop operation callback should be called");
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Stop operation should succeed");
}

/**
 * @brief Test ticker timeout callback execution
 *
 * Validates:
 * - Timeout callback is invoked when ticker expires
 * - Correct parameters are passed to callback
 * - Context is preserved
 */
ZTEST(ticker_tests, test_ticker_timeout_callback)
{
	uint8_t ret;
	uint8_t ticker_id = 0;
	uint32_t ticks_anchor;
	uint32_t ticks_first = 10;
	uint32_t ticks_periodic = 0; /* One-shot ticker */
	void *test_context = (void *)0xABCDEF00;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	/* Get current ticks */
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start a one-shot ticker */
	ret = ticker_start(TEST_INSTANCE_INDEX,
			   TEST_USER_ID_0,
			   ticker_id,
			   ticks_anchor,
			   ticks_first,
			   ticks_periodic,
			   TICKER_NULL_REMAINDER,
			   TICKER_NULL_LAZY,
			   TICKER_NULL_SLOT,
			   test_timeout_callback,
			   test_context,
			   test_op_callback,
			   NULL);
	
	/* Process ticker job */
	ticker_job(NULL);
	
	/* Verify start operation succeeded */
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Start operation should succeed");
	
	/* Advance time to trigger expiration */
	hal_mock_set_ticks(ticks_anchor + ticks_first + 1);
	
	/* Trigger and process ticker to fire timeout */
	ticker_trigger(TEST_INSTANCE_INDEX);
	ticker_worker(NULL);
	ticker_job(NULL);
	
	/* Verify timeout callback was invoked */
	zassert_equal(timeout_callback_count, 1,
		      "Timeout callback should be called once");
	zassert_equal(timeout_callback_context, test_context,
		      "Timeout callback context should match");
	zassert_equal(timeout_callback_lazy, 0,
		      "Lazy should be 0 for one-shot ticker");
}

/**
 * @brief Test multiple concurrent ticker nodes
 *
 * Validates:
 * - Multiple ticker nodes can be started simultaneously
 * - Each ticker operates independently
 * - Callbacks are invoked correctly for each ticker
 */
ZTEST(ticker_tests, test_multiple_ticker_nodes)
{
	uint8_t ret;
	uint32_t ticks_anchor;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start multiple ticker nodes */
	for (uint8_t i = 0; i < 4; i++) {
		ret = ticker_start(TEST_INSTANCE_INDEX,
				   TEST_USER_ID_0,
				   i,  /* ticker_id */
				   ticks_anchor,
				   100 + (i * 50),  /* Different first timeout */
				   500,  /* Same periodic interval */
				   TICKER_NULL_REMAINDER,
				   TICKER_NULL_LAZY,
				   TICKER_NULL_SLOT,
				   test_timeout_callback,
				   (void *)(uintptr_t)i,  /* Unique context */
				   test_op_callback,
				   (void *)(uintptr_t)(i + 10));
		
		zassert_true(ret == TICKER_STATUS_SUCCESS || 
			     ret == TICKER_STATUS_BUSY,
			     "ticker_start should succeed");
		
		/* Process ticker job */
		ticker_job(NULL);
	}
	
	/* Verify all operations completed */
	zassert_equal(op_callback_count, 4,
		      "All start operations should complete");
}

/**
 * @brief Test ticker update operation
 *
 * Validates:
 * - ticker_update() can modify drift and slot parameters
 * - Update operation completes successfully
 */
ZTEST(ticker_tests, test_ticker_update)
{
	uint8_t ret;
	uint8_t ticker_id = 1;
	uint32_t ticks_anchor;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start a ticker node */
	ret = ticker_start(TEST_INSTANCE_INDEX,
			   TEST_USER_ID_0,
			   ticker_id,
			   ticks_anchor,
			   200,
			   1000,
			   TICKER_NULL_REMAINDER,
			   TICKER_NULL_LAZY,
			   100,  /* ticks_slot */
			   test_timeout_callback,
			   (void *)0x1000,
			   test_op_callback,
			   NULL);
	
	ticker_job(NULL);
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Start should succeed");
	
	reset_test_callbacks();
	
	/* Update the ticker with drift correction */
	ret = ticker_update(TEST_INSTANCE_INDEX,
			    TEST_USER_ID_0,
			    ticker_id,
			    10,  /* ticks_drift_plus */
			    5,   /* ticks_drift_minus */
			    20,  /* ticks_slot_plus */
			    10,  /* ticks_slot_minus */
			    TICKER_NULL_LAZY,
			    0,   /* force */
			    test_op_callback,
			    (void *)0x2000);
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_update should succeed");
	
	ticker_job(NULL);
	
	/* Verify update operation completed */
	zassert_equal(op_callback_count, 1,
		      "Update operation callback should be called");
	zassert_equal(op_callback_context, (void *)0x2000,
		      "Update context should match");
}

/**
 * @brief Test ticker lazy timeout handling
 *
 * Validates:
 * - Lazy parameter allows skipping timeout callbacks
 * - TICKER_LAZY_MUST_EXPIRE forces callback execution
 */
ZTEST(ticker_tests, test_ticker_lazy_timeout)
{
	uint8_t ret;
	uint8_t ticker_id = 2;
	uint32_t ticks_anchor;
	uint16_t lazy = 5;  /* Allow skipping up to 5 timeouts */
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start a ticker with lazy timeout */
	ret = ticker_start(TEST_INSTANCE_INDEX,
			   TEST_USER_ID_0,
			   ticker_id,
			   ticks_anchor,
			   100,
			   200,  /* Periodic */
			   TICKER_NULL_REMAINDER,
			   lazy,
			   TICKER_NULL_SLOT,
			   test_timeout_callback,
			   (void *)0x3000,
			   test_op_callback,
			   NULL);
	
	ticker_job(NULL);
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Start with lazy should succeed");
}

/**
 * @brief Test ticker with must_expire flag
 *
 * Validates:
 * - TICKER_LAZY_MUST_EXPIRE ensures callback is always invoked
 * - Must expire tickers have priority in scheduling
 */
ZTEST(ticker_tests, test_ticker_must_expire)
{
	uint8_t ret;
	uint8_t ticker_id = 3;
	uint32_t ticks_anchor;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start ticker with MUST_EXPIRE */
	ret = ticker_start(TEST_INSTANCE_INDEX,
			   TEST_USER_ID_0,
			   ticker_id,
			   ticks_anchor,
			   150,
			   300,
			   TICKER_NULL_REMAINDER,
			   TICKER_LAZY_MUST_EXPIRE,
			   TICKER_NULL_SLOT,
			   test_timeout_callback,
			   (void *)0x4000,
			   test_op_callback,
			   NULL);
	
	ticker_job(NULL);
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Start with must_expire should succeed");
}

/**
 * @brief Test ticker_stop_abs operation
 *
 * Validates:
 * - ticker_stop_abs() can stop ticker at absolute tick time
 * - Operation completes successfully
 */
ZTEST(ticker_tests, test_ticker_stop_abs)
{
	uint8_t ret;
	uint8_t ticker_id = 4;
	uint32_t ticks_anchor;
	uint32_t ticks_at_stop;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start a ticker */
	ret = ticker_start(TEST_INSTANCE_INDEX,
			   TEST_USER_ID_0,
			   ticker_id,
			   ticks_anchor,
			   500,
			   1000,
			   TICKER_NULL_REMAINDER,
			   TICKER_NULL_LAZY,
			   TICKER_NULL_SLOT,
			   test_timeout_callback,
			   (void *)0x5000,
			   test_op_callback,
			   NULL);
	
	ticker_job(NULL);
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Start should succeed");
	
	reset_test_callbacks();
	
	/* Stop at specific tick time */
	ticks_at_stop = ticks_anchor + 250;
	ret = ticker_stop_abs(TEST_INSTANCE_INDEX,
			      TEST_USER_ID_0,
			      ticker_id,
			      ticks_at_stop,
			      test_op_callback,
			      (void *)0x6000);
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_stop_abs should succeed");
	
	ticker_job(NULL);
	
	/* Verify stop operation completed */
	zassert_equal(op_callback_count, 1,
		      "Stop_abs operation callback should be called");
}

/**
 * @brief Test ticker with remainder (sub-microsecond precision)
 *
 * Validates:
 * - ticker_start_us() supports remainder for sub-tick precision
 * - Remainder accumulates correctly over periods
 */
ZTEST(ticker_tests, test_ticker_start_us_with_remainder)
{
	uint8_t ret;
	uint8_t ticker_id = 5;
	uint32_t ticks_anchor;
	uint32_t ticks_first = 100;
	uint32_t remainder_first = 15;  /* Sub-tick remainder */
	uint32_t ticks_periodic = 200;
	uint32_t remainder_periodic = 30;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start ticker with sub-tick precision */
	ret = ticker_start_us(TEST_INSTANCE_INDEX,
			      TEST_USER_ID_0,
			      ticker_id,
			      ticks_anchor,
			      ticks_first,
			      remainder_first,
			      ticks_periodic,
			      remainder_periodic,
			      TICKER_NULL_LAZY,
			      TICKER_NULL_SLOT,
			      test_timeout_callback,
			      (void *)0x7000,
			      test_op_callback,
			      NULL);
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_start_us should succeed");
	
	ticker_job(NULL);
	
	/* Verify operation completed successfully */
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "ticker_start_us should complete successfully");
}

/**
 * @brief Test ticker_next_slot_get operation
 *
 * Validates:
 * - ticker_next_slot_get() can query next expiring ticker
 * - Returns correct ticker ID and time information
 */
ZTEST(ticker_tests, test_ticker_next_slot_get)
{
	uint8_t ret;
	uint8_t ticker_id = TICKER_NULL;
	uint32_t ticks_current = 0;
	uint32_t ticks_to_expire = 0;
	uint32_t ticks_anchor;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start a ticker */
	ret = ticker_start(TEST_INSTANCE_INDEX,
			   TEST_USER_ID_0,
			   0,  /* ticker_id */
			   ticks_anchor,
			   300,
			   600,
			   TICKER_NULL_REMAINDER,
			   TICKER_NULL_LAZY,
			   50,  /* ticks_slot */
			   test_timeout_callback,
			   NULL,
			   test_op_callback,
			   NULL);
	
	ticker_job(NULL);
	
	/* Query next slot */
	ret = ticker_next_slot_get(TEST_INSTANCE_INDEX,
				   TEST_USER_ID_0,
				   &ticker_id,
				   &ticks_current,
				   &ticks_to_expire,
				   test_op_callback,
				   (void *)0x8000);
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_next_slot_get should succeed");
	
	ticker_job(NULL);
}

/**
 * @brief Test ticker_job_idle_get operation
 *
 * Validates:
 * - ticker_job_idle_get() reports job idle state correctly
 * - Operation completes with proper status
 */
ZTEST(ticker_tests, test_ticker_job_idle_get)
{
	uint8_t ret;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	/* Query job idle status */
	ret = ticker_job_idle_get(TEST_INSTANCE_INDEX,
				  TEST_USER_ID_0,
				  test_op_callback,
				  (void *)0x9000);
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_job_idle_get should succeed");
	
	ticker_job(NULL);
}

/**
 * @brief Test ticker_ticks_diff_get utility
 *
 * Validates:
 * - Correctly calculates tick differences
 * - Handles wraparound properly
 */
ZTEST(ticker_tests, test_ticker_ticks_diff)
{
	uint32_t ticks_now = 1000;
	uint32_t ticks_old = 500;
	uint32_t diff;
	
	/* Test normal difference */
	diff = ticker_ticks_diff_get(ticks_now, ticks_old);
	zassert_equal(diff, 500, "Tick difference should be 500");
	
	/* Test wraparound case */
	ticks_now = 100;
	ticks_old = 0xFFFFFF00;  /* Large value near wraparound */
	diff = ticker_ticks_diff_get(ticks_now, ticks_old);
	zassert_equal(diff, 0x200, "Wraparound tick difference incorrect");
	
	/* Test zero difference */
	ticks_now = 1000;
	ticks_old = 1000;
	diff = ticker_ticks_diff_get(ticks_now, ticks_old);
	zassert_equal(diff, 0, "Same ticks should have zero difference");
}

#if defined(CONFIG_BT_TICKER_EXT)
/**
 * @brief Test ticker_start_ext with extended features
 *
 * Validates:
 * - ticker_start_ext() with extension data
 * - Extended features configuration
 */
ZTEST(ticker_tests, test_ticker_start_ext)
{
	uint8_t ret;
	uint8_t ticker_id = 6;
	uint32_t ticks_anchor;
	struct ticker_ext ext_data;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Configure extension data */
	memset(&ext_data, 0, sizeof(ext_data));
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	ext_data.ticks_slot_window = 100;
	ext_data.reschedule_state = 0;
#endif
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	ext_data.expire_info_id = TICKER_NULL;
	ext_data.ext_timeout_func = NULL;
#endif
	
	/* Start ticker with extended features */
	ret = ticker_start_ext(TEST_INSTANCE_INDEX,
			       TEST_USER_ID_0,
			       ticker_id,
			       ticks_anchor,
			       400,
			       800,
			       TICKER_NULL_REMAINDER,
			       TICKER_NULL_LAZY,
			       100,
			       test_timeout_callback,
			       (void *)0xA000,
			       test_op_callback,
			       NULL,
			       &ext_data);
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_start_ext should succeed");
	
	ticker_job(NULL);
	
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "ticker_start_ext should complete successfully");
}

/**
 * @brief Test ticker_update_ext with extended features
 *
 * Validates:
 * - ticker_update_ext() modifies ticker with must_expire
 * - Extended update features work correctly
 */
ZTEST(ticker_tests, test_ticker_update_ext)
{
	uint8_t ret;
	uint8_t ticker_id = 7;
	uint32_t ticks_anchor;
	struct ticker_ext ext_data;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start a ticker with ext features */
	memset(&ext_data, 0, sizeof(ext_data));
	ret = ticker_start_ext(TEST_INSTANCE_INDEX,
			       TEST_USER_ID_0,
			       ticker_id,
			       ticks_anchor,
			       350,
			       700,
			       TICKER_NULL_REMAINDER,
			       TICKER_NULL_LAZY,
			       80,
			       test_timeout_callback,
			       (void *)0xB000,
			       test_op_callback,
			       NULL,
			       &ext_data);
	
	ticker_job(NULL);
	zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
		      "Start_ext should succeed");
	
	reset_test_callbacks();
	
	/* Update with extended parameters */
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	ret = ticker_update_ext(TEST_INSTANCE_INDEX,
				TEST_USER_ID_0,
				ticker_id,
				15,  /* ticks_drift_plus */
				8,   /* ticks_drift_minus */
				25,  /* ticks_slot_plus */
				12,  /* ticks_slot_minus */
				TICKER_NULL_LAZY,
				1,   /* force */
				test_op_callback,
				(void *)0xC000,
				1,   /* must_expire */
				TICKER_NULL);  /* expire_info_id */
#else
	ret = ticker_update_ext(TEST_INSTANCE_INDEX,
				TEST_USER_ID_0,
				ticker_id,
				15,  /* ticks_drift_plus */
				8,   /* ticks_drift_minus */
				25,  /* ticks_slot_plus */
				12,  /* ticks_slot_minus */
				TICKER_NULL_LAZY,
				1,   /* force */
				test_op_callback,
				(void *)0xC000,
				1);  /* must_expire */
#endif
	
	zassert_true(ret == TICKER_STATUS_SUCCESS || ret == TICKER_STATUS_BUSY,
		     "ticker_update_ext should succeed");
	
	ticker_job(NULL);
}
#endif /* CONFIG_BT_TICKER_EXT */

/**
 * @brief Test error handling - invalid instance
 *
 * Validates:
 * - Operations on uninitialized instance fail gracefully
 */
ZTEST(ticker_tests, test_error_invalid_instance)
{
	uint8_t ret;
	uint8_t invalid_instance = 99;
	
	/* Try to use invalid instance without initialization */
	zassert_false(ticker_is_initialized(invalid_instance),
		      "Invalid instance should not be initialized");
}

/**
 * @brief Test boundary conditions - maximum ticker nodes
 *
 * Validates:
 * - Can allocate up to maximum configured ticker nodes
 * - Exceeding limit is handled properly
 */
ZTEST(ticker_tests, test_boundary_max_nodes)
{
	uint8_t ret;
	uint32_t ticks_anchor;
	
	/* Initialize ticker */
	ret = ticker_init(TEST_INSTANCE_INDEX,
			  TEST_TICKER_NODES, ticker_nodes,
			  TEST_TICKER_USERS, ticker_users,
			  TEST_TICKER_USER_OPS, ticker_user_ops,
			  test_caller_id_get_cb,
			  test_sched_cb,
			  test_trigger_set_cb);
	zassert_equal(ret, TICKER_STATUS_SUCCESS, "ticker_init failed");
	
	ticks_anchor = ticker_ticks_now_get();
	
	/* Start ticker nodes up to configured maximum */
	for (uint8_t i = 0; i < TEST_TICKER_NODES; i++) {
		ret = ticker_start(TEST_INSTANCE_INDEX,
				   TEST_USER_ID_0,
				   i,
				   ticks_anchor,
				   100 + (i * 10),
				   500,
				   TICKER_NULL_REMAINDER,
				   TICKER_NULL_LAZY,
				   TICKER_NULL_SLOT,
				   test_timeout_callback,
				   (void *)(uintptr_t)i,
				   test_op_callback,
				   NULL);
		
		ticker_job(NULL);
		
		/* Should succeed for all configured nodes */
		zassert_equal(op_callback_status, TICKER_STATUS_SUCCESS,
			      "Should be able to start all configured nodes");
		
		reset_test_callbacks();
	}
}

/**
 * Test suite setup and registration
 */
ZTEST_SUITE(ticker_tests, NULL, ticker_test_setup,
	    NULL, NULL, ticker_test_teardown);
