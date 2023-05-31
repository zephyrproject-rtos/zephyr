/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Intel SoC FPGA platform specific functions used by FPGA bridges.
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "fpga_bridge_intel.h"

LOG_MODULE_REGISTER(fpga_bridge, CONFIG_FPGA_BRIDGE_LOG_LEVEL);

#define MAX_TIMEOUT_MSECS (1 * 1000UL)

static uint32_t mailbox_client_token;
static struct sip_svc_controller *mailbox_smc_dev;

/**
 * @brief Open SiP SVC client session
 *
 * @return 0 on success or negative value on failure
 */
static int32_t svc_client_open(void)
{
	if ((!mailbox_smc_dev) || (mailbox_client_token == 0)) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	if (sip_svc_open(mailbox_smc_dev, mailbox_client_token, K_MSEC(MAX_TIMEOUT_MSECS))) {
		LOG_ERR("Mailbox client open fail");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Initialize the SiP SVC client
 * It will get the controller and register the client.
 *
 * @return 0 on success or negative value on fail
 */
static int32_t fpga_bridge_init(void)
{
	mailbox_smc_dev = sip_svc_get_controller("smc");
	if (!mailbox_smc_dev) {
		LOG_ERR("Arm SiP service not found");
		return -ENODEV;
	}

	mailbox_client_token = sip_svc_register(mailbox_smc_dev, NULL);
	if (mailbox_client_token == SIP_SVC_ID_INVALID) {
		mailbox_smc_dev = NULL;
		LOG_ERR("Mailbox client register fail");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Close the svc client
 *
 * @return 0 on success or negative value on fail
 */
static int32_t svc_client_close(void)
{
	int32_t err;
	uint32_t cmd_size = sizeof(uint32_t);
	struct sip_svc_request request;

	uint32_t *cmd_addr = (uint32_t *)k_malloc(cmd_size);

	if (!cmd_addr) {
		return -ENOMEM;
	}

	/* Fill the SiP SVC buffer with CANCEL request */
	*cmd_addr = MAILBOX_CANCEL_COMMAND;

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
	request.a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
	request.a1 = 0;
	request.a2 = (uint64_t)cmd_addr;
	request.a3 = (uint64_t)cmd_size;
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;
	request.resp_data_addr = (uint64_t)NULL;
	request.resp_data_size = 0;
	request.priv_data = NULL;

	err = sip_svc_close(mailbox_smc_dev, mailbox_client_token, &request);
	if (err) {
		k_free(cmd_addr);
		LOG_ERR("Mailbox client close fail (%d)", err);
	}

	return err;
}

/**
 * @brief Call back function which we receive when we send the data
 * based on the current stage it will collect the data
 *
 * @param[in] c_token Token id for our svc services
 * @param[in] response Buffer will contain the response
 *
 * @return void
 */
static void smc_callback(uint32_t c_token, struct sip_svc_response *response)
{
	if (response == NULL) {
		return;
	}

	struct private_data_t *private_data = (struct private_data_t *)response->priv_data;

	LOG_DBG("SiP SVC callback");

	LOG_DBG("\tresponse data below:");
	LOG_DBG("\theader=%08x", response->header);
	LOG_DBG("\ta0=%016lx", response->a0);
	LOG_DBG("\ta1=%016lx", response->a1);
	LOG_DBG("\ta2=%016lx", response->a2);
	LOG_DBG("\ta3=%016lx", response->a3);

	private_data->response.header = response->header;
	private_data->response.a0 = response->a0;
	private_data->response.a1 = response->a1;
	private_data->response.a2 = response->a2;
	private_data->response.a3 = response->a3;
	private_data->response.resp_data_size = response->resp_data_size;

	k_sem_give(&(private_data->smc_sem));
}

/**
 * @brief Send the data to sip_svc service layer
 *	based on the cmd_type further data will be send to SDM using mailbox
 *
 * @param[in] cmd_type Command type (Mailbox or Non-Mailbox)
 * @param[in] function_identifier Function identifier for each command type
 * @param[in] cmd_request
 * @param[in] private_data
 *
 * @return 0 on success or negative value on fail
 */
static int32_t smc_send(uint32_t cmd_type, uint64_t function_identifier, uint32_t *cmd_request,
			struct private_data_t *private_data)
{
	int32_t trans_id = 0;
	struct sip_svc_request request;

	if (!mailbox_smc_dev) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	/* Fill SiP SVC request buffer */
	request.header = SIP_SVC_PROTO_HEADER(cmd_type, 0);
	request.a0 = function_identifier;
	request.a1 = 0;
	request.a2 = cmd_request[SMC_REQUEST_A2_INDEX];
	request.a3 = cmd_request[SMC_REQUEST_A3_INDEX];
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;
	request.resp_data_addr = 0;
	request.resp_data_size = 0;
	request.priv_data = (void *)private_data;

	/* Send SiP SVC request */
	trans_id = sip_svc_send(mailbox_smc_dev, mailbox_client_token, &request, smc_callback);

	if (trans_id == SIP_SVC_ID_INVALID) {
		LOG_ERR("SiP SVC send request fail");
		return -EBUSY;
	}

	return 0;
}

static int32_t socfpga_bridges_reset(uint32_t enable, uint32_t mask)
{
	uint32_t smc_cmd[2] = {0};
	int32_t ret = 0;
	struct private_data_t priv_data;

	/* Initialize the semaphore */
	k_sem_init(&(priv_data.smc_sem), 0, 1);

	smc_cmd[SMC_REQUEST_A2_INDEX] = FIELD_GET(BIT(0), enable);

	if (mask != BRIDGE_MASK) {
		/* Set bit-1 to indicate mask value */
		smc_cmd[SMC_REQUEST_A2_INDEX] |= BIT(1);
		smc_cmd[SMC_REQUEST_A3_INDEX] = mask;
	}

	ret = smc_send(SIP_SVC_PROTO_CMD_SYNC, SMC_FUNC_ID_SET_HPS_BRIDGES, smc_cmd, &priv_data);
	if (ret) {
		LOG_ERR("Failed to send the smc Command !!");
		return ret;
	}

	/* Wait for the SiP SVC callback */
	k_sem_take(&(priv_data.smc_sem), K_FOREVER);

	/* Check error code */
	if (priv_data.response.a0 && priv_data.response.a0 == FPGA_CONFIG_NOT_DONE_ERROR) {
		ret = -EIO;
	} else if (priv_data.response.a0) {
		ret = -ENOMSG;
	}

	return ret;
}

int32_t do_bridge_reset_plat(uint32_t enable, uint32_t mask)
{
	int32_t ret = 0;

	if ((mask != ~0) && (mask > BRIDGE_MASK)) {
		LOG_ERR("Please provide mask in correct range");
		return -ENOTSUP;
	}

	/* Opening SIP SVC session */
	ret = svc_client_open();
	if (ret) {
		LOG_ERR("Client open Failed!");
		return ret;
	}

	/* Bridge reset */
	ret = socfpga_bridges_reset(enable, mask);
	if (ret) {
		LOG_ERR("Bridge reset failed");
	}

	/* Ignoring the return value to return bridge reset status */
	if (svc_client_close()) {
		LOG_ERR("Unregistering & Closing failed");
	}
	return ret;
}

SYS_INIT(fpga_bridge_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
