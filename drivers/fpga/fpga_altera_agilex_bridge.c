/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT altr_socfpga_agilex_bridge

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/logging/log.h>
#include "fpga_altera_agilex_bridge.h"

LOG_MODULE_REGISTER(fpga_altera, CONFIG_FPGA_LOG_LEVEL);

struct fpga_bridge_dev_data {
	/* SiP SVC controller */
	struct sip_svc_controller *mailbox_smc_dev;
	/* SiP SVC client token id */
	uint32_t mailbox_client_token;
};

#define MAX_TIMEOUT_MSECS (1 * 1000UL)

/**
 * @brief Open SiP SVC client session
 *
 * @return 0 on success or negative value on failure
 */
static int32_t svc_client_open(const struct device *dev)
{
	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	struct fpga_bridge_dev_data *const data = (struct fpga_bridge_dev_data *const)(dev->data);

	if ((!data->mailbox_smc_dev) || (data->mailbox_client_token == 0)) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	if (sip_svc_open(data->mailbox_smc_dev, data->mailbox_client_token,
			 K_MSEC(MAX_TIMEOUT_MSECS))) {
		LOG_ERR("Mailbox client open fail");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Close the svc client
 *
 * @return 0 on success or negative value on fail
 */
static int32_t svc_client_close(const struct device *dev)
{
	int32_t err;
	uint32_t cmd_size = sizeof(uint32_t);
	struct sip_svc_request request;

	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	struct fpga_bridge_dev_data *const data = (struct fpga_bridge_dev_data *const)(dev->data);

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

	err = sip_svc_close(data->mailbox_smc_dev, data->mailbox_client_token, &request);
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

	uint32_t *resp_data = NULL;
	uint32_t resp_len = 0;
	uint32_t mbox_idx = 0;

	struct sip_svc_private_data *private_data =
		(struct sip_svc_private_data *)response->priv_data;
	union mailbox_response_header response_header;

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

	/* Condition to check only for the mailbox command not for the non-mailbox command */
	if (response->resp_data_size) {
		resp_data = (uint32_t *)response->resp_data_addr;
		resp_len = response->resp_data_size / 4;
		private_data->mbox_response_len = resp_len;
		if (resp_data && resp_len) {
			response_header = (union mailbox_response_header)resp_data[0];
			private_data->mbox_response_data =
				(uint32_t *)k_malloc(sizeof(uint32_t) * resp_len);
			for (mbox_idx = 0; mbox_idx < resp_len; mbox_idx++) {
				LOG_DBG("\t\t[%4d] %08x", mbox_idx, resp_data[mbox_idx]);
				private_data->mbox_response_data[mbox_idx] = resp_data[mbox_idx];
			}
		} else {
			LOG_ERR("\t\tInvalid addr (%p) or len (%d)", resp_data, resp_len);
		}
	}
	/* Condition for non-mailbox command*/
	else {
		LOG_DBG("Response Data size is zero !!");
	}

	/* Client only responsible to free the response data memory space,
	 * the command data memory space had been freed by SiP SVC service.
	 */
	if (response->resp_data_addr) {
		LOG_DBG("\tFree response memory %p", (char *)response->resp_data_addr);
		k_free((char *)response->resp_data_addr);
	}

	k_sem_give(&(private_data->smc_sem));
}

/**
 * @brief Send the data to SiP SVC service layer
 *	based on the command type further data will be send to SDM using mailbox
 *
 * @param[in] cmd_type Command type (Mailbox or Non-Mailbox)
 * @param[in] function_identifier Function identifier for each command type
 * @param[in] cmd_request
 * @param[in] private_data
 *
 * @return 0 on success or negative value on fail
 */
static int32_t smc_send(const struct device *dev, uint32_t cmd_type, uint64_t function_identifier,
			uint32_t *cmd_request, struct sip_svc_private_data *private_data)
{
	int32_t trans_id = 0;
	uint32_t *cmd_addr = NULL;
	uint32_t *resp_addr = NULL;
	struct sip_svc_request request;

	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	struct fpga_bridge_dev_data *const data = (struct fpga_bridge_dev_data *const)(dev->data);

	if (!data->mailbox_smc_dev) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	if (cmd_type == SIP_SVC_PROTO_CMD_ASYNC) {
		cmd_addr = (uint32_t *)k_malloc(FPGA_MB_CMD_ADDR_MEM_SIZE);
		if (!cmd_addr) {
			LOG_ERR("Failed to allocate command memory");
			return -ENOMEM;
		}
		cmd_addr[MBOX_CMD_HEADER_INDEX] =
			MBOX_REQUEST_HEADER(cmd_request[SMC_REQUEST_A2_INDEX], 0, 0);
		resp_addr = (uint32_t *)k_malloc(FPGA_MB_RESPONSE_MEM_SIZE);
		if (!resp_addr) {
			k_free(cmd_addr);
			return -ENOMEM;
		}

		request.a2 = (uint64_t)cmd_addr;
		request.a3 = sizeof(uint32_t);
		request.resp_data_addr = (uint64_t)resp_addr;
		request.resp_data_size = FPGA_MB_RESPONSE_MEM_SIZE;

#if defined(CONFIG_LOG)
		for (int32_t mbox_idx = 0; mbox_idx < request.a3 / 4; mbox_idx++) {
			LOG_DBG("\t [%d ] %08x", mbox_idx, cmd_addr[mbox_idx]);
		}
#endif

	} else {
		request.a2 = cmd_request[SMC_REQUEST_A2_INDEX];
		request.a3 = cmd_request[SMC_REQUEST_A3_INDEX];
		request.resp_data_addr = 0;
		request.resp_data_size = 0;
	}

	/* Fill SiP SVC request buffer */
	request.header = SIP_SVC_PROTO_HEADER(cmd_type, 0);
	request.a0 = function_identifier;
	request.a1 = 0;
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;
	request.priv_data = (void *)private_data;

	/* Send SiP SVC request */
	trans_id = sip_svc_send(data->mailbox_smc_dev, data->mailbox_client_token, &request,
				smc_callback);

	if (trans_id == SIP_SVC_ID_INVALID) {
		LOG_ERR("SiP SVC send request fail");
		return -EBUSY;
	}

	return 0;
}

/* Validate the Reconfig status response */
static int32_t fpga_reconfig_status_validate(struct fpga_config_status *reconfig_status_resp)
{
	uint32_t ret = 0;

	/* Check for any error */
	ret = reconfig_status_resp->state;
	if (ret == MBOX_CFGSTAT_VAB_BS_PREAUTH) {
		return MBOX_CONFIG_STATUS_STATE_CONFIG;
	}

	if (ret && ret != MBOX_CONFIG_STATUS_STATE_CONFIG) {
		return ret;
	}

	/* Make sure nStatus is not 0 */
	ret = reconfig_status_resp->pin_status.pin_status;
	if (!(ret & RECONFIG_PIN_STATUS_NSTATUS)) {
		return MBOX_CFGSTAT_STATE_ERROR_HARDWARE;
	}

	ret = reconfig_status_resp->soft_function_status;
	if ((ret & RECONFIG_SOFTFUNC_STATUS_CONF_DONE) &&
	    (ret & RECONFIG_SOFTFUNC_STATUS_INIT_DONE) && !reconfig_status_resp->state) {
		return 0; /* Configuration success */
	}

	return MBOX_CONFIG_STATUS_STATE_CONFIG;
}

/* Will send the SMC command to check the status of the FPGA */
static int32_t fpga_config_ready_check(const struct device *dev)
{
	uint32_t smc_cmd[2] = {0};
	int32_t ret = 0;
	struct sip_svc_private_data priv_data;

	/* Initialize the semaphore */
	k_sem_init(&(priv_data.smc_sem), 0, 1);

	smc_cmd[SMC_REQUEST_A2_INDEX] = FPGA_CONFIG_STATUS;
	smc_cmd[SMC_REQUEST_A3_INDEX] = 0;

	/* Sending the FPGA config status mailbox command */
	ret = smc_send(dev, SIP_SVC_PROTO_CMD_ASYNC, SMC_FUNC_ID_MAILBOX_SEND_COMMAND, smc_cmd,
				&priv_data);
	if (ret) {
		LOG_ERR("Failed to Send the Mailbox Command !!");
		return -ECANCELED;
	}

	k_sem_take(&(priv_data.smc_sem), K_FOREVER);

	/* Verify the SMC response */
	if (!priv_data.response.resp_data_size &&
		priv_data.mbox_response_len != FPGA_CONFIG_STATUS_RESPONSE_LEN) {
		return -EINVAL;
	}

	/* Verify the FPGA config status response */
	ret = fpga_reconfig_status_validate(
		(struct fpga_config_status *)priv_data.mbox_response_data);

	k_free(priv_data.mbox_response_data);

	return ret;
}

static int32_t socfpga_bridges_reset(const struct device *dev, uint32_t enable)
{
	uint32_t smc_cmd[2] = {0};
	int ret = 0;
	struct sip_svc_private_data priv_data;

	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	/* Initialize the semaphore */
	k_sem_init(&(priv_data.smc_sem), 0, 1);

	smc_cmd[SMC_REQUEST_A2_INDEX] = FIELD_GET(BIT(0), enable);

	smc_cmd[SMC_REQUEST_A2_INDEX] |= BIT(1);
	smc_cmd[SMC_REQUEST_A3_INDEX] = BRIDGE_MASK;

	ret = smc_send(dev, SIP_SVC_PROTO_CMD_SYNC, SMC_FUNC_ID_SET_HPS_BRIDGES, smc_cmd,
				&priv_data);
	if (ret) {
		LOG_ERR("Failed to send the smc Command !!");
		return ret;
	}

	/* Wait for the SiP SVC callback */
	k_sem_take(&(priv_data.smc_sem), K_FOREVER);

	/* Check error code */
	if (priv_data.response.a0) {
		ret = -ENOMSG;
	}

	return ret;
}

static int altera_fpga_on(const struct device *dev)
{
	int32_t ret = 0;

	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	/* Opening SIP SVC session */
	ret = svc_client_open(dev);
	if (ret) {
		LOG_ERR("Client open Failed!");
		return ret;
	}

	/* Check FPGA status before bridge enable/disable */
	ret = fpga_config_ready_check(dev);
	if (ret) {
		LOG_ERR("FPGA not ready. Bridge reset aborted!");
		svc_client_close(dev);
		return -EIO;
	}

	/* Bridge reset */
	ret = socfpga_bridges_reset(dev, 0x01);
	if (ret) {
		LOG_ERR("Bridge reset failed");
	}

	/* Ignoring the return value to return bridge reset status */
	if (svc_client_close(dev)) {
		LOG_ERR("Unregistering & Closing failed");
	}

	return ret;
}

static int altera_fpga_off(const struct device *dev)
{
	int32_t ret = 0;

	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	/* Opening SIP SVC session */
	ret = svc_client_open(dev);
	if (ret) {
		LOG_ERR("Client open Failed!");
		return ret;
	}

	/* Check FPGA status before bridge enable/disable */
	ret = fpga_config_ready_check(dev);
	if (ret) {
		LOG_ERR("FPGA not ready. Bridge reset aborted!");
		svc_client_close(dev);
		return -EIO;
	}

	/* Bridge reset */
	ret = socfpga_bridges_reset(dev, 0x00);
	if (ret) {
		LOG_ERR("Bridge reset failed");
	}

	/* Ignoring the return value to return bridge reset status */
	if (svc_client_close(dev)) {
		LOG_ERR("Unregistering & Closing failed");
	}

	return ret;
}

static int altera_fpga_init(const struct device *dev)
{
	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	struct fpga_bridge_dev_data *const data = (struct fpga_bridge_dev_data *const)(dev->data);

	data->mailbox_smc_dev = sip_svc_get_controller("smc");
	if (!data->mailbox_smc_dev) {
		LOG_ERR("Arm SiP service not found");
		return -ENODEV;
	}

	data->mailbox_client_token = sip_svc_register(data->mailbox_smc_dev, NULL);
	if (data->mailbox_client_token == SIP_SVC_ID_INVALID) {
		data->mailbox_smc_dev = NULL;
		LOG_ERR("Mailbox client register fail");
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(fpga, altera_fpga_api) = {
	.on = altera_fpga_on,
	.off = altera_fpga_off,
};

#define CREATE_ALTERA_FPGA_BRIDGE_DEV(inst)						\
	static struct fpga_bridge_dev_data fpga_bridge_data_##inst;		\
	DEVICE_DT_INST_DEFINE(inst,						\
			altera_fpga_init,					\
			NULL, &fpga_bridge_data_##inst,				\
			NULL, POST_KERNEL,			\
			CONFIG_FPGA_INIT_PRIORITY,				\
			&altera_fpga_api);					\

DT_INST_FOREACH_STATUS_OKAY(CREATE_ALTERA_FPGA_BRIDGE_DEV);
