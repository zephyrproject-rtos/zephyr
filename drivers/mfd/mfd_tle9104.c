/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle9104

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/tle9104.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

/*
 * The values for the defines below as well as the register definitions were
 * taken from the datasheet, which can be found at:
 * https://www.infineon.com/dgdl/Infineon-TLE9104SH-DataSheet-v01_31-EN.pdf?fileId=5546d462766cbe86017676144d76581b
 */
#define TLE9104_RESET_DURATION_TIME_US                    10
#define TLE9104_RESET_DURATION_WAIT_TIME_SAFETY_MARGIN_US 200
#define TLE9104_RESET_DURATION_WAIT_TIME_US               10
#define TLE9104_INITIALIZATION_TIMEOUT_MS                 1
#define TLE9104_ICVERSIONID                               0xB1

#define TLE9104_FRAME_RW_POS                 15
#define TLE9104_FRAME_PARITY_POS             14
#define TLE9104_FRAME_FAULTCOMMUNICATION_POS 13
#define TLE9104_FRAME_FAULTGLOBAL_POS        12
#define TLE9104_FRAME_ADDRESS_POS            8
#define TLE9104_FRAME_DATA_POS               0

#define TLE9104_CFG_CWDTIME_LENGTH 2
#define TLE9104_CFG_CWDTIME_POS    6

#define TLE9104_OFFDIAGCFG_DIAGFILTCFG_LENGTH 2
#define TLE9104_OFFDIAGCFG_DIAGFILTCFG_POS    4
#define TLE9104_OFFDIAGCFG_OUT4DIAGEN_BIT     BIT(3)
#define TLE9104_OFFDIAGCFG_OUT3DIAGEN_BIT     BIT(2)
#define TLE9104_OFFDIAGCFG_OUT2DIAGEN_BIT     BIT(1)
#define TLE9104_OFFDIAGCFG_OUT1DIAGEN_BIT     BIT(0)

#define TLE9104_ONDIAGCFG_OCFILTCFG_LENGTH 3
#define TLE9104_ONDIAGCFG_OCFILTCFG_POS    2
#define TLE9104_ONDIAGCFG_OCTH_LENGTH      2
#define TLE9104_ONDIAGCFG_OCTH_POS         0

#define TLE9104_DIAGOUT12ON_OUT2STAT_BIT     BIT(7)
#define TLE9104_DIAGOUT12ON_OUT1STAT_BIT     BIT(6)
#define TLE9104_DIAGOUT12ON_DIAGCH2ON_LENGTH 3
#define TLE9104_DIAGOUT12ON_DIAGCH2ON_POS    3
#define TLE9104_DIAGOUT12ON_DIAGCH1ON_LENGTH 3
#define TLE9104_DIAGOUT12ON_DIAGCH1ON_POS    0

#define TLE9104_DIAGOUT34ON_OUT4STAT_BIT     BIT(7)
#define TLE9104_DIAGOUT34ON_OUT3STAT_BIT     BIT(6)
#define TLE9104_DIAGOUT34ON_DIAGCH4ON_LENGTH 3
#define TLE9104_DIAGOUT34ON_DIAGCH4ON_POS    3
#define TLE9104_DIAGOUT34ON_DIAGCH3ON_LENGTH 3
#define TLE9104_DIAGOUT34ON_DIAGCH3ON_POS    0

#define TLE9104_DIAGOFF_DIAGCH4OFF_LENGTH 2
#define TLE9104_DIAGOFF_DIAGCH4OFF_POS    6
#define TLE9104_DIAGOFF_DIAGCH3OFF_LENGTH 2
#define TLE9104_DIAGOFF_DIAGCH3OFF_POS    4
#define TLE9104_DIAGOFF_DIAGCH2OFF_LENGTH 2
#define TLE9104_DIAGOFF_DIAGCH2OFF_POS    2
#define TLE9104_DIAGOFF_DIAGCH1OFF_LENGTH 2
#define TLE9104_DIAGOFF_DIAGCH1OFF_POS    0

#define TLE9104_CTRL_OUT1ONS_BIT                BIT(1)
#define TLE9104_CTRL_OUT1ONC_BIT                BIT(0)
#define TLE9104_CFG_OUT1DD_BIT                  BIT(0)
#define TLE9104_GLOBALSTATUS_OUTEN_BIT          BIT(7)
#define TLE9104_GLOBALSTATUS_POR_LATCH_BIT      BIT(0)
#define TLE9104_SPIFRAME_FAULTCOMMUNICATION_BIT BIT(13)

enum tle9104_register {
	TLE9104REGISTER_CTRL = 0x00,
	TLE9104REGISTER_CFG = 0x01,
	TLE9104REGISTER_OFFDIAGCFG = 0x02,
	TLE9104REGISTER_ONDIAGCFG = 0x03,
	TLE9104REGISTER_DIAGOUT12ON = 0x04,
	TLE9104REGISTER_DIAGOUT34ON = 0x05,
	TLE9104REGISTER_DIAGOFF = 0x06,
	TLE9104REGISTER_GLOBALSTATUS = 0x07,
	TLE9104REGISTER_ICVID = 0x08,
};

LOG_MODULE_REGISTER(infineon_tle9104, CONFIG_MFD_LOG_LEVEL);

struct tle9104_config {
	struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_reset;
	const struct gpio_dt_spec gpio_enable;
	const struct gpio_dt_spec gpio_control[TLE9104_GPIO_COUNT];
	uint16_t diagnostic_filter_time;
	uint16_t overcurrent_shutdown_delay_time;
	uint16_t overcurrent_shutdown_threshold;
};

struct tle9104_data {
	/* communication watchdog is getting ignored */
	bool cwd_ignore;
	/* each bit is one output channel, bit 0 = OUT1, ... */
	uint8_t previous_state;
	struct k_mutex lock;
};

static void tle9104_set_register_bits(uint8_t *destination, uint8_t pos, uint8_t length,
				      uint8_t value)
{
	*destination &= ~GENMASK(pos + length - 1, pos);
	*destination |= FIELD_PREP(GENMASK(pos + length - 1, pos), value);
}

static uint8_t tle9104_get_register_bits(uint8_t value, uint8_t pos, uint8_t length)
{
	return FIELD_GET(GENMASK(pos + length - 1, pos), value);
}

static int tle9104_calculate_parity(uint16_t value)
{
	int parity = 1 + POPCOUNT(value);

	if ((value & BIT(TLE9104_FRAME_PARITY_POS)) != 0) {
		parity--;
	}

	return parity % 2;
}

static void tle9104_apply_parity(uint16_t *value)
{
	int parity = tle9104_calculate_parity(*value);

	WRITE_BIT(*value, TLE9104_FRAME_PARITY_POS, parity);
}

static bool tle9104_check_parity(uint16_t value)
{
	int parity = tle9104_calculate_parity(value);

	return ((value & BIT(TLE9104_FRAME_PARITY_POS)) >> TLE9104_FRAME_PARITY_POS) == parity;
}

static int tle9104_transceive_frame(const struct device *dev, bool write,
				    enum tle9104_register write_reg, uint8_t write_data,
				    enum tle9104_register *read_reg, uint8_t *read_data)
{
	const struct tle9104_config *config = dev->config;
	struct tle9104_data *data = dev->data;
	uint16_t write_frame;
	uint16_t read_frame;
	int result;
	uint8_t buffer_tx[2];
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	write_frame = write_data << TLE9104_FRAME_DATA_POS;
	write_frame |= write_reg << TLE9104_FRAME_ADDRESS_POS;
	WRITE_BIT(write_frame, TLE9104_FRAME_RW_POS, write);
	tle9104_apply_parity(&write_frame);
	sys_put_be16(write_frame, buffer_tx);
	LOG_DBG("writing in register 0x%02X of TLE9104 value 0x%02X, complete frame 0x%04X",
		write_reg, write_data, write_frame);

	result = spi_transceive_dt(&config->bus, &tx, &rx);
	if (result != 0) {
		LOG_ERR("spi_write failed with error %i", result);
		return result;
	}

	read_frame = sys_get_be16(buffer_rx);
	LOG_DBG("received complete frame 0x%04X", read_frame);

	if (!tle9104_check_parity(read_frame)) {
		LOG_ERR("parity check for received frame of TLE9104 failed");
		return -EIO;
	}

	if (!data->cwd_ignore) {
		if ((TLE9104_SPIFRAME_FAULTCOMMUNICATION_BIT & read_frame) != 0) {
			LOG_WRN("%s: communication fault reported by TLE9104", dev->name);
		}
	}

	*read_reg = FIELD_GET(GENMASK(TLE9104_FRAME_FAULTGLOBAL_POS - 1, TLE9104_FRAME_ADDRESS_POS),
			      read_frame);
	*read_data = FIELD_GET(GENMASK(TLE9104_FRAME_ADDRESS_POS - 1, TLE9104_FRAME_DATA_POS),
			       read_frame);

	return 0;
}

static int tle9104_write_register(const struct device *dev, enum tle9104_register reg,
				  uint8_t value)
{
	enum tle9104_register read_reg;
	uint8_t read_data;

	return tle9104_transceive_frame(dev, true, reg, value, &read_reg, &read_data);
}

static int tle9104_write_state_internal(const struct device *dev, uint8_t state)
{
	const struct tle9104_config *config = dev->config;
	struct tle9104_data *data = dev->data;
	bool spi_update_required = false;
	uint8_t register_ctrl = 0x00;
	int result;

	LOG_DBG("writing state 0x%02X to TLE9104", state);

	for (size_t i = 0; i < TLE9104_GPIO_COUNT; ++i) {
		uint8_t mask = GENMASK(i, i);
		bool current_value = (state & mask) != 0;
		bool previous_value = (data->previous_state & mask) != 0;

		/*
		 * Setting the OUTx_ON bits results in a high impedance output,
		 * clearing them pulls the output to ground. Therefore the
		 * meaning here is intentionally inverted, as this will then turn
		 * out for a low active open drain output to be pulled to ground
		 * if set to off.
		 */
		if (current_value == 0) {
			register_ctrl |= TLE9104_CTRL_OUT1ONS_BIT << (2 * i);
		} else {
			register_ctrl |= TLE9104_CTRL_OUT1ONC_BIT << (2 * i);
		}

		if (current_value == previous_value) {
			continue;
		}

		if (config->gpio_control[i].port == NULL) {
			spi_update_required = true;
			continue;
		}

		result = gpio_pin_set_dt(&config->gpio_control[i], current_value);
		if (result != 0) {
			LOG_ERR("unable to set control GPIO");
			return result;
		}
	}

	if (spi_update_required) {
		result = tle9104_write_register(dev, TLE9104REGISTER_CTRL, register_ctrl);
		if (result != 0) {
			LOG_ERR("unable to set control register");
			return result;
		}
	}

	data->previous_state = state;

	return 0;
}

int tle9104_write_state(const struct device *dev, uint8_t state)
{
	struct tle9104_data *data = dev->data;
	int result;

	k_mutex_lock(&data->lock, K_FOREVER);
	result = tle9104_write_state_internal(dev, state);
	k_mutex_unlock(&data->lock);

	return result;
}

static int
tle9104_get_diagnostics_internal(const struct device *dev,
				 struct gpio_tle9104_channel_diagnostics diag[TLE9104_GPIO_COUNT])
{
	enum tle9104_register read_reg;
	uint8_t diag_out12_on;
	uint8_t diag_out34_on;
	uint8_t diag_off;

	int result = tle9104_transceive_frame(dev, false, TLE9104REGISTER_DIAGOUT12ON, 0x00,
					      &read_reg, &diag_out12_on);
	if (result != 0) {
		return result;
	}

	result = tle9104_transceive_frame(dev, false, TLE9104REGISTER_DIAGOUT34ON, 0x00, &read_reg,
					  &diag_out12_on);
	if (result != 0) {
		return result;
	}
	if (read_reg != TLE9104REGISTER_DIAGOUT12ON) {
		LOG_ERR("expected to read different register");
		return -EFAULT;
	}

	result = tle9104_transceive_frame(dev, false, TLE9104REGISTER_DIAGOFF, 0x00, &read_reg,
					  &diag_out34_on);
	if (result != 0) {
		return result;
	}
	if (read_reg != TLE9104REGISTER_DIAGOUT34ON) {
		LOG_ERR("expected to read different register");
		return -EFAULT;
	}

	result = tle9104_transceive_frame(dev, false, TLE9104REGISTER_DIAGOFF, 0x00, &read_reg,
					  &diag_off);
	if (result != 0) {
		return result;
	}
	if (read_reg != TLE9104REGISTER_DIAGOFF) {
		LOG_ERR("expected to read different register");
		return -EFAULT;
	}

	diag[0].on = tle9104_get_register_bits(diag_out12_on, TLE9104_DIAGOUT12ON_DIAGCH1ON_POS,
					       TLE9104_DIAGOUT12ON_DIAGCH1ON_LENGTH);
	diag[1].on = tle9104_get_register_bits(diag_out12_on, TLE9104_DIAGOUT12ON_DIAGCH2ON_POS,
					       TLE9104_DIAGOUT12ON_DIAGCH2ON_LENGTH);
	diag[2].on = tle9104_get_register_bits(diag_out34_on, TLE9104_DIAGOUT34ON_DIAGCH3ON_POS,
					       TLE9104_DIAGOUT34ON_DIAGCH3ON_LENGTH);
	diag[3].on = tle9104_get_register_bits(diag_out34_on, TLE9104_DIAGOUT34ON_DIAGCH4ON_POS,
					       TLE9104_DIAGOUT34ON_DIAGCH4ON_LENGTH);
	diag[0].off = tle9104_get_register_bits(diag_off, TLE9104_DIAGOFF_DIAGCH1OFF_POS,
						TLE9104_DIAGOFF_DIAGCH1OFF_LENGTH);
	diag[1].off = tle9104_get_register_bits(diag_off, TLE9104_DIAGOFF_DIAGCH2OFF_POS,
						TLE9104_DIAGOFF_DIAGCH2OFF_LENGTH);
	diag[2].off = tle9104_get_register_bits(diag_off, TLE9104_DIAGOFF_DIAGCH3OFF_POS,
						TLE9104_DIAGOFF_DIAGCH3OFF_LENGTH);
	diag[3].off = tle9104_get_register_bits(diag_off, TLE9104_DIAGOFF_DIAGCH4OFF_POS,
						TLE9104_DIAGOFF_DIAGCH4OFF_LENGTH);

	return 0;
}

int tle9104_get_diagnostics(const struct device *dev,
			    struct gpio_tle9104_channel_diagnostics diag[TLE9104_GPIO_COUNT])
{
	struct tle9104_data *data = dev->data;
	int result;

	k_mutex_lock(&data->lock, K_FOREVER);
	result = tle9104_get_diagnostics_internal(dev, diag);
	k_mutex_unlock(&data->lock);

	return result;
}

static int tle9104_clear_diagnostics_internal(const struct device *dev)
{
	enum tle9104_register read_reg;
	uint8_t temp;
	int result;

	result = tle9104_transceive_frame(dev, true, TLE9104REGISTER_DIAGOUT12ON, 0x00, &read_reg,
					  &temp);
	if (result != 0) {
		return result;
	}

	result = tle9104_transceive_frame(dev, true, TLE9104REGISTER_DIAGOUT34ON, 0x00, &read_reg,
					  &temp);
	if (result != 0) {
		return result;
	}

	result = tle9104_transceive_frame(dev, true, TLE9104REGISTER_DIAGOFF, 0x00, &read_reg,
					  &temp);
	if (result != 0) {
		return result;
	}

	return 0;
}

int tle9104_clear_diagnostics(const struct device *dev)
{
	struct tle9104_data *data = dev->data;
	int result;

	k_mutex_lock(&data->lock, K_FOREVER);
	result = tle9104_clear_diagnostics_internal(dev);
	k_mutex_unlock(&data->lock);

	return result;
}

static int tle9104_init(const struct device *dev)
{
	const struct tle9104_config *config = dev->config;
	struct tle9104_data *data = dev->data;
	uint8_t register_cfg;
	uint8_t register_globalstatus;
	uint8_t register_icvid;
	enum tle9104_register read_reg;
	int result;

	LOG_DBG("initialize TLE9104 instance %s", dev->name);

	data->cwd_ignore = true;

	result = k_mutex_init(&data->lock);
	if (result != 0) {
		LOG_ERR("unable to initialize mutex");
		return result;
	}

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	register_cfg = 0x00;

	for (int i = 0; i < TLE9104_GPIO_COUNT; ++i) {
		const struct gpio_dt_spec *current = config->gpio_control + i;

		if (current->port == NULL) {
			LOG_DBG("got no control port for output %i, will control it via SPI", i);
			continue;
		}

		register_cfg |= TLE9104_CFG_OUT1DD_BIT << i;

		if (!gpio_is_ready_dt(current)) {
			LOG_ERR("%s: control GPIO is not ready", dev->name);
			return -ENODEV;
		}

		result = gpio_pin_configure_dt(current, GPIO_OUTPUT_INACTIVE);
		if (result != 0) {
			LOG_ERR("failed to initialize control GPIO %i", i);
			return result;
		}
	}

	if (config->gpio_enable.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_enable)) {
			LOG_ERR("%s: enable GPIO is not ready", dev->name);
			return -ENODEV;
		}

		result = gpio_pin_configure_dt(&config->gpio_enable, GPIO_OUTPUT_ACTIVE);
		if (result != 0) {
			LOG_ERR("failed to enable TLE9104");
			return result;
		}
	}

	if (config->gpio_reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_reset)) {
			LOG_ERR("%s: reset GPIO is not yet ready", dev->name);
			return -ENODEV;
		}

		result = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (result != 0) {
			LOG_ERR("failed to initialize GPIO for reset");
			return result;
		}

		k_busy_wait(TLE9104_RESET_DURATION_TIME_US);
		gpio_pin_set_dt(&config->gpio_reset, 0);
		k_busy_wait(TLE9104_RESET_DURATION_WAIT_TIME_US +
			    TLE9104_RESET_DURATION_WAIT_TIME_SAFETY_MARGIN_US);
	}

	/*
	 * The first read value should be the ICVID, this acts also as the setup of the
	 * global status register address.
	 */
	result = tle9104_transceive_frame(dev, false, TLE9104REGISTER_GLOBALSTATUS, 0x00, &read_reg,
					  &register_icvid);
	if (result != 0) {
		return result;
	}

	if (read_reg != TLE9104REGISTER_ICVID) {
		LOG_ERR("expected to read register ICVID, got instead 0x%02X", read_reg);
		return -EIO;
	}

	if (register_icvid != TLE9104_ICVERSIONID) {
		LOG_ERR("got unexpected IC version id 0x%02X", register_icvid);
		return -EIO;
	}

	result = tle9104_transceive_frame(dev, false, TLE9104REGISTER_GLOBALSTATUS, 0x00, &read_reg,
					  &register_globalstatus);
	if (result != 0) {
		return result;
	}

	if (read_reg != TLE9104REGISTER_GLOBALSTATUS) {
		LOG_ERR("expected to read register GLOBALSTATUS, got instead 0x%02X", read_reg);
		return -EIO;
	}

	if ((register_globalstatus & TLE9104_GLOBALSTATUS_POR_LATCH_BIT) == 0) {
		LOG_ERR("no power on reset detected");
		return -EIO;
	}

	/* disable communication watchdog */
	tle9104_set_register_bits(&register_cfg, TLE9104_CFG_CWDTIME_POS,
				  TLE9104_CFG_CWDTIME_LENGTH, 0);

	result = tle9104_write_register(dev, TLE9104REGISTER_CFG, register_cfg);
	if (result != 0) {
		LOG_ERR("unable to write configuration");
		return result;
	}

	register_cfg = 0x00;
	tle9104_set_register_bits(&register_cfg, TLE9104_OFFDIAGCFG_DIAGFILTCFG_POS,
				  TLE9104_OFFDIAGCFG_DIAGFILTCFG_LENGTH,
				  config->diagnostic_filter_time);
	register_cfg |= TLE9104_OFFDIAGCFG_OUT4DIAGEN_BIT;
	register_cfg |= TLE9104_OFFDIAGCFG_OUT3DIAGEN_BIT;
	register_cfg |= TLE9104_OFFDIAGCFG_OUT2DIAGEN_BIT;
	register_cfg |= TLE9104_OFFDIAGCFG_OUT1DIAGEN_BIT;

	result = tle9104_write_register(dev, TLE9104REGISTER_OFFDIAGCFG, register_cfg);
	if (result != 0) {
		LOG_ERR("unable to write OFF-diag configuration");
		return result;
	}

	register_cfg = 0x00;
	tle9104_set_register_bits(&register_cfg, TLE9104_ONDIAGCFG_OCFILTCFG_POS,
				  TLE9104_ONDIAGCFG_OCFILTCFG_LENGTH,
				  config->overcurrent_shutdown_delay_time);
	tle9104_set_register_bits(&register_cfg, TLE9104_ONDIAGCFG_OCTH_POS,
				  TLE9104_ONDIAGCFG_OCTH_LENGTH,
				  config->overcurrent_shutdown_threshold);

	result = tle9104_write_register(dev, TLE9104REGISTER_ONDIAGCFG, register_cfg);
	if (result != 0) {
		LOG_ERR("unable to write ON-diag configuration");
		return result;
	}

	register_globalstatus = 0x00;
	/* enable outputs */
	register_globalstatus |= TLE9104_GLOBALSTATUS_OUTEN_BIT;

	result = tle9104_write_register(dev, TLE9104REGISTER_GLOBALSTATUS, register_globalstatus);
	if (result != 0) {
		LOG_ERR("unable to write global status");
		return result;
	}

	data->cwd_ignore = false;

	return 0;
}

#define TLE9104_INIT_GPIO_FIELDS(inst, gpio)                                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, gpio),                                             \
		    (GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), gpio, 0)), ({0}))

#define TLE9104_INIT(inst)                                                                         \
	static const struct tle9104_config tle9104_##inst##_config = {                             \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst, SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),            \
		.gpio_enable = TLE9104_INIT_GPIO_FIELDS(inst, en_gpios),                           \
		.gpio_reset = TLE9104_INIT_GPIO_FIELDS(inst, resn_gpios),                          \
		.gpio_control = {                                                                  \
			TLE9104_INIT_GPIO_FIELDS(inst, in1_gpios),                                 \
			TLE9104_INIT_GPIO_FIELDS(inst, in2_gpios),                                 \
			TLE9104_INIT_GPIO_FIELDS(inst, in3_gpios),                                 \
			TLE9104_INIT_GPIO_FIELDS(inst, in4_gpios),                                 \
		},                                                                                 \
		.diagnostic_filter_time = DT_INST_ENUM_IDX(inst, diagnostic_filter_time),          \
		.overcurrent_shutdown_delay_time =                                                 \
			DT_INST_ENUM_IDX(inst, overcurrent_shutdown_delay_time),                   \
		.overcurrent_shutdown_threshold =                                                  \
			DT_INST_ENUM_IDX(inst, overcurrent_shutdown_threshold),                    \
	};                                                                                         \
                                                                                                   \
	static struct tle9104_data tle9104_##inst##_data;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, tle9104_init, NULL, &tle9104_##inst##_data,                    \
			      &tle9104_##inst##_config, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TLE9104_INIT);
