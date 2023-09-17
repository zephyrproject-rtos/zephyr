/*io96b.c for Intel IO96B ECC driver*/

/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_io96b

#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "edac.h"
#include "io96b_ecc.h"

LOG_MODULE_REGISTER(io96b, CONFIG_IO96B_LOG_LEVEL);

typedef void (*io96b_config_irq_t)(const struct device *port);
typedef void (*io96b_enable_irq_t)(const struct device *port, bool en);

/*
 * IO96B ECC driver runtime data
 *
 * @num_mem_intf:	Number of memory interfaces instantiated
 * @mem_intf_info:  IP type and IP identifier for every IP instance
 *                  implemented on the IO96B
 * @ecc_info_cb: Pointer to call back function. This call back function
 *               will be register by EDAC module. It will be invoked by
 *               IO96B driver When an ECC error interrupt occurs.
 * @cb_usr_data: Callback function user data pointer. It will be
 *               an argument when the callback function is invoked.
 */
struct io96b_data {
	DEVICE_MMIO_RAM;
	uint8_t num_mem_intf;
	uint8_t mem_intf_info[MAX_INTERFACES];
	edac_callback_t ecc_info_cb;
	struct io96b_ecc_info ecc_info;
	void *cb_usr_data;
	int sbe_count;
	bool init_status;
};
/*
 * IO96B ECC driver configuration data
 *
 * @irq_config_fn: Pointer to IO96B interrupt configuration function.
 * @irq_enable_fn: Pointer to IO96B interrupt enable function.
 */
struct io96b_config {
	DEVICE_MMIO_ROM;
	uint32_t max_ecc_buff_entries;
	uint32_t max_producer_count_val;
	io96b_config_irq_t irq_config_fn;
	io96b_enable_irq_t irq_enable_fn;
};

static inline int wait_for_cmnd_resp_ready(mem_addr_t reg_addr, uint32_t reg_mask)
{
	uint8_t timeout = CMD_RESP_TIMEOUT;
	uint32_t reg_val;

	while (timeout) {
		reg_val = sys_read32(reg_addr);
		if (reg_val & reg_mask) {
			return 0;
		}
		k_sleep(K_MSEC(1u));
		timeout--;
	}
	return -EBUSY;
}

/** @brief Function called to send an IO96B mailbox command.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param req_resp pointer ro io96b mailbox command request and response
 *                 data structure.
 *
 * @return 0 if the write is accepted, or a negative error code.
 *         -EBUSY if there is previous command processing is in
 *                progress or command timeout occurs
 *         -EINVAL if input values are invalid
 */
int io96b_mb_request(const struct device *dev, struct io96b_mb_req_resp *req_resp)
{
	struct io96b_data *data = dev->data;
	mem_addr_t ioaddr = (mem_addr_t)DEVICE_MMIO_GET(dev);
	uint32_t reg_val;
	int ret;

	if ((req_resp->req.usr_cmd_type != CMD_GET_SYS_INFO) &&
	    (req_resp->req.io96b_intf_inst_num >= data->num_mem_intf)) {
		LOG_DBG("Invalid interface instance number.Maximum interfaces per IO96B IP are %d",
			data->num_mem_intf);
		return -EINVAL;
	}

	switch (req_resp->req.usr_cmd_type) {
	case CMD_GET_SYS_INFO:
		switch (req_resp->req.usr_cmd_opcode) {
		case GET_MEM_INTF_INFO:
			break;
		default:
			LOG_DBG("Invalid command opcode requested");
			return -EINVAL;
		}
		reg_val = (req_resp->req.usr_cmd_opcode << 0) | (req_resp->req.usr_cmd_type << 16);
		sys_write32(reg_val, ioaddr + IO96B_CMD_REQ_OFFSET);
		break;
	case CMD_TRIG_CONTROLLER_OP:
		switch (req_resp->req.usr_cmd_opcode) {
		case ECC_ENABLE_SET:
		case ECC_INJECT_ERROR:
			sys_write32(req_resp->req.cmd_param_0, ioaddr + IO96B_CMD_PARAM_0_OFFSET);
		case ECC_ENABLE_STATUS:
			break;
		default:
			LOG_DBG("Invalid command opcode requested");
			return -EINVAL;
		}
		reg_val = (req_resp->req.usr_cmd_opcode << 0) | (req_resp->req.usr_cmd_type << 16) |
			  (data->mem_intf_info[req_resp->req.io96b_intf_inst_num] << 24);
		sys_write32(reg_val, ioaddr + IO96B_CMD_REQ_OFFSET);
		break;

	default:
		LOG_DBG("Invalid command type requested");
		return -EINVAL;
	}

	ret = wait_for_cmnd_resp_ready(ioaddr + IO96B_CMD_RESPONSE_STATUS_OFFSET,
				       IO96B_STATUS_COMMAND_RESPONSE_READY);

	if (ret) {
		LOG_DBG("Command response timedout");
		return ret;
	}

	req_resp->resp.cmd_resp_status = sys_read32(ioaddr + IO96B_CMD_RESPONSE_STATUS_OFFSET);
	req_resp->resp.cmd_resp_data_0 = sys_read32(ioaddr + IO96B_CMD_RESPONSE_DATA_0_OFFSET);
	req_resp->resp.cmd_resp_data_1 = sys_read32(ioaddr + IO96B_CMD_RESPONSE_DATA_1_OFFSET);
	req_resp->resp.cmd_resp_data_2 = sys_read32(ioaddr + IO96B_CMD_RESPONSE_DATA_2_OFFSET);

	return 0;
}

/*
 * Initial function to be called to set memory interface IP type and instance ID
 * IP type and instance ID need to be determined before sending mailbox command
 */
static int io96b_init(const struct device *dev)
{
	struct io96b_data *data = dev->data;
	const struct io96b_config *config = dev->config;
	struct io96b_mb_req_resp req_resp;
	int i, ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	memset(&req_resp, 0, sizeof(struct io96b_mb_req_resp));
	/*
	 * Get memory interface IP type & instance ID (IP identifier)
	 */
	req_resp.req.usr_cmd_type = CMD_GET_SYS_INFO;
	req_resp.req.usr_cmd_opcode = GET_MEM_INTF_INFO;
	ret = io96b_mb_request(dev, &req_resp);

	if (ret) {
		LOG_DBG("%s : IO96B mailbox init failed", dev->name);
		return ret;
	}

	data->num_mem_intf = IO96B_CMD_RESPONSE_DATA_SHORT(req_resp.resp.cmd_resp_status) &
			     IO96B_GET_MEM_INFO_NUM_USED_MEM_INF_MASK;

	if (!data->num_mem_intf) {
		LOG_DBG("%s : IO96B mailbox init failed. Invalid number of memory instances",
			dev->name);
		return -EIO;
	}

	for (i = 0; i < data->num_mem_intf; i++) {
		switch (i) {
		case 0:
			data->mem_intf_info[0u] =
				IO96B_CMD_RESPONSE_MEM_INFO(req_resp.resp.cmd_resp_data_0);
			break;

		case 1:
			data->mem_intf_info[1u] =
				IO96B_CMD_RESPONSE_MEM_INFO(req_resp.resp.cmd_resp_data_1);
			break;
		default:
			/*Do nothing*/
		}
	}
	(config->irq_config_fn)(dev);
	(config->irq_enable_fn)(dev, true);

	data->init_status = true;

	return 0;
}

/** @brief Function called to read the ECC error information.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param errs_data array pointer to copy ECC errors information.
 * @param errs_cnt number of ECC errors
 */
static void io96b_read_ecc_err_info(const struct device *dev, struct io96b_ecc_data *errs_data,
				    uint32_t errs_cnt)
{
	const struct io96b_config *config = dev->config;
	mem_addr_t ioaddr = (mem_addr_t)DEVICE_MMIO_GET(dev);
	uint32_t entry_cnt = 0;
	uint32_t consumer_ctr;

	while (errs_cnt--) {
		errs_data->word0 = sys_read32(ioaddr + IO96B_ECC_BUF_ENTRY_WORD0_OFFSET(entry_cnt));
		errs_data->word1 = sys_read32(ioaddr + IO96B_ECC_BUF_ENTRY_WORD1_OFFSET(entry_cnt));
		errs_data++;
		entry_cnt++;
	}
	/*
	 * Increment the consumer counter value to free the buffer.
	 */
	consumer_ctr = sys_read32(ioaddr + IO96B_ECC_BUF_CONSUMER_CNTR_OFFSET);
	consumer_ctr = consumer_ctr + entry_cnt;
	if (consumer_ctr > config->max_producer_count_val) {
		consumer_ctr = consumer_ctr - config->max_producer_count_val;
	}
	sys_write32(consumer_ctr, ioaddr + IO96B_ECC_BUF_CONSUMER_CNTR_OFFSET);
}

/** @brief Function called to get the latest ECC errors count.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return value >= 0 ECC errors count value
 *         -1 for invalid producer or consumer counter value
 */
static int io96b_get_ecc_err_cnt(const struct device *dev)
{
	const struct io96b_config *config = dev->config;
	uint32_t producer_ctr;
	uint32_t consumer_ctr;
	mem_addr_t ioaddr = (mem_addr_t)DEVICE_MMIO_GET(dev);
	int cnt = 0;

	producer_ctr = sys_read32(ioaddr + IO96B_ECC_BUF_PRODUCER_CNTR_OFFSET);
	consumer_ctr = sys_read32(ioaddr + IO96B_ECC_BUF_CONSUMER_CNTR_OFFSET);

	if (producer_ctr < config->max_producer_count_val &&
	    consumer_ctr < config->max_producer_count_val) {
		if (producer_ctr >= consumer_ctr) {
			cnt = (producer_ctr - consumer_ctr);
		} else {
			cnt = ((config->max_producer_count_val - consumer_ctr) + producer_ctr);
		}
	} else {
		LOG_ERR("%s : ECC producer or consumer counter value out of range\nproducer "
			"counter = "
			"0x%x producer_ctr\n consumer counter = 0x%x",
			dev->name, producer_ctr, consumer_ctr);
		return -ERANGE;
	}

	return cnt;
}

/** @brief Function read ECC error information buffer overflow status.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return overflow status
 */
static uint32_t io96b_read_ecc_errs_ovf(const struct device *dev)
{
	mem_addr_t ioaddr = (mem_addr_t)DEVICE_MMIO_GET(dev);

	return sys_read32(ioaddr + IO96B_ECC_RING_BUF_OVRFLOW_STATUS_OFFSET);
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
/**
 * @brief To inject ecc error in a HPS peripheral RAM specified by ecc_modules_id
 *
 * @param dev			Pointer to the device structure for the driver.
 * @param emif_id		EMIF interface ID
 * @param error_type	error type DBE/SBE
 *
 * @return 0 Error injection is success
 *			-EINVAL in case of invalid error_type parameter
 *			-ENODEV in case of ECC is not initialised or disabled
 */
static int io96b_ecc_inject_error(const struct device *dev, uint32_t emif_id, uint32_t error_type)
{
	struct io96b_data *data = dev->data;
	struct io96b_mb_req_resp req_resp;
	int err;

	if (data->init_status != true) {
		LOG_DBG("%s : IO96B ECC not initialized or disabled", dev->name);
		return -ENODEV;
	}

	memset(&req_resp, 0, sizeof(struct io96b_mb_req_resp));

	req_resp.req.io96b_intf_inst_num = emif_id;
	req_resp.req.usr_cmd_type = CMD_TRIG_CONTROLLER_OP;
	req_resp.req.usr_cmd_opcode = ECC_INJECT_ERROR;
	if (error_type == INJECT_DBE) {
		req_resp.req.cmd_param_0 = ECC_DBE_SYNDROME;
	} else {
		req_resp.req.cmd_param_0 = ECC_SBE_SYNDROME;
	}
	err = io96b_mb_request(dev, &req_resp);
	if (err) {
		LOG_DBG("%s : IO96B inject ECC error failed", dev->name);
		return err;
	}
	return 0;
}
#endif

/**
 * @brief to get the single bit error count
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param emif_id	EMIF interface ID
 *
 * @return  >= 0 Get SB error count success and count value equals to return value
 *			-ENODEV in case of ECC is not initialised or disabled
 */
int io96b_get_sbe_ecc_error_cnt(const struct device *dev, uint32_t ecc_modules_id)
{

	ARG_UNUSED(ecc_modules_id);
	struct io96b_data *data = dev->data;

	if (data->init_status != true) {
		LOG_DBG("%s : IO96B not initialized or disabled", dev->name);
		return -ENODEV;
	}

	return data->sbe_count;
}

/** @brief Function to set a call back function for reporting ECC error.
 *  this call back will be called from io96b ISR if an ECC error occurs.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param cb		callback function
 * @param user_data Pointer to the ECC info buffer.
 *
 * @return	0 if callback function set is success
 *			-EINVAL if invalid value passed for input parameter 'cb'.
 */
static int io96b_set_ecc_error_cb(const struct device *dev, edac_callback_t cb, void *user_data)
{
	struct io96b_data *data = dev->data;
	int ret = 0;

	if (data->init_status != true) {
		LOG_DBG("%s : IO96B ECC not initialized or disabled", dev->name);
		return -ENODEV;
	}

	if (cb != NULL) {
		data->ecc_info_cb = cb;
		data->cb_usr_data = user_data;
	} else {
		ret = -EINVAL;
	}
	return ret;
}

static void io96b_isr(const struct device *dev)
{
	struct io96b_data *data = dev->data;
	const struct io96b_config *config = dev->config;
	bool dbe = false, sbe = false;
	int err_cnt;
	uint32_t error_type;
	uint32_t emif_id;
	uint32_t error_add = 0;

	/*
	 * Read the ECC information and invoke the callback function to EDAC module.
	 */
	err_cnt = io96b_get_ecc_err_cnt(dev);
	if ((err_cnt > 0) && (err_cnt <= config->max_ecc_buff_entries)) {
		/* The difference between producer counter and consumer counter
		 * should always be less than or equal to maximum size of the ECC buffer.
		 * If new ECC error occurred and if the difference between consumer and
		 * producer counter is equal to maximum size of the ECC buffer, then overflow
		 * flag will be set and new ECC error info will be discarded
		 */
		io96b_read_ecc_err_info(dev, data->ecc_info.buff, err_cnt);
		data->ecc_info.err_cnt = err_cnt;
		data->ecc_info.ovf_status = io96b_read_ecc_errs_ovf(dev);
		if (data->ecc_info_cb != NULL) {
			for (uint32_t i = 0; i < data->ecc_info.err_cnt; i++) {
				/* Bit field 9:6 in Word0 contains the error type */
				error_type = (data->ecc_info.buff[i].word0 & ECC_ERROR_TYPE_MASK) >>
					     ECC_ERROR_TYPE_BIT_OFST;
				/* Bit field 24:27 in word0 contains EMIF ID*/
				emif_id = (data->ecc_info.buff[i].word0 & ECC_EMIF_ID_MASK) >>
					  ECC_EMIF_ID_BIT_OFST;
				/* Word1 contains ECC error ADDRESS LSB*/
				error_add = data->ecc_info.buff[i].word1;
				switch (error_type) {
				case ECC_RMW_READ_LINK_DBE:
				case ECC_READ_LINK_DBE:
				case ECC_WRITE_LINK_DBE:
				case ECC_MULTI_DBE:
				case ECC_SINGLE_DBE:
					/*Set dbe true for all kind of DBE error types*/
					dbe = true;
					break;
				case ECC_READ_LINK_SBE:
				case ECC_WRITE_LINK_SBE:
				case ECC_MULTI_SBE:
				case ECC_SINGLE_SBE:
					/*Set sbe true for all kind of SBE error types*/
					sbe = true;
					data->sbe_count++;
					break;
				default:
				}
				LOG_DBG("%s : An ECC error detected at 0x%x, EMIF "
					"ID: %d, error type: %d ",
					dev->name, error_add, emif_id, error_type);
				data->ecc_info_cb(dev, dbe, sbe, data->cb_usr_data);
			}
		} else {
			LOG_DBG("%s : Invalid call back function", dev->name);
		}
		LOG_DBG("%s : %d ECC errors occurred ", dev->name, err_cnt);
	} else {
		/* The difference between producer counter and consumer counter
		 * should always be less than or equal to maximum size of the ECC buffer
		 */
		LOG_ERR("%s : %d Invalid ECC errors count ", dev->name, err_cnt);
	}
}

static const struct edac_ecc_driver_api io96b_driver_api = {
#if defined(CONFIG_EDAC_ERROR_INJECT)
	.inject_ecc_error = io96b_ecc_inject_error,
#endif
	.set_ecc_error_cb = io96b_set_ecc_error_cb,
};

/* Interrupt configuration function macro */
#define IO96B_CONFIG_IRQ_FUNC(inst)                                                                \
	static void io96b##inst##_irq_config(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), io96b_isr,            \
			    DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, flags));                   \
	}                                                                                          \
	static void io96b##inst##_irq_enable(const struct device *dev, bool en)                    \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		en ? irq_enable(DT_INST_IRQN(inst)) : irq_disable(DT_INST_IRQN(inst));             \
	}

#define CREATE_IO96B_DEV(inst)                                                                     \
	IO96B_CONFIG_IRQ_FUNC(inst)                                                                \
	struct io96b_ecc_data                                                                      \
		io96b##inst##_ecc_data_buff[DT_INST_PROP(inst, max_ecc_buff_entries)];             \
	static const struct io96b_config iosmm_mb_cfg_##inst = {                                   \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.max_ecc_buff_entries = DT_INST_PROP(inst, max_ecc_buff_entries),                  \
		.max_producer_count_val = DT_INST_PROP(inst, producer_counter_cap),                \
		.irq_config_fn = io96b##inst##_irq_config,                                         \
		.irq_enable_fn = io96b##inst##_irq_enable,                                         \
	};                                                                                         \
	static struct io96b_data io96b_data_##inst = {                                             \
		.ecc_info.buff = io96b##inst##_ecc_data_buff,                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, io96b_init, NULL, &io96b_data_##inst, &iosmm_mb_cfg_##inst,    \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                    \
			      &io96b_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_IO96B_DEV);
