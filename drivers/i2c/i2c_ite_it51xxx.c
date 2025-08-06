/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_i2c

#include <errno.h>
#include <soc.h>
#include <soc_dt.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/i2c/it51xxx-i2c.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2c_ite_it51xxx, CONFIG_I2C_LOG_LEVEL);
#include "i2c_bitbang.h"
#include "i2c-priv.h"

/*
 * it51xxx SMBus host registers definition
 * base1(0xf04100): A, B, C, D, E, F; base2(0xf04200): G, H, I
 */
/* Host Status Register: base1: 0x00. 0x28, 0x50, 0x78, 0xa0, 0xc8
 *                       base2: 0x60, 0x88, 0xb0
 */
#define SMB_HOSTA       0x00
#define SMB_BDS         BIT(7)
#define SMB_TMOE        BIT(6)
#define SMB_NACK        BIT(5)
#define SMB_FAIL        BIT(4)
#define SMB_BSER        BIT(3)
#define SMB_DVER        BIT(2)
#define SMB_FINTR       BIT(1)
#define SMB_HOBY        BIT(0)
/* Host Control Register: base1: 0x01. 0x29, 0x51, 0x79, 0xa1, 0xc9
 *                        base2: 0x61, 0x89, 0xb1
 */
#define SMB_HOCTL       0x01
#define SMB_PEC_EN      BIT(7)
#define SMB_SRT         BIT(6)
#define SMB_LABY        BIT(5)
#define SMB_SMCD(n)     FIELD_PREP(GENMASK(4, 2), n)
#define SMB_KILL        BIT(1)
#define SMB_INTREN      BIT(0)
/* Transmit Slave Address Register: base1: 0x03, 0x2b, 0x53, 0x7b, 0xa3, 0xcb
 *                                  base2: 0x63, 0x8b, 0xb3
 */
#define SMB_TRASLA      0x03
#define SMB_DIR         BIT(0)
/* Data 0 Register: base1: 0x04, 0x2c, 0x54, 0x7c, 0xa4, 0xcc
 *                  base2: 0x64, 0x8c, 0xb4
 */
#define SMB_D0REG       0x04
/* I2C Shared FIFO Byte Count H: base1: 0x05, 0x2d, 0x55, 0x7d, 0xa5, 0xcd
 *                               base2: 0x65, 0x8d, 0xb5
 */
#define SMB_ISFBCH      0x05
/* Host Block Data Byte Register: base1: 0x07, 0x2f, 0x57, 0x7f, 0xa7, 0xcf
 *                                base2: 0x67, 0x8f, 0xb7
 */
#define SMB_HOBDB       0x07
/* SMBus Pin Control Register: base1: 0x09, 0x31, 0x59, 0x81, 0xa9, 0xd1
 *                             base2: 0x69, 0x91, 0xb9
 */
#define SMB_SMBPCTL     0x09
#define SMB_DASTI(n)    FIELD_PREP(GENMASK(7, 4), n)
#define SMB_HSMBDCS     BIT(1)
#define SMB_HSMBCS      BIT(0)
/* Host Nack Source: base1: 0x0a, 0x32, 0x5a, 0x82, 0xaa, 0xd2
 *                   base2: 0x6a, 0x92, 0xba
 */
#define SMB_HONACKSRC   0x0a
#define SMB_HSMCDTD     BIT(4)
/* Host Control 2: base1: 0x0b, 0x33, 0x5b, 0x83, 0xab, 0xd3
 *                 base2: 0x6b, 0x93, 0xbb
 */
#define SMB_HOCTL2      0x0b
#define SMB_HTIFYEN     BIT(6)
#define SMB_SMD_TO_EN   BIT(4)
#define I2C_SW_EN       BIT(3)
#define I2C_SW_WAIT     BIT(2)
#define I2C_EN          BIT(1)
#define SMB_SMH_EN      BIT(0)
/* SMCLK Timing Setting Register: base1: 0x0c, 0x34, 0x5c, 0x84, 0xac, 0xd4
 *                                base2: 0x6c, 0x94, 0xbc
 */
#define SMB_MSCLKTS     0x0c
/* BIT[1:0]: SMCLK Setting */
#define SMB_CLKS_1M     4
#define SMB_CLKS_400K   3
#define SMB_CLKS_100K   2
#define SMB_CLKS_50K    1
/* 4.7us Low Register: base1: 0x0d, 0x35, 0x5d, 0x85, 0xad, 0xd5
 *                     base2: 0x6d, 0x95, 0xbd
 */
#define SMB_4P7USL      0x0d
/* 4.0us Low Register: base1: 0x0e, 0x36, 0x5e, 0x86, 0xae, 0xd6
 *                     base2: 0x6e, 0x96, 0xbe
 */
#define SMB_4P0USL      0x0e
/* 250ns Register: base1: 0x10, 0x38, 0x60, 0x88, 0xb0, 0xd8
 *                 base2: 0x70, 0x98, 0xc0
 */
#define SMB_250NSREG    0x10
/* 25ms Register: base1: 0x11, 0x39, 0x61, 0x89, 0xb1, 0xd9
 *                base2: 0x71, 0x99, 0xc1
 */
#define SMB_25MSREG     0x11
/* 45.3us Low Register: base1: 0x12, 0x3a, 0x62, 0x8a, 0xb2, 0xda
 *                      base2: 0x72, 0x9a, 0xc2
 */
#define SMB_45P3USLREG  0x12
/* 45.3us High Register: base1: 0x13, 0x3b, 0x63, 0x8b, 0xb3, 0xdb
 *                       base2: 0x73, 0x9b, 0xc3
 */
#define SMB_45P3USHREG  0x13
/* 4.7us And 4.0us High Register: base1: 0x14, 0x3c, 0x64, 0x8c, 0xb4, 0xdc
 *                                base2: 0x74, 0x9c, 0xc4
 */
#define SMB_4P7A4P0H    0x14
/* I2C Wr to Rd FIFO Register: base1: 0x1b, 0x43, 0x6b, 0x93, 0xbb, 0xe3
 *                             base2: 0x7b, 0xa3, 0xcb
 */
#define SMB_I2CW2RF     0x1b
#define SMB_MAIFID      BIT(2)
#define SMB_MAIF        BIT(1)
#define SMB_MAIFI       BIT(0)
/* 0x16: Shared FIFO Base Address MSB for Master A */
#define SMB_SFBAMMA     0x16
/* 0x17: Shared FIFO Base Address for Master A */
#define SMB_SFBAMA      0x17
/* 0x18: Shared FIFO Ctrl for Master A */
#define SMB_SFCMA       0x18
#define SMB_SFSAE       BIT(3)
#define SMB_SFSFSA(n)   FIELD_PREP(GENMASK(2, 0), n)
/* Shared FIFO Base Address MSB for Master n: base1: 0x3e, 0x66, 0x8e, 0xb6, 0xde
 *                                            base2: 0x76, 0x9e, 0xc6
 */
#define SMB_SFBAMMn     0x3e
/* Shared FIFO Base Address LSB for Master n: base1: 0x3f, 0x67, 0x8f, 0xb7, 0xdf
 *                                            base2: 0x77, 0x9f, 0xc7
 */
#define SMB_SFBAMn      0x3f
/* Master n Shared FIFO Size Select: base1: 0x40, 0x68, 0x90, 0xb8, 0xe0
 *                                   base2: 0x78, 0xa0, 0xc8
 */
#define SMB_MnSFSS      0x40
/* 0xf0: Master FIFO Control Status Register */
#define SMB_MSTFCSTS    0xf0
#define SMB_BLKDS2      BIT(6)
#define SMB_SFDFSF      BIT(6)
#define SMB_FF2EN       BIT(5)
#define SMB_BLKDS1      BIT(4)
#define SMB_SFDFSFA     BIT(4)
#define SMB_FF1EN       BIT(3)
#define SMB_FFCHSEL2(n) FIELD_PREP(GENMASK(2, 0), n)
/* 0xf1: Master FIFO Status 1 Register */
#define SMB_MSTFSTS1    0xf1
#define SMB_FIFO1_EMPTY BIT(7)
#define SMB_FIFO1_FULL  BIT(6)
/* 0xf2: Master FIFO Status 2 Register */
#define SMB_MSTFSTS2    0xf2
#define SMB_FIFO2_EMPTY BIT(7)
#define SMB_FIFO2_FULL  BIT(6)
/* 0xf4: SMBus Interface Switch Pin Control 0 */
#define SMB_SISPC0      0xf4
/* 0xf5: SMBus Interface Switch Pin Control 1 */
#define SMB_SISPC1      0xf5

/*
 * it51xxx SMBus target registers definition
 * base(0xf04200): A, B, C
 */
/* 0x00, 0x20, 0x40: Receive Slave Address Register */
#define SMB_RESLADR   0x00
/* 0x01, 0x21, 0x41: Slave Data Register n */
#define SMB_SLDn      0x01
/* 0x02, 0x22, 0x42: Slave Status Register n */
#define SMB_SLSTn     0x02
#define SMB_SPDS      BIT(5)
#define SMB_MSLA2     BIT(4)
enum it51xxx_msla2 {
	SMB_SADR,
	SMB_SADR2,
	MAX_I2C_TARGET_ADDRS,
};
#define SMB_RCS       BIT(3)
#define SMB_STS       BIT(2)
#define SMB_SDS       BIT(1)
/* 0x03, 0x23, 0x43: Slave Interrupt Control Register n */
#define SMB_SICRn     0x03
#define SMB_SDSEN     BIT(3)
#define SMB_SDLTOEN   BIT(2)
#define SMB_SITEN     BIT(1)
/* 0x05, 0x25, 0x45: Slave Control Register n */
#define SMB_SLVCTLn   0x05
#define SMB_RSCS      BIT(2)
#define SMB_SSCL      BIT(1)
#define SMB_SLVEN     BIT(0)
/* 0x06, 0x26, 0x45: SMCLK Timing Setting Register n */
#define SMB_SSCLKTSn  0x06
#define SMB_DSASTI(n) FIELD_PREP(GENMASK(5, 2), n)
#define SMB_SCLKSA1M  BIT(1)
#define SMB_SSMCDTD   BIT(0)
/* 0x07, 0x27, 0x47: 25 ms Slave Register */
#define SMB_25SLVREGn 0x07
/* 0x08, 0x28, 0x48: Receive Slave Address Register */
#define SMB_RESLADR2n 0x08
#define SMB_SADR2_EN  BIT(7)
/* 0x0a, 0x2a, 0x4a: Slave n Dedicated FIFO Pre-defined Control */
#define SMB_SnDFPCTL  0x0a
#define SMB_SADFE     BIT(0)
/* 0x0b, 0x2b, 0x4b: Slave n Dedicated FIFO status */
#define SMB_SFFSTn    0x0b
#define SMB_FIFO_FULL BIT(6)
/* 0x0e, 0x2e, 0x4e: Shared FIFO Base Address MSB for Slave n */
#define SMB_SFBAMSn   0x0e
/* 0x0f, 0x2f, 0x4f: Shared FIFO Base Address LSB for Slave n */
#define SMB_SFBASn    0x0f
/* 0x11, 0x31, 0x51: Slave Shared FIFO Ctrl n */
#define SMB_SSFIFOCn  0x11

/*
 * TODO: Some registers are not correctly mapped to the new baseaddress: 0xf04100, so the old
 *       baseaddress must be used to avoid invalid functionality.
 */
#ifdef CONFIG_SOC_IT51526AW
/* 0x09, 0x1a, 0x5b: Slave Data */
#define SMB_SLDA(ch)  ((ch) == 0 ? 0x09 : (ch) == 1 ? 0x1a : (ch) == 2 ? 0x5b : 0)
/* 0x0b, 0x1c, 0x52: Slave Status */
#define SMB_SLSTA(ch) ((ch) == 0 ? 0x0b : (ch) == 1 ? 0x1c : (ch) == 2 ? 0x52 : 0)
/* 0x45: Master FIFO Control 1 */
#define SMB_MSTFCTRL1 0x45
/* 0x47: Master FIFO Control 2 */
#define SMB_MSTFCTRL2 0x47
#define SMB_BLKDS     BIT(4)
#define SMB_FFEN      BIT(3)

#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
BUILD_ASSERT((DT_PROP(DT_NODELABEL(i2c6), fifo_enable) == false) &&
		     (DT_PROP(DT_NODELABEL(i2c7), fifo_enable) == false) &&
		     (DT_PROP(DT_NODELABEL(i2c8), fifo_enable) == false),
	     "I2C6, I2C7, I2C8 cannot use FIFO mode in it51526aw soc.");
#endif /* CONFIG_I2C_IT51XXX_FIFO_MODE */
#endif /* CONFIG_SOC_IT51526AW */

#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
#define SMB_FIFO_MODE_MAX_SIZE  32
#define SMB_FIFO_MODE_TOTAL_LEN 255
#define SMB_MSG_BURST_READ_MASK (I2C_MSG_RESTART | I2C_MSG_STOP | I2C_MSG_READ)
#define FIFO_ENABLE_NODE(idx)   DT_PROP(DT_NODELABEL(i2c##idx), fifo_enable)
#define FIFO_ENABLE_COUNT                                                                          \
	(FIFO_ENABLE_NODE(1) + FIFO_ENABLE_NODE(2) + FIFO_ENABLE_NODE(3) + FIFO_ENABLE_NODE(4) +   \
	 FIFO_ENABLE_NODE(5) + FIFO_ENABLE_NODE(6) + FIFO_ENABLE_NODE(7) + FIFO_ENABLE_NODE(8))
BUILD_ASSERT(FIFO_ENABLE_COUNT <= 1, "More than one node has fifo2-enable property enabled!");
#endif /* CONFIG_I2C_IT51XXX_FIFO_MODE */

#ifdef CONFIG_I2C_TARGET
#define SMB_TARGET_IT51XXX_MAX_FIFO_SIZE 16

struct target_shared_fifo_size_sel {
	uint16_t fifo_size;
	uint8_t value;
};

static const struct target_shared_fifo_size_sel fifo_size_table[] = {
	[0] = {16, 0x1}, [1] = {32, 0x2}, [2] = {64, 0x3}, [3] = {128, 0x4}, [4] = {256, 0x5},
};
#endif /* CONFIG_I2C_TARGET */

/* Start smbus session from idle state */
#define SMB_MSG_START     BIT(5)
#define SMB_LINE_SCL_HIGH BIT(0)
#define SMB_LINE_SDA_HIGH BIT(1)
#define SMB_LINE_IDLE     (SMB_LINE_SCL_HIGH | SMB_LINE_SDA_HIGH)

struct i2c_it51xxx_config {
	/* I2C alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	/* SCL GPIO cells */
	struct gpio_dt_spec scl_gpios;
	/* SDA GPIO cells */
	struct gpio_dt_spec sda_gpios;
	int transfer_timeout_ms;
	mm_reg_t host_base;
	mm_reg_t target_base;
	mm_reg_t i2cbase;
	mm_reg_t i2cbase_mapping;
	uint32_t bitrate;
	uint8_t i2c_irq_base;
	uint8_t i2cs_irq_base;
	uint8_t port;
	uint8_t channel_switch_sel;
	bool fifo_enable;
	bool target_enable;
	bool target_fifo_mode;
	bool target_shared_fifo_mode;
	bool push_pull_recovery;
};

enum i2c_ch_status {
	I2C_CH_NORMAL = 0,
	I2C_CH_REPEAT_START,
	I2C_CH_WAIT_READ,
	I2C_CH_WAIT_NEXT_XFER,
};

enum i2c_ite_pm_policy_state_flag {
	I2CM_ITE_PM_POLICY_FLAG,
	I2CS_ITE_PM_POLICY_FLAG,
	I2C_ITE_PM_POLICY_FLAG_COUNT,
};

struct i2c_it51xxx_data {
	struct i2c_msg *msg;
	struct k_mutex mutex;
	struct k_sem device_sync_sem;
	struct i2c_bitbang bitbang;
	enum i2c_ch_status i2ccs;
	/* Index into output data */
	size_t widx;
	/* Index into input data */
	size_t ridx;
	/* operation freq of i2c */
	uint32_t bus_freq;
	/* Error code, if any */
	uint32_t err;
	/* Address of device */
	uint16_t addr_16bit;
	/* Wait for stop bit interrupt */
	uint8_t stop;
#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
	struct i2c_msg *msgs_list;
	/* Read or write byte counts. */
	uint32_t bytecnt;
	/* Number of messages. */
	uint8_t num_msgs;
	uint8_t msg_index;
#endif
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_cfg[MAX_I2C_TARGET_ADDRS];
	const struct target_shared_fifo_size_sel *fifo_size_list;
	atomic_t num_registered_addrs;
	uint32_t w_index;
	uint32_t r_index;
	/* Target mode FIFO buffer. */
	uint8_t __aligned(4) target_in_buffer[CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE];
	uint8_t __aligned(4) target_out_buffer[CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE];
	/* Target shared FIFO mode. */
	uint8_t __aligned(16) target_shared_fifo[CONFIG_I2C_IT51XXX_MAX_SHARE_FIFO_SIZE];
	uint8_t registered_addrs[MAX_I2C_TARGET_ADDRS];
#endif
#ifdef CONFIG_PM
	ATOMIC_DEFINE(pm_policy_state_flag, I2C_ITE_PM_POLICY_FLAG_COUNT);
#endif
};

enum i2c_host_status {
	/* Host busy */
	HOSTA_HOBY = 0x01,
	/* Finish Interrupt */
	HOSTA_FINTR = 0x02,
	/* Device error */
	HOSTA_DVER = 0x04,
	/* Bus error */
	HOSTA_BSER = 0x08,
	/* Fail */
	HOSTA_FAIL = 0x10,
	/* Not response ACK */
	HOSTA_NACK = 0x20,
	/* Time-out error */
	HOSTA_TMOE = 0x40,
	/* Byte done status */
	HOSTA_BDS = 0x80,
	/* Error bit is set */
	HOSTA_ANY_ERROR = (HOSTA_DVER | HOSTA_BSER | HOSTA_FAIL | HOSTA_NACK | HOSTA_TMOE),
	/* W/C for next byte */
	HOSTA_NEXT_BYTE = HOSTA_BDS,
	/* W/C host status register */
	HOSTA_ALL_WC_BIT = (HOSTA_FINTR | HOSTA_ANY_ERROR | HOSTA_BDS),
};

enum i2c_reset_cause {
	I2C_RC_NO_IDLE_FOR_START = 1,
	I2C_RC_TIMEOUT,
};

#ifdef CONFIG_PM
static void i2c_ite_pm_policy_state_lock_get(struct i2c_it51xxx_data *data,
					     enum i2c_ite_pm_policy_state_flag flag)
{
	if (atomic_test_and_set_bit(data->pm_policy_state_flag, flag) == 0) {
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
}

static void i2c_ite_pm_policy_state_lock_put(struct i2c_it51xxx_data *data,
					     enum i2c_ite_pm_policy_state_flag flag)
{
	if (atomic_test_and_clear_bit(data->pm_policy_state_flag, flag) == 1) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	}
}
#endif /* CONFIG_PM */

#ifdef CONFIG_I2C_TARGET
static void target_i2c_isr_fifo(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	struct i2c_target_config *target_cfg;
	const struct i2c_target_callbacks *target_cb;
	uint32_t count, len;
	uint8_t sdfpctl;
	uint8_t target_status, fifo_status, target_idx;

#ifdef CONFIG_SOC_IT51526AW
	target_status = sys_read8(config->i2cbase_mapping + SMB_SLSTA(config->port));
#else
	target_status = sys_read8(config->target_base + SMB_SLSTn);
#endif
	fifo_status = sys_read8(config->target_base + SMB_SFFSTn);
	/* bit0-4 : FIFO byte count */
	count = fifo_status & GENMASK(4, 0);

	/* Any error */
	if (target_status & SMB_STS) {
		data->w_index = 0;
		data->r_index = 0;
		goto done;
	}

	/* Which target address to match. */
	target_idx = (target_status & SMB_MSLA2) ? SMB_SADR2 : SMB_SADR;
	target_cfg = data->target_cfg[target_idx];
	target_cb = target_cfg->callbacks;

	/* Target data status, the register is waiting for read or write. */
	if (target_status & SMB_SDS) {
		if (target_status & SMB_RCS) {
			uint8_t *rdata = NULL;

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
			/* Read data callback function */
			if (target_cb->buf_read_requested) {
				target_cb->buf_read_requested(target_cfg, &rdata, &len);
			}
#endif
			if (len > sizeof(data->target_out_buffer)) {
				LOG_ERR("I2CS ch%d: The length exceeds out_buffer size=%d",
					config->port, sizeof(data->target_out_buffer));
			} else {
				memcpy(data->target_out_buffer, rdata, len);
			}

			for (int i = 0; i < SMB_TARGET_IT51XXX_MAX_FIFO_SIZE; i++) {
				/* Host receiving, target transmitting */
#ifdef CONFIG_SOC_IT51526AW
				sys_write8(data->target_out_buffer[i + data->r_index],
					   config->i2cbase_mapping + SMB_SLDA(config->port));
#else
				sys_write8(data->target_out_buffer[i + data->r_index],
					   config->target_base + SMB_SLDn);
#endif
			}
			/* Index to next 16 bytes of read buffer */
			data->r_index += SMB_TARGET_IT51XXX_MAX_FIFO_SIZE;
		} else {
			for (int i = 0; i < count; i++) {
				/* Host transmitting, target receiving */
#ifdef CONFIG_SOC_IT51526AW
				data->target_in_buffer[i + data->w_index] =
					sys_read8(config->i2cbase_mapping + SMB_SLDA(config->port));
#else
				data->target_in_buffer[i + data->w_index] =
					sys_read8(config->target_base + SMB_SLDn);
#endif
			}
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
			/* Write data done callback function */
			if (target_cb->buf_write_received) {
				target_cb->buf_write_received(target_cfg, data->target_in_buffer,
							      count);
			}
#endif
			/* Index to next 16 bytes of write buffer */
			data->w_index += count;
			if (data->w_index > sizeof(data->target_in_buffer)) {
				LOG_ERR("I2CS ch%d: The write size exceeds in buffer size=%d",
					config->port, sizeof(data->target_in_buffer));
			}
		}
	}
	/* Stop condition, indicate stop condition detected. */
	if (target_status & SMB_SPDS) {
		/* Read data less 16 bytes status */
		if (target_status & SMB_RCS) {
			/* Disable FIFO mode to clear left count */
			sdfpctl = sys_read8(config->target_base + SMB_SnDFPCTL);
			sys_write8(sdfpctl & ~SMB_SADFE, config->target_base + SMB_SnDFPCTL);
			/* Target n FIFO Enable */
			sdfpctl = sys_read8(config->target_base + SMB_SnDFPCTL);
			sys_write8(sdfpctl | SMB_SADFE, config->target_base + SMB_SnDFPCTL);
		} else {
			for (int i = 0; i < count; i++) {
				/* Host transmitting, target receiving */
#ifdef CONFIG_SOC_IT51526AW
				data->target_in_buffer[i + data->w_index] =
					sys_read8(config->i2cbase_mapping + SMB_SLDA(config->port));
#else
				data->target_in_buffer[i + data->w_index] =
					sys_read8(config->target_base + SMB_SLDn);
#endif
			}
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
			/* Write data done callback function */
			if (target_cb->buf_write_received) {
				target_cb->buf_write_received(target_cfg, data->target_in_buffer,
							      count);
			}
#endif
		}

		/* Transfer done callback function */
		if (target_cb->stop) {
			target_cb->stop(target_cfg);
		}
		data->w_index = 0;
		data->r_index = 0;
	}

done:
	/* W/C */
#ifdef CONFIG_SOC_IT51526AW
	sys_write8(target_status, config->i2cbase_mapping + SMB_SLSTA(config->port));
#else
	sys_write8(target_status, config->target_base + SMB_SLSTn);
#endif
}

static void clear_target_status(const struct device *dev, uint8_t status)
{
	const struct i2c_it51xxx_config *config = dev->config;

	/* Write to clear a specific status */
#ifdef CONFIG_SOC_IT51526AW
	sys_write8(status, config->i2cbase_mapping + SMB_SLSTA(config->port));
#else
	sys_write8(status, config->target_base + SMB_SLSTn);
#endif
}

static void target_i2c_isr_pio(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	struct i2c_target_config *target_cfg;
	const struct i2c_target_callbacks *target_cb;
	uint8_t target_status, target_idx;
	uint8_t val;

	target_status = sys_read8(config->target_base + SMB_SLSTn);
	/* Write to clear a target status */
	clear_target_status(dev, target_status);

	/* Any error */
	if (target_status & SMB_STS) {
		data->w_index = 0;
		data->r_index = 0;

		return;
	}

	/* Which target address to match. */
	target_idx = (target_status & SMB_MSLA2) ? SMB_SADR2 : SMB_SADR;
	target_cfg = data->target_cfg[target_idx];
	target_cb = target_cfg->callbacks;

	/* Stop condition, indicate stop condition detected. */
	if (target_status & SMB_SPDS) {
		/* Transfer done callback function */
		if (target_cb->stop) {
			target_cb->stop(target_cfg);
		}
		data->w_index = 0;
		data->r_index = 0;

		if (config->target_shared_fifo_mode) {
			uint8_t sdfpctl;

			/* Disable FIFO mode to clear left count */
			sdfpctl = sys_read8(config->target_base + SMB_SnDFPCTL);
			sys_write8(sdfpctl & ~SMB_SADFE, config->target_base + SMB_SnDFPCTL);
		}
	}

	if (target_status & SMB_SDS) {
		if (target_status & SMB_RCS) {
			/* Target shared FIFO mode */
			if (config->target_shared_fifo_mode) {
				uint32_t len;
				uint8_t *rdata = NULL;
				uint8_t sndfpctl;

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
				/* Read data callback function */
				if (target_cb->buf_read_requested) {
					target_cb->buf_read_requested(target_cfg, &rdata, &len);
				}
#endif
				if (len > sizeof(data->target_shared_fifo)) {
					LOG_ERR("I2CS ch%d: The length exceeds shared fifo size=%d",
						config->port, sizeof(data->target_shared_fifo));
				} else {
					memcpy(data->target_shared_fifo, rdata, len);
				}
				/* Target n FIFO enable */
				sndfpctl = sys_read8(config->target_base + SMB_SnDFPCTL);
				sys_write8(sndfpctl | SMB_SADFE,
					   config->target_base + SMB_SnDFPCTL);
				/* Write to clear data status of target */
				clear_target_status(dev, SMB_SDS);
			} else {
				/* Host receiving, target transmitting */
				if (!data->r_index) {
					if (target_cb->read_requested) {
						target_cb->read_requested(target_cfg, &val);
					}
				} else {
					if (target_cb->read_processed) {
						target_cb->read_processed(target_cfg, &val);
					}
				}
				/* Write data */
				sys_write8(val, config->target_base + SMB_SLDn);
				/* Release clock pin */
				sys_write8(val, config->target_base + SMB_SLDn);
				data->r_index++;
			}
		} else {
			/* Host transmitting, target receiving */
			if (!data->w_index) {
				if (target_cb->write_requested) {
					target_cb->write_requested(target_cfg);
				}
			}
			/* Read data */
			val = sys_read8(config->target_base + SMB_SLDn);
			if (target_cb->write_received) {
				target_cb->write_received(target_cfg, val);
			}
			/* Release target clock stretch */
			sys_write8(sys_read8(config->target_base + SMB_SLVCTLn) | SMB_RSCS,
				   config->target_base + SMB_SLVCTLn);
			data->w_index++;
		}
	}
}

static void target_i2c_isr(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;

	if (config->target_fifo_mode) {
		target_i2c_isr_fifo(dev);
	} else {
		target_i2c_isr_pio(dev);
	}
}
#endif /* CONFIG_I2C_TARGET */

static int i2c_parsing_return_value(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;

	if (!data->err) {
		return 0;
	}

	if (data->err == ETIMEDOUT) {
		/* Connection timed out */
		LOG_ERR("I2C ch%d Address:0x%X Transaction time out.", config->port,
			data->addr_16bit);
	} else {
		LOG_DBG("I2C ch%d Address:0x%X Host error bits message:", config->port,
			data->addr_16bit);
		/* Host error bits message*/
		if (data->err & HOSTA_TMOE) {
			LOG_ERR("Time-out error: hardware time-out error.");
		}
		if (data->err & HOSTA_NACK) {
			LOG_DBG("NACK error: device does not response ACK.");
		}
		if (data->err & HOSTA_FAIL) {
			LOG_ERR("Fail: a processing transmission is killed.");
		}
		if (data->err & HOSTA_BSER) {
			LOG_ERR("BUS error: SMBus has lost arbitration.");
		}
	}

	return -EIO;
}

static int i2c_get_line_levels(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;

	return (sys_read8(config->host_base + SMB_SMBPCTL) & (SMB_HSMBDCS | SMB_HSMBCS));
}

static int i2c_is_busy(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;

	return (sys_read8(config->host_base + SMB_HOSTA) & (HOSTA_HOBY | HOSTA_ALL_WC_BIT));
}

static int i2c_bus_not_available(const struct device *dev)
{
	if (i2c_is_busy(dev) || (i2c_get_line_levels(dev) != SMB_LINE_IDLE)) {
		return -EIO;
	}

	return 0;
}

static void i2c_reset(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;

	/* bit1, kill current transaction. */
	sys_write8(SMB_KILL, config->host_base + SMB_HOCTL);
	sys_write8(0, config->host_base + SMB_HOCTL);
	/* W/C host status register */
	sys_write8(HOSTA_ALL_WC_BIT, config->host_base + SMB_HOSTA);
}

void i2c_r_last_byte(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint8_t hoctl;

	/*
	 * bit5, The firmware shall write 1 to this bit
	 * when the next byte will be the last byte for i2c read.
	 */
	if ((data->msg->flags & I2C_MSG_STOP) && (data->ridx == data->msg->len - 1)) {
		hoctl = sys_read8(config->host_base + SMB_HOCTL);
		sys_write8(hoctl | SMB_LABY, config->host_base + SMB_HOCTL);
	}
}

void i2c_w2r_change_direction(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	uint8_t hoctl2;

	/* I2C switch direction */
	if (sys_read8(config->host_base + SMB_HOCTL2) & I2C_SW_EN) {
		i2c_r_last_byte(dev);
		sys_write8(SMB_BDS, config->host_base + SMB_HOSTA);
	} else {
		hoctl2 = sys_read8(config->host_base + SMB_HOCTL2);
		sys_write8(hoctl2 | I2C_SW_EN | I2C_SW_WAIT, config->host_base + SMB_HOCTL2);

		sys_write8(SMB_BDS, config->host_base + SMB_HOSTA);
		i2c_r_last_byte(dev);

		hoctl2 = sys_read8(config->host_base + SMB_HOCTL2);
		sys_write8(hoctl2 & ~I2C_SW_WAIT, config->host_base + SMB_HOCTL2);
	}
}

int i2c_tran_read(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;

	if (data->msg->flags & SMB_MSG_START) {
		/* I2C enable */
		sys_write8(SMB_SMD_TO_EN | I2C_EN | SMB_SMH_EN, config->host_base + SMB_HOCTL2);

		sys_write8((uint8_t)(data->addr_16bit << 1) | SMB_DIR,
			   config->host_base + SMB_TRASLA);
		/* Clear start flag */
		data->msg->flags &= ~SMB_MSG_START;

		if ((data->msg->len == 1) && (data->msg->flags & I2C_MSG_STOP)) {
			sys_write8(SMB_SRT | SMB_LABY | SMB_SMCD(7) | SMB_INTREN,
				   config->host_base + SMB_HOCTL);
		} else {
			sys_write8(SMB_SRT | SMB_SMCD(7) | SMB_INTREN,
				   config->host_base + SMB_HOCTL);
		}
	} else {
		if ((data->i2ccs == I2C_CH_REPEAT_START) || (data->i2ccs == I2C_CH_WAIT_READ)) {
			if (data->i2ccs == I2C_CH_REPEAT_START) {
				/* Write to read */
				i2c_w2r_change_direction(dev);
			} else {
				/* For last byte */
				i2c_r_last_byte(dev);
				/* W/C for next byte */
				sys_write8(SMB_BDS, config->host_base + SMB_HOSTA);
			}
			data->i2ccs = I2C_CH_NORMAL;
		} else if (sys_read8(config->host_base + SMB_HOSTA) & SMB_BDS) {
			if (data->ridx < data->msg->len) {
				/* To get received data. */
				*(data->msg->buf++) = sys_read8(config->host_base + SMB_HOBDB);
				data->ridx++;
				/* For last byte */
				i2c_r_last_byte(dev);
				/* Done */
				if (data->ridx == data->msg->len) {
					data->msg->len = 0;
					if (data->msg->flags & I2C_MSG_STOP) {
						/* W/C for finish */
						sys_write8(SMB_BDS, config->host_base + SMB_HOSTA);

						data->stop = 1;
					} else {
						data->i2ccs = I2C_CH_WAIT_READ;
						return 0;
					}
				} else {
					/* W/C for next byte */
					sys_write8(SMB_BDS, config->host_base + SMB_HOSTA);
				}
			}
		}
	}

	return 1;
}

int i2c_tran_write(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;

	if (data->msg->flags & SMB_MSG_START) {
		/* I2C enable */
		sys_write8(SMB_SMD_TO_EN | I2C_EN | SMB_SMH_EN, config->host_base + SMB_HOCTL2);

		sys_write8((uint8_t)(data->addr_16bit << 1), config->host_base + SMB_TRASLA);
		/* Send first byte */
		sys_write8(*(data->msg->buf++), config->host_base + SMB_HOBDB);

		data->widx++;
		/* Clear start flag */
		data->msg->flags &= ~SMB_MSG_START;

		sys_write8(SMB_SRT | SMB_SMCD(7) | SMB_INTREN, config->host_base + SMB_HOCTL);
	} else {
		/* Host has completed the transmission of a byte */
		if (sys_read8(config->host_base + SMB_HOSTA) & SMB_BDS) {
			if (data->widx < data->msg->len) {
				/* Send next byte */
				sys_write8(*(data->msg->buf++), config->host_base + SMB_HOBDB);

				data->widx++;
				/* W/C byte done for next byte */
				sys_write8(SMB_BDS, config->host_base + SMB_HOSTA);

				if (data->i2ccs == I2C_CH_REPEAT_START) {
					data->i2ccs = I2C_CH_NORMAL;
				}
			} else {
				/* Done */
				data->msg->len = 0;
				if (data->msg->flags & I2C_MSG_STOP) {
					/* Set I2C_EN = 0 */
					sys_write8(SMB_SMD_TO_EN | SMB_SMH_EN,
						   config->host_base + SMB_HOCTL2);
					/* W/C byte done for finish */
					sys_write8(SMB_BDS, config->host_base + SMB_HOSTA);

					data->stop = 1;
				} else {
					data->i2ccs = I2C_CH_REPEAT_START;
					return 0;
				}
			}
		}
	}

	return 1;
}

int i2c_pio_transaction(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;

	/* Any error */
	if (sys_read8(config->host_base + SMB_HOSTA) & HOSTA_ANY_ERROR) {
		data->err = sys_read8(config->host_base + SMB_HOSTA) & HOSTA_ANY_ERROR;
	} else {
		if (!data->stop) {
			/*
			 * The return value indicates if there is more data to be read or written.
			 * If the return value = 1, it means that the interrupt cannot be disable
			 * and continue to transmit data.
			 */
			if (data->msg->flags & I2C_MSG_READ) {
				return i2c_tran_read(dev);
			} else {
				return i2c_tran_write(dev);
			}
		}
		/* Wait finish */
		if (!(sys_read8(config->host_base + SMB_HOSTA) & SMB_FINTR)) {
			return 1;
		}
	}
	/* W/C */
	sys_write8(HOSTA_ALL_WC_BIT, config->host_base + SMB_HOSTA);

	/* Disable the SMBus host interface */
	sys_write8(0, config->host_base + SMB_HOCTL2);

	data->stop = 0;
	/* Done doing work */
	return 0;
}

#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
void i2c_fifo_en_w2r(const struct device *dev, bool enable)
{
	const struct i2c_it51xxx_config *config = dev->config;
	unsigned int key = irq_lock();
	uint8_t i2cw2rf;

	i2cw2rf = sys_read8(config->host_base + SMB_I2CW2RF);

	if (enable) {
		sys_write8(i2cw2rf | SMB_MAIF | SMB_MAIFI, config->host_base + SMB_I2CW2RF);

	} else {
		sys_write8(i2cw2rf & ~(SMB_MAIF | SMB_MAIFI), config->host_base + SMB_I2CW2RF);
	}

	irq_unlock(key);
}

void i2c_tran_fifo_write_start(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint32_t i;
	uint8_t fifo_en;

	/* Clear start flag. */
	data->msg->flags &= ~SMB_MSG_START;

	fifo_en = (config->port == SMB_CHANNEL_A) ? SMB_FF1EN : SMB_FF2EN;
	/* Enable SMB channel in FIFO mode. */
	sys_write8(sys_read8(config->i2cbase + SMB_MSTFCSTS) | fifo_en,
		   config->i2cbase + SMB_MSTFCSTS);

	/* I2C enable. */
	sys_write8(SMB_SMD_TO_EN | I2C_EN | SMB_SMH_EN, config->host_base + SMB_HOCTL2);
	/* Set write byte counts. */
	sys_write8(data->msg->len, config->host_base + SMB_D0REG);
	/* Set transmit target address */
	sys_write8((uint8_t)(data->addr_16bit << 1), config->host_base + SMB_TRASLA);

	/* The maximum fifo size is 32 bytes. */
	data->bytecnt = MIN(data->msg->len, SMB_FIFO_MODE_MAX_SIZE);
	for (i = 0; i < data->bytecnt; i++) {
		/* Set host block data byte. */
		sys_write8(*(data->msg->buf++), config->host_base + SMB_HOBDB);
	}
	/* Calculate the remaining byte counts. */
	data->bytecnt = data->msg->len - data->bytecnt;

	/* Set host control */
	sys_write8(SMB_SRT | SMB_SMCD(7) | SMB_INTREN, config->host_base + SMB_HOCTL);
}

void i2c_tran_fifo_write_next_block(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint32_t i, _bytecnt;
	uint8_t blkds;

	blkds = (config->port == SMB_CHANNEL_A) ? SMB_BLKDS1 : SMB_BLKDS2;

	/* The maximum fifo size is 32 bytes. */
	_bytecnt = MIN(data->bytecnt, SMB_FIFO_MODE_MAX_SIZE);
	for (i = 0; i < _bytecnt; i++) {
		/* Set host block data byte. */
		sys_write8(*(data->msg->buf++), config->host_base + SMB_HOBDB);
	}

	/* Clear FIFO block done status. */
#ifdef CONFIG_SOC_IT51526AW
	uint8_t mstfctrl;

	mstfctrl = (config->port == SMB_CHANNEL_A) ? SMB_MSTFCTRL1 : SMB_MSTFCTRL2;
	sys_write8(sys_read8(config->i2cbase_mapping + mstfctrl) | SMB_BLKDS,
		   config->i2cbase_mapping + mstfctrl);
#else
	sys_write8(sys_read8(config->i2cbase + SMB_MSTFCSTS) | blkds,
		   config->i2cbase + SMB_MSTFCSTS);
#endif
	/* Calculate the remaining byte counts. */
	data->bytecnt -= _bytecnt;
}

void i2c_tran_fifo_write_finish(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;

	/* Clear byte count register. */
	sys_write8(0, config->host_base + SMB_D0REG);
	/* W/C */
	sys_write8(HOSTA_ALL_WC_BIT, config->host_base + SMB_HOSTA);
	/* Disable the SMBus host interface. */
	sys_write8(0, config->host_base + SMB_HOCTL2);
}

int i2c_tran_fifo_w2r_change_direction(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint8_t hoctl2;

	if (++data->msg_index >= data->num_msgs) {
		LOG_ERR("%s: Current message index is error.", dev->name);
		data->err = EINVAL;
		/* W/C */
		sys_write8(HOSTA_ALL_WC_BIT, config->host_base + SMB_HOSTA);
		/* Disable the SMBus host interface. */
		sys_write8(0, config->host_base + SMB_HOCTL2);

		return 0;
	}

	/* Set I2C_SW_EN = 1 */
	hoctl2 = sys_read8(config->host_base + SMB_HOCTL2);
	sys_write8(hoctl2 | I2C_SW_EN | I2C_SW_WAIT, config->host_base + SMB_HOCTL2);

	hoctl2 = sys_read8(config->host_base + SMB_HOCTL2);
	sys_write8(hoctl2 & ~I2C_SW_WAIT, config->host_base + SMB_HOCTL2);

	/* Point to the next msg for the read location. */
	data->msg = &data->msgs_list[data->msg_index];
	/* Set read byte counts. */
	sys_write8(data->msg->len, config->host_base + SMB_D0REG);
	data->bytecnt = data->msg->len;

	/* W/C I2C W2R FIFO interrupt status. */
	sys_write8(sys_read8(config->host_base + SMB_I2CW2RF) | SMB_MAIFID,
		   config->host_base + SMB_I2CW2RF);

	return 1;
}

void i2c_tran_fifo_read_start(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint8_t fifo_en;

	/* Clear start flag. */
	data->msg->flags &= ~SMB_MSG_START;

	fifo_en = (config->port == SMB_CHANNEL_A) ? SMB_FF1EN : SMB_FF2EN;
	/* Enable SMB channel in FIFO mode. */
	sys_write8(sys_read8(config->i2cbase + SMB_MSTFCSTS) | fifo_en,
		   config->i2cbase + SMB_MSTFCSTS);

	data->bytecnt = data->msg->len;

	/* I2C enable. */
	sys_write8(SMB_SMD_TO_EN | I2C_EN | SMB_SMH_EN, config->host_base + SMB_HOCTL2);
	/* Set read byte counts. */
	sys_write8(data->msg->len, config->host_base + SMB_D0REG);
	/* Set transmit target address */
	sys_write8((uint8_t)(data->addr_16bit << 1) | SMB_DIR, config->host_base + SMB_TRASLA);
	/* Set host control */
	sys_write8(SMB_SRT | SMB_SMCD(7) | SMB_INTREN, config->host_base + SMB_HOCTL);
}

void i2c_tran_fifo_read_next_block(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint32_t i;
	uint8_t blkds;

	blkds = (config->port == SMB_CHANNEL_A) ? SMB_BLKDS1 : SMB_BLKDS2;

	for (i = 0; i < SMB_FIFO_MODE_MAX_SIZE; i++) {
		/* To get received data. */
		*(data->msg->buf++) = sys_read8(config->host_base + SMB_HOBDB);
	}
	/* Clear FIFO block done status. */
	sys_write8(sys_read8(config->i2cbase + SMB_MSTFCSTS) | blkds,
		   config->i2cbase + SMB_MSTFCSTS);

	/* Calculate the remaining byte counts. */
	data->bytecnt -= SMB_FIFO_MODE_MAX_SIZE;
}

void i2c_tran_fifo_read_finish(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint32_t i;

	for (i = 0; i < data->bytecnt; i++) {
		/* To get received data. */
		*(data->msg->buf++) = sys_read8(config->host_base + SMB_HOBDB);
	}
	/* Clear byte count register. */
	sys_write8(0, config->host_base + SMB_D0REG);
	/* W/C */
	sys_write8(HOSTA_ALL_WC_BIT, config->host_base + SMB_HOSTA);
	/* Disable the SMBus host interface. */
	sys_write8(0, config->host_base + SMB_HOCTL2);
}

int i2c_tran_fifo_write_to_read(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	int ret = 1;
	uint8_t blkds;

	blkds = (config->port == SMB_CHANNEL_A) ? SMB_BLKDS1 : SMB_BLKDS2;

	if (data->msg->flags & SMB_MSG_START) {
		/* Enable I2C write to read FIFO mode. */
		i2c_fifo_en_w2r(dev, true);
		i2c_tran_fifo_write_start(dev);
	} else {
		/* Check block done status. */
		if (sys_read8(config->i2cbase + SMB_MSTFCSTS) & blkds) {
			if (sys_read8(config->host_base + SMB_HOCTL2) & I2C_SW_EN) {
				i2c_tran_fifo_read_next_block(dev);
			} else {
				i2c_tran_fifo_write_next_block(dev);
			}
		} else if (sys_read8(config->host_base + SMB_I2CW2RF) & SMB_MAIFID) {
			/*
			 * This function returns 0 on a failure to indicate that the current
			 * transaction is completed and returned the data->err.
			 */
			ret = i2c_tran_fifo_w2r_change_direction(dev);
		} else {
			/* Wait finish. */
			if (sys_read8(config->host_base + SMB_HOSTA) & HOSTA_FINTR) {
				i2c_tran_fifo_read_finish(dev);
				/* Done doing work. */
				ret = 0;
			}
		}
	}

	return ret;
}

int i2c_tran_fifo_read(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint8_t blkds;

	blkds = (config->port == SMB_CHANNEL_A) ? SMB_BLKDS1 : SMB_BLKDS2;

	if (data->msg->flags & SMB_MSG_START) {
		i2c_tran_fifo_read_start(dev);
	} else {
		/* Check block done status. */
		if (sys_read8(config->i2cbase + SMB_MSTFCSTS) & blkds) {
			i2c_tran_fifo_read_next_block(dev);
		} else {
			/* Wait finish. */
			if (sys_read8(config->host_base + SMB_HOSTA) & HOSTA_FINTR) {
				i2c_tran_fifo_read_finish(dev);
				/* Done doing work. */
				return 0;
			}
		}
	}

	return 1;
}

int i2c_tran_fifo_write(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint8_t blkds;

	blkds = (config->port == SMB_CHANNEL_A) ? SMB_BLKDS1 : SMB_BLKDS2;

	if (data->msg->flags & SMB_MSG_START) {
		i2c_tran_fifo_write_start(dev);
	} else {
		/* Check block done status. */
		if (sys_read8(config->i2cbase + SMB_MSTFCSTS) & blkds) {
			i2c_tran_fifo_write_next_block(dev);
		} else {
			/* Wait finish. */
			if (sys_read8(config->host_base + SMB_HOSTA) & HOSTA_FINTR) {
				i2c_tran_fifo_write_finish(dev);
				/* Done doing work. */
				return 0;
			}
		}
	}

	return 1;
}

int i2c_fifo_transaction(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;

	/* Any error. */
	if (sys_read8(config->host_base + SMB_HOSTA) & HOSTA_ANY_ERROR) {
		data->err = (sys_read8(config->host_base + SMB_HOSTA) & HOSTA_ANY_ERROR);
	} else {
		if (data->num_msgs == 2) {
			return i2c_tran_fifo_write_to_read(dev);
		} else if (data->msg->flags & I2C_MSG_READ) {
			return i2c_tran_fifo_read(dev);
		} else {
			return i2c_tran_fifo_write(dev);
		}
	}
	/* W/C */
	sys_write8(HOSTA_ALL_WC_BIT, config->host_base + SMB_HOSTA);
	/* Disable the SMBus host interface. */
	sys_write8(0, config->host_base + SMB_HOCTL2);

	return 0;
}

static void i2c_it51xxx_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;

#ifdef CONFIG_I2C_TARGET
	if (atomic_get(&data->num_registered_addrs) != 0) {
		target_i2c_isr(dev);
	} else {
#endif
#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
		uint8_t fifo_en;

		fifo_en = (config->port == SMB_CHANNEL_A) ? SMB_FF1EN : SMB_FF2EN;
		/* If done doing work, wake up the task waiting for the transfer. */
		if (config->fifo_enable && (sys_read8(config->i2cbase + SMB_MSTFCSTS) & fifo_en)) {
			if (i2c_fifo_transaction(dev)) {
				return;
			}
		} else {
#endif
			if (i2c_pio_transaction(dev)) {
				return;
			}
#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
		}
#endif
		irq_disable(config->i2c_irq_base);
		k_sem_give(&data->device_sync_sem);
#ifdef CONFIG_I2C_TARGET
	}
#endif
}

bool fifo_mode_allowed(const struct device *dev, struct i2c_msg *msgs)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;

	/*
	 * If the transaction of write or read is divided into two transfers(not two messages),
	 * the FIFO mode does not support.
	 */
	if (data->i2ccs != I2C_CH_NORMAL) {
		return false;
	}
	/*
	 * FIFO2 only supports one channel of B or C. If the FIFO of channel is not enabled,
	 * it will select PIO mode.
	 */
	if (!config->fifo_enable) {
		return false;
	}
	/*
	 * When there is only one message, use the FIFO mode transfer directly.
	 * Transfer payload too long (>255 bytes), use PIO mode.
	 * Write or read of I2C target address without data, used by cmd_i2c_scan. Use PIO mode.
	 */
	if (data->num_msgs == 1 && (msgs[0].flags & I2C_MSG_STOP) &&
	    (msgs[0].len <= SMB_FIFO_MODE_TOTAL_LEN) && (msgs[0].len != 0)) {
		return true;
	}
	/*
	 * When there are two messages, we need to judge whether or not there is I2C_MSG_RESTART
	 * flag from the second message, and then decide todo the FIFO mode or PIO mode transfer.
	 */
	if (data->num_msgs == 2) {
		/*
		 * The first of two messages must be write.
		 * Transfer payload too long (>255 bytes), use PIO mode.
		 */
		if (((msgs[0].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) &&
		    (msgs[0].len <= SMB_FIFO_MODE_TOTAL_LEN)) {
			/*
			 * The transfer is i2c_burst_read().
			 *
			 * e.g. msg[0].flags = I2C_MSG_WRITE;
			 *      msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ |
			 *                     I2C_MSG_STOP;
			 */
			if ((msgs[1].flags == SMB_MSG_BURST_READ_MASK) &&
			    (msgs[1].len <= SMB_FIFO_MODE_TOTAL_LEN)) {
				return true;
			}
		}
	}

	return false;
}
#endif /* CONFIG_I2C_IT51XXX_FIFO_MODE */

static void i2c_standard_port_timing_regs_400khz(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;

	/* Port clock frequency depends on setting of timing registers. */
	sys_write8(0, config->host_base + SMB_MSCLKTS);
	/* Suggested setting of timing registers of 400kHz. */
	sys_write8(0x05, config->host_base + SMB_4P7USL);
	sys_write8(0x01, config->host_base + SMB_4P0USL);
	sys_write8(0x03, config->host_base + SMB_250NSREG);
	sys_write8(0xc9, config->host_base + SMB_45P3USLREG);
	sys_write8(0x01, config->host_base + SMB_45P3USHREG);
	sys_write8(0x00, config->host_base + SMB_4P7A4P0H);
}

static void i2c_standard_port_set_frequency(const struct device *dev, int freq_hz, int freq_set)
{
	const struct i2c_it51xxx_config *config = dev->config;
	uint8_t honacksrc;

	/*
	 * If port's clock frequency is 400kHz, we use timing registers for setting. So we can
	 * adjust tlow to meet timing. The others use basic 50/100/1000 KHz setting.
	 */
	if (freq_hz == I2C_BITRATE_FAST) {
		i2c_standard_port_timing_regs_400khz(dev);
	} else {
		sys_write8(freq_set, config->host_base + SMB_MSCLKTS);
	}

	/* Host SMCLK & SMDAT timeout disable */
	honacksrc = sys_read8(config->host_base + SMB_HONACKSRC);
	sys_write8(honacksrc | SMB_HSMCDTD, config->host_base + SMB_HONACKSRC);
}

static int i2c_it51xxx_configure(const struct device *dev, uint32_t dev_config_raw)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *const data = dev->data;
	uint32_t freq_set;

	if (!(I2C_MODE_CONTROLLER & dev_config_raw)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -ENOTSUP;
	}

	data->bus_freq = I2C_SPEED_GET(dev_config_raw);

	switch (data->bus_freq) {
	case I2C_SPEED_DT:
		freq_set = SMB_CLKS_50K;
		break;
	case I2C_SPEED_STANDARD:
		freq_set = SMB_CLKS_100K;
		break;
	case I2C_SPEED_FAST:
		freq_set = SMB_CLKS_400K;
		break;
	case I2C_SPEED_FAST_PLUS:
		freq_set = SMB_CLKS_1M;
		break;
	default:
		return -EINVAL;
	}

	i2c_standard_port_set_frequency(dev, config->bitrate, freq_set);

	return 0;
}

static int i2c_it51xxx_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_it51xxx_data *const data = dev->data;
	uint32_t speed;

	if (!data->bus_freq) {
		LOG_ERR("The bus frequency is not initially configured.");
		return -EIO;
	}

	switch (data->bus_freq) {
	case I2C_SPEED_DT:
	case I2C_SPEED_STANDARD:
	case I2C_SPEED_FAST:
	case I2C_SPEED_FAST_PLUS:
		speed = I2C_SPEED_SET(data->bus_freq);
		break;
	default:
		return -ERANGE;
	}

	*dev_config = (I2C_MODE_CONTROLLER | speed);

	return 0;
}

static int i2c_it51xxx_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	int ret;

#ifdef CONFIG_I2C_TARGET
	if (atomic_get(&data->num_registered_addrs) != 0) {
		LOG_ERR("I2CS ch%d: Device is registered as target", config->port);
		return -EBUSY;
	}
#endif
	/* Lock mutex of i2c controller */
	k_mutex_lock(&data->mutex, K_FOREVER);
#ifdef CONFIG_PM
	/* Block to enter power policy. */
	i2c_ite_pm_policy_state_lock_get(data, I2CM_ITE_PM_POLICY_FLAG);
#endif
	/*
	 * If the transaction of write to read is divided into two transfers, the repeat start
	 * transfer uses this flag to exclude checking bus busy.
	 */
	if (data->i2ccs == I2C_CH_NORMAL) {
		/* Make sure we're in a good state to start */
		if (i2c_bus_not_available(dev)) {
			/* Recovery I2C bus */
			i2c_recover_bus(dev);
			/*
			 * After resetting I2C bus, if I2C bus is not available
			 * (No external pull-up), drop the transaction.
			 */
			if (i2c_bus_not_available(dev)) {
				ret = -EIO;
				goto done;
			}
		}

		msgs[0].flags |= SMB_MSG_START;
	}

#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
	/* Store num_msgs to data struct. */
	data->num_msgs = num_msgs;
	/* Store msgs to data struct. */
	data->msgs_list = msgs;
	data->msg_index = 0;

	bool fifo_mode_enable = fifo_mode_allowed(dev, msgs);
#endif
	for (int i = 0; i < num_msgs; i++) {
		data->widx = 0;
		data->ridx = 0;
		data->err = 0;
		data->msg = &msgs[i];
		data->addr_16bit = addr;

#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
		/*
		 * Start transaction.
		 * The return value indicates if the initial configuration
		 * of I2C transaction for read or write has been completed.
		 */
		if (fifo_mode_enable) {
			if (i2c_fifo_transaction(dev)) {
				/* Enable i2c interrupt */
				irq_enable(config->i2c_irq_base);
			}
		} else {
#endif
			if (i2c_pio_transaction(dev)) {
				/* Enable i2c interrupt */
				irq_enable(config->i2c_irq_base);
			}
#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
		}
#endif
		/* Wait for the transfer to complete */
		ret = k_sem_take(&data->device_sync_sem, K_MSEC(config->transfer_timeout_ms));
		/*
		 * The irq will be enabled at the condition of start or repeat start of I2C.
		 * If timeout occurs without being wake up during suspend(ex: interrupt is not
		 * fired), the irq should be disabled immediately.
		 */
		irq_disable(config->i2c_irq_base);
		/*
		 * The transaction is dropped on any error(timeout, NACK, fail,bus error,
		 * device error).
		 */
		if (data->err) {
			break;
		}

		if (ret != 0) {
			data->err = ETIMEDOUT;
			/* Reset i2c port */
			i2c_reset(dev);
			LOG_ERR("I2C ch%d:0x%X reset cause %d", config->port, data->addr_16bit,
				I2C_RC_TIMEOUT);
			/* If this message is sent fail, drop the transaction. */
			break;
		}

#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
		/* In FIFO mode, messages are compressed into a single transaction. */
		if (fifo_mode_enable) {
			break;
		}
#endif
	}
#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
	if (fifo_mode_enable) {
		uint8_t fifo_en;

		fifo_en = (config->port == SMB_CHANNEL_A) ? SMB_FF1EN : SMB_FF2EN;

		/* Disable SMB channels in FIFO mode. */
		sys_write8(sys_read8(config->i2cbase + SMB_MSTFCSTS) & ~fifo_en,
			   config->i2cbase + SMB_MSTFCSTS);

		/* Disable I2C write to read FIFO mode. */
		if (data->num_msgs == 2) {
			i2c_fifo_en_w2r(dev, false);
		}
	}
#endif
	/* Reset i2c channel status */
	if (data->err || (data->msg->flags & I2C_MSG_STOP)) {
		data->i2ccs = I2C_CH_NORMAL;
	}

	/* Save return value. */
	ret = i2c_parsing_return_value(dev);

done:
#ifdef CONFIG_PM
	/* Permit to enter power policy. */
	i2c_ite_pm_policy_state_lock_put(data, I2CM_ITE_PM_POLICY_FLAG);
#endif
	/* Unlock mutex of i2c controller */
	k_mutex_unlock(&data->mutex);

	return ret;
}

static void i2c_it51xxx_set_scl(void *io_context, int state)
{
	const struct i2c_it51xxx_config *config = io_context;

	gpio_pin_set_dt(&config->scl_gpios, state);
}

static void i2c_it51xxx_set_sda(void *io_context, int state)
{
	const struct i2c_it51xxx_config *config = io_context;

	gpio_pin_set_dt(&config->sda_gpios, state);
}

static int i2c_it51xxx_get_sda(void *io_context)
{
	const struct i2c_it51xxx_config *config = io_context;
	int ret = gpio_pin_get_dt(&config->sda_gpios);

	/* Default high as that would be a NACK */
	return ret != 0;
}

static const struct i2c_bitbang_io i2c_it51xxx_bitbang_io = {
	.set_scl = i2c_it51xxx_set_scl,
	.set_sda = i2c_it51xxx_set_sda,
	.get_sda = i2c_it51xxx_get_sda,
};

static int i2c_it51xxx_recover_bus(const struct device *dev)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	int ret;

	/* Output type selection */
	gpio_flags_t flags = GPIO_OUTPUT | (config->push_pull_recovery ? 0 : GPIO_OPEN_DRAIN);
	/* Set SCL of I2C as GPIO pin */
	gpio_pin_configure_dt(&config->scl_gpios, flags);
	/* Set SDA of I2C as GPIO pin */
	gpio_pin_configure_dt(&config->sda_gpios, flags);

	i2c_bitbang_init(&data->bitbang, &i2c_it51xxx_bitbang_io, (void *)config);

	ret = i2c_bitbang_recover_bus(&data->bitbang);
	if (ret != 0) {
		LOG_ERR("%s: Failed to recover bus (err %d)", dev->name, ret);
	}

	/* Set GPIO back to I2C alternate function of SCL */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s: Failed to configure I2C pins", dev->name);
		return ret;
	}

	/* Reset i2c port */
	i2c_reset(dev);
	LOG_ERR("I2C ch%d reset cause %d", config->port, I2C_RC_NO_IDLE_FOR_START);

	return 0;
}

#ifdef CONFIG_I2C_TARGET
static int i2c_it51xxx_target_register(const struct device *dev,
				       struct i2c_target_config *target_cfg)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	uint8_t slsta;

	if (!target_cfg) {
		return -EINVAL;
	}

	if (target_cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) {
		return -ENOTSUP;
	}

	if (atomic_get(&data->num_registered_addrs) >= MAX_I2C_TARGET_ADDRS) {
		LOG_ERR("%s: One device supports at most two target addresses", __func__);
		return -ENOMEM;
	}

	/* Compare with the saved I2C address */
	for (int i = 0; i < MAX_I2C_TARGET_ADDRS; i++) {
		if (data->registered_addrs[i] == target_cfg->address) {
			LOG_ERR("%s: I2C target address=%x already registered", __func__,
				target_cfg->address);
			return -EALREADY;
		}
	}

	/* To confirm which target_cfg is empty */
	for (int i = 0; i < MAX_I2C_TARGET_ADDRS; i++) {
		if (data->target_cfg[i] == NULL && data->registered_addrs[i] == 0) {
			if (i == SMB_SADR) {
				LOG_INF("I2C target register address=%x", target_cfg->address);
				/* Target address[6:0] */
				sys_write8(target_cfg->address, config->target_base + SMB_RESLADR);
			} else if (i == SMB_SADR2) {
				LOG_INF("I2C target register address2=%x", target_cfg->address);
				/* Target address 2[6:0] */
				sys_write8(target_cfg->address,
					   config->target_base + SMB_RESLADR2n);
				/* Target address 2 enable */
				sys_write8(sys_read8(config->target_base + SMB_RESLADR2n) |
						   SMB_SADR2_EN,
					   config->target_base + SMB_RESLADR2n);
			}

			/* Save the registered I2C target_cfg */
			data->target_cfg[i] = target_cfg;
			/* Save the registered I2C target address */
			data->registered_addrs[i] = target_cfg->address;

			break;
		}
	}

	if (atomic_get(&data->num_registered_addrs) == 0) {
		if (config->target_shared_fifo_mode) {
			uint32_t fifo_addr;

			memset(data->target_shared_fifo, 0, sizeof(data->target_shared_fifo));
			fifo_addr = (uint32_t)data->target_shared_fifo & GENMASK(23, 0);
			/* Define shared FIFO base address bit[11:4] */
			sys_write8((fifo_addr >> 4) & GENMASK(7, 0),
				   config->target_base + SMB_SFBASn);
			/* Define shared FIFO base address bit[17:12] */
			sys_write8((fifo_addr >> 12) & GENMASK(5, 0),
				   config->target_base + SMB_SFBAMSn);
			/* Block to enter idle mode. */
			chip_block_idle();
		}
#ifdef CONFIG_PM
		/* Block to enter power policy. */
		i2c_ite_pm_policy_state_lock_get(data, I2CS_ITE_PM_POLICY_FLAG);
#endif
		/* Enable the SMBus target device. */
		sys_write8(sys_read8(config->target_base + SMB_SLVCTLn) | SMB_SLVEN,
			   config->target_base + SMB_SLVCTLn);

		/* Reset i2c port */
		i2c_reset(dev);

		/* W/C all target status */
		slsta = sys_read8(config->target_base + SMB_SLSTn);
		sys_write8(slsta | SMB_SPDS | SMB_STS | SMB_SDS, config->target_base + SMB_SLSTn);

		ite_intc_isr_clear(config->i2cs_irq_base);
		irq_enable(config->i2cs_irq_base);
	}
	/* data->num_registered_addrs++ */
	atomic_inc(&data->num_registered_addrs);

	return 0;
}

static int i2c_it51xxx_target_unregister(const struct device *dev,
					 struct i2c_target_config *target_cfg)
{
	const struct i2c_it51xxx_config *config = dev->config;
	struct i2c_it51xxx_data *data = dev->data;
	bool match_reg = false;

	/* Compare with the saved I2C address */
	for (int i = 0; i < MAX_I2C_TARGET_ADDRS; i++) {
		if (data->target_cfg[i] == target_cfg &&
		    data->registered_addrs[i] == target_cfg->address) {
			if (i == SMB_SADR) {
				LOG_INF("I2C target unregister address=%x", target_cfg->address);
				sys_write8(0, config->target_base + SMB_RESLADR);
			} else if (i == SMB_SADR2) {
				LOG_INF("I2C target unregister address2=%x", target_cfg->address);
				sys_write8(0, config->target_base + SMB_RESLADR2n);
			}

			data->target_cfg[i] = NULL;
			data->registered_addrs[i] = 0;
			match_reg = true;

			break;
		}
	}

	if (!match_reg) {
		LOG_ERR("%s: I2C cannot be unregistered due to address=%x mismatch", __func__,
			target_cfg->address);
		return -EINVAL;
	}

	if (atomic_get(&data->num_registered_addrs) > 0) {
		/* data->num_registered_addrs-- */
		atomic_dec(&data->num_registered_addrs);

		if (atomic_get(&data->num_registered_addrs) == 0) {
#ifdef CONFIG_PM
			/* Permit to enter power policy. */
			i2c_ite_pm_policy_state_lock_put(data, I2CS_ITE_PM_POLICY_FLAG);
#endif
			if (config->target_shared_fifo_mode) {
				/* Permit to enter idle mode. */
				chip_permit_idle();
			}

			irq_disable(config->i2cs_irq_base);
		}
	}

	return 0;
}
#endif /* CONFIG_I2C_TARGET */

static DEVICE_API(i2c, i2c_it51xxx_driver_api) = {
	.configure = i2c_it51xxx_configure,
	.get_config = i2c_it51xxx_get_config,
	.transfer = i2c_it51xxx_transfer,
	.recover_bus = i2c_it51xxx_recover_bus,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_it51xxx_target_register,
	.target_unregister = i2c_it51xxx_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int i2c_it51xxx_init(const struct device *dev)
{
	struct i2c_it51xxx_data *data = dev->data;
	const struct i2c_it51xxx_config *config = dev->config;
	int error, status;
	uint32_t bitrate_cfg;
	uint8_t smbpctlr;

#ifdef CONFIG_I2C_TARGET
	uint8_t ssclkts;

	if (config->target_enable) {
		if (config->target_fifo_mode) {
			LOG_INF("I2CS ch%d: target_in_buffer=%p, target_out_buffer=%p\n",
				config->port, data->target_in_buffer, data->target_out_buffer);
			/* Target A or B or C FIFO Enable */
			sys_write8(sys_read8(config->target_base + SMB_SnDFPCTL) | SMB_SADFE,
				   config->target_base + SMB_SnDFPCTL);
		} else if (config->target_shared_fifo_mode) {
			uint8_t ssfifoc, target_fifo_size_val = 0;

			LOG_INF("I2CS ch%d: target_shared_fifo=%p\n", config->port,
				data->target_shared_fifo);

			data->fifo_size_list = fifo_size_table;
			for (int i = 0; i <= ARRAY_SIZE(fifo_size_table); i++) {
				if (i == ARRAY_SIZE(fifo_size_table)) {
					LOG_ERR("I2CS ch%d: Unsupported target FIFO size %d",
						config->port, sizeof(data->target_shared_fifo));
					return -ENOTSUP;
				}

				if (sizeof(data->target_shared_fifo) ==
				    data->fifo_size_list[i].fifo_size) {
					target_fifo_size_val = data->fifo_size_list[i].value;
					break;
				}
			}
			/* Shared FIFO size for target A, B, C */
			ssfifoc = sys_read8(config->target_base + SMB_SSFIFOCn);
			sys_write8(ssfifoc | SMB_SFSFSA(target_fifo_size_val),
				   config->target_base + SMB_SSFIFOCn);
			/* Shared FIFO for target enable */
			sys_write8(sys_read8(config->target_base + SMB_SSFIFOCn) | SMB_SFSAE,
				   config->target_base + SMB_SSFIFOCn);
		}

		/* Target SMCLK & SMDAT timeout disable */
		sys_write8(sys_read8(config->target_base + SMB_SSCLKTSn) | SMB_SSMCDTD,
			   config->target_base + SMB_SSCLKTSn);
		/* Target SMCLK 1MHz setting disable */
		sys_write8(sys_read8(config->target_base + SMB_SSCLKTSn) & ~SMB_SCLKSA1M,
			   config->target_base + SMB_SSCLKTSn);
		/* Target channelA-C switch selection of interface */
		ssclkts = sys_read8(config->target_base + SMB_SSCLKTSn);
		sys_write8((ssclkts & ~GENMASK(5, 2)) | SMB_DSASTI(config->channel_switch_sel),
			   config->target_base + SMB_SSCLKTSn);

		/* target interrupt control */
		sys_write8(SMB_SDSEN | SMB_SDLTOEN | SMB_SITEN, config->target_base + SMB_SICRn);

		irq_connect_dynamic(config->i2cs_irq_base, 0, i2c_it51xxx_isr, dev, 0);

		goto pin_config;
	}
#endif
	/* Initialize mutex and semaphore */
	k_mutex_init(&data->mutex);
	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);

	/* Enable SMBus function */
	sys_write8(SMB_SMD_TO_EN | SMB_SMH_EN, config->host_base + SMB_HOCTL2);
	/* Kill SMBus host transaction. And enable the interrupt for the master interface */
	sys_write8(SMB_KILL | SMB_INTREN, config->host_base + SMB_HOCTL);
	sys_write8(SMB_INTREN, config->host_base + SMB_HOCTL);
	/* W/C host status register */
	sys_write8(HOSTA_ALL_WC_BIT, config->host_base + SMB_HOSTA);
	sys_write8(0, config->host_base + SMB_HOCTL2);

	/* Set clock frequency for I2C ports */
	if (config->bitrate == I2C_BITRATE_STANDARD || config->bitrate == I2C_BITRATE_FAST ||
	    config->bitrate == I2C_BITRATE_FAST_PLUS) {
		bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	} else {
		/* Device tree specified speed */
		bitrate_cfg = I2C_SPEED_DT << I2C_SPEED_SHIFT;
	}

	/* Host channelA-I switch selection of interface */
	smbpctlr = sys_read8(config->host_base + SMB_SMBPCTL);
	sys_write8((smbpctlr & ~GENMASK(7, 4)) | SMB_DASTI(config->channel_switch_sel),
		   config->host_base + SMB_SMBPCTL);

#ifdef CONFIG_I2C_IT51XXX_FIFO_MODE
	/* Select which port to use FIFO2 except port A */
	if ((config->port != SMB_CHANNEL_A) && config->fifo_enable) {
		sys_write8(SMB_FFCHSEL2(config->port - 1), config->i2cbase + SMB_MSTFCSTS);
	}
#endif
	error = i2c_it51xxx_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	data->i2ccs = I2C_CH_NORMAL;

	if (error) {
		LOG_ERR("%s: Host failure initializing", dev->name);
		return error;
	}

	irq_connect_dynamic(config->i2c_irq_base, 0, i2c_it51xxx_isr, dev, 0);

#ifdef CONFIG_I2C_TARGET
pin_config:
#endif
	/* Set the pin to I2C alternate function. */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("%s: Failed to configure I2C pins", dev->name);
		return status;
	}

	return 0;
}

#define I2C_ITE_IT51XXX_INIT(inst)                                                                 \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) == 50000) ||                             \
			     (DT_INST_PROP(inst, clock_frequency) == I2C_BITRATE_STANDARD) ||      \
			     (DT_INST_PROP(inst, clock_frequency) == I2C_BITRATE_FAST) ||          \
			     (DT_INST_PROP(inst, clock_frequency) == I2C_BITRATE_FAST_PLUS),       \
		     "Not support I2C bit rate value");                                            \
	BUILD_ASSERT(!(DT_INST_PROP(inst, target_enable) &&                                        \
		       (DT_INST_PROP(inst, port_num) > SMB_CHANNEL_C)),                            \
		     "Only ports 0~2 support I2C target mode");                                    \
	static const struct i2c_it51xxx_config i2c_it51xxx_cfg_##inst = {                          \
		.i2cbase = DT_REG_ADDR_BY_IDX(DT_NODELABEL(i2cbase), 0),                           \
		.i2cbase_mapping = DT_REG_ADDR_BY_IDX(DT_NODELABEL(i2cbase), 1),                   \
		.host_base = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                     \
		.target_base = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.scl_gpios = GPIO_DT_SPEC_INST_GET(inst, scl_gpios),                               \
		.sda_gpios = GPIO_DT_SPEC_INST_GET(inst, sda_gpios),                               \
		.transfer_timeout_ms = DT_INST_PROP(inst, transfer_timeout_ms),                    \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                                    \
		.i2c_irq_base = DT_INST_IRQ_BY_IDX(inst, 0, irq),                                  \
		.i2cs_irq_base = DT_INST_IRQ_BY_IDX(inst, 1, irq),                                 \
		.port = DT_INST_PROP(inst, port_num),                                              \
		.channel_switch_sel = DT_INST_PROP(inst, channel_switch_sel),                      \
		.fifo_enable = DT_INST_PROP(inst, fifo_enable),                                    \
		.target_enable = DT_INST_PROP(inst, target_enable),                                \
		.target_fifo_mode = DT_INST_PROP(inst, target_fifo_mode),                          \
		.target_shared_fifo_mode = DT_INST_PROP(inst, target_shared_fifo_mode),            \
		.push_pull_recovery = DT_INST_PROP(inst, push_pull_recovery),                      \
	};                                                                                         \
                                                                                                   \
	static struct i2c_it51xxx_data i2c_it51xxx_data_##inst;                                    \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_it51xxx_init, NULL, &i2c_it51xxx_data_##inst,          \
				  &i2c_it51xxx_cfg_##inst, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,  \
				  &i2c_it51xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_ITE_IT51XXX_INIT)
