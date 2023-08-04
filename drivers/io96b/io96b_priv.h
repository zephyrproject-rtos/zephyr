/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
