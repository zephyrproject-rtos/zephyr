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

LOG_MODULE_REGISTER(gpio_max14916);

#include <zephyr/drivers/gpio/gpio_utils.h>

#include "gpio_max14916.h"
#include "gpio_max149x6.h"

#define DT_DRV_COMPAT adi_max14916_gpio

static int gpio_max14916_diag_chan_get(const struct device *dev);

static int max14916_pars_spi_diag(const struct device *dev, uint8_t *rx_diag_buff, uint8_t rw)
{
	struct max14916_data *data = dev->data;
	int ret = 0;

	if (rx_diag_buff[0]) {
		LOG_ERR("[DIAG] MAX14916 in SPI diag - error detected");

		data->glob.interrupt.reg_bits.SHT_VDD_FLT = MAX149X6_GET_BIT(rx_diag_buff[0], 5);
		data->glob.interrupt.reg_bits.OW_ON_FLT = MAX149X6_GET_BIT(rx_diag_buff[0], 4);
		data->glob.interrupt.reg_bits.OW_OFF_FLT = MAX149X6_GET_BIT(rx_diag_buff[0], 3);
		data->glob.interrupt.reg_bits.CURR_LIM = MAX149X6_GET_BIT(rx_diag_buff[0], 2);
		data->glob.interrupt.reg_bits.OVER_LD_FLT = MAX149X6_GET_BIT(rx_diag_buff[0], 1);

		if (MAX149X6_GET_BIT(rx_diag_buff[0], 0)) {
			LOG_ERR("[DIAG] MAX14916 in SPI diag - GLOBAL FAULT detected");
		}

		ret = -EIO;

		PRINT_ERR(data->glob.interrupt.reg_bits.SHT_VDD_FLT);
		PRINT_ERR(data->glob.interrupt.reg_bits.OW_ON_FLT);
		PRINT_ERR(data->glob.interrupt.reg_bits.OW_OFF_FLT);
		PRINT_ERR(data->glob.interrupt.reg_bits.CURR_LIM);
		PRINT_ERR(data->glob.interrupt.reg_bits.OVER_LD_FLT);
	}

	if (rw == MAX149x6_WRITE && (rx_diag_buff[1] & 0x0f)) {
		/* +-----------------------------------------------------------------------+
		 * | LSB                             BYTE 2                            MSB |
		 * +--------+--------+--------+--------+--------+--------+--------+--------+
		 * |   BIT0 |   BIT1 |   BIT2 |   BIT3 |   BIT4 |   BIT5 |   BIT6 |   BIT7 |
		 * +--------+--------+--------+--------+--------+--------+--------+--------+
		 * | Fault1 | Fault2 | Fault3 | Fault4 | Fault5 | Fault6 | Fault7 | Fault8 |
		 * +--------+--------+--------+--------+--------+--------+--------+--------+
		 */

		LOG_ERR("[DIAG] Flt1[%x] Flt2[%x] Flt3[%x]"
			"Flt4[%x] Flt5[%x] Flt6[%x] Flt7[%x] Flt8[%x]\n",
			MAX149X6_GET_BIT(rx_diag_buff[1], 0), MAX149X6_GET_BIT(rx_diag_buff[1], 1),
			MAX149X6_GET_BIT(rx_diag_buff[1], 2), MAX149X6_GET_BIT(rx_diag_buff[1], 3),
			MAX149X6_GET_BIT(rx_diag_buff[1], 4), MAX149X6_GET_BIT(rx_diag_buff[1], 5),
			MAX149X6_GET_BIT(rx_diag_buff[1], 6), MAX149X6_GET_BIT(rx_diag_buff[1], 7));

		if (rx_diag_buff[1]) {
			LOG_ERR("[DIAG] gpio_max14916_diag_chan_get(%x)\n", rx_diag_buff[1] & 0x0f);
			ret = gpio_max14916_diag_chan_get(dev);
		}
	}

	return ret;
}

static int max14916_reg_trans_spi_diag(const struct device *dev, uint8_t addr, uint8_t tx,
				       uint8_t rw)
{
	const struct max14916_config *config = dev->config;
	uint8_t rx_diag_buff[2];

	if (!gpio_pin_get_dt(&config->fault_gpio)) {
		LOG_ERR(" >>> FLT PIN");
	}

	uint8_t ret = max149x6_reg_transceive(dev, addr, tx, rx_diag_buff, rw);

	if (max14916_pars_spi_diag(dev, rx_diag_buff, rw)) {
		ret = -EIO;
	}

	return ret;
}

#define MAX14916_REG_READ(dev, addr) max14916_reg_trans_spi_diag(dev, addr, 0, MAX149x6_READ)
#define MAX14916_REG_WRITE(dev, addr, val)                                                         \
	max14916_reg_trans_spi_diag(dev, addr, val, MAX149x6_WRITE)

static int gpio_max14916_diag_chan_get(const struct device *dev)
{
	const struct max14916_config *config = dev->config;
	struct max14916_data *data = dev->data;
	int ret = 0;

	if (!gpio_pin_get_dt(&config->fault_gpio)) {
		LOG_ERR("FLT flag is rised");
		ret = -EIO;
	}

	data->glob.interrupt.reg_raw =
		max149x6_reg_transceive(dev, MAX14916_INT_REG, 0, NULL, MAX149x6_READ);

	if (data->glob.interrupt.reg_raw) {
		if (data->glob.interrupt.reg_bits.OVER_LD_FLT) {
			data->chan.ovr_ld = max149x6_reg_transceive(dev, MAX14916_OVR_LD_REG, 0,
								    NULL, MAX149x6_READ);
		}
		if (data->glob.interrupt.reg_bits.CURR_LIM) {
			data->chan.curr_lim = max149x6_reg_transceive(dev, MAX14916_CURR_LIM_REG, 0,
								      NULL, MAX149x6_READ);
		}
		if (data->glob.interrupt.reg_bits.OW_OFF_FLT) {
			data->chan.ow_off = max149x6_reg_transceive(dev, MAX14916_OW_OFF_FLT_REG, 0,
								    NULL, MAX149x6_READ);
		}
		if (data->glob.interrupt.reg_bits.OW_ON_FLT) {
			data->chan.ow_on = max149x6_reg_transceive(dev, MAX14916_OW_ON_FLT_REG, 0,
								   NULL, MAX149x6_READ);
		}
		if (data->glob.interrupt.reg_bits.SHT_VDD_FLT) {
			data->chan.sht_vdd = max149x6_reg_transceive(dev, MAX14916_SHT_VDD_FLT_REG,
								     0, NULL, MAX149x6_READ);
		}

		if (data->glob.interrupt.reg_bits.SUPPLY_ERR) {
			data->glob.glob_err.reg_raw = max149x6_reg_transceive(
				dev, MAX14916_GLOB_ERR_REG, 0, NULL, MAX149x6_READ);
			PRINT_ERR(data->glob.glob_err.reg_bits.VINT_UV);
			PRINT_ERR(data->glob.glob_err.reg_bits.VA_UVLO);
			PRINT_ERR(data->glob.glob_err.reg_bits.VDD_BAD);
			PRINT_ERR(data->glob.glob_err.reg_bits.VDD_WARN);
			PRINT_ERR(data->glob.glob_err.reg_bits.VDD_UVLO);
			PRINT_ERR(data->glob.glob_err.reg_bits.THRMSHUTD);
			PRINT_ERR(data->glob.glob_err.reg_bits.SYNC_ERR);
			PRINT_ERR(data->glob.glob_err.reg_bits.WDOG_ERR);
		}

		if (data->glob.interrupt.reg_bits.COM_ERR) {
			LOG_ERR("MAX14916 Communication Error");
		}
		ret = -EIO;
	}

	return ret;
}

static int gpio_max14916_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t reg_val = 0;

	ret = MAX14916_REG_READ(dev, MAX14916_SETOUT_REG);
	reg_val = ret | pins;

	return MAX14916_REG_WRITE(dev, MAX14916_SETOUT_REG, reg_val);
}

static int gpio_max14916_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t reg_val = 0;

	ret = MAX14916_REG_READ(dev, MAX14916_SETOUT_REG);
	reg_val = ret & ~pins;

	return MAX14916_REG_WRITE(dev, MAX14916_SETOUT_REG, reg_val);
}

static int gpio_max14916_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
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
	case GPIO_OUTPUT:
		break;
	case GPIO_INPUT:
	default:
		LOG_ERR("NOT SUPPORTED OPTION!");
		return -ENOTSUP;
	}

	return err;
}

static int gpio_max14916_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	*value = MAX14916_REG_READ(dev, MAX14916_SETOUT_REG);

	return 0;
}

static int gpio_max14916_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t reg_val = 0;

	ret = MAX14916_REG_READ(dev, MAX14916_SETOUT_REG);

	reg_val = ret;
	reg_val ^= pins;

	MAX14916_REG_WRITE(dev, MAX14916_SETOUT_REG, reg_val);

	return 0;
}

static int gpio_max14916_clean_on_power(const struct device *dev)
{
	int ret;

	/* Clear the latched faults generated at power up */
	ret = MAX14916_REG_READ(dev, MAX14916_OW_OFF_FLT_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14916_OW_OFF_FLT_REG");
		goto err_clean_on_power_max14916;
	}

	ret = MAX14916_REG_READ(dev, MAX14916_OVR_LD_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14916_OVR_LD_REG");
		goto err_clean_on_power_max14916;
	}

	ret = MAX14916_REG_READ(dev, MAX14916_SHT_VDD_FLT_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14916_SHD_VDD_FLT_REG");
		goto err_clean_on_power_max14916;
	}

	ret = MAX14916_REG_READ(dev, MAX14916_GLOB_ERR_REG);
	if (ret < 0) {
		LOG_ERR("Error reading MAX14916_GLOBAL_FLT_REG");
		goto err_clean_on_power_max14916;
	}

err_clean_on_power_max14916:
	return ret;
}

static int gpio_max14916_config_diag(const struct device *dev)
{
	const struct max14916_config *config = dev->config;
	struct max14916_data *data = dev->data;

	MAX14916_REG_WRITE(dev, MAX14916_CONFIG1_REG, config->config1.reg_raw);
	MAX14916_REG_WRITE(dev, MAX14916_CONFIG2_REG, config->config2.reg_raw);
	MAX14916_REG_WRITE(dev, MAX14916_OW_OFF_EN_REG, data->chan_en.ow_on_en);
	MAX14916_REG_WRITE(dev, MAX14916_OW_OFF_EN_REG, data->chan_en.ow_off_en);
	MAX14916_REG_WRITE(dev, MAX14916_SHT_VDD_EN_REG, data->chan_en.sht_vdd_en);
	return 0;
}

static int gpio_max14916_init(const struct device *dev)
{
	const struct max14916_config *config = dev->config;
	int err = 0;

	LOG_DBG(" --- GPIO MAX14916 init IN ---");

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

	/* setup FLT gpio - normal high */
	if (!gpio_is_ready_dt(&config->fault_gpio)) {
		LOG_ERR("FLT GPIO device not ready");
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

	LOG_ERR("[GPIO] FALUT - %d\n", gpio_pin_get_dt(&config->fault_gpio));
	LOG_ERR("[GPIO] READY - %d\n", gpio_pin_get_dt(&config->ready_gpio));
	LOG_ERR("[GPIO] SYNC  - %d\n", gpio_pin_get_dt(&config->sync_gpio));
	LOG_ERR("[GPIO] EN    - %d\n", gpio_pin_get_dt(&config->en_gpio));

	int ret = gpio_max14916_clean_on_power(dev);

	MAX14916_REG_WRITE(dev, MAX14916_SETOUT_REG, 0);

	gpio_max14916_config_diag(dev);

	LOG_DBG(" --- GPIO MAX14916 init OUT ---");

	return ret;
}

static DEVICE_API(gpio, gpio_max14916_api) = {
	.pin_configure = gpio_max14916_config,
	.port_get_raw = gpio_max14916_port_get_raw,
	.port_set_bits_raw = gpio_max14916_port_set_bits_raw,
	.port_clear_bits_raw = gpio_max14916_port_clear_bits_raw,
	.port_toggle_bits = gpio_max14916_port_toggle_bits,
};

#define GPIO_MAX14906_DEVICE(id)                                                                   \
	static const struct max14916_config max14916_##id##_cfg = {                                \
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
	static struct max14916_data max14916_##id##_data;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &gpio_max14916_init, NULL, &max14916_##id##_data,                \
			      &max14916_##id##_cfg, POST_KERNEL,                                   \
			      CONFIG_GPIO_MAX14916_INIT_PRIORITY, &gpio_max14916_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX14906_DEVICE)
