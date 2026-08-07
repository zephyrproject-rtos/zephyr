/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_FPGA_BRIDGE_INTEL_H_
#define ZEPHYR_SUBSYS_FPGA_BRIDGE_INTEL_H_

#include <zephyr/kernel.h>

/* Mask for FPGA-HPS bridges */
#define BRIDGE_MASK					0x0F
/* Mailbox command header index */
#define MBOX_CMD_HEADER_INDEX       0x00
/* Mailbox command memory size */
#define FPGA_MB_CMD_ADDR_MEM_SIZE   20
/* Mailbox command response memory size */
#define FPGA_MB_RESPONSE_MEM_SIZE   20
/* Config status response length */
#define FPGA_CONFIG_STATUS_RESPONSE_LEN	   0x07

#define MBOX_CMD_CODE_OFFSET 0x00
#define MBOX_CMD_ID_MASK     0x7FF

#define MBOX_CMD_MODE_OFFSET 0x0B
#define MBOX_CMD_MODE_MASK   0x800

#define MBOX_DATA_LEN_OFFSET 0x0C
#define MBOX_DATA_LEN_MASK   0xFFF000

#define RECONFIG_DIRECT_COUNT_OFFSET 0x00
#define RECONFIG_DIRECT_COUNT_MASK   0xFF

#define RECONFIG_INDIRECT_ARG_OFFSET 0x08
#define RECONFIG_INDIRECT_COUNT_MASK 0xFF00

#define RECONFIG_INDIRECT_RESPONSE_OFFSET 0x10
#define RECONFIG_RESPONSE_COUNT_MASK	  0xFF0000

#define RECONFIG_DATA_MB_CMD_SIZE	   0x10
#define RECONFIG_DATA_MB_CMD_INDIRECT_MODE 0x01

#define RECONFIG_DATA_MB_CMD_LENGTH 0x03

#define RECONFIG_DATA_MB_CMD_DIRECT_COUNT      0x00
#define RECONFIG_DATA_MB_CMD_INDIRECT_ARG      0x01
#define RECONFIG_DATA_MB_CMD_INDIRECT_RESPONSE 0x00
#define RECONFIG_STATUS_INTERVAL_DELAY_US      1000
#define RECONFIG_STATUS_RETRY_COUNT            20

#define MBOX_CONFIG_STATUS_STATE_CONFIG	  0x10000000
#define MBOX_CFGSTAT_VAB_BS_PREAUTH       0x20000000

#define FPGA_NOT_CONFIGURED_ERROR         0x02000004

#define MBOX_CFGSTAT_STATE_ERROR_HARDWARE 0xF0000005
#define RECONFIG_SOFTFUNC_STATUS_CONF_DONE	  BIT(0)
#define RECONFIG_SOFTFUNC_STATUS_INIT_DONE	  BIT(1)
#define RECONFIG_SOFTFUNC_STATUS_SEU_ERROR	  BIT(3)
#define RECONFIG_PIN_STATUS_NSTATUS		      BIT(31)

#define MBOX_REQUEST_HEADER(cmd_id, cmd_mode, len)                                                 \
	((cmd_id << MBOX_CMD_CODE_OFFSET) & (MBOX_CMD_ID_MASK)) |                                  \
		((cmd_mode << MBOX_CMD_MODE_OFFSET) & (MBOX_CMD_MODE_MASK)) |                      \
		((len << MBOX_DATA_LEN_OFFSET) & (MBOX_DATA_LEN_MASK))

#define MBOX_RECONFIG_REQUEST_DATA_FORMAT(direct_count, indirect_arg_count, response_arg_count)    \
	((direct_count << RECONFIG_DIRECT_COUNT_OFFSET) & (RECONFIG_DIRECT_COUNT_MASK)) |          \
		((indirect_arg_count << RECONFIG_INDIRECT_ARG_OFFSET) &                            \
		 (RECONFIG_INDIRECT_COUNT_MASK)) |                                                 \
		((response_arg_count << RECONFIG_INDIRECT_RESPONSE_OFFSET) &                       \
		 (RECONFIG_RESPONSE_COUNT_MASK))

union mailbox_response_header {
	/* Header of the config status response */
	uint32_t header;

	struct {
		/* error_code – Field provides a basic description of whether the command
		 * succeeded or not. A successful response returns an error code of 0x0,
		 * non-zero values indicate failure
		 */
		uint32_t error_code : 11;
		/* indirect_bit - Field indicates an indirect command */
		uint32_t indirect_bit : 1;
		/* data_length - Field counts the number of word arguments which follow the
		 * response header word. The meaning of these words depends on the command
		 * code. Units are words
		 */
		uint32_t data_length : 11;
		/* reserve bit */
		uint32_t reserved_bit : 1;
		/* id - Field is returned unchanged from the matching command header and is
		 * useful for matching responses to commands along with the CLIENT
		 */
		uint32_t id : 4;
		/* client_id - Field is returned unchanged from the matching command header and
		 * is useful for matching responses to commands along with the ID
		 */
		uint32_t client_id : 4;
	} mailbox_resp_header;
};

union config_status_version {
	/* Version of the config status response */
	uint32_t version;

	struct {
		/* update number bits */
		uint32_t update_number : 8;
		/* minor acds release number bits */
		uint32_t minor_acds_release_number : 8;
		/* major acds release number bits */
		uint32_t major_acds_release_number : 8;
		/* qspi flash index bits */
		uint32_t qspi_flash_index : 8;
	} response_version_member;
};

union config_status_pin_status {
	uint32_t pin_status;

	struct {
		/* msel bits */
		uint32_t msel : 4;
		/* pmf data bits */
		uint32_t pmf_data : 4;
		/* reserve bits */
		uint32_t reserved_bit : 22;
		/* nconfig bits */
		uint32_t nconfig : 1;
		/* nconfig_status bits */
		uint32_t nconfig_status : 1;
	} pin_status_member;
};

/* Struct to store the fpga_config_status */
struct fpga_config_status {
	/* Response header */
	union mailbox_response_header header;
	/* Config state idle or config mode */
	uint32_t state;
	/* Version number */
	union config_status_version version;
	/* Pin status */
	union config_status_pin_status pin_status;
	/* Soft function status details */
	uint32_t soft_function_status;
	/* Location in the bitstream where the error occurred */
	uint32_t error_location;
	/* Data is non-zero only for certain errors. The contents are highly dependent
	 * on which error was reported. The meaning of this data will not be made available to
	 * customers and can only be interpreted by investigating the source code directly
	 */
	uint32_t error_details;
};

enum smc_cmd_code {
	/* SMC COMMAND ID to disable all the bridges */
	FPGA_ALL_BRIDGE_DISABLE = 0x00,
	/* SMC COMMAND ID to enable all the bridges */
	FPGA_ALL_BRIDGE_ENABLE = 0x01,
	/* SMC Cancel Command */
	FPGA_CANCEL = 0x03,
	/* SMC COMMAND ID to check Reconfig status to SDM via mailbox */
	FPGA_CONFIG_STATUS = 0x04,
	/* SMC COMMAND ID to check Reconfig status to SDM via mailbox */
	FPGA_RECONFIG_STATUS = 0x09
};

enum mbox_reconfig_status_resp {
	/* Mailbox reconfig status header */
	MBOX_RECONFIG_STATUS_HEADER,
	/* Mailbox reconfig status state */
	MBOX_RECONFIG_STATUS_STATE,
	/* Mailbox reconfig status version */
	MBOX_RECONFIG_STATUS_VERSION,
	/* Mailbox reconfig status pin status */
	MBOX_RECONFIG_STATUS_PIN_STATUS,
	/* Mailbox reconfig status soft function */
	MBOX_RECONFIG_STATUS_SOFT_FUNCTION,
	/* Mailbox reconfig status error location */
	MBOX_RECONFIG_STATUS_ERROR_LOCATION,
	/* Mailbox reconfig status error details */
	MBOX_RECONFIG_STATUS_ERROR_DETAILS
};

enum smc_request {
	/* SMC request parameter a2 index*/
	SMC_REQUEST_A2_INDEX = 0x00,
	/* SMC request parameter a3 index */
	SMC_REQUEST_A3_INDEX = 0x01
};

/* SIP SVC response private data */
struct sip_svc_private_data {
	struct sip_svc_response response;
	uint32_t *mbox_response_data;
	uint32_t mbox_response_len;
	struct k_sem smc_sem;
	struct fpga_config_status config_status;
};

#endif
