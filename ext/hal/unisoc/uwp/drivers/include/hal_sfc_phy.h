/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MARLIN3_HAL_SFC_PHY_H
#define __MARLIN3_HAL_SFC_PHY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define SFC_CMD_CFG			(BASE_AON_SFC_CFG + 0x0000)
#define SFC_SOFT_REQ        (BASE_AON_SFC_CFG + 0x0004)
#define SFC_TBUF_CLR        (BASE_AON_SFC_CFG + 0x0008)
#define SFC_INT_CLR         (BASE_AON_SFC_CFG + 0x000C)
#define SFC_STATUS          (BASE_AON_SFC_CFG + 0x0010)
#define SFC_CS_TIMING_CFG   (BASE_AON_SFC_CFG + 0x0014)
#define SFC_RD_SAMPLE_CFG   (BASE_AON_SFC_CFG + 0x0018)
#define SFC_CLK_CFG         (BASE_AON_SFC_CFG + 0x001C)
#define SFC_CS_CFG          (BASE_AON_SFC_CFG + 0x0020)
#define SFC_ENDIAN_CFG      (BASE_AON_SFC_CFG + 0x0024)
#define SFC_IO_DLY_CFG      (BASE_AON_SFC_CFG + 0x0028)
#define SFC_WP_HLD_INIT     (BASE_AON_SFC_CFG + 0x002C)
#define SFC_CMD_BUF0        (BASE_AON_SFC_CFG + 0x0040)
#define SFC_CMD_BUF1        (BASE_AON_SFC_CFG + 0x0044)
#define SFC_CMD_BUF2        (BASE_AON_SFC_CFG + 0x0048)
#define SFC_CMD_BUF3        (BASE_AON_SFC_CFG + 0x004C)
#define SFC_CMD_BUF4        (BASE_AON_SFC_CFG + 0x0050)
#define SFC_CMD_BUF5        (BASE_AON_SFC_CFG + 0x0054)
#define SFC_CMD_BUF6        (BASE_AON_SFC_CFG + 0x0058)
#define SFC_CMD_BUF7        (BASE_AON_SFC_CFG + 0x005C)
#define SFC_CMD_BUF8        (BASE_AON_SFC_CFG + 0x0060)
#define SFC_CMD_BUF9        (BASE_AON_SFC_CFG + 0x0064)
#define SFC_CMD_BUF10       (BASE_AON_SFC_CFG + 0x0068)
#define SFC_CMD_BUF11       (BASE_AON_SFC_CFG + 0x006C)
#define SFC_TYPE_BUF0       (BASE_AON_SFC_CFG + 0x0070)
#define SFC_TYPE_BUF1       (BASE_AON_SFC_CFG + 0x0074)
#define SFC_TYPE_BUF2       (BASE_AON_SFC_CFG + 0x0078)

#define SFC_IEN             (BASE_AON_SFC_CFG + 0x0204)
#define SFC_INT_RAW			(BASE_AON_SFC_CFG + 0x0208)
#define SFC_INT_STS         (BASE_AON_SFC_CFG + 0x020C)


	typedef union sfc_cmd_cfg_tag {
		struct sfc_cmd_cfg_map {
			volatile unsigned int cmd_set:1;
			volatile unsigned int rdata_bit_mode:2;
			volatile unsigned int sts_ini_addr_sel:2;
			volatile unsigned int reserved0:6;
			volatile unsigned int encrypt_de_en:1;
			volatile unsigned int crc_en:1;
			volatile unsigned int reserved:19;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_CFG_U;

	typedef union sfc_soft_req_tag {
		struct sfc_soft_req_map {
			volatile unsigned int soft_req:1;
			volatile unsigned int reserved:31;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_SOFT_REQ_U;

	typedef union sfc_tbuf_clr_tag {
		struct sfc_tbuf_clr_map {
			volatile unsigned int tbuf_clr:1;
			volatile unsigned int reserved:31;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_TBUF_CLR_U;

	typedef union sfc_int_clr_tag {
		struct sfc_int_clr_map {
			volatile unsigned int int_clr:1;
			volatile unsigned int reserved:30;  //???
		} mBits;
		volatile unsigned int dwValue;
	} SFC_INT_CLR_U;

	typedef union sfc_status_tag {
		struct sfc_status_map {
			volatile unsigned int int_req:1;
			volatile unsigned int idle:1;
			volatile unsigned int cur_state:4;
			volatile unsigned int reserved:26;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_STATUS_U;

	typedef union sfc_cs_timing_cfg_tag {
		struct sfc_cs_timing_cfg_map {
			volatile unsigned int tslch_dly:1;
			volatile unsigned int tshch_dly:1;
			volatile unsigned int csh_clkout0_en:1;
			volatile unsigned int csh_clkout1_en:1;
			volatile unsigned int tslsh_dly_num:3;
			volatile unsigned int cs_dly_sel:2;
			volatile unsigned int reserved:22;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CS_TIMING_CFG_U;

	typedef union sfc_rd_sample_cfg_tag {
		struct sfc_rd_sample_cfg_map {
			volatile unsigned int t_sample_dly_r:2;
			volatile unsigned int t_sample_dly_w:2;
			volatile unsigned int sample_rst:1;
			volatile unsigned int bit_sample_vld_dlysel:3;
			volatile unsigned int byte_sample_vld_dlysel:3;
			volatile unsigned int reserved:21;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_RD_SAMPLE_CFG_U;

	typedef union sfc_clk_cfg_tag {
		struct sfc_clk_cfg_map {
			volatile unsigned int clk_div_mode:2;
			volatile unsigned int clk_polarity:1;
			volatile unsigned int clk_out_dly_inv:1;
			volatile unsigned int clk_out_dly_sel:4;
			volatile unsigned int clk_sample_dly_inv:1;
			volatile unsigned int clk_sample_dly_sel:4;
			volatile unsigned int clk_out_en_dly_sel:3;
			volatile unsigned int reserved:16;

		} mBits;
		volatile unsigned int dwValue;
	} SFC_CLK_CFG_U;

	typedef union sfc_cs_cfg_tag {
		struct sfc_cs_cfg_map {
			volatile unsigned int soft_cs:2;
			volatile unsigned int cs_position:3;
			volatile unsigned int reserved:27;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CS_CFG_U;

	typedef union sfc_endian_cfg_tag {
		struct sfc_endian_cfg_map {
			volatile unsigned int ahb_endian:2;
			volatile unsigned int spi_endian:1;
			volatile unsigned int reserved:29;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_ENDIAN_CFG_U;

	typedef union sfc_io_dly_cfg_tag {
		struct sfc_io_dly_cfg_map {
			volatile unsigned int data_oe_dly_sel:2;
			volatile unsigned int data_ie_dly_sel:2;
			volatile unsigned int data_ie_width_ctrl:1;
			volatile unsigned int do_dly_sel:2;
			volatile unsigned int reserved:25;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_IO_DLY_CFG_U;


	typedef union sfc_wp_hld_tag {
		struct sfc_wp_hld_map {
			volatile unsigned int wp_init:1;
			volatile unsigned int hld_init:1;
			volatile unsigned int reserved:30;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_WP_HLD_U;

	typedef union sfc_cmd_buf0_tag {
		struct sfc_cmd_buf0_map {
			volatile unsigned int cmd_buf0:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF0_U;

	typedef union sfc_cmd_buf1_tag {
		struct sfc_cmd_buf1_map {
			volatile unsigned int cmd_buf1:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF1_U;

	typedef union sfc_cmd_buf2_tag {
		struct sfc_cmd_buf2_map {
			volatile unsigned int cmd_buf2:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF2_U;

	typedef union sfc_cmd_buf3_tag {
		struct sfc_cmd_buf3_map {
			volatile unsigned int cmd_buf3:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF3_U;

	typedef union sfc_cmd_buf4_tag {
		struct sfc_cmd_buf4_map {
			volatile unsigned int cmd_buf4:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF4_U;


	typedef union sfc_cmd_buf5_tag {
		struct sfc_cmd_buf5_map {
			volatile unsigned int cmd_buf5:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF5_U;

	typedef union sfc_cmd_buf6_tag {
		struct sfc_cmd_buf6_map {
			volatile unsigned int cmd_buf6:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF6_U;

	typedef union sfc_cmd_buf7_tag {
		struct sfc_cmd_buf7_map {
			volatile unsigned int cmd_buf7:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF7_U;

	typedef union sfc_cmd_buf8_tag {
		struct sfc_cmd_buf8_map {
			volatile unsigned int cmd_buf8:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF8_U;

	typedef union sfc_cmd_buf9_tag {
		struct sfc_cmd_buf9_map {
			volatile unsigned int cmd_buf9:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF9_U;

	typedef union sfc_cmd_buf10_tag {
		struct sfc_cmd_buf10_map {
			volatile unsigned int cmd_buf10:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF10_U;

	typedef union sfc_cmd_buf11_tag {
		struct sfc_cmd_buf11_map {
			volatile unsigned int cmd_buf11:32;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_CMD_BUF11_U;

	typedef union sfc_type_buf0_tag {
		struct sfc_type_buf0_map {
			volatile unsigned int valid0:1;
			volatile unsigned int bit_mode0:2;
			volatile unsigned int byte_num0:2;
			volatile unsigned int operation_status0:2;
			volatile unsigned int byte_send_mode0:1;

			volatile unsigned int valid1:1;
			volatile unsigned int bit_mode1:2;
			volatile unsigned int byte_num1:2;
			volatile unsigned int operation_status1:2;
			volatile unsigned int byte_send_mode1:1;

			volatile unsigned int valid2:1;
			volatile unsigned int bit_mode2:2;
			volatile unsigned int byte_num2:2;
			volatile unsigned int operation_status2:2;
			volatile unsigned int byte_send_mode2:1;

			volatile unsigned int valid3:1;
			volatile unsigned int bit_mode3:2;
			volatile unsigned int byte_num3:2;
			volatile unsigned int operation_status3:2;
			volatile unsigned int byte_send_mode3:1;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_TYPE_BUF0_U;

	typedef union sfc_type_buf1_tag {
		struct sfc_type_buf1_map {
			volatile unsigned int valid4:1;
			volatile unsigned int bit_mode4:2;
			volatile unsigned int byte_num4:2;
			volatile unsigned int operation_status4:2;
			volatile unsigned int byte_send_mode4:1;

			volatile unsigned int valid5:1;
			volatile unsigned int bit_mode5:2;
			volatile unsigned int byte_num5:2;
			volatile unsigned int operation_status5:2;
			volatile unsigned int byte_send_mode5:1;

			volatile unsigned int valid6:1;
			volatile unsigned int bit_mode6:2;
			volatile unsigned int byte_num6:2;
			volatile unsigned int operation_status6:2;
			volatile unsigned int byte_send_mode6:1;

			volatile unsigned int valid7:1;
			volatile unsigned int bit_mode7:2;
			volatile unsigned int byte_num7:2;
			volatile unsigned int operation_status7:2;
			volatile unsigned int byte_send_mode7:1;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_TYPE_BUF1_U;

	typedef union sfc_type_buf2_tag {
		struct sfc_type_buf2_map {
			volatile unsigned int valid8:1;
			volatile unsigned int bit_mode8:2;
			volatile unsigned int byte_num8:2;
			volatile unsigned int operation_status8:2;
			volatile unsigned int byte_send_mode8:1;

			volatile unsigned int valid9:1;
			volatile unsigned int bit_mode9:2;
			volatile unsigned int byte_num9:2;
			volatile unsigned int operation_status9:2;
			volatile unsigned int byte_send_mode9:1;

			volatile unsigned int valid10:1;
			volatile unsigned int bit_mode10:2;
			volatile unsigned int byte_num10:2;
			volatile unsigned int operation_status10:2;
			volatile unsigned int byte_send_mode10:1;

			volatile unsigned int valid11:1;
			volatile unsigned int bit_mode11:2;
			volatile unsigned int byte_num11:2;
			volatile unsigned int operation_status11:2;
			volatile unsigned int byte_send_mode11:1;
		} mBits;
		volatile unsigned int dwValue;
	} SFC_TYPE_BUF2_U;

	typedef struct sfc_reg_tag {
		volatile SFC_CMD_CFG_U cmd_cfg;
		volatile SFC_SOFT_REQ_U soft_req;
		volatile SFC_TBUF_CLR_U tbuf_clr;
		volatile SFC_INT_CLR_U int_clr;
		volatile SFC_STATUS_U status;
		volatile SFC_CS_TIMING_CFG_U cs_timing_cfg;
		volatile SFC_RD_SAMPLE_CFG_U rd_timing_cfg;
		volatile SFC_CLK_CFG_U clk_cfg;
		volatile SFC_CS_CFG_U cs_cfg;
		volatile SFC_ENDIAN_CFG_U endian_cfg;
		volatile SFC_IO_DLY_CFG_U io_dly_cfg;
		volatile SFC_WP_HLD_U wp_hld_init;
		volatile unsigned int reserved1;
		volatile unsigned int reserved2;
		volatile unsigned int reserved3;
		volatile unsigned int reserved4;
		volatile SFC_CMD_BUF0_U cmd_buf0;
		volatile SFC_CMD_BUF1_U cmd_buf1;
		volatile SFC_CMD_BUF2_U cmd_buf2;
		volatile SFC_CMD_BUF3_U cmd_buf3;
		volatile SFC_CMD_BUF4_U cmd_buf4;
		volatile SFC_CMD_BUF5_U cmd_buf5;
		volatile SFC_CMD_BUF6_U cmd_buf6;
		volatile SFC_CMD_BUF7_U cmd_buf7;
		volatile SFC_CMD_BUF8_U cmd_buf8;
		volatile SFC_CMD_BUF9_U cmd_buf9;
		volatile SFC_CMD_BUF10_U cmd_buf10;
		volatile SFC_CMD_BUF11_U cmd_buf11;
		volatile SFC_TYPE_BUF0_U type_buf0;
		volatile SFC_TYPE_BUF1_U type_buf1;
		volatile SFC_TYPE_BUF2_U type_buf2;
	} SFC_REG_T;

#define SFC_REG_NUM         (sizeof(SFC_REG_T)/sizeof(u32_t))

#define SFC_CLK_OUT_DIV_1			(0x0)
#define SFC_CLK_OUT_DIV_2			BIT(0)
#define SFC_CLK_OUT_DIV_4			BIT(1)
#define SFC_CLK_SAMPLE_DELAY_SEL    BIT(2)
#define SFC_CLK_2X_EN				BIT(10)
#define SFC_CLK_OUT_2X_EN			BIT(9)
#define SFC_CLK_SAMPLE_2X_PHASE     BIT(8)
#define SFC_CLK_SAMPLE_2X_EN        BIT(7)

#define CMDBUF_INDEX_MAX 12

#undef SFC_DEBUG
#define SFC_DEBUG_BUF_LEN 48

#define SFC_IDLE_STATUS  BIT(1)
#define SFC_INT_STATUS      SFC_IDLE_STATUS

	typedef enum CMD_MODE_E_TAG {
		CMD_MODE_WRITE = 0,
		CMD_MODE_READ,
		CMD_MODE_MAX
	} CMD_MODE_E;

	typedef enum BIT_MODE_E_TAG {
		BIT_MODE_1 = 0,
		BIT_MODE_2,

		BIT_MODE_4,
		BIT_MODE_MAX
	} BIT_MODE_E;

	typedef enum INI_ADD_SEL_E_TAG {
		INI_CMD_BUF_7 = 0,
		INI_CMD_BUF_6,
		INI_CMD_BUF_5,
		INI_CMD_BUF_4,
		INI_CMD_BUF_MAX
	} INI_ADD_SEL_E;

	typedef enum CMD_BUF_INDEX_E_TAG {
		CMD_BUF_0 = 0,
		CMD_BUF_1,
		CMD_BUF_2,
		CMD_BUF_3,
		CMD_BUF_4,
		CMD_BUF_5,
		CMD_BUF_6,
		CMD_BUF_7,
		CMD_BUF_8,
		CMD_BUF_9,
		CMD_BUF_10,
		CMD_BUF_11,
		CMD_BUF_MAX
	} CMD_BUF_INDEX_E;

	typedef enum SEND_MODE_E_TAG {
		SEND_MODE_0 = 0,
		SEND_MODE_1,
		SEND_MODE_MAX
	} SEND_MODE_E;

	typedef enum BYTE_NUM_E_TAG {
		BYTE_NUM_1 = 0,
		BYTE_NUM_2,
		BYTE_NUM_3,
		BYTE_NUM_4,
		BYTE_NUM_MAX
	} BYTE_NUM_E;

#define SPI_CMD_DATA_BEGIN  0x01
#define SPI_CMD_DATA_END        0x02

	typedef struct _sfc_cmd_des {
		u32_t cmd;
		u32_t cmd_byte_len;
		u32_t is_valid;
		CMD_MODE_E cmd_mode;
		BIT_MODE_E bit_mode;
		SEND_MODE_E send_mode;
	} SFC_CMD_DES_T, *SFC_CMD_DES_PTR;

	void sfcdrv_setcmdencryptcfgreg(u32_t cmdmode);
	void sfcdrv_setcmdcfgreg(CMD_MODE_E cmdmode, BIT_MODE_E bitmode,
			INI_ADD_SEL_E iniAddSel);
	void sfcdrv_softreq(void);
	void sfcdrv_cmd_bufclr(void);
	void sfcdrv_typebufclr(void);
	void sfcdrv_intclr(void);
	u32_t sfcdrv_getstatus(void);
	void sfcdrv_cstimingcfg(u32_t value);
	void sfcdrv_rdtimingcfg(u32_t value);
	void sfcdrv_clkcfg(u32_t value);
	void sfcdrv_cscfg(u32_t value);
	void sfcdrv_endiancfg(u32_t value);
	void sfcdrv_setcmdbuf(CMD_BUF_INDEX_E index, u32_t value);
	void sfcdrv_setcmdbufex(CMD_BUF_INDEX_E index,
		const u8_t *buf, u32_t count);
	u32_t sfcdrv_getcmdbuf(CMD_BUF_INDEX_E index);
	__ramfunc void sfcdrv_settypeinfbuf(CMD_BUF_INDEX_E index,
			BIT_MODE_E bitmode,
			BYTE_NUM_E bytenum, CMD_MODE_E cmdmode,
			SEND_MODE_E sendmode);
	__ramfunc void sfcdrv_resetallbuf(void);
	__ramfunc void sfcdrv_setreadbuf(SFC_CMD_DES_T *cmd_des_ptr,
			u32_t cmd_flag);
	__ramfunc void sfcdrv_setcmddata(SFC_CMD_DES_T *cmd_des_ptr,
			u32_t cmd_flag);
	__ramfunc void sfcdrv_getreadbuf(u32_t *buffer, u32_t word_cnt);
	void sfcdrv_getbuf(void *buffer, u32_t nbytes);
	__ramfunc void sfcdrv_intcfg(u32_t op);
	__ramfunc void sfcdrv_clkcfg(u32_t value);
	__ramfunc void sfcdrv_req(void);

#ifdef __cplusplus
}
#endif

#endif
