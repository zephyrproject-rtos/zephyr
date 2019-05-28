/*
 * Copyright (c) 2019 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SD/MMC interface
 */

#ifndef ZEPHYR_INCLUDE_SDMMC_H_
#define ZEPHYR_INCLUDE_SDMMC_H_

/**
 * @brief SD/MMC Interface
 * @defgroup sdmmc_interface SD/MMC Interface
 * @ingroup io_interfaces
 * @{
 */

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SD/MMC interface power states */
enum sdmmc_power_state {
	SDMMC_POWER_OFF,
	SDMMC_POWER_ON,
};

/** @brief SD/MMC interface error codes */
enum sdmmc_error {
	SDMMC_ERROR_OK_E = 0,
	SDMMC_ERROR_TIMEOUT_E,
	SDMMC_ERROR_CRC_E,
	SDMMC_ERROR_AKE_SEQ_E,
	SDMMC_ERROR_CSD_OVERWRITE_E,
	SDMMC_ERROR_GENERAL_E,
	SDMMC_ERROR_CC_E,
	SDMMC_ERROR_CARD_ECC_E,
	SDMMC_ERROR_ILLEGAL_CMD_E,
	SDMMC_ERROR_COM_CRC_E,
	SDMMC_ERROR_LOCK_UNLOCK_E,
	SDMMC_ERROR_CARD_IS_LOCKED_E,
	SDMMC_ERROR_WP_VIOLATION_E,
	SDMMC_ERROR_ERASE_PARAM_E,
	SDMMC_ERROR_ERASE_SEQ_E,
	SDMMC_ERROR_BLOCK_LEN_E,
	SDMMC_ERROR_ADDRESS_ERR_E,
	SDMMC_ERROR_OUT_OF_RANGE_E
};

/** @brief SD/MMC command codes */
enum sdmmc_cmd_index {
	/* Basic Commands (class 0) */
	SDMMC_CMD_GO_IDLE_STATE_E = 0,
	SDMMC_CMD_ALL_SEND_CID_E = 2,
	SDMMC_CMD_SEND_RELATIVE_ADDR_E = 3,
	SDMMC_CMD_SET_DSR_E = 4,
	SDMMC_CMD_SELECT_DESELECT_CARD_E = 7,
	SDMMC_CMD_SEND_IF_COND_E = 8,
	SDMMC_CMD_SEND_CSD_E = 9,
	SDMMC_CMD_SEND_CID_E = 10,
	SDMMC_CMD_SEND_VOLTAGE_SWITCH_E = 11,
	SDMMC_CMD_SEND_STOP_TRANSMISSION_E = 12,
	SDMMC_CMD_SEND_STATUS_E = 13,
	SDMMC_CMD_GO_INACTIVE_STATE_E = 15,

	/* Block-Oriented Read Commands (class 2) */
	SDMMC_CMD_SET_BLOCKLEN_E = 16,
	SDMMC_CMD_READ_SINGLE_BLOCK_E = 17,
	SDMMC_CMD_READ_MULTIPLE_BLOCK_E = 18,
	SDMMC_CMD_SEND_TUNING_BLOCK_E = 19,
	SDMMC_CMD_SPEED_CLASS_CONTROL_E = 20,
	SDMMC_CMD_SET_BLOCK_COUNT_E = 23,

	/* Block-Oriented Write Commands (class 4) */
	SDMMC_CMD_WRITE_BLOCK_E = 24,
	SDMMC_CMD_WRITE_MULTIPLE_BLOCK_E = 25,
	SDMMC_CMD_PROGRAM_CSD_E = 27,

	/* Block-Oriented Write Protection Commands (class 6) */
	SDMMC_CMD_SET_WRITE_PROT_E = 28,
	SDMMC_CMD_CLR_WRITE_PROT_E = 29,
	SDMMC_CMD_SEND_WRITE_PROT_E = 30,

	/* Application-Specific Commands (class 8) */
	SDMMC_CMD_APP_CMD_E = 55,

	/* Application */
	SDMMC_APP_CMD_SEND_OP_COND_E = 41,
};

/** @brief SD/MMC command responses */
enum sdmmc_resp_index {
	SDMMC_RESP_NO_RESPONSE_E = 0, /* No response */
	SDMMC_RESP_R1_E, /* Normal response */
	SDMMC_RESP_R1b_E, /* Normal response with busy signal */
	SDMMC_RESP_R2_E, /* CID, CSD registers */
	SDMMC_RESP_R3_E, /* OCR register */
	SDMMC_RESP_R6_E, /* Published RCA response */
	SDMMC_RESP_R7_E, /* Card Interface condition */
};

/** @brief SD/MMC command structure */
struct sdmmc_cmd {
	enum sdmmc_cmd_index cmd_index;
	u32_t argument;
	enum sdmmc_resp_index resp_index;
	void *resp_data;
};

/** @brief SD/MMC device data */
struct sdmmc_data {
	enum sdmmc_error err_code;
};

#pragma pack(push, 1)

/** @brief SD/MMC R1 card status */
struct sdmmc_r1_card_status {
	union {
		struct {
			u32_t reserved_1 : 3; /* [2:0] */
			u32_t ake_seq_error : 1; /* [3] */
			u32_t reserved_2 : 1; /* [4] */
			u32_t app_cmd : 1; /* [5] */
			u32_t fx_event : 1; /* [6] */
			u32_t reserved_3 : 1; /* [7] */
			u32_t ready_for_data : 1; /* [8] */
			u32_t current_state : 4; /* [12:9] */
			u32_t erase_reset : 1; /* [13] */
			u32_t card_ecc_enabled : 1; /* [14] */
			u32_t wp_erase_skip : 1; /* [15] */
			u32_t csd_overwrite : 1; /* [16] */
			u32_t reserved_4 : 2; /* [18:17] */
			u32_t error : 1; /* [19] */
			u32_t cc_error : 1; /* [20] */
			u32_t card_ecc_failed : 1; /* [21] */
			u32_t illegal_command : 1; /* [22] */
			u32_t com_crc_error : 1; /* [23] */
			u32_t lock_unlock_failed : 1; /* [24] */
			u32_t card_is_locked : 1; /* [25] */
			u32_t wp_violation : 1; /* [26] */
			u32_t erase_param : 1; /* [27] */
			u32_t erase_seq_error : 1; /* [28] */
			u32_t block_len_error : 1; /* [29] */
			u32_t address_error : 1; /* [30] */
			u32_t out_of_range : 1; /* [31] */
		} fields;
		u32_t value;
	};
};

/** @brief SD/MMC CSD register */
struct sdmmc_csd_register {
	union {
		struct {
			u32_t not_used : 1; /* [0] not used, always 1 */
			u32_t crc : 7; /* [7:1] CRC */
			u32_t reserved_6 : 2; /* [9:8] reserved */
			u32_t file_format : 2; /* [11:10] file format */
			u32_t tmp_write_protect : 1; /* [12] temporary write protection */
			u32_t perm_write_protect : 1; /* [13] permanent write protection */
			u32_t copy : 1; /* [14] copy flag */
			u32_t file_format_gpr : 1; /* [15] file format group */
			u32_t reserved_5 : 5; /* [20:16] reserved */
			u32_t write_bl_partial : 1; /* [21] partial blocks for write allowed */
			u32_t write_bl_len : 4; /* [25:22] max write data block length */
			u32_t r2w_factor : 3; /* [28:26] */
			u32_t reserved_4 : 2; /* [30:29] */
			u32_t wp_gpr_enable : 1; /* [31] */
			u32_t wp_gpr_size : 7; /* [38:32] */
			u32_t sector_size : 7; /* [45:39] */
			u32_t erase_blk_en : 1; /* [46] */
			u32_t reserved_3 : 1; /* [47] */
			u32_t c_size : 22; /* [69:48] */
			u32_t reserved_2 : 6; /* [75:70] */
			u32_t dsr_imp : 1; /* [76] */
			u32_t read_blk_misalign : 1; /* [77] */
			u32_t write_blk_misalign : 1; /* [78] */
			u32_t read_bl_partial : 1; /* [79] */
			u32_t read_bl_len : 4; /* [83:80] */
			u32_t ccc : 12; /* [95:84] */
			u8_t tran_speed; /* [103:96] */
			u8_t nsac; /* [111:104] */
			u8_t taac; /* [119:112] */
		} v2;
	};
	u8_t reserved_1 : 6; /* [125:120] */
	u8_t csd_structure : 2; /* [127:126] */
};

/** @brief SD/MMC OCR register */
struct sdmmc_ocr_register {
	u32_t reserved_1 : 15; /* [0:14] */
	u32_t v27_28 : 1; /* [15] */
	u32_t v28_29 : 1; /* [16] */
	u32_t v29_30 : 1; /* [17] */
	u32_t v30_31 : 1; /* [18] */
	u32_t v31_32 : 1; /* [19] */
	u32_t v32_33 : 1; /* [20] */
	u32_t v33_34 : 1; /* [20] */
	u32_t v34_35 : 1; /* [22] */
	u32_t v35_36 : 1; /* [23] */
	u32_t S18A : 1; /* [24] Switching to 1.8V accepted */
	u32_t reserved_2 : 4;
	u32_t UHSII : 1; /* [29] UHS-II Card Status */
	u32_t CCS : 1; /* [30] Card Capacity Status */
	u32_t busy : 1; /* [31] Busy status */
};

/** @brief SD/MMC R6 responses */
struct sdmmc_r6_resp {
	union {
		struct {
			u16_t reserved_1 : 3; /* [2:0] */
			u16_t ake_seq_error : 1; /* [3] */
			u16_t reserved_2 : 1; /* [4] */
			u16_t app_cmd : 1; /* [5] */
			u16_t fx_event : 1; /* [6] */
			u16_t reserved_3 : 1; /* [7] */
			u16_t ready_for_data : 1; /* [8] */
			u16_t current_state : 4; /* [12:9] */
			u16_t error : 1; /* [13] */
			u16_t illegal_command : 1; /* [14] */
			u16_t com_crc_error : 1; /* [15] */
			u16_t rca : 16; /* [31:16] */
		} fields;
		u32_t value;
	};
};

/** @brief SD/MMC R7 responses */
struct sdmmc_r7_resp {
	u32_t echo_back : 8;
	u32_t voltage_accepted : 4;
	u32_t reserved : 20;
};

/** @brief SD/MMC ACMD41 argument */
struct sdmmc_acmd41_arg {
	union {
		struct {
			u32_t reserved_1 : 8;
			u32_t OCR : 16;
			u32_t S18R : 1;
			u32_t reserved_2 : 3;
			u32_t XPC : 1;
			u32_t FB : 1;
			u32_t HCS : 1;
			u32_t busy : 1;
		} fields;
		u32_t value;
	};
};

/** @brief SD/MMC ACMD41 response */
struct sdmmc_acmd41_resp {
	union {
		struct {
			u32_t reserved_1 : 8;
			u32_t OCR : 16;
			u32_t S18A : 1; /* Switching to 1.8V accepted */
			u32_t reserved_2 : 4;
			u32_t UHSII : 1; /* UHS-II Card Status */
			u32_t CCS : 1; /* Card Capacity Status */
			u32_t busy : 1; /* Busy status */
		} fields;
		u32_t value;
	};
};

#pragma pack(pop)

/**
 * @brief SD/MMC driver API
 * This is the mandatory API any SD/MMC driver needs to expose.
 */
struct sdmcc_driver_api {
	int (*init)(struct device *dev);
	int (*write_cmd)(struct device *dev, struct sdmmc_cmd *cmd);
	int (*get_short_cmd_resp)(struct device *dev, u32_t *resp);
	int (*get_long_cmd_resp)(struct device *dev, u32_t *resp);
	int (*get_power_state)(struct device *dev, enum sdmmc_power_state *state);
	int (*check_resp_flags)(struct device *dev);
	int (*cmd_sent_wait)(struct device *dev);
	int (*write_block_data)(struct device *dev, u32_t block_addr,
				u32_t *data, u32_t datalen);
	int (*read_block_data)(struct device *dev, u32_t block_addr,
			       u32_t datalen, u32_t *data);
	int (*get_device_data)(struct device *dev, struct sdmmc_data **data);
};

/* SD/MMC commands */

/**
 * @brief GO_IDLE_STATE command (CMD0).
 *
 * @param dev pointer to device structure.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_go_idle_state_cmd(struct device *dev);

/**
 * @brief ALL_SEND_CID command (CMD2).
 *
 * @param dev pointer to device structure.
 * @param cid pointer to variable where CID register value
 *        to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_all_send_cid_cmd(struct device *dev, u32_t *cid);

/**
 * @brief SEND_RELATIVE_ADDR command (CMD3).
 *
 * @param dev pointer to device structure.
 * @param rca pointer to variable where relative address
 *        to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_send_rel_addr_cmd(struct device *dev, u16_t *rca);

/**
 * @brief SELECT/DESELECT_CARD command (CMD7).
 *
 * @param dev pointer to device structure.
 * @param rca relative card address
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_select_deselect_card_cmd(struct device *dev, u16_t rca);

/**
 * @brief SEND_IF_COND command (CMD8).
 *
 * @param dev pointer to device structure.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_if_cond_cmd(struct device *dev);

/**
 * @brief SEND_CSD command (CMD9).
 *
 * @param dev pointer to device structure.
 * @param rca relative card status
 * @param csd pointer to sdmmc_csd_register structure
 *        where csd register to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_send_csd_cmd(struct device *dev, u16_t rca,
		       struct sdmmc_csd_register *csd);

/**
 * @brief SEND_STATUS/SEND_TASK_STATUS command (CMD13).
 *
 * @param dev pointer to device structure.
 * @param rca relative card status
 * @param card_status pointer to sdmmc_r1_card_status structure
 *        where csd register to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_send_status_cmd(struct device *dev, u16_t rca,
			  struct sdmmc_r1_card_status *card_status);

/**
 * @brief SET_BLOCKLEN command (CMD16).
 *
 * @param dev pointer to device structure.
 * @param block_len block length
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_set_block_length_cmd(struct device *dev, u32_t block_len);

/**
 * @brief READ_SINGLE_BLOCK command (CMD17).
 *
 * @param dev pointer to device structure.
 * @param block_addr block address
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_read_block(struct device *dev, u32_t block_addr);

/**
 * @brief WRITE_BLOCK command (CMD24).
 *
 * @param dev pointer to device structure.
 * @param block_addr block address
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_write_block(struct device *dev, u32_t block_addr);

/**
 * @brief WRITE_MULTIPLE_BLOCK command (CMD25).
 *
 * @param dev pointer to device structure.
 * @param block_addr block address
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_write_multiple_block(struct device *dev, u32_t block_addr);

/**
 * @brief APP_CMD command (CMD55).
 *
 * @param dev pointer to device structure.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_app_cmd(struct device *dev);

/* SD/MMC application commands */

/**
 * @brief SD_SEND_OP_COND command (ACMD41).
 *
 * @param dev pointer to device structure.
 * @param arg AMCD41 argument structure
 * @param resp pointer to sdmmc_acmd41_resp structure
 *        where response to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_send_op_cond_acmd(struct device *dev, struct sdmmc_acmd41_arg arg,
			    struct sdmmc_acmd41_resp *resp);

/* Command responses */

/**
 * @brief R1 repsonse handler.
 *
 * @param dev pointer to device structure.
 * @param status pointer to sdmmc_r1_card_status structure
 *        where card status to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_cmd_resp1(struct device *dev,
			struct sdmmc_r1_card_status *status);

/**
 * @brief R2 repsonse handler.
 *
 * @param dev pointer to device structure.
 * @param resp_data pointer to response data array (4 double words)
 *        where data to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_cmd_resp2(struct device *dev, u32_t *resp_data);

/**
 * @brief R3 repsonse handler.
 *
 * @param dev pointer to device structure.
 * @param ocr pointer to sdmmc_ocr_register structure
 *        where OCR register data to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_cmd_resp3(struct device *dev, struct sdmmc_ocr_register *ocr);

/**
 * @brief R6 repsonse handler.
 *
 * @param dev pointer to device structure.
 * @param resp pointer to sdmmc_r6_resp structure
 *        where response data to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_cmd_resp6(struct device *dev, struct sdmmc_r6_resp *resp);

/**
 * @brief R7 repsonse handler.
 *
 * @param dev pointer to device structure.
 * @param resp pointer to sdmmc_r7_resp structure
 *        where response data to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_cmd_resp7(struct device *dev, struct sdmmc_r7_resp *resp);

/**
 * @brief Get power SD/MMC power state.
 *
 * @param dev pointer to device structure.
 * @param state pointer to power state value
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_power_state(struct device *dev, enum sdmmc_power_state *state);

/**
 * @brief Get device data structure.
 *
 * @param dev pointer to device structure.
 * @param data pointer to sdmmc_data pointer
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_device_data(struct device *dev, struct sdmmc_data **data);

/**
 * @brief Get response in short format.
 *
 * @param dev pointer to device structure.
 * @param resp pointer to short response data (1 dobule word)
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_short_cmd_resp(struct device *dev, u32_t *resp);

/**
 * @brief Get response in long format.
 *
 * @param dev pointer to device structure.
 * @param resp pointer to long response data (4 dobule words)
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_get_long_cmd_resp(struct device *dev, u32_t *resp);

/* Read/Write operations */

/**
 * @brief Write block data to SD/MMC device.
 *
 * @param dev pointer to device structure.
 * @param block_addr start address of the block
 * @param data pointer to data
 * @param datalen number of double words to write
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_write_block_data(struct device *dev, u32_t block_addr, u32_t *data,
			   u32_t datalen);

/**
 * @brief Write block data to SD/MMC device.
 *
 * @param dev pointer to device structure.
 * @param block_addr start address of the block
 * @param datalen number of double words to read
 * @param data pointer to buffer where data to be written
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
int sdmmc_read_block_data(struct device *dev, u32_t block_addr, u32_t datalen,
			  u32_t *data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SDMMC_H_ */
