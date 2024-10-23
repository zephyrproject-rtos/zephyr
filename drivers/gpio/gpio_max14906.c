/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_max14906);

#include <zephyr/drivers/gpio/gpio_utils.h>

#include "gpio_max14906.h"
#include "gpio_max149x6.h"

#define DT_DRV_COMPAT adi_max14906_gpio

static int gpio_max14906_diag_chan_get(const struct device *dev);

static int max14906_pars_spi_diag(const struct device *dev, uint8_t *rx_diag_buff, uint8_t rw)
{
	struct max14906_data *data = dev->data;
	int ret = 0;

	if (rx_diag_buff[0]) {
		LOG_ERR("[DIAG] MAX14906 in SPI diag - error detected\n");
		data->glob.interrupt.reg_bits.SHT_VDD_FAULT = MAX149X6_GET_BIT(rx_diag_buff[0], 5);
		data->glob.interrupt.reg_bits.ABOVE_VDD_FAULT =
			MAX149X6_GET_BIT(rx_diag_buff[0], 4);
		data->glob.interrupt.reg_bits.OW_OFF_FAULT = MAX149X6_GET_BIT(rx_diag_buff[0], 3);
		data->glob.interrupt.reg_bits.CURR_LIM = MAX149X6_GET_BIT(rx_diag_buff[0], 2);
		data->glob.interrupt.reg_bits.OVER_LD_FAULT = MAX149X6_GET_BIT(rx_diag_buff[0], 1);

		uint8_t globlf = MAX149X6_GET_BIT(rx_diag_buff[0], 0);

		ret = -EIO;

		PRINT_ERR(data->glob.interrupt.reg_bits.SHT_VDD_FAULT);
		PRINT_ERR(data->glob.interrupt.reg_bits.ABOVE_VDD_FAULT);
		PRINT_ERR(data->glob.interrupt.reg_bits.OW_OFF_FAULT);
		PRINT_ERR(data->glob.interrupt.reg_bits.CURR_LIM);
		PRINT_ERR(data->glob.interrupt.reg_bits.OVER_LD_FAULT);
		PRINT_ERR(globlf);
	}

	if (rw == MAX149x6_WRITE && (rx_diag_buff[1] & 0x0f)) {
		/* +-----------------------------------------------------------------------+
		 * | LSB                             BYTE 2                            MSB |
		 * +--------+--------+--------+--------+--------+--------+--------+--------+
		 * |   BIT0 |   BIT1 |   BIT2 |   BIT3 |   BIT4 |   BIT5 |   BIT6 |   BIT7 |
		 * +--------+--------+--------+--------+--------+--------+--------+--------+
		 * | Fault1 | Fault2 | Fault3 | Fault4 | DiLvl1 | DiLvl2 | DiLvl3 | DiLvl4 |
		 * +--------+--------+--------+--------+--------+--------+--------+--------+
		 */

		LOG_ERR("[DIAG] Flt1[%x] Flt2[%x] Flt3[%x] Flt4[%x]",
			MAX149X6_GET_BIT(rx_diag_buff[1], 0), MAX149X6_GET_BIT(rx_diag_buff[1], 1),
			MAX149X6_GET_BIT(rx_diag_buff[1], 2), MAX149X6_GET_BIT(rx_diag_buff[1], 3));
		if (rx_diag_buff[1] & 0x0f) {
			LOG_ERR("[DIAG] gpio_max14906_diag_chan_get(%x)\n", rx_diag_buff[1] & 0x0f);
			ret = gpio_max14906_diag_chan_get(dev);
		}
	}

	return ret;
}

static int max14906_reg_trans_spi_diag(const struct device *dev, uint8_t addr, uint8_t tx,
				       uint8_t rw)
{
	const struct max14906_config *config = dev->config;
	uint8_t rx_diag_buff[2];

	if (!gpio_pin_get_dt(&config->fault_gpio)) {
		LOG_ERR("[FAULT] pin triggered");
	}

	uint8_t ret = max149x6_reg_transceive(dev, addr, tx, rx_diag_buff, rw);

	if (max14906_pars_spi_diag(dev, rx_diag_buff, rw)) {
		ret = -EIO;
	}

	return ret;
}

#define MAX14906_REG_READ(dev, addr) max14906_reg_trans_spi_diag(dev, addr, 0, MAX149x6_READ)
#define MAX14906_REG_WRITE(dev, addr, val)                                                         \
	max14906_reg_trans_spi_diag(dev, addr, val, MAX149x6_WRITE)

/*
 * @brief Register update function for MAX14906
 *
 * @param dev - MAX149x6 device.
 * @param addr - Register valueto wich data is updated.
 * @param mask - Corresponding mask to the data that will be updated.
 * @param val - Updated value to be written in the register at update.
 * @return 0 in case of success, negative error code otherwise.
 */
static int max14906_reg_update(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	int ret;
	uint32_t reg_val = 0;

	ret = MAX14906_REG_READ(dev, addr);

	reg_val = ret;
	reg_val &= ~mask;
	reg_val |= mask & val;

	return MAX14906_REG_WRITE(dev, addr, reg_val);
}

static int gpio_max14906_diag_chan_get(const struct device *dev)
{
	const struct max14906_config *config = dev->config;
	struct max14906_data *data = dev->data;
	int ret;

	if (!gpio_pin_get_dt(&config->fault_gpio)) {
		LOG_ERR("[DIAG] FAULT flag is rised");
	}

	data->glob.interrupt.reg_raw =
		max149x6_reg_transceive(dev, MAX14906_INT_REG, 0, NULL, MAX149x6_READ);
	if (data->glob.interrupt.reg_raw) {
		if (data->glob.interrupt.reg_bits.OVER_LD_FAULT ||
		    data->glob.interrupt.reg_bits.CURR_LIM) {
			data->chan.ovr_ld.reg_raw = max149x6_reg_transceive(
				dev, MAX14906_OVR_LD_REG, 0, NULL, MAX149x6_READ);
		}
		if (data->glob.interrupt.reg_bits.OW_OFF_FAULT ||
		    data->glob.interrupt.reg_bits.ABOVE_VDD_FAULT) {
			data->chan.opn_wir.reg_raw = max149x6_reg_transceive(
				dev, MAX14906_OPN_WIR_FLT_REG, 0, NULL, MAX149x6_READ);
		}
		if (data->glob.interrupt.reg_bits.SHT_VDD_FAULT) {
			data->chan.sht_vdd.reg_raw = max149x6_reg_transceive(
				dev, MAX14906_SHT_VDD_FLT_REG, 0, NULL, MAX149x6_READ);
		}
		if (data->glob.interrupt.reg_bits.DE_MAG_FAULT) {
			data->chan.doi_level.reg_raw = max149x6_reg_transceive(
				dev, MAX14906_DOILEVEL_REG, 0, NULL, MAX149x6_READ);
			if (data->chan.doi_level.reg_raw) {
				PRINT_ERR(data->chan.doi_level.reg_bits.VDDOK_FAULT1);
				PRINT_ERR(data->chan.doi_level.reg_bits.VDDOK_FAULT2);
				PRINT_ERR(data->chan.doi_level.reg_bits.VDDOK_FAULT3);
				PRINT_ERR(data->chan.doi_level.reg_bits.VDDOK_FAULT4);
				PRINT_ERR(data->chan.doi_level.reg_bits.SAFE_DAMAGE_F1);
				PRINT_ERR(data->chan.doi_level.reg_bits.SAFE_DAMAGE_F2);
				PRINT_ERR(data->chan.doi_level.reg_bits.SAFE_DAMAGE_F3);
				PRINT_ERR(data->chan.doi_level.reg_bits.SAFE_DAMAGE_F4);
			}
		}
		if (data->glob.interrupt.reg_bits.SUPPLY_ERR) {
			data->glob.glob_err.reg_raw = max149x6_reg_transceive(
				dev, MAX14906_GLOB_ERR_REG, 0, NULL, MAX149x6_READ);
			PRINT_ERR(data->glob.glob_err.reg_bits.VINT_UV);
			PRINT_ERR(data->glob.glob_err.reg_bits.V5_UVLO);
			PRINT_ERR(data->glob.glob_err.reg_bits.VDD_LOW);
			PRINT_ERR(data->glob.glob_err.reg_bits.VDD_WARN);
			PRINT_ERR(data->glob.glob_err.reg_bits.VDD_UVLO);
			PRINT_ERR(data->glob.glob_err.reg_bits.THRMSHUTD);
			PRINT_ERR(data->glob.glob_err.reg_bits.LOSSGND);
			PRINT_ERR(data->glob.glob_err.reg_bits.WDOG_ERR);
		}
		if (data->glob.interrupt.reg_bits.COM_ERR) {
			LOG_ERR("[DIAG] MAX14906 Communication Error");
		}
	}

	ret = data->chan.doi_level.reg_raw | data->chan.ovr_ld.reg_raw |
	      data->chan.opn_wir.reg_raw | data->chan.sht_vdd.reg_raw;

	return ret;
}

/**
 * @brief Configure a channel's function.
 * @param desc - device descriptor for the MAX14906
 * @param ch - channel index (0 based).
 * @param function - channel configuration (input, output or high-z).
 * @return 0 in case of success, negative error code otherwise
 */
static int max14906_ch_func(const struct device *dev, uint32_t ch, enum max14906_function function)
{
	uint8_t setout_reg_val;

	switch (function) {
	case MAX14906_HIGH_Z:
		setout_reg_val = MAX14906_IN;
		max14906_reg_update(dev, MAX14906_CONFIG_DO_REG, MAX14906_DO_MASK(ch),
				    FIELD_PREP(MAX14906_DO_MASK(ch), MAX14906_PUSH_PULL));
		break;
	case MAX14906_IN:
		setout_reg_val = MAX14906_IN;
		max14906_reg_update(dev, MAX14906_CONFIG_DO_REG, MAX14906_DO_MASK(ch),
				    FIELD_PREP(MAX14906_DO_MASK(ch), MAX14906_HIGH_SIDE));
		break;
	case MAX14906_OUT:
		setout_reg_val = MAX14906_OUT;
		break;
	default:
		return -EINVAL;
	}

	return max14906_reg_update(dev, MAX14906_SETOUT_REG, MAX14906_CH_DIR_MASK(ch),
				   FIELD_PREP(MAX14906_CH_DIR_MASK(ch), setout_reg_val));
}

static int gpio_max14906_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t reg_val = 0;

	ret = MAX14906_REG_READ(dev, MAX14906_SETOUT_REG);
	reg_val = ret | (pins & 0x0f);

	return MAX14906_REG_WRITE(dev, MAX14906_SETOUT_REG, reg_val);
}

static int gpio_max14906_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t reg_val = 0;

	ret = MAX14906_REG_READ(dev, MAX14906_SETOUT_REG);
	reg_val = ret & (0xf0 & ~pins);

	return MAX14906_REG_WRITE(dev, MAX14906_SETOUT_REG, reg_val);
}

static int gpio_max14906_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	int err = 0;

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INT_ENABLE) {
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		max14906_ch_func(dev, (uint32_t)pin, MAX14906_IN);
		LOG_DBG("SETUP AS INPUT %d", pin);
		break;
	case GPIO_OUTPUT:
		max14906_ch_func(dev, (uint32_t)pin, MAX14906_OUT);
		LOG_DBG("SETUP AS OUTPUT %d", pin);
		break;
	default:
		LOG_ERR("On MAX14906 only input option is available!");
		err = -ENOTSUP;
		break;
	}

	return err;
}

static int gpio_max14906_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	/* We care only for first 4 bits of the reg.
	 * Next set of bits is for direction.
	 * NOTE : in case and only if pin is INPUT DOILEVEL reg bits 0-4 shows PIN state.
	 * In case PIN is OUTPUT same bits show VDDOKFault state.
	 */

	*value = (0x0f & MAX14906_REG_READ(dev, MAX14906_DOILEVEL_REG));

	return 0;
}

static int gpio_max14906_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t reg_val, direction, state, new_reg_val;

	ret = MAX14906_REG_READ(dev, MAX14906_SETOUT_REG);

	direction = ret & 0xf0;
	state = ret & 0x0f;
	reg_val = state ^ pins;
	new_reg_val = direction | (0x0f & state);

	return MAX14906_REG_WRITE(dev, MAX14906_SETOUT_REG, new_reg_val);
}

static int gpio_max14906_clean_on_power(const struct device *dev)
{
	int ret;

	/* Clear the latched faults generated at power up */
	ret = MAX14906_REG_READ(dev, MAX14906_OPN_WIR_FLT_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14906_OPN_WIR_FLT_REG");
		goto err_clean_on_power_max14906;
	}

	ret = MAX14906_REG_READ(dev, MAX14906_OVR_LD_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14906_OVR_LD_REG");
		goto err_clean_on_power_max14906;
	}

	ret = MAX14906_REG_READ(dev, MAX14906_SHT_VDD_FLT_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14906_SHD_VDD_FLT_REG");
		goto err_clean_on_power_max14906;
	}

	ret = MAX14906_REG_READ(dev, MAX14906_GLOB_ERR_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14906_GLOBAL_FLT_REG");
		goto err_clean_on_power_max14906;
	}

err_clean_on_power_max14906:
	return ret;
}

static int gpio_max14906_config_diag(const struct device *dev)
{
	const struct max14906_data *data = dev->data;
	const struct max14906_config *config = dev->config;

	/* Set Config1 and Config2 regs */
	MAX14906_REG_WRITE(dev, MAX14906_CONFIG1_REG, config->config1.reg_raw);
	MAX14906_REG_WRITE(dev, MAX14906_CONFIG2_REG, config->config2.reg_raw);

	/* Configure per channel diagnostics */
	MAX14906_REG_WRITE(dev, MAX14906_OPN_WR_EN_REG, data->chan_en.opn_wr_en.reg_raw);
	MAX14906_REG_WRITE(dev, MAX14906_SHT_VDD_EN_REG, data->chan_en.sht_vdd_en.reg_raw);

	return 0;
}

static int gpio_max14906_init(const struct device *dev)
{
	const struct max14906_config *config = dev->config;
	int err = 0;

	LOG_DBG(" --- GPIO MAX14906 init IN ---");

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready\n");
		return -ENODEV;
	}

	/* setup READY gpio - normal low */
	if (!gpio_is_ready_dt(&config->ready_gpio)) {
		LOG_ERR("READY GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->ready_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure reset GPIO");
		return err;
	}

	/* setup FAULT gpio - normal high */
	if (!gpio_is_ready_dt(&config->fault_gpio)) {
		LOG_ERR("FAULT GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->fault_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure DC GPIO");
		return err;
	}

	/* setup LATCH gpio - normal high */
	if (!gpio_is_ready_dt(&config->sync_gpio)) {
		LOG_ERR("SYNC GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->sync_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure busy GPIO");
		return err;
	}

	/* setup LATCH gpio - normal high */
	if (!gpio_is_ready_dt(&config->en_gpio)) {
		LOG_ERR("SYNC GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->en_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure busy GPIO");
		return err;
	}

	gpio_pin_set_dt(&config->en_gpio, 1);
	gpio_pin_set_dt(&config->sync_gpio, 1);

	LOG_DBG("[GPIO] FAULT - %d\n", gpio_pin_get_dt(&config->fault_gpio));
	LOG_DBG("[GPIO] READY - %d\n", gpio_pin_get_dt(&config->ready_gpio));
	LOG_DBG("[GPIO] SYNC  - %d\n", gpio_pin_get_dt(&config->sync_gpio));
	LOG_DBG("[GPIO] EN    - %d\n", gpio_pin_get_dt(&config->en_gpio));

	int ret = gpio_max14906_clean_on_power(dev);

	MAX14906_REG_WRITE(dev, MAX14906_SETOUT_REG, 0);

	gpio_max14906_config_diag(dev);

	LOG_DBG(" --- GPIO MAX14906 init OUT ---");

	return ret;
}

static const struct gpio_driver_api gpio_max14906_api = {
	.pin_configure = gpio_max14906_config,
	.port_get_raw = gpio_max14906_port_get_raw,
	.port_set_bits_raw = gpio_max14906_port_set_bits_raw,
	.port_clear_bits_raw = gpio_max14906_port_clear_bits_raw,
	.port_toggle_bits = gpio_max14906_port_toggle_bits,
};

#define GPIO_MAX14906_DEVICE(id)                                                                   \
	static const struct max14906_config max14906_##id##_cfg = {                                \
		.spi = SPI_DT_SPEC_INST_GET(id, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),        \
		.ready_gpio = GPIO_DT_SPEC_INST_GET(id, drdy_gpios),                               \
		.fault_gpio = GPIO_DT_SPEC_INST_GET(id, fault_gpios),                              \
		.sync_gpio = GPIO_DT_SPEC_INST_GET(id, sync_gpios),                                \
		.en_gpio = GPIO_DT_SPEC_INST_GET(id, en_gpios),                                    \
		.crc_en = DT_INST_PROP(id, crc_en),                                                \
		.config1.reg_bits.FLED_SET = DT_INST_PROP(id, fled_set),                           \
		.config1.reg_bits.SLED_SET = DT_INST_PROP(id, sled_set),                           \
		.config1.reg_bits.FLED_STRETCH = DT_INST_PROP(id, fled_stretch),                   \
		.config1.reg_bits.FFILTER_EN = DT_INST_PROP(id, ffilter_en),                       \
		.config1.reg_bits.FILTER_LONG = DT_INST_PROP(id, filter_long),                     \
		.config1.reg_bits.FLATCH_EN = DT_INST_PROP(id, flatch_en),                         \
		.config1.reg_bits.LED_CURR_LIM = DT_INST_PROP(id, led_cur_lim),                    \
		.config2.reg_bits.VDD_ON_THR = DT_INST_PROP(id, vdd_on_thr),                       \
		.config2.reg_bits.SYNCH_WD_EN = DT_INST_PROP(id, synch_wd_en),                     \
		.config2.reg_bits.SHT_VDD_THR = DT_INST_PROP(id, sht_vdd_thr),                     \
		.config2.reg_bits.OW_OFF_CS = DT_INST_PROP(id, ow_off_cs),                         \
		.config2.reg_bits.WD_TO = DT_INST_PROP(id, wd_to),                                 \
		.pkt_size = (DT_INST_PROP(id, crc_en) & 0x1) ? 3 : 2,                              \
		.spi_addr = DT_INST_PROP(id, spi_addr),                                            \
	};                                                                                         \
                                                                                                   \
	static struct max14906_data max14906_##id##_data = {                                       \
		.chan_en.opn_wr_en.reg_bits =                                                      \
			{                                                                          \
				.OW_OFF_EN1 = DT_INST_PROP_BY_IDX(id, ow_en, 0),                   \
				.OW_OFF_EN2 = DT_INST_PROP_BY_IDX(id, ow_en, 1),                   \
				.OW_OFF_EN3 = DT_INST_PROP_BY_IDX(id, ow_en, 2),                   \
				.OW_OFF_EN4 = DT_INST_PROP_BY_IDX(id, ow_en, 3),                   \
				.GDRV_EN1 = DT_INST_PROP_BY_IDX(id, gdrv_en, 0),                   \
				.GDRV_EN1 = DT_INST_PROP_BY_IDX(id, gdrv_en, 1),                   \
				.GDRV_EN2 = DT_INST_PROP_BY_IDX(id, gdrv_en, 2),                   \
				.GDRV_EN3 = DT_INST_PROP_BY_IDX(id, gdrv_en, 3),                   \
			},                                                                         \
		.chan_en.sht_vdd_en.reg_bits =                                                     \
			{                                                                          \
				.VDD_OV_EN1 = DT_INST_PROP_BY_IDX(id, vdd_ov_en, 0),               \
				.VDD_OV_EN2 = DT_INST_PROP_BY_IDX(id, vdd_ov_en, 1),               \
				.VDD_OV_EN3 = DT_INST_PROP_BY_IDX(id, vdd_ov_en, 2),               \
				.VDD_OV_EN4 = DT_INST_PROP_BY_IDX(id, vdd_ov_en, 3),               \
				.SH_VDD_EN1 = DT_INST_PROP_BY_IDX(id, sh_vdd_en, 0),               \
				.SH_VDD_EN2 = DT_INST_PROP_BY_IDX(id, sh_vdd_en, 1),               \
				.SH_VDD_EN3 = DT_INST_PROP_BY_IDX(id, sh_vdd_en, 2),               \
				.SH_VDD_EN4 = DT_INST_PROP_BY_IDX(id, sh_vdd_en, 3),               \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &gpio_max14906_init, NULL, &max14906_##id##_data,                \
			      &max14906_##id##_cfg, POST_KERNEL,                                   \
			      CONFIG_GPIO_MAX14906_INIT_PRIORITY, &gpio_max14906_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX14906_DEVICE)
