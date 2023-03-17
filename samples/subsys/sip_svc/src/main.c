/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * A sample application using sip_svc subsystem to get query values from secure device.
 * The access to the secure device is defined via EL3 exception level and uses
 * the ARM Trusted Firmware to provide access.The application runs on Intel Agilex FPGA SoC,
 * where the app queries voltage sampled from the SDM(Secure Device Manager).
 */

#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_mailbox.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include <zephyr/sys/__assert.h>

#define SVC_METHOD	       "smc"
#define GET_VOLTAGE_CMD	       (0x18U)
#define SET_VOLTAGE_CHANNEL(x) ((1 << (x)) & 0xffff)
#define TIME_DELAY	       (K_MSEC(1000U))

struct private_data {
	struct k_sem semaphore;
	uint32_t voltage_channel0;
};

void get_voltage_callback(uint32_t c_token, struct sip_svc_response *response)
{
	if (response == NULL) {
		return;
	}

	struct private_data *priv = (struct private_data *)response->priv_data;

	uint32_t *resp_data = (uint32_t *)response->resp_data_addr;
	uint32_t resp_len = response->resp_data_size / 4;

	if (resp_data && resp_len) {
		priv->voltage_channel0 = resp_data[1];
	}

	k_sem_give(&(priv->semaphore));
}

void main(void)
{
	void *mb_smc_ctrl = NULL;
	uint32_t mb_c_token = SIP_SVC_ID_INVALID;
	struct sip_svc_request request = {0};
	uint32_t *cmd_addr = NULL, *resp_addr = NULL;
	uint32_t cmd_size = (2 * sizeof(uint32_t));
	uint32_t resp_size = (2 * sizeof(uint32_t));
	struct private_data priv;

	float voltage;
	int err, trans_id;

	resp_addr = (uint32_t *)k_malloc(resp_size);
	__ASSERT(resp_addr != NULL, "Failed to get memory");

	mb_smc_ctrl = sip_svc_get_controller(SVC_METHOD);
	__ASSERT(mb_smc_ctrl != NULL, "Failed to get the controller from sip_svc");

	mb_c_token = sip_svc_register(mb_smc_ctrl, NULL);
	__ASSERT(mb_c_token != SIP_SVC_ID_INVALID, "Failed to register with sip_svc");

	k_sem_init(&(priv.semaphore), 0, 1);

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
	request.a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
	request.resp_data_addr = (uint64_t)resp_addr;
	request.resp_data_size = (uint64_t)resp_size;
	request.priv_data = (void *)&priv;

	while (1) {
		err = sip_svc_open(mb_smc_ctrl, mb_c_token, K_FOREVER);
		__ASSERT(err != SIP_SVC_ID_INVALID, "Failed to open with sip_svc");

		cmd_addr = (uint32_t *)k_malloc(cmd_size);
		__ASSERT(cmd_addr != NULL, "Failed to get memory");

		/**
		 * Populate the SDM mailbox command ,where first word will be the header,
		 * header will contain the mailbox command and the size of command data to be sent
		 * to SDM. The transaction id and client id will be filled by sip_svc subsystem.
		 */
		cmd_addr[0] = ((cmd_size / 4 - 1) << 12) | GET_VOLTAGE_CMD;
		cmd_addr[1] = SET_VOLTAGE_CHANNEL(2);

		/**
		 * Set the pointer to mailbox command buffer to a2 parameter and
		 * mailbox command buffer size to a3 parameter ,which EL3 software will
		 * expect for a ASYNC transaction.
		 */
		request.a2 = (uint64_t)cmd_addr;
		request.a3 = (uint64_t)cmd_size;

		trans_id = sip_svc_send(mb_smc_ctrl, mb_c_token, &request, get_voltage_callback);
		__ASSERT(trans_id < 0, "Error in sending a request to SDM mailbox");

		err = k_sem_take(&(priv.semaphore), K_FOREVER);
		__ASSERT(err != 0, "Error in taking semaphore");

		/* Voltage is retrieved as a fixed point number with 16 bits below binary point */
		voltage = ((float)priv.voltage_channel0 / 65536);

		printk("Got response of transaction id 0x%02x and voltage is %fv\n", trans_id,
		       voltage);

		err = sip_svc_close(mb_smc_ctrl, mb_c_token, NULL);
		__ASSERT(err != SIP_SVC_ID_INVALID, "Failed to close with sip_svc");

		k_sleep(TIME_DELAY);
	}
}
