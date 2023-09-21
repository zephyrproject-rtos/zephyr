/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Single Event Upsets (SEUs) Error Detection and Reporting Driver
 *
 *
 * ********
 * Overview
 * ********
 * SEUs can occur due to radiation particles affecting memory, leading to data corruption or
 * system errors.
 * The SDM (Secure Device Mananger) is responsible for detecting SEU errors within the system
 * and initiating an interrupt from the SDM to the HPS (Hard Processor System).
 * This driver provides functions to detect SEUs via Interrupt from SDM and report errors to
 * the user via using Mailbox commands from HPS(Hard Processor System) to Secure Device Mananger.
 *
 * Driver Typical Workflow
 * (1) Register a callback function that specifies the required error mode. This registration
 *     will return a unique client number.
 * (2) Enable the callback function using the assigned client number.
 * (3) When an error detection event occurs, the driver will automatically trigger the registered
 *     callback function.
 * (4) To simulate an error, you can use the error injection API provided by the driver.
 *
 * Callback Function Implementation Requirement:
 * (1) The user must provide a callback function. When an error occurs, this callback function
 *     will be invoked, providing it with error information data.
 *
 * ***************************************
 * SEU and callback overview
 * ***************************************
 * ------------------------------------------------------
 *                 callback1     callback2     callback3 ...
 * Register           |           |               |
 * callback           |           |               |
 * functions          |           |               |
 *                    |           |               |
 * ------------------------------------------------------
 * Enable the callback functions
 *                      -----
 *           --------->| SDM | <------------------------
 *          |           -----                           |
 *          |             |     flow1                   |
 *          |      ---------------------                |
 *          |     | Interrupt triggered |               |
 *          |      ---------------------                |
 *          |              |     flow1                  |
 *          |        ------------------                 |
 *           -------| Mailbox Commands | read SEU error |
 *                   ------------------                 |
 *                           |    flow1                 |
 *          -------------------------------------       |
 *         |                  |                  |      |
 *      -----------      -----------        ----------- |
 *     | callback1 |    | callback2 |      | callback3 ||
 *      -----------      -----------        ----------- |
 *                                                      |
 *                    ------------------       flow2    |
 *            flow2  | Mailbox Commands |---------------
 *                    ------------------
 *                           ^
 *                           |   flow2
 *                    --------------
 *                   | Inject Error |
 *                    --------------
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/seu/seu.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>

LOG_MODULE_REGISTER(seu, CONFIG_SEU_LOG_LEVEL);

#define DT_DRV_COMPAT intel_socfpga_seu

#define DT_SEU DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)

/* Retrieve the interrupt number from the device tree */
#define SEU_ERROR_IRQn DT_IRQN(DT_NODELABEL(seu))
#define SEU_PRIORITY   DT_IRQ(DT_NODELABEL(seu), priority)
#define SEU_IRQ_FLAGS  0

/* Command codes for reading/injection  from the mailbox */
#define READ_SEU_ERROR_CMD     (0x3C)
#define SEU_INSERT_ERR_CMD     (0x3D)
#define SEU_READ_STATS_CMD     (0x40)
#define SEU_INSERT_SAFE_CMD    (0x41)
#define SEU_INSERT_ECC_ERR_CMD (0x42)

/* SVC Method to call */
#define SVC_METHOD "smc"

/* Length for SEU frames */
#define SEU_READ_STATISTICS_LENGTH   (1)
#define SEU_INSERT_ERROR_LENGTH      (2)
#define SEU_INSERT_SAFE_ERROR_LENGTH (2)
#define SEU_INSERT_ECC_ERR_LENGTH    (1)

/* Read SEU statistics index's */
#define INDEX_SEU_CYCLE      (1)
#define INDEX_SEU_DETECT     (2)
#define INDEX_SEU_CORRECT    (3)
#define INDEX_SEU_INJECT     (4)
#define INDEX_SEU_POLL       (5)
#define INDEX_SEU_PIN_TOGGLE (6)

/* Index of response header */
#define INDEX_RESPONSE_HEADER (0)

/* Index of SEU error response */
#define INDEX_SEU_ERROR_1 (1)
#define INDEX_SEU_ERROR_2 (2)
#define INDEX_SEU_ERROR_3 (3)
#define INDEX_SEU_ERROR_4 (4)

/* Command buffer size */
#define SEU_READ_RESPONSE_SIZE (4)
#define INSERT_SAFE_CMD_SIZE   (3)
#define INSERT_SAFE_RESP_SIZE  (7)
#define READ_SEU_STAT_CMD_SIZE (2)
#define READ_SEU_STAT_RES_SIZE (7)

/* error buffer index */
#define INJECT_SEU_ERR_CMD_SIZE (3)
#define INJECT_SEU_ERR_RES_SIZE (7)
#define INSERT_ECC_CMD_SIZE     (2)

/* Command buffer index */
#define INDEX_CMD_0 (0)
#define INDEX_CMD_1 (1)
#define INDEX_CMD_2 (2)

/* get response correction status */
#define GET_SEU_ERR_READ_CORRECTION_STATUS(x)   (FIELD_GET(BIT(28), x))
/* get response read no of error */
#define GET_SEU_ERR_READ_NO_OF_ERR(x)           (FIELD_GET(GENMASK(3, 0), x))
/* get response sector error type */
#define GET_SEU_ERR_SECTOR_ERR_TYPE(x)          (FIELD_GET(GENMASK(7, 4), x))
/* get response sector */
#define GET_SEU_ERR_SECTOR_ERROR(x)             (FIELD_GET(GENMASK(23, 16), x))
/* get response sub error type */
#define GET_SEU_ERR_READ_ERR_DATA_TYPE(x)       (FIELD_GET(GENMASK(31, 29), x))
/* get response error code */
#define GET_SEU_ERR_READ_RES_HEADER_ERR_CODE(x) (FIELD_GET(GENMASK(10, 0), x))
/* get response header length */
#define GET_SEU_ERR_READ_RES_HEADER_LENGTH(x)   (FIELD_GET(GENMASK(22, 12), x))
/* get response header row frame index */
#define GET_SEU_ERR_READ_ROW_FRAME_INDEX(x)     (FIELD_GET(GENMASK(11, 0), x))
/* get response header bit position */
#define GET_SEU_ERR_READ_BIT_POS_FRAME(x)       (FIELD_GET(GENMASK(24, 12), x))
/* get response header error frame type */
#define ERROR_FRAME_TYPE(x)                     (FIELD_GET(GENMASK(7, 4), x))
/* get response header error detect */
#define ERROR_FRAME_DETECT(x)                   (FIELD_GET(GENMASK(3, 0), x))
/* get response header ECC data */
#define GET_ECC_ERR_DATA(x)                     (FIELD_GET(GENMASK(11, 0), x))
/* get response header misc cnt type */
#define GET_MISC_CNT_TYPE(x)                    (FIELD_GET(GENMASK(15, 12), x))
/* get response header misc cnt data */
#define GET_MISC_ERR_READ_CNT_TYPE(x)           (FIELD_GET(GENMASK(11, 0), x))
/* get response header watchdog error status */
#define GET_WDT_ERROR_STATUS_TYPE(x)            (FIELD_GET(GENMASK(11, 0), x))
/* get response header EMIF id */
#define GET_EMIF_ID(x)                          (FIELD_GET(GENMASK(24, 17), x))
/* get response header source id */
#define GET_SOURCE_ID(x)                        (FIELD_GET(GENMASK(16, 10), x))
/* get response header EMIF error type */
#define GET_EMIF_ERROR_TYPE(x)                  (FIELD_GET(GENMASK(9, 6), x))
/* get response header EMIF DDR LSB */
#define GET_EMIF_DDR_LSB(x)                     (FIELD_GET(GENMASK(5, 0), x))
/* set command header frame */
#define SET_FRAME_CMD(x)                        (FIELD_PREP(GENMASK(10, 0), x))
/* set command header sector address */
#define SET_SECTOR_ADDR(x)                      (FIELD_PREP(GENMASK(23, 16), x))
/* set command header inject type */
#define SET_INJECT_TYPE(x)                      (FIELD_PREP(GENMASK(5, 4), x))
/* set command header number of injection */
#define SET_NUMBER_OF_INJECTION(x)              (FIELD_PREP(GENMASK(3, 0), x))
/* set command header cram selection 0 */
#define SET_CRAM_SEL_O(x)                       (FIELD_PREP(GENMASK(3, 0), x))
/* set command header cram selection 1 */
#define SET_CRAM_SEL_1(x)                       (FIELD_PREP(GENMASK(7, 4), x))
/* set command header length */
#define SET_SEU_HEADER_LENGTH(x)                (FIELD_PREP(GENMASK(22, 12), x))
/* set command header ECC injection */
#define SET_NUMBER_OF_ECC_INJECTION(x)          (FIELD_PREP(GENMASK(1, 0), x))
/* set command header ECC RAM id */
#define SET_ECC_RAM_ID(x)                       (FIELD_PREP(GENMASK(6, 2), x))

/* enum of all supported SEU commands*/
enum seu_commands {
	NOT_SELECTED = 0,
	READ_SEU_ERROR,
	INSERT_SEU_ERROR,
	READ_SEU_STATS,
	INSERT_SAFE_SEU_ERROR,
	INSERT_ECC_ERROR
};

/* private data struture */
struct private_data {
	/* Semaphore used to signal from callback function */
	struct k_sem semaphore;
	/* SEU commands */
	enum seu_commands seu_commands;
	/* Error status */
	int32_t status;
	/* SEU statistics */
	struct seu_statistics_data seu_statistics;
	/* Pointer to SEU data */
	struct seu_intel_socfpga_data *seu_data_ptr;
};

/* SEU client for control user function calls */
struct seu_client {
	/* function pointer for store user register function */
	seu_isr_callback_t seu_isr_callback[CONFIG_SEU_MAX_CLIENT];
	/* callback function mask mode */
	uint8_t seu_isr_callback_mode[CONFIG_SEU_MAX_CLIENT];
	/* callback function enable/disable bit */
	bool seu_isr_callback_enable[CONFIG_SEU_MAX_CLIENT];
	/* Number of function calls registered */
	uint8_t total_callback_func;
};

struct seu_intel_socfpga_data {
	/* Synchronize critical data */
	struct k_mutex seu_mutex;
	/* SiP SVC mailbox controller */
	struct sip_svc_controller *mailbox_smc_dev;
	/* Mailbox client token */
	uint32_t mailbox_client_token;
	/* SEU delayed work queue */
	struct k_work_delayable seu_work_delay;
	/* SEU client to control user function registration */
	struct seu_client seu_client_local;
};

static int32_t svc_client_open(struct sip_svc_controller *mailbox_smc_dev,
			       uint32_t mailbox_client_token)
{
	if ((!mailbox_smc_dev) || (mailbox_client_token == 0)) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	if (sip_svc_open(mailbox_smc_dev, mailbox_client_token, K_MSEC(CONFIG_MAX_TIMEOUT_MSECS))) {
		LOG_ERR("Mailbox client open fail");
		return -ENODEV;
	}

	return 0;
}

static int32_t svc_client_close(struct sip_svc_controller *mailbox_smc_dev,
				uint32_t mailbox_client_token)
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
		LOG_ERR("Mailbox client close fail (%d)", err);
		k_free(cmd_addr);
	}

	return err;
}

static int handle_data(uint32_t *data, uint8_t mask, struct seu_intel_socfpga_data *priv_seu_data)
{
	uint32_t client_count;
	struct seu_err_data seu_data;
	struct ecc_err_data ecc_data;
	struct misc_err_data misc_data;
	struct pmf_err_data pmf_data;
	struct misc_sdm_err_data misc_sdm_data;
	struct emif_err_data emif_data;
	void *error_data;

	/* Total number of clients registered */
	client_count = priv_seu_data->seu_client_local.total_callback_func;

	if (client_count == 0) {
		LOG_ERR("No function callback has been registered");
		return -EINVAL;
	}

	switch (mask) {
	case SEU_ERROR_MODE:
		memset(&seu_data, 0, sizeof(struct seu_err_data));
		seu_data.sub_error_type = GET_SEU_ERR_READ_ERR_DATA_TYPE(data[INDEX_SEU_ERROR_3]);
		seu_data.sector_addr = GET_SEU_ERR_SECTOR_ERROR(data[INDEX_SEU_ERROR_2]);
		seu_data.correction_status =
			GET_SEU_ERR_READ_CORRECTION_STATUS(data[INDEX_SEU_ERROR_3]);
		seu_data.row_frame_index =
			GET_SEU_ERR_READ_ROW_FRAME_INDEX(data[INDEX_SEU_ERROR_3]);
		seu_data.bit_position = GET_SEU_ERR_READ_BIT_POS_FRAME(data[INDEX_SEU_ERROR_3]);
		error_data = (void *)&seu_data;
		break;

	case ECC_ERROR_MODE:
		memset(&ecc_data, 0, sizeof(struct ecc_err_data));
		ecc_data.sub_error_type = GET_SEU_ERR_READ_ERR_DATA_TYPE(data[INDEX_SEU_ERROR_3]);
		ecc_data.sector_addr = GET_SEU_ERR_SECTOR_ERROR(data[INDEX_SEU_ERROR_2]);
		ecc_data.correction_status =
			GET_SEU_ERR_READ_CORRECTION_STATUS(data[INDEX_SEU_ERROR_3]);
		ecc_data.ram_id_error = GET_ECC_ERR_DATA(data[INDEX_SEU_ERROR_3]);
		error_data = (void *)&ecc_data;
		break;

	case MISC_CNT_ERROR_MODE:
		memset(&misc_data, 0, sizeof(struct misc_err_data));
		misc_data.sub_error_type = GET_SEU_ERR_READ_ERR_DATA_TYPE(data[INDEX_SEU_ERROR_3]);
		misc_data.sector_addr = GET_SEU_ERR_SECTOR_ERROR(data[INDEX_SEU_ERROR_2]);
		misc_data.correction_status =
			GET_SEU_ERR_READ_CORRECTION_STATUS(data[INDEX_SEU_ERROR_3]);
		misc_data.cnt_type = GET_MISC_CNT_TYPE(data[INDEX_SEU_ERROR_3]);
		misc_data.status_code = GET_MISC_ERR_READ_CNT_TYPE(data[INDEX_SEU_ERROR_3]);
		error_data = (void *)&misc_data;
		break;

	case PMF_ERROR_MODE:
		memset(&pmf_data, 0, sizeof(struct pmf_err_data));
		pmf_data.sub_error_type = GET_SEU_ERR_READ_ERR_DATA_TYPE(data[INDEX_SEU_ERROR_3]);
		pmf_data.sector_addr = GET_SEU_ERR_SECTOR_ERROR(data[INDEX_SEU_ERROR_2]);
		pmf_data.correction_status =
			GET_SEU_ERR_READ_CORRECTION_STATUS(data[INDEX_SEU_ERROR_3]);
		pmf_data.status_code = GET_MISC_ERR_READ_CNT_TYPE(data[INDEX_SEU_ERROR_3]);
		error_data = (void *)&pmf_data;
		break;

	case MISC_SDM_ERROR_MODE:
		memset(&misc_sdm_data, 0, sizeof(struct misc_sdm_err_data));
		misc_sdm_data.sub_error_type =
			GET_SEU_ERR_READ_ERR_DATA_TYPE(data[INDEX_SEU_ERROR_3]);
		misc_sdm_data.sector_addr = GET_SEU_ERR_SECTOR_ERROR(data[INDEX_SEU_ERROR_2]);
		misc_sdm_data.correction_status =
			GET_SEU_ERR_READ_CORRECTION_STATUS(data[INDEX_SEU_ERROR_3]);
		misc_sdm_data.wdt_code = GET_WDT_ERROR_STATUS_TYPE(data[INDEX_SEU_ERROR_3]);
		error_data = (void *)&misc_sdm_data;
		break;

	case MISC_EMIF_ERROR_MODE:
		memset(&emif_data, 0, sizeof(struct emif_err_data));
		emif_data.sector_addr = GET_SEU_ERR_SECTOR_ERROR(data[INDEX_SEU_ERROR_2]);
		emif_data.emif_id = GET_EMIF_ID(data[INDEX_SEU_ERROR_3]);
		emif_data.source_id = GET_SOURCE_ID(data[INDEX_SEU_ERROR_3]);
		emif_data.emif_error_type = GET_EMIF_ERROR_TYPE(data[INDEX_SEU_ERROR_3]);
		emif_data.ddr_addr_msb = data[INDEX_SEU_ERROR_3];
		emif_data.ddr_addr_lsb = GET_EMIF_DDR_LSB(data[INDEX_SEU_ERROR_4]);
		error_data = (void *)&emif_data;
		break;

	default:
		LOG_ERR("Error type not valid");
		return -EINVAL;
	}

	k_mutex_lock(&(priv_seu_data->seu_mutex), K_FOREVER);

	for (int index = 0; index < client_count; index++) {
		if ((priv_seu_data->seu_client_local.seu_isr_callback_enable[index] == true) &&
		    (priv_seu_data->seu_client_local.seu_isr_callback_mode[index] == mask)) {
			priv_seu_data->seu_client_local.seu_isr_callback[index](error_data);
		}
	}
	k_mutex_unlock(&(priv_seu_data->seu_mutex));
	return 0;
}

static void seu_callback(uint32_t c_token, struct sip_svc_response *response)
{
	uint32_t error_code;
	uint32_t response_length;
	uint32_t resp_len;
	uint32_t error_detect_type;
	uint32_t *resp_data;
	int ret;

	struct private_data *priv = (struct private_data *)response->priv_data;

	priv->status = 0;

	if (response == NULL) {
		LOG_ERR("The callback response is NULL");
		k_sem_give(&(priv->semaphore));
		priv->status = -EINVAL;
		return;
	}

	resp_data = (uint32_t *)response->resp_data_addr;
	/* Right shift by 2 as the size is in bytes. */
	resp_len = response->resp_data_size >> 2;

	error_code = GET_SEU_ERR_READ_RES_HEADER_ERR_CODE(resp_data[INDEX_RESPONSE_HEADER]);
	if (priv->seu_commands == READ_SEU_ERROR) {
		response_length =
			GET_SEU_ERR_READ_RES_HEADER_LENGTH(resp_data[INDEX_RESPONSE_HEADER]);
		if (error_code == 0) {
			if ((response_length == 1) && (resp_data[INDEX_SEU_ERROR_1] == 0)) {
				LOG_INF("No error occur");
				priv->status = error_code;
			} else {
				if ((response_length == 3) || (response_length == 4)) {
					if (ERROR_FRAME_DETECT(resp_data[INDEX_SEU_ERROR_2]) != 0) {
						priv->status = error_code;
						LOG_ERR("Error detected parameter not zero");
					}
					error_detect_type =
						ERROR_FRAME_TYPE(resp_data[INDEX_SEU_ERROR_2]);
					ret = handle_data(resp_data, error_detect_type,
							  priv->seu_data_ptr);
					if (ret != 0) {
						LOG_ERR("The SEU callback function failed");
					}
					priv->status = ret;
				} else {
					LOG_ERR("Error in response");
				}
			}
		} else {
			LOG_ERR("Negative response code is 0x%x", error_code);
			priv->status = error_code;
		}

	} else if (priv->seu_commands == READ_SEU_STATS) {
		response_length =
			GET_SEU_ERR_READ_RES_HEADER_LENGTH(resp_data[INDEX_RESPONSE_HEADER]);
		if ((error_code == 0) && (response_length == 6)) {
			priv->seu_statistics.t_seu_cycle = resp_data[INDEX_SEU_CYCLE];
			priv->seu_statistics.t_seu_detect = resp_data[INDEX_SEU_DETECT];
			priv->seu_statistics.t_seu_correct = resp_data[INDEX_SEU_CORRECT];
			priv->seu_statistics.t_seu_inject_detect = resp_data[INDEX_SEU_INJECT];
			priv->seu_statistics.t_sdm_seu_poll_interval = resp_data[INDEX_SEU_POLL];
			priv->seu_statistics.t_sdm_seu_pin_toggle_overhead =
				resp_data[INDEX_SEU_PIN_TOGGLE];
		} else {
			memset(&priv->seu_statistics, 0, sizeof(struct seu_statistics_data));
			priv->status = -EINVAL;
		}

	} else {
		priv->status = error_code;
	}

	k_sem_give(&(priv->semaphore));
}

static int seu_send_sip_svc(uint32_t *cmd_addr, uint32_t cmd_size, uint32_t *resp_addr,
			    uint32_t resp_size, struct private_data *private_data)
{
	struct sip_svc_request request;
	int trans_id;
	int err;

	/* Initialize the semaphore */
	k_sem_init(&(private_data->semaphore), 0, 1);

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
	request.a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
	request.priv_data = private_data;
	request.resp_data_addr = (uint64_t)resp_addr;
	request.resp_data_size = (uint64_t)resp_size;
	request.a2 = (uint64_t)cmd_addr;
	request.a3 = (uint64_t)cmd_size;

	/* Opening SiP SVC session */
	err = svc_client_open(private_data->seu_data_ptr->mailbox_smc_dev,
			      private_data->seu_data_ptr->mailbox_client_token);
	if (err) {
		LOG_ERR("Client open failed!");
		k_free(cmd_addr);
		return err;
	}

	trans_id = sip_svc_send(private_data->seu_data_ptr->mailbox_smc_dev,
				private_data->seu_data_ptr->mailbox_client_token, &request,
				seu_callback);
	if (trans_id < 0) {
		LOG_ERR("SiP SVC send request fail");
		k_free(cmd_addr);
		return -EBUSY;
	}

	err = k_sem_take(&(private_data->semaphore), K_FOREVER);
	if (err != 0) {
		LOG_ERR("Error in taking semaphore");
		return -EINVAL;
	}

	err = svc_client_close(private_data->seu_data_ptr->mailbox_smc_dev,
			       private_data->seu_data_ptr->mailbox_client_token);
	if (err) {
		LOG_ERR("Unregistering & Closing failed");
		k_free(cmd_addr);
		return err;
	}

	err = private_data->status;

	return err;
}

int read_seu_error(struct seu_intel_socfpga_data *const seu_data)
{
	uint32_t *cmd_addr = NULL, *resp_addr = NULL;
	uint32_t cmd_size = (sizeof(uint32_t));
	uint32_t resp_size = (sizeof(uint32_t) * SEU_READ_RESPONSE_SIZE);
	struct private_data priv;
	int ret = 0;

	resp_addr = (uint32_t *)k_malloc(resp_size);
	if (resp_addr == NULL) {
		LOG_ERR("Failed to get memory");
		return -ENOSR;
	}

	cmd_addr = (uint32_t *)k_malloc(cmd_size);
	if (cmd_addr == NULL) {
		LOG_ERR("Failed to get memory");
		k_free(resp_addr);
		return -ENOSR;
	}

	cmd_addr[INDEX_CMD_0] = SET_FRAME_CMD(READ_SEU_ERROR_CMD);
	priv.seu_commands = READ_SEU_ERROR;
	priv.seu_data_ptr = seu_data;

	ret = seu_send_sip_svc(cmd_addr, cmd_size, resp_addr, resp_size, &priv);

	k_free(resp_addr);

	return ret;
}

static void seu_delayed_work(struct k_work *work)
{
	int ret;
	struct seu_intel_socfpga_data *seu_data;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	/* To get structure base address using structure member */
	seu_data = CONTAINER_OF(dwork, struct seu_intel_socfpga_data, seu_work_delay);

	ret = read_seu_error(seu_data);
	if (ret != 0) {
		LOG_ERR("SEU read error failed! - %x", ret);
	}

	/* After reading the SEU data from the FIFO, enable the interrupt */
	irq_enable(SEU_ERROR_IRQn);
}

static void seu_irq_handler(const struct device *dev)
{
	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	/*
	 * Disable the interrupt while waiting for data to be read from the FIFO else it will
	 * keep interrupting the system.
	 */
	irq_disable(SEU_ERROR_IRQn);

	/* Schedule the work item */
	k_work_schedule(&(seu_data->seu_work_delay), K_NO_WAIT);
}

static int intel_socfpga_seu_callback_function_register(const struct device *dev,
							seu_isr_callback_t func,
							enum seu_reg_mode mode, uint32_t *client)
{
	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	if ((client == NULL) || (func == NULL)) {
		LOG_ERR("Input parameters value null");
		return -EINVAL;
	}

	if (seu_data->seu_client_local.total_callback_func > CONFIG_SEU_MAX_CLIENT) {
		LOG_ERR("Unable to register a callback as the maximum count has been reached");
		return -EINVAL;
	}

	k_mutex_lock(&(seu_data->seu_mutex), K_FOREVER);

	/* Registering the SEU callback function */
	seu_data->seu_client_local
		.seu_isr_callback[seu_data->seu_client_local.total_callback_func] = func;
	seu_data->seu_client_local
		.seu_isr_callback_mode[seu_data->seu_client_local.total_callback_func] = mode;
	*client = seu_data->seu_client_local.total_callback_func;
	seu_data->seu_client_local.total_callback_func++;

	k_mutex_unlock(&(seu_data->seu_mutex));

	return 0;
}

static int intel_socfpga_seu_callback_function_enable(const struct device *dev, uint32_t client)
{
	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	k_mutex_lock(&(seu_data->seu_mutex), K_FOREVER);
	if (seu_data->seu_client_local.total_callback_func < client) {
		LOG_ERR("No client registration found!");
		k_mutex_unlock(&(seu_data->seu_mutex));
		return -EINVAL;
	}
	seu_data->seu_client_local.seu_isr_callback_enable[client] = true;
	k_mutex_unlock(&(seu_data->seu_mutex));

	return 0;
}

static int intel_socfpga_seu_callback_function_disable(const struct device *dev, uint32_t client)
{
	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	k_mutex_lock(&(seu_data->seu_mutex), K_FOREVER);
	if (seu_data->seu_client_local.total_callback_func < client) {
		LOG_ERR("No client registration found!");
		k_mutex_unlock(&(seu_data->seu_mutex));
		return -EINVAL;
	}
	seu_data->seu_client_local.seu_isr_callback_enable[client] = false;
	k_mutex_unlock(&(seu_data->seu_mutex));

	return 0;
}

static int intel_socfpga_insert_safe_seu_error(const struct device *dev,
					       struct inject_safe_seu_error_frame *error_frame)
{
	uint32_t *cmd_addr = NULL, *resp_addr = NULL;
	uint32_t cmd_size = (sizeof(uint32_t) * INSERT_SAFE_CMD_SIZE);
	uint32_t resp_size = (sizeof(uint32_t) * INSERT_SAFE_RESP_SIZE);
	struct private_data priv;
	int ret;

	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	if (error_frame == NULL) {
		LOG_ERR("Input parameter value null");
		return -EINVAL;
	}

	resp_addr = (uint32_t *)k_malloc(resp_size);

	if (resp_addr == NULL) {
		LOG_ERR("Failed to get memory");
		return -ENOSR;
	}

	cmd_addr = (uint32_t *)k_malloc(cmd_size);

	if (cmd_addr == NULL) {
		LOG_ERR("Failed to get memory");
		k_free(resp_addr);
		return -ENOSR;
	}

	cmd_addr[INDEX_CMD_0] = SET_FRAME_CMD(SEU_INSERT_SAFE_CMD) |
				SET_SEU_HEADER_LENGTH(SEU_INSERT_SAFE_ERROR_LENGTH);
	cmd_addr[INDEX_CMD_1] = SET_SECTOR_ADDR(error_frame->sector_address) |
				SET_INJECT_TYPE(error_frame->inject_type) |
				SET_NUMBER_OF_INJECTION(error_frame->number_of_injection);
	cmd_addr[INDEX_CMD_2] =
		SET_CRAM_SEL_O(error_frame->cram_sel_0) | SET_CRAM_SEL_1(error_frame->cram_sel_1);
	priv.seu_commands = INSERT_SAFE_SEU_ERROR;
	priv.seu_data_ptr = seu_data;

	ret = seu_send_sip_svc(cmd_addr, cmd_size, resp_addr, resp_size, &priv);
	k_free(resp_addr);

	return ret;
}

static int intel_socfpga_insert_seu_error(const struct device *dev,
					  struct inject_seu_error_frame *error_frame)
{
	uint32_t *cmd_addr = NULL, *resp_addr = NULL;
	uint32_t cmd_size =
		(sizeof(uint32_t) * (INJECT_SEU_ERR_CMD_SIZE + error_frame->error_inject));
	uint32_t resp_size = (sizeof(uint32_t) * INJECT_SEU_ERR_RES_SIZE);
	struct private_data priv;
	int ret;

	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	if (error_frame == NULL) {
		LOG_ERR("Input parameter value null");
		return -EINVAL;
	}

	resp_addr = (uint32_t *)k_malloc(resp_size);
	if (resp_addr == NULL) {
		LOG_ERR("Failed to get memory");
		return -ENOSR;
	}

	cmd_addr = (uint32_t *)k_malloc(cmd_size);
	if (cmd_addr == NULL) {
		LOG_ERR("Failed to get memory");
		k_free(resp_addr);
		return -ENOSR;
	}

	cmd_addr[INDEX_CMD_0] =
		SET_FRAME_CMD(SEU_INSERT_ERR_CMD) |
		SET_SEU_HEADER_LENGTH(SEU_INSERT_ERROR_LENGTH + error_frame->error_inject);
	cmd_addr[INDEX_CMD_1] = SET_SECTOR_ADDR(error_frame->sector_address) |
				SET_NUMBER_OF_INJECTION(error_frame->error_inject);
	for (int index = 0; index <= error_frame->error_inject; index++) {
		cmd_addr[INDEX_CMD_2 + index] = error_frame->frame[index].seu_frame_data;
	}
	priv.seu_commands = INSERT_SEU_ERROR;
	priv.seu_data_ptr = seu_data;

	ret = seu_send_sip_svc(cmd_addr, cmd_size, resp_addr, resp_size, &priv);
	k_free(resp_addr);

	return ret;
}

static int intel_socfpga_insert_ecc_error(const struct device *dev,
					  struct inject_ecc_error_frame *ecc_error_frame)
{
	uint32_t *cmd_addr = NULL, *resp_addr = NULL;
	uint32_t cmd_size = (sizeof(uint32_t) * INSERT_ECC_CMD_SIZE);
	uint32_t resp_size = (sizeof(uint32_t));
	struct private_data priv;
	int ret;

	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	if (ecc_error_frame == NULL) {
		LOG_ERR("Input parameter value null");
		return -EINVAL;
	}

	resp_addr = (uint32_t *)k_malloc(resp_size);
	if (resp_addr == NULL) {
		LOG_ERR("Failed to get memory");
		return -ENOSR;
	}

	cmd_addr = (uint32_t *)k_malloc(cmd_size);
	if (cmd_addr == NULL) {
		LOG_ERR("Failed to get memory");
		k_free(resp_addr);
		return -ENOSR;
	}

	cmd_addr[INDEX_CMD_0] = SET_FRAME_CMD(SEU_INSERT_ECC_ERR_CMD) |
				SET_SEU_HEADER_LENGTH(SEU_INSERT_ECC_ERR_LENGTH);
	cmd_addr[INDEX_CMD_1] = SET_SECTOR_ADDR(ecc_error_frame->sector_address) |
				SET_NUMBER_OF_ECC_INJECTION(ecc_error_frame->inject_type) |
				SET_ECC_RAM_ID(ecc_error_frame->ram_id);
	priv.seu_commands = INSERT_ECC_ERROR;
	priv.seu_data_ptr = seu_data;

	ret = seu_send_sip_svc(cmd_addr, cmd_size, resp_addr, resp_size, &priv);

	k_free(resp_addr);

	return ret;
}

static int intel_socfpga_read_seu_statistics(const struct device *dev, uint8_t sector,
					     struct seu_statistics_data *seu_statistics)
{
	uint32_t *cmd_addr = NULL, *resp_addr = NULL;
	int32_t ret;
	uint32_t cmd_size = (sizeof(uint32_t) * READ_SEU_STAT_CMD_SIZE);
	uint32_t resp_size = (sizeof(uint32_t) * READ_SEU_STAT_RES_SIZE);
	struct private_data priv;

	struct seu_intel_socfpga_data *const seu_data =
		(struct seu_intel_socfpga_data *const)dev->data;

	if (seu_statistics == NULL) {
		LOG_ERR("Input parameter value null");
		return -EINVAL;
	}

	resp_addr = (uint32_t *)k_malloc(resp_size);
	if (resp_addr == NULL) {
		LOG_ERR("Failed to get memory");
		return -ENOSR;
	}

	cmd_addr = (uint32_t *)k_malloc(cmd_size);
	if (cmd_addr == NULL) {
		LOG_ERR("Failed to get memory");
		k_free(resp_addr);
		return -ENOSR;
	}

	cmd_addr[INDEX_CMD_0] = SET_FRAME_CMD(SEU_READ_STATS_CMD) |
				SET_SEU_HEADER_LENGTH(SEU_READ_STATISTICS_LENGTH);
	cmd_addr[INDEX_CMD_1] = SET_SECTOR_ADDR(sector);
	priv.seu_commands = READ_SEU_STATS;
	priv.seu_data_ptr = seu_data;
	ret = seu_send_sip_svc(cmd_addr, cmd_size, resp_addr, resp_size, &priv);
	memcpy(seu_statistics, &priv.seu_statistics, sizeof(struct seu_statistics_data));
	k_free(resp_addr);

	return ret;
}

static int seu_intel_socfpga_init(const struct device *dev)
{
	int ret;

	struct seu_intel_socfpga_data *const seu_data_ptr =
		(struct seu_intel_socfpga_data *const)dev->data;

	/* Initialize the client count to zero */
	seu_data_ptr->seu_client_local.total_callback_func = 0;
	/* Initialize the Mutex */
	ret = k_mutex_init(&(seu_data_ptr->seu_mutex));

	if (ret != 0) {
		LOG_ERR("SEU mutex creation failed");
		return ret;
	}

	seu_data_ptr->mailbox_smc_dev = sip_svc_get_controller(SVC_METHOD);
	if (!seu_data_ptr->mailbox_smc_dev) {
		LOG_ERR("Arm SiP service not found");
		return -ENODEV;
	}

	seu_data_ptr->mailbox_client_token = sip_svc_register(seu_data_ptr->mailbox_smc_dev, NULL);
	if (seu_data_ptr->mailbox_client_token == SIP_SVC_ID_INVALID) {
		seu_data_ptr->mailbox_smc_dev = NULL;
		LOG_ERR("Mailbox client register fail");
		return -EINVAL;
	}

	/* Initialize the work queue*/
	k_work_init_delayable(&(seu_data_ptr->seu_work_delay), seu_delayed_work);

	/* Enable interrupt for Single Event Upsets (SEU)*/
	IRQ_CONNECT(SEU_ERROR_IRQn, SEU_PRIORITY, seu_irq_handler, DEVICE_DT_GET(DT_NODELABEL(seu)),
		    SEU_IRQ_FLAGS);
	irq_enable(SEU_ERROR_IRQn);

	LOG_INF(" SEU driver initialized successfully");
	return 0;
}

static const struct seu_api api = {
	.seu_callback_function_register = intel_socfpga_seu_callback_function_register,
	.seu_callback_function_enable = intel_socfpga_seu_callback_function_enable,
	.seu_callback_function_disable = intel_socfpga_seu_callback_function_disable,
	.insert_safe_seu_error = intel_socfpga_insert_safe_seu_error,
	.insert_seu_error = intel_socfpga_insert_seu_error,
	.insert_ecc_error = intel_socfpga_insert_ecc_error,
	.read_seu_statistics = intel_socfpga_read_seu_statistics,
};

static struct seu_intel_socfpga_data seu_data;

DEVICE_DT_DEFINE(DT_SEU, seu_intel_socfpga_init, NULL, &seu_data, NULL, POST_KERNEL,
		 CONFIG_SEU_INIT_PRIORITY, &api);
