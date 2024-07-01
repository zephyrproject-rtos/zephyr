/*
 * Copyright 2020 Broadcom
 * Copyright 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_iproc_i2c

#include <errno.h>
#include <stdint.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
LOG_MODULE_REGISTER(iproc_i2c);

#include "i2c-priv.h"

/* Registers */
#define CFG_OFFSET            0x00
#define CFG_RESET_SHIFT       31
#define CFG_EN_SHIFT          30
#define CFG_M_RETRY_CNT_SHIFT 16
#define CFG_M_RETRY_CNT_MASK  0x0f

#define TIM_CFG_OFFSET                0x04
#define TIM_CFG_MODE_400_SHIFT        31
#define TIM_RAND_TARGET_STRETCH_SHIFT 24
#define TIM_RAND_TARGET_STRETCH_MASK  0x7f

#define S_ADDR_OFFSET              0x08
#define S_ADDR_OFFSET_ADDR0_MASK   0x7f
#define S_ADDR_OFFSET_ADDR0_SHIFT  0
#define S_ADDR_OFFSET_ADDR0_EN_BIT 7

#define M_FIFO_CTRL_OFFSET    0x0c
#define M_FIFO_RX_FLUSH_SHIFT 31
#define M_FIFO_TX_FLUSH_SHIFT 30
#define M_FIFO_RX_CNT_SHIFT   16
#define M_FIFO_RX_CNT_MASK    0x7f
#define M_FIFO_RX_THLD_SHIFT  8
#define M_FIFO_RX_THLD_MASK   0x3f

#define S_FIFO_CTRL_OFFSET    0x10
#define S_FIFO_RX_FLUSH_SHIFT 31
#define S_FIFO_TX_FLUSH_SHIFT 30

#define M_CMD_OFFSET               0x30
#define M_CMD_START_BUSY_SHIFT     31
#define M_CMD_STATUS_SHIFT         25
#define M_CMD_STATUS_MASK          0x07
#define M_CMD_STATUS_SUCCESS       0x0
#define M_CMD_STATUS_LOST_ARB      0x1
#define M_CMD_STATUS_NACK_ADDR     0x2
#define M_CMD_STATUS_NACK_DATA     0x3
#define M_CMD_STATUS_TIMEOUT       0x4
#define M_CMD_STATUS_FIFO_UNDERRUN 0x5
#define M_CMD_STATUS_RX_FIFO_FULL  0x6
#define M_CMD_SMB_PROT_SHIFT       9
#define M_CMD_SMB_PROT_QUICK       0x0
#define M_CMD_SMB_PROT_MASK        0xf
#define M_CMD_SMB_PROT_BLK_WR      0x7
#define M_CMD_SMB_PROT_BLK_RD      0x8
#define M_CMD_PEC_SHIFT            8
#define M_CMD_RD_CNT_MASK          0xff

#define S_CMD_OFFSET              0x34
#define S_CMD_START_BUSY_SHIFT    31
#define S_CMD_STATUS_SHIFT        23
#define S_CMD_STATUS_MASK         0x07
#define S_CMD_STATUS_TIMEOUT      0x5
#define S_CMD_STATUS_MASTER_ABORT 0x7

#define IE_OFFSET               0x38
#define IE_M_RX_FIFO_FULL_SHIFT 31
#define IE_M_RX_THLD_SHIFT      30
#define IE_M_START_BUSY_SHIFT   28
#define IE_M_TX_UNDERRUN_SHIFT  27
#define IE_S_RX_FIFO_FULL_SHIFT 26
#define IE_S_RX_THLD_SHIFT      25
#define IE_S_RX_EVENT_SHIFT     24
#define IE_S_START_BUSY_SHIFT   23
#define IE_S_TX_UNDERRUN_SHIFT  22
#define IE_S_RD_EN_SHIFT        21

#define IS_OFFSET               0x3c
#define IS_M_RX_FIFO_FULL_SHIFT 31
#define IS_M_RX_THLD_SHIFT      30
#define IS_M_START_BUSY_SHIFT   28
#define IS_M_TX_UNDERRUN_SHIFT  27
#define IS_S_RX_FIFO_FULL_SHIFT 26
#define IS_S_RX_THLD_SHIFT      25
#define IS_S_RX_EVENT_SHIFT     24
#define IS_S_START_BUSY_SHIFT   23
#define IS_S_TX_UNDERRUN_SHIFT  22
#define IS_S_RD_EN_SHIFT        21

#define M_TX_OFFSET          0x40
#define M_TX_WR_STATUS_SHIFT 31
#define M_TX_DATA_MASK       0xff

#define M_RX_OFFSET        0x44
#define M_RX_STATUS_SHIFT  30
#define M_RX_STATUS_MASK   0x03
#define M_RX_PEC_ERR_SHIFT 29
#define M_RX_DATA_SHIFT    0
#define M_RX_DATA_MASK     0xff

#define S_TX_OFFSET          0x48
#define S_TX_WR_STATUS_SHIFT 31

#define S_RX_OFFSET       0x4c
#define S_RX_STATUS_SHIFT 30
#define S_RX_STATUS_MASK  0x03
#define S_RX_DATA_SHIFT   0x0
#define S_RX_DATA_MASK    0xff

#define I2C_TIMEOUT_MSEC         100
#define TX_RX_FIFO_SIZE          64
#define M_RX_FIFO_MAX_THLD_VALUE (TX_RX_FIFO_SIZE - 1)
#define M_RX_FIFO_THLD_VALUE     50

#define I2C_MAX_TARGET_ADDR 0x7f

#define I2C_TARGET_RX_FIFO_EMPTY 0x0
#define I2C_TARGET_RX_START      0x1
#define I2C_TARGET_RX_DATA       0x2
#define I2C_TARGET_RX_END        0x3

#define IE_S_ALL_INTERRUPT_SHIFT 21
#define IE_S_ALL_INTERRUPT_MASK  0x3f

#define TARGET_CLOCK_STRETCH_TIME 25

/*
 * To keep running in ISR for less time,
 * max target read per interrupt is set to 10 bytes.
 */
#define MAX_TARGET_RX_PER_INT 10

#define ISR_MASK_TARGET                                                                            \
	(BIT(IS_S_START_BUSY_SHIFT) | BIT(IS_S_RX_EVENT_SHIFT) | BIT(IS_S_RD_EN_SHIFT) |           \
	 BIT(IS_S_TX_UNDERRUN_SHIFT) | BIT(IS_S_RX_FIFO_FULL_SHIFT) | BIT(IS_S_RX_THLD_SHIFT))

#define ISR_MASK                                                                                   \
	(BIT(IS_M_START_BUSY_SHIFT) | BIT(IS_M_TX_UNDERRUN_SHIFT) | BIT(IS_M_RX_THLD_SHIFT))

#define DEV_CFG(dev)  ((struct iproc_i2c_config *)(dev)->config)
#define DEV_DATA(dev) ((struct iproc_i2c_data *)(dev)->data)
#define DEV_BASE(dev) ((DEV_CFG(dev))->base)

struct iproc_i2c_config {
	mem_addr_t base;
	uint32_t bitrate;
	void (*irq_config_func)(const struct device *dev);
};

struct iproc_i2c_data {
	struct i2c_target_config *target_cfg;
	struct i2c_msg *msg;
	uint32_t tx_bytes;
	uint32_t rx_bytes;
	uint32_t thld_bytes;
	uint32_t tx_underrun;
	struct k_sem device_sync_sem;
	uint32_t target_int_mask;
	bool rx_start_rcvd;
	bool target_read_complete;
	bool target_rx_only;
};

static void iproc_i2c_enable_disable(const struct device *dev, bool enable)
{
	mem_addr_t base = DEV_BASE(dev);
	uint32_t val;

	val = sys_read32(base + CFG_OFFSET);
	if (enable) {
		val |= BIT(CFG_EN_SHIFT);
	} else {
		val &= ~BIT(CFG_EN_SHIFT);
	}
	sys_write32(val, base + CFG_OFFSET);
}

static void iproc_i2c_reset_controller(const struct device *dev)
{
	mem_addr_t base = DEV_BASE(dev);
	uint32_t val;

	/* put controller in reset */
	val = sys_read32(base + CFG_OFFSET);
	val |= BIT(CFG_RESET_SHIFT);
	val &= ~BIT(CFG_EN_SHIFT);
	sys_write32(val, base + CFG_OFFSET);

	k_busy_wait(100);

	/* bring controller out of reset */
	sys_clear_bit(base + CFG_OFFSET, CFG_RESET_SHIFT);
}

#ifdef CONFIG_I2C_TARGET
/* Set target addr */
static int iproc_i2c_target_set_address(const struct device *dev, uint16_t addr)
{
	mem_addr_t base = DEV_BASE(dev);
	uint32_t val;

	if ((addr == 0) && (addr > I2C_MAX_TARGET_ADDR)) {
		LOG_ERR("Invalid target address(0x%x) received", addr);
		return -EINVAL;
	}

	addr = ((addr & S_ADDR_OFFSET_ADDR0_MASK) | BIT(S_ADDR_OFFSET_ADDR0_EN_BIT));
	val = sys_read32(base + S_ADDR_OFFSET);
	val &= ~(S_ADDR_OFFSET_ADDR0_MASK | BIT(S_ADDR_OFFSET_ADDR0_EN_BIT));
	val |= addr;
	sys_write32(val, base + S_ADDR_OFFSET);

	return 0;
}

static int iproc_i2c_target_init(const struct device *dev, bool need_reset)
{
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	mem_addr_t base = DEV_BASE(dev);
	struct i2c_target_config *target_config = dd->target_cfg;
	uint32_t val;
	int ret;

	if (need_reset) {
		iproc_i2c_reset_controller(dev);
	}

	/* flush target TX/RX FIFOs */
	val = BIT(S_FIFO_RX_FLUSH_SHIFT) | BIT(S_FIFO_TX_FLUSH_SHIFT);
	sys_write32(val, base + S_FIFO_CTRL_OFFSET);

	/* Maximum target stretch time */
	val = sys_read32(base + TIM_CFG_OFFSET);
	val &= ~(TIM_RAND_TARGET_STRETCH_MASK << TIM_RAND_TARGET_STRETCH_SHIFT);
	val |= (TARGET_CLOCK_STRETCH_TIME << TIM_RAND_TARGET_STRETCH_SHIFT);
	sys_write32(val, base + TIM_CFG_OFFSET);

	/* Set target address */
	ret = iproc_i2c_target_set_address(dev, target_config->address);
	if (ret) {
		return ret;
	}

	/* clear all pending target interrupts */
	sys_write32(ISR_MASK_TARGET, base + IS_OFFSET);

	/* Enable interrupt register to indicate a valid byte in receive fifo */
	val = BIT(IE_S_RX_EVENT_SHIFT);
	/* Enable interrupt register to indicate target Rx FIFO Full */
	val |= BIT(IE_S_RX_FIFO_FULL_SHIFT);
	/* Enable interrupt register to indicate a Master read transaction */
	val |= BIT(IE_S_RD_EN_SHIFT);
	/* Enable interrupt register for the target BUSY command */
	val |= BIT(IE_S_START_BUSY_SHIFT);
	dd->target_int_mask = val;
	sys_write32(val, base + IE_OFFSET);

	return ret;
}

static int iproc_i2c_check_target_status(const struct device *dev)
{
	mem_addr_t base = DEV_BASE(dev);
	uint32_t val;

	val = sys_read32(base + S_CMD_OFFSET);
	/* status is valid only when START_BUSY is cleared after it was set */
	if (val & BIT(S_CMD_START_BUSY_SHIFT)) {
		return -EBUSY;
	}

	val = (val >> S_CMD_STATUS_SHIFT) & S_CMD_STATUS_MASK;
	if ((val == S_CMD_STATUS_TIMEOUT) || (val == S_CMD_STATUS_MASTER_ABORT)) {
		if (val == S_CMD_STATUS_TIMEOUT) {
			LOG_ERR("target random stretch time timeout");
		} else if (val == S_CMD_STATUS_MASTER_ABORT) {
			LOG_ERR("Master aborted read transaction");
		}

		/* re-initialize i2c for recovery */
		iproc_i2c_enable_disable(dev, false);
		iproc_i2c_target_init(dev, true);
		iproc_i2c_enable_disable(dev, true);

		return -ETIMEDOUT;
	}

	return 0;
}

static void iproc_i2c_target_read(const struct device *dev)
{
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	struct i2c_target_config *target_cfg = dd->target_cfg;
	mem_addr_t base = DEV_BASE(dev);
	uint8_t rx_data, rx_status;
	uint32_t rx_bytes = 0;
	uint32_t val;

	while (rx_bytes < MAX_TARGET_RX_PER_INT) {
		val = sys_read32(base + S_RX_OFFSET);
		rx_status = (val >> S_RX_STATUS_SHIFT) & S_RX_STATUS_MASK;
		rx_data = ((val >> S_RX_DATA_SHIFT) & S_RX_DATA_MASK);

		if (rx_status == I2C_TARGET_RX_START) {
			/* Start of SMBUS Master write */
			target_cfg->callbacks->write_requested(target_cfg);
			dd->rx_start_rcvd = true;
			dd->target_read_complete = false;
		} else if ((rx_status == I2C_TARGET_RX_DATA) && dd->rx_start_rcvd) {
			/* Middle of SMBUS Master write */
			target_cfg->callbacks->write_received(target_cfg, rx_data);
		} else if ((rx_status == I2C_TARGET_RX_END) && dd->rx_start_rcvd) {
			/* End of SMBUS Master write */
			if (dd->target_rx_only) {
				target_cfg->callbacks->write_received(target_cfg, rx_data);
			}
			target_cfg->callbacks->stop(target_cfg);
		} else if (rx_status == I2C_TARGET_RX_FIFO_EMPTY) {
			dd->rx_start_rcvd = false;
			dd->target_read_complete = true;
			break;
		}

		rx_bytes++;
	}
}

static void iproc_i2c_target_rx(const struct device *dev)
{
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	mem_addr_t base = DEV_BASE(dev);

	iproc_i2c_target_read(dev);

	if (!dd->target_rx_only && dd->target_read_complete) {
		/*
		 * In case of single byte master-read request,
		 * IS_S_TX_UNDERRUN_SHIFT event is generated before
		 * IS_S_START_BUSY_SHIFT event. Hence start target data send
		 * from first IS_S_TX_UNDERRUN_SHIFT event.
		 *
		 * This means don't send any data from target when
		 * IS_S_RD_EN_SHIFT event is generated else it will increment
		 * eeprom or other backend target driver read pointer twice.
		 */
		dd->tx_underrun = 0;
		dd->target_int_mask |= BIT(IE_S_TX_UNDERRUN_SHIFT);

		/* clear IS_S_RD_EN_SHIFT interrupt */
		sys_write32(BIT(IS_S_RD_EN_SHIFT), base + IS_OFFSET);
	}

	/* enable target interrupts */
	sys_write32(dd->target_int_mask, base + IE_OFFSET);
}

static void iproc_i2c_target_isr(const struct device *dev, uint32_t status)
{
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	struct i2c_target_config *target_cfg = dd->target_cfg;
	mem_addr_t base = DEV_BASE(dev);
	uint32_t val;
	uint8_t data;

	LOG_DBG("iproc_i2c(0x%x): %s: sl_sts 0x%x", (uint32_t)base, __func__, status);

	if (status & BIT(IS_S_RX_EVENT_SHIFT) || status & BIT(IS_S_RD_EN_SHIFT) ||
	    status & BIT(IS_S_RX_FIFO_FULL_SHIFT)) {
		/* disable target interrupts */
		val = sys_read32(base + IE_OFFSET);
		val &= ~dd->target_int_mask;
		sys_write32(val, base + IE_OFFSET);

		if (status & BIT(IS_S_RD_EN_SHIFT)) {
			/* Master-write-read request */
			dd->target_rx_only = false;
		} else {
			/* Master-write request only */
			dd->target_rx_only = true;
		}

		/*
		 * Clear IS_S_RX_EVENT_SHIFT &
		 * IS_S_RX_FIFO_FULL_SHIFT interrupt
		 */
		val = BIT(IS_S_RX_EVENT_SHIFT);
		if (status & BIT(IS_S_RX_FIFO_FULL_SHIFT)) {
			val |= BIT(IS_S_RX_FIFO_FULL_SHIFT);
		}
		sys_write32(val, base + IS_OFFSET);

		iproc_i2c_target_rx(dev);
	}

	if (status & BIT(IS_S_TX_UNDERRUN_SHIFT)) {
		dd->tx_underrun++;
		if (dd->tx_underrun == 1) {
			/* Start of SMBUS for Master Read */
			target_cfg->callbacks->read_requested(target_cfg, &data);
		} else {
			/* Master read other than start */
			target_cfg->callbacks->read_processed(target_cfg, &data);
		}

		sys_write32(data, base + S_TX_OFFSET);
		/* start transfer */
		val = BIT(S_CMD_START_BUSY_SHIFT);
		sys_write32(val, base + S_CMD_OFFSET);

		sys_write32(BIT(IS_S_TX_UNDERRUN_SHIFT), base + IS_OFFSET);
	}

	/* Stop received from master in case of master read transaction */
	if (status & BIT(IS_S_START_BUSY_SHIFT)) {
		/*
		 * Disable interrupt for TX FIFO becomes empty and
		 * less than PKT_LENGTH bytes were output on the SMBUS
		 */
		dd->target_int_mask &= ~BIT(IE_S_TX_UNDERRUN_SHIFT);
		sys_write32(dd->target_int_mask, base + IE_OFFSET);

		/* End of SMBUS for Master Read */
		val = BIT(S_TX_WR_STATUS_SHIFT);
		sys_write32(val, base + S_TX_OFFSET);

		val = BIT(S_CMD_START_BUSY_SHIFT);
		sys_write32(val, base + S_CMD_OFFSET);

		/* flush TX FIFOs */
		val = sys_read32(base + S_FIFO_CTRL_OFFSET);
		val |= (BIT(S_FIFO_TX_FLUSH_SHIFT));
		sys_write32(val, base + S_FIFO_CTRL_OFFSET);

		target_cfg->callbacks->stop(target_cfg);

		sys_write32(BIT(IS_S_START_BUSY_SHIFT), base + IS_OFFSET);
	}

	/* check target transmit status only if target is transmitting */
	if (!dd->target_rx_only) {
		iproc_i2c_check_target_status(dev);
	}
}

static int iproc_i2c_target_register(const struct device *dev,
				     struct i2c_target_config *target_config)
{
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	mem_addr_t base = DEV_BASE(dev);
	int ret = 0;

	if (dd->target_cfg) {
		LOG_ERR("Target already registered");
		return -EBUSY;
	}

	/* Save pointer to received target config */
	dd->target_cfg = target_config;

	ret = iproc_i2c_target_init(dev, false);
	if (ret < 0) {
		LOG_ERR("Failed to register iproc_i2c(0x%x) as target, ret %d", (uint32_t)base,
			ret);
		return ret;
	}

	return 0;
}

static int iproc_i2c_target_unregister(const struct device *dev, struct i2c_target_config *config)
{
	uint32_t val;
	mem_addr_t base = DEV_BASE(dev);
	struct iproc_i2c_data *dd = DEV_DATA(dev);

	if (!dd->target_cfg) {
		return -EINVAL;
	}

	/* Erase the target address programmed */
	sys_write32(0x0, base + S_ADDR_OFFSET);

	/* disable all target interrupts */
	val = sys_read32(base + IE_OFFSET);
	val &= ~(IE_S_ALL_INTERRUPT_MASK << IE_S_ALL_INTERRUPT_SHIFT);
	sys_write32(val, base + IE_OFFSET);

	dd->target_cfg = NULL;

	return 0;
}
#endif /* CONFIG_I2C_TARGET */

static void iproc_i2c_common_init(const struct device *dev)
{
	mem_addr_t base = DEV_BASE(dev);
	uint32_t val;

	iproc_i2c_reset_controller(dev);

	/* flush TX/RX FIFOs and set RX FIFO threshold to zero */
	val = BIT(M_FIFO_RX_FLUSH_SHIFT) | BIT(M_FIFO_TX_FLUSH_SHIFT);
	sys_write32(val, base + M_FIFO_CTRL_OFFSET);

	/* disable all interrupts */
	sys_write32(0, base + IE_OFFSET);

	/* clear all pending interrupts */
	sys_write32(~0, base + IS_OFFSET);
}

static int iproc_i2c_check_status(const struct device *dev, uint16_t dev_addr, struct i2c_msg *msg)
{
	mem_addr_t base = DEV_BASE(dev);
	uint32_t val;
	int rc;

	val = sys_read32(base + M_CMD_OFFSET);
	val >>= M_CMD_STATUS_SHIFT;
	val &= M_CMD_STATUS_MASK;

	switch (val) {
	case M_CMD_STATUS_SUCCESS:
		rc = 0;
		break;

	case M_CMD_STATUS_LOST_ARB:
		LOG_ERR("lost bus arbitration");
		rc = -EAGAIN;
		break;

	case M_CMD_STATUS_NACK_ADDR:
		LOG_ERR("NAK addr:0x%02x", dev_addr);
		rc = -ENXIO;
		break;

	case M_CMD_STATUS_NACK_DATA:
		LOG_ERR("NAK data");
		rc = -ENXIO;
		break;

	case M_CMD_STATUS_TIMEOUT:
		LOG_ERR("bus timeout");
		rc = -ETIMEDOUT;
		break;

	case M_CMD_STATUS_FIFO_UNDERRUN:
		LOG_ERR("FIFO Under-run");
		rc = -ENXIO;
		break;

	case M_CMD_STATUS_RX_FIFO_FULL:
		LOG_ERR("RX FIFO full");
		rc = -ETIMEDOUT;
		break;

	default:
		LOG_ERR("Unknown Error : 0x%x", val);
		iproc_i2c_enable_disable(dev, false);
		iproc_i2c_common_init(dev);
		iproc_i2c_enable_disable(dev, true);
		rc = -EIO;
		break;
	}

	if (rc < 0) {
		/* flush both Master TX/RX FIFOs */
		val = BIT(M_FIFO_RX_FLUSH_SHIFT) | BIT(M_FIFO_TX_FLUSH_SHIFT);
		sys_write32(val, base + M_FIFO_CTRL_OFFSET);
	}

	return rc;
}

static int iproc_i2c_configure(const struct device *dev, uint32_t dev_cfg_raw)
{
	mem_addr_t base = DEV_BASE(dev);

	if (I2C_ADDR_10_BITS & dev_cfg_raw) {
		LOG_ERR("10-bit addressing not supported");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_cfg_raw)) {
	case I2C_SPEED_STANDARD:
		sys_clear_bit(base + TIM_CFG_OFFSET, TIM_CFG_MODE_400_SHIFT);
		break;
	case I2C_SPEED_FAST:
		sys_set_bit(base + TIM_CFG_OFFSET, TIM_CFG_MODE_400_SHIFT);
		break;
	default:
		LOG_ERR("Only standard or Fast speed modes are supported");
		return -ENOTSUP;
	}

	return 0;
}

static void iproc_i2c_read_valid_bytes(const struct device *dev)
{
	mem_addr_t base = DEV_BASE(dev);
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	struct i2c_msg *msg = dd->msg;
	uint32_t val;

	/* Read valid data from RX FIFO */
	while (dd->rx_bytes < msg->len) {
		val = sys_read32(base + M_RX_OFFSET);

		/* rx fifo empty */
		if (!((val >> M_RX_STATUS_SHIFT) & M_RX_STATUS_MASK)) {
			break;
		}

		msg->buf[dd->rx_bytes] = (val >> M_RX_DATA_SHIFT) & M_RX_DATA_MASK;
		dd->rx_bytes++;
	}
}

static int iproc_i2c_data_recv(const struct device *dev)
{
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	mem_addr_t base = DEV_BASE(dev);
	struct i2c_msg *msg = dd->msg;
	uint32_t bytes_left, val;

	iproc_i2c_read_valid_bytes(dev);

	bytes_left = msg->len - dd->rx_bytes;
	if (bytes_left == 0) {
		/* finished reading all data, disable rx thld event */
		sys_clear_bit(base + IE_OFFSET, IS_M_RX_THLD_SHIFT);
	} else if (bytes_left < dd->thld_bytes) {
		/* set bytes left as threshold */
		val = sys_read32(base + M_FIFO_CTRL_OFFSET);
		val &= ~(M_FIFO_RX_THLD_MASK << M_FIFO_RX_THLD_SHIFT);
		val |= (bytes_left << M_FIFO_RX_THLD_SHIFT);
		sys_write32(val, base + M_FIFO_CTRL_OFFSET);
		dd->thld_bytes = bytes_left;
	}
	/*
	 * if bytes_left >= dd->thld_bytes, no need to change the THRESHOLD.
	 * It will remain as dd->thld_bytes itself
	 */

	return 0;
}

static int iproc_i2c_transfer_one(const struct device *dev, struct i2c_msg *msg, uint16_t dev_addr)
{
	mem_addr_t base = DEV_BASE(dev);
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	uint32_t val, addr, tx_bytes, val_intr_en;
	int rc;

	if (!!(sys_read32(base + M_CMD_OFFSET) & BIT(M_CMD_START_BUSY_SHIFT))) {
		LOG_ERR("Bus busy, prev xfer ongoing");
		return -EBUSY;
	}

	LOG_DBG("%s: msg dev_addr 0x%x flags 0x%x len 0x%x val 0x%x\n", __func__, dev_addr,
		msg->flags, msg->len, msg->buf[0]);

	/* Save current i2c_msg */
	dd->msg = msg;

	addr = dev_addr << 1 | (msg->flags & I2C_MSG_READ ? 1 : 0);
	sys_write32(addr, base + M_TX_OFFSET);

	tx_bytes = MIN(msg->len, (TX_RX_FIFO_SIZE - 1));
	if (!(msg->flags & I2C_MSG_READ)) {
		/* Fill master TX fifo */
		for (uint32_t i = 0; i < tx_bytes; i++) {
			val = msg->buf[i];
			/* For the last byte, set MASTER_WR_STATUS bit */
			if (i == msg->len - 1) {
				val |= BIT(M_TX_WR_STATUS_SHIFT);
			}
			sys_write32(val, base + M_TX_OFFSET);
		}

		dd->tx_bytes = tx_bytes;
	}

	/*
	 * Enable the "start busy" interrupt, which will be triggered after the
	 * transaction is done, i.e., the internal start_busy bit, transitions
	 * from 1 to 0.
	 */
	val_intr_en = BIT(IE_M_START_BUSY_SHIFT);

	if (!(msg->flags & I2C_MSG_READ) && (msg->len > dd->tx_bytes)) {
		val_intr_en |= BIT(IE_M_TX_UNDERRUN_SHIFT);
	}

	/*
	 * Program master command register (0x30) with protocol type and set
	 * start_busy_command bit to initiate the write transaction
	 */
	val = BIT(M_CMD_START_BUSY_SHIFT);
	if (msg->len == 0) {
		/* SMBUS QUICK Command (Read/Write) */
		val |= (M_CMD_SMB_PROT_QUICK << M_CMD_SMB_PROT_SHIFT);
	} else if (msg->flags & I2C_MSG_READ) {
		uint32_t tmp;

		dd->rx_bytes = 0;

		/* SMBUS Block Read Command */
		val |= M_CMD_SMB_PROT_BLK_RD << M_CMD_SMB_PROT_SHIFT;
		val |= msg->len;

		if (msg->len > M_RX_FIFO_MAX_THLD_VALUE) {
			dd->thld_bytes = M_RX_FIFO_THLD_VALUE;
		} else {
			dd->thld_bytes = msg->len;
		}

		/* set threshold value */
		tmp = sys_read32(base + M_FIFO_CTRL_OFFSET);
		tmp &= ~(M_FIFO_RX_THLD_MASK << M_FIFO_RX_THLD_SHIFT);
		tmp |= dd->thld_bytes << M_FIFO_RX_THLD_SHIFT;
		sys_write32(tmp, base + M_FIFO_CTRL_OFFSET);

		/* enable the RX threshold interrupt */
		val_intr_en |= BIT(IE_M_RX_THLD_SHIFT);
	} else {
		/* SMBUS Block Write Command */
		val |= M_CMD_SMB_PROT_BLK_WR << M_CMD_SMB_PROT_SHIFT;
	}

	sys_write32(val_intr_en, base + IE_OFFSET);

	sys_write32(val, base + M_CMD_OFFSET);

	/* Wait for the transfer to complete or timeout */
	rc = k_sem_take(&dd->device_sync_sem, K_MSEC(I2C_TIMEOUT_MSEC));

	/* disable all interrupts */
	sys_write32(0, base + IE_OFFSET);

	if (rc != 0) {
		/* flush both Master TX/RX FIFOs */
		val = BIT(M_FIFO_RX_FLUSH_SHIFT) | BIT(M_FIFO_TX_FLUSH_SHIFT);
		sys_write32(val, base + M_FIFO_CTRL_OFFSET);
		return rc;
	}

	/* Check for Master Xfer status */
	rc = iproc_i2c_check_status(dev, dev_addr, msg);

	return rc;
}

static int iproc_i2c_transfer_multi(const struct device *dev, struct i2c_msg *msgs,
				    uint8_t num_msgs, uint16_t addr)
{
	int rc;
	struct i2c_msg *msgs_chk = msgs;

	if (!msgs_chk) {
		return -EINVAL;
	}

	/* pre-check all msgs */
	for (uint8_t i = 0; i < num_msgs; i++, msgs_chk++) {
		if (!msgs_chk->buf) {
			LOG_ERR("Invalid msg buffer");
			return -EINVAL;
		}

		if (I2C_MSG_ADDR_10_BITS & msgs_chk->flags) {
			LOG_ERR("10-bit addressing not supported");
			return -ENOTSUP;
		}
	}

	for (uint8_t i = 0; i < num_msgs; i++, msgs++) {
		rc = iproc_i2c_transfer_one(dev, msgs, addr);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static void iproc_i2c_send_data(const struct device *dev)
{
	mem_addr_t base = DEV_BASE(dev);
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	struct i2c_msg *msg = dd->msg;
	uint32_t tx_bytes = msg->len - dd->tx_bytes;

	/* can only fill up to the FIFO size */
	tx_bytes = MIN(tx_bytes, TX_RX_FIFO_SIZE);
	for (uint32_t i = 0; i < tx_bytes; i++) {
		/* start from where we left over */
		uint32_t idx = dd->tx_bytes + i;

		uint32_t val = msg->buf[idx];

		/* mark the last byte */
		if (idx == (msg->len - 1)) {
			uint32_t tmp;

			val |= BIT(M_TX_WR_STATUS_SHIFT);

			/*
			 * Since this is the last byte, we should now
			 * disable TX FIFO underrun interrupt
			 */
			tmp = sys_read32(base + IE_OFFSET);
			tmp &= ~BIT(IE_M_TX_UNDERRUN_SHIFT);
			sys_write32(tmp, base + IE_OFFSET);
		}

		/* load data into TX FIFO */
		sys_write32(val, base + M_TX_OFFSET);
	}

	/* update number of transferred bytes */
	dd->tx_bytes += tx_bytes;
}

static void iproc_i2c_master_isr(const struct device *dev, uint32_t status)
{
	struct iproc_i2c_data *dd = DEV_DATA(dev);

	/* TX FIFO is empty and we have more data to send */
	if (status & BIT(IS_M_TX_UNDERRUN_SHIFT)) {
		iproc_i2c_send_data(dev);
	}

	/* RX FIFO threshold is reached and data needs to be read out */
	if (status & BIT(IS_M_RX_THLD_SHIFT)) {
		iproc_i2c_data_recv(dev);
	}

	/* transfer is done */
	if (status & BIT(IS_M_START_BUSY_SHIFT)) {
		k_sem_give(&dd->device_sync_sem);
	}
}

static void iproc_i2c_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	mem_addr_t base = DEV_BASE(dev);
	uint32_t status;
	uint32_t sl_status, curr_irqs;

	curr_irqs = sys_read32(base + IE_OFFSET);
	status = sys_read32(base + IS_OFFSET);

	/* process only target interrupt which are enabled */
	sl_status = status & curr_irqs & ISR_MASK_TARGET;
	LOG_DBG("iproc_i2c(0x%x): sts 0x%x, sl_sts 0x%x, curr_ints 0x%x", (uint32_t)base, status,
		sl_status, curr_irqs);

#ifdef CONFIG_I2C_TARGET
	/* target events */
	if (sl_status) {
		iproc_i2c_target_isr(dev, sl_status);
		return;
	}
#endif

	status &= ISR_MASK;
	/* master events */
	if (status) {
		iproc_i2c_master_isr(dev, status);
		sys_write32(status, base + IS_OFFSET);
	}
}

static int iproc_i2c_init(const struct device *dev)
{
	const struct iproc_i2c_config *config = DEV_CFG(dev);
	struct iproc_i2c_data *dd = DEV_DATA(dev);
	uint32_t bitrate = config->bitrate;
	int error;

	k_sem_init(&dd->device_sync_sem, 0, 1);

	iproc_i2c_common_init(dev);

	/* Set default clock frequency */
	bitrate = i2c_map_dt_bitrate(bitrate);

	if (dd->target_cfg == NULL) {
		bitrate |= I2C_MODE_CONTROLLER;
	}

	error = iproc_i2c_configure(dev, bitrate);
	if (error) {
		return error;
	}

	config->irq_config_func(dev);

	iproc_i2c_enable_disable(dev, true);

	return 0;
}

static const struct i2c_driver_api iproc_i2c_driver_api = {
	.configure = iproc_i2c_configure,
	.transfer = iproc_i2c_transfer_multi,
#ifdef CONFIG_I2C_TARGET
	.target_register = iproc_i2c_target_register,
	.target_unregister = iproc_i2c_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define IPROC_I2C_DEVICE_INIT(n)                                                                   \
	static void iproc_i2c_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), iproc_i2c_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct iproc_i2c_config iproc_i2c_config_##n = {                              \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = iproc_i2c_irq_config_func_##n,                                  \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
	};                                                                                         \
                                                                                                   \
	static struct iproc_i2c_data iproc_i2c_data_##n;                                           \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, &iproc_i2c_init, NULL, &iproc_i2c_data_##n,                   \
				  &iproc_i2c_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,    \
				  &iproc_i2c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IPROC_I2C_DEVICE_INIT)
