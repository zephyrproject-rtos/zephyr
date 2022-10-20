/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Intel SoC FPGA platform specific functions used by FPGA manager.
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include <zephyr/fpga_bridge/fpga_bridge.h>
#include "fpga_manager_intel.h"

LOG_MODULE_REGISTER(fpga_manager, CONFIG_FPGA_MANAGER_LOG_LEVEL);

#define DISABLE 0
#define ENABLE  1

#define BRIDGE_ALL 0xF

#define FPGA_MB_CMD_ADDR_MEM_SIZE 100
#define FPGA_MB_RESPONSE_MEM_SIZE 100
#define FPGA_MANAGER_THREAD_PRIORITY                                                               \
	((CONFIG_ARM_SIP_SVC_SUBSYS_THREAD_PRIORITY + 1) < (CONFIG_NUM_PREEMPT_PRIORITIES - 1))    \
		? (CONFIG_ARM_SIP_SVC_SUBSYS_THREAD_PRIORITY + 1)                                  \
		: CONFIG_ARM_SIP_SVC_SUBSYS_THREAD_PRIORITY

K_KERNEL_STACK_DEFINE(reconfig_thread_stack_area, CONFIG_FPGA_MANAGER_THREAD_STACK_SIZE);

static uint32_t mb_client_token;
static uint32_t curr_stage;
static uint32_t reconfig_progress;
static uint32_t fpga_reconfig_dma_count;
static uint32_t fpga_reconfig_dma_block_size;
static struct sip_svc_controller *mb_smc_dev;
static struct k_mutex config_state;
static struct k_thread config_thread_data;

/**
 * @file
 *
 * @brief Read the bitstream from the file and
 *	copy at bitstream pointing address
 *
 * @param[in] filename Name of the bitstream file
 * @param[in] bitstream Address where we copy bitstream data
 *
 * @return Bitstream file size on success or negative value on fail
 */
static int32_t copy_bitstream_at_loc(const char *filename, char *bitstream,
				     uint32_t fpga_config_max_size)
{
	int32_t res = 0;
	uint32_t bitstream_file_size = 0;
	uint32_t img_size = 0;
	struct fs_dirent entry;
	struct fs_file_t fileptr;

	/* Verify fs_stat() */
	res = fs_stat(filename, &entry);
	if (res) {
		LOG_ERR("Fail to open the file");
		return -ENOENT;
	}

	if (entry.size > fpga_config_max_size) {
		return -EFBIG;
	}

	img_size = entry.size;

	/* Init file pointer */
	fs_file_t_init(&fileptr);

	/* Verify fs_open() */
	res = fs_open(&fileptr, filename, FS_O_RDWR);
	if (res) {
		LOG_ERR("Failed opening file [%d]\n", res);
		return -ENOENT;
	}

	LOG_INF("Opened file %s\n", filename);

	/* Verify fs_read() */
	res = fs_read(&fileptr, (void *)bitstream, img_size);
	if (res < 0) {
		LOG_ERR("Failed reading file [%d]\n", res);
		fs_close(&fileptr);
		return res;
	}

	bitstream_file_size = res;

	res = fs_close(&fileptr);
	if (res) {
		LOG_ERR("Error closing file [%d]\n", res);
		return res;
	}

	return bitstream_file_size;
}

/**
 * @brief Open session the SiP svc client
 *
 * @return 0 on success or negative value on fail
 */
static int32_t svc_client_open(void)
{
	k_timeout_t timeout = K_FOREVER;

	if (!mb_smc_dev) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	if (sip_svc_open(mb_smc_dev, mb_client_token, timeout)) {
		LOG_ERR("Mailbox client open fail");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Init the sip_svc client
 *	first it will register the client then it will open the client
 *
 * @return 0 on success or negative value on fail
 */
static int32_t fpga_manager_init(void)
{
	if (mb_smc_dev) {
		LOG_INF("Mailbox client already registered");
		return 0;
	}

	mb_smc_dev = sip_svc_get_controller("smc");
	if (!mb_smc_dev) {
		LOG_ERR("Arm SiP service not found");
		return -ENODEV;
	}

	mb_client_token = sip_svc_register(mb_smc_dev, NULL);
	if (mb_client_token == SIP_SVC_ID_INVALID) {
		mb_smc_dev = NULL;
		LOG_ERR("Mailbox client register fail");
		return -EINVAL;
	}

	curr_stage = FPGA_CONFIG_INIT;
	k_mutex_init(&config_state);

	return 0;
}

/**
 * @brief Close the svc client
 *
 * @return 0 on success or negative value on fail
 */
static int32_t svc_client_close(void)
{
	int32_t err = 0;
	uint32_t cmd_size = sizeof(uint32_t);
	struct sip_svc_request request;

	if (!mb_smc_dev) {
		LOG_ERR("Mailbox client is not registered");
		return 0;
	}

	uint32_t *cmd_addr = k_malloc(cmd_size);

	if (!cmd_addr) {
		return -ENOMEM;
	}

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

	err = sip_svc_close(mb_smc_dev, mb_client_token, &request);
	if (err) {
		k_free(cmd_addr);
		LOG_ERR("Mailbox client close fail (%d)", err);
	}

	curr_stage = FPGA_CONFIG_INIT;
	reconfig_progress = 0;
	k_mutex_unlock(&config_state);
	return err;
}

/**
 * @brief Call back function which we receive when we send the data
 * based on the current stage it will collect the data
 *
 * @param[in] c_token Token id for our svc services
 * @param[in] res Buffer will contain the response
 * @param[in] size Size of the response
 *
 * @return void
 */
static void cmd_send_callback(uint32_t c_token, struct sip_svc_response *response)
{
	if (response == NULL) {
		return;
	}

	uint32_t *resp_data = NULL;
	uint32_t resp_len = 0;
	uint32_t i = 0;
	struct fm_private_data *private_data = (struct fm_private_data *)response->priv_data;
	union mailbox_response_header response_header;

	response_header.header = 0;

	LOG_DBG("sip_svc send command callback\n");
	LOG_DBG("\tresponse data=\n");

	/* Condition to check only for the mailbox command not for the non-mailbox command */
	if (response->resp_data_size) {
		resp_data = (uint32_t *)response->resp_data_addr;
		resp_len = response->resp_data_size / 4;
		if (resp_data && resp_len) {
			response_header = (union mailbox_response_header)resp_data[0];
			for (i = 0; i < resp_len; i++) {
				LOG_DBG("\t\t[%4d] %08x\n", i, resp_data[i]);
			}
		} else {
			LOG_ERR("\t\tInvalid addr (%p) or len (%d)", resp_data, resp_len);
		}
	}
	/* Condition for non-mailbox command*/
	else {
		LOG_DBG("Response data size is Zero !!");
	}

	switch (curr_stage) {
	case FPGA_CANCEL_STAGE:
		if (response_header.header_t.error_code == 0x00 ||
		    response_header.header_t.error_code == 0x3FF ||
		    response_header.header_t.error_code == 0x2FF) {
			LOG_DBG("Mailbox cancel command success");
			curr_stage = FPGA_RECONFIG_SEND;
		} else {
			LOG_ERR("Mailbox cancel command failed with error code %d",
				response_header.header_t.error_code);
			curr_stage = FPGA_RECONFIG_EXIT;
		}
		break;

	case FPGA_RECONFIG_SEND:
		if (resp_data && response_header.header_t.error_code == 0x00) {
			LOG_DBG("Mailbox reconfig command success");
			LOG_DBG("Number of the DMA count support by SDM %d\n",
				resp_data[MBOX_RECONFIG_DMA_COUNT]);
			fpga_reconfig_dma_count = resp_data[MBOX_RECONFIG_DMA_COUNT];
			LOG_DBG("Size of the each DMA Block %d\n",
				resp_data[MBOX_RECONFIG_DMA_SIZE]);
			fpga_reconfig_dma_block_size = resp_data[MBOX_RECONFIG_DMA_SIZE];
			curr_stage = FPGA_RECONFIG_DATA_SEND;
		} else {
			LOG_ERR("Mailbox reconfig command failed with error code %d",
				response_header.header_t.error_code);
			curr_stage = FPGA_RECONFIG_EXIT;
		}
		break;

	case FPGA_RECONFIG_DATA_SEND:
		if (response_header.header_t.error_code == 0x00) {
			LOG_INF("Reconfig data block received by SDM !!");

			if (private_data->reconfig_data_send_done == true) {
				curr_stage = FPGA_RECONFIG_CHECK_STATUS;
			}
		} else {
			LOG_ERR("Mailbox reconfig data command failed with error code %d",
				response_header.header_t.error_code);
			private_data->private_data_lock->response_status = true;
			curr_stage = FPGA_RECONFIG_EXIT;
		}

		if (private_data->reconfig_data_send_done == true) {
			k_sem_give(&(private_data->private_data_lock->reconfig_data_send_done_sem));
		}

		k_free(private_data);
		break;

	case FPGA_RECONFIG_CHECK_STATUS:
		if (resp_data)
			private_data->config_status.header = (union mailbox_response_header)
				resp_data[MBOX_RECONFIG_STATUS_HEADER];
		if (resp_data && response_header.header_t.error_code == 0x00) {
			LOG_DBG("Mailbox reconfig status command success");
			if (private_data) {
				private_data->config_status.state =
					resp_data[MBOX_RECONFIG_STATUS_STATE];
				private_data->config_status.version = (union config_status_version)
					resp_data[MBOX_RECONFIG_STATUS_VERSION];
				private_data->config_status.pin_status =
					(union config_status_pin_status)
						resp_data[MBOX_RECONFIG_STATUS_PIN_STATUS];
				private_data->config_status.soft_function_status =
					resp_data[MBOX_RECONFIG_STATUS_SOFT_FUNCTION];
				private_data->config_status.error_location =
					resp_data[MBOX_RECONFIG_STATUS_ERROR_LOCATION];
				private_data->config_status.error_details =
					resp_data[MBOX_RECONFIG_STATUS_ERROR_DETAILS];

				if (!private_data->config_status.state) {
					curr_stage = FPGA_BRIDGE_ENABLE;
				} else if (private_data->config_status.state !=
						   MBOX_CONFIG_STATUS_STATE_CONFIG &&
					   private_data->config_status.state !=
						   MBOX_CFGSTAT_VAB_BS_PREAUTH) {
					curr_stage = FPGA_RECONFIG_EXIT;
				}
			}
		} else {
			LOG_ERR("Mailbox reconfig status command failed with error code %d",
				response_header.header_t.error_code);
		}

		break;

	case FPGA_RECONFIG_EXIT:
		LOG_DBG("Exit stage");
		if (private_data && private_data->reconfig_data_send_done == true) {
			k_sem_give(&(private_data->private_data_lock->reconfig_data_send_done_sem));
		}

		if (private_data) {
			k_free(private_data);
		}
	}

	/* Client only responsible to free the response data memory space,
	 * the command data memory space had been freed by sip_svc service.
	 */
	if (response->resp_data_addr) {
		LOG_DBG("\tFree response memory %p\n", (char *)response->resp_data_addr);
		k_free((char *)response->resp_data_addr);
	}

	if (private_data) {
		k_sem_give(&(private_data->private_data_lock->reconfig_data_sem));
	}
}

/**
 * @brief Send the reconfig data to SDM
 *
 * @param[in] buf Bitstream data chunk
 * @param[in] size Size of the buf data
 *
 * @return 0 on success or negative value on fail
 */
static int32_t send_reconfig_data(char *buf, size_t size)
{
	uint32_t *cmd_addr_buffer;
	uint32_t *resp_addr;
	uint32_t svc_buf_idx = 0;
	uint32_t buf_idx = 0;
	int32_t trans_id = 0;
	int32_t err = 0;
	uint32_t rbf_data_total = size;
	uint32_t rbf_data_rem = rbf_data_total;
	uint32_t rbf_data_sent = 0;
	struct fm_private_data *private_data = NULL;
	struct fm_lock_data sync_lock;
	struct sip_svc_request *request_buffer;

	sync_lock.response_status = false;

	k_sem_init(&(sync_lock.reconfig_data_sem), 3, 3);
	k_sem_init(&(sync_lock.reconfig_data_send_done_sem), 0, 1);

	request_buffer = (struct sip_svc_request *)k_malloc(fpga_reconfig_dma_count *
							    sizeof(struct sip_svc_request));
	if (!request_buffer) {
		return -ENOMEM;
	}

	while (rbf_data_rem > 0 && (!sync_lock.response_status)) {
		private_data = (struct fm_private_data *)k_malloc(sizeof(struct fm_private_data));
		if (!private_data) {
			k_free(request_buffer);
			return -ENOMEM;
		}
		private_data->private_data_lock = &sync_lock;

		cmd_addr_buffer = (uint32_t *)k_malloc(FPGA_MB_CMD_ADDR_MEM_SIZE);
		if (!cmd_addr_buffer) {
			k_free(request_buffer);
			k_free(private_data);
			return -ENOMEM;
		}
		resp_addr = (uint32_t *)k_malloc(FPGA_MB_RESPONSE_MEM_SIZE);
		if (!resp_addr) {
			k_free(request_buffer);
			k_free(private_data);
			k_free(cmd_addr_buffer);
			return -ENOMEM;
		}

		request_buffer[svc_buf_idx].header =
			SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
		request_buffer[svc_buf_idx].a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
		request_buffer[svc_buf_idx].a1 = 0;
		request_buffer[svc_buf_idx].a2 = (uint64_t)cmd_addr_buffer;
		request_buffer[svc_buf_idx].a3 = (uint64_t)RECONFIG_DATA_MB_CMD_SIZE;
		request_buffer[svc_buf_idx].a4 = 0;
		request_buffer[svc_buf_idx].a5 = 0;
		request_buffer[svc_buf_idx].a6 = 0;
		request_buffer[svc_buf_idx].a7 = 0;
		request_buffer[svc_buf_idx].resp_data_addr = (uint64_t)resp_addr;
		request_buffer[svc_buf_idx].resp_data_size = FPGA_MB_RESPONSE_MEM_SIZE * 4;

		/* Filling the private data */
		request_buffer[svc_buf_idx].priv_data = (void *)private_data;

		cmd_addr_buffer[buf_idx++] =
			MBOX_REQUEST_HEADER(FPGA_RECONFIG_DATA, RECONFIG_DATA_MB_CMD_INDIRECT_MODE,
					    RECONFIG_DATA_MB_CMD_LENGTH);
		cmd_addr_buffer[buf_idx++] = MBOX_RECONFIG_REQUEST_DATA_FORMAT(
			RECONFIG_DATA_MB_CMD_DIRECT_COUNT, RECONFIG_DATA_MB_CMD_INDIRECT_ARG,
			RECONFIG_DATA_MB_CMD_INDIRECT_RESPONSE);
		/* Filling the memory address from where SDM will read the Reconfig data */
		cmd_addr_buffer[buf_idx++] = POINTER_TO_UINT(buf) + rbf_data_sent;

		if (rbf_data_rem < fpga_reconfig_dma_block_size) {
			/* Filling reconfig data size in the mailbox command header */
			cmd_addr_buffer[buf_idx++] = rbf_data_rem;
			private_data->reconfig_data_send_done = 1;
		} else {
			/* Filling reconfig data size in the mailbox command header */
			cmd_addr_buffer[buf_idx++] = fpga_reconfig_dma_block_size;
		}

		if (cmd_addr_buffer) {
			for (int32_t j = 0; j < buf_idx; j++) {
				LOG_DBG("\t [%d ] %08x\n", j, cmd_addr_buffer[j]);
			}
		}

		/* Sending the SMC request here */
		trans_id = sip_svc_send(mb_smc_dev, mb_client_token, &request_buffer[svc_buf_idx],
					cmd_send_callback);

		if (trans_id == SIP_SVC_ID_INVALID) {
			/* Freeing request Buffer */
			k_free(request_buffer);
			/* Free the mailbox command request buffer */
			k_free(cmd_addr_buffer);
			/* Free the mailbox command response buffer */
			k_free(resp_addr);
			/* Free private_data data memory*/
			k_free(private_data);
			err = -EBUSY;
		} else {
			if (k_sem_take(&(private_data->private_data_lock->reconfig_data_sem),
				       K_FOREVER) != 0) {
				return -ETIMEDOUT;
			}

			if (rbf_data_rem < fpga_reconfig_dma_block_size) {
				rbf_data_sent += rbf_data_rem;
			} else {
				/* Sent last reconfig data chunk */
				rbf_data_sent += fpga_reconfig_dma_block_size;
			}
			/* Checking the remaining reconfig data */
			rbf_data_rem = rbf_data_total - rbf_data_sent;
			svc_buf_idx++;

			if (svc_buf_idx >= fpga_reconfig_dma_count) {
				svc_buf_idx = 0;
			}

			buf_idx = 0;
			err = 0;
		}
	}

	if ((rbf_data_rem == 0) && (private_data->reconfig_data_send_done == true)) {
		/* Waiting for the last data chunk response */
		if (k_sem_take(&(sync_lock.reconfig_data_send_done_sem), K_FOREVER) != 0) {
			return -ETIMEDOUT;
		}
	}

	if (sync_lock.response_status) {
		return -EINVAL;
	}

	return err;
}

/**
 * @brief Send the data to sip_svc service layer
 *	based on the cmd_type further data will be send to SDM using mailbox
 *
 * @param[in] cmd_type Command type (Mailbox or Non-Mailbox)
 * @param[in] function_identifier Function identifier for each command type
 * @param[in] cmd_id Command id for each mailbox command
 *
 * @return 0 on success or negative value on fail
 */
static int32_t smc_send(uint32_t cmd_type, uint64_t function_identifier, uint32_t cmd_id,
			struct fm_private_data *private_data)
{
	int32_t trans_id = 0;
	uint32_t *cmd_addr = NULL;
	uint32_t *resp_addr = NULL;
	struct sip_svc_request request;

	if (!mb_smc_dev) {
		LOG_ERR("Mailbox client is not registered");
		return 0;
	}
	if (cmd_type == SIP_SVC_PROTO_CMD_ASYNC) {
		cmd_addr = (uint32_t *)k_malloc(FPGA_MB_CMD_ADDR_MEM_SIZE);
		if (!cmd_addr) {
			LOG_ERR("Failed to allocate command memory");
			return -ENOMEM;
		}
		cmd_addr[0] = MBOX_REQUEST_HEADER(cmd_id, 0, 0);
		resp_addr = (uint32_t *)k_malloc(FPGA_MB_RESPONSE_MEM_SIZE);
		if (!resp_addr) {
			k_free(cmd_addr);
			return -ENOMEM;
		}
	}
	request.header = SIP_SVC_PROTO_HEADER(cmd_type, 0);
	request.a0 = function_identifier;
	request.a1 = 0;
	if (cmd_type == SIP_SVC_PROTO_CMD_ASYNC) {
		request.a2 = (uint64_t)cmd_addr;
		request.a3 = 4;
	} else {
		request.a2 = cmd_id;
		request.a3 = 0;
	}
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;

	if (cmd_type == SIP_SVC_PROTO_CMD_ASYNC) {
		request.resp_data_addr = (uint64_t)resp_addr;
		request.resp_data_size = FPGA_MB_RESPONSE_MEM_SIZE;
		if (cmd_addr) {
			for (int32_t mbox_idx = 0; mbox_idx < request.a3 / 4; mbox_idx++) {
				LOG_DBG("\t [%d ] %08x\n", mbox_idx, cmd_addr[mbox_idx]);
			}
		}
	} else {
		request.resp_data_addr = 0;
		request.resp_data_size = 0;
	}
	request.priv_data = (void *)private_data;

	trans_id = sip_svc_send(mb_smc_dev, mb_client_token, &request, cmd_send_callback);

	if (trans_id == SIP_SVC_ID_INVALID) {
		LOG_ERR("Mailbox send fail (no open or no free trans_id)");
		k_free(resp_addr);
		return -EBUSY;
	}

	LOG_DBG("Mailbox send success: trans_id %d", trans_id);
	return 0;
}

/**
 * @brief Start the reconfiguration based on the type Full or Partial
 * run as a thread and complete the configuration
 *
 * @param[in] p1 Bitstream location
 * @param[in] p2 Image size
 * @param[in] p3 Null
 *
 */
static void reconfig_start(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	char *image_ptr = (char *)p1;
	uint32_t img_size = POINTER_TO_UINT(p2);
	uint32_t ret = 0;
	uint32_t cmd_type = 0;
	uint64_t function_identifier = 0;
	uint32_t cmd_id = 0;
	uint32_t retry_count = 0;
	struct fm_private_data private_data;

	private_data.private_data_lock =
		(struct fm_lock_data *)k_malloc(sizeof(struct fm_lock_data));
	if (!private_data.private_data_lock) {
		LOG_ERR("Failed to allocate command memory");
		k_mutex_unlock(&config_state);
		return;
	}
	k_sem_init(&(private_data.private_data_lock->reconfig_data_sem), 0, 1);

	while (true) {
		switch (curr_stage) {
		case FPGA_BRIDGE_DISABLE:
			/* Will disable all the fpga bridges here */
			LOG_DBG("Sending the bridge disable command");
			ret = do_bridge_reset(DISABLE, BRIDGE_ALL);
			if (!ret || ret == -EIO) {
				LOG_DBG("All bridges disable successfully !!");
				curr_stage = FPGA_CONFIG_INIT;
			} else {
				LOG_ERR("All bridge disable failed");
				k_mutex_unlock(&config_state);
				return;
			}
			break;

		case FPGA_CONFIG_INIT:
			/* Open sip_svc client here */
			LOG_DBG("Sending the init command");
			ret = svc_client_open();
			if (!ret) {
				LOG_DBG("Client init Success !!");
				curr_stage = FPGA_CANCEL_STAGE;
			} else {
				LOG_ERR("Client init Failed !!");
				k_mutex_unlock(&config_state);
				return;
			}
			break;

		case FPGA_CANCEL_STAGE:
			/* Will cancel the ongoing transaction */
			LOG_DBG("Sending the mailbox cancel command");
			cmd_type = SIP_SVC_PROTO_CMD_ASYNC;
			function_identifier = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
			cmd_id = FPGA_CANCEL;
			break;

		case FPGA_RECONFIG_SEND:
			/* Will send reconfig SMC call here */
			LOG_DBG("Sending the reconfig command");
			cmd_type = SIP_SVC_PROTO_CMD_ASYNC;
			function_identifier = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
			cmd_id = FPGA_RECONFIG;
			break;

		case FPGA_RECONFIG_DATA_SEND:
			/* Will send the bitstream data here */
			LOG_DBG("Sending the reconfig data command");
			if (send_reconfig_data(image_ptr, img_size)) {
				LOG_ERR("Failed !!");
				curr_stage = FPGA_RECONFIG_EXIT;
			}
			continue;

		case FPGA_RECONFIG_CHECK_STATUS:
			/* Will check the reconfig status here */
			LOG_DBG("Sending the reconfig status command");
			cmd_type = SIP_SVC_PROTO_CMD_ASYNC;
			function_identifier = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
			cmd_id = FPGA_RECONFIG_STATUS;
			if (retry_count++ == RECONFIG_STATUS_RETRY_COUNT) {
				LOG_ERR("Reconfig status timeout !!");
				curr_stage = FPGA_RECONFIG_EXIT;
				retry_count = 0;
				continue;
			}
			/* Need some time after reconfig data send */
			k_sleep(K_MSEC(RECONFIG_STATUS_INTERVAL_DELAY_US));
			break;

		case FPGA_BRIDGE_ENABLE:
			/* Will enable all the fpga bridges once the reconfig is done */
			LOG_DBG("Sending the bridge enable command");
			/* TODO: SiP client close to be removed
			 * once SiP SVC multiple client is supported
			 */
			(void)svc_client_close();

			function_identifier = 0;
			ret = do_bridge_reset(ENABLE, BRIDGE_ALL);
			if (!ret) {
				LOG_DBG("All bridges enable successfully !!");
			}
			/* TODO: SiP client open to be removed
			 * once SiP SVC multiple client is supported
			 */
			(void)svc_client_open();

			curr_stage = FPGA_RECONFIG_EXIT;
			break;

		case FPGA_RECONFIG_EXIT:
			LOG_DBG("FPGA configuration all stages completed !!");
			k_free(private_data.private_data_lock);
			(void)svc_client_close();
			return;
		}

		if (function_identifier) {
			ret = smc_send(cmd_type, function_identifier, cmd_id, &private_data);
			if (ret == -EBUSY) {
				LOG_ERR("Failed to send the mailbox command !!");
				k_free(private_data.private_data_lock);
				(void)svc_client_close();
				return;
			}
			/* Wait for callback */
			if (k_sem_take(&(private_data.private_data_lock->reconfig_data_sem),
				       K_FOREVER) != 0) {
				return;
			}
		}
	}
}

/* Validate the Reconfig status response */
static int32_t fpga_reconfig_status_validate(struct fpga_config_status *reconfig_status_resp)
{
	int32_t ret = 0;

	/* Check for any error */
	ret = reconfig_status_resp->state;
	if (ret && ret != MBOX_CONFIG_STATUS_STATE_CONFIG)
		return ret;

	/* Make sure nStatus is not 0 */
	ret = reconfig_status_resp->pin_status.pin_status;
	if (!(ret & RECONFIG_PIN_STATUS_NSTATUS))
		return MBOX_CFGSTAT_STATE_ERROR_HARDWARE;

	ret = reconfig_status_resp->soft_function_status;
	if (ret & RECONFIG_SOFTFUNC_STATUS_SEU_ERROR)
		return MBOX_CFGSTAT_STATE_ERROR_HARDWARE;

	if ((ret & RECONFIG_SOFTFUNC_STATUS_CONF_DONE) &&
	    (ret & RECONFIG_SOFTFUNC_STATUS_INIT_DONE) && !reconfig_status_resp->state)
		return 0; /* Configuration success */

	return MBOX_CONFIG_STATUS_STATE_CONFIG;
}

/* Create and start the reconfiguration thread */
uint32_t config_thread_start(char *image_ptr, uint32_t img_size)
{
	/* Will start Reconfig state from here only */
	k_tid_t config_tid =
		k_thread_create(&config_thread_data, reconfig_thread_stack_area,
				K_KERNEL_STACK_SIZEOF(reconfig_thread_stack_area), reconfig_start,
				(void *)image_ptr, UINT_TO_POINTER(img_size), NULL,
				FPGA_MANAGER_THREAD_PRIORITY, 0, K_NO_WAIT);

	if (!config_tid) {
		LOG_ERR("Failed to Create Thread !!");
		k_mutex_unlock(&config_state);
		return -EINVAL;
	}

	k_thread_name_set(&config_thread_data, "fpga_manager");
	LOG_INF("Thread created successfully");
	return 0;
}

/* Will fetch the memory address for the dtsi file */
int32_t fpga_get_memory_plat(char **phyaddr, uint32_t *size)
{
	*phyaddr = (char *)DT_REG_ADDR(DT_NODELABEL(fpga_config));

	if (*phyaddr == NULL) {
		LOG_ERR("Failed to get the memory address");
		return -EADDRNOTAVAIL;
	}

	*size = DT_REG_SIZE(DT_NODELABEL(fpga_config));

	return 0;
}

int32_t fpga_load_plat(char *image_ptr, uint32_t img_size)
{
	char *fpga_memory_addr;
	uint32_t fpga_memory_size = 0;

	if (!image_ptr)
		return -EFAULT;

	if (fpga_get_memory_plat(&fpga_memory_addr, &fpga_memory_size)) {
		LOG_ERR("Failed to get the reserve memory pointer!!");
		return -EFAULT;
	}

	if (!fpga_memory_size || !img_size || (img_size > fpga_memory_size))
		return -ENOSR;

	if (k_mutex_lock(&config_state, K_NO_WAIT)) {
		LOG_ERR("Failed to acquire mutex lock");
		return -EBUSY;
	}

	/* if pointer and size is outside of reserved memory range,
	 * then return error
	 */
	if ((image_ptr < fpga_memory_addr) || (image_ptr > (fpga_memory_addr + fpga_memory_size)) ||
	    ((image_ptr + img_size) > (fpga_memory_addr + fpga_memory_size))) {
		return -EFAULT;
	}

	if (config_thread_start(image_ptr, img_size)) {
		return -EINVAL;
	}

	return 0;
}

/* Read the file content and copy at the fpga reserved memory region and send the data to SDM */
int32_t fpga_load_file_plat(const char *filename, uint32_t config_type)
{
	int32_t ret_val;
	uint32_t img_size = 0;
	uint32_t fpga_memory_size = 0;
	char *fpga_memory_addr = NULL;

	/* Partial configuration not supported */
	if (config_type) {
		return -ENOTSUP;
	}

	if (k_mutex_lock(&config_state, K_NO_WAIT)) {
		LOG_ERR("Failed to acquire mutex lock");
		return -EBUSY;
	}

	if (reconfig_progress) {
		return -EBUSY;
	}

	reconfig_progress = 1;

	if (fpga_get_memory_plat(&fpga_memory_addr, &fpga_memory_size)) {
		LOG_ERR("Failed to get the reserve memory pointer!!");
		k_mutex_unlock(&config_state);
		return -EFAULT;
	}

	ret_val = copy_bitstream_at_loc(filename, fpga_memory_addr, fpga_memory_size);
	if (ret_val <= 0) {
		LOG_ERR("Failed to read file");
		k_mutex_unlock(&config_state);
		reconfig_progress = 0;
		return ret_val;
	}

	img_size = ret_val;
	k_mutex_unlock(&config_state);

	ret_val = config_thread_start(fpga_memory_addr, img_size);
	if (ret_val) {
		LOG_ERR("Reconfiguration Failed!!");
		reconfig_progress = 0;
		return ret_val;
	}
	return 0;
}

/* Store the fpga status in config_status_info buffer*/
static void get_config_status_info(struct fpga_config_status *status, char *config_status_info)
{
	int32_t ret = 0;

	if ((status->header.header_t.data_length == 0x06) &&
	    (status->header.header_t.error_code == 0x00)) {
		sprintf(config_status_info, "\n\tConfig State 0x%08x\n", status->state);
		sprintf(config_status_info + strlen(config_status_info),
			"\tConfig Version 0x%08x\n", status->version.version);
		sprintf(config_status_info + strlen(config_status_info),
			"\tConfig Version update_number 0x%02x\n",
			status->version.response_version_member.update_number);
		sprintf(config_status_info + strlen(config_status_info),
			"\tMinor_acds_release_number 0x%02x\n",
			status->version.response_version_member.minor_acds_release_number);
		sprintf(config_status_info + strlen(config_status_info),
			"\tMajor_acds_release_number 0x%02x\n",
			status->version.response_version_member.major_acds_release_number);
		sprintf(config_status_info + strlen(config_status_info),
			"\tQspi_flash_index 0x%02x\n",
			status->version.response_version_member.qspi_flash_index);

		sprintf(config_status_info + strlen(config_status_info),
			"\tConfig Pin Status 0x%08x\n", status->pin_status.pin_status);
		sprintf(config_status_info + strlen(config_status_info), "\tMSEL 0x%02x\n",
			status->pin_status.pin_status_member.msel);
		sprintf(config_status_info + strlen(config_status_info), "\tPMF_data 0x%02x\n",
			status->pin_status.pin_status_member.pmf_data);
		sprintf(config_status_info + strlen(config_status_info), "\tnconfig 0x%02x\n",
			status->pin_status.pin_status_member.nconfig);
		sprintf(config_status_info + strlen(config_status_info),
			"\tnconfig_status 0x%02x\n",
			status->pin_status.pin_status_member.nconfig_status);

		sprintf(config_status_info + strlen(config_status_info),
			"\tConfig Soft Function Status 0x%08x\n", status->soft_function_status);
		sprintf(config_status_info + strlen(config_status_info),
			"\tConfig Error location 0x%08x\n", status->error_location);

		ret = fpga_reconfig_status_validate(status);
		if (!ret) {
			sprintf(config_status_info + strlen(config_status_info),
				"\nFPGA Configuration OK ...\n");
		} else {
			sprintf(config_status_info + strlen(config_status_info),
				"\nFPGA Configuration Failed ...\n");
		}
	} else if (status->header.header_t.error_code == 0x2FF) {
		sprintf(config_status_info, "No reply will be provided. Query from proper client "
					    "OR Start Reconfig first");
	} else if (status->header.header_t.error_code == 0x04) {
		sprintf(config_status_info, "Invalid length or Indirect setting in header");
	} else {
		sprintf(config_status_info, "Invalid header");
	}
}

/* Will send the SMC command to check the status of the FPGA configuration */
int32_t fpga_get_status_plat(void *config_status_buf)
{
	int32_t ret = 0;
	struct fm_private_data priv_data;
	struct fm_lock_data lock;

	if (!config_status_buf) {
		return -ENOMEM;
	}

	if (k_mutex_lock(&config_state, K_NO_WAIT)) {
		LOG_ERR("Failed to acquire mutex lock");
		return -EBUSY;
	}

	priv_data.private_data_lock = &lock;
	k_sem_init(&(priv_data.private_data_lock->reconfig_data_sem), 0, 1);

	ret = svc_client_open();
	if (ret) {
		LOG_ERR("Client open Failed !!");
		k_mutex_unlock(&config_state);
		return ret;
	}

	curr_stage = FPGA_RECONFIG_CHECK_STATUS;

	if (smc_send(SIP_SVC_PROTO_CMD_ASYNC, SMC_FUNC_ID_MAILBOX_SEND_COMMAND,
		     FPGA_RECONFIG_STATUS, &priv_data)) {
		LOG_ERR("Failed to send the Mailbox Command !!");
		ret = -ECANCELED;
	} else {
		k_sem_take(&(priv_data.private_data_lock->reconfig_data_sem), K_FOREVER);
		get_config_status_info(&priv_data.config_status, config_status_buf);
	}

	if (svc_client_close()) {
		LOG_ERR("SIP SVC client closing failed");
	}

	return ret;
}

SYS_INIT(fpga_manager_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
