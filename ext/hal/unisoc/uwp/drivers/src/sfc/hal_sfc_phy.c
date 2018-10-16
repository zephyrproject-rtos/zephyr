/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>

#include <hal_sfc.h>
#include <hal_sfc_phy.h>
#include <hal_sfc_hal.h>

static SFC_REG_T *reg_sfc = (SFC_REG_T *) BASE_AON_SFC_CFG;
u32_t cmd_buf_index = 0;
u32_t read_buf_index = 7;

void sfcdrv_setcmdencryptcfgreg(u32_t cmdmode)
{
	reg_sfc->cmd_cfg.mBits.encrypt_de_en = cmdmode;
}

__ramfunc void sfcdrv_setcmdcfgreg(CMD_MODE_E cmdmode, BIT_MODE_E bitmode,
			 INI_ADD_SEL_E iniAddSel)
{
	reg_sfc->cmd_cfg.mBits.cmd_set = cmdmode;
	reg_sfc->cmd_cfg.mBits.rdata_bit_mode = bitmode;
	reg_sfc->cmd_cfg.mBits.sts_ini_addr_sel = iniAddSel;
}

volatile u32_t s_while_count1 = 0;

__ramfunc void sfcdrv_softreq(void)
{
	reg_sfc->soft_req.mBits.soft_req = BIT(0);
}

__ramfunc void sfcdrv_cmdbufclr(void)
{
	reg_sfc->cmd_buf0.dwValue = 0;
	reg_sfc->cmd_buf1.dwValue = 0;
	reg_sfc->cmd_buf2.dwValue = 0;
	reg_sfc->cmd_buf3.dwValue = 0;
	reg_sfc->cmd_buf4.dwValue = 0;
	reg_sfc->cmd_buf5.dwValue = 0;
	reg_sfc->cmd_buf6.dwValue = 0;
	reg_sfc->cmd_buf7.dwValue = 0;
	reg_sfc->cmd_buf8.dwValue = 0;
	reg_sfc->cmd_buf9.dwValue = 0;
	reg_sfc->cmd_buf10.dwValue = 0;
	reg_sfc->cmd_buf11.dwValue = 0;
}

__ramfunc void sfcdrv_typebufclr(void)
{
	reg_sfc->tbuf_clr.mBits.tbuf_clr = BIT(0);
}

__ramfunc void sfcdrv_intclr(void)
{
	reg_sfc->int_clr.mBits.int_clr = BIT(0);
}

u32_t sfcdrv_getstatus(void)
{
	return reg_sfc->status.dwValue;
}

void sfcdrv_cstimingcfg(u32_t value)
{
	reg_sfc->cs_timing_cfg.dwValue = value;
}

void sfcdrv_rdtimingcfg(u32_t value)
{
	reg_sfc->rd_timing_cfg.dwValue = value;
}

__ramfunc void sfcdrv_clkcfg(u32_t value)
{
	reg_sfc->clk_cfg.dwValue = value;
}

void sfcdrv_cscfg(u32_t value)
{
	reg_sfc->cs_cfg.dwValue = 0;
}

void sfcdrv_endiancfg(u32_t value)
{
	reg_sfc->endian_cfg.dwValue = value;
}

__ramfunc void sfcdrv_setcmdbuf(CMD_BUF_INDEX_E index, u32_t value)
{
	switch (index) {
	case CMD_BUF_0:
		reg_sfc->cmd_buf0.mBits.cmd_buf0 = value;
		break;

	case CMD_BUF_1:
		reg_sfc->cmd_buf1.mBits.cmd_buf1 = value;
		break;

	case CMD_BUF_2:
		reg_sfc->cmd_buf2.mBits.cmd_buf2 = value;
		break;

	case CMD_BUF_3:
		reg_sfc->cmd_buf3.mBits.cmd_buf3 = value;
		break;

	case CMD_BUF_4:
		reg_sfc->cmd_buf4.mBits.cmd_buf4 = value;
		break;

	case CMD_BUF_5:
		reg_sfc->cmd_buf5.mBits.cmd_buf5 = value;
		break;

	case CMD_BUF_6:
		reg_sfc->cmd_buf6.mBits.cmd_buf6 = value;
		break;

	case CMD_BUF_7:
		reg_sfc->cmd_buf7.mBits.cmd_buf7 = value;
		break;

	case CMD_BUF_8:
		reg_sfc->cmd_buf8.mBits.cmd_buf8 = value;
		break;

	case CMD_BUF_9:
		reg_sfc->cmd_buf9.mBits.cmd_buf9 = value;
		break;

	case CMD_BUF_10:
		reg_sfc->cmd_buf10.mBits.cmd_buf10 = value;
		break;

	case CMD_BUF_11:
		reg_sfc->cmd_buf11.mBits.cmd_buf11 = value;
		break;
	default:
		break;
	}
}

void sfcdrv_setcmdbufex(CMD_BUF_INDEX_E index, const u8_t *buf, u32_t count)
{
	SFC_CMD_BUF0_U *cmd_buf;

	cmd_buf =
	    (SFC_CMD_BUF0_U *) ((u32_t) & reg_sfc->cmd_buf0 +
				index * sizeof(SFC_CMD_BUF0_U));
	memcpy((u8_t *) & cmd_buf->dwValue, buf, count);
}

__ramfunc u32_t sfcdrv_getcmdbuf(CMD_BUF_INDEX_E index)
{
	u32_t value = 0;

	switch (index) {
	case CMD_BUF_0:
		value = reg_sfc->cmd_buf0.mBits.cmd_buf0;
		break;

	case CMD_BUF_1:
		value = reg_sfc->cmd_buf1.mBits.cmd_buf1;
		break;

	case CMD_BUF_2:
		value = reg_sfc->cmd_buf2.mBits.cmd_buf2;
		break;

	case CMD_BUF_3:
		value = reg_sfc->cmd_buf3.mBits.cmd_buf3;
		break;

	case CMD_BUF_4:
		value = reg_sfc->cmd_buf4.mBits.cmd_buf4;
		break;

	case CMD_BUF_5:
		value = reg_sfc->cmd_buf5.mBits.cmd_buf5;
		break;

	case CMD_BUF_6:
		value = reg_sfc->cmd_buf6.mBits.cmd_buf6;
		break;

	case CMD_BUF_7:
		value = reg_sfc->cmd_buf7.mBits.cmd_buf7;
		break;

	case CMD_BUF_8:
		value = reg_sfc->cmd_buf8.mBits.cmd_buf8;
		break;

	case CMD_BUF_9:
		value = reg_sfc->cmd_buf9.mBits.cmd_buf9;
		break;

	case CMD_BUF_10:
		value = reg_sfc->cmd_buf10.mBits.cmd_buf10;
		break;

	case CMD_BUF_11:
		value = reg_sfc->cmd_buf11.mBits.cmd_buf11;
		break;

	default:
		break;
	}
	return value;
}

__ramfunc void sfcdrv_settypeinfbuf(CMD_BUF_INDEX_E index, BIT_MODE_E bitmode,
			  BYTE_NUM_E bytenum, CMD_MODE_E cmdmode,
			  SEND_MODE_E sendmode)
{
	switch (index) {
	case CMD_BUF_0:
		reg_sfc->type_buf0.mBits.valid0 = BIT(0);
		reg_sfc->type_buf0.mBits.bit_mode0 = bitmode;
		reg_sfc->type_buf0.mBits.byte_num0 = bytenum;
		reg_sfc->type_buf0.mBits.operation_status0 = cmdmode;
		reg_sfc->type_buf0.mBits.byte_send_mode0 = sendmode;
		break;

	case CMD_BUF_1:
		reg_sfc->type_buf0.mBits.valid1 = BIT(0);
		reg_sfc->type_buf0.mBits.bit_mode1 = bitmode;
		reg_sfc->type_buf0.mBits.byte_num1 = bytenum;
		reg_sfc->type_buf0.mBits.operation_status1 = cmdmode;
		reg_sfc->type_buf0.mBits.byte_send_mode1 = sendmode;
		break;

	case CMD_BUF_2:
		reg_sfc->type_buf0.mBits.valid2 = BIT(0);
		reg_sfc->type_buf0.mBits.bit_mode2 = bitmode;
		reg_sfc->type_buf0.mBits.byte_num2 = bytenum;
		reg_sfc->type_buf0.mBits.operation_status2 = cmdmode;
		reg_sfc->type_buf0.mBits.byte_send_mode2 = sendmode;
		break;

	case CMD_BUF_3:
		reg_sfc->type_buf0.mBits.valid3 = BIT(0);
		reg_sfc->type_buf0.mBits.bit_mode3 = bitmode;
		reg_sfc->type_buf0.mBits.byte_num3 = bytenum;
		reg_sfc->type_buf0.mBits.operation_status3 = cmdmode;
		reg_sfc->type_buf0.mBits.byte_send_mode3 = sendmode;
		break;

	case CMD_BUF_4:
		reg_sfc->type_buf1.mBits.valid4 = BIT(0);
		reg_sfc->type_buf1.mBits.bit_mode4 = bitmode;
		reg_sfc->type_buf1.mBits.byte_num4 = bytenum;
		reg_sfc->type_buf1.mBits.operation_status4 = cmdmode;
		reg_sfc->type_buf1.mBits.byte_send_mode4 = sendmode;
		break;

	case CMD_BUF_5:
		reg_sfc->type_buf1.mBits.valid5 = BIT(0);
		reg_sfc->type_buf1.mBits.bit_mode5 = bitmode;
		reg_sfc->type_buf1.mBits.byte_num5 = bytenum;
		reg_sfc->type_buf1.mBits.operation_status5 = cmdmode;
		reg_sfc->type_buf1.mBits.byte_send_mode5 = sendmode;
		break;

	case CMD_BUF_6:
		reg_sfc->type_buf1.mBits.valid6 = BIT(0);
		reg_sfc->type_buf1.mBits.bit_mode6 = bitmode;
		reg_sfc->type_buf1.mBits.byte_num6 = bytenum;
		reg_sfc->type_buf1.mBits.operation_status6 = cmdmode;
		reg_sfc->type_buf1.mBits.byte_send_mode6 = sendmode;
		break;

	case CMD_BUF_7:
		reg_sfc->type_buf1.mBits.valid7 = BIT(0);
		reg_sfc->type_buf1.mBits.bit_mode7 = bitmode;
		reg_sfc->type_buf1.mBits.byte_num7 = bytenum;
		reg_sfc->type_buf1.mBits.operation_status7 = cmdmode;
		reg_sfc->type_buf1.mBits.byte_send_mode7 = sendmode;
		break;

	case CMD_BUF_8:
		reg_sfc->type_buf2.mBits.valid8 = BIT(0);
		reg_sfc->type_buf2.mBits.bit_mode8 = bitmode;
		reg_sfc->type_buf2.mBits.byte_num8 = bytenum;
		reg_sfc->type_buf2.mBits.operation_status8 = cmdmode;
		reg_sfc->type_buf2.mBits.byte_send_mode8 = sendmode;
		break;

	case CMD_BUF_9:
		reg_sfc->type_buf2.mBits.valid9 = BIT(0);
		reg_sfc->type_buf2.mBits.bit_mode9 = bitmode;
		reg_sfc->type_buf2.mBits.byte_num9 = bytenum;
		reg_sfc->type_buf2.mBits.operation_status9 = cmdmode;
		reg_sfc->type_buf2.mBits.byte_send_mode9 = sendmode;
		break;

	case CMD_BUF_10:
		reg_sfc->type_buf2.mBits.valid10 = BIT(0);
		reg_sfc->type_buf2.mBits.bit_mode10 = bitmode;
		reg_sfc->type_buf2.mBits.byte_num10 = bytenum;
		reg_sfc->type_buf2.mBits.operation_status10 = cmdmode;
		reg_sfc->type_buf2.mBits.byte_send_mode10 = sendmode;
		break;

	case CMD_BUF_11:
		reg_sfc->type_buf2.mBits.valid11 = BIT(0);
		reg_sfc->type_buf2.mBits.bit_mode11 = bitmode;
		reg_sfc->type_buf2.mBits.byte_num11 = bytenum;
		reg_sfc->type_buf2.mBits.operation_status11 = cmdmode;
		reg_sfc->type_buf2.mBits.byte_send_mode11 = sendmode;
		break;

	default:
		break;
	}
}

void sfcdrv_setinitaddr(INI_ADD_SEL_E start_addr)
{
	reg_sfc->cmd_cfg.mBits.sts_ini_addr_sel = start_addr;
}

__ramfunc u32_t sfcdrv_getinitaddr(void)
{
	u32_t start_addr = 0;

	start_addr = reg_sfc->cmd_cfg.mBits.sts_ini_addr_sel;

	switch (start_addr) {
	case INI_CMD_BUF_7:
		start_addr = 7;
		break;

	case INI_CMD_BUF_6:
		start_addr = 6;
		break;

	case INI_CMD_BUF_5:
		start_addr = 5;
		break;

	case INI_CMD_BUF_4:
		start_addr = 4;
		break;

	default:
		start_addr = 7;
		break;
	}
	return start_addr;
}

void sfcdrv_waitcmddone(void)
{
	while (!(SFC_IDLE_STATUS & sfcdrv_getstatus())) {
	};
}

__ramfunc void sfcdrv_setcmddata(SFC_CMD_DES_T *cmd_des_ptr, u32_t cmd_flag)
{
	if (cmd_flag == SPI_CMD_DATA_BEGIN) {
		sfcdrv_cmdbufclr();
		sfcdrv_typebufclr();
		cmd_buf_index = 0;
		read_buf_index = sfcdrv_getinitaddr();
	}

	if (cmd_des_ptr != NULL) {
		sfcdrv_setcmdbuf(cmd_buf_index, cmd_des_ptr->cmd);
		sfcdrv_settypeinfbuf(cmd_buf_index,
				     cmd_des_ptr->bit_mode,
				     cmd_des_ptr->cmd_byte_len,
				     cmd_des_ptr->cmd_mode,
				     cmd_des_ptr->send_mode);
		cmd_buf_index++;
	}

	if (cmd_flag == SPI_CMD_DATA_END) {
		sfcdrv_req();
	}
}

__ramfunc void sfcdrv_setreadbuf(SFC_CMD_DES_T *cmd_des_ptr, u32_t cmd_flag)
{
	if (cmd_flag == SPI_CMD_DATA_BEGIN) {
		sfcdrv_cmdbufclr();
		sfcdrv_typebufclr();
		cmd_buf_index = 0;
		read_buf_index = sfcdrv_getinitaddr();
	}

	if (cmd_des_ptr != NULL) {
		sfcdrv_setcmdbuf(read_buf_index, cmd_des_ptr->cmd);
		sfcdrv_settypeinfbuf(read_buf_index,
				     cmd_des_ptr->bit_mode,
				     cmd_des_ptr->cmd_byte_len,
				     cmd_des_ptr->cmd_mode,
				     cmd_des_ptr->send_mode);
		read_buf_index++;
	}

	if (cmd_flag == SPI_CMD_DATA_END) {
		sfcdrv_req();
	}
}

void sfcdrv_getbuf(void *buffer, u32_t nbytes)
{
	u32_t i = 0, cnt = 0, temp = 0;
	u32_t _start_index = sfcdrv_getinitaddr();
	u32_t data_in = 0;
	u8_t *data_ptr8 = (u8_t *) (&data_in);

	for (cnt = _start_index; (cnt < read_buf_index) && (i < nbytes); cnt++) {
		data_in = sfcdrv_getcmdbuf(cnt);
		temp = (data_ptr8[0] << 24) | (data_ptr8[1] << 16) |
		    (data_ptr8[2] << 8) | (data_ptr8[3] << 0);
		if (nbytes - i < 4) {
			memcpy((void *)((u32_t)buffer + i), &temp, nbytes - i);
			i += nbytes - i;
		} else {
			memcpy((void *)((u32_t)buffer + i), &temp, 4);
			i += 4;
		}
	}
}

__ramfunc void sfcdrv_getreadbuf(u32_t *buffer, u32_t word_cnt)
{
	u32_t i = 0;
	u32_t cnt = 0;
	u32_t _start_index = sfcdrv_getinitaddr();
	u32_t data_in = 0;
	u8_t *data_ptr8 = (u8_t *) (&data_in);

	for (cnt = _start_index; (cnt < read_buf_index) && (i < word_cnt);
	     cnt++) {
		data_in = sfcdrv_getcmdbuf(cnt);
		buffer[i++] = (data_ptr8[0] << 24) | (data_ptr8[1] << 16) |
		    (data_ptr8[2] << 8) | (data_ptr8[3] << 0);
	}
}

__ramfunc void sfcdrv_resetallbuf(void)
{
	sfcdrv_cmdbufclr();
	sfcdrv_typebufclr();
	cmd_buf_index = 0;
	read_buf_index = sfcdrv_getinitaddr();
}

__ramfunc void sfcdrv_intcfg(u32_t op)
{
	if (op == TRUE) {
		*((volatile u32_t *)SFC_IEN) = 0xFF;
	} else {
		*((volatile u32_t *)SFC_IEN) = 0x00;
	}
}

__ramfunc void sfcdrv_req(void)
{
	sfcdrv_intclr();
	sfcdrv_softreq();

#if 0
	while (!(SFC_IDLE_STATUS & sfcdrv_getstatus())) {
	};
#else
	while (0 == (*((volatile u32_t *)SFC_INT_RAW))) {
	};
#endif
}
