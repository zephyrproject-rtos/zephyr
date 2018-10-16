/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal_log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <kernel.h>
#include <string.h>
#include <stdio.h>
#include <uwp_hal.h>

#include <hal_sfc.h>
#include <hal_sfc_cfg.h>
#include <hal_sfc_phy.h>
#include <hal_sfc_hal.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

void uwp_spi_dump(u32_t arg_in)
{
	LOG_ERR("dump SFC register:[#### %d ####]\n", arg_in);

	LOG_ERR("SFC_CMD_SET		reg, addr:[%08x], value:[0x%08x]\n",
		0x40890000, sci_read32(0x40890000));
	LOG_ERR("SFC_SOFT_REQ	reg, addr:[%08x], value:[0x%08x]\n",
		0x40890004, sci_read32(0x40890004));
	LOG_ERR("SFC_CLK_CFG		reg, addr:[%08x], value:[0x%08x]\n",
		0x4089001C, sci_read32(0x4089001C));
	LOG_ERR("CS0_CMD_BUF0	reg, addr:[%08x], value:[0x%08x]\n",
		0x40890040, sci_read32(0x40890040));
	LOG_ERR("CS0_TYPE_BUF0	reg, addr:[%08x], value:[0x%08x]\n",
		0x40890070, sci_read32(0x40890070));
	LOG_ERR("CS0_TYPE_BUF1	reg, addr:[%08x], value:[0x%08x]\n",
		0x40890074, sci_read32(0x40890074));
	LOG_ERR("CS0_TYPE_BUF2	reg, addr:[%08x], value:[0x%08x]\n",
		0x40890078, sci_read32(0x40890078));

	LOG_ERR("SFC_1X_CFG		reg, addr:[%08x], value:[0x%08x]\n",
		REG_AON_CLK_RF_CGM_SFC_1X_CFG,
		sci_read32(REG_AON_CLK_RF_CGM_SFC_1X_CFG));
	LOG_ERR("SFC_2X_CFG		reg, addr:[%08x], value:[0x%08x]\n",
		REG_AON_CLK_RF_CGM_SFC_2X_CFG,
		sci_read32(REG_AON_CLK_RF_CGM_SFC_2X_CFG));
}

__ramfunc void create_cmd(SFC_CMD_DES_T *cmd_desc_ptr, u32_t cmd,
		u32_t byte_len,
		CMD_MODE_E cmd_mode, BIT_MODE_E bit_mode, SEND_MODE_E send_mode)
{
	cmd_desc_ptr->cmd = cmd;
	cmd_desc_ptr->cmd_byte_len = byte_len;
	cmd_desc_ptr->cmd_mode = cmd_mode;
	cmd_desc_ptr->bit_mode = bit_mode;
	cmd_desc_ptr->send_mode = send_mode;
}

__ramfunc void spiflash_read_write(SFC_CMD_DES_T *cmd_des_ptr, u32_t cmd_len,
		u32_t * din)
{
	u32_t i;
	u32_t *read_ptr = din;
	u32_t read_count = 0;

	SCI_ASSERT(cmd_len <= 12);

	sfcdrv_resetallbuf();
	sfcdrv_setcmdcfgreg(CMD_MODE_WRITE, 0, INI_CMD_BUF_7);

	for (i = 0; i < cmd_len; i++) {
		cmd_des_ptr[i].is_valid = TRUE;
		if (cmd_des_ptr[i].cmd_mode == CMD_MODE_WRITE)
			sfcdrv_setcmddata(&(cmd_des_ptr[i]), 0);
		else if (cmd_des_ptr[i].cmd_mode == CMD_MODE_READ) {
			sfcdrv_setreadbuf(&(cmd_des_ptr[i]), 0);
			read_count++;
		} else
			SCI_ASSERT(0);
	}

	sfcdrv_req();

	if (0 != read_count)
		sfcdrv_getreadbuf(read_ptr, read_count);
}

void spiflash_read_write_sec(SFC_CMD_DES_T * cmd_des_ptr, u32_t cmd_len)
{
	u32_t i;

	SCI_ASSERT(cmd_len <= 12);

	sfcdrv_resetallbuf();
	sfcdrv_setcmdcfgreg(CMD_MODE_WRITE, 0, INI_CMD_BUF_7);

	sfcdrv_setcmdencryptcfgreg(1);

	for (i = 0; i < cmd_len; i++) {
		cmd_des_ptr[i].is_valid = TRUE;
		cmd_des_ptr[i].send_mode = SEND_MODE_0;

		if (1 == i)
			cmd_des_ptr[i].send_mode = SEND_MODE_1;

		sfcdrv_setcmddata(&(cmd_des_ptr[i]), 0);
	}
	sfcdrv_req();

	sfcdrv_setcmdencryptcfgreg(0);
}

__ramfunc static void spiflash_get_status(struct spi_flash *flash,
		u8_t *status1, u8_t *status2)
{
	u8_t cmd = 0;
	char logbuf[256] = { 0 };

	cmd = CMD_READ_STATUS1;
	spiflash_cmd_read(flash, &cmd, 1, 0xFFFFFFFF, status1, 1);

	cmd = CMD_READ_STATUS2;
	spiflash_cmd_read(flash, &cmd, 1, 0xFFFFFFFF, status2, 1);

	spiflash_select_xip(TRUE);

	snprintf(logbuf, sizeof(logbuf), " SF: spiflash status: "
		"status1 = 0x%x, status2 = 0x%x in [%s]\n",
		*status1, *status2, __func__);
	LOG_DBG("[%s]", logbuf);
}

__ramfunc BYTE_NUM_E spi_flash_addr(u32_t *addr, u32_t support_4addr)
{
	u8_t cmd[4] = { 0 };
	u32_t address = *addr;

	cmd[0] = ((address >> 0) & (0xFF));
	cmd[1] = ((address >> 8) & (0xFF));
	cmd[2] = ((address >> 16) & (0xFF));
	cmd[3] = ((address >> 24) & (0xFF));

	if (support_4addr == TRUE) {
#ifdef BIG_ENDIAN
		*addr = (cmd[3] << 0) | (cmd[2] << 8) |
			(cmd[1] << 16) | (cmd[0] << 24);
#else
		*addr = (cmd[3] << 0) | (cmd[2] << 8) |
			(cmd[1] << 16) | (cmd[0] << 24);
#endif
		return (BYTE_NUM_4);
	} else {
#ifdef BIG_ENDIAN
		*addr = (cmd[3] << 0) | (cmd[2] << 8) |
			(cmd[1] << 16) | (cmd[0] << 32);
#else
		*addr = (cmd[2] << 0) | (cmd[1] << 8) | (cmd[0] << 16);
#endif
		return (BYTE_NUM_3);
	}
}

__ramfunc struct spi_flash_params *spiflash_scan(void)
{
	u8_t i;
	u8_t idcode[5];
	u16_t jedec, ext_jedec, manufacturer_id;
	struct spi_flash_spec_s *spi_spec;
	SFC_CMD_DES_T cmd_desc[2];
	struct spi_flash_params *params = NULL;

	/* only for spi_mode */
	CREATE_CMD_(cmd_desc[0], CMD_READ_ID,
		BYTE_NUM_1, CMD_MODE_WRITE, BIT_MODE_1);
	CREATE_CMD_(cmd_desc[1], 0, BYTE_NUM_3,
		CMD_MODE_READ, BIT_MODE_1);

	spiflash_read_write(cmd_desc, 2, (u32_t *) idcode);

	spiflash_select_xip(TRUE);

	LOG_INF("SF: new Got idcode 0: 0x%x 1:0x%x 2:0x%x 3:0x%x 4:0x%x\n",
			idcode[0], idcode[1], idcode[2], idcode[3], idcode[4]);

	manufacturer_id = (u16_t) idcode[0];
	jedec = idcode[1] << 8 | idcode[2];
	ext_jedec = idcode[3] << 8 | idcode[4];

	for (i = 0; i < ARRAY_SIZE(spi_flash_spec_table); i++) {
		spi_spec = &spi_flash_spec_table[i];
		if (spi_spec->id_manufacturer == manufacturer_id) {
			LOG_INF("SF: get manufacture ID %04x\n", manufacturer_id);
			break;
		}
	}

	if (i == ARRAY_SIZE(spi_flash_spec_table)) {
		LOG_ERR("SF: Unsupported manufacture %04x\n", manufacturer_id);
		return NULL;
	}

	for (i = 0; i < spi_spec->table_num; i++) {
		params = &(spi_spec->table)[i];
		if (params->idcode1 == jedec) {
			if (params->idcode2 == ext_jedec)
				LOG_INF("SF: get ID %04x %04x\n", jedec,
						ext_jedec);
			break;
		}
	}

	if (i == spi_spec->table_num) {
		LOG_ERR("SF: Unsupported ID %04x %04x\n", jedec, ext_jedec);
		return NULL;
	} else {
		return params;
	}
}

LOCK_PATTERN_E spiflash_get_lock_pattern(u32_t start_addr, u32_t size,
		const struct spi_flash_lock_desc *
		lock_table, u32_t lock_table_size)
{
	LOCK_PATTERN_E lock_pattern = LOCK_PATTERN_MAX;
	u32_t i = 0, j = 0;
	u32_t lock_match_index = 0xFFFF;
	u32_t delta_size = 0xFFFFFFFF;

	for (i = 0; i < lock_table_size; i++) {
		lock_pattern = (LOCK_PATTERN_E) (lock_table[i].lock_pattern);

		for (j = 0; j < ARRAY_SIZE(sf_lock_pattern); j++) {
			if ((lock_pattern == sf_lock_pattern[j].lock_pattern) &&
					(start_addr == sf_lock_pattern[j].start_addr) &&
					(size >= sf_lock_pattern[j].size)) {
				if (size - sf_lock_pattern[j].size < delta_size) {
					delta_size =
						size - sf_lock_pattern[j].size;
					lock_match_index = j;
				}
			}
		}
	}

	if (0xFFFF == lock_match_index) {
		return LOCK_PATTERN_MAX;
	}

	return (LOCK_PATTERN_E) (sf_lock_pattern
			[lock_match_index].lock_pattern);
}

__ramfunc void spiflash_set_xip(SFC_CMD_DES_T *cmd_des_ptr, u32_t cmd_len,
		BIT_MODE_E bit_mode)
{
	u32_t i;

	sfcdrv_resetallbuf();
	sfcdrv_setcmdcfgreg(CMD_MODE_READ, bit_mode, INI_CMD_BUF_7);

	for (i = 0; i < cmd_len; i++) {
		cmd_des_ptr[i].is_valid = TRUE;

		if (i == 1)
			cmd_des_ptr[i].send_mode = SEND_MODE_1;
		else
			cmd_des_ptr[i].send_mode = SEND_MODE_0;

		sfcdrv_setcmddata(&(cmd_des_ptr[i]), 0);
	}

}

__ramfunc void spiflash_set_xip_cmd(struct spi_flash *flash,
		const u8_t *cmd_read, u8_t dummy_bytes)
{
	SFC_CMD_DES_T cmd_desc[5];
	u32_t cmd_desc_size = 0;
	u32_t dummy[2] = { 0 };

	BIT_MODE_E bitmode_cmd = BIT_MODE_1;
	BIT_MODE_E bitmode_addr = BIT_MODE_1;
	BIT_MODE_E bitmode_dummy = BIT_MODE_1;
	BIT_MODE_E bitmode_data = BIT_MODE_1;

	SCI_ASSERT(dummy_bytes <= 8);

	if (flash->work_mode == QPI_MODE) {
		bitmode_cmd = BIT_MODE_4;
		bitmode_addr = BIT_MODE_4;
		bitmode_dummy = BIT_MODE_4;
		bitmode_data = BIT_MODE_4;
		CREATE_CMD_(cmd_desc[0], (*(u32_t *) (cmd_read)) & 0xFF,
				BYTE_NUM_1, CMD_MODE_WRITE,
			    bitmode_cmd);
	} else {
		bitmode_cmd = BIT_MODE_1;
		bitmode_addr = BIT_MODE_1;
		bitmode_dummy = BIT_MODE_1;
		bitmode_data = BIT_MODE_1;

		if (RD_CMD_4BIT == (flash->spi_rw_mode & RD_CMD_MSK)) {
			bitmode_cmd = BIT_MODE_4;
		} else if (RD_CMD_2BIT == (flash->spi_rw_mode & RD_CMD_MSK)) {
			bitmode_cmd = BIT_MODE_2;
		} else {
			bitmode_cmd = BIT_MODE_1;
		}

		if (RD_ADDR_4BIT == (flash->spi_rw_mode & RD_ADDR_MSK)) {
			bitmode_addr = BIT_MODE_4;
		} else if (RD_ADDR_2BIT == (flash->spi_rw_mode & RD_ADDR_MSK)) {
			bitmode_addr = BIT_MODE_2;
		} else {
			bitmode_addr = BIT_MODE_1;
		}

		if (RD_DUMY_4BIT == (flash->spi_rw_mode & RD_DUMY_MSK)) {
			bitmode_dummy = BIT_MODE_4;
		} else if (RD_DUMY_2BIT == (flash->spi_rw_mode & RD_DUMY_MSK)) {
			bitmode_dummy = BIT_MODE_2;
		} else {
			bitmode_dummy = BIT_MODE_1;
		}

		if (RD_DATA_4BIT == (flash->spi_rw_mode & RD_DATA_MSK)) {
			bitmode_data = BIT_MODE_4;
		} else if (RD_DATA_2BIT == (flash->spi_rw_mode & RD_DATA_MSK)) {
			bitmode_data = BIT_MODE_2;
		} else {
			bitmode_data = BIT_MODE_1;
		}
		CREATE_CMD_(cmd_desc[0], (*(u32_t *) (cmd_read)) & 0xFF,
				BYTE_NUM_1, CMD_MODE_WRITE,
				bitmode_cmd);
	}

	BYTE_NUM_E addr_byte_num = BYTE_NUM_3;
	u32_t dest_addr;

	dest_addr = cmd_read[1] << 16 | cmd_read[2] << 8 | cmd_read[3];
	addr_byte_num =
	    spi_flash_addr(&dest_addr, flash->support_4addr);

	CREATE_CMD_(cmd_desc[1], dest_addr, addr_byte_num,
		CMD_MODE_WRITE, bitmode_addr);

	if (dummy_bytes > 4) {
		dummy[0] = BYTE_NUM_4;
		switch (dummy_bytes % 4) {
			case 1:
				dummy[1] = BYTE_NUM_1;
				break;
			case 2:
				dummy[1] = BYTE_NUM_2;
				break;
			case 3:
				dummy[1] = BYTE_NUM_3;
				break;
			case 4:
				dummy[1] = BYTE_NUM_4;
				break;
			default:
				break;
		}
		CREATE_CMD_(cmd_desc[2], 0, dummy[0], CMD_MODE_WRITE,
				bitmode_dummy);
		CREATE_CMD_(cmd_desc[3], 0, dummy[1], CMD_MODE_WRITE,
				bitmode_dummy);
		cmd_desc_size = 4;
	} else if (dummy_bytes > 0) {
		switch (dummy_bytes) {
			case 1:
				dummy[0] = BYTE_NUM_1;
				break;
			case 2:
				dummy[0] = BYTE_NUM_2;
				break;
			case 3:
				dummy[0] = BYTE_NUM_3;
				break;
			case 4:
				dummy[0] = BYTE_NUM_4;
				break;
			default:
				break;
		}
		CREATE_CMD_(cmd_desc[2], 0, dummy[0], CMD_MODE_WRITE,
				bitmode_dummy);
		cmd_desc_size = 3;
	} else {
		cmd_desc_size = 2;
	}

	spiflash_set_xip(cmd_desc, cmd_desc_size, bitmode_data);
}

__ramfunc void spiflash_cmd_read(struct spi_flash *flash, const u8_t *cmd,
		u32_t cmd_len, u32_t address, const void *data_in,
		u32_t data_len)
{
	SFC_CMD_DES_T cmd_desc[4];
	BIT_MODE_E bitmode = BIT_MODE_1;
	BYTE_NUM_E data_byte_num = 0;
	u32_t *data_ptr32 = (u32_t *) (data_in);
	u32_t cmd_idx = 0;

	SCI_ASSERT(cmd_len <= 4);
	SCI_ASSERT(data_len <= 8);

	if (QPI_MODE == flash->work_mode) {
		bitmode = BIT_MODE_4;
	}

	CREATE_CMD_(cmd_desc[cmd_idx++], cmd[0], BYTE_NUM_1, CMD_MODE_WRITE,
			bitmode);

	if (address != 0xFFFFFFFF) {
		CREATE_CMD_REV(cmd_desc[cmd_idx++], address, BYTE_NUM_3,
				CMD_MODE_WRITE, bitmode);
	}

	if (data_len > 4) {

		if (data_len == 1)
			data_byte_num = BYTE_NUM_1;
		if (data_len == 2)
			data_byte_num = BYTE_NUM_2;
		if (data_len == 3)
			data_byte_num = BYTE_NUM_3;
		if (data_len == 0)
			data_byte_num = BYTE_NUM_4;

		CREATE_CMD_(cmd_desc[cmd_idx++], 0, BYTE_NUM_4, CMD_MODE_READ,
				bitmode);
		CREATE_CMD_(cmd_desc[cmd_idx++], 0, data_byte_num,
				CMD_MODE_READ, bitmode);
	} else if (data_len > 0) {

		if (data_len == 1)
			data_byte_num = BYTE_NUM_1;
		if (data_len == 2)
			data_byte_num = BYTE_NUM_2;
		if (data_len == 3)
			data_byte_num = BYTE_NUM_3;
		if (data_len == 0)
			data_byte_num = BYTE_NUM_4;

		CREATE_CMD_(cmd_desc[cmd_idx++], 0, data_byte_num,
				CMD_MODE_READ, bitmode);
	} else {

	}

	spiflash_read_write(cmd_desc, cmd_idx, data_ptr32);
}

__ramfunc void spiflash_cmd_write(struct spi_flash *flash, const u8_t *cmd,
		u32_t cmd_len, const void *data_out, u32_t data_len)
{
	SFC_CMD_DES_T cmd_desc[4];
	BIT_MODE_E bitmode = BIT_MODE_1;
	BYTE_NUM_E cmd_byte_num = 0;
	BYTE_NUM_E data_byte_num = 0;
	u32_t cmd_data = 0;
	u32_t i = 0;
	u8_t *data_ptr8 = (u8_t *) (data_out);
	u32_t cmd_length = 2;

	SCI_ASSERT(cmd_len <= 4);
	SCI_ASSERT(data_len <= 4);

	for (i = 0; i < data_len; i++) {
		cmd_data = cmd_data | ((*data_ptr8) << (i * 8));
		data_ptr8++;
	}

	if (QPI_MODE == flash->work_mode) {
		bitmode = BIT_MODE_4;
	}

	switch (cmd_len) {
		case 1:
			cmd_byte_num = BYTE_NUM_1;
			break;
		case 2:
			cmd_byte_num = BYTE_NUM_2;
			break;
		case 3:
			cmd_byte_num = BYTE_NUM_3;
			break;
		case 4:
			cmd_byte_num = BYTE_NUM_4;
			break;
		default:
			break;
	}

	switch (data_len) {
		case 1:
			data_byte_num = BYTE_NUM_1;
			break;
		case 2:
			data_byte_num = BYTE_NUM_2;
			break;
		case 3:
			data_byte_num = BYTE_NUM_3;
			break;
		case 4:
			data_byte_num = BYTE_NUM_4;
			break;
		default:
			break;
	}

	if (data_len) {
		CREATE_CMD_(cmd_desc[0], cmd[0], cmd_byte_num, CMD_MODE_WRITE,
				bitmode);
		CREATE_CMD_(cmd_desc[1], cmd_data, data_byte_num,
				CMD_MODE_WRITE, bitmode);
		cmd_length = 2;
	} else {
		CREATE_CMD_(cmd_desc[0], cmd[0], cmd_byte_num, CMD_MODE_WRITE,
				bitmode);
		cmd_length = 1;
	}
	spiflash_read_write(cmd_desc, cmd_length, NULL);
}

__ramfunc int spiflash_cmd_poll_bit(struct spi_flash *flash,
	unsigned long timeout, u8_t cmd, u32_t poll_bit, u32_t bit_value)
{
	u32_t status = 0;

	do {

		spiflash_cmd_read(flash, &cmd, 1, 0xFFFFFFFF, &status, 1);
		if (bit_value) {
			if ((status & poll_bit))
				return 0;
		} else {
			if ((status & poll_bit) == 0)
				return 0;
		}

	} while (timeout--);

	spiflash_select_xip(TRUE);
	LOG_ERR("SF: time out!\n");

	return -1;
}

__ramfunc int spiflash_cmd_wait_ready(struct spi_flash *flash,
		unsigned long timeout)
{
	return spiflash_cmd_poll_bit(flash, timeout, CMD_READ_STATUS1,
			STATUS_WIP, 0);
}

__ramfunc int spiflash_cmd_erase(struct spi_flash *flash,
		u8_t erase_cmd, u32_t offset)
{
	u32_t addr;
	u32_t dummp;
	int ret = 0;
	SFC_CMD_DES_T cmd_desc[2];
	BIT_MODE_E bitmode = BIT_MODE_1;
	BYTE_NUM_E addr_byte_num = BYTE_NUM_3;

	if (QPI_MODE == flash->work_mode) {
		bitmode = BIT_MODE_4;
	}

	dummp = offset % flash->sector_size;
	if (dummp) {
		LOG_ERR("SF: Erase offset/length not multiple of erase size\n");
		return -1;
	}

	ret = spiflash_write_enable(flash);
	if (ret)
		goto out;
	addr = offset;
	addr_byte_num = spi_flash_addr(&addr, flash->support_4addr);

	CREATE_CMD_(cmd_desc[0], erase_cmd, BYTE_NUM_1, CMD_MODE_WRITE,
			bitmode);
	CREATE_CMD_(cmd_desc[1], addr, addr_byte_num, CMD_MODE_WRITE, bitmode);
	spiflash_read_write(cmd_desc, 2, NULL);

	ret = spiflash_cmd_wait_ready(flash, SPI_FLASH_CHIP_ERASE_TIMEOUT);
	if (ret)
		goto out;
out:

	spiflash_select_xip(TRUE);

	return ret;

}

static int
__attribute__ ((optimize("-O0"))) spiflash_write_page_sec(struct spi_flash
		*flash,
		SFC_CMD_DES_T *
		cmd_des_ptr,
		u32_t cmd_len,
		const void *data,
		u32_t data_len)
{
	u32_t i, j, k;
	u32_t piece_bytes_max = (10 - 2) * 4;
	u32_t cmd_write = cmd_des_ptr[0].cmd;
	u32_t addr = cmd_des_ptr[1].cmd;
	u32_t dest_addr = addr;
	u32_t *data_ptr32 = (u32_t *) (data);
	u32_t data_tmp[10];

	BIT_MODE_E bitmode_cmd = BIT_MODE_1;
	BIT_MODE_E bitmode_addr = BIT_MODE_1;
	BIT_MODE_E bitmode_data = BIT_MODE_1;
	SFC_CMD_DES_T cmd_desc[10];

	SCI_ASSERT(data_len <= flash->page_size);

	if (QPI_MODE == flash->work_mode) {
		bitmode_cmd = BIT_MODE_4;
		bitmode_addr = BIT_MODE_4;
		bitmode_data = BIT_MODE_4;
	} else {
		if (WR_CMD_4BIT == (flash->spi_rw_mode & WR_CMD_MSK)) {
			bitmode_cmd = BIT_MODE_4;
		} else if (WR_CMD_2BIT == (flash->spi_rw_mode & WR_CMD_MSK)) {
			bitmode_cmd = BIT_MODE_2;
		} else {
			bitmode_cmd = BIT_MODE_1;
		}

		if (WR_ADDR_4BIT == (flash->spi_rw_mode & WR_ADDR_MSK)) {
			bitmode_addr = BIT_MODE_4;
		} else if (WR_ADDR_2BIT == (flash->spi_rw_mode & WR_ADDR_MSK)) {
			bitmode_addr = BIT_MODE_2;
		} else {
			bitmode_addr = BIT_MODE_1;
		}

		if (WR_DATA_4BIT == (flash->spi_rw_mode & WR_DATA_MSK)) {
			bitmode_data = BIT_MODE_4;
		} else if (WR_DATA_2BIT == (flash->spi_rw_mode & WR_DATA_MSK)) {
			bitmode_data = BIT_MODE_2;
		} else {
			bitmode_data = BIT_MODE_1;
		}
	}

	for (i = 0; i < data_len;) {
		BYTE_NUM_E addr_byte_num = BYTE_NUM_3;
		u32_t buffer_cnt = 0;
		u32_t piece_cnt = min(piece_bytes_max, data_len - i);

		spiflash_write_enable(flash);

		dest_addr = addr;
		CREATE_CMD_(cmd_desc[buffer_cnt], cmd_write, BYTE_NUM_1,
				CMD_MODE_WRITE, bitmode_cmd);
		buffer_cnt++;
		CREATE_CMD_(cmd_desc[buffer_cnt], dest_addr, addr_byte_num,
				CMD_MODE_WRITE, bitmode_addr);
		buffer_cnt++;

		if (0 == (piece_cnt % (0x10))) {
			for (j = 0; j < piece_cnt;) {
				CREATE_CMD_(cmd_desc[buffer_cnt], *data_ptr32,
						BYTE_NUM_4, CMD_MODE_WRITE,
						bitmode_data);
				buffer_cnt++;
				data_ptr32++;
				j = j + 4;
			}
		} else {
			memset(data_tmp, 0xff, sizeof(data_tmp));
			memcpy(data_tmp, data_ptr32, piece_cnt);

			for (j = 0, k = 0;;) {
				CREATE_CMD_(cmd_desc[buffer_cnt], data_tmp[k],
						BYTE_NUM_4, CMD_MODE_WRITE,
						bitmode_data);
				buffer_cnt++;
				k++;
				j = j + 4;
				if ((j > piece_cnt) && (0 == (j % (0x10))))
					break;
			}
		}
		spiflash_read_write_sec(cmd_desc, buffer_cnt);

		if (spiflash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT))
			return -1;

		i = i + piece_cnt;
		addr = addr + piece_cnt;
	}
	return 0;
}

__ramfunc static int spiflash_write_page(struct spi_flash *flash,
		SFC_CMD_DES_T * cmd_des_ptr, u32_t cmd_len,
		const void *data, u32_t data_len)
{
	u32_t i, j;
	u32_t piece_bytes_max = (12 - 2) * 4;
	u32_t cmd_write = cmd_des_ptr[0].cmd;
	u32_t addr = cmd_des_ptr[1].cmd;
	u32_t dest_addr = addr;
	u32_t *data_ptr32 = (u32_t *) (data);

	BIT_MODE_E bitmode_cmd = BIT_MODE_1;
	BIT_MODE_E bitmode_addr = BIT_MODE_1;
	BIT_MODE_E bitmode_data = BIT_MODE_1;
	SFC_CMD_DES_T cmd_desc[12];

	SCI_ASSERT(data_len <= flash->page_size);

	if (QPI_MODE == flash->work_mode) {
		bitmode_cmd = BIT_MODE_4;
		bitmode_addr = BIT_MODE_4;
		bitmode_data = BIT_MODE_4;
	} else {
		if (WR_CMD_4BIT == (flash->spi_rw_mode & WR_CMD_MSK)) {
			bitmode_cmd = BIT_MODE_4;
		} else if (WR_CMD_2BIT == (flash->spi_rw_mode & WR_CMD_MSK)) {
			bitmode_cmd = BIT_MODE_2;
		} else {
			bitmode_cmd = BIT_MODE_1;
		}

		if (WR_ADDR_4BIT == (flash->spi_rw_mode & WR_ADDR_MSK)) {
			bitmode_addr = BIT_MODE_4;
		} else if (WR_ADDR_2BIT == (flash->spi_rw_mode & WR_ADDR_MSK)) {
			bitmode_addr = BIT_MODE_2;
		} else {
			bitmode_addr = BIT_MODE_1;
		}

		if (WR_DATA_4BIT == (flash->spi_rw_mode & WR_DATA_MSK)) {
			bitmode_data = BIT_MODE_4;
		} else if (WR_DATA_2BIT == (flash->spi_rw_mode & WR_DATA_MSK)) {
			bitmode_data = BIT_MODE_2;
		} else {
			bitmode_data = BIT_MODE_1;
		}
	}

	for (i = 0; i < data_len;) {
		BYTE_NUM_E addr_byte_num = BYTE_NUM_3;
		u32_t buffer_cnt = 0;
		u32_t piece_cnt = min(piece_bytes_max, data_len - i);

		spiflash_write_enable(flash);
		dest_addr = addr;
		addr_byte_num =
			spi_flash_addr(&dest_addr, flash->support_4addr);

		CREATE_CMD_(cmd_desc[buffer_cnt], cmd_write, BYTE_NUM_1,
				CMD_MODE_WRITE, bitmode_cmd);
		buffer_cnt++;
		CREATE_CMD_(cmd_desc[buffer_cnt], dest_addr, addr_byte_num,
				CMD_MODE_WRITE, bitmode_addr);
		buffer_cnt++;

		for (j = 0; j < piece_cnt;) {
			if ((piece_cnt - j) >= 4) {
				CREATE_CMD_(cmd_desc[buffer_cnt], *data_ptr32,
						BYTE_NUM_4, CMD_MODE_WRITE,
						bitmode_data);
				buffer_cnt++;
				data_ptr32++;
				j = j + 4;
			} else if (((piece_cnt - j) < 4)
					&& ((piece_cnt - j) % 4)) {
				u32_t tail_bytes = piece_cnt - j, byte_number =
					BYTE_NUM_1;
				switch (tail_bytes) {
					case 1:
						byte_number = BYTE_NUM_1;
						break;
					case 2:
						byte_number = BYTE_NUM_2;
						break;
					case 3:
						byte_number = BYTE_NUM_3;
						break;
					default:
						break;
				}
				CREATE_CMD_(cmd_desc[buffer_cnt], *data_ptr32,
						byte_number, CMD_MODE_WRITE,
						bitmode_data);
				buffer_cnt++;

				j = piece_cnt;
			}
		}

		spiflash_read_write(cmd_desc, buffer_cnt, NULL);

		if (spiflash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT))
			return -1;
		i = i + piece_cnt;
		addr = addr + piece_cnt;
	}
	return 0;
}

int __attribute__ ((optimize("-O0"))) spiflash_cmd_program_sec(struct spi_flash
		*flash,
		u32_t offset,
		u32_t len,
		const void *buf,
		u8_t cmd)
{
	unsigned long page_addr, page_size;
	unsigned int byte_addr;
	u32_t chunk_len, actual;
	SFC_CMD_DES_T cmd_desc[2];
	BIT_MODE_E bitmode = BIT_MODE_1;
	u32_t data_len, space_len;
	int ret = 0;

	page_size = flash->page_size;

	if (0 == offset) {
		byte_addr = 0;
		page_addr = 0;
	} else {
		page_addr = offset / page_size;
		byte_addr = offset % page_size;
	}

	if (ret < 0) {
		LOG_ERR("SF: enabling write failed \n");
		return ret;
	}

	for (actual = 0; actual < len; actual += chunk_len) {
		data_len = len - actual;
		space_len = page_size - byte_addr;
		chunk_len = min(data_len, space_len);
		ret = spiflash_write_enable(flash);

		if (QPI_MODE != flash->work_mode) {
			bitmode = BIT_MODE_4;
		}
		CREATE_CMD_(cmd_desc[0], cmd, BYTE_NUM_1, CMD_MODE_WRITE,
				bitmode);
		CREATE_CMD_(cmd_desc[1],
				(u32_t) (page_addr * page_size + byte_addr),
				BYTE_NUM_4, CMD_MODE_WRITE, bitmode);

		ret =
			spiflash_write_page_sec(flash, cmd_desc, 2,
					((char *)buf + actual), chunk_len);
		if (ret < 0) {
			LOG_ERR("SF: write failed\n");
			break;
		}

		page_addr++;
		byte_addr = 0;
	}
	return ret;
}

__ramfunc int spiflash_cmd_program(struct spi_flash *flash, u32_t offset,
		u32_t len, const void *buf, u8_t cmd)
{
	unsigned long page_addr, page_size;
	unsigned int byte_addr;
	u32_t chunk_len, actual;
	SFC_CMD_DES_T cmd_desc[2];
	BIT_MODE_E bitmode = BIT_MODE_1;
	u32_t data_len, space_len;
	int ret = 0;

	page_size = flash->page_size;

	if (0 == offset) {
		byte_addr = 0;
		page_addr = 0;
	} else {
		page_addr = offset / page_size;
		byte_addr = offset % page_size;
	}

	for (actual = 0; actual < len; actual += chunk_len) {
		data_len = len - actual;
		space_len = page_size - byte_addr;
		chunk_len = min(data_len, space_len);

		ret = spiflash_write_enable(flash);
		if (ret < 0) {
			spiflash_select_xip(TRUE);
			LOG_ERR("SF: enabling write failed\n");
			break;
		}

		if (QPI_MODE != flash->work_mode) {
			bitmode = BIT_MODE_4;
		}
		CREATE_CMD_(cmd_desc[0], cmd, BYTE_NUM_1, CMD_MODE_WRITE,
				bitmode);
		CREATE_CMD_(cmd_desc[1],
				(u32_t) (page_addr * page_size + byte_addr),
				BYTE_NUM_4, CMD_MODE_WRITE, bitmode);

		ret =
			spiflash_write_page(flash, cmd_desc, 2,
					((char *)buf + actual), chunk_len);


		spiflash_select_xip(TRUE);

		if (ret < 0) {
			LOG_ERR("SF: write failed\n");
			break;
		}

		page_addr++;
		byte_addr = 0;
	}
	return ret;
}

__ramfunc int spiflash_write_enable(struct spi_flash *flash)
{
	u32_t timeout = SPI_FLASH_WEL_TIMEOUT;
	u8_t cmd = CMD_WRITE_ENABLE;

	spiflash_cmd_write(flash, &cmd, 1, NULL, 0);

	return spiflash_cmd_poll_bit(flash, timeout, CMD_READ_STATUS1,
			STATUS_WEL, 1);
}

int spiflash_write_disable(struct spi_flash *flash)
{
	SFC_CMD_DES_T cmd_desc[2];
	BIT_MODE_E bitmode = BIT_MODE_1;

	if (QPI_MODE == flash->work_mode) {
		bitmode = BIT_MODE_4;
	}

	CREATE_CMD_(cmd_desc[0], CMD_WRITE_DISABLE, BYTE_NUM_1, CMD_MODE_WRITE,
			bitmode);
	spiflash_read_write(cmd_desc, 1, NULL);

	return 0;
}

__ramfunc int spiflash_lock(struct spi_flash *flash, u32_t offset, u32_t len)
{
	u8_t cmd;
	u8_t status1 = 0, status2 = 0;
	int dout = 0;
	int ret = 0;

	offset = 0;
	len = 0;

	spiflash_get_status(flash, &status1, &status2);
	spiflash_write_enable(flash);

	cmd = CMD_WRITE_STATUS;
	dout = (status2 << 8) | (status1) | (SR_LOCK);

	spiflash_cmd_write(flash, &cmd, 1, &dout, 2);

	ret = spiflash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);

	spiflash_select_xip(TRUE);

	return ret;
}

__ramfunc int spiflash_unlock(struct spi_flash *flash, u32_t offset, u32_t len)
{
	u8_t cmd;
	u8_t status1 = 0, status2 = 0;
	int dout = 0;
	int ret = 0;

	offset = 0;
	len = 0;

	spiflash_get_status(flash, &status1, &status2);
	spiflash_write_enable(flash);

	cmd = CMD_WRITE_STATUS;
	dout = (status2 << 8) | (status1) | (SR_UNLOCK);

	spiflash_cmd_write(flash, &cmd, 1, &dout, 2);

	ret = spiflash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);

	spiflash_select_xip(TRUE);

	return ret;
}

__ramfunc int spiflash_reset_anyway(void)
{
	int i = 0;
	SFC_CMD_DES_T cmd_desc[2];
	BIT_MODE_E bitmode = BIT_MODE_1;

	bitmode = BIT_MODE_4;
	CREATE_CMD_(cmd_desc[0], CMD_RSTEN, BYTE_NUM_1, CMD_MODE_WRITE,
			bitmode);
	spiflash_read_write(cmd_desc, 1, NULL);

	CREATE_CMD_(cmd_desc[0], CMD_RST, BYTE_NUM_1, CMD_MODE_WRITE, bitmode);
	spiflash_read_write(cmd_desc, 1, NULL);
	for (i = 0; i < 1000; i++) ;

	bitmode = BIT_MODE_1;
	CREATE_CMD_(cmd_desc[0], CMD_RSTEN, BYTE_NUM_1, CMD_MODE_WRITE,
			bitmode);
	spiflash_read_write(cmd_desc, 1, NULL);
	CREATE_CMD_(cmd_desc[0], CMD_RST, BYTE_NUM_1, CMD_MODE_WRITE, bitmode);
	spiflash_read_write(cmd_desc, 1, NULL);
	for (i = 0; i < 1000; i++) ;

	return 0;
}

int spiflash_write_sec(struct spi_flash *flash, u32_t offset,
		u32_t len, const void *buf)
{
	return spiflash_cmd_program_sec(flash, offset, len, buf,
			CMD_PAGE_PROGRAM);
}

__ramfunc int spiflash_write(struct spi_flash *flash, u32_t offset,
		u32_t len, const void *buf)
{
	return spiflash_cmd_program(flash, offset, len, buf, CMD_PAGE_PROGRAM);
}

__ramfunc int spiflash_erase(struct spi_flash *flash, u32_t offset, u32_t len)
{
	u32_t i, sectors_nr;
	u32_t dummp;

	dummp = len % flash->sector_size;
	if (dummp) {
		LOG_ERR("SF: Erase param length not multiple of erase size\n");
		return -1;
	}

	sectors_nr = len / flash->sector_size;
	for (i = 0; i < sectors_nr; i++) {
		if (spiflash_cmd_erase(flash, CMD_SECTOR_ERASE, offset))
			return -1;
		offset += flash->sector_size;
	}
	return 0;
}

int spiflash_suspend(struct spi_flash *flash)
{
	u8_t cmd = CMD_PE_SUSPEND;

	spiflash_cmd_write(flash, &cmd, 1, NULL, 0);
	return 0;
}

int spiflash_resume(struct spi_flash *flash)
{
	u8_t cmd = CMD_PE_RESUME;

	spiflash_cmd_write(flash, &cmd, 1, NULL, 0);
	return 0;
}

__ramfunc int spiflash_erase_chip(struct spi_flash *flash)
{
	int ret = 0;

	u8_t cmd = CMD_CHIP_ERASE;

	spiflash_write_enable(flash);
	spiflash_cmd_write(flash, &cmd, 1, NULL, 0);

	ret = spiflash_cmd_wait_ready(flash, SPI_FLASH_CHIP_ERASE_TIMEOUT);

	spiflash_select_xip(TRUE);

	return ret;
}

u32_t spiflash_read_common(struct spi_flash * flash, u32_t offset)
{
	/* size: 4MB base: 0x200_0000 */
	return offset;
}

int spiflash_read_data_xip(struct spi_flash *flash, u32_t offset,
		u32_t * buf, u32_t dump_byte, READ_CMD_TYPE_E type)
{
	u8_t cmd;
	u32_t off_cmd;


	switch (type) {
		case READ_SPI:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd = CMD_NORMAL_READ;
			flash->spi_rw_mode = RD_CMD_1BIT | RD_ADDR_1BIT | RD_DATA_1BIT;
			break;
		case READ_SPI_FAST:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd = CMD_FAST_READ;
			flash->spi_rw_mode =
				RD_CMD_1BIT | RD_ADDR_1BIT | RD_DUMY_1BIT | RD_DATA_1BIT;
			break;
		case READ_SPI_2IO:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd = CMD_2IO_READ;
			flash->spi_rw_mode =
				RD_CMD_1BIT | RD_ADDR_2BIT | RD_DUMY_2BIT | RD_DATA_2BIT;
			break;
		case READ_SPI_4IO:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd = CMD_4IO_READ;
			flash->spi_rw_mode =
				RD_CMD_1BIT | RD_ADDR_4BIT | RD_DUMY_4BIT | RD_DATA_4BIT;
			break;
		case READ_QPI_4IO:
			if (QPI_MODE != flash->work_mode)
				return -1;
			cmd = CMD_4IO_READ;
			break;
		default:
			return -1;
	}

	off_cmd = offset << 8 | cmd;
	spiflash_set_xip_cmd(flash, (const u8_t *)(&off_cmd), dump_byte);
	*buf = spiflash_read_common(flash, offset);

	return 0;
}

int spiflash_read_data_noxip(struct spi_flash *flash, u32_t address,
		u32_t * buf, u32_t buf_size, READ_CMD_TYPE_E type)
{
	u8_t cmd[1] = { 0 };
	u32_t pbuf_size = 0;
	u8_t *pbuf = NULL;
	u32_t block_size = 8;

	switch (type) {
		case READ_SPI:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd[0] = CMD_NORMAL_READ;
			flash->spi_rw_mode = RD_CMD_1BIT | RD_ADDR_1BIT | RD_DATA_1BIT;
			break;
		case READ_SPI_FAST:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd[0] = CMD_FAST_READ;
			flash->spi_rw_mode =
				RD_CMD_1BIT | RD_ADDR_1BIT | RD_DUMY_1BIT | RD_DATA_1BIT;
			break;
		case READ_SPI_2IO:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd[0] = CMD_2IO_READ;
			flash->spi_rw_mode =
				RD_CMD_1BIT | RD_ADDR_2BIT | RD_DUMY_2BIT | RD_DATA_2BIT;
			break;
		case READ_SPI_4IO:
			if (SPI_MODE != flash->work_mode)
				return -1;
			cmd[0] = CMD_4IO_READ;
			flash->spi_rw_mode =
				RD_CMD_1BIT | RD_ADDR_4BIT | RD_DUMY_4BIT | RD_DATA_4BIT;
			break;
		case READ_QPI_4IO:
			if (QPI_MODE != flash->work_mode)
				return -1;
			cmd[0] = CMD_4IO_READ;
			break;
		default:
			return -1;
	}

	pbuf = (u8_t *)buf;
	pbuf_size = buf_size;
	while (pbuf_size > block_size) {
		spiflash_cmd_read(flash, cmd, 1, address, pbuf, block_size);
		address += block_size;
		pbuf += block_size;
		pbuf_size -= block_size;
	}
	spiflash_cmd_read(flash, cmd, 1, address, pbuf, block_size);

	return 0;
}

static int spiflash_set_encrypt(u32_t op)
{
	sfcdrv_setcmdencryptcfgreg(op);
	return 0;
}

static int spiflash_change_4io(struct spi_flash *flash, u32_t op)
{
	u32_t dout = 0;
	u8_t status1 = 0, status2 = 0;
	u8_t cmd = 0;
	int ret = 0;

	spiflash_get_status(flash, &status1, &status2);
	spiflash_write_enable(flash);
	cmd = CMD_WRITE_STATUS;
	if (op == TRUE) {
		dout = ((status2 << 8) + (status1)) | (SR_QE);
	} else {
		dout = ((status2 << 8) + (status1)) & (~SR_QE);
	}

	spiflash_cmd_write(flash, &cmd, 1, &dout, 2);

	ret = spiflash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);

	spiflash_select_xip(TRUE);

	return ret;
}

static int spiflash_enter_qpi(struct spi_flash *flash)
{
	int para = 0;
	u8_t cmd = 0;
	u8_t dummy_clocks = flash->dummy_bytes;

	spiflash_change_4io(flash, TRUE);

	cmd = CMD_ENTER_QPI;
	spiflash_cmd_write(flash, &cmd, 1, NULL, 0);
	flash->work_mode = QPI_MODE;

	switch (dummy_clocks) {
		case DUMMY_2CLOCKS:
			para = DUMMYCLK_2_FREQ_50MHZ;
			break;
		case DUMMY_4CLOCKS:
			para = DUMMYCLK_4_FREQ_80MHZ;
			break;
		case DUMMY_6CLOCKS:
			para = DUMMYCLK_6_FREQ_104MHZ;
			break;
		default:
			break;
	}

	cmd = CMD_SETBURST;
	spiflash_cmd_write(flash, &cmd, 1, &para, 1);
	return spiflash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
}

static int spiflash_exit_qpi(struct spi_flash *flash)
{
	u8_t cmd = 0;
	spiflash_change_4io(flash, FALSE);
	cmd = CMD_EXIT_QPI;
	spiflash_cmd_write(flash, &cmd, 1, NULL, 0);
	flash->work_mode = SPI_MODE;
	return 0;
}

static int spiflash_set_qpi(struct spi_flash *flash, u32_t op)
{
	if (TRUE == op) {
		return spiflash_enter_qpi(flash);
	} else {
		return spiflash_exit_qpi(flash);
	}
}

static int spiflash_read(struct spi_flash *flash, u32_t offset,
		u32_t *buf, u32_t len, READ_CMD_TYPE_E type)
{

	unsigned int key;
	int ret = TRUE;

#ifndef CONFIG_W25_READONLY

	u8_t dump_byte = 1;
	u32_t read_char = 0;

	/* modify for xip-sfc */
	key = irq_lock_primask();

	ret = spiflash_read_data_xip(flash, offset, (u32_t *) &read_char,
				     dump_byte, type);
	irq_unlock_primask(key);

	memcpy((char *)buf, (char *)read_char, len);

#else
	LOG_INF("defined CONFIG_W25_READONLY\n");
	memcpy((char *)buf, (char *)(offset), len);
#endif

	return ret;
}

__ramfunc static void spiflash_exit_xip(void)
{
	/* pin mux and drive configuration */
	sci_write32(PIN_ESMCLK_REG, 0x4100);

	sci_write32(SFC_CMD_CFG, 0x00);
	sci_write32(SFC_CMD_BUF0, 0x06);
	sci_write32(SFC_TYPE_BUF0, 0x01);
	sci_write32(SFC_TYPE_BUF1, 0x00000000);
	sci_write32(SFC_TYPE_BUF2, 0x00000000);
}

__ramfunc static void spiflash_enter_xip(void)
{
	sci_write32(SFC_CMD_CFG, 0x01);
	sci_write32(SFC_CMD_BUF0, 0x0b);
	sci_write32(SFC_TYPE_BUF0, 0x419101);
	sci_write32(SFC_TYPE_BUF1, 0x00000000);
	sci_write32(SFC_TYPE_BUF2, 0x00000000);
}

__ramfunc void spiflash_select_xip(u32_t op)
{
	if (op == TRUE) {
		spiflash_enter_xip();
	} else {
		spiflash_exit_xip();
	}
}

__ramfunc void spiflash_set_clk(void)
{
	sfcdrv_clkcfg(SFC_CLK_OUT_DIV_1 | SFC_CLK_OUT_2X_EN |
			SFC_CLK_2X_EN | SFC_CLK_SAMPLE_2X_PHASE |
			SFC_CLK_SAMPLE_2X_EN);

/*
 *  cgm_sfc_1x_div: clk_sfc_1x = clk_src/(bit 9:8 + 1)
 */
	sci_write32(REG_AON_CLK_RF_CGM_SFC_1X_CFG, 0x100);

/*
 *  0: xtal MHz 1: 133MHz 2: 139MHz 3: 160MHz 4: 208MHz
 *  cgm_sfc_2x_sel: clk_sfc_1x source (bit 2:1:0)
 */
	sci_write32(REG_AON_CLK_RF_CGM_SFC_2X_CFG, 0x4);
}

void uwp_spi_xip_init(void)
{
	sfcdrv_clkcfg(SFC_CLK_OUT_DIV_2 | SFC_CLK_OUT_2X_EN |
			SFC_CLK_2X_EN | SFC_CLK_SAMPLE_2X_PHASE |
			SFC_CLK_SAMPLE_2X_EN);

	/*
	 * cgm_sfc_1x_div: clk_sfc_1x = clk_src/(bit 9:8 + 1)
	 * */
	sci_write32(REG_AON_CLK_RF_CGM_SFC_1X_CFG, 0x100);
	/* 0: xtal MHz 1: 133MHz 2: 139MHz 3: 160MHz 4: 208MHz
	 * cgm_sfc_2x_sel: clk_sfc_1x source (bit 2:1:0)
	 * */
	sci_write32(REG_AON_CLK_RF_CGM_SFC_2X_CFG, 0x4);
}

__ramfunc int uwp_spi_flash_init(struct spi_flash *flash,
		struct spi_flash_params **params)
{

	struct spi_flash_params *p = spiflash_scan();

	if (p == NULL) {
		LOG_ERR("[%s] == p:[%p], scan fail!\n", __func__, p);
		return -1;
	}

	flash->name = p->name;
	flash->work_mode = SPI_MODE;
	flash->support_4addr = FALSE;
	flash->dummy_bytes = p->dummy_clocks;

	flash->page_size = p->page_size;
	flash->sector_size = p->page_size * p->sector_size;
	flash->size = p->page_size * p->sector_size
		* p->nr_sectors * p->nr_blocks;

	*params = p;

	/* flash.read_sec_noxip = spiflash_read_data_noxip; */
	flash->read_sec = spiflash_read_data_xip;
	flash->write_sec = spiflash_write_sec;
	/* flash.read_noxip = spiflash_read_data_noxip; */
	flash->read = spiflash_read;
	flash->write = spiflash_write;
	flash->erase = spiflash_erase;
	flash->erase_chip = spiflash_erase_chip;
	flash->reset = spiflash_reset_anyway;
	flash->suspend = spiflash_suspend;
	flash->resume = spiflash_resume;
	flash->wren = spiflash_write_enable;
	flash->lock = spiflash_lock;
	flash->unlock = spiflash_unlock;
	flash->set_4io = spiflash_change_4io;
	flash->set_qpi = spiflash_set_qpi;
	flash->set_encrypt = spiflash_set_encrypt;


	LOG_INF("SF: Detected %s with page size %u,"
			" total ", flash->name, flash->page_size);
	LOG_INF("flash size is 0x%x\n", flash->size);

#ifdef CONFIG_QPI_MODE_ENABLE
	spiflash_set_qpi(flash_ctr(), TRUE);
#endif

	return 0;
}

