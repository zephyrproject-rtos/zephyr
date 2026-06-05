/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_i2c

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <esp_rom_gpio.h>
#include <soc/gpio_sig_map.h>
#include <hal/i2c_ll.h>
#include <hal/i2c_hal.h>
#include <hal/gpio_hal.h>
#include <esp_clk_tree.h>
#include <esp_private/esp_clk_tree_common.h>

#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_esp32, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

#if defined(CONFIG_I2C_TARGET) && SOC_I2C_SUPPORT_SLAVE && SOC_I2C_SLAVE_CAN_GET_STRETCH_CAUSE
#define I2C_ESP32_TARGET_ENABLED 1
#else
#define I2C_ESP32_TARGET_ENABLED 0
#endif

#define I2C_FILTER_CYC_NUM_DEF 7	/* Number of apb cycles filtered by default */
#define I2C_CLR_BUS_SCL_NUM 9		/* Number of SCL clocks to restore SDA signal */
#define I2C_CLR_BUS_HALF_PERIOD_US 5	/* Period of SCL clock to restore SDA signal */
#define I2C_TRANSFER_TIMEOUT_MSEC 500	/* Transfer timeout period */

/* Freq limitation when using different clock sources */
#define I2C_CLK_LIMIT_REF_TICK (1 * 1000 * 1000 / 20)	/* REF_TICK, no more than REF_TICK/20*/
#define I2C_CLK_LIMIT_APB (80 * 1000 * 1000 / 20)	/* Limited by APB, no more than APB/20 */
#define I2C_CLK_LIMIT_RTC (20 * 1000 * 1000 / 20)	/* Limited by RTC, no more than RTC/20 */
#define I2C_CLK_LIMIT_XTAL (40 * 1000 * 1000 / 20)	/* Limited by RTC, no more than XTAL/20 */

#define I2C_CLOCK_INVALID                 (-1)

enum i2c_status_t {
	I2C_STATUS_READ,	/* read status for current master command */
	I2C_STATUS_WRITE,	/* write status for current master command */
	I2C_STATUS_IDLE,	/* idle status for current master command */
	I2C_STATUS_ACK_ERROR,	/* ack error status for current master command */
	I2C_STATUS_DONE,	/* I2C command done */
	I2C_STATUS_TIMEOUT,	/* I2C bus status error, and operation timeout */
};

#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
struct i2c_esp32_pin {
	struct gpio_dt_spec gpio;
	int sig_out;
	int sig_in;
};
#endif

struct i2c_esp32_data {
	i2c_hal_context_t hal;
	struct k_sem cmd_sem;
	struct k_sem transfer_sem;
	volatile enum i2c_status_t status;
	uint32_t dev_config;
	int cmd_idx;
	int irq_line;
#if I2C_ESP32_TARGET_ENABLED
	struct i2c_target_config *target_cfg;
	bool target_attached;
	bool target_reading;
	bool target_writing;
	bool target_in_master_xfer;
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
	bool target_buf_mode;
	uint32_t target_rx_len;
	uint8_t target_rx_buf[CONFIG_I2C_ESP32_TARGET_BUF];
	const uint8_t *target_tx_buf;
	uint32_t target_tx_len;
	uint32_t target_tx_idx;
#endif
#endif
};

typedef void (*irq_connect_cb)(void);

struct i2c_esp32_config {
	int index;

	const struct device *clock_dev;
#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
	const struct i2c_esp32_pin scl;
	const struct i2c_esp32_pin sda;
#endif
	const struct pinctrl_dev_config *pcfg;

	const clock_control_subsys_t clock_subsys;

	const struct {
		bool tx_lsb_first;
		bool rx_lsb_first;
	} mode;

	int irq_source;
	int irq_priority;
	int irq_flags;

	const uint32_t bitrate;
	const uint32_t scl_timeout;
};

static uint32_t i2c_get_src_clk_freq(i2c_clock_source_t clk_src)
{
	uint32_t periph_src_clk_hz = 0;

	switch (clk_src) {
#if SOC_I2C_SUPPORT_APB
	case I2C_CLK_SRC_APB:
		periph_src_clk_hz = esp_clk_apb_freq();
		break;
#endif
#if SOC_I2C_SUPPORT_XTAL
	case I2C_CLK_SRC_XTAL:
		periph_src_clk_hz = esp_clk_xtal_freq();
		break;
#endif
#if SOC_I2C_SUPPORT_RTC
	case I2C_CLK_SRC_RC_FAST:
		esp_clk_tree_enable_src(SOC_MOD_CLK_RC_FAST, true);
		esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_RC_FAST,
					     ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX,
					     &periph_src_clk_hz);
		break;
#endif
#if SOC_I2C_SUPPORT_REF_TICK
	case RMT_CLK_SRC_REF_TICK:
		periph_src_clk_hz = REF_CLK_FREQ;
		break;
#endif
	default:
		LOG_ERR("clock source %d is not supported", clk_src);
		break;
	}

	return periph_src_clk_hz;
}

static i2c_clock_source_t i2c_get_clk_src(uint32_t clk_freq)
{
	i2c_clock_source_t clk_srcs[] = SOC_I2C_CLKS;

	for (size_t i = 0; i < ARRAY_SIZE(clk_srcs); i++) {
		/* I2C SCL clock frequency should not larger than clock source frequency/20 */
		if (clk_freq <= (i2c_get_src_clk_freq(clk_srcs[i]) / 20)) {
			return clk_srcs[i];
		}
	}

	return I2C_CLOCK_INVALID;
}

#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
static int i2c_esp32_config_pin(const struct device *dev)
{
	const struct i2c_esp32_config *config = dev->config;
	int ret = 0;

	if (config->index >= SOC_I2C_NUM) {
		LOG_ERR("Invalid I2C peripheral number");
		return -EINVAL;
	}

	gpio_pin_set_dt(&config->sda.gpio, 1);
	ret = gpio_pin_configure_dt(&config->sda.gpio, GPIO_PULL_UP | GPIO_OUTPUT | GPIO_INPUT);
	esp_rom_gpio_matrix_out(config->sda.gpio.pin, config->sda.sig_out, 0, 0);
	esp_rom_gpio_matrix_in(config->sda.gpio.pin, config->sda.sig_in, 0);

	gpio_pin_set_dt(&config->scl.gpio, 1);
	ret |= gpio_pin_configure_dt(&config->scl.gpio, GPIO_PULL_UP | GPIO_OUTPUT | GPIO_INPUT);
	esp_rom_gpio_matrix_out(config->scl.gpio.pin, config->scl.sig_out, 0, 0);
	esp_rom_gpio_matrix_in(config->scl.gpio.pin, config->scl.sig_in, 0);

	return ret;
}
#endif

/* Some slave device will die by accident and keep the SDA in low level,
 * in this case, master should send several clock to make the slave release the bus.
 * Slave mode of ESP32 might also get in wrong state that held the SDA low,
 * in this case, master device could send a stop signal to make esp32 slave release the bus.
 **/
static void IRAM_ATTR i2c_master_clear_bus(const struct device *dev)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
	const struct i2c_esp32_config *config = dev->config;
	const int scl_half_period = I2C_CLR_BUS_HALF_PERIOD_US; /* use standard 100kHz data rate */
	int i = 0;

	gpio_pin_configure_dt(&config->scl.gpio, GPIO_OUTPUT);
	gpio_pin_configure_dt(&config->sda.gpio, GPIO_OUTPUT | GPIO_INPUT);
	/* If a SLAVE device was in a read operation when the bus was interrupted, */
	/* the SLAVE device is controlling SDA. If the slave is sending a stream of ZERO bytes, */
	/* it will only release SDA during the  ACK bit period. So, this reset code needs */
	/* to synchronize the bit stream with either the ACK bit, or a 1 bit to correctly */
	/* generate a STOP condition. */
	gpio_pin_set_dt(&config->sda.gpio, 1);
	esp_rom_delay_us(scl_half_period);
	while (!gpio_pin_get_dt(&config->sda.gpio) && (i++ < I2C_CLR_BUS_SCL_NUM)) {
		gpio_pin_set_dt(&config->scl.gpio, 1);
		esp_rom_delay_us(scl_half_period);
		gpio_pin_set_dt(&config->scl.gpio, 0);
		esp_rom_delay_us(scl_half_period);
	}
	gpio_pin_set_dt(&config->sda.gpio, 0); /* setup for STOP */
	gpio_pin_set_dt(&config->scl.gpio, 1);
	esp_rom_delay_us(scl_half_period);
	gpio_pin_set_dt(&config->sda.gpio, 1); /* STOP, SDA low -> high while SCL is HIGH */
	i2c_esp32_config_pin(dev);
#else
	i2c_ll_master_clr_bus(data->hal.dev, I2C_LL_RESET_SLV_SCL_PULSE_NUM_DEFAULT, true);
#endif
	i2c_ll_update(data->hal.dev);
}

static void IRAM_ATTR i2c_hw_fsm_reset(const struct device *dev)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

#if !I2C_LL_SUPPORT_HW_FSM_RST
	const struct i2c_esp32_config *config = dev->config;
	int scl_low_period, scl_high_period;
	int scl_start_hold, scl_rstart_setup;
	int scl_stop_hold, scl_stop_setup;
	int sda_hold, sda_sample;
	int timeout;
	uint8_t filter_cfg;

	i2c_ll_get_scl_timing(data->hal.dev, &scl_high_period, &scl_low_period);
	i2c_ll_get_start_timing(data->hal.dev, &scl_rstart_setup, &scl_start_hold);
	i2c_ll_get_stop_timing(data->hal.dev, &scl_stop_setup, &scl_stop_hold);
	i2c_ll_get_sda_timing(data->hal.dev, &sda_sample, &sda_hold);
	i2c_ll_get_tout(data->hal.dev, &timeout);
	i2c_ll_master_get_filter(data->hal.dev, &filter_cfg);

	/* to reset the I2C hw module, we need re-enable the hw */
	clock_control_off(config->clock_dev, config->clock_subsys);
	i2c_master_clear_bus(dev);
	clock_control_on(config->clock_dev, config->clock_subsys);

	i2c_hal_init(&data->hal, config->index);

	i2c_hal_master_init(&data->hal);
	i2c_ll_disable_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);
	i2c_ll_clear_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);
	i2c_ll_set_scl_timing(data->hal.dev, scl_high_period, scl_low_period);
	i2c_ll_master_set_start_timing(data->hal.dev, scl_rstart_setup, scl_start_hold);
	i2c_ll_master_set_stop_timing(data->hal.dev, scl_stop_setup, scl_stop_hold);
	i2c_ll_set_sda_timing(data->hal.dev, sda_sample, sda_hold);
	i2c_ll_set_tout(data->hal.dev, timeout);
	i2c_ll_master_set_filter(data->hal.dev, filter_cfg);
#else
	i2c_ll_master_fsm_rst(data->hal.dev);
	i2c_master_clear_bus(dev);
#endif
	i2c_ll_update(data->hal.dev);
}

static int i2c_esp32_recover(const struct device *dev)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	k_sem_take(&data->transfer_sem, K_FOREVER);
	i2c_hw_fsm_reset(dev);
	k_sem_give(&data->transfer_sem);

	return 0;
}

static void IRAM_ATTR i2c_esp32_configure_bitrate(const struct device *dev, uint32_t bitrate)
{
	const struct i2c_esp32_config *config = dev->config;
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	i2c_clock_source_t sclk = i2c_get_clk_src(bitrate);
	uint32_t clk_freq_mhz = i2c_get_src_clk_freq(sclk);

	i2c_hal_set_bus_timing(&data->hal, bitrate, sclk, clk_freq_mhz);

	if (config->scl_timeout > 0) {
		uint32_t timeout_cycles = MIN(I2C_LL_MAX_TIMEOUT,
					      clk_freq_mhz / MHZ(1) * config->scl_timeout);
		i2c_ll_set_tout(data->hal.dev, timeout_cycles);
		LOG_DBG("SCL timeout: %d us, value: %d", config->scl_timeout, timeout_cycles);
	} else {
		/* Disabling the timeout by clearing the I2C_TIME_OUT_EN bit does not seem to work,
		 * at least for ESP32-C3 (tested with communication to bq76952 chip). So we set the
		 * timeout to maximum supported value instead.
		 */
		i2c_ll_set_tout(data->hal.dev, I2C_LL_MAX_TIMEOUT);
	}

	i2c_ll_update(data->hal.dev);
}

static void i2c_esp32_configure_data_mode(const struct device *dev)
{
	const struct i2c_esp32_config *config = dev->config;
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	i2c_trans_mode_t tx_mode = I2C_DATA_MODE_MSB_FIRST;
	i2c_trans_mode_t rx_mode = I2C_DATA_MODE_MSB_FIRST;

	if (config->mode.tx_lsb_first) {
		tx_mode = I2C_DATA_MODE_LSB_FIRST;
	}

	if (config->mode.rx_lsb_first) {
		rx_mode = I2C_DATA_MODE_LSB_FIRST;
	}

	i2c_ll_set_data_mode(data->hal.dev, tx_mode, rx_mode);
	i2c_ll_master_set_filter(data->hal.dev, I2C_FILTER_CYC_NUM_DEF);
	i2c_ll_update(data->hal.dev);

}

static int i2c_esp32_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;
	uint32_t bitrate;

	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Only I2C Master mode supported.");
		return -ENOTSUP;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		bitrate = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		bitrate = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		bitrate = MHZ(1);
		break;
	default:
		LOG_ERR("Error configuring I2C speed.");
		return -ENOTSUP;
	}

	k_sem_take(&data->transfer_sem, K_FOREVER);

	data->dev_config = dev_config;

	i2c_esp32_configure_bitrate(dev, bitrate);

	k_sem_give(&data->transfer_sem);

	return 0;
}

static int i2c_esp32_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	if (data->dev_config == 0) {
		LOG_ERR("I2C controller not configured");
		return -EIO;
	}

	*config = data->dev_config;

	return 0;
}

static void IRAM_ATTR i2c_esp32_reset_fifo(const struct device *dev)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	/* reset fifo buffers */
	i2c_ll_txfifo_rst(data->hal.dev);
	i2c_ll_rxfifo_rst(data->hal.dev);
}

static int IRAM_ATTR i2c_esp32_transmit(const struct device *dev)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;
	int ret = 0;

	/* Start transmission*/
	i2c_ll_update(data->hal.dev);
	i2c_ll_start_trans(data->hal.dev);
	data->cmd_idx = 0;

	ret = k_sem_take(&data->cmd_sem, K_MSEC(I2C_TRANSFER_TIMEOUT_MSEC));
	if (ret != 0) {
		/* If the I2C slave is powered off or the SDA/SCL is */
		/* connected to ground, for example, I2C hw FSM would get */
		/* stuck in wrong state, we have to reset the I2C module in this case. */
		i2c_hw_fsm_reset(dev);
		return -ETIMEDOUT;
	}

	if (data->status == I2C_STATUS_TIMEOUT) {
		i2c_hw_fsm_reset(dev);
		ret = -ETIMEDOUT;
	} else if (data->status == I2C_STATUS_ACK_ERROR) {
		ret = -EFAULT;
	}

	return ret;
}

static void IRAM_ATTR i2c_esp32_master_start(const struct device *dev)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	i2c_ll_hw_cmd_t cmd = {
		.op_code = I2C_LL_CMD_RESTART
	};

	i2c_ll_master_write_cmd_reg(data->hal.dev, cmd, data->cmd_idx++);
}

static void IRAM_ATTR i2c_esp32_master_stop(const struct device *dev)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	i2c_ll_hw_cmd_t cmd = {
		.op_code = I2C_LL_CMD_STOP
	};

	i2c_ll_master_write_cmd_reg(data->hal.dev, cmd, data->cmd_idx++);
}

static int IRAM_ATTR i2c_esp32_write_addr(const struct device *dev, uint16_t addr)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;
	uint8_t addr_len = 1;
	uint8_t addr_byte = addr & 0xFF;

	data->status = I2C_STATUS_WRITE;

	/* write address value in tx buffer */
	i2c_ll_write_txfifo(data->hal.dev, &addr_byte, 1);
	if (data->dev_config & I2C_ADDR_10_BITS) {
		addr_byte = (addr >> 8) & 0xFF;
		i2c_ll_write_txfifo(data->hal.dev, &addr_byte, 1);
		addr_len++;
	}

	const i2c_ll_hw_cmd_t cmd_end = {
		.op_code = I2C_LL_CMD_END,
	};

	i2c_ll_hw_cmd_t cmd = {
		.op_code = I2C_LL_CMD_WRITE,
		.ack_en = true,
		.byte_num = addr_len,
	};

	i2c_ll_master_write_cmd_reg(data->hal.dev, cmd, data->cmd_idx++);
	i2c_ll_master_write_cmd_reg(data->hal.dev, cmd_end, data->cmd_idx++);
	i2c_ll_master_enable_tx_it(data->hal.dev);

	return i2c_esp32_transmit(dev);
}

static int IRAM_ATTR i2c_esp32_master_read(const struct device *dev, struct i2c_msg *msg)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

	uint32_t msg_len = msg->len;
	uint8_t *msg_buf = msg->buf;
	uint8_t rd_filled = 0;
	int ret = 0;

	data->status = I2C_STATUS_READ;

	i2c_ll_hw_cmd_t cmd = {
		.op_code = I2C_LL_CMD_READ,
	};
	const i2c_ll_hw_cmd_t cmd_end = {
		.op_code = I2C_LL_CMD_END,
	};

	while (msg_len) {
		rd_filled = (msg_len > I2C_LL_FIFO_LEN) ? I2C_LL_FIFO_LEN : (msg_len - 1);

		/* I2C master won't acknowledge the last byte read from the
		 * slave device. Divide the read command in two segments as
		 * recommended by the ESP32 Technical Reference Manual.
		 */
		if (msg_len == 1) {
			rd_filled = 1;
			cmd.ack_val = 1;
		} else {
			cmd.ack_val = 0;
		}
		cmd.byte_num = rd_filled;

		i2c_ll_master_write_cmd_reg(data->hal.dev, cmd, data->cmd_idx++);
		i2c_ll_master_write_cmd_reg(data->hal.dev, cmd_end, data->cmd_idx++);
		i2c_ll_master_enable_tx_it(data->hal.dev);
		ret = i2c_esp32_transmit(dev);
		if (ret < 0) {
			return ret;
		}

		i2c_ll_read_rxfifo(data->hal.dev, msg_buf, rd_filled);
		msg_buf += rd_filled;
		msg_len -= rd_filled;
	}

	return 0;
}

static int IRAM_ATTR i2c_esp32_read_msg(const struct device *dev,
					struct i2c_msg *msg, uint16_t addr)
{
	int ret = 0;

	/* Set the R/W bit to R */
	addr |= BIT(0);

	if (msg->flags & I2C_MSG_RESTART) {
		i2c_esp32_master_start(dev);
		ret = i2c_esp32_write_addr(dev, addr);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_esp32_master_read(dev, msg);
	if (ret < 0) {
		return ret;
	}

	if (msg->flags & I2C_MSG_STOP) {
		i2c_esp32_master_stop(dev);
		ret = i2c_esp32_transmit(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int IRAM_ATTR i2c_esp32_master_write(const struct device *dev, struct i2c_msg *msg)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;
	uint8_t wr_filled = 0;
	uint32_t msg_len = msg->len;
	uint8_t *msg_buf = msg->buf;
	int ret = 0;

	data->status = I2C_STATUS_WRITE;

	i2c_ll_hw_cmd_t cmd = {
		.op_code = I2C_LL_CMD_WRITE,
		.ack_en = true,
	};

	const i2c_ll_hw_cmd_t cmd_end = {
		.op_code = I2C_LL_CMD_END,
	};

	while (msg_len) {
		wr_filled = (msg_len > I2C_LL_FIFO_LEN) ? I2C_LL_FIFO_LEN : msg_len;
		cmd.byte_num = wr_filled;

		if (wr_filled > 0) {
			i2c_ll_write_txfifo(data->hal.dev, msg_buf, wr_filled);
			i2c_ll_master_write_cmd_reg(data->hal.dev, cmd, data->cmd_idx++);
			i2c_ll_master_write_cmd_reg(data->hal.dev, cmd_end, data->cmd_idx++);
			i2c_ll_master_enable_tx_it(data->hal.dev);
			ret = i2c_esp32_transmit(dev);
			if (ret < 0) {
				return ret;
			}
		}

		msg_buf += wr_filled;
		msg_len -= wr_filled;
	}

	return 0;
}

static int IRAM_ATTR i2c_esp32_write_msg(const struct device *dev,
					 struct i2c_msg *msg, uint16_t addr)
{
	int ret = 0;

	if (msg->flags & I2C_MSG_RESTART) {
		i2c_esp32_master_start(dev);
		ret = i2c_esp32_write_addr(dev, addr);
		if (ret < 0) {
			return ret;
		}
	}

	ret = i2c_esp32_master_write(dev, msg);
	if (ret < 0) {
		return ret;
	}

	if (msg->flags & I2C_MSG_STOP) {
		i2c_esp32_master_stop(dev);
		ret = i2c_esp32_transmit(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#if I2C_ESP32_TARGET_ENABLED
static void i2c_esp32_target_pause(const struct device *dev);
static void i2c_esp32_target_resume(const struct device *dev);
#endif

static int IRAM_ATTR i2c_esp32_transfer(const struct device *dev, struct i2c_msg *msgs,
					uint8_t num_msgs, uint16_t addr)
{
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;
	struct i2c_msg *current, *next;
	uint32_t timeout = I2C_TRANSFER_TIMEOUT_MSEC * USEC_PER_MSEC;
	int ret = 0;

	while (i2c_ll_is_bus_busy(data->hal.dev)) {
		k_busy_wait(1);
		if (timeout-- == 0) {
			return -EBUSY;
		}
	}

	/* Check for validity of all messages before transfer */
	current = msgs;

	/* Add restart flag on first message to send start event */
	current->flags |= I2C_MSG_RESTART;

	for (int k = 1; k <= num_msgs; k++) {
		if (k < num_msgs) {
			next = current + 1;

			/* messages of different direction require restart event */
			if ((current->flags & I2C_MSG_RW_MASK) != (next->flags & I2C_MSG_RW_MASK)) {
				if (!(next->flags & I2C_MSG_RESTART)) {
					ret = -EINVAL;
					break;
				}
			}

			/* check if there is any stop event in the middle of the transaction */
			if (current->flags & I2C_MSG_STOP) {
				ret = -EINVAL;
				break;
			}
		}

		current++;
	}

	if (ret) {
		return ret;
	}

	k_sem_take(&data->transfer_sem, K_FOREVER);

#if I2C_ESP32_TARGET_ENABLED
	bool was_target = data->target_attached;

	if (was_target) {
		i2c_esp32_target_pause(dev);
	}
#endif

	/* Mask out unused address bits, and make room for R/W bit */
	addr &= BIT_MASK(data->dev_config & I2C_ADDR_10_BITS ? 10 : 7);
	addr <<= 1;

	for (; num_msgs > 0; num_msgs--, msgs++) {

		if (data->status == I2C_STATUS_TIMEOUT || i2c_ll_is_bus_busy(data->hal.dev)) {
			i2c_hw_fsm_reset(dev);
		}

		/* reset all fifo buffer before start */
		i2c_esp32_reset_fifo(dev);

		/* These two interrupts some times can not be cleared when the FSM gets stuck. */
		/* So we disable them when these two interrupt occurs and re-enable them here. */
		i2c_ll_disable_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);
		i2c_ll_clear_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);

		if ((msgs->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			ret = i2c_esp32_read_msg(dev, msgs, addr);
		} else {
			ret = i2c_esp32_write_msg(dev, msgs, addr);
		}

		if (ret < 0) {
			break;
		}
	}

#if I2C_ESP32_TARGET_ENABLED
	if (was_target) {
		i2c_esp32_target_resume(dev);
	}
#endif

	k_sem_give(&data->transfer_sem);

	return ret;
}

#if I2C_ESP32_TARGET_ENABLED

static void i2c_esp32_target_setup(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;
	struct i2c_target_config *cfg = data->target_cfg;
	bool addr10 = (cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) != 0;

	i2c_ll_disable_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);
	i2c_ll_clear_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);

	i2c_hal_slave_init(&data->hal);
	i2c_ll_enable_fifo_mode(data->hal.dev, true);
	i2c_ll_set_slave_addr(data->hal.dev, cfg->address, addr10);
	i2c_ll_set_rxfifo_full_thr(data->hal.dev, I2C_LL_FIFO_LEN / 2);
	i2c_ll_set_txfifo_empty_thr(data->hal.dev, I2C_LL_FIFO_LEN / 2);
	i2c_ll_set_tout(data->hal.dev, I2C_LL_MAX_TIMEOUT);
	i2c_ll_set_sda_timing(data->hal.dev, 10, 10);

	i2c_ll_slave_set_stretch_protect_num(data->hal.dev, I2C_LL_STRETCH_PROTECT_TIME);
	i2c_ll_slave_enable_scl_stretch(data->hal.dev, true);
	i2c_ll_slave_clear_stretch(data->hal.dev);
	i2c_ll_enable_intr_mask(data->hal.dev, I2C_LL_SLAVE_RX_EVENT_INTR);
	i2c_ll_update(data->hal.dev);
}

static void i2c_esp32_target_teardown(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;

	i2c_ll_slave_enable_scl_stretch(data->hal.dev, false);
	i2c_ll_slave_disable_rx_it(data->hal.dev);
	i2c_ll_slave_disable_tx_it(data->hal.dev);
	i2c_ll_disable_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);
	i2c_ll_clear_intr_mask(data->hal.dev, I2C_LL_INTR_MASK);
}

static void i2c_esp32_target_pause(const struct device *dev)
{
	const struct i2c_esp32_config *config = dev->config;
	struct i2c_esp32_data *data = dev->data;

	data->target_in_master_xfer = true;
	i2c_esp32_target_teardown(dev);
	i2c_hal_master_init(&data->hal);
	i2c_esp32_configure_data_mode(dev);
	i2c_esp32_configure_bitrate(dev, config->bitrate);
}

static void i2c_esp32_target_resume(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;

	i2c_esp32_target_setup(dev);
	data->target_reading = false;
	data->target_writing = false;
	data->target_in_master_xfer = false;
}

static inline void IRAM_ATTR i2c_esp32_target_drain_rx(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target_cfg->callbacks;
	uint32_t cnt = 0;
	uint8_t byte;

	i2c_ll_get_rxfifo_cnt(data->hal.dev, &cnt);
	while (cnt--) {
		bool first = !data->target_writing;

		i2c_ll_read_rxfifo(data->hal.dev, &byte, 1);
		data->target_writing = true;
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
		if (data->target_buf_mode) {
			if (data->target_rx_len < CONFIG_I2C_ESP32_TARGET_BUF) {
				data->target_rx_buf[data->target_rx_len++] = byte;
			}
			continue;
		}
#endif
		if (first && cb->write_requested) {
			cb->write_requested(data->target_cfg);
		}
		if (cb->write_received) {
			cb->write_received(data->target_cfg, byte);
		}
	}
}

static inline void IRAM_ATTR i2c_esp32_target_flush_rx_buf(const struct device *dev)
{
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
	struct i2c_esp32_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target_cfg->callbacks;

	if (!data->target_buf_mode || data->target_rx_len == 0) {
		return;
	}
	if (cb->buf_write_received) {
		cb->buf_write_received(data->target_cfg, data->target_rx_buf, data->target_rx_len);
	}
	data->target_rx_len = 0;
#else
	ARG_UNUSED(dev);
#endif
}

#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
static inline void IRAM_ATTR i2c_esp32_target_push_one_buf(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target_cfg->callbacks;
	uint8_t byte = 0xff;

	if (!data->target_reading) {
		uint8_t *ptr = NULL;
		uint32_t len = 0;

		data->target_reading = true;
		data->target_tx_buf = NULL;
		data->target_tx_len = 0;
		data->target_tx_idx = 0;
		if (cb->buf_read_requested &&
		    cb->buf_read_requested(data->target_cfg, &ptr, &len) == 0) {
			data->target_tx_buf = ptr;
			data->target_tx_len = len;
		}
	}
	if (data->target_tx_buf && data->target_tx_idx < data->target_tx_len) {
		byte = data->target_tx_buf[data->target_tx_idx++];
	}
	i2c_ll_write_txfifo(data->hal.dev, &byte, 1);
}
#endif

static inline void IRAM_ATTR i2c_esp32_target_push_one_byte(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target_cfg->callbacks;
	uint8_t byte = 0xff;

	if (!data->target_reading) {
		data->target_reading = true;
		if (cb->read_requested) {
			cb->read_requested(data->target_cfg, &byte);
		}
	} else if (cb->read_processed) {
		cb->read_processed(data->target_cfg, &byte);
	}
	i2c_ll_write_txfifo(data->hal.dev, &byte, 1);
}

static inline void IRAM_ATTR i2c_esp32_target_push_one(const struct device *dev)
{
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
	struct i2c_esp32_data *data = dev->data;

	if (data->target_buf_mode) {
		i2c_esp32_target_push_one_buf(dev);
		return;
	}
#endif
	i2c_esp32_target_push_one_byte(dev);
}

static inline void IRAM_ATTR i2c_esp32_target_end_xaction(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target_cfg->callbacks;

	i2c_esp32_target_drain_rx(dev);
	i2c_esp32_target_flush_rx_buf(dev);
	if (cb->stop) {
		cb->stop(data->target_cfg);
	}
	data->target_reading = false;
	data->target_writing = false;
}

static void IRAM_ATTR i2c_esp32_target_isr(const struct device *dev)
{
	struct i2c_esp32_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target_cfg->callbacks;
	uint32_t int_mask = data->hal.dev->int_status.val;

	i2c_ll_clear_intr_mask(data->hal.dev, int_mask);

	if ((int_mask & I2C_ARBITRATION_LOST_INT_ST_M) && cb->error) {
		cb->error(data->target_cfg, I2C_ERROR_ARBITRATION);
	}
	if ((int_mask & I2C_TIME_OUT_INT_ST_M) && cb->error) {
		cb->error(data->target_cfg, I2C_ERROR_TIMEOUT);
	}

	if (int_mask & I2C_RXFIFO_WM_INT_ST_M) {
		i2c_esp32_target_drain_rx(dev);
	}

	if (int_mask & I2C_SLAVE_STRETCH_INT_ST_M) {
		i2c_slave_stretch_cause_t cause;

		i2c_ll_slave_get_stretch_cause(data->hal.dev, &cause);

		if (cause == I2C_SLAVE_STRETCH_CAUSE_ADDRESS_MATCH) {
			i2c_slave_read_write_status_t rw =
				i2c_ll_slave_get_read_write_status(data->hal.dev);
			uint32_t rx_pending = 0;

			/* Repeated-start does not fire TRANS_COMPLETE on every
			 * SoC, so drain bytes from the preceding write phase.
			 */
			i2c_ll_get_rxfifo_cnt(data->hal.dev, &rx_pending);
			if (rx_pending) {
				i2c_esp32_target_end_xaction(dev);
			}

			if (rw == I2C_SLAVE_READ_BY_MASTER) {
				data->target_reading = false;
				i2c_ll_txfifo_rst(data->hal.dev);
				i2c_esp32_target_push_one(dev);
			} else {
				data->target_writing = false;
			}
		} else if (cause == I2C_SLAVE_STRETCH_CAUSE_TX_EMPTY) {
			i2c_esp32_target_push_one(dev);
		} else if (cause == I2C_SLAVE_STRETCH_CAUSE_RX_FULL) {
			i2c_esp32_target_drain_rx(dev);
		}
		i2c_ll_slave_clear_stretch(data->hal.dev);
	}

	if (int_mask & I2C_TRANS_COMPLETE_INT_ST_M) {
		i2c_ll_slave_disable_tx_it(data->hal.dev);
		i2c_esp32_target_end_xaction(dev);
		i2c_ll_txfifo_rst(data->hal.dev);
		i2c_ll_rxfifo_rst(data->hal.dev);
	}
}
#endif

static void IRAM_ATTR i2c_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;
	i2c_intr_event_t evt_type = I2C_INTR_EVENT_ERR;

#if I2C_ESP32_TARGET_ENABLED
	if (data->target_attached && !data->target_in_master_xfer) {
		i2c_esp32_target_isr(dev);
		return;
	}
#endif

	if (data->status == I2C_STATUS_WRITE) {
		i2c_hal_master_handle_tx_event(&data->hal, &evt_type);
	} else if (data->status == I2C_STATUS_READ) {
		i2c_hal_master_handle_rx_event(&data->hal, &evt_type);
	}

	if (evt_type == I2C_INTR_EVENT_NACK) {
		data->status = I2C_STATUS_ACK_ERROR;
	} else if (evt_type == I2C_INTR_EVENT_TOUT) {
		data->status = I2C_STATUS_TIMEOUT;
	} else if (evt_type == I2C_INTR_EVENT_ARBIT_LOST) {
		data->status = I2C_STATUS_TIMEOUT;
	} else if (evt_type == I2C_INTR_EVENT_TRANS_DONE) {
		data->status = I2C_STATUS_DONE;
	}

	k_sem_give(&data->cmd_sem);
}

#if I2C_ESP32_TARGET_ENABLED
static int i2c_esp32_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_esp32_data *data = dev->data;

	if (!cfg) {
		return -EINVAL;
	}
	if (data->target_attached) {
		return -EBUSY;
	}

	k_sem_take(&data->transfer_sem, K_FOREVER);

	data->target_cfg = cfg;
	data->target_reading = false;
	data->target_writing = false;
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
	data->target_buf_mode = (cfg->callbacks->buf_write_received != NULL) ||
				(cfg->callbacks->buf_read_requested != NULL);
	data->target_rx_len = 0;
	data->target_tx_buf = NULL;
	data->target_tx_len = 0;
	data->target_tx_idx = 0;
#endif

	i2c_esp32_target_setup(dev);

	data->target_attached = true;
	data->dev_config = 0;

	k_sem_give(&data->transfer_sem);

	return 0;
}

static int i2c_esp32_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_esp32_data *data = dev->data;

	if (!data->target_attached) {
		return -EINVAL;
	}
	if (cfg != data->target_cfg) {
		return -EINVAL;
	}

	k_sem_take(&data->transfer_sem, K_FOREVER);

	i2c_esp32_target_teardown(dev);

	data->target_attached = false;
	data->target_cfg = NULL;
	data->target_reading = false;
	data->target_writing = false;
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
	data->target_buf_mode = false;
	data->target_rx_len = 0;
	data->target_tx_buf = NULL;
	data->target_tx_len = 0;
	data->target_tx_idx = 0;
#endif

	i2c_hal_master_init(&data->hal);
	i2c_esp32_configure_data_mode(dev);

	k_sem_give(&data->transfer_sem);

	return 0;
}
#endif

static DEVICE_API(i2c, i2c_esp32_driver_api) = {
	.configure = i2c_esp32_configure,
	.get_config = i2c_esp32_get_config,
	.transfer = i2c_esp32_transfer,
	.recover_bus = i2c_esp32_recover,
#if I2C_ESP32_TARGET_ENABLED
	.target_register = i2c_esp32_target_register,
	.target_unregister = i2c_esp32_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

static int IRAM_ATTR i2c_esp32_init(const struct device *dev)
{
	const struct i2c_esp32_config *config = dev->config;
	struct i2c_esp32_data *data = (struct i2c_esp32_data *const)(dev)->data;

#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
	if (!gpio_is_ready_dt(&config->scl.gpio)) {
		LOG_ERR("SCL GPIO device is not ready");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&config->sda.gpio)) {
		LOG_ERR("SDA GPIO device is not ready");
		return -EINVAL;
	}
#endif
	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("Failed to configure I2C pins");
		return -EINVAL;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	clock_control_on(config->clock_dev, config->clock_subsys);

	ret = esp_intr_alloc(config->irq_source,
			ESP_PRIO_TO_FLAGS(config->irq_priority) |
			ESP_INT_FLAGS_CHECK(config->irq_flags) | ESP_INTR_FLAG_IRAM,
			i2c_esp32_isr,
			(void *)dev,
			NULL);

	if (ret != 0) {
		LOG_ERR("could not allocate interrupt (err %d)", ret);
		return ret;
	}

	i2c_hal_init(&data->hal, config->index);

	i2c_hal_master_init(&data->hal);

	i2c_esp32_configure_data_mode(dev);

	return i2c_esp32_configure(dev, I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate));
}

#define I2C(idx) DT_NODELABEL(i2c##idx)

#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
#define I2C_ESP32_GET_PIN_INFO(idx)					\
	.scl = {							\
		.gpio = GPIO_DT_SPEC_GET(I2C(idx), scl_gpios),		\
		.sig_out = I2CEXT##idx##_SCL_OUT_IDX,			\
		.sig_in = I2CEXT##idx##_SCL_IN_IDX,			\
	},								\
	.sda = {							\
		.gpio = GPIO_DT_SPEC_GET(I2C(idx), sda_gpios),		\
		.sig_out = I2CEXT##idx##_SDA_OUT_IDX,			\
		.sig_in = I2CEXT##idx##_SDA_IN_IDX,			\
	},
#else
#define I2C_ESP32_GET_PIN_INFO(idx)
#endif /* I2C_LL_SUPPORT_HW_CLR_BUS */

#define I2C_ESP32_TIMEOUT(inst)						\
	COND_CODE_1(DT_NODE_HAS_PROP(I2C(inst), scl_timeout_us),	\
		    (DT_PROP(I2C(inst), scl_timeout_us)), (0))

#define I2C_ESP32_FREQUENCY(bitrate)					\
	 (bitrate == I2C_BITRATE_STANDARD ? KHZ(100)			\
	: bitrate == I2C_BITRATE_FAST     ? KHZ(400)			\
	: bitrate == I2C_BITRATE_FAST_PLUS  ? MHZ(1) : 0)

#define I2C_FREQUENCY(idx)						\
	I2C_ESP32_FREQUENCY(DT_PROP(I2C(idx), clock_frequency))

#define ESP32_I2C_INIT(idx)									   \
												   \
	PINCTRL_DT_DEFINE(I2C(idx));								   \
												   \
	static struct i2c_esp32_data i2c_esp32_data_##idx = {					   \
		.hal = {									   \
			.dev = (i2c_dev_t *) DT_REG_ADDR(I2C(idx)),				   \
		},										   \
		.cmd_sem = Z_SEM_INITIALIZER(i2c_esp32_data_##idx.cmd_sem, 0, 1),		   \
		.transfer_sem = Z_SEM_INITIALIZER(i2c_esp32_data_##idx.transfer_sem, 1, 1),	   \
	};											   \
												   \
	static const struct i2c_esp32_config i2c_esp32_config_##idx = {				   \
		.index = idx,									   \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(I2C(idx))),				   \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(I2C(idx)),					   \
		.clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(I2C(idx), offset),	   \
		I2C_ESP32_GET_PIN_INFO(idx)							   \
		.mode = {									   \
			.tx_lsb_first = DT_PROP(I2C(idx), tx_lsb),				   \
			.rx_lsb_first = DT_PROP(I2C(idx), rx_lsb),				   \
		},										   \
		.irq_source = DT_IRQ_BY_IDX(I2C(idx), 0, irq),				   \
		.irq_priority = DT_IRQ_BY_IDX(I2C(idx), 0, priority),		   \
		.irq_flags = DT_IRQ_BY_IDX(I2C(idx), 0, flags),				   \
		.bitrate = I2C_FREQUENCY(idx),							   \
		.scl_timeout = I2C_ESP32_TIMEOUT(idx),						   \
	};											   \
	I2C_DEVICE_DT_DEFINE(I2C(idx), i2c_esp32_init, NULL, &i2c_esp32_data_##idx,		   \
			     &i2c_esp32_config_##idx, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,	   \
			     &i2c_esp32_driver_api);

#if DT_NODE_HAS_STATUS_OKAY(I2C(0))
#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
#if !DT_NODE_HAS_PROP(I2C(0), sda_gpios) || !DT_NODE_HAS_PROP(I2C(0), scl_gpios)
#error "Missing <sda-gpios> and <scl-gpios> properties to build for this target."
#endif
#else
#if DT_NODE_HAS_PROP(I2C(0), sda_gpios) || DT_NODE_HAS_PROP(I2C(0), scl_gpios)
#error "Properties <sda-gpios> and <scl-gpios> are not required for this target."
#endif
#endif /* !I2C_LL_SUPPORT_HW_CLR_BUS */
ESP32_I2C_INIT(0);
#endif /* DT_NODE_HAS_STATUS_OKAY(I2C(0)) */

#if DT_NODE_HAS_STATUS_OKAY(I2C(1))
#ifndef I2C_LL_SUPPORT_HW_CLR_BUS
#if !DT_NODE_HAS_PROP(I2C(1), sda_gpios) || !DT_NODE_HAS_PROP(I2C(1), scl_gpios)
#error "Missing <sda-gpios> and <scl-gpios> properties to build for this target."
#endif
#else
#if DT_NODE_HAS_PROP(I2C(1), sda_gpios) || DT_NODE_HAS_PROP(I2C(1), scl_gpios)
#error "Properties <sda-gpios> and <scl-gpios> are not required for this target."
#endif
#endif /* !I2C_LL_SUPPORT_HW_CLR_BUS */
ESP32_I2C_INIT(1);
#endif /* DT_NODE_HAS_STATUS_OKAY(I2C(1)) */
