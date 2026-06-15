/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdhc_emul_core.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <string.h>

LOG_MODULE_REGISTER(sdhc_emul_core, CONFIG_SDHC_LOG_LEVEL);

void sdhc_emul_core_build_cid(struct sdhc_emul_card *card, int ordinal)
{
	card->cid[0] = 0x01;
	card->cid[1] = 'Z';
	card->cid[2] = 'E';
	card->cid[3] = 'Z';
	card->cid[4] = 'E';
	card->cid[5] = 'M';
	card->cid[6] = 'U';
	card->cid[7] = 'L';
	card->cid[9] = (ordinal >> 24) & 0xFF;
	card->cid[10] = (ordinal >> 16) & 0xFF;
	card->cid[11] = (ordinal >> 8) & 0xFF;
	card->cid[12] = ordinal & 0xFF;
}

void sdhc_emul_core_build_csd(struct sdhc_emul_card *card)
{
	uint32_t c_size;

	memset(card->csd, 0, sizeof(card->csd));

	if (card->type == SDHC_EMUL_TYPE_SDHC || card->type == SDHC_EMUL_TYPE_SDXC) {
		card->csd[0] = 0x40;
		card->csd[5] = 9;
		c_size = (card->n_blocks / 1024) - 1;
		card->csd[7] = (c_size >> 16) & 0x3F;
		card->csd[8] = (c_size >> 8) & 0xFF;
		card->csd[9] = c_size & 0xFF;
	} else if (card->type == SDHC_EMUL_TYPE_EMMC) {
		card->csd[0] = 0xD0;
		card->csd[5] = 9;
	} else {
		/* SDIO cards do not use a standard CSD; already zeroed above */
	}
}

void sdhc_emul_core_build_ext_csd(struct sdhc_emul_card *card)
{
	memset(card->ext_csd, 0, sizeof(card->ext_csd));
	card->ext_csd[EXT_CSD_SEC_COUNT + 0] = card->n_blocks & 0xFF;
	card->ext_csd[EXT_CSD_SEC_COUNT + 1] = (card->n_blocks >> 8) & 0xFF;
	card->ext_csd[EXT_CSD_SEC_COUNT + 2] = (card->n_blocks >> 16) & 0xFF;
	card->ext_csd[EXT_CSD_SEC_COUNT + 3] = (card->n_blocks >> 24) & 0xFF;
	card->ext_csd[EXT_CSD_STRUCTURE] = MMC_5_1;
	card->ext_csd[EXT_CSD_CARD_TYPE] = (1 << 0) | (1 << 1) | (1 << 4) | (1 << 5);
}

void sdhc_emul_core_init_sdio_regs(struct sdhc_emul_card *card)
{
	memset(card->cccr, 0, sizeof(card->cccr));
	memset(card->fbr, 0, sizeof(card->fbr));
	card->cccr[0x00] = 0x32;
	for (uint8_t fn = 1; fn <= card->sdio_fn_count && fn < ARRAY_SIZE(card->fbr); fn++) {
		card->fbr[fn][0x00] = 0x01;
	}
}

uint8_t *sdhc_emul_core_block_ptr(struct sdhc_emul_card *card, uint32_t block_num)
{
	if (block_num >= card->n_blocks) {
		return NULL;
	}
	return card->storage + (block_num * card->block_size);
}

int sdhc_emul_core_validate_data(struct sdhc_emul_card *card, struct sdhc_data *data)
{
	size_t total_size;

	if (data == NULL || data->data == NULL || data->blocks == 0) {
		return -EINVAL;
	}
	if (data->block_size != 0 && data->block_size != card->block_size) {
		return -EINVAL;
	}
	if (data->block_addr >= card->n_blocks ||
	    data->blocks > card->n_blocks - data->block_addr) {
		return -EINVAL;
	}
	if (size_mul_overflow((size_t)data->blocks, (size_t)card->block_size, &total_size)) {
		return -EOVERFLOW;
	}
	return 0;
}

int sdhc_emul_core_xfer_size(struct sdhc_emul_card *card, struct sdhc_data *data,
			     size_t *total_size)
{
	int ret = sdhc_emul_core_validate_data(card, data);

	if (ret != 0) {
		return ret;
	}
	(void)size_mul_overflow((size_t)data->blocks, (size_t)card->block_size, total_size);
	return 0;
}

static void sdhc_emul_cmd0(struct sdhc_emul_card *card)
{
	card->state = SDHC_EMUL_STATE_IDLE;
	card->rca = 0;
	card->bus_width = 1;
	card->hs200_mode = false;
	card->hs400_mode = false;
	card->next_is_acmd = false;
	card->multi_block_count = 0;
	card->ext_csd[EXT_CSD_BUS_WIDTH] = 0;
	card->ext_csd[EXT_CSD_HS_TIMING] = 0;
}

static int sdhc_emul_cmd_app(struct sdhc_emul_card *card, struct sdhc_command *cmd,
		       struct sdhc_data *data)
{
	if (cmd->opcode == SD_APP_SET_BUS_WIDTH) {
		card->bus_width = (cmd->arg & 0x02) ? 4 : 1;
		return 0;
	}
	if (cmd->opcode == SD_APP_SEND_OP_COND) {
		cmd->response[0] = card->ocr | 0x80000000U;
		card->state = SDHC_EMUL_STATE_READY;
		return 0;
	}
	if (cmd->opcode == SD_APP_SEND_SCR) {
		if (!data || !data->data) {
			return -EINVAL;
		}
		uint8_t *scr = (uint8_t *)data->data;

		memset(scr, 0, 8);
		scr[0] = 0x02;
		scr[1] = 0x25;
		cmd->response[0] = (card->state << 9);
		data->bytes_xfered = 8;
		return 0;
	}
	return -ENOTSUP;
}

static int sdhc_emul_cmd_sdio_op_cond(struct sdhc_emul_card *card,
				    struct sdhc_command *cmd)
{
	if (card->type == SDHC_EMUL_TYPE_SDIO) {
		cmd->response[0] = (card->sdio_fn_count << 28) | 0x80000000U;
	} else if (card->type == SDHC_EMUL_TYPE_EMMC) {
		if (cmd->arg & 0x8000) {
			card->state = SDHC_EMUL_STATE_SLEEP;
		} else {
			card->state = SDHC_EMUL_STATE_STANDBY;
		}
	} else {
		/* SD cards do not use CMD5 */
	}
	return 0;
}

static int sdhc_emul_cmd_switch(struct sdhc_emul_card *card,
			    struct sdhc_command *cmd)
{
	if (card->type != SDHC_EMUL_TYPE_EMMC) {
		return 0;
	}

	uint8_t access = (cmd->arg >> 24) & 0x3;
	uint8_t index = (cmd->arg >> 16) & 0xFF;
	uint8_t value = (cmd->arg >> 8) & 0xFF;

	if (access != 3) {
		return 0;
	}
	if (index == EXT_CSD_HS_TIMING && value == 0x03 && !card->hs200_mode) {
		return -EINVAL;
	}
	card->ext_csd[index] = value;
	if (index == EXT_CSD_HS_TIMING) {
		if (value == 0x02) {
			card->hs200_mode = true;
		}
		if (value == 0x03) {
			card->hs400_mode = true;
		}
	}
	return 0;
}

static int sdhc_emul_cmd_select_card(struct sdhc_emul_card *card)
{
	if (card->state == SDHC_EMUL_STATE_STANDBY) {
		card->state = SDHC_EMUL_STATE_TRANSFER;
	} else if (card->state == SDHC_EMUL_STATE_TRANSFER) {
		card->state = SDHC_EMUL_STATE_STANDBY;
	}
	return 0;
}

static int sdhc_emul_cmd_if_cond(struct sdhc_emul_card *card,
				  struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	if (card->type == SDHC_EMUL_TYPE_EMMC) {
		if (data && data->data) {
			memcpy(data->data, card->ext_csd, 512);
			data->bytes_xfered = 512;
		}
		return 0;
	}
	cmd->response[0] = (cmd->arg & 0xFFF00) | (cmd->arg & 0xFF);
	return 0;
}

static int sdhc_emul_cmd_send_csd(struct sdhc_emul_card *card,
			      struct sdhc_command *cmd)
{
	if (card->type == SDHC_EMUL_TYPE_SDHC || card->type == SDHC_EMUL_TYPE_SDXC) {
		uint32_t c_size = (card->n_blocks >= 1024) ?
				  (card->n_blocks / 1024) - 1 : 0;
		memset(cmd->response, 0, sizeof(cmd->response));
		cmd->response[3] = 0x40000000;
		cmd->response[2] = (c_size >> 16) & 0x3F;
		cmd->response[1] = (c_size & 0xFFFF) << 16;
	} else {
		memcpy(cmd->response, card->csd, 16);
	}
	return 0;
}

static int sdhc_emul_cmd_read_block(struct sdhc_emul_card *card,
				struct sdhc_data *data)
{
	size_t total_size;
	uint8_t *ptr;

	if (card->state != SDHC_EMUL_STATE_TRANSFER) {
		return -EINVAL;
	}
	if (sdhc_emul_core_xfer_size(card, data, &total_size) != 0) {
		return -EINVAL;
	}
	ptr = sdhc_emul_core_block_ptr(card, data->block_addr);
	if (!ptr) {
		return -EINVAL;
	}
	memcpy(data->data, ptr, total_size);
	data->bytes_xfered = total_size;
	return 0;
}

static int sdhc_emul_cmd_write_block(struct sdhc_emul_card *card,
				 struct sdhc_data *data)
{
	size_t total_size;
	uint8_t *ptr;

	if (card->state != SDHC_EMUL_STATE_TRANSFER) {
		return -EINVAL;
	}
	if (sdhc_emul_core_xfer_size(card, data, &total_size) != 0) {
		return -EINVAL;
	}
	ptr = sdhc_emul_core_block_ptr(card, data->block_addr);
	if (!ptr) {
		return -EINVAL;
	}
	memcpy(ptr, data->data, total_size);
	data->bytes_xfered = total_size;
	return 0;
}

static int sdhc_emul_cmd_erase(struct sdhc_emul_card *card)
{
	if (card->erase_start > card->erase_end ||
	    card->erase_start >= card->n_blocks ||
	    card->erase_end >= card->n_blocks) {
		return -EINVAL;
	}
	for (uint32_t i = card->erase_start; i <= card->erase_end; i++) {
		uint8_t *blk = sdhc_emul_core_block_ptr(card, i);

		memset(blk, 0, card->block_size);
	}
	return 0;
}

static int sdhc_emul_cmd_sdio_direct(struct sdhc_emul_card *card,
				 struct sdhc_command *cmd)
{
	if (card->type != SDHC_EMUL_TYPE_SDIO) {
		return -ENOTSUP;
	}

	bool write = (cmd->arg >> 31) & 0x1;
	bool read_after_write = (cmd->arg >> 27) & 0x1;
	uint8_t func = (cmd->arg >> 28) & 0x7;
	uint32_t addr = (cmd->arg >> 9) & 0x1FFFF;
	uint8_t write_data = cmd->arg & 0xFF;
	uint8_t read_data;

	if (func > card->sdio_fn_count) {
		return -EINVAL;
	}
	if (func == 0) {
		if (addr >= sizeof(card->cccr)) {
			return -EINVAL;
		}
		read_data = card->cccr[addr];
		if (write) {
			card->cccr[addr] = write_data;
		}
	} else {
		if (addr < 0x100 || addr >= 0x100 + sizeof(card->fbr[func])) {
			return -EINVAL;
		}
		read_data = card->fbr[func][addr - 0x100];
		if (write) {
			card->fbr[func][addr - 0x100] = write_data;
		}
	}
	cmd->response[0] = (1 << 12) |
			   ((write && !read_after_write) ? write_data : read_data);
	return 0;
}

static int sdhc_emul_cmd_sdio_extended(struct sdhc_emul_card *card,
				   struct sdhc_command *cmd,
				   struct sdhc_data *data)
{
	if (card->type != SDHC_EMUL_TYPE_SDIO) {
		return -ENOTSUP;
	}
	if (data == NULL || data->data == NULL) {
		return -EINVAL;
	}

	bool write = (cmd->arg >> 31) & 0x1;
	uint8_t func = (cmd->arg >> 28) & 0x7;
	bool block_mode = (cmd->arg >> 27) & 0x1;
	uint32_t addr = (cmd->arg >> 9) & 0x1FFFF;
	uint32_t count = cmd->arg & 0x1FF;
	uint32_t len;

	if (block_mode) {
		len = count * 512;
	} else if (count == 0) {
		len = 512;
	} else {
		len = count;
	}

	if (func > card->sdio_fn_count) {
		return -EINVAL;
	}
	if (addr > 4096 || len > 4096 - addr) {
		return -EINVAL;
	}
	if (write) {
		memcpy(&card->sdio_fn_buf[func][addr], data->data, len);
	} else {
		memcpy(data->data, &card->sdio_fn_buf[func][addr], len);
	}
	data->bytes_xfered = len;
	cmd->response[0] = (1 << 12);
	return 0;
}

static int sdhc_emul_cmd_set_block_size(struct sdhc_emul_card *card,
					struct sdhc_command *cmd)
{
	if (cmd->arg != 512) {
		return -EINVAL;
	}
	card->block_size = cmd->arg;
	return 0;
}

static int sdhc_emul_cmd_tuning_block(struct sdhc_data *data)
{
	if (data && data->data) {
		memset(data->data, 0xA5, 128);
		data->bytes_xfered = 128;
	}
	return 0;
}

int sdhc_emul_core_request(struct sdhc_emul_card *card, struct sdhc_command *cmd,
			   struct sdhc_data *data)
{
	bool is_acmd;
	int ret = 0;

	if (!card->card_present) {
		return -ENODEV;
	}

	is_acmd = card->next_is_acmd;
	card->next_is_acmd = false;

	if (card->inject_cmd == cmd->opcode) {
		LOG_WRN("fault inject: error on CMD%d", cmd->opcode);
		return -EIO;
	}

	if (is_acmd) {
		return sdhc_emul_cmd_app(card, cmd, data);
	}

	switch (cmd->opcode) {
	case SD_GO_IDLE_STATE:
		sdhc_emul_cmd0(card);
		break;
	case MMC_SEND_OP_COND:
		cmd->response[0] = card->ocr | 0x80000000U;
		card->state = SDHC_EMUL_STATE_READY;
		break;
	case SD_ALL_SEND_CID:
		card->state = SDHC_EMUL_STATE_IDENT;
		memcpy(cmd->response, card->cid, 16);
		break;
	case SD_SEND_RELATIVE_ADDR:
		card->rca = 0x1234;
		cmd->response[0] = (card->rca << 16) | (card->state << 9);
		card->state = SDHC_EMUL_STATE_STANDBY;
		break;
	case SDIO_SEND_OP_COND:
		ret = sdhc_emul_cmd_sdio_op_cond(card, cmd);
		break;
	case SD_SWITCH:
		ret = sdhc_emul_cmd_switch(card, cmd);
		break;
	case SD_SELECT_CARD:
		ret = sdhc_emul_cmd_select_card(card);
		break;
	case SD_SEND_IF_COND:
		ret = sdhc_emul_cmd_if_cond(card, cmd, data);
		break;
	case SD_SEND_CSD:
		ret = sdhc_emul_cmd_send_csd(card, cmd);
		break;
	case SD_STOP_TRANSMISSION:
		card->state = SDHC_EMUL_STATE_TRANSFER;
		break;
	case SD_SEND_STATUS:
		cmd->response[0] = (card->state << 9) | (1 << 8);
		break;
	case SD_SET_BLOCK_SIZE:
		ret = sdhc_emul_cmd_set_block_size(card, cmd);
		break;
	case SD_READ_SINGLE_BLOCK:
	case SD_READ_MULTIPLE_BLOCK:
		ret = sdhc_emul_cmd_read_block(card, data);
		break;
	case MMC_SEND_TUNING_BLOCK:
		ret = sdhc_emul_cmd_tuning_block(data);
		break;
	case SD_SET_BLOCK_COUNT:
		card->multi_block_count = cmd->arg & 0xFFFF;
		break;
	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		ret = sdhc_emul_cmd_write_block(card, data);
		break;
	case SD_ERASE_BLOCK_START:
		card->erase_start = cmd->arg;
		break;
	case SD_ERASE_BLOCK_END:
		card->erase_end = cmd->arg;
		break;
	case SD_ERASE_BLOCK_OPERATION:
		ret = sdhc_emul_cmd_erase(card);
		break;
	case SDIO_RW_DIRECT:
		ret = sdhc_emul_cmd_sdio_direct(card, cmd);
		break;
	case SDIO_RW_EXTENDED:
		ret = sdhc_emul_cmd_sdio_extended(card, cmd, data);
		break;
	case SD_APP_CMD:
		card->next_is_acmd = true;
		cmd->response[0] = (card->state << 9) | 0x00000020;
		break;
	default:
		LOG_DBG("Unhandled CMD%d", cmd->opcode);
		ret = -ENOTSUP;
	}

	return ret;
}
