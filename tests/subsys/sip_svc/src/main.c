/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_mailbox.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include <zephyr/sys/__assert.h>

ZTEST_SUITE(sip_svc_tests, NULL, NULL, NULL, NULL, NULL);

#define SVC_METHOD "smc"
#define ECHO_CMD   (0x01U)
#define TEST_VAL   (0xDEADBEEFU)

#define SIP_SVC_CLIENT_INSTANCES CONFIG_ZTRESS_MAX_THREADS

struct private_data {
	uint64_t time_start;
	uint64_t time_end;
	struct k_sem semaphore;
};

struct total_time {
	uint64_t sync_time;
	uint64_t async_time;
};

static void get_sync_callback(uint32_t c_token, struct sip_svc_response *response)
{
	if (response == NULL) {
		return;
	}

	struct private_data *priv = (struct private_data *)response->priv_data;

	priv->time_end = k_cycle_get_64();
	printk("sip_svc version in TFA is %2ld.%02ld\n", response->a2, response->a3);

	k_sem_give(&(priv->semaphore));
}

/**
 * @brief send SYNC request
 *
 * @param token sip_svc token
 */
static void sip_svc_send_sync_request(uint32_t token)
{
	int err, trans_id;
	void *ctrl;
	uint64_t t = 0;
	struct total_time *tot_time;
	struct private_data priv;
	struct sip_svc_request req = {0};

	ctrl = sip_svc_get_controller(SVC_METHOD);
	__ASSERT(ctrl != NULL, "couldn't get the controller");

	tot_time = (struct total_time *)sip_svc_get_priv_data(ctrl, token);
	__ASSERT(tot_time != NULL, "tot_time should not be NULL");

	err = sip_svc_open(ctrl, token, K_FOREVER);
	__ASSERT(err == 0, "couldn't open channel");

	k_sem_init(&(priv.semaphore), 0, 1);

	req.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_SYNC, 0);
	req.a0 = SMC_FUNC_ID_GET_SVC_VERSION;
	req.priv_data = (void *)&priv;

	priv.time_start = k_cycle_get_64();
	trans_id = sip_svc_send(ctrl, token, &req, get_sync_callback);
	__ASSERT(trans_id >= 0, "error in sending request");

	err = k_sem_take(&(priv.semaphore), K_FOREVER);
	__ASSERT(err == 0, "Error in taking semaphore");

	t = k_cyc_to_us_ceil64(priv.time_end - priv.time_start);
	tot_time->sync_time += t;

	printk("In %s got SYNC response for id 0x%02x and time taken is %lldus\n",
	       k_thread_name_get(k_current_get()), trans_id, t);

	err = sip_svc_close(ctrl, token, NULL);
	__ASSERT(err == 0, "error in closing channel");
}

static void get_async_callback(uint32_t c_token, struct sip_svc_response *response)
{
	if (response == NULL) {
		return;
	}

	struct private_data *priv = (struct private_data *)response->priv_data;

	priv->time_end = k_cycle_get_64();
	uint32_t *resp_data = (uint32_t *)response->resp_data_addr;
	uint32_t resp_len = response->resp_data_size / sizeof(uint32_t);

	if (resp_data && resp_len) {
		__ASSERT((resp_data[1] == TEST_VAL), "SDM response is not matching");
	}

	k_sem_give(&(priv->semaphore));
	k_free(resp_data);
}

/**
 * @brief send ASYNC request
 *
 * @param token sip_svc token
 */
static void sip_svc_send_async_request(uint32_t token)
{
	int err, trans_id;
	/* size of mailbox command buffer, here we require 2 words */
	uint32_t cmd_size = (2 * sizeof(uint32_t));
	/* size of mailbox response buffer, here we require 2 words */
	uint32_t resp_size = (2 * sizeof(uint32_t));
	uint64_t t = 0;
	uint32_t *cmd_addr, *resp_addr;
	void *ctrl;
	struct total_time *tot_time;
	struct private_data priv;
	struct sip_svc_request req = {0};

	ctrl = sip_svc_get_controller(SVC_METHOD);
	__ASSERT(ctrl != NULL, "couldn't get the controller");

	err = sip_svc_open(ctrl, token, K_FOREVER);
	__ASSERT(err == 0, "couldn't open channel");

	tot_time = (struct total_time *)sip_svc_get_priv_data(ctrl, token);
	__ASSERT(tot_time != NULL, "tot_time should not be NULL");

	resp_addr = (uint32_t *)k_malloc(resp_size);
	__ASSERT(resp_addr != NULL, "couldn' get memory");

	k_sem_init(&(priv.semaphore), 0, 1);

	cmd_addr = (uint32_t *)k_malloc(cmd_size);
	__ASSERT(cmd_addr != NULL, "couldn't get memory");

	cmd_addr[0] = (1 << 12) | ECHO_CMD;
	cmd_addr[1] = 0xDEADBEEF;

	req.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
	req.a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
	req.a2 = (uint64_t)cmd_addr;
	req.a3 = (uint64_t)cmd_size;
	req.resp_data_addr = (uint64_t)resp_addr;
	req.resp_data_size = (uint64_t)resp_size;
	req.priv_data = (void *)&priv;

	priv.time_start = k_cycle_get_64();
	trans_id = sip_svc_send(ctrl, token, &req, get_async_callback);
	__ASSERT(trans_id >= 0, "error in sending request");

	err = k_sem_take(&(priv.semaphore), K_FOREVER);
	__ASSERT(err == 0, "Error in taking semaphore");

	t = k_cyc_to_us_ceil64(priv.time_end - priv.time_start);
	tot_time->async_time += t;

	printk("In %s got ASYNC response for id 0x%02x and time taken is %lldus\n",
	       k_thread_name_get(k_current_get()), trans_id, t);

	err = sip_svc_close(ctrl, token, NULL);
	__ASSERT(err == 0, "error in closing channel");
}

static bool sip_svc_register_and_send(void *user_data, uint32_t cnt, bool last, int prio)
{
	int err, token;
	void *ctrl;

	printk("\nIn %s and count is %d\n", k_thread_name_get(k_current_get()), cnt);

	ctrl = sip_svc_get_controller(SVC_METHOD);
	if (ctrl == NULL) {
		return false;
	}

	token = sip_svc_register(ctrl, user_data);
	if (token == SIP_SVC_ID_INVALID) {
		return false;
	}

	for (uint32_t i = 0; i < CONFIG_PACKETS_PER_ITERATION; i++) {
		sip_svc_send_sync_request(token);
		sip_svc_send_async_request(token);
	}

	err = sip_svc_unregister(ctrl, token);
	if (err < 0) {
		return false;
	}

	return true;
}

ZTEST(sip_svc_tests, test_sip_stress)
{
	struct total_time t[SIP_SVC_CLIENT_INSTANCES] = {0};
	struct total_time average = {0};

	ZTRESS_EXECUTE(ZTRESS_THREAD(sip_svc_register_and_send, &t[0], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[1], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[2], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[3], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[4], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[5], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[6], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[7], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[8], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[9], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[10], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[11], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[12], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[13], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[14], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)),
		       ZTRESS_THREAD(sip_svc_register_and_send, &t[15], CONFIG_ITERATIONS, 0,
				     Z_TIMEOUT_TICKS(10)));

	for (uint32_t i = 0; i < SIP_SVC_CLIENT_INSTANCES; i++) {
		average.sync_time += t[i].sync_time;
		average.async_time += t[i].async_time;
	}

	average.sync_time = average.sync_time / (CONFIG_ITERATIONS * CONFIG_PACKETS_PER_ITERATION *
						 SIP_SVC_CLIENT_INSTANCES);
	average.async_time =
		average.async_time /
		(CONFIG_ITERATIONS * CONFIG_PACKETS_PER_ITERATION * SIP_SVC_CLIENT_INSTANCES);

	printk("\n***************************************\n");
	printk("Average SYNC transaction time is %lldus\n", average.sync_time);
	printk("Average ASYNC transaction time is %lldus\n", average.async_time);
	printk("\n***************************************\n");
}
