/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

/* supported mailbox command types */
#define CMD_GET_SYS_INFO       (0x01)
#define CMD_TRIG_CONTROLLER_OP (0x04)

/* supported mailbox command opcodes */
#define GET_MEM_INTF_INFO (0x0001u)
#define ECC_ENABLE_SET    (0x0101u)
#define ECC_ENABLE_STATUS (0x0102u)
#define ECC_INJECT_ERROR  (0x0109u)

/**
 * @brief IO96B mailbox response outputs
 *
 * @cmd_resp_status: Command Interface status
 * @cmd_resp_data_*: More spaces for command response
 */
struct io96b_mb_resp {
	uint32_t cmd_resp_status;
	uint32_t cmd_resp_data_0;
	uint32_t cmd_resp_data_1;
	uint32_t cmd_resp_data_2;
};

/**
 * @brief IO96B mailbox request inputs
 *
 * @io96b_intf_inst_num: EMIF Instance ID
 * @usr_cmd_type: User input command type
 * @usr_cmd_opcode: user input command opcode enum
 * @cmd_param_: optional command parameters
 */
struct io96b_mb_req {
	uint32_t io96b_intf_inst_num;
	uint32_t usr_cmd_type;
	uint32_t usr_cmd_opcode;
	uint32_t cmd_param_0;
	uint32_t cmd_param_1;
	uint32_t cmd_param_2;
	uint32_t cmd_param_3;
	uint32_t cmd_param_4;
	uint32_t cmd_param_5;
	uint32_t cmd_param_6;
};
/**
 * @brief mail box request and response structure
 */
struct io96b_mb_req_resp {
	/*
	 * mail box request structure
	 */
	struct io96b_mb_req req;
	/*
	 * mail box response structure
	 */
	struct io96b_mb_resp resp;
};

/**
 * @brief data
 * @Word0:	5:0		ECC Error Address MSB
 *			9:6		ECC Error Type
 *			16:10	Source Transaction AXI ID
 *			24:17	EMIF Instance ID
 *			31:25	Reserved
 * @Word1:  31:0	ECC Error Address LSB
 */
struct io96b_ecc_data {
	uint32_t word0;
	uint32_t word1;
};
/**
 * @brief ECC error information
 * @buff: Pointer to ECC error data buffer.
 * @err_cnt: ECC error count.
 * @ovf_status: ECC errors overflow status.
 */
struct io96b_ecc_info {
	struct io96b_ecc_data *buff;
	uint32_t err_cnt;
	uint32_t ovf_status;
};

/**
 * @brief Define the callback function signature for
 * `set_ecc_error_cb()` function.
 *
 * @param dev       IO96B device structure
 * @param state     Pointer to io96b_ecc_info structure.
 * @param user_data Pointer to data specified by user
 */
typedef void (*io96b_callback_t)(const struct device *dev, struct io96b_ecc_info *ecc_info,
				 void *user_data);

/**
 * @brief Function called to send an IO96B mailbox command.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param req_resp pointer to IO96B mailbox command request and response
 *                 data structure.
 *
 * @return 0 if the write is accepted, or a negative error code.
 *         -EBUSY if there is previous command processing is in
 *                progress or command timeout occurs
 *         -EINVAL if input values are invalid
 */
typedef int (*io96b_mb_req_t)(const struct device *dev, struct io96b_mb_req_resp *req_resp);

/**
 * @brief Function to set a callback function for reporting ECC error.
 *        The callback will be called from IO96B ISR if an ECC error occurs.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cb ECC error count, describes ECC info buffer
 * @param user_data Pointer to the ECC info buffer.
 *
 * @return -EINVAL if invalid value passed for input parameter 'cb'.
 */
typedef int (*io96b_set_ecc_error_cb_t)(const struct device *dev, io96b_callback_t cb,
					void *user_data);

struct io96b_driver_api {
	io96b_mb_req_t mb_cmnd_send;
	io96b_set_ecc_error_cb_t set_ecc_error_cb;
};
