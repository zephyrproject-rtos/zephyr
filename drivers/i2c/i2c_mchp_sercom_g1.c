/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sercom_g1_i2c

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

LOG_MODULE_REGISTER(i2c_mchp_sercom_g1, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#define I2C_MCHP_SUCCESS                    0
#define I2C_MCHP_MESSAGE_DIR_READ_MASK      1
#define I2C_MCHP_SPEED_FAST                 400000
#define I2C_MCHP_SPEED_FAST_PLUS            1000000
#define I2C_MCHP_SPEED_HIGH_SPEED           3400000
#define I2C_BAUD_LOW_HIGH_MAX               382U
#define I2C_MCHP_START_CONDITION_SETUP_TIME (100.0f / 1000000000.0f)
#define I2C_INVALID_ADDR                    0x00
#define I2C_MESSAGE_DIR_READ                1U
#ifdef CONFIG_I2C_MCHP_TRANSFER_TIMEOUT
#define I2C_TRANSFER_TIMEOUT_MSEC K_MSEC(CONFIG_I2C_MCHP_TRANSFER_TIMEOUT)
#else
#define I2C_TRANSFER_TIMEOUT_MSEC K_FOREVER
#endif /*CONFIG_I2C_MCHP_TRANSFER_TIMEOUT*/
#define I2C_MCHP_TARGET_ACK_STATUS_RECEIVED_ACK  0
#define I2C_MCHP_TARGET_ACK_STATUS_RECEIVED_NACK 1

/* Timeout values for WAIT_FOR macro*/
#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

enum i2c_mchp_target_cmd {
	I2C_MCHP_TARGET_COMMAND_SEND_ACK = 0,
	I2C_MCHP_TARGET_COMMAND_SEND_NACK,
	I2C_MCHP_TARGET_COMMAND_RECEIVE_ACK_NAK,
	I2C_MCHP_TARGET_COMMAND_WAIT_FOR_START
};

struct i2c_mchp_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
};

struct i2c_mchp_dev_config {
	sercom_registers_t *regs;
	struct i2c_mchp_clock i2c_clock;
	const struct pinctrl_dev_config *pcfg;
	uint32_t bitrate;
	void (*irq_config_func)(const struct device *dev);
	uint8_t run_in_standby;
};

struct i2c_mchp_msg {
	uint8_t *buffer;
	uint32_t size;
	uint16_t status;
};

struct i2c_mchp_dev_data {
	const struct device *dev;
	struct k_mutex i2c_bus_mutex;
	struct k_sem i2c_sync_sem;
	struct i2c_mchp_msg current_msg;
	struct i2c_msg *msgs_array;
	uint8_t num_msgs;
	bool target_mode;
	uint32_t dev_config;
	uint32_t target_addr;
	uint8_t msg_index;
#ifdef CONFIG_I2C_CALLBACK
	i2c_callback_t i2c_async_callback;
	void *user_data;
#endif /*CONFIG_I2C_CALLBACK*/
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config target_config;
	struct i2c_target_callbacks target_callbacks;
	uint8_t rx_tx_data;
#endif /*CONFIG_I2C_TARGET*/
	bool firstReadAfterAddrMatch;
};

static void i2c_swrst(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	i2c_regs->I2CM.SERCOM_CTRLA |= SERCOM_I2CM_CTRLA_SWRST(1);

	/* Wait for synchronization */
	if (WAIT_FOR(((i2c_regs->I2CM.SERCOM_SYNCBUSY & SERCOM_I2CM_SYNCBUSY_SWRST_Msk) == 0),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for I2C SYNCBUSY SWRST clear");
	}
}

static uint8_t i2c_byte_read(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;
	uint8_t value;

	if ((i2c_regs->I2CM.SERCOM_CTRLA & SERCOM_I2CM_CTRLA_MODE_I2C_MASTER) ==
	    SERCOM_I2CM_CTRLA_MODE_I2C_MASTER) {
		value = (uint8_t)i2c_regs->I2CM.SERCOM_DATA;
	} else {
		value = (uint8_t)i2c_regs->I2CS.SERCOM_DATA;
	}

	return value;
}

static void i2c_byte_write(const struct device *dev, uint8_t data)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((i2c_regs->I2CM.SERCOM_CTRLA & SERCOM_I2CM_CTRLA_MODE_I2C_MASTER) ==
	    SERCOM_I2CM_CTRLA_MODE_I2C_MASTER) {
		i2c_regs->I2CM.SERCOM_DATA = data;

		/* Wait for synchronization */
		if (WAIT_FOR(((i2c_regs->I2CM.SERCOM_SYNCBUSY & SERCOM_I2CM_SYNCBUSY_SYSOP_Msk) ==
			      0),
			     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
			LOG_ERR("Timeout waiting for I2C SYNCBUSY SYSOP clear");
		}

	} else {
		i2c_regs->I2CS.SERCOM_DATA = data;
	}
}

static void i2c_controller_enable(const struct device *dev, bool enable)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if (enable == true) {
		i2c_regs->I2CM.SERCOM_CTRLA |= SERCOM_I2CM_CTRLA_ENABLE(1);
	} else {
		i2c_regs->I2CM.SERCOM_CTRLA &= ~SERCOM_I2CM_CTRLA_ENABLE(1);
	}

	/* Wait for synchronization */
	if (WAIT_FOR(((i2c_regs->I2CM.SERCOM_SYNCBUSY & SERCOM_I2CM_SYNCBUSY_ENABLE_Msk) == 0),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for I2C SYNCBUSY ENABLE clear");
	}
}

static void i2c_controller_runstandby_enable(const struct device *dev)
{
	const struct i2c_mchp_dev_config *const i2c_cfg = dev->config;
	sercom_registers_t *i2c_regs = i2c_cfg->regs;
	uint32_t reg32_val = i2c_regs->I2CM.SERCOM_CTRLA;

	reg32_val &= ~SERCOM_I2CM_CTRLA_RUNSTDBY_Msk;
	reg32_val |= SERCOM_I2CM_CTRLA_RUNSTDBY(i2c_cfg->run_in_standby);
	i2c_regs->I2CM.SERCOM_CTRLA = reg32_val;
}

static inline void i2c_set_controller_auto_ack(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	i2c_regs->I2CM.SERCOM_CTRLB =
		((i2c_regs->I2CM.SERCOM_CTRLB & ~SERCOM_I2CM_CTRLB_ACKACT_Msk) |
		 SERCOM_I2CM_CTRLB_ACKACT(0));
}

static void i2c_set_controller_mode(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	/* Enable i2c device smart mode features */
	i2c_regs->I2CM.SERCOM_CTRLB = ((i2c_regs->I2CM.SERCOM_CTRLB & ~SERCOM_I2CM_CTRLB_SMEN_Msk) |
				       SERCOM_I2CM_CTRLB_SMEN(1));

	i2c_regs->I2CM.SERCOM_CTRLA =
		(i2c_regs->I2CM.SERCOM_CTRLA &
		 ~(SERCOM_I2CM_CTRLA_MODE_Msk | SERCOM_I2CM_CTRLA_INACTOUT_Msk |
		   SERCOM_I2CM_CTRLA_LOWTOUTEN_Msk)) |
		(SERCOM_I2CM_CTRLA_MODE(0x5) | SERCOM_I2CM_CTRLA_LOWTOUTEN(1) |
		 SERCOM_I2CM_CTRLA_INACTOUT(0x3));
}

static void i2c_controller_transfer_stop(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	i2c_regs->I2CM.SERCOM_CTRLB =
		(i2c_regs->I2CM.SERCOM_CTRLB &
		 ~(SERCOM_I2CM_CTRLB_ACKACT_Msk | SERCOM_I2CM_CTRLB_CMD_Msk)) |
		(SERCOM_I2CM_CTRLB_ACKACT(1) | SERCOM_I2CM_CTRLB_CMD(0x3));

	/* Wait for synchronization */
	if (WAIT_FOR(((i2c_regs->I2CM.SERCOM_SYNCBUSY & SERCOM_I2CM_SYNCBUSY_SYSOP_Msk) == 0),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for I2C SYNCBUSY SYSOP clear");
	}
}

static void i2c_set_controller_bus_state_idle(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	i2c_regs->I2CM.SERCOM_STATUS = SERCOM_I2CM_STATUS_BUSSTATE(0x1);

	/* Wait for synchronization */
	if (WAIT_FOR(((i2c_regs->I2CM.SERCOM_SYNCBUSY & SERCOM_I2CM_SYNCBUSY_SYSOP_Msk) == 0),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for I2C SYNCBUSY SYSOP clear");
	}
}

static uint16_t i2c_controller_status_get(const struct device *dev)
{
	uint16_t status_reg_val;
	uint16_t status_flags = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	status_reg_val = i2c_regs->I2CM.SERCOM_STATUS;

	if ((status_reg_val & SERCOM_I2CM_STATUS_BUSERR_Msk) == SERCOM_I2CM_STATUS_BUSERR_Msk) {
		status_flags |= SERCOM_I2CM_STATUS_BUSERR_Msk;
	}
	if ((status_reg_val & SERCOM_I2CM_STATUS_ARBLOST_Msk) == SERCOM_I2CM_STATUS_ARBLOST_Msk) {
		status_flags |= SERCOM_I2CM_STATUS_ARBLOST_Msk;
	}
	if ((status_reg_val & SERCOM_I2CM_STATUS_BUSSTATE_Msk) ==
	    SERCOM_I2CM_STATUS_BUSSTATE_BUSY) {
		status_flags |= SERCOM_I2CM_STATUS_BUSSTATE_BUSY;
	}
	if ((status_reg_val & SERCOM_I2CM_STATUS_MEXTTOUT_Msk) == SERCOM_I2CM_STATUS_MEXTTOUT_Msk) {
		status_flags |= SERCOM_I2CM_STATUS_MEXTTOUT_Msk;
	}
	if ((status_reg_val & SERCOM_I2CM_STATUS_SEXTTOUT_Msk) == SERCOM_I2CM_STATUS_SEXTTOUT_Msk) {
		status_flags |= SERCOM_I2CM_STATUS_SEXTTOUT_Msk;
	}
	if ((status_reg_val & SERCOM_I2CM_STATUS_LOWTOUT_Msk) == SERCOM_I2CM_STATUS_LOWTOUT_Msk) {
		status_flags |= SERCOM_I2CM_STATUS_LOWTOUT_Msk;
	}
	if ((status_reg_val & SERCOM_I2CM_STATUS_LENERR_Msk) == SERCOM_I2CM_STATUS_LENERR_Msk) {
		status_flags |= SERCOM_I2CM_STATUS_LENERR_Msk;
	}

	return status_flags;
}

static void i2c_controller_status_clear(const struct device *dev, uint16_t status_flags)
{
	const struct i2c_mchp_dev_config *const cfg = dev->config;
	sercom_registers_t *i2c_regs = cfg->regs;
	uint16_t reg_val = i2c_regs->I2CM.SERCOM_STATUS;

	if ((status_flags & SERCOM_I2CM_STATUS_BUSERR_Msk) == SERCOM_I2CM_STATUS_BUSERR_Msk) {
		reg_val |= SERCOM_I2CM_STATUS_BUSERR(1);
	}
	if ((status_flags & SERCOM_I2CM_STATUS_ARBLOST_Msk) == SERCOM_I2CM_STATUS_ARBLOST_Msk) {
		reg_val |= SERCOM_I2CM_STATUS_ARBLOST(1);
	}
	if ((status_flags & SERCOM_I2CM_STATUS_BUSSTATE_Msk) == SERCOM_I2CM_STATUS_BUSSTATE_Msk) {
		reg_val |= SERCOM_I2CM_STATUS_BUSSTATE(SERCOM_I2CM_STATUS_BUSSTATE_IDLE_Val);
	}
	if ((status_flags & SERCOM_I2CM_STATUS_MEXTTOUT_Msk) == SERCOM_I2CM_STATUS_MEXTTOUT_Msk) {
		reg_val |= SERCOM_I2CM_STATUS_MEXTTOUT(1);
	}
	if ((status_flags & SERCOM_I2CM_STATUS_SEXTTOUT_Msk) == SERCOM_I2CM_STATUS_SEXTTOUT_Msk) {
		reg_val |= SERCOM_I2CM_STATUS_SEXTTOUT(1);
	}
	if ((status_flags & SERCOM_I2CM_STATUS_LOWTOUT_Msk) == SERCOM_I2CM_STATUS_LOWTOUT_Msk) {
		reg_val |= SERCOM_I2CM_STATUS_LOWTOUT(1);
	}
	if ((status_flags & SERCOM_I2CM_STATUS_LENERR_Msk) == SERCOM_I2CM_STATUS_LENERR_Msk) {
		reg_val |= SERCOM_I2CM_STATUS_LENERR(1);
	}

	i2c_regs->I2CM.SERCOM_STATUS = reg_val;
}

static void i2c_controller_int_enable(const struct device *dev, uint8_t int_enable_mask)
{
	uint8_t int_enable_flags = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((int_enable_mask & SERCOM_I2CM_INTENSET_MB_Msk) == SERCOM_I2CM_INTENSET_MB_Msk) {
		int_enable_flags |= SERCOM_I2CM_INTENSET_MB(1);
	}
	if ((int_enable_mask & SERCOM_I2CM_INTENSET_SB_Msk) == SERCOM_I2CM_INTENSET_SB_Msk) {
		int_enable_flags |= SERCOM_I2CM_INTENSET_SB(1);
	}
	if ((int_enable_mask & SERCOM_I2CM_INTENSET_ERROR_Msk) == SERCOM_I2CM_INTENSET_ERROR_Msk) {
		int_enable_flags |= SERCOM_I2CM_INTENSET_ERROR(1);
	}

	i2c_regs->I2CM.SERCOM_INTENSET = int_enable_flags;
}

static void i2c_controller_int_disable(const struct device *dev, uint8_t int_disable_mask)
{
	uint8_t int_clear_flags = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((int_disable_mask & SERCOM_I2CM_INTENCLR_MB_Msk) == SERCOM_I2CM_INTENCLR_MB_Msk) {
		int_clear_flags |= SERCOM_I2CM_INTENCLR_MB(1);
	}
	if ((int_disable_mask & SERCOM_I2CM_INTENCLR_SB_Msk) == SERCOM_I2CM_INTENCLR_SB_Msk) {
		int_clear_flags |= SERCOM_I2CM_INTENCLR_SB(1);
	}
	if ((int_disable_mask & SERCOM_I2CM_INTENCLR_ERROR_Msk) == SERCOM_I2CM_INTENCLR_ERROR_Msk) {
		int_clear_flags |= SERCOM_I2CM_INTENCLR_ERROR(1);
	}

	i2c_regs->I2CM.SERCOM_INTENCLR = int_clear_flags;
}

static uint8_t i2c_controller_int_flag_get(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;
	uint8_t flag_reg_val = i2c_regs->I2CM.SERCOM_INTFLAG;
	uint8_t interrupt_flags = 0;

	if ((flag_reg_val & SERCOM_I2CM_INTFLAG_MB_Msk) == SERCOM_I2CM_INTFLAG_MB_Msk) {
		interrupt_flags |= SERCOM_I2CM_INTFLAG_MB(1);
	}
	if ((flag_reg_val & SERCOM_I2CM_INTFLAG_SB_Msk) == SERCOM_I2CM_INTFLAG_SB_Msk) {
		interrupt_flags |= SERCOM_I2CM_INTFLAG_SB(1);
	}
	if ((flag_reg_val & SERCOM_I2CM_INTFLAG_ERROR_Msk) == SERCOM_I2CM_INTFLAG_ERROR_Msk) {
		interrupt_flags |= SERCOM_I2CM_INTFLAG_ERROR(1);
	}

	return interrupt_flags;
}

static void i2c_controller_int_flag_clear(const struct device *dev, uint8_t intflag_mask)
{
	uint8_t flag_clear = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((intflag_mask & SERCOM_I2CM_INTFLAG_MB_Msk) == SERCOM_I2CM_INTFLAG_MB_Msk) {
		flag_clear |= SERCOM_I2CM_INTFLAG_MB(1);
	}
	if ((intflag_mask & SERCOM_I2CM_INTFLAG_SB_Msk) == SERCOM_I2CM_INTFLAG_SB_Msk) {
		flag_clear |= SERCOM_I2CM_INTFLAG_SB(1);
	}
	if ((intflag_mask & SERCOM_I2CM_INTFLAG_ERROR_Msk) == SERCOM_I2CM_INTFLAG_ERROR_Msk) {
		flag_clear |= SERCOM_I2CM_INTFLAG_ERROR(1);
	}

	i2c_regs->I2CM.SERCOM_INTFLAG = flag_clear;
}

static void i2c_controller_addr_write(const struct device *dev, uint32_t addr)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((addr & (uint32_t)I2C_MCHP_MESSAGE_DIR_READ_MASK) == I2C_MCHP_MESSAGE_DIR_READ_MASK) {
		i2c_set_controller_auto_ack(dev);
	}

	i2c_regs->I2CM.SERCOM_ADDR = (i2c_regs->I2CM.SERCOM_ADDR & ~SERCOM_I2CM_ADDR_ADDR_Msk) |
				     SERCOM_I2CM_ADDR_ADDR(addr);

	/* Wait for synchronization */
	if (WAIT_FOR(((i2c_regs->I2CM.SERCOM_SYNCBUSY & SERCOM_I2CM_SYNCBUSY_SYSOP_Msk) == 0),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for I2C SYNCBUSY SYSOP clear");
	}
}

static bool i2c_baudrate_calc(uint32_t bitrate, uint32_t sys_clock_rate, uint32_t *baud_val)
{
	uint32_t baud_value = 0U;
	float fsrc_clk_freq = (float)sys_clock_rate;
	float fi2c_clk_speed = (float)bitrate;
	float fbaud_value = 0.0f;

	if (sys_clock_rate < (2U * bitrate)) {
		LOG_ERR("Invalid I2C clock configuration: sys_clk=%u Hz, bitrate=%u Hz",
			sys_clock_rate, bitrate);
		return false; /* Ref clock freq must be at least two time baud rate */
	}
	if (bitrate > I2C_SPEED_FAST_PLUS) {

		/* HS mode baud calculation */
		fbaud_value = (fsrc_clk_freq / fi2c_clk_speed) - 2.0f;
	} else {

		/* Standard, FM and FM+ baud calculation */
		fbaud_value = (fsrc_clk_freq / fi2c_clk_speed) -
			      ((fsrc_clk_freq * I2C_MCHP_START_CONDITION_SETUP_TIME) + 10.0f);
	}

	baud_value = (uint32_t)fbaud_value;

	if (bitrate <= I2C_SPEED_FAST) {

		/* For I2C clock speed up to 400 kHz, the value of BAUD<7:0>
		 * determines both SCL_L and SCL_H with SCL_L = SCL_H
		 */
		if (baud_value > (0xFFU * 2U)) {

			/* Set baud rate to the maximum possible value */
			baud_value = 0xFFU;
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

		/* Set baud rate to the minimum possible value while
		 * maintaining SCL_L:SCL_H to 2:1
		 */
		baud_value = (0xFFUL << 8U) | (0x7FU);
	} else if (baud_value <= 3U) {

		/* Baud value cannot be 0. Set baud rate to maximum possible
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

static bool i2c_set_baudrate(const struct device *dev, uint32_t bitrate, uint32_t sys_clock_rate)
{
	uint32_t i2c_speed_mode = 0;
	uint32_t baud_value;
	uint32_t hsbaud_value;
	uint32_t sda_hold_time = 0;
	bool is_success = false;

	if (bitrate == I2C_SPEED_HIGH) {

		/* HS mode requires baud values for both FS and HS frequency. First
		 * calculate baud for FS
		 */
		if (i2c_baudrate_calc(I2C_SPEED_FAST, sys_clock_rate, &baud_value) == true &&
		    i2c_baudrate_calc(bitrate, sys_clock_rate, &hsbaud_value) == true) {
			is_success = true;
			baud_value |= (hsbaud_value << 16U);
			i2c_speed_mode = 2U;
			sda_hold_time = 2U;
		}

	} else {
		if (i2c_baudrate_calc(bitrate, sys_clock_rate, &baud_value) == true) {
			if (bitrate == I2C_SPEED_FAST_PLUS) {
				i2c_speed_mode = 1U;
				sda_hold_time = 1U;
			}
			is_success = true;
		}
	}

	if (is_success == true) {
		sercom_registers_t *i2c_regs =
			((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

		/* Baud rate - controller Baud Rate*/
		i2c_regs->I2CM.SERCOM_BAUD = baud_value;

		i2c_regs->I2CM.SERCOM_CTRLA =
			((i2c_regs->I2CM.SERCOM_CTRLA &
			  (~SERCOM_I2CM_CTRLA_SPEED_Msk | ~SERCOM_I2CM_CTRLA_SDAHOLD_Msk)) |
			 (SERCOM_I2CM_CTRLA_SPEED(i2c_speed_mode) |
			  SERCOM_I2CM_CTRLA_SDAHOLD(sda_hold_time)));
	}

	return is_success;
}

static bool i2c_is_terminate_on_error(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;

	data->current_msg.status = i2c_controller_status_get(dev);
	if (data->current_msg.status == 0) {
		return false;
	}

	i2c_controller_status_clear(dev, data->current_msg.status);
	i2c_controller_int_disable(dev, SERCOM_I2CM_INTENSET_Msk);
	i2c_controller_transfer_stop(dev);
#ifdef CONFIG_I2C_CALLBACK
	data->i2c_async_callback(dev, (int)data->current_msg.status, data->user_data);
#else
	k_sem_give(&data->i2c_sync_sem);
#endif /*CONFIG_I2C_CALLBACK*/

	return true;
}

static void i2c_restart(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;

	/* left-shift address by 1 for R/W bit. */
	uint32_t addr_reg = data->target_addr << 1U;

	bool is_read =
		((data->msgs_array[data->msg_index].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);
	if (is_read == true) {
		addr_reg |= 1U;
	}

	i2c_controller_addr_write(dev, addr_reg);
	i2c_controller_int_enable(dev, SERCOM_I2CM_INTENSET_Msk);
}

#ifdef CONFIG_I2C_TARGET
static uint8_t i2c_target_int_flag_get(const struct device *dev)
{
	uint8_t flag_reg_val;
	uint16_t interrupt_flags = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	flag_reg_val = i2c_regs->I2CS.SERCOM_INTFLAG;

	if ((flag_reg_val & SERCOM_I2CS_INTFLAG_PREC_Msk) == SERCOM_I2CS_INTFLAG_PREC_Msk) {
		interrupt_flags |= SERCOM_I2CS_INTFLAG_PREC_Msk;
	}
	if ((flag_reg_val & SERCOM_I2CS_INTFLAG_AMATCH_Msk) == SERCOM_I2CS_INTFLAG_AMATCH_Msk) {
		interrupt_flags |= SERCOM_I2CS_INTFLAG_AMATCH_Msk;
	}
	if ((flag_reg_val & SERCOM_I2CS_INTFLAG_DRDY_Msk) == SERCOM_I2CS_INTFLAG_DRDY_Msk) {
		interrupt_flags |= SERCOM_I2CS_INTFLAG_DRDY_Msk;
	}
	if ((flag_reg_val & SERCOM_I2CS_INTFLAG_ERROR_Msk) == SERCOM_I2CS_INTFLAG_ERROR_Msk) {
		interrupt_flags |= SERCOM_I2CS_INTFLAG_ERROR_Msk;
	}

	return interrupt_flags;
}

static uint16_t i2c_target_status_get(const struct device *dev)
{
	uint16_t status_flags = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;
	uint16_t status_reg_val = i2c_regs->I2CS.SERCOM_STATUS;

	if ((status_reg_val & SERCOM_I2CS_STATUS_BUSERR_Msk) == SERCOM_I2CS_STATUS_BUSERR_Msk) {
		status_flags |= SERCOM_I2CS_STATUS_BUSERR_Msk;
	}
	if ((status_reg_val & SERCOM_I2CS_STATUS_COLL_Msk) == SERCOM_I2CS_STATUS_COLL_Msk) {
		status_flags |= SERCOM_I2CS_STATUS_COLL_Msk;
	}
	if ((status_reg_val & SERCOM_I2CS_STATUS_DIR_Msk) == SERCOM_I2CS_STATUS_DIR_Msk) {
		status_flags |= SERCOM_I2CS_STATUS_DIR_Msk;
	}
	if ((status_reg_val & SERCOM_I2CS_STATUS_LOWTOUT_Msk) == SERCOM_I2CS_STATUS_LOWTOUT_Msk) {
		status_flags |= SERCOM_I2CS_STATUS_LOWTOUT_Msk;
	}
	if ((status_reg_val & SERCOM_I2CS_STATUS_SEXTTOUT_Msk) == SERCOM_I2CS_STATUS_SEXTTOUT_Msk) {
		status_flags |= SERCOM_I2CS_STATUS_SEXTTOUT_Msk;
	}

	return status_flags;
}

static void i2c_target_status_clear(const struct device *dev, uint16_t status_flags)
{
	uint16_t status_clear = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((status_flags & SERCOM_I2CS_STATUS_BUSERR_Msk) == SERCOM_I2CS_STATUS_BUSERR_Msk) {
		status_clear |= SERCOM_I2CS_STATUS_BUSERR(1);
	}
	if ((status_flags & SERCOM_I2CS_STATUS_COLL_Msk) == SERCOM_I2CS_STATUS_COLL_Msk) {
		status_clear |= SERCOM_I2CS_STATUS_COLL(1);
	}
	if ((status_flags & SERCOM_I2CS_STATUS_LOWTOUT_Msk) == SERCOM_I2CS_STATUS_LOWTOUT_Msk) {
		status_clear |= SERCOM_I2CS_STATUS_LOWTOUT(1);
	}
	if ((status_flags & SERCOM_I2CS_STATUS_SEXTTOUT_Msk) == SERCOM_I2CS_STATUS_SEXTTOUT_Msk) {
		status_clear |= SERCOM_I2CS_STATUS_SEXTTOUT(1);
	}

	i2c_regs->I2CS.SERCOM_STATUS = status_clear;
}

static void i2c_target_int_flag_clear(const struct device *dev, uint8_t target_intflag)
{
	uint8_t flag_clear = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((target_intflag & SERCOM_I2CS_INTFLAG_PREC_Msk) == SERCOM_I2CS_INTFLAG_PREC_Msk) {
		flag_clear |= SERCOM_I2CS_INTFLAG_PREC(1);
	}
	if ((target_intflag & SERCOM_I2CS_INTFLAG_AMATCH_Msk) == SERCOM_I2CS_INTFLAG_AMATCH_Msk) {
		flag_clear |= SERCOM_I2CS_INTFLAG_AMATCH(1);
	}
	if ((target_intflag & SERCOM_I2CS_INTFLAG_DRDY_Msk) == SERCOM_I2CS_INTFLAG_DRDY_Msk) {
		flag_clear |= SERCOM_I2CS_INTFLAG_DRDY(1);
	}
	if ((target_intflag & SERCOM_I2CS_INTFLAG_ERROR_Msk) == SERCOM_I2CS_INTFLAG_ERROR_Msk) {
		flag_clear |= SERCOM_I2CS_INTFLAG_ERROR(1);
	}

	i2c_regs->I2CS.SERCOM_INTFLAG = flag_clear;
}

static inline uint8_t i2c_target_get_lastbyte_ack_status(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	return ((i2c_regs->I2CS.SERCOM_STATUS & SERCOM_I2CS_STATUS_RXNACK_Msk) != 0U)
		       ? I2C_MCHP_TARGET_ACK_STATUS_RECEIVED_NACK
		       : I2C_MCHP_TARGET_ACK_STATUS_RECEIVED_ACK;
}

void i2c_target_set_command(const struct device *dev, enum i2c_mchp_target_cmd cmd)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	i2c_regs->I2CS.SERCOM_CTRLB &= ~SERCOM_I2CS_CTRLB_CMD_Msk;

	switch (cmd) {
	case I2C_MCHP_TARGET_COMMAND_SEND_ACK:
		i2c_regs->I2CS.SERCOM_CTRLB =
			(i2c_regs->I2CS.SERCOM_CTRLB & ~SERCOM_I2CS_CTRLB_ACKACT_Msk) |
			SERCOM_I2CS_CTRLB_CMD(0x03UL);
		break;
	case I2C_MCHP_TARGET_COMMAND_SEND_NACK:
		i2c_regs->I2CS.SERCOM_CTRLB =
			(i2c_regs->I2CS.SERCOM_CTRLB | SERCOM_I2CS_CTRLB_ACKACT_Msk) |
			SERCOM_I2CS_CTRLB_CMD(0x03UL);
		break;
	case I2C_MCHP_TARGET_COMMAND_RECEIVE_ACK_NAK:
		i2c_regs->I2CS.SERCOM_CTRLB |= SERCOM_I2CS_CTRLB_CMD(0x03UL);
		break;
	case I2C_MCHP_TARGET_COMMAND_WAIT_FOR_START:
		i2c_regs->I2CS.SERCOM_CTRLB =
			(i2c_regs->I2CS.SERCOM_CTRLB | SERCOM_I2CS_CTRLB_ACKACT_Msk) |
			SERCOM_I2CS_CTRLB_CMD(0x02UL);
		break;
	default:
		break;
	}
}

static void i2c_target_address_match(const struct device *dev, struct i2c_mchp_dev_data *data,
				     uint16_t target_status)
{
	const struct i2c_target_callbacks *target_cb = &data->target_callbacks;

	i2c_target_set_command(dev, I2C_MCHP_TARGET_COMMAND_SEND_ACK);
	data->firstReadAfterAddrMatch = true;

	if ((target_status & SERCOM_I2CS_STATUS_DIR_Msk) == SERCOM_I2CS_STATUS_DIR_Msk) {

		/* Load the first byte for host read */
		target_cb->read_requested(&data->target_config, &data->rx_tx_data);
	} else {

		/* Host writing */
		target_cb->write_requested(&data->target_config);
	}
}

static void i2c_target_data_ready(const struct device *dev, struct i2c_mchp_dev_data *data,
				  uint16_t target_status)
{
	const struct i2c_target_callbacks *target_cb = &data->target_callbacks;
	int retval;

	if (((target_status & SERCOM_I2CS_STATUS_DIR_Msk) == SERCOM_I2CS_STATUS_DIR_Msk)) {
		if ((data->firstReadAfterAddrMatch == true) ||
		    (i2c_target_get_lastbyte_ack_status(dev) ==
		     I2C_MCHP_TARGET_ACK_STATUS_RECEIVED_ACK)) {

			/* Host is reading */
			i2c_byte_write(dev, data->rx_tx_data);

			/* first byte read after address match done*/
			data->firstReadAfterAddrMatch = false;
			i2c_target_set_command(dev, I2C_MCHP_TARGET_COMMAND_RECEIVE_ACK_NAK);

			/* Load the next byte for host read*/
			target_cb->read_processed(&data->target_config, &data->rx_tx_data);

		} else {
			i2c_target_set_command(dev, I2C_MCHP_TARGET_COMMAND_WAIT_FOR_START);
		}
	} else {
		/* Host is writing */
		i2c_target_set_command(dev, I2C_MCHP_TARGET_COMMAND_SEND_ACK);
		data->rx_tx_data = i2c_byte_read(dev);
		retval = target_cb->write_received(&data->target_config, data->rx_tx_data);
		if (retval != I2C_MCHP_SUCCESS) {
			i2c_target_set_command(dev, I2C_MCHP_TARGET_COMMAND_SEND_NACK);
		}
	}
}

static void i2c_target_handler(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;
	const struct i2c_target_callbacks *target_cb = &data->target_callbacks;
	uint8_t int_status = i2c_target_int_flag_get(dev);
	uint16_t target_status = i2c_target_status_get(dev);

	/* Handle error conditions */
	if ((int_status & SERCOM_I2CS_INTFLAG_ERROR_Msk) == SERCOM_I2CS_INTFLAG_ERROR_Msk) {
		i2c_target_int_flag_clear(dev, SERCOM_I2CS_INTFLAG_ERROR_Msk);
		LOG_ERR("Interrupt Error generated");
		target_cb->stop(&data->target_config);
	} else {

		/* Handle address match */
		if ((int_status & SERCOM_I2CS_INTFLAG_AMATCH_Msk) ==
		    SERCOM_I2CS_INTFLAG_AMATCH_Msk) {
			i2c_target_set_command(dev, I2C_MCHP_TARGET_COMMAND_SEND_ACK);
			data->firstReadAfterAddrMatch = true;
			i2c_target_address_match(dev, data, target_status);
		}

		/* Handle data ready (Read/Write Operations) */
		if ((int_status & SERCOM_I2CS_INTFLAG_DRDY_Msk) == SERCOM_I2CS_INTFLAG_DRDY_Msk) {
			i2c_target_data_ready(dev, data, target_status);
		}
	}

	/* Handle stop condition interrupt */
	if ((int_status & SERCOM_I2CS_INTFLAG_PREC_Msk) == SERCOM_I2CS_INTFLAG_PREC_Msk) {
		i2c_target_int_flag_clear(dev, SERCOM_I2CS_INTFLAG_PREC_Msk);

		/* Notify that a stop condition was received */
		target_cb->stop(&data->target_config);
	}

	i2c_target_status_clear(dev, target_status);
}
#endif /* CONFIG_I2C_TARGET */

static bool i2c_controller_check_continue_next(struct i2c_mchp_dev_data *data)
{
	bool continue_next = false;

	if ((data->current_msg.size == 1U) && (data->num_msgs > 1U) &&
	    ((data->msgs_array[data->msg_index].flags & I2C_MSG_RW_MASK) ==
	     (data->msgs_array[data->msg_index + 1U].flags & I2C_MSG_RW_MASK)) &&
	    ((data->msgs_array[data->msg_index + 1U].flags & I2C_MSG_RESTART) == 0U)) {
		continue_next = true;
	}

	return continue_next;
}

static void i2c_handle_controller_error(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;

	i2c_controller_transfer_stop(dev);
	i2c_controller_int_disable(dev, SERCOM_I2CM_INTENSET_Msk);
#ifdef CONFIG_I2C_CALLBACK
	data->i2c_async_callback(dev, (int)data->current_msg.status, data->user_data);
#else
	k_sem_give(&data->i2c_sync_sem);
#endif /*CONFIG_I2C_CALLBACK*/
}

static void i2c_handle_controller_write_mode(const struct device *dev, bool continue_next)
{
	struct i2c_mchp_dev_data *data = dev->data;

	if (data->current_msg.size == 0U) {
		i2c_controller_transfer_stop(dev);
		i2c_controller_int_disable(dev, SERCOM_I2CM_INTFLAG_MB_Msk);

		if (data->num_msgs > 1U) {
			data->msg_index++;
			data->num_msgs--;
			data->current_msg.buffer = data->msgs_array[data->msg_index].buf;
			data->current_msg.size = data->msgs_array[data->msg_index].len;
			data->current_msg.status = 0U;
			i2c_restart(dev);
		} else {
#ifdef CONFIG_I2C_CALLBACK
			data->i2c_async_callback(dev, (int)data->current_msg.status,
						 data->user_data);
#else
			k_sem_give(&data->i2c_sync_sem);
#endif /*CONFIG_I2C_CALLBACK*/
		}
	} else {
		i2c_byte_write(dev, *data->current_msg.buffer);
		data->current_msg.buffer++;
		data->current_msg.size--;
	}

	if (continue_next == true) {
		data->msg_index++;
		data->num_msgs--;
		data->current_msg.buffer = data->msgs_array[data->msg_index].buf;
		data->current_msg.size = data->msgs_array[data->msg_index].len;
		data->current_msg.status = 0U;
	}
}

static void i2c_handle_controller_read_mode(const struct device *dev, bool continue_next)
{
	struct i2c_mchp_dev_data *data = dev->data;

	if ((continue_next == false) && (data->current_msg.size == 1U)) {
		i2c_controller_transfer_stop(dev);
	}

	*data->current_msg.buffer = i2c_byte_read(dev);
	data->current_msg.buffer++;
	data->current_msg.size--;

	if ((continue_next == false) && (data->current_msg.size == 0U)) {
		i2c_controller_int_disable(dev, SERCOM_I2CM_INTFLAG_SB_Msk);

		if (data->num_msgs > 1U) {
			data->msg_index++;
			data->num_msgs--;
			data->current_msg.buffer = data->msgs_array[data->msg_index].buf;
			data->current_msg.size = data->msgs_array[data->msg_index].len;
			data->current_msg.status = 0U;
			i2c_restart(dev);
		} else {
#ifdef CONFIG_I2C_CALLBACK
			data->i2c_async_callback(dev, (int)data->current_msg.status,
						 data->user_data);
#else
			k_sem_give(&data->i2c_sync_sem);
#endif /*CONFIG_I2C_CALLBACK*/
		}
	}

	if (continue_next == true) {
		data->msg_index++;
		data->num_msgs--;
		data->current_msg.buffer = data->msgs_array[data->msg_index].buf;
		data->current_msg.size = data->msgs_array[data->msg_index].len;
		data->current_msg.status = 0U;
	}
}

static void i2c_mchp_isr(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;
	bool continue_next = false;

#ifdef CONFIG_I2C_TARGET
	if (data->target_mode == true) {
		i2c_target_handler(dev);
	} else {
#endif /*CONFIG_I2C_TARGET*/

		uint8_t int_status = i2c_controller_int_flag_get(dev);

		if (i2c_is_terminate_on_error(dev) == false) {

			/* Handle ERROR interrupt flag for controller mode tx and rx */
			if (int_status == SERCOM_I2CM_INTFLAG_ERROR_Msk) {
				i2c_handle_controller_error(dev);
			} else {
				continue_next = i2c_controller_check_continue_next(data);

				switch (int_status) {
				case SERCOM_I2CM_INTFLAG_MB_Msk:
					i2c_handle_controller_write_mode(dev, continue_next);
					break;
				case SERCOM_I2CM_INTFLAG_SB_Msk:
					i2c_handle_controller_read_mode(dev, continue_next);
					break;
				default:
					break;
				}
			}
		}
#ifdef CONFIG_I2C_TARGET
	}
#endif /*CONFIG_I2C_TARGET*/
}

#ifdef CONFIG_I2C_MCHP_TARGET
static void i2c_target_enable(const struct device *dev, bool enable)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if (enable == true) {
		i2c_regs->I2CS.SERCOM_CTRLA |= SERCOM_I2CS_CTRLA_ENABLE(1);
	} else {
		i2c_regs->I2CS.SERCOM_CTRLA &= SERCOM_I2CS_CTRLA_ENABLE(0);
	}

	/* Wait for synchronization */
	if (WAIT_FOR(((i2c_regs->I2CS.SERCOM_SYNCBUSY & SERCOM_I2CS_SYNCBUSY_ENABLE_Msk) == 0),
		     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("Timeout waiting for I2C SYNCBUSY ENABLE clear");
	}
}

static void i2c_target_runstandby_enable(const struct device *dev)
{
	const struct i2c_mchp_dev_config *const i2c_cfg = dev->config;
	sercom_registers_t *i2c_regs = i2c_cfg->regs;
	uint32_t reg32_val = i2c_regs->I2CS.SERCOM_CTRLA;

	reg32_val &= ~SERCOM_I2CS_CTRLA_RUNSTDBY_Msk;
	reg32_val |= SERCOM_I2CS_CTRLA_RUNSTDBY(i2c_cfg->run_in_standby);
	i2c_regs->I2CS.SERCOM_CTRLA = reg32_val;
}

static void i2c_set_target_mode(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	/* Enable i2c device smart mode features */
	i2c_regs->I2CS.SERCOM_CTRLB = ((i2c_regs->I2CS.SERCOM_CTRLB & ~SERCOM_I2CS_CTRLB_SMEN_Msk) |
				       SERCOM_I2CS_CTRLB_SMEN(1));
	i2c_regs->I2CS.SERCOM_CTRLA =
		(i2c_regs->I2CS.SERCOM_CTRLA & ~SERCOM_I2CS_CTRLA_MODE_Msk) |
		(SERCOM_I2CS_CTRLA_MODE(0x4) | SERCOM_I2CS_CTRLA_SDAHOLD(0x1) |
		 SERCOM_I2CS_CTRLA_SPEED(0x1));
}

static void i2c_reset_target_addr(const struct device *dev)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	i2c_regs->I2CS.SERCOM_ADDR = (i2c_regs->I2CS.SERCOM_ADDR & ~SERCOM_I2CS_ADDR_ADDR_Msk) |
				     SERCOM_I2CS_ADDR_ADDR(0);
}

static void i2c_target_int_enable(const struct device *dev, uint8_t target_int)
{
	uint8_t int_set = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((target_int & SERCOM_I2CS_INTENSET_PREC_Msk) == SERCOM_I2CS_INTENSET_PREC_Msk) {
		int_set |= SERCOM_I2CS_INTENSET_PREC(1);
	}
	if ((target_int & SERCOM_I2CS_INTENSET_AMATCH_Msk) == SERCOM_I2CS_INTENSET_AMATCH_Msk) {
		int_set |= SERCOM_I2CS_INTENSET_AMATCH(1);
	}
	if ((target_int & SERCOM_I2CS_INTENSET_DRDY_Msk) == SERCOM_I2CS_INTENSET_DRDY_Msk) {
		int_set |= SERCOM_I2CS_INTENSET_DRDY(1);
	}
	if ((target_int & SERCOM_I2CS_INTENSET_ERROR_Msk) == SERCOM_I2CS_INTENSET_ERROR_Msk) {
		int_set |= SERCOM_I2CS_INTENSET_ERROR(1);
	}

	i2c_regs->I2CS.SERCOM_INTENSET = int_set;
}

static void i2c_target_int_disable(const struct device *dev, uint8_t target_int)
{
	uint8_t int_clear = 0;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((target_int & SERCOM_I2CS_INTENCLR_PREC_Msk) == SERCOM_I2CS_INTENCLR_PREC_Msk) {
		int_clear |= SERCOM_I2CS_INTENCLR_PREC(1);
	}
	if ((target_int & SERCOM_I2CS_INTENCLR_AMATCH_Msk) == SERCOM_I2CS_INTENCLR_AMATCH_Msk) {
		int_clear |= SERCOM_I2CS_INTENCLR_AMATCH(1);
	}
	if ((target_int & SERCOM_I2CS_INTENCLR_DRDY_Msk) == SERCOM_I2CS_INTENCLR_DRDY_Msk) {
		int_clear |= SERCOM_I2CS_INTENCLR_DRDY(1);
	}
	if ((target_int & SERCOM_I2CS_INTENCLR_ERROR_Msk) == SERCOM_I2CS_INTENCLR_ERROR_Msk) {
		int_clear |= SERCOM_I2CS_INTENCLR_ERROR(1);
	}

	i2c_regs->I2CS.SERCOM_INTENCLR = int_clear;
}

static void i2c_set_target_addr(const struct device *dev, uint32_t addr)
{
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	i2c_regs->I2CS.SERCOM_ADDR = (i2c_regs->I2CS.SERCOM_ADDR & ~SERCOM_I2CS_ADDR_ADDR_Msk) |
				     SERCOM_I2CS_ADDR_ADDR(addr);
}

static int i2c_mchp_target_register(const struct device *dev, struct i2c_target_config *target_cfg)
{
	struct i2c_mchp_dev_data *data = dev->data;

	if (data->target_mode == true) {
		LOG_ERR("Device already registered in target mode.");
		return -EBUSY;
	}
	if ((target_cfg == NULL) || (target_cfg->callbacks == NULL)) {
		LOG_ERR("Invalid target configuration or missing callbacks");
		return -EINVAL;
	}
	if (target_cfg->address == I2C_INVALID_ADDR) {
		LOG_ERR("device can't be register in target mode with 0x00 "
			"address\n");
		return -EINVAL;
	}

	k_mutex_lock(&data->i2c_bus_mutex, K_FOREVER);
	data->target_config.address = target_cfg->address;
	data->target_callbacks.write_requested = target_cfg->callbacks->write_requested;
	data->target_callbacks.write_received = target_cfg->callbacks->write_received;
	data->target_callbacks.read_requested = target_cfg->callbacks->read_requested;
	data->target_callbacks.read_processed = target_cfg->callbacks->read_processed;
	data->target_callbacks.stop = target_cfg->callbacks->stop;

	i2c_target_enable(dev, false);
	i2c_target_int_disable(dev, SERCOM_I2CS_INTENSET_Msk);
	i2c_set_target_mode(dev);
	i2c_set_target_addr(dev, data->target_config.address);
	i2c_target_int_enable(dev, SERCOM_I2CS_INTENSET_Msk);
	data->target_mode = true;
	i2c_target_runstandby_enable(dev);
	i2c_target_enable(dev, true);
	k_mutex_unlock(&data->i2c_bus_mutex);

	return I2C_MCHP_SUCCESS;
}

static int i2c_mchp_target_unregister(const struct device *dev,
				      struct i2c_target_config *target_cfg)
{
	struct i2c_mchp_dev_data *data = dev->data;

	if (target_cfg == NULL) {
		LOG_ERR("target_cfg is NULL");
		return -EINVAL;
	}
	if (data->target_mode != true) {
		LOG_ERR("device are not configured as target device\n");
		return -EBUSY;
	}
	if (data->target_config.address != target_cfg->address) {
		LOG_ERR("Target address mismatch");
		return -EINVAL;
	}

	k_mutex_lock(&data->i2c_bus_mutex, K_FOREVER);
	i2c_target_enable(dev, false);
	i2c_target_int_disable(dev, SERCOM_I2CS_INTENSET_Msk);
	i2c_reset_target_addr(dev);
	data->target_mode = false;
	data->target_config.address = 0x00;
	data->target_config.callbacks = NULL;
	i2c_target_enable(dev, true);
	k_mutex_unlock(&data->i2c_bus_mutex);

	return I2C_MCHP_SUCCESS;
}
#endif /*CONFIG_I2C_MCHP_TARGET*/

#ifdef CONFIG_I2C_CALLBACK
static int i2c_mchp_transfer_cb(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr, i2c_callback_t i2c_async_callback, void *user_data)
{
	struct i2c_mchp_dev_data *data = dev->data;
	uint32_t addr_reg;

	if (num_msgs == 0) {
		LOG_ERR("Invalid number of messages (num_msgs = 0)");
		return -EINVAL;
	}
	if (data->target_mode == true) {
		LOG_ERR("Device currently running in target mode\n");
		return -EBUSY;
	}

	for (uint8_t i = 0U; i < num_msgs; i++) {
		if ((msgs[i].len == 0U) || (msgs[i].buf == NULL)) {
			LOG_ERR("Invalid I2C message");
			return -EINVAL;
		}
	}

	k_mutex_lock(&data->i2c_bus_mutex, K_FOREVER);
	data->num_msgs = num_msgs;
	data->msgs_array = msgs;
	data->i2c_async_callback = i2c_async_callback;
	data->user_data = user_data;
	data->target_addr = addr;
	data->msg_index = 0;

	i2c_controller_int_disable(dev, SERCOM_I2CM_INTENSET_Msk);
	i2c_controller_int_flag_clear(dev, SERCOM_I2CM_INTFLAG_Msk);
	i2c_controller_status_clear(dev, SERCOM_I2CM_STATUS_Msk);

	data->current_msg.buffer = data->msgs_array[data->msg_index].buf;
	data->current_msg.size = data->msgs_array[data->msg_index].len;
	data->current_msg.status = 0;

	addr_reg = addr << 1U;
	bool is_read = ((data->msgs_array->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ);

	if (is_read == true) {
		addr_reg |= I2C_MESSAGE_DIR_READ;
	}
	i2c_controller_addr_write(dev, addr_reg);
	i2c_controller_int_enable(dev, SERCOM_I2CM_INTENSET_Msk);
	k_mutex_unlock(&data->i2c_bus_mutex);

	return I2C_MCHP_SUCCESS;
}
#endif /*CONFIG_I2C_CALLBACK*/

#if !defined(CONFIG_I2C_MCHP_INTERRUPT_DRIVEN)
static bool i2c_is_nack(const struct device *dev)
{
	bool retval;
	sercom_registers_t *i2c_regs = ((const struct i2c_mchp_dev_config *)(dev)->config)->regs;

	if ((i2c_regs->I2CM.SERCOM_CTRLA & SERCOM_I2CM_CTRLA_MODE_I2C_MASTER) ==
	    SERCOM_I2CM_CTRLA_MODE_I2C_MASTER) {
		retval = ((i2c_regs->I2CM.SERCOM_STATUS & SERCOM_I2CM_STATUS_RXNACK_Msk) != 0);
	} else {
		retval = ((i2c_regs->I2CS.SERCOM_STATUS & SERCOM_I2CS_STATUS_RXNACK_Msk) != 0);
	}

	return retval;
}

static int i2c_poll_in(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;

	if (i2c_is_nack(dev) == true) {
		i2c_controller_transfer_stop(dev);
		LOG_ERR("NACK received during I2C read operation");
		return -EIO;
	}

	for (uint32_t i = 0; i < data->current_msg.size; i++) {
		if (WAIT_FOR(((i2c_controller_int_flag_get(dev) & SERCOM_I2CM_INTFLAG_SB_Msk) != 0),
			     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
			LOG_ERR("Timeout waiting for SB flag");
			return -ETIMEDOUT;
		}

		/* Stop the I2C transfer when reading the last byte. */
		if (i == data->current_msg.size - 1) {
			i2c_controller_transfer_stop(dev);
		}
		data->current_msg.buffer[i] = i2c_byte_read(dev);
	}

	return I2C_MCHP_SUCCESS;
}

static int i2c_poll_out(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;

	if (i2c_is_nack(dev) == true) {
		i2c_controller_transfer_stop(dev);
		LOG_ERR("NACK received during I2C write operation");
		return -EIO;
	}

	for (uint32_t i = 0; i < data->current_msg.size; i++) {
		if (WAIT_FOR(((i2c_controller_int_flag_get(dev) & SERCOM_I2CM_INTFLAG_MB_Msk) != 0),
			     TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
			LOG_ERR("Timeout waiting for MB flag");
			return -ETIMEDOUT;
		}

		i2c_byte_write(dev, data->current_msg.buffer[i]);

		/* Check for a NACK condition after writing each byte. */
		if (i2c_is_nack(dev) == true) {
			i2c_controller_transfer_stop(dev);
			LOG_ERR("NACK received during byte write operation");
			return -EIO;
		}
	}

	i2c_controller_transfer_stop(dev);

	return I2C_MCHP_SUCCESS;
}
#endif /*!CONFIG_I2C_MCHP_INTERRUPT_DRIVEN*/

static int i2c_validate_transfer_params(const struct i2c_mchp_dev_data *data, struct i2c_msg *msgs,
					uint8_t num_msgs)
{
	if (num_msgs == 0) {
		LOG_ERR("Invalid number of messages (num_msgs = 0)");
		return -EINVAL;
	}

	if (data->target_mode == true) {
		LOG_ERR("Device currently configured in target mode\n");
		return -EBUSY;
	}

	/* Check for empty messages (invalid read/write). */
	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].len == 0 || msgs[i].buf == NULL) {
			LOG_ERR("Invalid transfer: message[%u] has null buffer or zero length", i);
			return -EINVAL;
		}
	}

	return I2C_MCHP_SUCCESS;
}

#ifdef CONFIG_I2C_MCHP_INTERRUPT_DRIVEN
static int i2c_check_interrupt_flag_errors(const struct device *dev, struct i2c_mchp_dev_data *data)
{
	if (data->current_msg.status == 0) {
		return I2C_MCHP_SUCCESS;
	}
	if ((data->current_msg.status & SERCOM_I2CM_STATUS_ARBLOST_Msk) ==
	    SERCOM_I2CM_STATUS_ARBLOST_Msk) {
		LOG_ERR("Arbitration lost on %s", dev->name);
		return -EAGAIN;
	}

	LOG_ERR("Transaction error on %s: %08X", dev->name, data->current_msg.status);

	return -EIO;
}
#endif /*CONFIG_I2C_MCHP_INTERRUPT_DRIVEN*/

static int i2c_mchp_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			     uint16_t addr)
{
	struct i2c_mchp_dev_data *data = dev->data;
	uint32_t addr_reg;
	int retval;

	retval = i2c_validate_transfer_params(data, msgs, num_msgs);
	if (retval != I2C_MCHP_SUCCESS) {
		LOG_ERR("Invalid transfer parameters");
		return retval;
	}

	k_mutex_lock(&data->i2c_bus_mutex, K_FOREVER);

	i2c_controller_int_disable(dev, SERCOM_I2CM_INTENSET_Msk);
	i2c_controller_int_flag_clear(dev, SERCOM_I2CM_INTFLAG_Msk);
	i2c_controller_status_clear(dev, SERCOM_I2CM_STATUS_Msk);

	data->num_msgs = num_msgs;
	data->msgs_array = msgs;
	data->msg_index = 0;
	data->target_addr = addr;

	while (data->num_msgs > 0) {

		data->current_msg.buffer = data->msgs_array[data->msg_index].buf;
		data->current_msg.size = data->msgs_array[data->msg_index].len;
		data->current_msg.status = 0;

		addr_reg = addr << 1U;
		if ((data->msgs_array[data->msg_index].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			addr_reg |= I2C_MESSAGE_DIR_READ;
		}
		i2c_controller_addr_write(dev, addr_reg);

#ifdef CONFIG_I2C_MCHP_INTERRUPT_DRIVEN
		i2c_controller_int_enable(dev, SERCOM_I2CM_INTENSET_Msk);

		/* Wait for transfer completion */
		retval = k_sem_take(&data->i2c_sync_sem, I2C_TRANSFER_TIMEOUT_MSEC);
		if (retval != 0) {
			LOG_ERR("Transfer timeout on %s", dev->name);
			i2c_controller_transfer_stop(dev);
			break;
		}

		retval = i2c_check_interrupt_flag_errors(dev, data);
		if (retval != I2C_MCHP_SUCCESS) {
			LOG_ERR("I2C interrupt flag error: %d", retval);
			break;
		}
#else
		if ((data->msgs_array[data->msg_index].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			retval = i2c_poll_in(dev);
		} else {
			retval = i2c_poll_out(dev);
		}
		if (retval != I2C_MCHP_SUCCESS) {
			LOG_ERR("I2C polling transfer failed: %d", retval);
			break;
		}
#endif /*CONFIG_I2C_MCHP_INTERRUPT_DRIVEN*/

		data->num_msgs--;
		data->msg_index++;
	}
	k_mutex_unlock(&data->i2c_bus_mutex);

	return retval;
}

static int i2c_mchp_recover_bus(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;
	const struct i2c_mchp_dev_config *const cfg = dev->config;
	int retval;

	k_mutex_lock(&data->i2c_bus_mutex, K_FOREVER);
	i2c_controller_enable(dev, false);
	i2c_controller_int_disable(dev, SERCOM_I2CM_INTENSET_Msk);

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval != I2C_MCHP_SUCCESS) {
		LOG_ERR("Failed to apply default pin state: %d", retval);
		k_mutex_unlock(&data->i2c_bus_mutex);
		return retval;
	}

	i2c_controller_enable(dev, true);
	i2c_set_controller_bus_state_idle(dev);
	k_mutex_unlock(&data->i2c_bus_mutex);

	return retval;
}

static int i2c_mchp_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_mchp_dev_data *data = dev->data;

	if (data->dev_config == 0U) {
		LOG_ERR("Device configuration not initialized");
		return -EINVAL;
	}

	if (dev_config == NULL) {
		LOG_ERR("dev_config pointer is NULL");
		return -EINVAL;
	}

	*dev_config = data->dev_config;
	LOG_DBG("Retrieved I2C device configuration: 0x%08X", *dev_config);

	return I2C_MCHP_SUCCESS;
}

static int i2c_set_apply_bitrate(const struct device *dev, uint32_t config)
{
	uint32_t sys_clock_rate = 0;
	uint32_t bitrate;
	const struct i2c_mchp_dev_config *const cfg = dev->config;

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		bitrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		bitrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		bitrate = MHZ(1);
		break;
	case I2C_SPEED_HIGH:
		bitrate = MHZ(3.4);
		break;
	default:
		LOG_ERR("Unsupported speed code: %d", I2C_SPEED_GET(config));
		return -ENOTSUP;
	}

	clock_control_get_rate(cfg->i2c_clock.clock_dev, cfg->i2c_clock.gclk_sys, &sys_clock_rate);
	if (sys_clock_rate == 0) {
		LOG_ERR("Failed to retrieve system clock rate.");
		return -EIO;
	}
	if (i2c_set_baudrate(dev, bitrate, sys_clock_rate) != true) {
		LOG_ERR("Failed to set I2C baud rate to %u Hz.", bitrate);
		return -EIO;
	}

	return I2C_MCHP_SUCCESS;
}

static int i2c_mchp_configure(const struct device *dev, uint32_t config)
{
	struct i2c_mchp_dev_data *data = dev->data;
	int retval;

	if (data->target_mode == true) {
		LOG_ERR("Cannot reconfigure while device is in target mode.");
		return -EBUSY;
	}

	k_mutex_lock(&data->i2c_bus_mutex, K_FOREVER);
	i2c_controller_enable(dev, false);

	if ((config & I2C_MODE_CONTROLLER) == I2C_MODE_CONTROLLER) {

		i2c_set_controller_mode(dev);
	}
	if (I2C_SPEED_GET(config) != I2C_MCHP_SUCCESS) {

		retval = i2c_set_apply_bitrate(dev, config);
		if (retval != I2C_MCHP_SUCCESS) {
			LOG_ERR("Failed to set bitrate: %d", retval);
			k_mutex_unlock(&data->i2c_bus_mutex);
			return retval;
		}
	}

	data->dev_config = I2C_SPEED_GET(config);
	i2c_controller_enable(dev, true);
	i2c_set_controller_bus_state_idle(dev);
	k_mutex_unlock(&data->i2c_bus_mutex);

	return retval;
}

static int i2c_mchp_init(const struct device *dev)
{
	struct i2c_mchp_dev_data *data = dev->data;
	const struct i2c_mchp_dev_config *const cfg = dev->config;
	int retval;

	retval = clock_control_on(cfg->i2c_clock.clock_dev, cfg->i2c_clock.gclk_sys);
	if (retval != I2C_MCHP_SUCCESS) {
		LOG_ERR("Failed to enable GCLK_SYS clock: %d", retval);
		return retval;
	}

	retval = clock_control_on(cfg->i2c_clock.clock_dev, cfg->i2c_clock.mclk_sys);
	if (retval != I2C_MCHP_SUCCESS) {
		LOG_ERR("Failed to enable main clock: %d", retval);
		return retval;
	}

	i2c_swrst(dev);
	k_mutex_init(&data->i2c_bus_mutex);
	k_sem_init(&data->i2c_sync_sem, 0, 1);
	data->target_mode = false;

	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval != I2C_MCHP_SUCCESS) {
		LOG_ERR("Failed to apply pinctrl state: %d", retval);
		return retval;
	}

	retval = i2c_configure(dev, (I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(cfg->bitrate)));
	if (retval != I2C_MCHP_SUCCESS) {
		LOG_ERR("Failed to apply pinctrl state. Error: %d", retval);
		return retval;
	}

	i2c_controller_enable(dev, false);
	cfg->irq_config_func(dev);
	i2c_controller_runstandby_enable(dev);
	i2c_controller_enable(dev, true);
	i2c_set_controller_bus_state_idle(dev);

	return I2C_MCHP_SUCCESS;
}

static DEVICE_API(i2c, i2c_mchp_api) = {
	.configure = i2c_mchp_configure,
	.get_config = i2c_mchp_get_config,
	.transfer = i2c_mchp_transfer,
#ifdef CONFIG_I2C_MCHP_TARGET
	.target_register = i2c_mchp_target_register,
	.target_unregister = i2c_mchp_target_unregister,
#endif /*CONFIG_I2C_MCHP_TARGET*/
#ifdef CONFIG_I2C_CALLBACK
	.transfer_cb = i2c_mchp_transfer_cb,
#endif /*CONFIG_I2C_CALLBACK*/
	.recover_bus = i2c_mchp_recover_bus,
};

#define I2C_MCHP_REG_DEFN(n) .regs = (sercom_registers_t *)DT_INST_REG_ADDR(n),

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
#endif /*DT_INST_IRQ_HAS_IDX(0, 3)*/

#define I2C_MCHP_CLOCK_DEFN(n)                                                                     \
	.i2c_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                 \
	.i2c_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),           \
	.i2c_clock.gclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),

#define I2C_MCHP_IRQ_CONNECT(n, m)                                                                 \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    i2c_mchp_isr, DEVICE_DT_INST_GET(n), 0);                               \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define I2C_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct i2c_mchp_dev_config i2c_mchp_dev_config_##n = {                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		.irq_config_func = &i2c_mchp_irq_config_##n,                                       \
		.run_in_standby = DT_INST_PROP(n, run_in_standby_en),                              \
		I2C_MCHP_REG_DEFN(n) I2C_MCHP_CLOCK_DEFN(n)}

#define I2C_MCHP_DEVICE_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void i2c_mchp_irq_config_##n(const struct device *dev);                             \
	I2C_MCHP_CONFIG_DEFN(n);                                                                   \
	static struct i2c_mchp_dev_data i2c_mchp_dev_data_##n;                                     \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_mchp_init, NULL, &i2c_mchp_dev_data_##n,                  \
				  &i2c_mchp_dev_config_##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, \
				  &i2c_mchp_api);                                                  \
	I2C_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(I2C_MCHP_DEVICE_INIT)
