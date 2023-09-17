/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_IO96B_ECC_H_
#define ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_IO96B_ECC_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define MAX_INTERFACES (0x2u)

/* IO96B register address offset from the base address */
#define IO96B_CMD_RESPONSE_STATUS_OFFSET 0x45C
#define IO96B_CMD_RESPONSE_DATA_0_OFFSET 0x458
#define IO96B_CMD_RESPONSE_DATA_1_OFFSET 0x454
#define IO96B_CMD_RESPONSE_DATA_2_OFFSET 0x450
#define IO96B_CMD_REQ_OFFSET             0x43C
#define IO96B_CMD_PARAM_0_OFFSET         0x438
#define IO96B_CMD_PARAM_1_OFFSET         0x434
#define IO96B_CMD_PARAM_2_OFFSET         0x430
#define IO96B_CMD_PARAM_3_OFFSET         0x42C
#define IO96B_CMD_PARAM_4_OFFSET         0x428
#define IO96B_CMD_PARAM_5_OFFSET         0x424
#define IO96B_CMD_PARAM_6_OFFSET         0x420
#define IO96B_STATUS_OFFSET              0x400

#define IO96B_ECC_BUF_PRODUCER_CNTR_OFFSET       0x550
#define IO96B_ECC_BUF_CONSUMER_CNTR_OFFSET       0x554
#define IO96B_ECC_RING_BUF_OVRFLOW_STATUS_OFFSET 0x558
#define IO96B_ECC_BUF_ENTRIES_OFFSET             0x560
#define IO96B_ECC_BUF_ENTRY_WORD0_OFFSET(entry)                                                    \
	(IO96B_ECC_BUF_ENTRIES_OFFSET + (entry * 8)) /* each entry size is 8 bytes */
#define IO96B_ECC_BUF_ENTRY_WORD1_OFFSET(entry)                                                    \
	(IO96B_ECC_BUF_ENTRIES_OFFSET + (entry * 8) + 4) /* each entry size is 8 bytes */

/* Macros for operations of IO96B registers */
#define IO96B_CMD_RESPONSE_DATA_SHORT_MASK GENMASK(31, 16)
#define IO96B_CMD_RESPONSE_DATA_SHORT(data)                                                        \
	((uint16_t)(((data)&IO96B_CMD_RESPONSE_DATA_SHORT_MASK) >> 16))
#define IO96B_CMD_RESPONSE_MEM_INFO_MASK GENMASK(31, 24)
#define IO96B_CMD_RESPONSE_MEM_INFO(data)                                                          \
	((uint8_t)(((data)&IO96B_CMD_RESPONSE_MEM_INFO_MASK) >> 24))
#define IO96B_STATUS_COMMAND_RESPONSE_READY      BIT(0)
#define IO96B_ECC_ENABLE_RESPONSE_MODE_MASK      (0x3u)
#define IO96B_ECC_ENABLE_RESPONSE_TYPE_MASK      (0x4u)
#define IO96B_GET_MEM_INFO_NUM_USED_MEM_INF_MASK (0x3u)
#define CMD_RESP_TIMEOUT                         (200u) /* 200MSEC */

/* supported mailbox command types */
#define CMD_GET_SYS_INFO       (0x01)
#define CMD_TRIG_CONTROLLER_OP (0x04)

/* supported mailbox command opcodes */
#define GET_MEM_INTF_INFO (0x0001u)
#define ECC_ENABLE_SET    (0x0101u)
#define ECC_ENABLE_STATUS (0x0102u)
#define ECC_INJECT_ERROR  (0x0109u)

#define ECC_RMW_READ_LINK_DBE (1)
#define ECC_READ_LINK_DBE     (2)
#define ECC_READ_LINK_SBE     (3)
#define ECC_WRITE_LINK_DBE    (4)
#define ECC_WRITE_LINK_SBE    (5)
#define ECC_MULTI_DBE         (6)
#define ECC_SINGLE_DBE        (7)
#define ECC_MULTI_SBE         (8)
#define ECC_SINGLE_SBE        (9)

#define ECC_ERR_ADDR_MSB_MASK   (0x3F)
#define ECC_ERROR_TYPE_MASK     (0x3C0)
#define ECC_EMIF_ID_MASK        (0x1FE0000)
#define ECC_ERROR_TYPE_BIT_OFST (6)
#define ECC_EMIF_ID_BIT_OFST    (17)
#define BIT_POS_32              (32)

#define ECC_SBE_SYNDROME (0xF4)
#define ECC_DBE_SYNDROME (0x03)
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
 * @brief   data
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
extern int io96b_mb_request(const struct device *dev, struct io96b_mb_req_resp *req_resp);

#endif /* ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_IO96B_ECC_H_ */
