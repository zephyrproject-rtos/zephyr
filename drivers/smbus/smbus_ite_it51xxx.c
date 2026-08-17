/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_smbus

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "smbus_utils.h"

LOG_MODULE_REGISTER(smbus_ite, CONFIG_SMBUS_LOG_LEVEL);

/* Host Status Register */
#define SMB_HOSTARn     0x00
/* Byte Done Status */
#define SMB_BDS         BIT(7)
/* Time-out Error */
#define SMB_TMOE        BIT(6)
/* Not Response ACK */
#define SMB_NACK        BIT(5)
/* Fail (KILL acknowledged) */
#define SMB_FAIL        BIT(4)
/* Bus Error (lost arbitration) */
#define SMB_BSER        BIT(3)
/* Device Error */
#define SMB_DVER        BIT(2)
/* Finish Interrupt (STOP detected) */
#define SMB_FINTR       BIT(1)
/* Host Busy */
#define SMB_HOBY        BIT(0)
#define HOSTA_ANY_ERROR (SMB_TMOE | SMB_NACK | SMB_FAIL | SMB_BSER | SMB_DVER)
#define HOSTA_ALL_WC    (SMB_FINTR | HOSTA_ANY_ERROR | SMB_BDS)

/* Host Control Register */
#define SMB_HOCTLRn     0x01
#define SMB_PEC_EN      BIT(7)
#define SMB_SRT         BIT(6)
#define SMB_LABY        BIT(5)
/* BIT[4:2]: SMBus command codes for SMCD field */
#define SMB_SMCD(n)     FIELD_PREP(GENMASK(4, 2), n)
#define SMCD_QUICK      0
#define SMCD_BYTE       1
#define SMCD_BYTE_DATA  2
#define SMCD_WORD_DATA  3
#define SMCD_PROC_CALL  4
#define SMCD_BLOCK      5
#define SMCD_I2C_BLOCK  6
#define SMB_KILL        BIT(1)
#define SMB_INTREN      BIT(0)
/* Pre-composed HOCTL start values (SRT | SMCD | INTREN) */
#define START_QUICK_CMD (SMB_SRT | SMB_SMCD(SMCD_QUICK) | SMB_INTREN)
#define START_BYTE_CMD  (SMB_SRT | SMB_SMCD(SMCD_BYTE) | SMB_INTREN)
#define START_BYTE_DATA (SMB_SRT | SMB_SMCD(SMCD_BYTE_DATA) | SMB_INTREN)
#define START_WORD_DATA (SMB_SRT | SMB_SMCD(SMCD_WORD_DATA) | SMB_INTREN)
#define START_PROC_CALL (SMB_SRT | SMB_SMCD(SMCD_PROC_CALL) | SMB_INTREN)
#define START_BLOCK_CMD (SMB_SRT | SMB_SMCD(SMCD_BLOCK) | SMB_INTREN)

/* Host Command Register */
#define SMB_HOCMDRn       0x02
/* Transmit Slave Address Register */
#define SMB_TRASLAn       0x03
/* 0=Write, 1=Read */
#define SMB_DIR           BIT(0)
/* Data 0 Register */
#define SMB_D0REGn        0x04
/* Data 1 Register */
#define SMB_D1REGn        0x06
/* Host Block Data Byte Register */
#define SMB_HOBDBRn       0x07
/* Master PIO Packet Error Check Register */
#define SMB_PECERCRn      0x08
/* SMBus Pin Control Register */
#define SMB_SMBPCTLRn     0x09
/* Host SMDAT Current State */
#define SMB_HSMBDCS       BIT(1)
/* Host SMCLK Current State */
#define SMB_HSMBCS        BIT(0)
/* Line-idle check */
#define SMB_LINE_SCL_HIGH SMB_HSMBCS
#define SMB_LINE_SDA_HIGH SMB_HSMBDCS
#define SMB_LINE_IDLE     (SMB_LINE_SCL_HIGH | SMB_LINE_SDA_HIGH)

/* Host Nack Source */
#define SMB_HONACKSRCn   0x0a
#define SMB_HSMCDTD      BIT(4)
/* Host Control 2 Register */
#define SMB_HOCTL2Rn     0x0b
/* Host Notify Enable (A/B/C only) */
#define SMB_HTIFYEN      BIT(6)
/* SMDAT Timeout Enable */
#define SMB_SMD_TO_EN    BIT(4)
/* I2C Enable (0=SMBus, 1=I2C mode) */
#define I2C_EN           BIT(1)
/* SMBus Host Enable */
#define SMB_SMH_EN       BIT(0)
/* SMCLK Timing Setting Register */
#define SMB_MSCLKTSn     0x0c
#define MSCLKTS_SCLKS(n) FIELD_PREP(GENMASK(2, 0), n)
/* BIT[2:0]: SMCLK Setting */
#define SMB_CLKS_1M      4
#define SMB_CLKS_400K    3
#define SMB_CLKS_100K    2
#define SMB_CLKS_50K     1

#define IT51XXX_SMBUS_BITRATE_50K 50000

/* 25 ms Register */
#define SMB_25MSREGn 0x11
/* HW PEC Enable and Status Register */
#define SMB_SPESRn   0x19
/* R/WC: Hardwired PEC Check Error */
#define SMB_MAHPCE   BIT(1)
/* R/W : Hardwired PEC Function Enable */
#define SMB_MAHPF    BIT(0)
/* I2C Wr to Rd FIFO Register */
#define SMB_I2CW2RFn 0x1b
/* Host Notify Interrupt Enable */
#define SMB_HONOIN   BIT(3)
/* Host Notify registers (A/B/C only) */
#define SMB_NDADRn   0x1c
#define SMB_HONOST   BIT(0)
/* Notify Data Low Byte Register  */
#define SMB_NDLBn    0x1d
/* Notify Data High Byte Register */
#define SMB_NDHBn    0x1e

/* Master FIFO Control Status Register */
#define SMB_MSTFCSTS    0xf0
#define SMB_BLKDS2      BIT(6)
#define SMB_FF2EN       BIT(5)
#define SMB_BLKDS1      BIT(4)
#define SMB_FF1EN       BIT(3)
#define SMB_FFCHSEL2(n) FIELD_PREP(GENMASK(2, 0), n)

/*
 * Host Notify (HTIFYEN) and the NDADRn/NDLBn/NDHBn capture registers are only
 * present on host channel A/B/C.
 */
#define IT51XXX_SMBUS_HOST_NOTIFY_MAX_PORT 2

/* Transaction timeout */
#define IT51XXX_SMBUS_TIMEOUT_MS 25

static const uint8_t start_cmd_table[] = {
	[SMBUS_CMD_QUICK] = START_QUICK_CMD,     [SMBUS_CMD_BYTE] = START_BYTE_CMD,
	[SMBUS_CMD_BYTE_DATA] = START_BYTE_DATA, [SMBUS_CMD_WORD_DATA] = START_WORD_DATA,
	[SMBUS_CMD_PROC_CALL] = START_PROC_CALL, [SMBUS_CMD_BLOCK] = START_BLOCK_CMD,
};

struct smbus_it51xxx_config {
	/* SMBus alternate configuration */
	const struct pinctrl_dev_config *pcfg;
	mm_reg_t host_base;
	mm_reg_t smbbase;
	uint32_t bitrate;
	void (*irq_config_func)(const struct device *dev);
	uint8_t smbus_irq;
	uint8_t port;
	bool fifo_enable;
};

struct smbus_it51xxx_data {
	const struct device *dev;
	struct k_sem bus_lock;
	struct k_sem xfer_done;
	/* Host Notify work, deferred out of ISR context */
	struct k_work host_notify_work;
	/* Host Notify callback list */
	sys_slist_t host_notify_cbs;
	/* Configured mode flags */
	uint32_t dev_config;
	/* Current transaction error bits captured from HOSTA */
	int err;
	uint8_t *blk_rlen;
	uint8_t *g_w_buf;
	uint8_t *g_r_buf;
	uint16_t xfer_addr;
	/* Host Notify data captured from NDHB (high) / NDLB (low) */
	uint16_t notify_data;
	uint8_t xfer_protocol;
	uint8_t blk_idx;
	uint8_t blk_len;
	/* Host Notify peripheral address captured from NDADR */
	uint8_t notify_addr;
	uint8_t pec_crc;
	uint8_t pec_recv;
	bool blk_is_write;
	bool blk_active;
	bool pec_sw;
};

static inline void it51xxx_smb_pec_update(const struct device *dev, const uint8_t *buf, size_t len)
{
	struct smbus_it51xxx_data *data = dev->data;

	data->pec_crc = crc8_ccitt(data->pec_crc, buf, len);
}

static inline bool it51xxx_smb_pec_sw_check(const struct device *dev, enum smbus_direction rw,
					    uint8_t protocol)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	/*
	 * SMBus Process Call and FIFO mode of Block Read do not support PEC hardware check.
	 */
	if ((protocol == SMBUS_CMD_PROC_CALL) ||
	    (cfg->fifo_enable && protocol == SMBUS_CMD_BLOCK && rw == SMBUS_MSG_READ)) {
		data->pec_sw = true;
	} else {
		data->pec_sw = false;
	}

	return data->pec_sw;
}

static void it51xxx_smb_reset(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	if (!(data->dev_config & SMBUS_MODE_HOST_NOTIFY)) {
		irq_disable(cfg->smbus_irq);
	}
	/* bit1, kill current transaction. */
	sys_write8(SMB_KILL, cfg->host_base + SMB_HOCTLRn);
	sys_write8(0, cfg->host_base + SMB_HOCTLRn);

	/* W/C host status register */
	sys_write8(HOSTA_ALL_WC, cfg->host_base + SMB_HOSTARn);

	/* Keep host notify listening */
	if (!(data->dev_config & SMBUS_MODE_HOST_NOTIFY)) {
		sys_write8(0, cfg->host_base + SMB_HOCTL2Rn);
	}

	data->blk_active = false;
}

static inline void it51xxx_smb_xfer_reset(struct smbus_it51xxx_data *data, uint16_t addr,
					  uint8_t protocol)
{
	data->err = 0;
	data->xfer_addr = addr;
	data->xfer_protocol = protocol;
	data->blk_active = false;
	data->blk_is_write = false;
	data->g_r_buf = NULL;
	data->g_w_buf = NULL;
	data->blk_idx = 0;
	data->blk_len = 0;
	data->blk_rlen = NULL;
	data->pec_sw = false;
	data->pec_crc = 0;
	data->pec_recv = 0;
	k_sem_reset(&data->xfer_done);
}

static inline bool it51xxx_smb_bus_idle(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;

	/* Bus is busy */
	if (sys_read8(cfg->host_base + SMB_HOSTARn) & SMB_HOBY) {
		return false;
	}

	/* SCL or SDA is not high */
	if ((sys_read8(cfg->host_base + SMB_SMBPCTLRn) & SMB_LINE_IDLE) != SMB_LINE_IDLE) {
		return false;
	}

	return true;
}

static int it51xxx_smb_not_available(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	uint8_t status;

	status = sys_read8(cfg->host_base + SMB_HOSTARn);
	if (status & HOSTA_ALL_WC) {
		LOG_DBG("%s: Clearing HOSTA=0x%02x before transfer", dev->name, status);
		sys_write8(status & HOSTA_ALL_WC, cfg->host_base + SMB_HOSTARn);
	}

	if (!it51xxx_smb_bus_idle(dev)) {
		LOG_ERR("%s: SMBus busy or line not idle", dev->name);
		return -EIO;
	}

	return 0;
}

static inline void it51xxx_smb_fifo_enable(const struct device *dev, bool enable)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	uint8_t fifo_en;

	fifo_en = (cfg->port == SMB_CHANNEL_A) ? SMB_FF1EN : SMB_FF2EN;

	if (enable) {
		sys_write8(sys_read8(cfg->smbbase + SMB_MSTFCSTS) | fifo_en,
			   cfg->smbbase + SMB_MSTFCSTS);
	} else {
		sys_write8(sys_read8(cfg->smbbase + SMB_MSTFCSTS) & ~fifo_en,
			   cfg->smbbase + SMB_MSTFCSTS);
	}
}

static inline void it51xxx_smb_host_enable(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;

	/* Enable SMBus host controller (I2C_EN=0, SMD_EN=1) */
	sys_write8(sys_read8(cfg->host_base + SMB_HOCTL2Rn) | SMB_SMD_TO_EN | SMB_SMH_EN,
		   cfg->host_base + SMB_HOCTL2Rn);
}

static void it51xxx_smb_host_disable(const struct device *dev, uint8_t status)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	/* Keep host notify listening */
	if (!(data->dev_config & SMBUS_MODE_HOST_NOTIFY)) {
		irq_disable(cfg->smbus_irq);
		/* Disable SMBus host controller */
		sys_write8(0, cfg->host_base + SMB_HOCTL2Rn);
	}

	data->blk_active = false;
	/* W/C */
	sys_write8(status, cfg->host_base + SMB_HOSTARn);
}

static inline void it51xxx_smb_pec_setup(const struct device *dev, enum smbus_direction rw,
					 uint8_t protocol)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	if (!(data->dev_config & SMBUS_MODE_PEC) || protocol == SMBUS_CMD_QUICK) {
		/*
		 * No PEC for this transaction; clear any stale MAHPF from a previous
		 * PEC transaction
		 */
		data->pec_sw = false;
		sys_write8(SMB_MAHPCE, cfg->host_base + SMB_SPESRn);
		return;
	}

	if (it51xxx_smb_pec_sw_check(dev, rw, protocol)) {
		sys_write8(SMB_MAHPCE, cfg->host_base + SMB_SPESRn);
	} else {
		/* Supports HW PEC calculation */
		sys_write8(SMB_MAHPCE | SMB_MAHPF, cfg->host_base + SMB_SPESRn);
	}
}

static inline void it51xxx_smb_target_addr(const struct device *dev, uint16_t addr,
					   enum smbus_direction rw)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	uint8_t target_addr;

	target_addr = (uint8_t)((addr << 1) | (rw == SMBUS_MSG_READ ? SMB_DIR : 0));
	sys_write8(target_addr, cfg->host_base + SMB_TRASLAn);

	if (data->pec_sw) {
		/*
		 * FIFO Block Read: target_addr (=ADDR+R) is what's written to TRASLAn,
		 * but on the wire the command byte still goes out under ADDR+W first;
		 * ADDR+R only appears later via the hardware's own repeated START. PEC must
		 * be seeded with the actual first byte on the wire (ADDR+W), not target_addr.
		 */
		if (rw == SMBUS_MSG_READ) {
			target_addr &= ~SMB_DIR;
		}
		it51xxx_smb_pec_update(dev, &target_addr, 1);
	}
}

static void it51xxx_smb_write_cmd(const struct device *dev, enum smbus_direction rw, uint8_t cmd,
				  uint8_t protocol)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	/* Optional read/write command byte */
	switch (protocol) {
	case SMBUS_CMD_QUICK:
		/* No command byte */
		break;

	case SMBUS_CMD_BYTE:
		if (rw == SMBUS_MSG_WRITE) {
			/* Send Byte has a command byte */
			sys_write8(cmd, cfg->host_base + SMB_HOCMDRn);
		}
		/* Receive byte has not command byte */
		break;
	default:
		/* Byte data / Word data / Block command / Process call have command byte */
		sys_write8(cmd, cfg->host_base + SMB_HOCMDRn);
		break;
	}

	if (data->pec_sw) {
		it51xxx_smb_pec_update(dev, &cmd, 1);
	}
}

static void it51xxx_smb_write_data(const struct device *dev, uint8_t *buf, size_t count)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	LOG_DBG("%s: count=%u buf[0]=0x%02x", dev->name, count, buf[0]);

	sys_write8(buf[0], cfg->host_base + SMB_D0REGn);

	if (count > 1) {
		LOG_DBG("%s: buf[1]=0x%02x", dev->name, buf[1]);
		sys_write8(buf[1], cfg->host_base + SMB_D1REGn);
	}

	if (data->pec_sw) {
		it51xxx_smb_pec_update(dev, buf, count);
	}
}

static void it51xxx_smb_setup_block_xfer(const struct device *dev, enum smbus_direction rw,
					 uint8_t tx_cnt, uint8_t *tx_buf, uint8_t *rx_cnt,
					 uint8_t *rx_buf)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	data->blk_is_write = (rw == SMBUS_MSG_WRITE);

	/* FIFO mode */
	if (cfg->fifo_enable) {
		if (data->blk_is_write) {
			/* Byte count field, then the first data byte (HOBDB) */
			sys_write8(tx_cnt, cfg->host_base + SMB_D0REGn);

			/* Set host block data byte */
			for (uint8_t i = 0; i < tx_cnt; i++) {
				sys_write8(tx_buf[i], cfg->host_base + SMB_HOBDBRn);
			}
		}

		return;
	}

	/* PIO mode */
	data->blk_active = true;

	if (data->blk_is_write) {
		/* Byte count field, then the first data byte (HOBDB) */
		sys_write8(tx_cnt, cfg->host_base + SMB_D0REGn);

		sys_write8(tx_buf[0], cfg->host_base + SMB_HOBDBRn);

		/*
		 * tx_buf[0] is preloaded into HOBDB before START. The ISR pushes
		 * tx_buf[1..count - 1] on each subsequent BDS.
		 */
		data->blk_idx = 1;
		data->blk_len = tx_cnt;
		data->g_w_buf = tx_buf;
	} else {
		data->blk_idx = 0;
		data->blk_rlen = rx_cnt;
		data->g_r_buf = rx_buf;
	}
}

static inline int smbus_it51xxx_get_start_cmd(const struct device *dev, uint8_t protocol,
					      uint8_t *start_cmd)
{
	if (protocol >= ARRAY_SIZE(start_cmd_table)) {
		LOG_ERR("%s: Unsupported SMBus protocol %u", dev->name, protocol);
		return -EIO;
	}

	*start_cmd = start_cmd_table[protocol];

	return 0;
}

static int it51xxx_smb_set_protocol(const struct device *dev, uint8_t protocol)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	int ret;
	uint8_t start_cmd;

	ret = smbus_it51xxx_get_start_cmd(dev, protocol, &start_cmd);
	if (ret) {
		it51xxx_smb_host_disable(dev, 0);
		return ret;
	}

	if (data->dev_config & SMBUS_MODE_PEC && protocol != SMBUS_CMD_QUICK) {
		start_cmd |= SMB_PEC_EN;
	}

	LOG_DBG("%s: IRQ num=%u start_cmd=0x%02x", dev->name, cfg->smbus_irq, start_cmd);

	/* Enable SMBus interrupt */
	irq_enable(cfg->smbus_irq);

	sys_write8(start_cmd, cfg->host_base + SMB_HOCTLRn);

	return 0;
}

static int it51xxx_smb_parsing_err(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	if (!data->err) {
		return 0;
	}

	LOG_ERR("%s: port%u addr=0x%02x HOSTA error=0x%02x", dev->name, cfg->port, data->xfer_addr,
		data->err);

	if (data->err == ETIMEDOUT) {
		/* Connection timed out */
		LOG_ERR("Transaction time out");
	} else if (data->err == EINVAL) {
		/* PEC check error */
		LOG_ERR("PEC check error");
	} else {
		/* Host error bits message*/
		if (data->err & SMB_TMOE) {
			LOG_ERR("Hardware time-out error");
		}
		if (data->err & SMB_NACK) {
			LOG_DBG("NACK received");
		}
		if (data->err & SMB_FAIL) {
			LOG_ERR("Transaction killed");
		}
		if (data->err & SMB_BSER) {
			LOG_ERR("Lost arbitration.");
		}
		if (data->err & SMB_DVER) {
			LOG_ERR("Device error");
		}
	}

	return -EIO;
}

static int it51xxx_smb_wait_finish(const struct device *dev)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	ret = k_sem_take(&data->xfer_done, K_MSEC(IT51XXX_SMBUS_TIMEOUT_MS + 10));
	if (ret == -EAGAIN) {
		data->err = ETIMEDOUT;
		it51xxx_smb_reset(dev);
		LOG_ERR("%s: Transaction timed out", dev->name);

		return ret;
	}

	return 0;
}

static inline uint8_t it51xxx_smb_protocol_data_len(uint8_t protocol)
{
	switch (protocol) {
	case SMBUS_CMD_BYTE:
	case SMBUS_CMD_BYTE_DATA:
		return 1;
	case SMBUS_CMD_WORD_DATA:
	case SMBUS_CMD_PROC_CALL:
		return 2;
	default:
		return 0;
	}
}

static inline void it51xxx_smb_read_data(const struct device *dev, uint8_t *buf, uint8_t count)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	buf[0] = sys_read8(cfg->host_base + SMB_D0REGn);

	if (count > 1) {
		buf[1] = sys_read8(cfg->host_base + SMB_D1REGn);
	}

	if (data->pec_sw) {
		/* Repeated-start Read address byte, as it appears on the wire */
		uint8_t addr_r_byte = (uint8_t)((data->xfer_addr << 1) | SMB_DIR);

		it51xxx_smb_pec_update(dev, &addr_r_byte, 1);
		it51xxx_smb_pec_update(dev, buf, count);
		/*
		 * PEC data is loaded from the SMBus into this register and is then
		 * read by software
		 */
		data->pec_recv = sys_read8(cfg->host_base + SMB_PECERCRn);
	}
}

static inline void it51xxx_smb_post_read(const struct device *dev, uint8_t *rx_buf,
					 uint8_t protocol)
{
	uint8_t data_len = it51xxx_smb_protocol_data_len(protocol);

	if (data_len > 0) {
		it51xxx_smb_read_data(dev, rx_buf, data_len);
	}
}

static int it51xxx_smb_fifo_block_read_data(const struct device *dev, uint8_t *rx_buf,
					    uint8_t *rx_cnt)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	uint8_t byte_count;

	byte_count = sys_read8(cfg->host_base + SMB_D0REGn);

	if (byte_count == 0 || byte_count > SMBUS_BLOCK_BYTES_MAX) {
		LOG_ERR("%s: FIFO block read invalid byte count %u", dev->name, byte_count);
		data->err = SMB_FAIL;
		return -EIO;
	}

	*rx_cnt = byte_count;

	/* HOBDB acts as a FIFO output port while dedicated FIFO mode is enabled */
	for (uint8_t i = 0; i < byte_count; i++) {
		rx_buf[i] = sys_read8(cfg->host_base + SMB_HOBDBRn);
	}

	if (data->pec_sw) {
		/* Repeated-start Read address byte, as it appears on the wire */
		uint8_t addr_r_byte = (uint8_t)((data->xfer_addr << 1) | SMB_DIR);

		it51xxx_smb_pec_update(dev, &addr_r_byte, 1);
		it51xxx_smb_pec_update(dev, &byte_count, 1);
		it51xxx_smb_pec_update(dev, rx_buf, byte_count);
		/*
		 * PEC data is loaded from the SMBus into this register and is then
		 * read by software
		 */
		data->pec_recv = sys_read8(cfg->host_base + SMB_PECERCRn);
	}

	return 0;
}

static void it51xxx_smb_pec_verify(const struct device *dev, uint8_t protocol)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	/* Only if this transaction actually requested PEC */
	if (!(data->dev_config & SMBUS_MODE_PEC) || protocol == SMBUS_CMD_QUICK) {
		return;
	}

	if (!data->pec_sw) {
		uint8_t spesr = sys_read8(cfg->host_base + SMB_SPESRn);

		/* HW PEC check */
		if (spesr & SMB_MAHPCE) {
			/* W/C the error bit */
			sys_write8(SMB_MAHPCE, cfg->host_base + SMB_SPESRn);
			data->err = EINVAL;
		}
	} else {
		/* SW PEC check */
		if (data->pec_crc != data->pec_recv) {
			data->err = EINVAL;
		}
	}
}

static int it51xxx_smbus_xfer(const struct device *dev, uint16_t periph_addr,
			      enum smbus_direction rw, uint8_t cmd, uint8_t tx_cnt, uint8_t *tx_buf,
			      uint8_t *rx_cnt, uint8_t *rx_buf, uint8_t protocol)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	it51xxx_smb_xfer_reset(data, periph_addr, protocol);

	/* Ensure bus idle before starting a new transaction */
	ret = it51xxx_smb_not_available(dev);
	if (ret) {
		return ret;
	}

	/* Enable FIFO mode */
	if (cfg->fifo_enable && protocol == SMBUS_CMD_BLOCK) {
		it51xxx_smb_fifo_enable(dev, true);
	}

	/* Enable SMBus host interface */
	it51xxx_smb_host_enable(dev);

	/*
	 * Must run before it51xxx_smb_target_addr(): decides HW vs SW PEC
	 * for this transaction, and SW-PEC CRC accumulation starts there.
	 */
	it51xxx_smb_pec_setup(dev, rw, protocol);

	/* Target address + R/W bit */
	it51xxx_smb_target_addr(dev, periph_addr, rw);

	/* Set host command register */
	it51xxx_smb_write_cmd(dev, rw, cmd, protocol);

	if (protocol == SMBUS_CMD_BLOCK) {
		/* SMB_BLOCK_CMD */
		it51xxx_smb_setup_block_xfer(dev, rw, tx_cnt, tx_buf, rx_cnt, rx_buf);
	} else if (tx_buf != NULL && tx_cnt > 0) {
		/* Command byte / Byte data / Word data / Process call */
		it51xxx_smb_write_data(dev, tx_buf, tx_cnt);
	}

	/* Start transaction with selected protocol */
	ret = it51xxx_smb_set_protocol(dev, protocol);
	if (ret) {
		return ret;
	}

	/*
	 * Wait for either FINTR (transaction complete) or any error bit.
	 * Returns 0 on success, negative errno on error or timeout.
	 */
	ret = it51xxx_smb_wait_finish(dev);
	if (ret) {
		goto done;
	}

	LOG_DBG("%s: Complete addr=0x%02x protocol=0x%02x", dev->name, periph_addr, protocol);

	/* Command byte / Byte data / Word data / Process call have read data */
	if (rx_buf != NULL) {
		it51xxx_smb_post_read(dev, rx_buf, protocol);
	}

	if (cfg->fifo_enable && protocol == SMBUS_CMD_BLOCK && rw == SMBUS_MSG_READ) {
		ret = it51xxx_smb_fifo_block_read_data(dev, rx_buf, rx_cnt);
		if (ret) {
			goto done;
		}
	}

	/* PEC check */
	it51xxx_smb_pec_verify(dev, protocol);
done:
	if (cfg->fifo_enable && protocol == SMBUS_CMD_BLOCK) {
		it51xxx_smb_fifo_enable(dev, false);
	}

	return it51xxx_smb_parsing_err(dev);
}

static int it51xxx_smbus_set_port_frequency(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	uint8_t msclkts, freq_set;

	switch (cfg->bitrate) {
	case IT51XXX_SMBUS_BITRATE_50K:
		freq_set = MSCLKTS_SCLKS(SMB_CLKS_50K);
		break;

	case I2C_BITRATE_STANDARD:
		freq_set = MSCLKTS_SCLKS(SMB_CLKS_100K);
		break;

	case I2C_BITRATE_FAST:
		freq_set = MSCLKTS_SCLKS(SMB_CLKS_400K);
		break;

	case I2C_BITRATE_FAST_PLUS:
		freq_set = MSCLKTS_SCLKS(SMB_CLKS_1M);
		break;

	default:
		LOG_ERR("%s: Unsupported SMBus bitrate: %u", dev->name, cfg->bitrate);
		return -EIO;
	}

	msclkts = sys_read8(cfg->host_base + SMB_MSCLKTSn);
	msclkts &= ~GENMASK(2, 0);
	sys_write8(msclkts | freq_set, cfg->host_base + SMB_MSCLKTSn);

	return 0;
}

static int smbus_it51xxx_configure(const struct device *dev, uint32_t config_value)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	LOG_DBG("%s: port%u config=0x%02x", dev->name, cfg->port, config_value);

	if ((config_value & SMBUS_MODE_CONTROLLER) == 0) {
		LOG_ERR("%s: Only controller mode is supported", dev->name);
		return -EIO;
	}

	if (config_value & SMBUS_MODE_PEC) {
		LOG_INF("%s: PEC enabled", dev->name);
	}

	if (config_value & SMBUS_MODE_HOST_NOTIFY) {
		if (cfg->port > IT51XXX_SMBUS_HOST_NOTIFY_MAX_PORT) {
			LOG_ERR("%s: Host Notify only available on host A/B/C", dev->name);
			return -EIO;
		}

		/* Enable Host Notify command reception + its interrupt source */
		sys_write8(sys_read8(cfg->host_base + SMB_HOCTL2Rn) | SMB_HTIFYEN | SMB_SMH_EN,
			   cfg->host_base + SMB_HOCTL2Rn);
		sys_write8(sys_read8(cfg->host_base + SMB_I2CW2RFn) | SMB_HONOIN,
			   cfg->host_base + SMB_I2CW2RFn);
	}

	if (config_value & SMBUS_MODE_SMBALERT) {
		LOG_ERR("%s: SMBALERT# is not support", dev->name);
		return -EIO;
	}

	data->dev_config = config_value;

	ret = it51xxx_smbus_set_port_frequency(dev);
	if (ret) {
		return ret;
	}

	if (config_value & SMBUS_MODE_HOST_NOTIFY) {
		irq_enable(cfg->smbus_irq);
	}

	return 0;
}

static int smbus_it51xxx_get_config(const struct device *dev, uint32_t *config)
{
	struct smbus_it51xxx_data *data = dev->data;

	if (config == NULL) {
		return -EIO;
	}

	*config = data->dev_config;

	return 0;
}

static int smbus_it51xxx_quick(const struct device *dev, uint16_t periph_addr,
			       enum smbus_direction rw)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	LOG_DBG("%s: addr=0x%02x rw=%d", dev->name, periph_addr, rw);

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, rw, 0, 0, NULL, NULL, NULL, SMBUS_CMD_QUICK);

	k_sem_give(&data->bus_lock);

	return ret;
}

static int smbus_it51xxx_byte_write(const struct device *dev, uint16_t periph_addr, uint8_t byte)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	LOG_DBG("%s: addr=0x%02x byte=0x%02x", dev->name, periph_addr, byte);

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_WRITE, byte, 0, NULL, NULL, NULL,
				 SMBUS_CMD_BYTE);

	k_sem_give(&data->bus_lock);

	return ret;
}

static int smbus_it51xxx_byte_read(const struct device *dev, uint16_t periph_addr, uint8_t *byte)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	if (byte == NULL) {
		return -EIO;
	}

	LOG_DBG("%s: addr=0x%02x", dev->name, periph_addr);

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_READ, 0, 0, NULL, NULL, byte,
				 SMBUS_CMD_BYTE);

	k_sem_give(&data->bus_lock);

	return ret;
}

static int smbus_it51xxx_byte_data_write(const struct device *dev, uint16_t periph_addr,
					 uint8_t command, uint8_t byte)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	LOG_DBG("%s: addr=0x%02x cmd=0x%02x", dev->name, periph_addr, command);

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_WRITE, command, sizeof(byte), &byte,
				 NULL, NULL, SMBUS_CMD_BYTE_DATA);

	k_sem_give(&data->bus_lock);

	return ret;
}

static int smbus_it51xxx_byte_data_read(const struct device *dev, uint16_t periph_addr,
					uint8_t command, uint8_t *byte)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	LOG_DBG("%s: addr=0x%02x cmd=0x%02x", dev->name, periph_addr, command);

	if (byte == NULL) {
		return -EIO;
	}

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_READ, command, 0, NULL, NULL, byte,
				 SMBUS_CMD_BYTE_DATA);

	k_sem_give(&data->bus_lock);

	return ret;
}

static int smbus_it51xxx_word_data_write(const struct device *dev, uint16_t periph_addr,
					 uint8_t command, uint16_t word)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;
	uint8_t buf[2];

	LOG_DBG("%s: addr=0x%02x cmd=0x%02x", dev->name, periph_addr, command);

	sys_put_le16(word, buf);

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_WRITE, command, sizeof(buf), buf, NULL,
				 NULL, SMBUS_CMD_WORD_DATA);

	k_sem_give(&data->bus_lock);

	return ret;
}

static int smbus_it51xxx_word_data_read(const struct device *dev, uint16_t periph_addr,
					uint8_t command, uint16_t *word)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;
	uint8_t r_buf[2];

	LOG_DBG("%s: addr=0x%02x cmd=0x%02x", dev->name, periph_addr, command);

	if (word == NULL) {
		return -EIO;
	}

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_READ, command, 0, NULL, NULL, r_buf,
				 SMBUS_CMD_WORD_DATA);

	k_sem_give(&data->bus_lock);

	if (ret) {
		return ret;
	}

	*word = sys_get_le16(r_buf);

	return 0;
}

static int smbus_it51xxx_pcall(const struct device *dev, uint16_t periph_addr, uint8_t command,
			       uint16_t send_word, uint16_t *recv_word)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;
	uint8_t w_buf[2], r_buf[2];

	if (recv_word == NULL) {
		return -EIO;
	}

	LOG_DBG("%s: addr=0x%02x cmd=0x%02x", dev->name, periph_addr, command);

	sys_put_le16(send_word, w_buf);

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_WRITE, command, sizeof(w_buf), w_buf,
				 NULL, r_buf, SMBUS_CMD_PROC_CALL);

	k_sem_give(&data->bus_lock);

	if (ret) {
		return ret;
	}

	*recv_word = sys_get_le16(r_buf);

	return 0;
}

static int smbus_it51xxx_block_write(const struct device *dev, uint16_t periph_addr,
				     uint8_t command, uint8_t count, uint8_t *buf)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	LOG_DBG("%s: addr=0x%02x cmd=0x%02x count=%u", dev->name, periph_addr, command, count);

	if (buf == NULL || count == 0 || count > SMBUS_BLOCK_BYTES_MAX) {
		return -EIO;
	}

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_WRITE, command, count, buf, NULL, NULL,
				 SMBUS_CMD_BLOCK);

	k_sem_give(&data->bus_lock);

	return ret;
}

static int smbus_it51xxx_block_read(const struct device *dev, uint16_t periph_addr, uint8_t command,
				    uint8_t *count, uint8_t *buf)
{
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	LOG_DBG("%s: addr=0x%02x cmd=0x%02x", dev->name, periph_addr, command);

	if (count == NULL || buf == NULL) {
		return -EIO;
	}

	k_sem_take(&data->bus_lock, K_FOREVER);

	ret = it51xxx_smbus_xfer(dev, periph_addr, SMBUS_MSG_READ, command, 0, NULL, count, buf,
				 SMBUS_CMD_BLOCK);

	k_sem_give(&data->bus_lock);

	return ret;
}

static void it51xxx_smb_isr_block_write(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	LOG_DBG("%s: Block write byte done idx=%u len=%u", dev->name, data->blk_idx, data->blk_len);

	if (data->blk_idx < data->blk_len) {
		sys_write8(data->g_w_buf[data->blk_idx], cfg->host_base + SMB_HOBDBRn);
		data->blk_idx++;
	}

	/*
	 * W/C BDS: releases the clock stretch so hardware can send the ACK/NACK and
	 * clock out the next byte (or stop).
	 */
	sys_write8(SMB_BDS, cfg->host_base + SMB_HOSTARn);
}

static void it51xxx_smb_isr_block_read(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	uint8_t byte_count;

	if (data->blk_idx == 0) {
		byte_count = sys_read8(cfg->host_base + SMB_D0REGn);
		*data->blk_rlen = byte_count;

		LOG_DBG("%s: Block read data len=%u", dev->name, *data->blk_rlen);
		if (byte_count == 0 || byte_count > SMBUS_BLOCK_BYTES_MAX) {
			LOG_ERR("%s: Block read invalid byte count %u", dev->name, byte_count);
			data->err = SMB_FAIL;
			it51xxx_smb_reset(dev);
			k_sem_give(&data->xfer_done);
			return;
		}
	}

	/*
	 * LABY bit must be set before the actual last byte arrives, so hardware knows the
	 * incoming byte is the final one. That means we have to set it one byte early:
	 * blk_rlen == 1: no need to set the last byte.
	 * blk_idx == blk_rlen - 2: normal case, we're currently on the second-to-last byte,
	 * meaning the next one (blk_idx + 1, i.e. blk_rlen - 1) will be the last byte.
	 */
	if (*data->blk_rlen > 1 && (data->blk_idx == *data->blk_rlen - 2)) {
		sys_write8(sys_read8(cfg->host_base + SMB_HOCTLRn) | SMB_LABY,
			   cfg->host_base + SMB_HOCTLRn);
	}

	data->g_r_buf[data->blk_idx] = sys_read8(cfg->host_base + SMB_HOBDBRn);
	data->blk_idx++;
	/* W/C BDS */
	sys_write8(SMB_BDS, cfg->host_base + SMB_HOSTARn);
}

static void it51xxx_smb_isr_byte_done(const struct device *dev)
{
	struct smbus_it51xxx_data *data = dev->data;

	if (data->xfer_protocol != SMBUS_CMD_BLOCK) {
		return;
	}

	if (data->blk_is_write) {
		it51xxx_smb_isr_block_write(dev);
	} else {
		it51xxx_smb_isr_block_read(dev);
	}
}

static int smbus_it51xxx_host_notify_set_cb(const struct device *dev, struct smbus_callback *cb)
{
	struct smbus_it51xxx_data *data = dev->data;

	LOG_DBG("%s: Host notify set: dev %p cb %p", dev->name, dev, cb);

	return smbus_callback_set(&data->host_notify_cbs, cb);
}

static int smbus_it51xxx_host_notify_remove_cb(const struct device *dev, struct smbus_callback *cb)
{
	struct smbus_it51xxx_data *data = dev->data;

	LOG_DBG("%s: Host notify remove: dev %p cb %p", dev->name, dev, cb);

	return smbus_callback_remove(&data->host_notify_cbs, cb);
}

static void it51xxx_smb_host_notify_work(struct k_work *work)
{
	struct smbus_it51xxx_data *data =
		CONTAINER_OF(work, struct smbus_it51xxx_data, host_notify_work);
	const struct device *dev = data->dev;

	LOG_DBG("%s: Host notify callback addr=0x%02x", dev->name, data->notify_addr);

	smbus_fire_callbacks(&data->host_notify_cbs, dev, data->notify_addr);
}

static void it51xxx_smb_host_notify_handle(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;

	/* NDADR[7:1] = 7-bit peripheral address, bit0 reserved. */
	data->notify_addr = sys_read8(cfg->host_base + SMB_NDADRn) >> 1;
	data->notify_data = sys_read8(cfg->host_base + SMB_NDLBn);
	data->notify_data |= (uint16_t)sys_read8(cfg->host_base + SMB_NDHBn) << 8;

	LOG_DBG("%s: Host notify from addr=0x%02x data=0x%04x", dev->name, data->notify_addr,
		data->notify_data);

	k_work_submit(&data->host_notify_work);
}

static void smbus_it51xxx_isr(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	uint8_t status;

	status = sys_read8(cfg->host_base + SMB_HOSTARn);

	/* Any bus/device error ends the transaction */
	if (status & HOSTA_ANY_ERROR) {
		data->err = status & HOSTA_ANY_ERROR;
		goto done;
	}

	if (data->dev_config & SMBUS_MODE_HOST_NOTIFY) {
		uint8_t ndadr;

		ndadr = sys_read8(cfg->host_base + SMB_NDADRn);
		if (ndadr & SMB_HONOST) {
			it51xxx_smb_host_notify_handle(dev);
			sys_write8(SMB_HONOST, cfg->host_base + SMB_NDADRn);
			return;
		}
	}

	/* Byte done status */
	if (status & SMB_BDS && data->blk_active) {
		it51xxx_smb_isr_byte_done(dev);
		return;
	}

	if (!(status & SMB_FINTR)) {
		return;
	}

	/* Finish Interrupt: stop condition detected, transaction is done */
done:
	it51xxx_smb_host_disable(dev, status);
	/* Wake the waiting thread */
	k_sem_give(&data->xfer_done);
}

static int smbus_it51xxx_init(const struct device *dev)
{
	const struct smbus_it51xxx_config *cfg = dev->config;
	struct smbus_it51xxx_data *data = dev->data;
	int ret;

	it51xxx_smb_reset(dev);

	/* Initialize bus lock (mutex) and transfer-done semaphore */
	k_sem_init(&data->bus_lock, 1, 1);
	k_sem_init(&data->xfer_done, 0, 1);

	/* Initialize work structures */
	data->dev = dev;
	sys_slist_init(&data->host_notify_cbs);
	k_work_init(&data->host_notify_work, it51xxx_smb_host_notify_work);

	cfg->irq_config_func(dev);

	ret = smbus_it51xxx_configure(dev, SMBUS_MODE_CONTROLLER);
	if (ret) {
		LOG_ERR("%s: Cannot set default configuration", dev->name);
		return ret;
	}

	/* This field defines the low timeout for design A-I clocks */
	sys_write8(IT51XXX_SMBUS_TIMEOUT_MS, cfg->host_base + SMB_25MSREGn);

	/* Set the pin to SMBus alternate function. */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("%s: Failed to configure SMbus pins", dev->name);
		return ret;
	}

	/* Select which port to use FIFO2 except port A */
	if ((cfg->port != SMB_CHANNEL_A) && cfg->fifo_enable) {
		sys_write8(sys_read8(cfg->smbbase + SMB_MSTFCSTS) | SMB_FFCHSEL2(cfg->port - 1),
			   cfg->smbbase + SMB_MSTFCSTS);
	}

	return 0;
}

static DEVICE_API(smbus, smbus_it51xxx_api) = {
	.configure = smbus_it51xxx_configure,
	.get_config = smbus_it51xxx_get_config,
	.smbus_quick = smbus_it51xxx_quick,
	.smbus_byte_write = smbus_it51xxx_byte_write,
	.smbus_byte_read = smbus_it51xxx_byte_read,
	.smbus_byte_data_write = smbus_it51xxx_byte_data_write,
	.smbus_byte_data_read = smbus_it51xxx_byte_data_read,
	.smbus_word_data_write = smbus_it51xxx_word_data_write,
	.smbus_word_data_read = smbus_it51xxx_word_data_read,
	.smbus_pcall = smbus_it51xxx_pcall,
	.smbus_block_write = smbus_it51xxx_block_write,
	.smbus_block_read = smbus_it51xxx_block_read,

	.smbus_host_notify_set_cb = smbus_it51xxx_host_notify_set_cb,
	.smbus_host_notify_remove_cb = smbus_it51xxx_host_notify_remove_cb,
};

#define IT51XXX_I2C_COMPAT ite_it51xxx_i2c

/* Ensure no enabled I2C instance uses the same port as this SMBus instance. */
#define SMBUS_IT51XXX_I2C_PORT_CONFLICT_CHECK(i2c_node_id, smbus_inst)                             \
	BUILD_ASSERT(DT_REG_ADDR(i2c_node_id) != DT_INST_REG_ADDR(smbus_inst),                     \
		     "it51xxx: an I2C and SMBus instance cannot use the same port");

#define SMBUS_IT51XXX_CHECK_NOT_ALSO_I2C(smbus_inst)                                               \
	DT_FOREACH_STATUS_OKAY_VARGS(IT51XXX_I2C_COMPAT, SMBUS_IT51XXX_I2C_PORT_CONFLICT_CHECK,    \
				     smbus_inst)

#define IT51XXX_FIFO_ENABLE_ADD_NON_A(node_id)                                                     \
	+((DT_PROP(node_id, port_num) != SMB_CHANNEL_A) ? DT_PROP(node_id, fifo_enable) : 0)

#define IT51XXX_SMBUS_FIFO_ENABLE_COUNT                                                            \
	(0 DT_FOREACH_STATUS_OKAY(DT_DRV_COMPAT, IT51XXX_FIFO_ENABLE_ADD_NON_A))

#define IT51XXX_I2C_FIFO_ENABLE_COUNT                                                              \
	(0 DT_FOREACH_STATUS_OKAY(IT51XXX_I2C_COMPAT, IT51XXX_FIFO_ENABLE_ADD_NON_A))

BUILD_ASSERT((IT51XXX_SMBUS_FIFO_ENABLE_COUNT + IT51XXX_I2C_FIFO_ENABLE_COUNT) <= 1,
	     "it51xxx: at most one SMBus/I2C port (smbus1-8, i2c1-8) may enable fifo mode");

#define SMBUS_IT51XXX_DEVICE_INIT(inst)                                                            \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	SMBUS_IT51XXX_CHECK_NOT_ALSO_I2C(inst)                                                     \
	static void smbus_it51xxx_irq_config_##inst(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), 0, smbus_it51xxx_isr, DEVICE_DT_INST_GET(inst),    \
			    0);                                                                    \
	};                                                                                         \
	static struct smbus_it51xxx_config smbus_it51xxx_config_##inst = {                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.host_base = DT_INST_REG_ADDR(inst),                                               \
		.smbbase = DT_REG_ADDR_BY_IDX(DT_NODELABEL(i2cbase), 0),                           \
		.smbus_irq = DT_INST_IRQN(inst),                                                   \
		.bitrate = DT_INST_PROP(inst, clock_frequency),                                    \
		.port = DT_INST_PROP(inst, port_num),                                              \
		.irq_config_func = smbus_it51xxx_irq_config_##inst,                                \
		.fifo_enable = DT_INST_PROP(inst, fifo_enable),                                    \
	};                                                                                         \
                                                                                                   \
	static struct smbus_it51xxx_data smbus_it51xxx_data_##inst;                                \
                                                                                                   \
	SMBUS_DEVICE_DT_INST_DEFINE(inst, smbus_it51xxx_init, NULL, &smbus_it51xxx_data_##inst,    \
				    &smbus_it51xxx_config_##inst, POST_KERNEL,                     \
				    CONFIG_SMBUS_INIT_PRIORITY, &smbus_it51xxx_api);

DT_INST_FOREACH_STATUS_OKAY(SMBUS_IT51XXX_DEVICE_INIT)
