/*
 * Copyright (c) 2019 Yurii Hamann.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SD/MMC interface driver.
 */

/* Note: SD/MMC driver isn't ready for production use */

#include <kernel.h>
#include <device.h>
#include <sdmmc/sdmmc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(sdmmc);

#define SD_STATUS_ERROR_MASK 0xFFF90008
#define R6_ERROR_MASK 0x0000E008

const char *sdmmc_error_str[] = { "SD_ERROR_NONE",
				  "SD_ERROR_AKE_SEQ",
				  "SD_ERROR_CSD_OVERWRITE",
				  "SD_ERROR_GENERAL",
				  "SD_ERROR_CC",
				  "SD_ERROR_CARD_ECC",
				  "SD_ERROR_ILLEGAL_CMD",
				  "SD_ERROR_COM_CRC",
				  "SD_ERROR_LOCK_UNLOCK",
				  "SD_ERROR_CARD_IS_LOCKED",
				  "SD_ERROR_WP_VIOLATION",
				  "SD_ERROR_ERASE_PARAM",
				  "SD_ERROR_ERASE_SEQ",
				  "SD_ERROR_BLOCK_LEN",
				  "SD_ERROR_ADDRESS_ERR",
				  "SD_ERROR_OUT_OF_RANGE" };

int sdmmc_cmd_sent_wait(struct device *dev)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->cmd_sent_wait(dev);
}

int sdmmc_check_resp_flags(struct device *dev)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->check_resp_flags(dev);
}

int sdmmc_get_cmd_resp1(struct device *dev, struct sdmmc_r1_card_status *status)
{
	int ret;
	struct sdmmc_data *data;
	struct sdmmc_r1_card_status card_status;

	ret = sdmmc_get_device_data(dev, &data);
	if (ret) {
		return ret;
	}

	ret = sdmmc_check_resp_flags(dev);
	if (ret) {
		return ret;
	}

	ret = sdmmc_get_short_cmd_resp(dev, (u32_t *)&card_status);
	if (ret) {
		return ret;
	}

	if (status != NULL) {
		*status = card_status;
	}

	if ((card_status.value & SD_STATUS_ERROR_MASK) == 0) {
		data->err_code = SDMMC_ERROR_OK_E;
		return 0;
	} else if (card_status.fields.ake_seq_error) {
		data->err_code = SDMMC_ERROR_AKE_SEQ_E;
	} else if (card_status.fields.csd_overwrite) {
		data->err_code = SDMMC_ERROR_CSD_OVERWRITE_E;
	} else if (card_status.fields.cc_error) {
		data->err_code = SDMMC_ERROR_CC_E;
	} else if (card_status.fields.card_ecc_failed) {
		data->err_code = SDMMC_ERROR_CARD_ECC_E;
	} else if (card_status.fields.illegal_command) {
		data->err_code = SDMMC_ERROR_ILLEGAL_CMD_E;
	} else if (card_status.fields.com_crc_error) {
		data->err_code = SDMMC_ERROR_COM_CRC_E;
	} else if (card_status.fields.lock_unlock_failed) {
		data->err_code = SDMMC_ERROR_LOCK_UNLOCK_E;
	} else if (card_status.fields.card_is_locked) {
		data->err_code = SDMMC_ERROR_CARD_IS_LOCKED_E;
	} else if (card_status.fields.wp_violation) {
		data->err_code = SDMMC_ERROR_WP_VIOLATION_E;
	} else if (card_status.fields.erase_param) {
		data->err_code = SDMMC_ERROR_ERASE_PARAM_E;
	} else if (card_status.fields.erase_seq_error) {
		data->err_code = SDMMC_ERROR_ERASE_SEQ_E;
	} else if (card_status.fields.block_len_error) {
		data->err_code = SDMMC_ERROR_BLOCK_LEN_E;
	} else if (card_status.fields.address_error) {
		data->err_code = SDMMC_ERROR_ADDRESS_ERR_E;
	} else if (card_status.fields.out_of_range) {
		data->err_code = SDMMC_ERROR_OUT_OF_RANGE_E;
	} else {
		data->err_code = SDMMC_ERROR_GENERAL_E;
	}

	LOG_ERR("get_cmd_resp1 failed: %s", sdmmc_error_str[data->err_code]);
	return -EIO;
}

int sdmmc_get_cmd_resp2(struct device *dev, u32_t *resp_data)
{
	int ret;

	ret = sdmmc_check_resp_flags(dev);
	if (ret) {
		return ret;
	}
	if (resp_data) {
		return sdmmc_get_long_cmd_resp(dev, resp_data);
	}
	return -EFAULT;
}

int sdmmc_get_cmd_resp3(struct device *dev, struct sdmmc_ocr_register *ocr)
{
	int ret;

	ret = sdmmc_check_resp_flags(dev);
	if (ret) {
		return ret;
	}
	if (ocr) {
		return sdmmc_get_short_cmd_resp(dev, (u32_t *)ocr);
	}
	return -EFAULT;
}

int sdmmc_get_cmd_resp6(struct device *dev, struct sdmmc_r6_resp *resp)
{
	int ret;
	struct sdmmc_r6_resp cmd_resp;
	struct sdmmc_data *data;

	if (resp == NULL) {
		return -EFAULT;
	}

	ret = sdmmc_get_device_data(dev, &data);
	if (ret) {
		return ret;
	}

	ret = sdmmc_check_resp_flags(dev);
	if (ret) {
		return ret;
	}

	ret = sdmmc_get_short_cmd_resp(dev, (u32_t *)&cmd_resp);
	if (ret) {
		return ret;
	}
	if ((cmd_resp.value & R6_ERROR_MASK) == 0) {
		*resp = cmd_resp;
		return 0;
	} else if (cmd_resp.fields.ake_seq_error) {
		data->err_code = SDMMC_ERROR_AKE_SEQ_E;
	} else if (cmd_resp.fields.illegal_command) {
		data->err_code = SDMMC_ERROR_ILLEGAL_CMD_E;
	} else if (cmd_resp.fields.com_crc_error) {
		data->err_code = SDMMC_ERROR_COM_CRC_E;
	} else {
		data->err_code = SDMMC_ERROR_GENERAL_E;
	}

	return 0;
}

int sdmmc_get_cmd_resp7(struct device *dev, struct sdmmc_r7_resp *resp)
{
	int ret;

	ret = sdmmc_check_resp_flags(dev);
	if (ret) {
		return ret;
	}

	ret = sdmmc_get_short_cmd_resp(dev, (u32_t *)resp);
	if (ret) {
		return ret;
	}
	return 0;
}

int sdmmc_write_cmd(struct device *dev, struct sdmmc_cmd *cmd)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->write_cmd(dev, cmd);
}

int sdmmc_send_cmd(struct device *dev, struct sdmmc_cmd *cmd)
{
	int ret;

	ret = sdmmc_write_cmd(dev, cmd);
	if (ret) {
		return ret;
	}
	switch (cmd->resp_index) {
	case SDMMC_RESP_NO_RESPONSE_E:
		ret = sdmmc_cmd_sent_wait(dev);
		break;
	case SDMMC_RESP_R1_E:
		ret = sdmmc_get_cmd_resp1(
			dev, (struct sdmmc_r1_card_status *)cmd->resp_data);
		break;
	case SDMMC_RESP_R2_E:
		ret = sdmmc_get_cmd_resp2(dev, (u32_t *)cmd->resp_data);
		break;
	case SDMMC_RESP_R3_E:
		ret = sdmmc_get_cmd_resp3(
			dev, (struct sdmmc_ocr_register *)cmd->resp_data);
		break;
	case SDMMC_RESP_R6_E:
		ret = sdmmc_get_cmd_resp6(
			dev, (struct sdmmc_r6_resp *)cmd->resp_data);
		break;
	case SDMMC_RESP_R7_E:
		ret = sdmmc_get_cmd_resp7(
			dev, (struct sdmmc_r7_resp *)cmd->resp_data);
		break;
	default:
		break;
	}

	return ret;
}

int sdmmc_go_idle_state_cmd(struct device *dev)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_GO_IDLE_STATE_E;
	cmd.argument = 0;
	cmd.resp_index = SDMMC_RESP_NO_RESPONSE_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_if_cond_cmd(struct device *dev)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_SEND_IF_COND_E;
	cmd.argument = 0;
	cmd.resp_index = SDMMC_RESP_NO_RESPONSE_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_app_cmd(struct device *dev)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_APP_CMD_E;
	cmd.argument = 0;
	cmd.resp_index = SDMMC_RESP_R1_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_send_op_cond_acmd(struct device *dev, struct sdmmc_acmd41_arg arg,
			    struct sdmmc_acmd41_resp *resp)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_APP_CMD_SEND_OP_COND_E;
	cmd.argument = arg.value;
	cmd.resp_index = SDMMC_RESP_R3_E;
	cmd.resp_data = resp;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_all_send_cid_cmd(struct device *dev, u32_t *cid)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_ALL_SEND_CID_E;
	cmd.argument = 0;
	cmd.resp_index = SDMMC_RESP_R2_E;
	cmd.resp_data = cid;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_send_rel_addr_cmd(struct device *dev, u16_t *rca)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_SEND_RELATIVE_ADDR_E;
	cmd.argument = 0;
	cmd.resp_index = SDMMC_RESP_R6_E;
	cmd.resp_data = rca;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_send_csd_cmd(struct device *dev, u16_t rca,
		       struct sdmmc_csd_register *csd)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_SEND_CSD_E;
	cmd.argument = (u32_t)(rca << 16U);
	cmd.resp_index = SDMMC_RESP_R2_E;
	cmd.resp_data = csd;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_set_block_length_cmd(struct device *dev, u32_t block_len)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_SET_BLOCKLEN_E;
	cmd.argument = block_len;
	cmd.resp_index = SDMMC_RESP_R1_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_write_block(struct device *dev, u32_t block_addr)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_WRITE_BLOCK_E;
	cmd.argument = block_addr;
	cmd.resp_index = SDMMC_RESP_R1_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_write_multiple_block(struct device *dev, u32_t block_addr)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_WRITE_MULTIPLE_BLOCK_E;
	cmd.argument = block_addr;
	cmd.resp_index = SDMMC_RESP_R1_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_read_block(struct device *dev, u32_t block_addr)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_READ_SINGLE_BLOCK_E;
	cmd.argument = block_addr;
	cmd.resp_index = SDMMC_RESP_R1_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_select_deselect_card_cmd(struct device *dev, u16_t rca)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_SELECT_DESELECT_CARD_E;
	cmd.argument = (u32_t)(rca << 16U);
	cmd.resp_index = SDMMC_RESP_R1_E;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_send_status_cmd(struct device *dev, u16_t rca,
			  struct sdmmc_r1_card_status *card_status)
{
	struct sdmmc_cmd cmd;

	cmd.cmd_index = SDMMC_CMD_SEND_STATUS_E;
	cmd.argument = (u32_t)(rca << 16U);
	cmd.resp_index = SDMMC_RESP_R1_E;
	cmd.resp_data = (void *)card_status;

	return sdmmc_send_cmd(dev, &cmd);
}

int sdmmc_get_short_cmd_resp(struct device *dev, u32_t *resp)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->get_short_cmd_resp(dev, resp);
}

int sdmmc_get_long_cmd_resp(struct device *dev, u32_t *resp)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->get_long_cmd_resp(dev, resp);
}

int sdmmc_get_power_state(struct device *dev, enum sdmmc_power_state *state)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->get_power_state(dev, state);
}

int sdmmc_get_device_data(struct device *dev, struct sdmmc_data **data)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->get_device_data(dev, data);
}

int sdmmc_write_block_data(struct device *dev, u32_t block_addr, u32_t *data,
			   u32_t datalen)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->write_block_data(dev, block_addr, data, datalen);
}

int sdmmc_read_block_data(struct device *dev, u32_t block_addr, u32_t datalen,
			  u32_t *data)
{
	struct sdmcc_driver_api *api =
		(struct sdmcc_driver_api *)dev->driver_api;

	return api->read_block_data(dev, block_addr, datalen, data);
}
