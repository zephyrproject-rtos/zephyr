/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <internal/nrfs_backend.h>
#include <nrfs_backend_ipc_service.h>
#include <nrfs_mram.h>
#include <nrfs_temp.h>
#include <zephyr/ztest.h>

#define IPC_BACKEND_CONNECTION_TIMEOUT_MS 5000
#define NUM_OF_MRAM_REQUESTS              10000
#define MRAM_REQUESTS_DEAD_TIME_MS        1
#define NUM_OF_TEMP_REQUESTS              100
#define TEMP_REQUESTS_DEAD_TIME_MS        100

struct ipc_perf_result {
	uint32_t sent_requests;
	uint32_t handled_requests;
	uint32_t failed_to_send;
};

static volatile uint32_t tst_perf_served_mram_requests;
static volatile uint32_t tst_perf_served_temp_meas_requests;

/*
 * Callback function for counting handled TEMP service requests
 */
static void temp_handler_for_performance_test(nrfs_temp_evt_t const *p_evt, void *context)
{
	switch (p_evt->type) {
	case NRFS_TEMP_EVT_MEASURE_DONE:
		tst_perf_served_temp_meas_requests++;
	default:
		break;
	}
}

/*
 * Callback function for counting handled MRAM service requests
 */
static void mram_latency_handler_for_performance_test(nrfs_mram_latency_evt_t const *p_evt,
						      void *context)
{
	switch (p_evt->type) {
	case NRFS_MRAM_LATENCY_REQ_APPLIED:
		tst_perf_served_mram_requests++;
	default:
		break;
	}
}

/*
 * Test NRFS MRAM latency service requests handling performance
 */
ZTEST(nrfs_stress_test, test_mram_nrfs_requests_performance)
{
	struct ipc_perf_result tst_ipc_perf_result;
	uint32_t request_counter = 0;
	volatile int32_t tst_ctx = 1;

	TC_PRINT("START test_mram_nrfs_requests_performance\n");
	zassert_equal(nrfs_mram_init(mram_latency_handler_for_performance_test), NRFS_SUCCESS,
		      "Failed to initialise NRFS MRAM latency service");
	memset(&tst_ipc_perf_result, 0, sizeof(tst_ipc_perf_result));
	while (request_counter < NUM_OF_MRAM_REQUESTS) {
		if (nrfs_mram_set_latency(true, (void *)tst_ctx) == NRFS_SUCCESS) {
			tst_ipc_perf_result.sent_requests++;
		} else {
			tst_ipc_perf_result.failed_to_send++;
		}
		k_msleep(MRAM_REQUESTS_DEAD_TIME_MS);
		tst_ctx++;
		request_counter++;
	}
	/* wait for any remaining requests responses */
	k_msleep(10 * MRAM_REQUESTS_DEAD_TIME_MS);
	tst_ipc_perf_result.handled_requests = tst_perf_served_mram_requests;
	TC_PRINT("STOP test_mram_nrfs_requests_performance\n");
	TC_PRINT("SENT: %d, HANDLED: %d, FAILED TO SEND: %d\n", tst_ipc_perf_result.sent_requests,
		 tst_ipc_perf_result.handled_requests, tst_ipc_perf_result.failed_to_send);
	zassert_equal(tst_ipc_perf_result.sent_requests, tst_ipc_perf_result.handled_requests,
		      "NRFS MRAM requests sent != served");
}

/*
 * Test temperature service requests handling performance
 */
ZTEST(nrfs_stress_test, test_temperature_nrfs_requests_performance)
{
	struct ipc_perf_result tst_ipc_perf_result;
	uint32_t request_counter = 0;
	volatile int32_t tst_ctx = 1;

	TC_PRINT("START test_temperature_nrfs_requests_performance\n");
	zassert_equal(nrfs_temp_init(temp_handler_for_performance_test), NRFS_SUCCESS,
		      "Failed to initialise NRFS temperature service");
	memset((void *)&tst_ipc_perf_result, 0, sizeof(tst_ipc_perf_result));
	while (request_counter < NUM_OF_TEMP_REQUESTS) {
		if (nrfs_temp_measure_request((void *)tst_ctx) == NRFS_SUCCESS) {
			tst_ipc_perf_result.sent_requests++;
		} else {
			tst_ipc_perf_result.failed_to_send++;
		}
		k_msleep(TEMP_REQUESTS_DEAD_TIME_MS);
		tst_ctx++;
		request_counter++;
	}
	/* wait for any remaining requests responses */
	k_msleep(10 * TEMP_REQUESTS_DEAD_TIME_MS);
	tst_ipc_perf_result.handled_requests = tst_perf_served_temp_meas_requests;
	TC_PRINT("STOP test_temperature_nrfs_requests_performance\n");
	TC_PRINT("SENT: %d, HANDLED: %d, FAILED TO SEND: %d\n", tst_ipc_perf_result.sent_requests,
		 tst_ipc_perf_result.handled_requests, tst_ipc_perf_result.failed_to_send);
	zassert_equal(tst_ipc_perf_result.sent_requests, tst_ipc_perf_result.handled_requests,
		      "NRFS TEMP requests sent != served");
}

/*
 * Test setup
 */
static void *test_setup(void)
{
	int ret;

	tst_perf_served_mram_requests = 0;
	tst_perf_served_temp_meas_requests = 0;

	TC_PRINT("Hello World! %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("Waiting for NRFS backend init\n");

	/* Wait for IPC backend connection */
	ret = nrfs_backend_wait_for_connection(K_MSEC(IPC_BACKEND_CONNECTION_TIMEOUT_MS));
	zassert_equal(ret, 0, "Failed to establih NRFS backend connection. err: %d", ret);
	return NULL;
}

ZTEST_SUITE(nrfs_stress_test, NULL, test_setup, NULL, NULL, NULL);
