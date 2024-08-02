/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM SiP services implementation.
 *
 *
 * ********
 * Overview
 * ********
 *
 * Typical flow
 *   (1) register a client, service returns a token
 *   (2) client opens a channel, (optionally only allow one channel at one time)
 *   (3) client send request with callback, service returns transaction id
 *   (5) service callback once the transaction complete
 *   (6) client close channel after receive callback
 *   (7) ... repeats (2) to (7) to send more request
 *   (8) unregister the client
 *
 * Abort opened channel
 *   (1) for some reasons, client want to terminate the operation
 *       on the opened channel. client may close the channel
 *       without waiting for all transaction being completed
 *   (2) service will proceed to close the channel and set the client
 *       at ABORT state. The client will be not allowed to reopen
 *       the channel until service complete all its associated transactions
 *       and bring the client back to IDLE state.
 *
 * callback implementation requirement
 *   (1) the callback is provided by client, it will be called and executed
 *       in sip_svc thread once transaction is completed.
 *   (2) callback is requested to do the following:
 *       - if the client is running with a thread, callback should ensure
 *         the thread is still alive before handle the response
 *       - response data pointer is not retained after the callback function.
 *         thus, the callback should copy the response data when needed.
 *       - callback responsible to free the asynchronous response data memory
 *         space
 *
 *
 * ***************************************
 * sip_svc service and client overview
 * ***************************************
 * ------------------------------------------------------
 *                 Client1     Client2     Client3 ...
 * Support            |           *           |
 * multiple           |           * open      |
 * clients            |           * channel   |
 *                    |           *           |
 * ------------------------------------------------------
 * sip_svc
 * service
 * Thread
 *                ----------
 *                | Create | when receive first request
 *                ----------
 *                     |
 *                     | Run
 *                     |
 *                -------------------
 *            --> | Request handler | Process the request, perform smc/hvc
 *            |   -------------------
 *            |        |
 *    Resume  |        |
 *    when    |        |
 *    receive |   --------------------------
 *    new     |   | Async response handler | Poll response of async request
 *    request |   -------------------------- perform smc/hvc
 *            |        |
 *            |        | Suspend when all transactions
 *            |        | completed without new request
 *            |        |
 *            |   ------------------
 *            --- | Suspend Thread |
 *                ------------------
 * ------------------------------------------------------
 *
 * ***************************************
 * sip_svc service ID management
 * ***************************************
 * ------------------------------------------------------
 * client         Client                    Client
 *                   |                         |
 *                   | Register                | Send
 *                   |                         | Request
 *                   V                         V
 * ------------------------------------------------------
 * sip_svc            ^                        ^
 * service            | Client Token           | Transaction ID
 *                    |                        |
 *          ---------------------   -----------------------
 *          |  Alloc an client  |   | Alloc a Transaction |
 *          |  placeholder and  |   | ID for the request  |
 *          | generate a unique |   -----------------------
 *          |   token for it    |              |
 *          ---------------------              |
 *                                             |
 *                                             | Transaction ID
 *                                             V
 * ------------------------------------------------------
 * EL2/EL3                                      ^
 * firmware                                     |
 *                                   Return same Transaction ID
 * ------------------------------------------------------
 *
 */

#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/sip_svc/sip_svc_controller.h>
#include <zephyr/drivers/sip_svc/sip_svc_driver.h>
#include <zephyr/sys/iterable_sections.h>
#include "sip_svc_id_mgr.h"
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sip_svc_subsys, CONFIG_ARM_SIP_SVC_SUBSYS_LOG_LEVEL);

static uint32_t sip_svc_generate_c_token(void)
{
	uint32_t c_token = k_cycle_get_32();
	return c_token;
}

static inline bool is_sip_svc_controller(void *ct)
{
	if (ct == NULL) {
		return false;
	}

	STRUCT_SECTION_FOREACH(sip_svc_controller, ctrl) {
		if ((void *)ctrl == ct) {
			return true;
		}
	}
	return false;
}

static uint32_t sip_svc_get_c_idx(struct sip_svc_controller *ctrl, uint32_t c_token)
{
	uint32_t i;

	if (!ctrl) {
		return SIP_SVC_ID_INVALID;
	}

	for (i = 0; i < ctrl->num_clients; i++) {
		if (ctrl->clients[i].token == c_token) {
			return i;
		}
	}

	return SIP_SVC_ID_INVALID;
}

uint32_t sip_svc_register(void *ct, void *priv_data)
{
	int err;
	uint32_t c_idx = SIP_SVC_ID_INVALID;

	if (ct == NULL || !is_sip_svc_controller(ct)) {
		return SIP_SVC_ID_INVALID;
	}

	struct sip_svc_controller *ctrl = (struct sip_svc_controller *)ct;

	err = k_mutex_lock(&ctrl->data_mutex, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Error in acquiring mutex %d", err);
		return SIP_SVC_ID_INVALID;
	}

	c_idx = sip_svc_id_mgr_alloc(ctrl->client_id_pool);
	if (c_idx != SIP_SVC_ID_INVALID) {
		ctrl->clients[c_idx].id = c_idx;
		ctrl->clients[c_idx].token = sip_svc_generate_c_token();
		ctrl->clients[c_idx].state = SIP_SVC_CLIENT_ST_IDLE;
		ctrl->clients[c_idx].priv_data = priv_data;
		k_mutex_unlock(&ctrl->data_mutex);
		LOG_INF("Register the client channel 0x%x", ctrl->clients[c_idx].token);
		return ctrl->clients[c_idx].token;
	}

	k_mutex_unlock(&ctrl->data_mutex);
	return SIP_SVC_ID_INVALID;
}

int sip_svc_unregister(void *ct, uint32_t c_token)
{
	int err;
	uint32_t c_idx;

	if (ct == NULL || !is_sip_svc_controller(ct)) {
		return -EINVAL;
	}

	struct sip_svc_controller *ctrl = (struct sip_svc_controller *)ct;

	err = k_mutex_lock(&ctrl->data_mutex, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Error in acquiring mutex %d", err);
		return -ENOLCK;
	}

	c_idx = sip_svc_get_c_idx(ctrl, c_token);
	if (c_idx == SIP_SVC_ID_INVALID) {
		k_mutex_unlock(&ctrl->data_mutex);
		return -EINVAL;
	}

	if (ctrl->clients[c_idx].id == SIP_SVC_ID_INVALID) {
		k_mutex_unlock(&ctrl->data_mutex);
		return -ENODATA;
	}

	if (ctrl->clients[c_idx].active_trans_cnt != 0) {
		k_mutex_unlock(&ctrl->data_mutex);
		return -EBUSY;
	}

	if (ctrl->clients[c_idx].state != SIP_SVC_CLIENT_ST_IDLE) {
		k_mutex_unlock(&ctrl->data_mutex);
		return -ECANCELED;
	}

	LOG_INF("Unregister the client channel 0x%x", ctrl->clients[c_idx].token);
	ctrl->clients[c_idx].id = SIP_SVC_ID_INVALID;
	ctrl->clients[c_idx].state = SIP_SVC_CLIENT_ST_INVALID;
	ctrl->clients[c_idx].token = SIP_SVC_ID_INVALID;
	ctrl->clients[c_idx].priv_data = NULL;
	sip_svc_id_mgr_free(ctrl->client_id_pool, c_idx);

	k_mutex_unlock(&ctrl->data_mutex);
	return 0;
}

static bool get_timer_status(bool *first_iteration, struct k_timer *timer, k_timeout_t duration)
{
	if (first_iteration == NULL || timer == NULL) {
		return false;
	}

	if (!(*first_iteration)) {
		/* start the timer using the timeout variable provided and return true*/
		k_timer_start(timer, duration, K_NO_WAIT);
		*first_iteration = true;
		return true;
	} else if (K_TIMEOUT_EQ(duration, K_NO_WAIT)) {
		/* here we will be at second iteration if duration is K_NO_WAIT, return false */
		return false;
	} else if (K_TIMEOUT_EQ(duration, K_FOREVER)) {
		/* k_timer won't start for K_FOREVER, so return true*/
		return true;
	} else if (k_timer_remaining_get(timer) > 0) {
		/* return true if timer has not expired */
		return true;
	}

	return false;
}

int sip_svc_open(void *ct, uint32_t c_token, k_timeout_t k_timeout)
{

	uint32_t c_idx;
	int ret;
	struct k_timer timer;

	if (ct == NULL || !is_sip_svc_controller(ct)) {
		return -EINVAL;
	}

	struct sip_svc_controller *ctrl = (struct sip_svc_controller *)ct;

	/* Initialize the timer */
	k_timer_init(&timer, NULL, NULL);

	/**
	 * Run through the loop until the client is in IDLE state.
	 * Then move the client state to open. If the client has any pending transactions,
	 * the client state will be ABORT state.This will only change when the pending
	 * transactions are complete.
	 */
	for (bool first_iteration = false; get_timer_status(&first_iteration, &timer, k_timeout);
	     k_usleep(CONFIG_ARM_SIP_SVC_SUBSYS_ASYNC_POLLING_DELAY)) {

		ret = k_mutex_lock(&ctrl->data_mutex, K_NO_WAIT);
		if (ret != 0) {
			LOG_WRN("0x%x didn't get data lock", c_token);
			continue;
		}

		c_idx = sip_svc_get_c_idx(ctrl, c_token);
		if (c_idx == SIP_SVC_ID_INVALID) {
			LOG_ERR("Invalid client token");
			k_mutex_unlock(&ctrl->data_mutex);
			k_timer_stop(&timer);
			return -EINVAL;
		}

		/* Check if the state of client is already open state*/
		if (ctrl->clients[c_idx].state == SIP_SVC_CLIENT_ST_OPEN) {
			LOG_DBG("client with token 0x%x is already open", c_token);
			k_mutex_unlock(&ctrl->data_mutex);
			k_timer_stop(&timer);
			return -EALREADY;
		}

		/* Check if the state of client is in idle state*/
		if (ctrl->clients[c_idx].state != SIP_SVC_CLIENT_ST_IDLE) {
			LOG_DBG("client with token 0x%x is not idle", c_token);
			k_mutex_unlock(&ctrl->data_mutex);
			continue;
		}

#if CONFIG_ARM_SIP_SVC_SUBSYS_SINGLY_OPEN
		/**
		 * Acquire open lock, when only one client can transact at
		 * a time.
		 */
		if (!atomic_cas(&ctrl->open_lock, SIP_SVC_OPEN_UNLOCKED, SIP_SVC_OPEN_LOCKED)) {
			LOG_DBG("0x%x didn't get open lock, wait for it to be released", c_token);
			k_mutex_unlock(&ctrl->data_mutex);
			continue;
		}
#endif

		/* Make the client state to be open and stop timer*/
		ctrl->clients[c_idx].state = SIP_SVC_CLIENT_ST_OPEN;
		LOG_INF("0x%x successfully opened a connection with sip_svc", c_token);
		k_mutex_unlock(&ctrl->data_mutex);
		k_timer_stop(&timer);
		return 0;
	}

	k_timer_stop(&timer);
	LOG_ERR("Timedout at %s for 0x%x", __func__, c_token);
	return -ETIMEDOUT;
}

int sip_svc_close(void *ct, uint32_t c_token, struct sip_svc_request *pre_close_req)
{
	uint32_t c_idx;
	int err;

	if (ct == NULL || !is_sip_svc_controller(ct)) {
		return -EINVAL;
	}

	struct sip_svc_controller *ctrl = (struct sip_svc_controller *)ct;

	/*If pre-close request is provided, send it to lower layers*/
	if (pre_close_req != NULL) {
		err = sip_svc_send(ct, c_token, pre_close_req, NULL);
		if (err < 0) {
			LOG_ERR("Error sending pre_close_req : %d", err);
			return -ENOTSUP;
		}
	}

	err = k_mutex_lock(&ctrl->data_mutex, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Error in acquiring lock %d", err);
		return err;
	}

	c_idx = sip_svc_get_c_idx(ctrl, c_token);
	if (c_idx == SIP_SVC_ID_INVALID) {
		k_mutex_unlock(&ctrl->data_mutex);
		return -EINVAL;
	}

	if (ctrl->clients[c_idx].state != SIP_SVC_CLIENT_ST_OPEN) {
		LOG_ERR("Client is in wrong state  %d", ctrl->clients[c_idx].state);
		k_mutex_unlock(&ctrl->data_mutex);
		return -EPROTO;
	}

	if (ctrl->clients[c_idx].active_trans_cnt != 0) {
		ctrl->clients[c_idx].state = SIP_SVC_CLIENT_ST_ABORT;
	} else {
		ctrl->clients[c_idx].state = SIP_SVC_CLIENT_ST_IDLE;
	}

#if CONFIG_ARM_SIP_SVC_SUBSYS_SINGLY_OPEN
	(void)atomic_set(&ctrl->open_lock, SIP_SVC_OPEN_UNLOCKED);
#endif
	k_mutex_unlock(&ctrl->data_mutex);

	LOG_INF("Close the client channel 0x%x", ctrl->clients[c_idx].token);
	return 0;
}

static void sip_svc_callback(struct sip_svc_controller *ctrl, uint32_t trans_id,
			     struct sip_svc_response *response)
{
	struct sip_svc_id_map_item *trans_id_item;
	uint64_t data_addr;
	uint64_t c_idx;
	int err;

	if (!ctrl) {
		return;
	}

	LOG_INF("Got response for trans id 0x%x", trans_id);

	err = k_mutex_lock(&ctrl->data_mutex, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Failed to get lock,%d", err);
		return;
	}

	/* Get trans id callback context from map */
	trans_id_item = sip_svc_id_map_query_item(ctrl->trans_id_map, trans_id);

	if (!trans_id_item) {
		LOG_ERR("Failed to get the entry from database");
		k_mutex_unlock(&ctrl->data_mutex);
		return;
	}

	c_idx = (uint64_t)trans_id_item->arg6;
	__ASSERT(c_idx < ctrl->num_clients, "c_idx shouldn't be greater than ctrl->num_clients");

	__ASSERT(ctrl->clients[c_idx].active_trans_cnt != 0,
		 "At this stage active_trans_cnt shouldn't be 0");
	--ctrl->clients[c_idx].active_trans_cnt;

	if (ctrl->clients[c_idx].state == SIP_SVC_CLIENT_ST_OPEN && trans_id_item->arg1) {

		((sip_svc_cb_fn)(trans_id_item->arg1))(ctrl->clients[c_idx].token, response);
	} else {
		LOG_INF("Resp data is released as the client channel is closed");
		/* Free response memory space if callback is skipped.*/
		data_addr =
			(((uint64_t)trans_id_item->arg2) << 32) | ((uint64_t)trans_id_item->arg3);

		if (data_addr) {
			k_free((char *)data_addr);
		}
	}

	/* Free trans id */
	sip_svc_id_map_remove_item(ctrl->trans_id_map, trans_id);
	sip_svc_id_mgr_free(ctrl->clients[c_idx].trans_idx_pool,
			    sip_svc_plat_get_trans_idx(ctrl->dev, trans_id));

	if (ctrl->clients[c_idx].active_trans_cnt != 0) {
		k_mutex_unlock(&ctrl->data_mutex);
		return;
	}

	if (ctrl->clients[c_idx].state == SIP_SVC_CLIENT_ST_ABORT) {
		ctrl->clients[c_idx].state = SIP_SVC_CLIENT_ST_IDLE;
	}

	k_mutex_unlock(&ctrl->data_mutex);
}

static int sip_svc_request_handler(struct sip_svc_controller *ctrl)
{
	struct arm_smccc_res res;
	struct sip_svc_request request;
	struct sip_svc_response response;
	uint32_t trans_id;
	uint32_t cmd_code;
	uint32_t error_code;

	if (!ctrl) {
		LOG_ERR("Error ctrl is NULL");
		return -EINVAL;
	}

	/**
	 * If transaction's are more than ctrl->max_transactions,
	 * return -EBUSY.
	 */
	if (ctrl->active_job_cnt >= ctrl->max_transactions) {
		return -EBUSY;
	}

	if (k_msgq_num_used_get(&ctrl->req_msgq) == 0) {
		return 0;
	}

	if (k_msgq_get(&ctrl->req_msgq, &request, K_NO_WAIT) != 0) {
		return -EAGAIN;
	}

	/* Get command code from request header */
	cmd_code = SIP_SVC_PROTO_HEADER_GET_CODE(request.header);

	/* Get trans_id from request header */
	trans_id = SIP_SVC_PROTO_HEADER_GET_TRANS_ID(request.header);

	/* Process the request, trigger smc/hvc call */
	if (cmd_code == SIP_SVC_PROTO_CMD_ASYNC) {
		sip_svc_plat_update_trans_id(ctrl->dev, &request, trans_id);
	}

	/* Increase active job count. Job means communication with
	 * secure monitor firmware
	 */
	++ctrl->active_job_cnt;

	/* Trigger smc call */
	LOG_INF("%s : triggering %s call", __func__, ctrl->method);
	LOG_DBG("\theader         %08x", request.header);
	LOG_DBG("\tresp_data_addr %08llx", request.resp_data_addr);
	LOG_DBG("\tresp_data_size %d", request.resp_data_size);
	LOG_DBG("\tpriv_data      %p", request.priv_data);

	sip_supervisory_call(ctrl->dev, request.a0, request.a1, request.a2, request.a3, request.a4,
			     request.a5, request.a6, request.a7, &res);

	/* Release async command data dynamic memory */
	if (cmd_code == SIP_SVC_PROTO_CMD_ASYNC) {
		sip_svc_plat_free_async_memory(ctrl->dev, &request);
	}

	/* Callback if fail or sync command */
	error_code = sip_svc_plat_get_error_code(ctrl->dev, &res);
	if (error_code != 0 || cmd_code == SIP_SVC_PROTO_CMD_SYNC) {
		response.header = SIP_SVC_PROTO_HEADER(error_code, trans_id);
		response.a0 = res.a0;
		response.a1 = res.a1;
		response.a2 = res.a2;
		response.a3 = res.a3;
		response.resp_data_addr = request.resp_data_addr;
		response.resp_data_size = request.resp_data_size;
		response.priv_data = request.priv_data;

		sip_svc_callback(ctrl, trans_id, &response);

		__ASSERT(ctrl->active_job_cnt != 0, "ctrl->active_job_cnt cannot be zero here");
		--ctrl->active_job_cnt;
	} else {
		++ctrl->active_async_job_cnt;
	}

	return -EINPROGRESS;
}

static int sip_svc_async_response_handler(struct sip_svc_controller *ctrl)
{
	struct sip_svc_id_map_item *trans_id_item;
	struct sip_svc_response response;
	uint32_t trans_id;
	uint64_t data_addr;
	size_t data_size;
	int ret;

	unsigned long a0 = 0;
	unsigned long a1 = 0;
	unsigned long a2 = 0;
	unsigned long a3 = 0;
	unsigned long a4 = 0;
	unsigned long a5 = 0;
	unsigned long a6 = 0;
	unsigned long a7 = 0;
	struct arm_smccc_res res;

	if (!ctrl) {
		LOG_ERR("controller is NULL");
		return -EINVAL;
	}

	/* Return if no busy job id */
	if (ctrl->active_async_job_cnt == 0) {
		LOG_INF("Async resp job queue is empty");
		return 0;
	}

	if (sip_svc_plat_async_res_req(ctrl->dev, &a0, &a1, &a2, &a3, &a4, &a5, &a6, &a7,
				       ctrl->async_resp_data, ctrl->resp_size)) {
		LOG_ERR("Error during creation of ASYNC polling request");
		return -ENOTSUP;
	}

	/* Trigger SMC call */
	LOG_INF("%s : triggering %s call", __func__, ctrl->method);
	LOG_DBG("%s (polling async response)", ctrl->method);

	sip_supervisory_call(ctrl->dev, a0, a1, a2, a3, a4, a5, a6, a7, &res);

	/* Callback if get response */
	ret = sip_svc_plat_async_res_res(ctrl->dev, &res, ctrl->async_resp_data, &data_size,
					 &trans_id);

	if (ret != 0) {
		return -EINPROGRESS;
	}

	/* get caller information based on trans id */
	trans_id_item = sip_svc_id_map_query_item(ctrl->trans_id_map, trans_id);

	if (!trans_id_item) {
		LOG_ERR("Failed to get entry from database");
		return -ENOENT;
	}

	/* Get caller provided memory space to put response */
	data_addr = (((uint64_t)trans_id_item->arg3) | ((uint64_t)trans_id_item->arg2) << 32);

	/* Check caller provided memory space to avoid overflow */
	if (data_size > ((size_t)trans_id_item->arg4)) {
		data_size = ((size_t)trans_id_item->arg4);
	}

	response.header =
		SIP_SVC_PROTO_HEADER(sip_svc_plat_get_error_code(ctrl->dev, &res), trans_id);
	response.a0 = res.a0;
	response.a1 = res.a1;
	response.a2 = res.a2;
	response.a3 = res.a3;
	response.resp_data_addr = data_addr;
	response.resp_data_size = data_size;
	response.priv_data = trans_id_item->arg5;

	/* Copy async cmd response into caller given memory space */
	if (data_addr) {
		memcpy((char *)data_addr, ctrl->async_resp_data, data_size);
	}

	sip_svc_callback(ctrl, trans_id, &response);

	__ASSERT(ctrl->active_job_cnt, "ctrl->active_job_cnt cannot be zero here");
	--ctrl->active_job_cnt;

	__ASSERT(ctrl->active_async_job_cnt != 0, "ctrl->active_async_job_cnt cannot be zero here");
	--ctrl->active_async_job_cnt;

	/* Check again is there any async busy job id */
	if (ctrl->active_async_job_cnt == 0) {
		LOG_INF("Async resp job queue is serviced");
		return 0;
	}

	return -EINPROGRESS;
}

static void sip_svc_thread(void *ctrl_ptr, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct sip_svc_controller *ctrl = (struct sip_svc_controller *)ctrl_ptr;
	int ret_msgq;
	int ret_resp;

	while (1) {
		ret_msgq = -EINPROGRESS;
		ret_resp = -EINPROGRESS;
		while (ret_msgq != 0 || ret_resp != 0) {
			ret_resp = sip_svc_async_response_handler(ctrl);
			ret_msgq = sip_svc_request_handler(ctrl);

			/* sleep only when waiting for ASYNC responses*/
			if (ret_msgq == 0 && ret_resp != 0) {
				k_usleep(CONFIG_ARM_SIP_SVC_SUBSYS_ASYNC_POLLING_DELAY);
			}
		}
		LOG_INF("Suspend thread, all transactions are completed");
		k_thread_suspend(ctrl->tid);
	}
}

int sip_svc_send(void *ct, uint32_t c_token, struct sip_svc_request *request, sip_svc_cb_fn cb)
{
	uint32_t trans_id = SIP_SVC_ID_INVALID;
	uint32_t trans_idx = SIP_SVC_ID_INVALID;
	uint32_t c_idx;
	int ret;

	if (ct == NULL || !is_sip_svc_controller(ct) || request == NULL) {
		return -EINVAL;
	}

	struct sip_svc_controller *ctrl = (struct sip_svc_controller *)ct;

	if (!sip_svc_plat_func_id_valid(ctrl->dev,
					(uint32_t)SIP_SVC_PROTO_HEADER_GET_CODE(request->header),
					(uint32_t)request->a0)) {
		return -EOPNOTSUPP;
	}

	ret = k_mutex_lock(&ctrl->data_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to get lock %d", ret);
		return -ENOLCK;
	}

	c_idx = sip_svc_get_c_idx(ctrl, c_token);
	if (c_idx == SIP_SVC_ID_INVALID) {
		k_mutex_unlock(&ctrl->data_mutex);
		return -EINVAL;
	}

	if (ctrl->clients[c_idx].state != SIP_SVC_CLIENT_ST_OPEN) {
		k_mutex_unlock(&ctrl->data_mutex);
		return -ESRCH;
	}

	/* Allocate a trans id for the request */
	trans_idx = sip_svc_id_mgr_alloc(ctrl->clients[c_idx].trans_idx_pool);
	if (trans_idx == SIP_SVC_ID_INVALID) {
		LOG_ERR("Fail to allocate transaction id");
		k_mutex_unlock(&ctrl->data_mutex);
		return -ENOMEM;
	}

	trans_id = sip_svc_plat_format_trans_id(ctrl->dev, c_idx, trans_idx);
	/* Additional check for an unsupported condition*/
	if (((int)trans_id) < 0) {
		LOG_ERR("Unsupported condition, trans_id < 0");
		sip_svc_id_mgr_free(ctrl->clients[c_idx].trans_idx_pool, trans_idx);
		k_mutex_unlock(&ctrl->data_mutex);
		return -ENOTSUP;
	}

	/* Assign the trans id of this request */
	SIP_SVC_PROTO_HEADER_SET_TRANS_ID(request->header, trans_id);

	/* Map trans id to client, callback, response data addr */
	if (sip_svc_id_map_insert_item(ctrl->trans_id_map, trans_id, (void *)cb,
				       (void *)((request->resp_data_addr >> 32) & 0xFFFFFFFF),
				       (void *)(request->resp_data_addr & 0xFFFFFFFF),
				       (void *)(uint64_t)request->resp_data_size,
				       request->priv_data, (void *)(uint64_t)c_idx) != 0) {

		LOG_ERR("Fail to insert transaction id to map");
		sip_svc_id_mgr_free(ctrl->clients[c_idx].trans_idx_pool, trans_idx);
		k_mutex_unlock(&ctrl->data_mutex);
		return -ENOMSG;
	}

	/* Insert request to MSGQ */
	LOG_INF("send command to msgq");
	if (k_msgq_put(&ctrl->req_msgq, (void *)request, K_NO_WAIT) != 0) {
		LOG_ERR("Request msgq full");
		sip_svc_id_map_remove_item(ctrl->trans_id_map, trans_id);
		sip_svc_id_mgr_free(ctrl->clients[c_idx].trans_idx_pool, trans_idx);
		k_mutex_unlock(&ctrl->data_mutex);
		return -ENOBUFS;
	}
	++ctrl->clients[c_idx].active_trans_cnt;

	if (!ctrl->tid) {
		LOG_ERR("Thread not spawned during init");
		sip_svc_id_map_remove_item(ctrl->trans_id_map, trans_id);
		sip_svc_id_mgr_free(ctrl->clients[c_idx].trans_idx_pool, trans_idx);
		k_mutex_unlock(&ctrl->data_mutex);
		return -EHOSTDOWN;
	}

	LOG_INF("Wakeup sip_svc thread");
	k_thread_resume(ctrl->tid);
	k_mutex_unlock(&ctrl->data_mutex);

	return (int)trans_id;
}

void *sip_svc_get_priv_data(void *ct, uint32_t c_token)
{
	uint32_t c_idx;
	int err;

	if (ct == NULL || !is_sip_svc_controller(ct)) {
		return NULL;
	}

	struct sip_svc_controller *ctrl = (struct sip_svc_controller *)ct;

	err = k_mutex_lock(&ctrl->data_mutex, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Failed to get lock %d", err);
		return NULL;
	}

	c_idx = sip_svc_get_c_idx(ctrl, c_token);
	if (c_idx == SIP_SVC_ID_INVALID) {
		LOG_ERR("Client id is invalid");
		k_mutex_unlock(&ctrl->data_mutex);
		return NULL;
	}

	k_mutex_unlock(&ctrl->data_mutex);
	return ctrl->clients[c_idx].priv_data;
}

void *sip_svc_get_controller(char *method)
{
	if (method == NULL) {
		LOG_ERR("controller is NULL");
		return NULL;
	}

	/**
	 * For more info on below code check @ref SIP_SVC_CONTROLLER_DEFINE()
	 */
	STRUCT_SECTION_FOREACH(sip_svc_controller, ctrl) {
		if (!strncmp(ctrl->method, method, SIP_SVC_SUBSYS_CONDUIT_NAME_LENGTH)) {
			return (void *)ctrl;
		}
	}

	LOG_ERR("controller couldn't be found");
	return NULL;
}

static int sip_svc_subsys_init(void)
{
	int ret = 0;
	uint32_t ctrl_count = 0;
	char *msgq_buf = NULL;
	struct device *dev = NULL;
	struct sip_svc_client *client = NULL;

	LOG_INF("Start of %s", __func__);

	STRUCT_SECTION_COUNT(sip_svc_controller, &ctrl_count);
	__ASSERT(ctrl_count <= 2, "There should be at most 2 controllers");

	/**
	 * Get controller array ,Controller which is instantiated by driver using
	 * SIP_SVC_CONTROLLER_DEFINE(),see @ref SIP_SVC_CONTROLLER_DEFINE() for more
	 * info.
	 */
	STRUCT_SECTION_FOREACH(sip_svc_controller, ctrl) {
		if (!device_is_ready(ctrl->dev)) {
			LOG_ERR("device not ready");
			return -ENODEV;
		}
		dev = (struct device *)(ctrl->dev);

		LOG_INF("Got registered conduit %.*s", (int)sizeof(ctrl->method), ctrl->method);

		ctrl->async_resp_data = k_malloc(ctrl->resp_size);
		if (ctrl->async_resp_data == NULL) {
			return -ENOMEM;
		}

		ctrl->client_id_pool = sip_svc_id_mgr_create(ctrl->num_clients);
		if (!ctrl->client_id_pool) {
			k_free(ctrl->async_resp_data);
			return -ENOMEM;
		}

		ctrl->trans_id_map = sip_svc_id_map_create(ctrl->max_transactions);
		if (!ctrl->trans_id_map) {
			sip_svc_id_mgr_delete(ctrl->client_id_pool);
			k_free(ctrl->async_resp_data);
			return -ENOMEM;
		}

		/* Alloc request msgq ring buffer */
		msgq_buf = k_malloc(sizeof(struct sip_svc_request) *
				    CONFIG_ARM_SIP_SVC_SUBSYS_MSGQ_DEPTH);
		if (!msgq_buf) {
			sip_svc_id_mgr_delete(ctrl->client_id_pool);
			sip_svc_id_map_delete(ctrl->trans_id_map);
			k_free(ctrl->async_resp_data);
			return -ENOMEM;
		}

		ctrl->clients = k_malloc(ctrl->num_clients * sizeof(struct sip_svc_client));
		if (ctrl->clients == NULL) {
			sip_svc_id_mgr_delete(ctrl->client_id_pool);
			sip_svc_id_map_delete(ctrl->trans_id_map);
			k_free(msgq_buf);
			k_free(ctrl->async_resp_data);
			return -ENOMEM;
		}

		memset(ctrl->clients, 0, ctrl->num_clients * sizeof(struct sip_svc_client));

		/* Initialize request msgq */
		k_msgq_init(&ctrl->req_msgq, msgq_buf, sizeof(struct sip_svc_request),
			    CONFIG_ARM_SIP_SVC_SUBSYS_MSGQ_DEPTH);

		/* Initialize client contents */
		for (uint32_t i = 0; i < ctrl->num_clients; i++) {
			client = &ctrl->clients[i];
			client->id = SIP_SVC_ID_INVALID;
			client->token = SIP_SVC_ID_INVALID;
			client->state = SIP_SVC_CLIENT_ST_INVALID;
			client->active_trans_cnt = 0;

			client->trans_idx_pool = sip_svc_id_mgr_create(
				CONFIG_ARM_SIP_SVC_SUBSYS_MAX_TRANSACTION_ID_COUNT);
			if (!client->trans_idx_pool) {
				ret = -ENOMEM;
				break;
			}
		}

		if (ret != 0) {
			sip_svc_id_mgr_delete(ctrl->client_id_pool);
			sip_svc_id_map_delete(ctrl->trans_id_map);
			k_free(msgq_buf);
			k_free(ctrl->clients);
			k_free(ctrl->async_resp_data);

			for (uint32_t i = 0; i < ctrl->num_clients; i++) {
				client = &ctrl->clients[i];
				if (client->trans_idx_pool) {
					sip_svc_id_mgr_delete(client->trans_idx_pool);
				}
			}
			return ret;
		}

		/* Create and run the thread */
		ctrl->tid = k_thread_create(
			&ctrl->thread, ctrl->stack, CONFIG_ARM_SIP_SVC_SUBSYS_THREAD_STACK_SIZE,
			sip_svc_thread, ctrl, NULL, NULL, CONFIG_ARM_SIP_SVC_SUBSYS_THREAD_PRIORITY,
			K_ESSENTIAL, K_NO_WAIT);
		k_thread_name_set(ctrl->tid, "sip_svc");

		ctrl->active_job_cnt = 0;
		ctrl->active_async_job_cnt = 0;

		/* Initialize atomic variable */
#if CONFIG_ARM_SIP_SVC_SUBSYS_SINGLY_OPEN
		(void)atomic_set(&ctrl->open_lock, SIP_SVC_OPEN_UNLOCKED);
#endif
		/* Initialize mutex */
		k_mutex_init(&ctrl->data_mutex);

		ctrl->init = true;
	}

	LOG_INF("Completed %s", __func__);
	return 0;
}

SYS_INIT(sip_svc_subsys_init, POST_KERNEL, CONFIG_ARM_SIP_SVC_SUBSYS_INIT_PRIORITY);
