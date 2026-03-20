/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sercom_g1_i2c

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
#include <zephyr/drivers/dma.h>
#include <mchp_dt_helper.h>
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

LOG_MODULE_REGISTER(i2c_mchp_sercom_g1, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#define I2C_BAUD_MAX          0xFFU
#define I2C_BAUD_LOW_HIGH_MAX 382U

/*
 * Maximum Rise Time (tRISE) with CLOAD = 30 pF for I2C pins
 */
#define I2C_TRISE_BACKUP_MODE_NS    4000U /* Backup pins in backup mode: 4 μs */
#define I2C_TRISE_NORMAL_DRVSTR0_NS 40U   /* Normal mode with DRVSTR=0: 40 ns */
#define I2C_TRISE_NORMAL_DRVSTR1_NS 10U   /* Normal mode with DRVSTR=1: 10 ns */
#define I2C_TRISE_DEFAULT_NS        I2C_TRISE_NORMAL_DRVSTR0_NS

#ifdef CONFIG_I2C_MCHP_TRANSFER_TIMEOUT
#define I2C_TRANSFER_TIMEOUT_MSEC K_MSEC(CONFIG_I2C_MCHP_TRANSFER_TIMEOUT)
#else
#define I2C_TRANSFER_TIMEOUT_MSEC K_FOREVER
#endif /*CONFIG_I2C_MCHP_TRANSFER_TIMEOUT*/
#define I2C_TIMEOUT_US                   1000U
#define I2C_POLL_DELAY_US                2U
#define I2C_CMD_STOP                     0x3U
#define SERCOM_I2CS_SPEED_FAST_MODE_PLUS 0x1U

#define DEV_CFG(dev)  ((const struct i2c_mchp_dev_config *)(dev)->config)
#define DEV_DATA(dev) ((struct i2c_mchp_dev_data *)(dev)->data)
#define DEV_REGS(dev) (DEV_CFG(dev)->regs)
#define I2CM(dev)     (DEV_REGS(dev)->I2CM)
#define I2CS(dev)     (DEV_REGS(dev)->I2CS)

#define MSG_HAS_RESTART(msg)    ((msg)->flags & I2C_MSG_RESTART)
#define I2C_CURR_MSG(data)      (&(data)->msgs[(data)->msg_idx])
#define I2C_CURR_BYTE_PTR(data) (&I2C_CURR_MSG(data)->buf[(data)->byte_idx])

#define I2C_WAIT_SYNC_M(dev, mask)                                                                 \
	WAIT_FOR(!(I2CM(dev).SERCOM_SYNCBUSY & (mask)), I2C_TIMEOUT_US,                            \
		 k_busy_wait(I2C_POLL_DELAY_US))
#define I2C_WAIT_SYNC_S(dev, mask)                                                                 \
	WAIT_FOR(!(I2CS(dev).SERCOM_SYNCBUSY & (mask)), I2C_TIMEOUT_US,                            \
		 k_busy_wait(I2C_POLL_DELAY_US))

enum i2c_target_cmd {
	I2C_TARGET_CMD_ACK,
	I2C_TARGET_CMD_NACK,
	I2C_TARGET_RECEIVE_ACK_NAK,
	I2C_TARGET_CMD_WAIT_START,
};

struct i2c_mchp_clock {
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
struct i2c_mchp_dma {
	const struct device *dma_dev;
	uint8_t tx_dma_request;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_request;
	uint8_t rx_dma_channel;
};
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/

struct i2c_mchp_dev_config {
	sercom_registers_t *regs;
	struct i2c_mchp_clock clk;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
	struct i2c_mchp_dma dma;
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/
	uint32_t bitrate;
	uint8_t run_in_standby;
};

struct i2c_mchp_dev_data {
	const struct device *dev;
	struct i2c_msg *msgs;
#if defined(CONFIG_I2C_CALLBACK)
	i2c_callback_t async_cb;
	void *user_data;
#endif /*CONFIG_I2C_CALLBACK*/
#if defined(CONFIG_I2C_TARGET)
	struct i2c_target_config tgt_cfg;
	struct i2c_target_callbacks tgt_cb;
#endif /*CONFIG_I2C_TARGET*/
	struct k_sem lock;
	struct k_sem sync_sem;
	int xfer_status;
	uint32_t dev_config;
	uint32_t byte_idx;
	uint32_t seg_remaining;
	uint16_t target_addr;
	uint8_t msg_idx;
	uint8_t num_msgs;
	uint8_t next_seg_idx;
	bool target_mode;
#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
	bool dma_active;
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/
#if defined(CONFIG_I2C_TARGET)
	bool first_read;
	uint8_t tgt_rx_data;
#endif /*CONFIG_I2C_TARGET*/
};

#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
static int i2c_dma_setup(const struct device *dev, bool is_read);
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/

/* Resets I2C peripheral registers to initial state */
static void i2c_reset(const struct device *dev)
{
	I2CM(dev).SERCOM_CTRLA = SERCOM_I2CM_CTRLA_SWRST(1);

	if (!I2C_WAIT_SYNC_M(dev, SERCOM_I2CM_SYNCBUSY_SWRST_Msk)) {
		LOG_ERR("SWRST sync timeout");
	}
}

/* Enables or disables I2C peripheral */
static void i2c_enable(const struct device *dev, bool enable)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	uint32_t ctrla;
	bool sync_ok;

	if (data->target_mode) {
		ctrla = I2CS(dev).SERCOM_CTRLA;
		ctrla = (ctrla & ~SERCOM_I2CS_CTRLA_ENABLE_Msk) | SERCOM_I2CS_CTRLA_ENABLE(enable);
		I2CS(dev).SERCOM_CTRLA = ctrla;
		sync_ok = I2C_WAIT_SYNC_S(dev, SERCOM_I2CS_SYNCBUSY_ENABLE_Msk);
	} else {
		ctrla = I2CM(dev).SERCOM_CTRLA;
		ctrla = (ctrla & ~SERCOM_I2CM_CTRLA_ENABLE_Msk) | SERCOM_I2CM_CTRLA_ENABLE(enable);
		I2CM(dev).SERCOM_CTRLA = ctrla;
		sync_ok = I2C_WAIT_SYNC_M(dev, SERCOM_I2CM_SYNCBUSY_ENABLE_Msk);
	}

	if (!sync_ok) {
		LOG_ERR("ENABLE sync timeout");
	}
}

/* Configures run-in-standby mode */
static void i2c_set_runstandby(const struct device *dev)
{
	const struct i2c_mchp_dev_config *cfg = DEV_CFG(dev);
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	uint32_t ctrla;

	if (data->target_mode) {
		ctrla = I2CS(dev).SERCOM_CTRLA;
		ctrla = (ctrla & ~SERCOM_I2CS_CTRLA_RUNSTDBY_Msk) |
			SERCOM_I2CS_CTRLA_RUNSTDBY(cfg->run_in_standby);
		I2CS(dev).SERCOM_CTRLA = ctrla;
	} else {
		ctrla = I2CM(dev).SERCOM_CTRLA;
		ctrla = (ctrla & ~SERCOM_I2CM_CTRLA_RUNSTDBY_Msk) |
			SERCOM_I2CM_CTRLA_RUNSTDBY(cfg->run_in_standby);
		I2CM(dev).SERCOM_CTRLA = ctrla;
	}
}

/* Sets I2C bus to idle state */
static void i2c_set_bus_idle(const struct device *dev)
{
	I2CM(dev).SERCOM_STATUS = (I2CM(dev).SERCOM_STATUS & ~SERCOM_I2CM_STATUS_BUSSTATE_Msk) |
				  SERCOM_I2CM_STATUS_BUSSTATE(SERCOM_I2CM_STATUS_BUSSTATE_IDLE_Val);

	if (!I2C_WAIT_SYNC_M(dev, SERCOM_I2CM_SYNCBUSY_SYSOP_Msk)) {
		LOG_ERR("Bus idle sync timeout");
	}
}

/* Sends I2C STOP condition on the bus */
static void i2c_send_stop(const struct device *dev)
{
	I2CM(dev).SERCOM_CTRLB = (I2CM(dev).SERCOM_CTRLB & ~SERCOM_I2CM_CTRLB_CMD_Msk) |
				 SERCOM_I2CM_CTRLB_CMD(I2C_CMD_STOP);

	if (!I2C_WAIT_SYNC_M(dev, SERCOM_I2CM_SYNCBUSY_SYSOP_Msk)) {
		LOG_ERR("STOP sync timeout");
	}
}

/* Sets ACK/NACK control for I2C target response */
static inline void i2c_set_ack(const struct device *dev, bool ack)
{
	uint32_t ctrlb = I2CM(dev).SERCOM_CTRLB;

	ctrlb = (ctrlb & ~SERCOM_I2CM_CTRLB_ACKACT_Msk) | SERCOM_I2CM_CTRLB_ACKACT(!ack);
	I2CM(dev).SERCOM_CTRLB = ctrlb;
}

/* Checks if NACK was received from I2C slave */
static inline bool i2c_got_nack(const struct device *dev)
{
	return (I2CM(dev).SERCOM_STATUS & SERCOM_I2CM_STATUS_RXNACK_Msk) != 0;
}

/* Writes I2C address */
static void i2c_write_addr(const struct device *dev, uint16_t addr, bool is_read)
{
	if (is_read) {
		addr |= BIT(0);
		i2c_set_ack(dev, true);
	}

	I2CM(dev).SERCOM_ADDR =
		(I2CM(dev).SERCOM_ADDR & ~SERCOM_I2CM_ADDR_ADDR_Msk) | SERCOM_I2CM_ADDR_ADDR(addr);

	if (!I2C_WAIT_SYNC_M(dev, SERCOM_I2CM_SYNCBUSY_SYSOP_Msk)) {
		LOG_ERR("ADDR write sync timeout");
	}
}

/* Calculates baud rate register values for requested I2C bitrate */
static bool i2c_baudrate_calc(uint32_t bitrate, uint32_t sys_clock_rate, uint32_t *baud_val)
{
	uint32_t baud_value = 0U;

	/* Reference clock frequency must be at least two times the baud rate */
	if (sys_clock_rate < (2U * bitrate)) {
		LOG_ERR("Invalid I2C clock configuration: sys_clk=%u Hz, bitrate=%u Hz",
			sys_clock_rate, bitrate);
		return false;
	}

	if (bitrate > I2C_BITRATE_FAST_PLUS) {
		/* HS mode baud calculation: BAUD = (f_ref / f_scl) - 2 */
		baud_value = (sys_clock_rate / bitrate) - 2U;
	} else {
		/* Standard, FM and FM+ baud calculation:
		 * BAUD = (f_ref / f_scl) - ((f_ref * T_RISE_ns) / 1,000,000,000) - 10
		 */
		baud_value = (sys_clock_rate / bitrate) -
			     ((sys_clock_rate * I2C_TRISE_DEFAULT_NS) / 1000000000U) - 10U;
	}

	if (bitrate <= I2C_BITRATE_FAST) {
		/* For I2C clock speed up to 400 kHz, the value of BAUD<7:0>
		 * determines both SCL_L and SCL_H with SCL_L = SCL_H
		 */
		if (baud_value > (I2C_BAUD_MAX * 2U)) {
			/* Set baud rate to the maximum possible value */
			baud_value = I2C_BAUD_MAX;
		} else if (baud_value <= 1U) {
			/* Baud value cannot be 0. Set baud rate to minimum possible */
			baud_value = 1U;
		} else {
			baud_value /= 2U;
		}

		*baud_val = baud_value;

		return true;
	}

	/* To maintain the ratio of SCL_L:SCL_H to 2:1, the max value of
	 * BAUD_LOW<15:8>:BAUD<7:0> can be 0xFF:0x7F. Hence BAUD_LOW + BAUD
	 * can not exceed 255+127 = 382
	 */
	if (baud_value >= I2C_BAUD_LOW_HIGH_MAX) {
		/* Set baud rate to the maximum possible value while
		 * maintaining SCL_L:SCL_H to 2:1
		 */
		baud_value = (0xFFUL << 8U) | (0x7FU);
	} else if (baud_value <= 3U) {
		/* Baud value cannot be 0. Set baud rate to minimum possible
		 * value while maintaining SCL_L:SCL_H to 2:1
		 */
		baud_value = (2UL << 8U) | 1U;
	} else {
		/* For Fm+ mode, I2C SCL_L:SCL_H to 2:1 */
		baud_value = ((((baud_value * 2U) / 3U) << 8U) | (baud_value / 3U));
	}

	*baud_val = baud_value;

	return true;
}

/* Set calculated baud rate to I2C registers */
static bool i2c_set_baudrate(const struct device *dev, uint32_t bitrate, uint32_t sys_clock_rate)
{
	uint32_t i2c_speed_mode = 0;
	uint32_t baud_value;
	uint32_t hsbaud_value;
	uint32_t sda_hold_time = 0;
	bool is_success = false;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if (bitrate == I2C_BITRATE_HIGH) {

		/* HS mode requires baud values for both FS and HS frequency. First
		 * calculate baud for FS
		 */
		if (i2c_baudrate_calc(I2C_BITRATE_FAST, sys_clock_rate, &baud_value) &&
		    i2c_baudrate_calc(bitrate, sys_clock_rate, &hsbaud_value)) {
			is_success = true;
			baud_value |= (hsbaud_value << 16U);
			i2c_speed_mode = 2U;
			sda_hold_time = 2U;
		}

	} else {
		if (i2c_baudrate_calc(bitrate, sys_clock_rate, &baud_value)) {
			if (bitrate == I2C_BITRATE_FAST_PLUS) {
				i2c_speed_mode = 1U;
				sda_hold_time = 1U;
			}
			is_success = true;
		}
	}

	if (is_success) {

		i2c_regs->I2CM.SERCOM_BAUD = baud_value;

		i2c_regs->I2CM.SERCOM_CTRLA =
			((i2c_regs->I2CM.SERCOM_CTRLA &
			  (~SERCOM_I2CM_CTRLA_SPEED_Msk | ~SERCOM_I2CM_CTRLA_SDAHOLD_Msk)) |
			 (SERCOM_I2CM_CTRLA_SPEED(i2c_speed_mode) |
			  SERCOM_I2CM_CTRLA_SDAHOLD(sda_hold_time)));
	}

	return is_success;
}

/* Applies I2C speed configuration and sets baud rate */
static int i2c_apply_speed(const struct device *dev, uint32_t config)
{
	const struct i2c_mchp_dev_config *cfg = DEV_CFG(dev);
	uint32_t f_scl;
	uint32_t f_ref = 0;

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		f_scl = I2C_BITRATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		f_scl = I2C_BITRATE_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		f_scl = I2C_BITRATE_FAST_PLUS;
		break;
	case I2C_SPEED_HIGH:
		f_scl = I2C_BITRATE_HIGH;
		break;
	default:
		LOG_ERR("Unsupported speed: %d", I2C_SPEED_GET(config));
		return -ENOTSUP;
	}

	clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(clock)), cfg->clk.gclk_sys, &f_ref);
	if (f_ref == 0) {
		LOG_ERR("Failed to get clock rate");
		return -EIO;
	}

	if (!i2c_set_baudrate(dev, f_scl, f_ref)) {
		LOG_ERR("Failed to set baudrate");
		return -EIO;
	}

	return 0;
}

/* Initializes I2C controller mode */
static void i2c_setup_controller_mode(const struct device *dev)
{
	I2CM(dev).SERCOM_CTRLB =
		(I2CM(dev).SERCOM_CTRLB & ~SERCOM_I2CM_CTRLB_SMEN_Msk) | SERCOM_I2CM_CTRLB_SMEN(1);

	uint32_t ctrla = I2CM(dev).SERCOM_CTRLA;

	ctrla &= ~(SERCOM_I2CM_CTRLA_MODE_Msk | SERCOM_I2CM_CTRLA_INACTOUT_Msk |
		   SERCOM_I2CM_CTRLA_LOWTOUTEN_Msk);
	ctrla |= SERCOM_I2CM_CTRLA_MODE(SERCOM_I2CM_CTRLA_MODE_I2C_MASTER_Val) |
		 SERCOM_I2CM_CTRLA_LOWTOUTEN(1) |
		 SERCOM_I2CM_CTRLA_INACTOUT(SERCOM_I2CM_CTRLA_INACTOUT_205US_Val);
	I2CM(dev).SERCOM_CTRLA = ctrla;
}

/* Calculates length of consecutive I2C messages with same direction (read/write) */
static uint32_t i2c_segment_len(struct i2c_msg *msgs, uint8_t num_msgs, uint8_t start_idx,
				uint8_t *next_idx)
{
	uint32_t total;
	uint8_t dir;
	uint8_t i;

	if (start_idx >= num_msgs) {
		*next_idx = num_msgs;
		return 0;
	}

	total = msgs[start_idx].len;
	dir = msgs[start_idx].flags & I2C_MSG_RW_MASK;

	for (i = start_idx + 1; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) != dir) {
			break;
		}
		total += msgs[i].len;
	}

	*next_idx = i;
	return total;
}

/* Initializes segment tracking for I2C transfer message handling */
static void i2c_init_segment(struct i2c_mchp_dev_data *data)
{
	data->msg_idx = data->next_seg_idx;
	data->byte_idx = 0;
	data->seg_remaining = i2c_segment_len(data->msgs, data->num_msgs, data->next_seg_idx,
					      &data->next_seg_idx);
}

/* Completes I2C transfer: signals callback (async) or semaphore (sync) */
static void i2c_xfer_complete(const struct device *dev, int status)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);

#if defined(CONFIG_I2C_CALLBACK)
	if (data->async_cb != NULL) {
		data->async_cb(dev, status, data->user_data);
	} else {
#endif /*CONFIG_I2C_CALLBACK*/
		data->xfer_status = status;
		k_sem_give(&data->sync_sem);
#if defined(CONFIG_I2C_CALLBACK)
	}
#endif /*CONFIG_I2C_CALLBACK*/

	k_sem_give(&data->lock);
}

/* Handles I2C errors: stops DMA/interrupts */
static void i2c_handle_error(const struct device *dev, int error_status)
{
	uint16_t hw_status = I2CM(dev).SERCOM_STATUS;

	LOG_ERR("I2C error: status=0x%04X", hw_status);

#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);

	if (data->dma_active) {
		const struct i2c_mchp_dev_config *cfg = DEV_CFG(dev);
		struct i2c_msg *msg = I2C_CURR_MSG(data);
		uint32_t ch =
			i2c_is_read_op(msg) ? cfg->dma.rx_dma_channel : cfg->dma.tx_dma_channel;
		dma_stop(cfg->dma.dma_dev, ch);
		data->dma_active = false;
	}
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/

	I2CM(dev).SERCOM_INTENCLR = SERCOM_I2CM_INTENCLR_Msk;
	I2CM(dev).SERCOM_INTFLAG = SERCOM_I2CM_INTFLAG_ERROR_Msk;

	I2CM(dev).SERCOM_STATUS =
		hw_status & (SERCOM_I2CM_STATUS_LENERR_Msk | SERCOM_I2CM_STATUS_SEXTTOUT_Msk |
			     SERCOM_I2CM_STATUS_MEXTTOUT_Msk | SERCOM_I2CM_STATUS_LOWTOUT_Msk |
			     SERCOM_I2CM_STATUS_BUSERR_Msk);

	i2c_send_stop(dev);
	i2c_xfer_complete(dev, error_status);
}

/* Validates I2C message array and sets RESTART flags */
static int i2c_validate_msgs(struct i2c_msg *msgs, uint8_t num_msgs)
{
	if ((msgs == NULL) || (num_msgs == 0)) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < num_msgs; i++) {

		if ((msgs[i].buf == NULL) || (msgs[i].len == 0)) {
			return -EINVAL;
		}

		if ((i < num_msgs - 1) && (i2c_is_stop_op(&msgs[i]))) {
			return -EINVAL;
		}
	}

	msgs[0].flags |= I2C_MSG_RESTART;

	for (uint8_t i = 1; i < num_msgs; i++) {
		uint8_t prev_dir = msgs[i - 1].flags & I2C_MSG_RW_MASK;
		uint8_t curr_dir = msgs[i].flags & I2C_MSG_RW_MASK;

		if (prev_dir != curr_dir) {
			msgs[i].flags |= I2C_MSG_RESTART;
		}
	}

	return 0;
}

#if !defined(CONFIG_I2C_MCHP_DMA_DRIVEN)

/* Advances to next byte in I2C transfer, returns true when segment complete */
static bool i2c_advance_byte(struct i2c_mchp_dev_data *data)
{
	struct i2c_msg *msg = I2C_CURR_MSG(data);

	data->byte_idx++;
	data->seg_remaining--;

	if (data->byte_idx >= msg->len) {
		if (data->seg_remaining > 0) {
			data->msg_idx++;
		}

		data->byte_idx = 0;
	}

	return (data->seg_remaining == 0);
}

/* Writes a single byte to I2C data register */
static void i2c_write_byte(const struct device *dev, uint8_t byte)
{
	I2CM(dev).SERCOM_DATA = byte;
	if (!I2C_WAIT_SYNC_M(dev, SERCOM_I2CM_SYNCBUSY_SYSOP_Msk)) {
		LOG_ERR("DATA write sync timeout");
	}
}
#endif /*(!CONFIG_I2C_MCHP_DMA_DRIVEN)*/

#if !defined(CONFIG_I2C_MCHP_DMA_DRIVEN)

/* Starts I2C transfer via interrupt mode */
static void i2c_int_start_xfer(const struct device *dev, uint16_t addr, bool is_read)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	struct i2c_msg *msg = I2C_CURR_MSG(data);
	uint8_t flag = (is_read) ? SERCOM_I2CM_INTFLAG_SB_Msk : SERCOM_I2CM_INTFLAG_MB_Msk;

	I2CM(dev).SERCOM_INTFLAG = flag;
	I2CM(dev).SERCOM_INTENSET = flag;

	if (MSG_HAS_RESTART(msg) != 0) {
		i2c_write_addr(dev, addr, is_read);
	}
}

/* Handles interrupt-driven I2C byte read operation */
static void i2c_int_read_byte(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	struct i2c_msg *msg = I2C_CURR_MSG(data);

	if ((data->seg_remaining == 1) && (i2c_is_stop_op(msg))) {
		i2c_set_ack(dev, false);
		i2c_send_stop(dev);
	}

	*I2C_CURR_BYTE_PTR(data) = I2CM(dev).SERCOM_DATA;
	bool seg_done = i2c_advance_byte(data);

	if (seg_done) {
		I2CM(dev).SERCOM_INTENCLR = SERCOM_I2CM_INTENCLR_SB_Msk;

		if (data->next_seg_idx < data->num_msgs) {
			i2c_init_segment(data);
			bool is_read = i2c_is_read_op(I2C_CURR_MSG(data));

			i2c_int_start_xfer(dev, data->target_addr << 1, is_read);
		} else {
			i2c_xfer_complete(dev, 0);
		}
	}
}

/* Handles interrupt-driven I2C byte write operation */
static void i2c_int_write_byte(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);

	if (i2c_got_nack(dev)) {
		I2CM(dev).SERCOM_INTENCLR = SERCOM_I2CM_INTENCLR_MB_Msk;
		i2c_send_stop(dev);
		i2c_xfer_complete(dev, -EIO);
		return;
	}

	if (data->seg_remaining == 0) {
		struct i2c_msg *msg = I2C_CURR_MSG(data);

		I2CM(dev).SERCOM_INTENCLR = SERCOM_I2CM_INTENCLR_MB_Msk;

		if (i2c_is_stop_op(msg)) {
			i2c_send_stop(dev);
		}

		if (data->next_seg_idx < data->num_msgs) {
			i2c_init_segment(data);
			bool is_read = i2c_is_read_op(I2C_CURR_MSG(data));

			i2c_int_start_xfer(dev, data->target_addr << 1, is_read);
		} else {
			i2c_xfer_complete(dev, 0);
		}
		return;
	}

	i2c_write_byte(dev, *I2C_CURR_BYTE_PTR(data));
	i2c_advance_byte(data);
}
#endif /* !CONFIG_I2C_MCHP_DMA_DRIVEN */

/* starts I2C transfer with DMA/interrupt, waits if sync, returns immediately if async */
static int i2c_transfer_internal(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				 uint16_t addr, i2c_callback_t cb, void *userdata)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	int ret;

	ret = i2c_validate_msgs(msgs, num_msgs);
	if (ret != 0) {
		LOG_ERR("Message validation failed: %d", ret);
		return ret;
	}

	k_sem_take(&data->lock, K_FOREVER);

	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->target_addr = addr;
#if defined(CONFIG_I2C_CALLBACK)
	data->async_cb = cb;
	data->user_data = userdata;
#endif /*CONFIG_I2C_CALLBACK*/
	data->dev = dev;
	data->next_seg_idx = 0;
	data->xfer_status = 0;

	i2c_init_segment(data);

	bool is_read = i2c_is_read_op(I2C_CURR_MSG(data));
	uint16_t hw_addr = addr << 1;

	I2CM(dev).SERCOM_INTENSET = SERCOM_I2CM_INTENSET_ERROR_Msk;

#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
	if (i2c_dma_setup(dev, is_read) != 0) {
		k_sem_give(&data->lock);
		return -EIO;
	}

	uint32_t ch =
		(is_read) ? DEV_CFG(dev)->dma.rx_dma_channel : DEV_CFG(dev)->dma.tx_dma_channel;

	dma_start(DEV_CFG(dev)->dma.dma_dev, ch);
	data->dma_active = true;
	i2c_write_addr(dev, hw_addr, is_read);
#else
	i2c_int_start_xfer(dev, hw_addr, is_read);
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/

	if (cb == NULL) {
		ret = k_sem_take(&data->sync_sem, I2C_TRANSFER_TIMEOUT_MSEC);
		if (ret != 0) {
			LOG_ERR("Transfer timeout");
			i2c_handle_error(dev, -ETIMEDOUT);
		}

		return data->xfer_status;
	}

	return 0;
}

#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)

/* DMA completion callback - advances to next segment or completes transfer */
static void i2c_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel, int status)
{
	struct i2c_mchp_dev_data *data = arg;
	const struct device *dev = data->dev;
	const struct i2c_mchp_dev_config *cfg = DEV_CFG(dev);
	struct i2c_msg *msg = I2C_CURR_MSG(data);
	bool is_read = i2c_is_read_op(msg);
	uint32_t prev_msg_len = msg->len;
	bool new_segment = (data->msg_idx + 1 >= data->next_seg_idx);

	ARG_UNUSED(channel);

	data->dma_active = false;

	if (status < 0) {
		i2c_handle_error(dev, status);
		return;
	}

	if (i2c_is_stop_op(msg)) {

		if (is_read) {
			I2CM(dev).SERCOM_INTENSET = SERCOM_I2CM_INTENSET_SB_Msk;
		} else {
			I2CM(dev).SERCOM_INTENSET = SERCOM_I2CM_INTENSET_MB_Msk;
		}

		return;
	}

	data->msg_idx++;
	data->byte_idx = 0;

	if (new_segment) {
		i2c_init_segment(data);
	} else {
		data->seg_remaining -= prev_msg_len;
	}

	msg = I2C_CURR_MSG(data);
	is_read = i2c_is_read_op(msg);

	if (i2c_dma_setup(dev, is_read) != 0) {
		i2c_xfer_complete(dev, -EIO);
		return;
	}

	uint32_t ch = (is_read) ? cfg->dma.rx_dma_channel : cfg->dma.tx_dma_channel;

	dma_start(cfg->dma.dma_dev, ch);
	data->dma_active = true;

	if ((new_segment) || (MSG_HAS_RESTART(msg) != 0)) {
		i2c_write_addr(dev, data->target_addr << 1, is_read);
	}
}

/* Configures DMA channel for I2C read/write transfer */
static int i2c_dma_setup(const struct device *dev, bool is_read)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	const struct i2c_mchp_dev_config *cfg = DEV_CFG(dev);
	struct i2c_msg *msg = I2C_CURR_MSG(data);
	struct dma_config dma_cfg = {0};
	struct dma_block_config blk = {0};
	uint32_t ch;
	int ret;

	dma_cfg.channel_direction = (is_read) ? PERIPHERAL_TO_MEMORY : MEMORY_TO_PERIPHERAL;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.user_data = data;
	dma_cfg.dma_callback = i2c_dma_callback;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &blk;

	if (is_read) {
		blk.block_size = msg->len - 1; /* Last byte read manually */
		blk.source_address = (uint32_t)&cfg->regs->I2CM.SERCOM_DATA;
		blk.dest_address = (uint32_t)msg->buf;
		blk.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		blk.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		dma_cfg.dma_slot = cfg->dma.rx_dma_request;
		ch = cfg->dma.rx_dma_channel;
	} else {
		blk.block_size = msg->len;
		blk.source_address = (uint32_t)msg->buf;
		blk.dest_address = (uint32_t)&cfg->regs->I2CM.SERCOM_DATA;
		blk.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		blk.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		dma_cfg.dma_slot = cfg->dma.tx_dma_request;
		ch = cfg->dma.tx_dma_channel;
	}

	ret = dma_config(cfg->dma.dma_dev, ch, &dma_cfg);

	if (ret != 0) {
		LOG_ERR("DMA config failed: %d", ret);
	}
	return ret;
}

/* Handles DMA transmission completion */
static void i2c_dma_tx_complete(const struct device *dev)
{
	I2CM(dev).SERCOM_INTENCLR = SERCOM_I2CM_INTENCLR_MB_Msk;
	i2c_send_stop(dev);
	i2c_xfer_complete(dev, 0);
}

/* Handles DMA reception completion */
static void i2c_dma_rx_complete(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	struct i2c_msg *msg = I2C_CURR_MSG(data);

	if (i2c_is_stop_op(msg)) {
		i2c_set_ack(dev, false);
		i2c_send_stop(dev);
		msg->buf[msg->len - 1] = I2CM(dev).SERCOM_DATA;
	}

	I2CM(dev).SERCOM_INTENCLR = SERCOM_I2CM_INTENCLR_SB_Msk;
	i2c_xfer_complete(dev, 0);
}
#endif /* CONFIG_I2C_MCHP_DMA_DRIVEN */

#if defined(CONFIG_I2C_TARGET)

/* Clears I2C target interrupt flags */
static inline void i2c_target_clear_intflags(const struct device *dev, uint8_t mask)
{
	I2CS(dev).SERCOM_INTFLAG = mask & SERCOM_I2CS_INTFLAG_Msk;
}

/* Clears I2C target status bits */
static inline void i2c_target_clear_status(const struct device *dev, uint16_t mask)
{
	const uint16_t clearable = SERCOM_I2CS_STATUS_BUSERR_Msk | SERCOM_I2CS_STATUS_COLL_Msk |
				   SERCOM_I2CS_STATUS_LOWTOUT_Msk | SERCOM_I2CS_STATUS_SEXTTOUT_Msk;

	I2CS(dev).SERCOM_STATUS = mask & clearable;
}

/* Sends I2C target mode command */
static void i2c_target_send_cmd(const struct device *dev, enum i2c_target_cmd cmd)
{
	uint32_t ctrlb = I2CS(dev).SERCOM_CTRLB;

	ctrlb &= ~(SERCOM_I2CS_CTRLB_CMD_Msk);

	switch (cmd) {
	case I2C_TARGET_CMD_ACK:
		ctrlb = (ctrlb | SERCOM_I2CS_CTRLB_CMD(0x3)) & (~SERCOM_I2CS_CTRLB_ACKACT_Msk);
		break;

	case I2C_TARGET_CMD_NACK:
		ctrlb |= SERCOM_I2CS_CTRLB_CMD(0x3) | SERCOM_I2CS_CTRLB_ACKACT_Msk;
		break;

	case I2C_TARGET_RECEIVE_ACK_NAK:
		ctrlb |= SERCOM_I2CS_CTRLB_CMD(0x3);
		break;

	case I2C_TARGET_CMD_WAIT_START:
		ctrlb = (ctrlb & ~SERCOM_I2CS_CTRLB_CMD_Msk) | SERCOM_I2CS_CTRLB_CMD(0x2);
		break;

	default:
		return;
	}

	I2CS(dev).SERCOM_CTRLB = ctrlb;
}

/* Initializes I2C target mode */
static void i2c_setup_target_mode(const struct device *dev)
{
	I2CS(dev).SERCOM_CTRLB =
		(I2CS(dev).SERCOM_CTRLB & ~SERCOM_I2CS_CTRLB_SMEN_Msk) | SERCOM_I2CS_CTRLB_SMEN(1);

	uint32_t ctrla = I2CS(dev).SERCOM_CTRLA;

	ctrla &= ~SERCOM_I2CS_CTRLA_MODE_Msk;
	ctrla |= SERCOM_I2CS_CTRLA_MODE(SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE_Val) |
		 SERCOM_I2CS_CTRLA_SDAHOLD(SERCOM_I2CS_CTRLA_SDAHOLD_75NS) |
		 SERCOM_I2CS_CTRLA_SPEED(SERCOM_I2CS_SPEED_FAST_MODE_PLUS);
	I2CS(dev).SERCOM_CTRLA = ctrla;
}

/* Sets I2C target address */
static void i2c_target_set_addr(const struct device *dev, uint16_t addr)
{
	I2CS(dev).SERCOM_ADDR =
		(I2CS(dev).SERCOM_ADDR & ~SERCOM_I2CS_ADDR_ADDR_Msk) | SERCOM_I2CS_ADDR_ADDR(addr);
}

/* Handles I2C target address match interrupt event */
static void i2c_target_on_addr_match(const struct device *dev, uint16_t status)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	const struct i2c_target_callbacks *cb = &data->tgt_cb;

	i2c_target_send_cmd(dev, I2C_TARGET_CMD_ACK);
	data->first_read = true;

	if ((status & SERCOM_I2CS_STATUS_DIR_Msk) != 0) {
		if (cb->read_requested != NULL) {
			cb->read_requested(&data->tgt_cfg, &data->tgt_rx_data);
		}
	} else {
		if (cb->write_requested != NULL) {
			cb->write_requested(&data->tgt_cfg);
		}
	}
}

/* Handles I2C target data ready interrupt event */
static void i2c_target_on_data_ready(const struct device *dev, uint16_t status)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	const struct i2c_target_callbacks *cb = &data->tgt_cb;
	int ret = -ENOTSUP;

	if ((status & SERCOM_I2CS_STATUS_DIR_Msk) != 0) {
		/* Host is reading from us */
		if ((data->first_read) || ((status & SERCOM_I2CS_STATUS_RXNACK_Msk) == 0)) {
			I2CS(dev).SERCOM_DATA = data->tgt_rx_data;

			data->first_read = false;
			i2c_target_send_cmd(dev, I2C_TARGET_RECEIVE_ACK_NAK);

			/* Prepare next byte */
			if (cb->read_processed != NULL) {
				cb->read_processed(&data->tgt_cfg, &data->tgt_rx_data);
			}
		} else {
			/* Host NACKed */
			i2c_target_send_cmd(dev, I2C_TARGET_CMD_WAIT_START);
		}
	} else {
		/* Host is writing to us - read the byte first, then ACK */
		data->tgt_rx_data = I2CS(dev).SERCOM_DATA;

		if (cb->write_received != NULL) {
			ret = cb->write_received(&data->tgt_cfg, data->tgt_rx_data);
		}
		i2c_target_send_cmd(dev, ret == 0 ? I2C_TARGET_CMD_ACK : I2C_TARGET_CMD_NACK);
	}
}

/* Handles I2C target STOP condition interrupt event */
static void i2c_target_on_stop(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	const struct i2c_target_callbacks *cb = &data->tgt_cb;

	i2c_target_clear_intflags(dev, SERCOM_I2CS_INTFLAG_PREC_Msk);

	if (cb->stop != NULL) {
		cb->stop(&data->tgt_cfg);
	}
}

/* I2C target mode ISR */
static void i2c_target_isr(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	uint8_t intflags = I2CS(dev).SERCOM_INTFLAG & SERCOM_I2CS_INTFLAG_Msk;
	uint16_t status = I2CS(dev).SERCOM_STATUS & SERCOM_I2CS_STATUS_Msk;

	if ((intflags & SERCOM_I2CS_INTFLAG_DRDY_Msk) != 0) {
		i2c_target_on_data_ready(dev, status);
	}

	if ((intflags & SERCOM_I2CS_INTFLAG_AMATCH_Msk) != 0) {
		i2c_target_on_addr_match(dev, status);
	}

	if ((intflags & SERCOM_I2CS_INTFLAG_PREC_Msk) != 0) {
		i2c_target_on_stop(dev);
	}

	if ((intflags & SERCOM_I2CS_INTFLAG_ERROR_Msk) != 0) {
		i2c_target_clear_intflags(dev, SERCOM_I2CS_INTFLAG_ERROR_Msk);
		LOG_ERR("Target error");
		if (data->tgt_cb.stop != NULL) {
			data->tgt_cb.stop(&data->tgt_cfg);
		}
	}

	i2c_target_clear_status(dev, status);
}
#endif /* CONFIG_I2C_TARGET */

/* I2C interrupt handler - routes to target ISR or handles controller mode interrupts */
static void i2c_mchp_isr(const struct device *dev)
{
#if defined(CONFIG_I2C_TARGET)
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);

	if (data->target_mode) {
		i2c_target_isr(dev);
		return;
	}
#endif /*CONFIG_I2C_TARGET*/

	uint8_t intflags = I2CM(dev).SERCOM_INTFLAG & SERCOM_I2CM_INTFLAG_Msk;

	if ((intflags & SERCOM_I2CM_INTFLAG_ERROR_Msk) != 0) {
		i2c_handle_error(dev, -EIO);
		return;
	}

	if ((intflags & SERCOM_I2CM_INTFLAG_MB_Msk) != 0) {
#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
		i2c_dma_tx_complete(dev);
#else
		i2c_int_write_byte(dev);
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/
	}

	if ((intflags & SERCOM_I2CM_INTFLAG_SB_Msk) != 0) {
#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
		i2c_dma_rx_complete(dev);
#else
		i2c_int_read_byte(dev);
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/
	}
}

/* Synchronous blocking I2C transfer (waits for completion) */
static int i2c_mchp_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			     uint16_t addr)
{
	return i2c_transfer_internal(dev, msgs, num_msgs, addr, NULL, NULL);
}

#if defined(CONFIG_I2C_CALLBACK)

/* Asynchronous I2C transfer with callback (returns immediately) */
static int i2c_mchp_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr, i2c_callback_t cb, void *userdata)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	return i2c_transfer_internal(dev, msgs, num_msgs, addr, cb, userdata);
}
#endif /* CONFIG_I2C_CALLBACK */

/* Performs I2C bus recovery: force bus state to IDLE and re-init synchronization primitives */
static int i2c_mchp_recover(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	const struct i2c_mchp_dev_config *cfg = DEV_CFG(dev);
	int ret;

	i2c_enable(dev, false);
	I2CM(dev).SERCOM_INTENCLR = SERCOM_I2CM_INTENCLR_Msk;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl: %d", ret);
		return ret;
	}

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->sync_sem, 0, 1);

	i2c_enable(dev, true);
	i2c_set_bus_idle(dev);

	return 0;
}

/* Gets current I2C device configuration */
static int i2c_mchp_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);

	if (config == NULL) {
		LOG_ERR("NULL config pointer");
		return -EINVAL;
	}

	if (I2C_SPEED_GET(data->dev_config) == 0) {
		LOG_ERR("Device not configured");
		return -ENODATA;
	}

	*config = data->dev_config;
	return 0;
}

/* Configures I2C controller mode and speed settings */
static int i2c_mchp_configure(const struct device *dev, uint32_t config)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	int ret;

	if (data->target_mode) {
		LOG_ERR("Cannot configure in target mode");
		return -EBUSY;
	}

	k_sem_take(&data->lock, K_FOREVER);
	i2c_enable(dev, false);

	if ((config & I2C_MODE_CONTROLLER) != 0) {
		i2c_setup_controller_mode(dev);
	}

	ret = i2c_apply_speed(dev, config);
	if (ret != 0) {
		k_sem_give(&data->lock);
		return ret;
	}

	data->dev_config = config;
	i2c_enable(dev, true);
	i2c_set_bus_idle(dev);

	k_sem_give(&data->lock);
	return 0;
}

#if defined(CONFIG_I2C_TARGET)

/* Registers I2C as target device */
static int i2c_mchp_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);

	if (data->target_mode) {
		LOG_ERR("Already in target mode");
		return -EBUSY;
	}
	if ((cfg == NULL) || (cfg->callbacks == NULL)) {
		LOG_ERR("Invalid config");
		return -EINVAL;
	}
	if (cfg->address == 0) {
		LOG_ERR("Invalid address 0x00");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	data->tgt_cfg.address = cfg->address;
	data->tgt_cb = *cfg->callbacks;

	i2c_enable(dev, false);
	I2CS(dev).SERCOM_INTENCLR = SERCOM_I2CS_INTENCLR_Msk;
	i2c_setup_target_mode(dev);
	i2c_target_set_addr(dev, cfg->address);
	I2CS(dev).SERCOM_INTENSET = SERCOM_I2CS_INTENSET_Msk;

	data->target_mode = true;
	i2c_set_runstandby(dev);
	i2c_enable(dev, true);

	k_sem_give(&data->lock);
	return 0;
}

/* Unregisters I2C from target mode, returns to controller mode */
static int i2c_mchp_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);

	if (cfg == NULL) {
		LOG_ERR("NULL config");
		return -EINVAL;
	}
	if (!data->target_mode) {
		LOG_ERR("Not in target mode");
		return -EBUSY;
	}
	if (data->tgt_cfg.address != cfg->address) {
		LOG_ERR("Address mismatch");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	(void)memset(&data->tgt_cb, 0, sizeof(data->tgt_cb));

	I2CS(dev).SERCOM_INTENCLR = SERCOM_I2CS_INTENCLR_Msk;
	i2c_enable(dev, false);
	i2c_target_set_addr(dev, 0);

	data->target_mode = false;
	data->tgt_cfg.address = 0;
	data->tgt_cfg.callbacks = NULL;

	i2c_setup_controller_mode(dev);
	i2c_set_runstandby(dev);
	i2c_enable(dev, true);
	i2c_set_bus_idle(dev);

	k_sem_give(&data->lock);
	return 0;
}
#endif /* CONFIG_I2C_TARGET */

/* Initializes I2C peripheral */
static int i2c_mchp_init(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = DEV_DATA(dev);
	const struct i2c_mchp_dev_config *cfg = DEV_CFG(dev);
	int ret;

	ret = clock_control_on(DEVICE_DT_GET(DT_NODELABEL(clock)), cfg->clk.gclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable GCLK: %d", ret);
		return ret;
	}

	ret = clock_control_on(DEVICE_DT_GET(DT_NODELABEL(clock)), cfg->clk.mclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable MCLK: %d", ret);
		return ret;
	}

	i2c_reset(dev);
	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->sync_sem, 0, 1);
	data->target_mode = false;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl: %d", ret);
		return ret;
	}

	ret = i2c_configure(dev, i2c_map_dt_bitrate(cfg->bitrate));
	if (ret != 0) {
		LOG_ERR("Failed to configure: %d", ret);
		return ret;
	}

#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
	data->dev = dev;

	if (cfg->dma.tx_dma_channel == 0xFF || cfg->dma.rx_dma_channel == 0xFF) {
		LOG_ERR("Invalid DMA channel configuration");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->dma.dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	int ch = cfg->dma.tx_dma_channel;

	ret = dma_request_channel(cfg->dma.dma_dev, &ch);
	if (ret < 0) {
		LOG_ERR("TX DMA channel %d request failed: %d", cfg->dma.tx_dma_channel, ret);
		return ret;
	}

	ch = cfg->dma.rx_dma_channel;
	ret = dma_request_channel(cfg->dma.dma_dev, &ch);
	if (ret < 0) {
		LOG_ERR("RX DMA channel %d request failed: %d", cfg->dma.rx_dma_channel, ret);
		return ret;
	}
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/

	i2c_enable(dev, false);
	cfg->irq_config_func(dev);
	i2c_set_runstandby(dev);
	i2c_enable(dev, true);
	i2c_set_bus_idle(dev);

	return 0;
}

static DEVICE_API(i2c, i2c_mchp_api) = {
	.configure = i2c_mchp_configure,
	.get_config = i2c_mchp_get_config,
	.transfer = i2c_mchp_transfer,
#if defined(CONFIG_I2C_TARGET)
	.target_register = i2c_mchp_target_register,
	.target_unregister = i2c_mchp_target_unregister,
#endif /*CONFIG_I2C_TARGET*/
#if defined(CONFIG_I2C_CALLBACK)
	.transfer_cb = i2c_mchp_transfer_cb,
#endif /*CONFIG_I2C_CALLBACK*/
	.recover_bus = i2c_mchp_recover,
};

#define I2C_MCHP_IRQ_CONNECT(n, idx)                                                               \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq), DT_INST_IRQ_BY_IDX(n, idx, priority), \
			    i2c_mchp_isr, DEVICE_DT_INST_GET(n), 0);                               \
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));                                       \
	} while (false)

#if DT_INST_IRQ_HAS_IDX(0, 3)
#define I2C_MCHP_IRQ_HANDLER(n)                                                                    \
	static void i2c_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		I2C_MCHP_IRQ_CONNECT(n, 0);                                                        \
		I2C_MCHP_IRQ_CONNECT(n, 1);                                                        \
		I2C_MCHP_IRQ_CONNECT(n, 2);                                                        \
		I2C_MCHP_IRQ_CONNECT(n, 3);                                                        \
	}
#else
#define I2C_MCHP_IRQ_HANDLER(n)                                                                    \
	static void i2c_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		I2C_MCHP_IRQ_CONNECT(n, 0);                                                        \
	}
#endif

#define I2C_MCHP_CLOCK_INIT(n)                                                                     \
	.clk.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),                 \
	.clk.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),

#if defined(CONFIG_I2C_MCHP_DMA_DRIVEN)
#define I2C_MCHP_DMA_INIT(n)                                                                       \
	.dma.dma_dev = DEVICE_DT_GET(MCHP_DT_INST_DMA_CTLR(n, tx)),                                \
	.dma.tx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, tx),                                     \
	.dma.tx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, tx),                                     \
	.dma.rx_dma_request = MCHP_DT_INST_DMA_TRIGSRC(n, rx),                                     \
	.dma.rx_dma_channel = MCHP_DT_INST_DMA_CHANNEL(n, rx),
#else
#define I2C_MCHP_DMA_INIT(n)
#endif /*CONFIG_I2C_MCHP_DMA_DRIVEN*/

#define I2C_MCHP_CONFIG_INIT(n)                                                                    \
	static const struct i2c_mchp_dev_config i2c_mchp_cfg_##n = {                               \
		.regs = (sercom_registers_t *)DT_INST_REG_ADDR(n),                                 \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		.irq_config_func = i2c_mchp_irq_config_##n,                                        \
		.run_in_standby = DT_INST_PROP(n, run_in_standby_en),                              \
		I2C_MCHP_CLOCK_INIT(n) I2C_MCHP_DMA_INIT(n)}

#define I2C_MCHP_DEVICE_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void i2c_mchp_irq_config_##n(const struct device *dev);                             \
	I2C_MCHP_CONFIG_INIT(n);                                                                   \
	static struct i2c_mchp_dev_data i2c_mchp_data_##n;                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_mchp_init, NULL, &i2c_mchp_data_##n, &i2c_mchp_cfg_##n,   \
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &i2c_mchp_api);           \
	I2C_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(I2C_MCHP_DEVICE_INIT)
