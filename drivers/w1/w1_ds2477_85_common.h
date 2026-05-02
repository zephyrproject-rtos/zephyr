/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_W1_DS2477_DS2485_COMMON_H_
#define ZEPHYR_DRIVERS_W1_DS2477_DS2485_COMMON_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/kernel.h>

/* memory specific commands */
#define CMD_WR_MEM		 0x96
#define CMD_RD_MEM		 0x44
#define CMD_SET_PAGE_PROTECT	 0xc3
#define CMD_RD_STATUS		 0xaa
/* configuration specific commands */
#define CMD_SET_I2C_ADDR	 0x75
#define CMD_RD_W1_PORT_CFG	 0x52
#define CMD_WR_W1_PORT_CFG	 0x99
#define CMD_MASTER_RESET	 0x62
/* 1-Wire specific commands */
#define CMD_W1_SCRIPT		 0x88
#define CMD_W1_BLOCK		 0xab
#define CMD_RD_BLOCK		 0x50
#define CMD_WR_BLOCK		 0x68
#define CMD_SEARCH		 0x11
#define CMD_FULL_CMD_SEQ	 0x57
/* crc16 specific commands */
#define CMD_COMPUTE_CRC		 0xcc

/* i2c command overhead len */
#define CMD_OVERHEAD_LEN         2U
/* memory specific commands' data length */
#define CMD_WR_MEM_LEN		 33U
#define CMD_RD_MEM_LEN		 1U
#define CMD_SET_PAGE_PROTECT_LEN 2U
#define CMD_RD_STATUS_LEN	 1U
/* configuration specific commands */
#define CMD_SET_I2C_ADDR_LEN	 1U
#define CMD_RD_W1_PORT_CFG_LEN	 1U
#define CMD_WR_W1_PORT_CFG_LEN	 3U
/* 1-Wire specific commands */
#define CMD_W1_SCRIPT_LEN	 1U
#define CMD_W1_BLOCK_LEN	 1U
#define CMD_RD_BLOCK_LEN	 1U
#define CMD_WR_BLOCK_LEN	 1U
#define CMD_SEARCH_LEN		 2U
#define CMD_FULL_CMD_SEQ_LEN	 9U
/* crc16 specific commands */
#define CMD_COMPUTE_CRC_LEN	 1U

/* I2C communication result bytes */
#define DS2477_88_RES_SUCCESS	    0xaa
#define DS2477_88_RES_INVALID_PARAM 0x77
#define DS2477_88_RES_COMM_FAILURE  0x22
#define DS2477_88_RES_RESET_FAILURE 0x22
#define DS2477_88_RES_NO_PRESENCE   0x33
#define DS2477_88_RES_WP_FAILURE    0x55

/* primitive commands, executable via the script command */
#define SCRIPT_OW_RESET	      0x00
#define SCRIPT_OW_WRITE_BIT   0x01
#define SCRIPT_OW_READ_BIT    0x02
#define SCRIPT_OW_WRITE_BYTE  0x03
#define SCRIPT_OW_READ_BYTE   0x04
#define SCRIPT_OW_TRIPLET     0x05
#define SCRIPT_OW_OV_SKIP     0x06
#define SCRIPT_OW_SKIP	      0x07
#define SCRIPT_OW_READ_BLOCK  0x08
#define SCRIPT_OW_WRITE_BLOCK 0x09
#define SCRIPT_OW_DELAY	      0x0a
#define SCRIPT_OW_PRIME_SPU   0x0b
#define SCRIPT_OW_SPU_OFF     0x0c
#define SCRIPT_OW_SPEED	      0x0d

/* port configuration register offsets */
#define PORT_REG_MASTER_CONFIGURATION	0x00
#define PORT_REG_STANDARD_SPEED_T_RSTL	0x01
#define PORT_REG_STANDARD_SPEED_T_MSI	0x02
#define PORT_REG_STANDARD_SPEED_T_MSP	0x03
#define PORT_REG_STANDARD_SPEED_T_RSTH	0x04
#define PORT_REG_STANDARD_SPEED_T_W0L	0x05
#define PORT_REG_STANDARD_SPEED_T_W1L	0x06
#define PORT_REG_STANDARD_SPEED_T_MSR	0x07
#define PORT_REG_STANDARD_SPEED_T_REC	0x08
#define PORT_REG_OVERDRIVE_SPEED_T_RSTL 0x09
#define PORT_REG_OVERDRIVE_SPEED_T_MSI	0x0a
#define PORT_REG_OVERDRIVE_SPEED_T_MSP	0x0b
#define PORT_REG_OVERDRIVE_SPEED_T_RSTH 0x0c
#define PORT_REG_OVERDRIVE_SPEED_T_W0L	0x0d
#define PORT_REG_OVERDRIVE_SPEED_T_W1L	0x0e
#define PORT_REG_OVERDRIVE_SPEED_T_MSR	0x0f
#define PORT_REG_OVERDRIVE_SPEED_T_REC	0x10
#define PORT_REG_RPUP_BUF		0x11
#define PORT_REG_PDSLEW			0x12
#define PORT_REG_COUNT			0x13

/* upper limit of 1-wire command length supported(in bytes) */
#define MAX_BLOCK_LEN	 126U
/* limit of 1-wire command len is 126 bytes, but currently not used: */
#define SCRIPT_WR_LEN 1U

/* variant independent timing */
#define DS2477_85_T_RM_us 50000U
#define DS2477_85_T_WM_us 100000U
#define DS2477_85_T_WS_us 15000U

/* default 1-wire timing parameters (cfg. value==6) */
#define DS2477_85_STD_SPD_T_RSTL_us 560U
#define DS2477_85_STD_SPD_T_MSI_us  7U
#define DS2477_85_STD_SPD_T_MSP_us  68U
#define DS2477_85_STD_SPD_T_RSTH_us 560U
#define DS2477_85_STD_SPD_T_W0L_us  68U
#define DS2477_85_STD_SPD_T_W1L_us  8U
#define DS2477_85_STD_SPD_T_MSR_us  12U
#define DS2477_85_STD_SPD_T_REC_us  6U

#define DS2477_85_OVD_SPD_T_RSTL_us 56U
#define DS2477_85_OVD_SPD_T_MSI_us  2U
#define DS2477_85_OVD_SPD_T_MSP_us  8U
#define DS2477_85_OVD_SPD_T_RSTH_us 56U
#define DS2477_85_OVD_SPD_T_W0L_us  8U
#define DS2477_85_OVD_SPD_T_W1L_us  1U
#define DS2477_85_OVD_SPD_T_MSR_us  2U
#define DS2477_85_OVD_SPD_T_REC_us  6U

#define DS2477_85_STD_SPD_T_SLOT_us                                            \
	(DS2477_85_STD_SPD_T_W0L_us + DS2477_85_STD_SPD_T_REC_us)
#define DS2477_85_STD_SPD_T_RESET_us                                           \
	(DS2477_85_STD_SPD_T_RSTL_us + DS2477_85_STD_SPD_T_RSTH_us)

#define DS2477_85_OVD_SPD_T_SLOT_us                                            \
	(DS2477_85_OVD_SPD_T_W0L_us + DS2477_85_OVD_SPD_T_REC_us)
#define DS2477_85_OVD_SPD_T_RESET_us                                           \
	(DS2477_85_OVD_SPD_T_RSTL_us + DS2477_85_OVD_SPD_T_RSTH_us)

/* defines for DTS switching-th, active-pull-th and weak-pullup enums */
#define RPUP_BUF_CUSTOM BIT(15)

#define RPUP_BUF_SW_TH_Msk	  GENMASK(5, 4)
#define RPUP_BUF_SW_TH_PREP(x)	  FIELD_PREP(RPUP_BUF_SW_TH_Msk, x)
#define RPUP_BUF_SW_TH_LOW	  RPUP_BUF_SW_TH_PREP(0)
#define RPUP_BUF_SW_TH_MEDIUM	  RPUP_BUF_SW_TH_PREP(1)
#define RPUP_BUF_SW_TH_HIGH	  RPUP_BUF_SW_TH_PREP(2)
#define RPUP_BUF_SW_TH_OFF	  RPUP_BUF_SW_TH_PREP(3)
#define RPUP_BUF_APULL_TH_Msk	  GENMASK(3, 2)
#define RPUP_BUF_APULL_TH_PREP(x) FIELD_PREP(RPUP_BUF_APULL_TH_Msk, x)
#define RPUP_BUF_APULL_TH_LOW	  RPUP_BUF_APULL_TH_PREP(0)
#define RPUP_BUF_APULL_TH_MEDIUM  RPUP_BUF_APULL_TH_PREP(1)
#define RPUP_BUF_APULL_TH_HIGH	  RPUP_BUF_APULL_TH_PREP(2)
#define RPUP_BUF_APULL_TH_OFF	  RPUP_BUF_APULL_TH_PREP(3)
#define RPUP_BUF_WPULL_Msk	  GENMASK(1, 0)
#define RPUP_BUF_WPULL_PREP(x)	  FIELD_PREP(RPUP_BUF_WPULL_Msk, x)
#define RPUP_BUF_WPULL_EXTERN	  RPUP_BUF_WPULL_PREP(0)
#define RPUP_BUF_WPULL_500	  RPUP_BUF_WPULL_PREP(1)
#define RPUP_BUF_WPULL_1000	  RPUP_BUF_WPULL_PREP(2)
#define RPUP_BUF_WPULL_333	  RPUP_BUF_WPULL_PREP(3)

/* defines for standard and overdrive slew enums */
#define PDSLEW_CUSTOM	   BIT(15)
#define PDSLEW_STD_Msk	   GENMASK(5, 3)
#define PDSLEW_STD_PREP(x) FIELD_PREP(PDSLEW_STD_Msk, BIT(x))
#define PDSLEW_STD_50	   PDSLEW_STD_PREP(0)
#define PDSLEW_STD_150	   PDSLEW_STD_PREP(1)
#define PDSLEW_STD_1300	   PDSLEW_STD_PREP(2)
#define PDSLEW_OVD_Msk	   GENMASK(2, 0)
#define PDSLEW_OVD_PREP(x) FIELD_PREP(PDSLEW_OVD_Msk, BIT(x))
#define PDSLEW_OVD_50	   PDSLEW_OVD_PREP(0)
#define PDSLEW_OVD_150	   PDSLEW_OVD_PREP(1)

/* speed mode dependent timing parameters */
struct mode_timing {
	uint16_t t_slot;
	uint16_t t_reset;
};

union master_config_reg {
	struct {
		uint16_t res : 12;
		uint16_t apu : 1;
		uint16_t spu : 1;
		uint16_t pdn : 1;
		uint16_t od_active : 1;
	};
	uint16_t value;
};

typedef int (*variant_w1_script_cmd_fn)(const struct device *dev,
					int w1_delay_us, uint8_t w1_cmd,
					const uint8_t *tx_buf,
					const uint8_t tx_len, uint8_t *rx_buf,
					uint8_t rx_len);

struct w1_ds2477_85_config {
	/** w1 master config, common to all drivers */
	struct w1_master_config master_config;
	/** I2C device */
	const struct i2c_dt_spec i2c_spec;
	/** config reg of weak pullup, active pullup, and switch threshold */
	uint16_t rpup_buf;
	/** config reg of standard and overdrive slew */
	uint16_t pdslew;
	/** mode dependent timing parameters (@0: standard, @1: overdrive) */
	struct mode_timing mode_timing[2];
	/** variant dependent time of 1 operation in us */
	uint16_t t_op_us;
	/** variant dependent time of 1 sequence in us */
	uint16_t t_seq_us;
	/** variant specific script command */
	variant_w1_script_cmd_fn w1_script_cmd;
	/** indicates enable active pull-up configuration */
	bool apu;
};

struct w1_ds2477_85_data {
	/** w1 master data, common to all drivers */
	struct w1_master_data master_data;
	/** master specific runtime configuration */
	union master_config_reg master_reg;
};

#define W1_DS2477_85_DT_CONFIG_GET(node_id, _t_op, _t_seq, _script_cmd)        \
	{                                                                      \
		.i2c_spec = I2C_DT_SPEC_GET(node_id),                          \
		.master_config.slave_count =                                   \
			W1_SLAVE_COUNT(node_id),                               \
		.rpup_buf = RPUP_BUF_CUSTOM |                                  \
			RPUP_BUF_SW_TH_PREP(DT_ENUM_IDX(node_id,               \
						       switching_threshold)) | \
			RPUP_BUF_APULL_TH_PREP(DT_ENUM_IDX(node_id,            \
						     active_pull_threshold)) | \
			RPUP_BUF_WPULL_PREP(DT_ENUM_IDX(node_id, weak_pullup)),\
		.pdslew = PDSLEW_CUSTOM |                                      \
			  PDSLEW_STD_PREP(DT_ENUM_IDX(node_id,                 \
						      standard_slew)) |        \
			  PDSLEW_OVD_PREP(DT_ENUM_IDX(node_id,                 \
						      overdrive_slew)),        \
		.apu = DT_PROP(node_id, active_pullup),                        \
		.mode_timing = {                                               \
			{                                                      \
				.t_slot = DS2477_85_STD_SPD_T_SLOT_us,         \
				.t_reset = DS2477_85_STD_SPD_T_RESET_us,       \
			},                                                     \
			{                                                      \
				.t_slot = DS2477_85_OVD_SPD_T_SLOT_us,         \
				.t_reset = DS2477_85_OVD_SPD_T_RESET_us,       \
			},                                                     \
		},                                                             \
		.t_op_us = _t_op,                                              \
		.t_seq_us = _t_seq,                                            \
		.w1_script_cmd = _script_cmd,                                  \
	}

#define W1_DS2477_85_DT_CONFIG_INST_GET(inst, _t_op, _t_seq, _script_cmd)      \
	W1_DS2477_85_DT_CONFIG_GET(DT_DRV_INST(inst), _t_op, _t_seq,           \
				   _script_cmd)

int w1_ds2477_85_init(const struct device *dev);
int ds2477_85_write_port_config(const struct device *dev, uint8_t reg,
				uint16_t value);
int ds2477_85_read_port_config(const struct device *dev, uint8_t reg,
			       uint16_t *value);
int ds2477_85_reset_master(const struct device *dev);
int ds2477_85_reset_bus(const struct device *dev);
int ds2477_85_read_bit(const struct device *dev);
int ds2477_85_write_bit(const struct device *dev, const bool bit);
int ds2477_85_read_byte(const struct device *dev);
int ds2477_85_write_byte(const struct device *dev, const uint8_t tx_byte);
int ds2477_85_write_block(const struct device *dev, const uint8_t *buffer,
			  size_t tx_len);
int ds2477_85_read_block(const struct device *dev, uint8_t *buffer,
			 size_t rx_len);
int ds2477_85_configure(const struct device *dev, enum w1_settings_type type,
			uint32_t value);

#endif /* ZEPHYR_DRIVERS_W1_DS2477_DS2485_COMMON_H_ */
