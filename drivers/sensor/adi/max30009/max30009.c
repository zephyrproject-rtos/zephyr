/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <string.h>
#include <zephyr/init.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/max30009.h>
#include <zephyr/sys/util.h>
#include <zephyr/rtio/rtio.h>
#include "max30009.h"

#define DT_DRV_COMPAT adi_max30009

LOG_MODULE_REGISTER(MAX30009, CONFIG_SENSOR_LOG_LEVEL);

/* SPI command byte appended after the register address (see datasheet
 * "Single-Word SPI Register Read and Write Transactions"): the first byte
 * clocked out is the register address, the second selects the direction.
 */

#ifdef MAX30009_BUS_I2C
static int max30009_bus_is_ready_i2c(const union max30009_bus *bus)
{
	if (!i2c_is_ready_dt(&bus->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(bus->i2c.bus);
		return -ENODEV;
	}

	return 0;
}

static int max30009_reg_access_i2c(const struct device *dev, bool read, uint8_t reg_addr,
				   uint8_t *data)
{
	const struct max30009_dev_config *cfg = dev->config;

	if (read) {
		return i2c_reg_read_byte_dt(&cfg->bus.i2c, reg_addr, data);
	}

	return i2c_reg_write_byte_dt(&cfg->bus.i2c, reg_addr, *data);
}
#endif /* MAX30009_BUS_I2C */

#ifdef MAX30009_BUS_SPI
static int max30009_bus_is_ready_spi(const union max30009_bus *bus)
{
	if (!spi_is_ready_dt(&bus->spi)) {
		LOG_ERR_DEVICE_NOT_READY(bus->spi.bus);
		return -ENODEV;
	}

	return 0;
}

static int max30009_reg_access_spi(const struct device *dev, bool read, uint8_t reg_addr,
				   uint8_t *data)
{
	const struct max30009_dev_config *cfg = dev->config;
	int ret;

	/* Command header: register address followed by the R/W command byte. */
	uint8_t cmd_buf[2] = {
		reg_addr,
		read ? MAX30009_SPI_READ_CMD : MAX30009_SPI_WRITE_CMD,
	};

	const struct spi_buf tx_bufs[] = {
		{
			.buf = cmd_buf,
			.len = sizeof(cmd_buf),
		},
		{
			.buf = read ? NULL : data,
			.len = 1,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = sizeof(cmd_buf),
		},
		{
			.buf = read ? data : NULL,
			.len = 1,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	ret = spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI %s failed: %d", read ? "read" : "write", ret);
		return ret;
	}

	return 0;
}
#endif /* MAX30009_BUS_SPI */

int max30009_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *data)
{
	const struct max30009_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, true, reg_addr, data);
}

int max30009_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t data)
{
	const struct max30009_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, false, reg_addr, &data);
}

int max30009_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value)
{
	int ret;
	uint8_t reg_val;

	ret = max30009_reg_read(dev, reg_addr, &reg_val);
	if (ret != 0) {
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, value);

	return max30009_reg_write(dev, reg_addr, reg_val);
}

static int max30009_bus_is_ready(const struct device *dev)
{
	const struct max30009_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

static int max30009_validate_part_id(const struct device *dev)
{
	uint8_t part_id;
	int ret;

	ret = max30009_reg_read(dev, MAX30009_PART_ID, &part_id);
	if (ret != 0) {
		LOG_ERR("Failed to read part ID: %d", ret);
		return ret;
	}

	if (part_id != MAX30009_PART_ID_VAL) {
		LOG_ERR("Unexpected part ID: 0x%02x (expected 0x%02x)", part_id,
			MAX30009_PART_ID_VAL);
		return -ENODEV;
	}

	LOG_INF("MAX30009 part ID 0x%02x confirmed", part_id);

	return 0;
}

static int max30009_soft_reset(const struct device *dev)
{
	int ret;

	/* Set BioZ_BG_EN */
	ret = max30009_reg_update(dev, MAX30009_BIOZ_CONFIG1, MAX30009_BIOZ_CONFIG1_BIOZ_BG_EN_MSK,
				  1);
	if (ret != 0) {
		LOG_ERR("Failed to set BIOZ_BG_EN: %d", ret);
		return ret;
	}

	/* Set SHDN = 0 */
	ret = max30009_reg_update(dev, MAX30009_SYSTEM_CONFIG_1, MAX30009_SYSTEM_CONFIG1_SHDN_MSK,
				  0);
	if (ret != 0) {
		LOG_ERR("Failed to clear SHDN: %d", ret);
		return ret;
	}

	/* Set REF_CLK_SEL = 0 */
	ret = max30009_reg_update(dev, MAX30009_PLL_CONFIG4, MAX30009_PLL_CONFIG4_REF_CLK_SEL_MSK,
				  0);
	if (ret != 0) {
		LOG_ERR("Failed to clear REF_CLK_SEL: %d", ret);
		return ret;
	}

	/* Set PLL_EN = 0 */
	ret = max30009_reg_update(dev, MAX30009_PLL_CONFIG1, MAX30009_PLL_CONFIG1_PLL_EN_MSK, 0);
	if (ret != 0) {
		LOG_ERR("Failed to clear PLL_EN: %d", ret);
		return ret;
	}

	k_sleep(K_MSEC(1));

	/* Set RESET = 1 */

	ret = max30009_reg_update(dev, MAX30009_SYSTEM_CONFIG_1, MAX30009_SYSTEM_CONFIG1_RESET_MSK,
				  1);
	if (ret != 0) {
		LOG_ERR("Failed to write soft reset: %d", ret);
		return ret;
	}

	LOG_DBG("MAX30009 soft reset completed");
	return 0;
}

/**
 * @brief Enable or disable the PLL.
 *
 * When enabling, sets PLL_EN and polls STATUS1 until the PLL reports both
 * frequency and phase lock, or the timeout expires. When disabling, simply
 * clears PLL_EN. The STATUS1 lock bits are clear-on-read, so the poll must not
 * race with other STATUS1 readers.
 *
 * @param dev Device pointer
 * @param enable true to enable the PLL and wait for lock, false to disable it.
 * @return 0 on success, -ETIMEDOUT if lock is not achieved, or a negative
 *         error code on bus failure.
 */
static int max30009_pll_set_enable(const struct device *dev, bool enable)
{
	uint8_t status1;
	int ret;

	ret = max30009_reg_update(dev, MAX30009_PLL_CONFIG1, MAX30009_PLL_CONFIG1_PLL_EN_MSK,
				  enable ? 1 : 0);
	if (ret != 0) {
		LOG_ERR("Failed to %s PLL_EN: %d", enable ? "set" : "clear", ret);
		return ret;
	}

	if (!enable) {
		return 0;
	}

	for (int i = 0; i < MAX30009_PLL_LOCK_RETRIES; i++) {
		ret = max30009_reg_read(dev, MAX30009_STATUS1, &status1);
		if (ret != 0) {
			LOG_ERR("Failed to read STATUS1: %d", ret);
			return ret;
		}

		if ((status1 & MAX30009_STATUS1_FREQ_LOCK_MSK) &&
		    !(status1 & MAX30009_STATUS1_FREQ_UNLOCK_MSK)) {
			LOG_DBG("MAX30009 PLL locked");
			return 0;
		}

		k_sleep(K_MSEC(1));
	}

	LOG_ERR("MAX30009 PLL failed to lock");
	return -ETIMEDOUT;
}

/**
 * @brief Validate and Write the PLL multiplier and clock dividers to the device.
 *
 * @param dev Device pointer
 * @return return int 0 on success, negative error code on failure
 */
static int max30009_config_pll_multiplier(const struct device *dev, int *pll_clk)
{
	const struct max30009_dev_config *cfg = dev->config;
	struct max30009_data *data = dev->data;
	uint16_t pll_multiplier = cfg->clk_cfg.pll_multiplier;
	uint16_t mdiv = pll_multiplier - 1;
	int ret = 0;

	if (cfg->clk_cfg.clk_freq_sel == 1) {
		if (pll_multiplier < MAX30009_PLL_MULT_MIN_32K768 ||
		    pll_multiplier > MAX30009_PLL_MULT_MAX_32K768) {
			LOG_ERR("Invalid PLL multiplier for 32.768 kHz clock: %d", pll_multiplier);
			return -EINVAL;
		}
	} else {
		if (pll_multiplier < MAX30009_PLL_MULT_MIN_32K ||
		    pll_multiplier > MAX30009_PLL_MULT_MAX_32K) {
			LOG_ERR("Invalid PLL multiplier for 32.0 kHz clock: %d", pll_multiplier);
			return -EINVAL;
		}
	}
	*pll_clk = cfg->clk_cfg.clk_freq_sel ? MAX30009_REF_CLK_32K768_HZ * pll_multiplier
					     : MAX30009_REF_CLK_32K_HZ * pll_multiplier;
	data->pll_clk = *pll_clk;

	/* Write MDIV (pll_multiplier - 1) config */
	ret = max30009_reg_update(dev, MAX30009_PLL_CONFIG1, MAX30009_PLL_CONFIG1_MDIV_MSB_MSK,
				  FIELD_GET(MAX30009_MDIV_MSB_MASK, mdiv));
	if (ret != 0) {
		LOG_ERR("Failed to write MDIV MSB: %d", ret);
		return ret;
	}

	ret = max30009_reg_write(dev, MAX30009_PLL_CONFIG2,
				 FIELD_GET(MAX30009_MDIV_LSB_MASK, mdiv));
	if (ret != 0) {
		LOG_ERR("Failed to write MDIV LSB: %d", ret);
		return ret;
	}

	return 0;
}

static int max30009_config_bioz_adc_clk(const struct device *dev, int pll_clk)
{
	int ret;
	struct max30009_data *data = dev->data;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t bioz_adc_clk_div = cfg->clk_cfg.bioz_adc_clk_div;
	int bioz_adc_clk;
	/* Check if BIOZ ADC Clock Divider is valid */
	bioz_adc_clk = pll_clk / (MAX30009_BIOZ_ADC_CLK_DIV_BASE << bioz_adc_clk_div);
	if (bioz_adc_clk < MAX30009_BIOZ_ADC_CLK_MIN_HZ ||
	    bioz_adc_clk > MAX30009_BIOZ_ADC_CLK_MAX_HZ) {
		LOG_ERR("Invalid BioZ ADC Clock: %d Hz", bioz_adc_clk);
		return -EINVAL;
	}
	data->bioz_adc_clk = bioz_adc_clk;
	/* Write BioZ ADC Clock Divider */
	ret = max30009_reg_update(dev, MAX30009_PLL_CONFIG1, MAX30009_PLL_CONFIG1_NDIV_MSK,
				  bioz_adc_clk_div);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ ADC Clock Divider: %d", ret);
		return ret;
	}

	return 0;
}

static int max30009_config_bioz_synth_clk(const struct device *dev, int pll_clk)
{
	int ret;
	struct max30009_data *data = dev->data;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t bioz_synth_clk_div = cfg->clk_cfg.bioz_synth_clk_div;
	int bioz_synth_clk;
	/* Check if BIOZ Synth Clock Divider is valid */
	bioz_synth_clk = pll_clk / (1 << bioz_synth_clk_div);
	if (bioz_synth_clk < MAX30009_BIOZ_SYNTH_CLK_MIN_HZ ||
	    bioz_synth_clk > MAX30009_BIOZ_SYNTH_CLK_MAX_HZ) {
		LOG_ERR("Invalid BioZ Synth Clock: %d Hz", bioz_synth_clk);
		return -EINVAL;
	}
	data->bioz_synth_clk = bioz_synth_clk;
	/* Write BioZ Synth Clock Divider */
	ret = max30009_reg_update(dev, MAX30009_PLL_CONFIG1, MAX30009_PLL_CONFIG1_KDIV_MSK,
				  bioz_synth_clk_div);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Synth Clock Divider: %d", ret);
		return ret;
	}

	return 0;
}

static int max30009_set_bioz_config1(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	/*
	 * I/Q channel enables are deliberately left off here. Per the datasheet
	 * ("Enabling and Disabling the PLL"), BIOZ_I_EN/BIOZ_Q_EN must be set
	 * only after the PLL is enabled and locked; they are applied later by
	 * max30009_bioz_channel_enable(). BG_EN is set before the PLL as required.
	 */
	uint8_t reg_val =
		FIELD_PREP(MAX30009_BIOZ_CONFIG1_BIOZ_DAC_OSR_MSK,
			   cfg->bioz_cfg.cfg_1.bioz_dac_osr) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG1_BIOZ_ADC_OSR_MSK,
			   cfg->bioz_cfg.cfg_1.bioz_adc_osr) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG1_BIOZ_BG_EN_MSK, cfg->bioz_cfg.cfg_1.bioz_bg_en);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_CONFIG1, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 1: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_bioz_channel_enable(const struct device *dev)
{
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t mask = MAX30009_BIOZ_CONFIG1_BIOZ_Q_EN_MSK | MAX30009_BIOZ_CONFIG1_BIOZ_I_EN_MSK;
	uint8_t value =
		FIELD_PREP(MAX30009_BIOZ_CONFIG1_BIOZ_Q_EN_MSK, cfg->bioz_cfg.cfg_1.bioz_q_en) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG1_BIOZ_I_EN_MSK, cfg->bioz_cfg.cfg_1.bioz_i_en);
	int ret;

	ret = max30009_reg_update(dev, MAX30009_BIOZ_CONFIG1, mask, value);
	if (ret != 0) {
		LOG_ERR("Failed to enable BioZ I/Q channels: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_set_bioz_config2(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val =
		FIELD_PREP(MAX30009_BIOZ_CONFIG2_BIOZ_DHPF_MSK, cfg->bioz_cfg.cfg_2.bioz_dhpf) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG2_BIOZ_DLPF_MSK, cfg->bioz_cfg.cfg_2.bioz_dlpf) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG2_BIOZ_CMP_MSK, cfg->bioz_cfg.cfg_2.bioz_cmp) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG2_EN_BIOZ_THRESH_MSK,
			   cfg->bioz_cfg.cfg_2.en_bioz_thresh);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_CONFIG2, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 2: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_set_bioz_config3(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = FIELD_PREP(MAX30009_BIOZ_CONFIG3_BIOZ_EXT_RES_MSK,
				     cfg->bioz_cfg.cfg_3.bioz_ext_res) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG3_BIOZ_LOFF_RAPID_MSK,
				     cfg->bioz_cfg.cfg_3.loff_rapid) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG3_BIOZ_VDRV_MAG_MSK,
				     cfg->bioz_cfg.cfg_3.bioz_vdrv_mag) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG3_BIOZ_IDRV_RGE_MSK,
				     cfg->bioz_cfg.cfg_3.bioz_idrv_rge) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG3_BIOZ_DRV_MODE_MSK,
				     cfg->bioz_cfg.cfg_3.bioz_drv_mode);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_CONFIG3, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 3: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_set_bioz_config4(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = FIELD_PREP(MAX30009_BIOZ_CONFIG4_BIOZ_FAST_MANUAL_MSK,
				     cfg->bioz_cfg.cfg_4.bioz_fast_manual) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG4_BIOZ_FAST_START_EN_MSK,
				     cfg->bioz_cfg.cfg_4.bioz_fast_start_en);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_CONFIG4, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 4: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_set_bioz_config5(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val =
		FIELD_PREP(MAX30009_BIOZ_CONFIG5_BIOZ_AHPF_MSK, cfg->bioz_cfg.cfg_5.bioz_ahpf) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG5_BIOZ_INA_MODE_MSK,
			   cfg->bioz_cfg.cfg_5.bioz_ina_mode) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG5_BIOZ_DM_DIS_MSK, cfg->bioz_cfg.cfg_5.bioz_dm_dis) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG5_BIOZ_GAIN_MSK, cfg->bioz_cfg.cfg_5.bioz_gain);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_CONFIG5, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 5: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_set_bioz_config6(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val =
		FIELD_PREP(MAX30009_BIOZ_CONFIG6_BIOZ_EXT_CAP_MSK,
			   cfg->bioz_cfg.cfg_6.bioz_ext_cap) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG6_BIOZ_DC_RESTORE_MSK,
			   cfg->bioz_cfg.cfg_6.bioz_dc_restore) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG6_BIOZ_AMP_RGE_MSK,
			   cfg->bioz_cfg.cfg_6.bioz_amp_rge) |
		FIELD_PREP(MAX30009_BIOZ_CONFIG6_BIOZ_AMP_BW_MSK, cfg->bioz_cfg.cfg_6.bioz_amp_bw);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_CONFIG6, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 6: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_set_bioz_config7(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = FIELD_PREP(MAX30009_BIOZ_CONFIG7_BIOZ_STBY_ON_MSK,
				     cfg->bioz_cfg.cfg_7.bioz_stby_on) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG7_BIOZ_Q_CLK_PHASE_MSK,
				     cfg->bioz_cfg.cfg_7.bioz_q_clk_phase) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG7_BIOZ_I_CLK_PHASE_MSK,
				     cfg->bioz_cfg.cfg_7.bioz_i_clk_phase) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG7_BIOZ_INA_CHOP_EN_MSK,
				     cfg->bioz_cfg.cfg_7.bioz_ina_chop_en) |
			  FIELD_PREP(MAX30009_BIOZ_CONFIG7_BIOZ_CH_F_SEL_MSK,
				     cfg->bioz_cfg.cfg_7.bioz_ch_f_sel);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_CONFIG7, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 7: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_config_bioz_mux1(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = FIELD_PREP(MAX30009_BIOZ_MUX_CONFIGURATION1_BMUX_RSEL_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_1.bmux_rsel) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIGURATION1_BMUX_BIST_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_1.bmux_bist_en) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIGURATION1_CONNECT_CAL_ONLY_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_1.connect_cal_only) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIGURATION1_MUX_EN_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_1.mux_en) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIGURATION1_CAL_EN_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_1.cal_en);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_MUX_CONFIGURATION1, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ MUX Configuration 1: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_config_bioz_mux2(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG2_BMUX_GSR_RSEL_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_2.bmux_gsr_rsel) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG2_GSR_LOAD_EN_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_2.gsr_load_en) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG2_EN_EXT_IN_LOAD_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_2.en_ext_in_load) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG2_EN_INT_IN_LOAD_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_2.en_int_in_load);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_MUX_CONFIGURATION2, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ MUX Configuration 2: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_config_bioz_mux3(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG3_BIP_ASSIGN_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_3.bip_assign) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG3_BIN_ASSIGN_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_3.bin_assign) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG3_DRVP_ASSIGN_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_3.drvp_assign) |
			  FIELD_PREP(MAX30009_BIOZ_MUX_CONFIG3_DRVN_ASSIGN_MSK,
				     cfg->bioz_mux_cfg.mux_cfg_3.drvn_assign);
	ret = max30009_reg_write(dev, MAX30009_BIOZ_MUX_CONFIGURATION3, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ MUX Configuration 3: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_config_bioz_config(const struct device *dev)
{
	int ret;

	/* Write BioZ Configuration 1 */
	ret = max30009_set_bioz_config1(dev);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 1: %d", ret);
		return ret;
	}

	/* Write BioZ Configuration 2 */
	ret = max30009_set_bioz_config2(dev);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 2: %d", ret);
		return ret;
	}

	/* Write BioZ Configuration 3 */
	ret = max30009_set_bioz_config3(dev);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 3: %d", ret);
		return ret;
	}

	ret = max30009_set_bioz_config4(dev);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 4: %d", ret);
		return ret;
	}

	ret = max30009_set_bioz_config5(dev);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 5: %d", ret);
		return ret;
	}

	ret = max30009_set_bioz_config6(dev);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 6: %d", ret);
		return ret;
	}

	ret = max30009_set_bioz_config7(dev);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Configuration 7: %d", ret);
		return ret;
	}

	return 0;
}

static int max30009_config_clk(const struct device *dev)
{
	int ret = 0;
	uint8_t reg_val;
	int pll_clk = 0;
	const struct max30009_dev_config *cfg = dev->config;
	/* Set PLL Config 4 for Source Clock */
	reg_val = FIELD_PREP(MAX30009_PLL_CONFIG4_REF_CLK_SEL_MSK, cfg->clk_cfg.ref_clk_source) |
		  FIELD_PREP(MAX30009_PLL_CONFIG4_CLK_FREQ_SEL_MSK, cfg->clk_cfg.clk_freq_sel) |
		  FIELD_PREP(MAX30009_PLL_CONFIG4_CLK_FINE_TUNE_MSK, cfg->clk_cfg.clk_fine_tune);
	ret = max30009_reg_write(dev, MAX30009_PLL_CONFIG4, reg_val);
	if (ret) {
		LOG_ERR("Failed to write PLL Config 4: %d", ret);
		return ret;
	}

	ret = max30009_config_pll_multiplier(dev, &pll_clk);
	if (ret) {
		/* PLL multiplier configuration failed */
		LOG_ERR("PLL multiplier configuration failed: %d", ret);
		return ret;
	}

	ret = max30009_config_bioz_adc_clk(dev, pll_clk);
	if (ret) {
		LOG_ERR("Failed to configure BioZ ADC Clock: %d", ret);
		return ret;
	}

	ret = max30009_config_bioz_synth_clk(dev, pll_clk);
	if (ret) {
		LOG_ERR("Failed to configure BioZ Synth Clock: %d", ret);
		return ret;
	}

	return 0;
}

static int max30009_config_bioz_thresh(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;

	ret = max30009_reg_write(dev, MAX30009_BIOZ_LOW_THRESHOLD, cfg->bioz_low_threshold);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ Low Threshold: %d", ret);
		return ret;
	}

	ret = max30009_reg_write(dev, MAX30009_BIOZ_HIGH_THRESHOLD, cfg->bioz_high_threshold);
	if (ret != 0) {
		LOG_ERR("Failed to write BioZ High Threshold: %d", ret);
		return ret;
	}

	return 0;
}

static int max30009_config_bioz_calibration(const struct device *dev)
{
	int ret = 0;

	ret = max30009_config_bioz_mux1(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure BioZ MUX Configuration 1: %d", ret);
		return ret;
	}
	ret = max30009_config_bioz_mux2(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure BioZ MUX Configuration 2: %d", ret);
		return ret;
	}
	ret = max30009_config_bioz_mux3(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure BioZ MUX Configuration 3: %d", ret);
		return ret;
	}
	return 0;
}

static int max30009_config_dc_leads(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = 0;

	reg_val =
		FIELD_PREP(MAX30009_DC_LEADS_CONFIG_EN_LON_DET_MSK, cfg->dc_leads_cfg.en_lon_det) |
		FIELD_PREP(MAX30009_DC_LEADS_CONFIG_EN_LOFF_DET_MSK,
			   cfg->dc_leads_cfg.en_loff_det) |
		FIELD_PREP(MAX30009_DC_LEADS_CONFIG_EN_EXT_LOFF_MSK,
			   cfg->dc_leads_cfg.en_ext_loff) |
		FIELD_PREP(MAX30009_DC_LEADS_CONFIG_EN_DRV_OOR_MSK, cfg->dc_leads_cfg.en_drv_oor) |
		FIELD_PREP(MAX30009_DC_LEADS_CONFIG_LOFF_IPOL_MSK, cfg->dc_leads_cfg.loff_ipol) |
		FIELD_PREP(MAX30009_DC_LEADS_CONFIG_LOFF_IMAG_MSK, cfg->dc_leads_cfg.loff_imag);
	ret = max30009_reg_write(dev, MAX30009_DC_LEADS_CONFIG, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write DC Leads Configuration: %d", ret);
		return ret;
	}

	reg_val = FIELD_PREP(MAX30009_DC_LEAD_DETECT_THRESH_LOFF_THRESH_MSK,
			     cfg->dc_leads_cfg.loff_thresh);
	ret = max30009_reg_write(dev, MAX30009_DC_LEAD_DETECT_THRESHOLD, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write DC Lead Detect Threshold: %d", ret);
		return ret;
	}

	return 0;
}

static int max30009_lead_bias_config(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = 0;

	reg_val = FIELD_PREP(MAX30009_LEAD_BIAS_CONFIG1_RBIAS_VALUE_MSK,
			     cfg->lead_bias_cfg_1.rbias_value) |
		  FIELD_PREP(MAX30009_LEAD_BIAS_CONFIG1_EN_RBIAS_BIP_MSK,
			     cfg->lead_bias_cfg_1.en_rbias_bip) |
		  FIELD_PREP(MAX30009_LEAD_BIAS_CONFIG1_EN_RBIAS_BIN_MSK,
			     cfg->lead_bias_cfg_1.en_rbias_bin);
	ret = max30009_reg_write(dev, MAX30009_LEAD_BIAS_CONFIG1, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write Lead Bias Configuration: %d", ret);
		return ret;
	}

	return 0;
}
#ifdef CONFIG_MAX30009_STREAM
static int max30009_fifo_config(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t reg_val = 0;

	reg_val = FIELD_PREP(MAX30009_FIFO_CONFIG2_FIFO_STAT_CLR_MSK, 1) |
		  FIELD_PREP(MAX30009_FIFO_CONFIG2_A_FULL_TYPE_MSK, cfg->fifo_cfg.fifo_a_full_type);
	ret = max30009_reg_write(dev, MAX30009_FIFO_CONFIG2, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write FIFO Configuration: %d", ret);
		return ret;
	}
	/* FIFO Configuration 1 register uses inverted logic for the watermark value */
	reg_val = MAX30009_MAX_FIFO_WATERMARK - cfg->fifo_cfg.fifo_watermark;

	ret = max30009_reg_write(dev, MAX30009_FIFO_CONFIG1, reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to write FIFO Configuration 1: %d", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_MAX30009_STREAM */

#ifdef CONFIG_PM_DEVICE
static int max30009_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	uint8_t tmp_mask;

	tmp_mask = MAX30009_BIOZ_CONFIG1_BIOZ_BG_EN_MSK | MAX30009_BIOZ_CONFIG1_BIOZ_Q_EN_MSK |
		   MAX30009_BIOZ_CONFIG1_BIOZ_I_EN_MSK;
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* BG_EN before PLL, I/Q only after PLL lock (datasheet sequence) */
		ret = max30009_reg_update(dev, MAX30009_SYSTEM_CONFIG_1,
					  MAX30009_SYSTEM_CONFIG1_SHDN_MSK, 0);
		if (ret != 0) {
			LOG_ERR("Failed to clear SHDN: %d", ret);
			return ret;
		}

		ret = max30009_reg_update(dev, MAX30009_BIOZ_CONFIG1,
					  MAX30009_BIOZ_CONFIG1_BIOZ_BG_EN_MSK,
					  cfg->bioz_cfg.cfg_1.bioz_bg_en);
		if (ret != 0) {
			LOG_ERR("Failed to restore BioZ bandgap: %d", ret);
			return ret;
		}

		ret = max30009_pll_set_enable(dev, true);
		if (ret != 0) {
			return ret;
		}

		ret = max30009_bioz_channel_enable(dev);
		if (ret != 0) {
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Disable BioZ, disable the PLL, then enter shutdown */
		ret = max30009_reg_update(dev, MAX30009_BIOZ_CONFIG1, tmp_mask, 0);
		if (ret != 0) {
			LOG_ERR("Failed to update BioZ configuration: %d", ret);
			return ret;
		}

		ret = max30009_pll_set_enable(dev, false);
		if (ret != 0) {
			return ret;
		}

		ret = max30009_reg_update(dev, MAX30009_SYSTEM_CONFIG_1,
					  MAX30009_SYSTEM_CONFIG1_SHDN_MSK, 1);
		if (ret != 0) {
			LOG_ERR("Failed to set SHDN: %d", ret);
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int max30009_probe(const struct device *dev)
{
	int ret;

	ret = max30009_config_clk(dev);
	if (ret != 0) {
		LOG_ERR("Clock configuration failed: %d", ret);
		return ret;
	}

	ret = max30009_config_bioz_config(dev);
	if (ret != 0) {
		LOG_ERR("BioZ configuration failed: %d", ret);
		return ret;
	}

	ret = max30009_config_bioz_thresh(dev);
	if (ret != 0) {
		LOG_ERR("BioZ threshold configuration failed: %d", ret);
		return ret;
	}

	ret = max30009_config_bioz_calibration(dev);
	if (ret != 0) {
		LOG_ERR("BioZ calibration configuration failed: %d", ret);
		return ret;
	}

	ret = max30009_config_dc_leads(dev);
	if (ret != 0) {
		LOG_ERR("DC leads configuration failed: %d", ret);
		return ret;
	}

	ret = max30009_lead_bias_config(dev);
	if (ret != 0) {
		LOG_ERR("Lead bias configuration failed: %d", ret);
		return ret;
	}

	LOG_DBG("MAX30009 probe completed successfully");
	return 0;
}

static int max30009_init(const struct device *dev)
{
	int ret;

	ret = max30009_bus_is_ready(dev);
	if (ret != 0) {
		LOG_ERR("Bus not ready");
		return ret;
	}

	/* Validate part ID */
	ret = max30009_validate_part_id(dev);
	if (ret != 0) {
		LOG_ERR("Part ID validation failed");
		return ret;
	}

	/* Perform soft reset */
	ret = max30009_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("Soft reset failed");
		return ret;
	}

	ret = max30009_probe(dev);
	if (ret != 0) {
		LOG_ERR("Probe failed: %d", ret);
		return ret;
	}

#ifdef CONFIG_MAX30009_TRIGGER
	ret = max30009_init_interrupt(dev);
	if (ret != 0) {
		LOG_ERR("Trigger initialization failed: %d", ret);
		return ret;
	}
#ifdef CONFIG_MAX30009_STREAM
	ret = max30009_fifo_config(dev);
	if (ret != 0) {
		LOG_ERR("FIFO configuration failed: %d", ret);
		return ret;
	}
#endif /* CONFIG_MAX30009_STREAM */
#endif /* CONFIG_MAX30009_TRIGGER */

	/* Let the BioZ bandgap settle before enabling the PLL (datasheet) */
	k_sleep(K_MSEC(MAX30009_BG_SETTLE_MS));

	ret = max30009_pll_set_enable(dev, true);
	if (ret != 0) {
		LOG_ERR("Failed to enable PLL: %d", ret);
		return ret;
	}

	/* Enable I/Q channels only after the PLL is locked (datasheet sequence) */
	ret = max30009_bioz_channel_enable(dev);
	if (ret != 0) {
		return ret;
	}
	return 0;
}

/*
 * Runtime attribute support (attr_set / attr_get).
 *
 * Most MAX30009 configuration attributes are the same operation: write (or read
 * back) a masked field in a single register. Rather than a hand-written function
 * per attribute, each one is a row in max30009_attrs[] describing its register,
 * field mask, an optional value transform, and whether the write must be done
 * with the PLL quiesced. attr_set()/attr_get() look up the row and apply it
 * generically. Attributes that need a value transform (the OSR fields, which the
 * user expresses in natural units but the register stores as a log2 code) supply
 * encode/decode callbacks; all others read/write the field value directly.
 */

/* Encode a natural OSR value (base << code) into its register field code. */
static int max30009_osr_encode(int32_t in, uint8_t base, uint8_t max_code, uint8_t *out)
{
	for (uint8_t code = 0; code <= max_code; code++) {
		if (in == (base << code)) {
			*out = code;
			return 0;
		}
	}
	return -EINVAL;
}

static int max30009_adc_osr_encode(int32_t in, uint8_t *out)
{
	return max30009_osr_encode(in, MAX30009_BIOZ_ADC_OSR_BASE, MAX30009_BIOZ_ADC_OSR_MAX_CODE,
				   out);
}

static int max30009_adc_osr_decode(uint8_t code, int32_t *out)
{
	*out = MAX30009_BIOZ_ADC_OSR_BASE << code;
	return 0;
}

static int max30009_dac_osr_encode(int32_t in, uint8_t *out)
{
	return max30009_osr_encode(in, MAX30009_BIOZ_DAC_OSR_BASE, MAX30009_BIOZ_DAC_OSR_MAX_CODE,
				   out);
}

static int max30009_dac_osr_decode(uint8_t code, int32_t *out)
{
	*out = MAX30009_BIOZ_DAC_OSR_BASE << code;
	return 0;
}

/* Applied with a PLL disable/enable cycle (acquisition/drive path reconfig). */
#define MAX30009_ATTR_PLL_OFF BIT(0)

/* mask value for an attribute that owns the whole register byte */
#define MAX30009_ATTR_FULL_BYTE 0xFFU

struct max30009_attr_desc {
	uint16_t attr; /* SENSOR_ATTR_MAX30009_* this row handles */
	uint8_t reg;   /* target register address */
	uint8_t mask;  /* field mask; MAX30009_ATTR_FULL_BYTE = whole byte */
	uint8_t flags; /* MAX30009_ATTR_* behavior flags */
	int (*encode)(int32_t in, uint8_t *out);      /* user value -> field code; NULL = raw */
	int (*decode)(uint8_t reg_val, int32_t *out); /* field code -> user value; NULL = raw */
};

static const struct max30009_attr_desc max30009_attrs[] = {
	{SENSOR_ATTR_MAX30009_DRV_MODE, MAX30009_BIOZ_CONFIG3,
	 MAX30009_BIOZ_CONFIG3_BIOZ_DRV_MODE_MSK, MAX30009_ATTR_PLL_OFF, NULL, NULL},
	{SENSOR_ATTR_MAX30009_IDRV_RGE, MAX30009_BIOZ_CONFIG3,
	 MAX30009_BIOZ_CONFIG3_BIOZ_IDRV_RGE_MSK, MAX30009_ATTR_PLL_OFF, NULL, NULL},
	{SENSOR_ATTR_MAX30009_VDRV_MAG, MAX30009_BIOZ_CONFIG3,
	 MAX30009_BIOZ_CONFIG3_BIOZ_VDRV_MAG_MSK, MAX30009_ATTR_PLL_OFF, NULL, NULL},
	{SENSOR_ATTR_MAX30009_GAIN, MAX30009_BIOZ_CONFIG5, MAX30009_BIOZ_CONFIG5_BIOZ_GAIN_MSK, 0,
	 NULL, NULL},
	{SENSOR_ATTR_MAX30009_AHPF, MAX30009_BIOZ_CONFIG5, MAX30009_BIOZ_CONFIG5_BIOZ_AHPF_MSK, 0,
	 NULL, NULL},
	{SENSOR_ATTR_MAX30009_DHPF, MAX30009_BIOZ_CONFIG2, MAX30009_BIOZ_CONFIG2_BIOZ_DHPF_MSK, 0,
	 NULL, NULL},
	{SENSOR_ATTR_MAX30009_DLPF, MAX30009_BIOZ_CONFIG2, MAX30009_BIOZ_CONFIG2_BIOZ_DLPF_MSK, 0,
	 NULL, NULL},
	{SENSOR_ATTR_MAX30009_AMP_RGE, MAX30009_BIOZ_CONFIG6,
	 MAX30009_BIOZ_CONFIG6_BIOZ_AMP_RGE_MSK, 0, NULL, NULL},
	{SENSOR_ATTR_MAX30009_AMP_BW, MAX30009_BIOZ_CONFIG6, MAX30009_BIOZ_CONFIG6_BIOZ_AMP_BW_MSK,
	 0, NULL, NULL},
	{SENSOR_ATTR_MAX30009_CMP, MAX30009_BIOZ_CONFIG2, MAX30009_BIOZ_CONFIG2_BIOZ_CMP_MSK, 0,
	 NULL, NULL},
	{SENSOR_ATTR_MAX30009_ADC_OSR, MAX30009_BIOZ_CONFIG1,
	 MAX30009_BIOZ_CONFIG1_BIOZ_ADC_OSR_MSK, MAX30009_ATTR_PLL_OFF, max30009_adc_osr_encode,
	 max30009_adc_osr_decode},
	{SENSOR_ATTR_MAX30009_DAC_OSR, MAX30009_BIOZ_CONFIG1,
	 MAX30009_BIOZ_CONFIG1_BIOZ_DAC_OSR_MSK, MAX30009_ATTR_PLL_OFF, max30009_dac_osr_encode,
	 max30009_dac_osr_decode},
	{SENSOR_ATTR_MAX30009_HIGH_THRESHOLD, MAX30009_BIOZ_HIGH_THRESHOLD, MAX30009_ATTR_FULL_BYTE,
	 0, NULL, NULL},
	{SENSOR_ATTR_MAX30009_LOW_THRESHOLD, MAX30009_BIOZ_LOW_THRESHOLD, MAX30009_ATTR_FULL_BYTE,
	 0, NULL, NULL},
	{SENSOR_ATTR_MAX30009_I_EN, MAX30009_BIOZ_CONFIG1, MAX30009_BIOZ_CONFIG1_BIOZ_I_EN_MSK, 0,
	 NULL, NULL},
	{SENSOR_ATTR_MAX30009_Q_EN, MAX30009_BIOZ_CONFIG1, MAX30009_BIOZ_CONFIG1_BIOZ_Q_EN_MSK, 0,
	 NULL, NULL},
	{SENSOR_ATTR_MAX30009_BG_EN, MAX30009_BIOZ_CONFIG1, MAX30009_BIOZ_CONFIG1_BIOZ_BG_EN_MSK, 0,
	 NULL, NULL},
};

static const struct max30009_attr_desc *max30009_attr_lookup(uint16_t attr)
{
	for (size_t i = 0; i < ARRAY_SIZE(max30009_attrs); i++) {
		if (max30009_attrs[i].attr == attr) {
			return &max30009_attrs[i];
		}
	}
	return NULL;
}

static int max30009_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct max30009_attr_desc *desc = max30009_attr_lookup((uint16_t)attr);
	uint8_t reg_val;
	int ret;

	ARG_UNUSED(chan);

	if (val == NULL) {
		return -EINVAL;
	}
	/* Attribute not in the table -> unsupported by this driver. */
	if (desc == NULL) {
		return -ENOTSUP;
	}

	/* Turn the user value into the raw field code. */
	if (desc->encode != NULL) {
		ret = desc->encode(val->val1, &reg_val);
		if (ret != 0) {
			LOG_ERR("Invalid value %d for attr %d", val->val1, attr);
			return ret;
		}
	} else {
		/* Largest value the field can hold once shifted down to bit 0. */
		uint8_t field_max = FIELD_GET(desc->mask, desc->mask);

		if (val->val1 < 0 || val->val1 > field_max) {
			LOG_ERR("Value %d out of range for attr %d", val->val1, attr);
			return -EINVAL;
		}
		reg_val = (uint8_t)val->val1;
	}

	/* Acquisition/drive-path fields must be changed with the PLL stopped. */
	if (desc->flags & MAX30009_ATTR_PLL_OFF) {
		ret = max30009_pll_set_enable(dev, false);
		if (ret != 0) {
			return ret;
		}
	}

	/* Full-byte fields (thresholds) are a plain write; others are masked. */
	if (desc->mask == MAX30009_ATTR_FULL_BYTE) {
		ret = max30009_reg_write(dev, desc->reg, reg_val);
	} else {
		ret = max30009_reg_update(dev, desc->reg, desc->mask, reg_val);
	}
	if (ret != 0) {
		LOG_ERR("Failed to write attr %d: %d", attr, ret);
	}

	/* Re-enable the PLL if it was stopped, even when the write failed. */
	if (desc->flags & MAX30009_ATTR_PLL_OFF) {
		int pll_ret = max30009_pll_set_enable(dev, true);

		if (ret == 0) {
			ret = pll_ret;
		}
	}

	return ret;
}

static int max30009_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct max30009_attr_desc *desc = max30009_attr_lookup((uint16_t)attr);
	uint8_t reg_val;
	int ret;

	ARG_UNUSED(chan);

	if (val == NULL) {
		return -EINVAL;
	}
	if (desc == NULL) {
		return -ENOTSUP;
	}

	ret = max30009_reg_read(dev, desc->reg, &reg_val);
	if (ret != 0) {
		LOG_ERR("Failed to read attr %d: %d", attr, ret);
		return ret;
	}

	if (desc->mask != MAX30009_ATTR_FULL_BYTE) {
		reg_val = FIELD_GET(desc->mask, reg_val);
	}

	if (desc->decode != NULL) {
		ret = desc->decode(reg_val, &val->val1);
		if (ret != 0) {
			return ret;
		}
	} else {
		val->val1 = reg_val;
	}
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, max30009_api_funcs) = {
#ifdef CONFIG_MAX30009_TRIGGER
	.trigger_set = max30009_trigger_set,
#endif /* CONFIG_MAX30009_TRIGGER */
	.attr_set = max30009_attr_set,
	.attr_get = max30009_attr_get,
#ifdef CONFIG_MAX30009_STREAM
	.submit = max30009_submit,
	.get_decoder = max30009_get_decoder,
#endif /* CONFIG_MAX30009_STREAM */
};

#define MAX30009_SPI_CFG (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

#define MAX30009_CONFIG_CLK(inst)                                                                  \
	.clk_cfg = {                                                                               \
		.ref_clk_source = DT_INST_PROP(inst, ref_clk_source),                              \
		.clk_freq_sel =                                                                    \
			(DT_INST_PROP(inst, clk_freq_sel) == MAX30009_REF_CLK_32K768_HZ) ? 1 : 0,  \
		.clk_fine_tune = DT_INST_PROP(inst, clk_fine_tune),                                \
		.pll_multiplier = DT_INST_PROP(inst, pll_multiplier),                              \
		.bioz_adc_clk_div = LOG2(DT_INST_PROP(inst, bioz_adc_clk_div)) - 9,                \
		.bioz_synth_clk_div = LOG2(DT_INST_PROP(inst, bioz_synth_clk_div)),                \
	},

#define MAX30009_CONFIG_FIFO(inst)                                                                 \
	.fifo_cfg = {                                                                              \
		.fifo_watermark = DT_INST_PROP(inst, fifo_watermark),                              \
		.fifo_a_full_type = DT_INST_PROP(inst, fifo_a_full_type),                          \
		.fifo_rollover = DT_INST_PROP(inst, fifo_rollover),                                \
	},

#define MAX30009_CONFIG_BIOZ_THRESH(inst)                                                          \
	.bioz_low_threshold = DT_INST_PROP(inst, bioz_low_threshold),                              \
	.bioz_high_threshold = DT_INST_PROP(inst, bioz_high_threshold),

#define MAX30009_CONFIG_BIOZ(inst)                                                                 \
	.bioz_cfg = {                                                                              \
		.cfg_1 =                                                                           \
			{                                                                          \
				.bioz_dac_osr = LOG2(DT_INST_PROP(inst, bioz_dac_osr)) - 5,        \
				.bioz_adc_osr = LOG2(DT_INST_PROP(inst, bioz_adc_osr)) - 3,        \
				.bioz_bg_en = DT_INST_PROP(inst, bioz_bg_en),                      \
				.bioz_q_en = DT_INST_PROP(inst, bioz_q_en),                        \
				.bioz_i_en = DT_INST_PROP(inst, bioz_i_en),                        \
			},                                                                         \
		.cfg_2 =                                                                           \
			{                                                                          \
				.bioz_dhpf = DT_INST_PROP(inst, bioz_dhpf),                        \
				.bioz_dlpf = DT_INST_PROP(inst, bioz_dlpf),                        \
				.bioz_cmp = DT_INST_PROP(inst, bioz_cmp),                          \
				.en_bioz_thresh = DT_INST_PROP(inst, en_bioz_thresh),              \
			},                                                                         \
		.cfg_3 =                                                                           \
			{                                                                          \
				.bioz_ext_res = DT_INST_PROP(inst, bioz_ext_res),                  \
				.loff_rapid = DT_INST_PROP(inst, loff_rapid),                      \
				.bioz_vdrv_mag = DT_INST_PROP(inst, bioz_vdrv_mag),                \
				.bioz_idrv_rge = DT_INST_PROP(inst, bioz_idrv_rge),                \
				.bioz_drv_mode = DT_INST_PROP(inst, bioz_drv_mode),                \
			},                                                                         \
		.cfg_4 =                                                                           \
			{                                                                          \
				.bioz_fast_manual = DT_INST_PROP(inst, bioz_fast_manual),          \
				.bioz_fast_start_en = DT_INST_PROP(inst, bioz_fast_start_en),      \
			},                                                                         \
		.cfg_5 =                                                                           \
			{                                                                          \
				.bioz_ahpf = DT_INST_PROP(inst, bioz_ahpf),                        \
				.bioz_ina_mode = DT_INST_PROP(inst, bioz_ina_mode),                \
				.bioz_dm_dis = DT_INST_PROP(inst, bioz_dm_dis),                    \
				.bioz_gain = DT_INST_PROP(inst, bioz_gain),                        \
			},                                                                         \
		.cfg_6 =                                                                           \
			{                                                                          \
				.bioz_ext_cap = DT_INST_PROP(inst, bioz_ext_cap),                  \
				.bioz_dc_restore = DT_INST_PROP(inst, bioz_dc_restore),            \
				.bioz_amp_rge = DT_INST_PROP(inst, bioz_amp_rge),                  \
				.bioz_amp_bw = DT_INST_PROP(inst, bioz_amp_bw),                    \
			},                                                                         \
		.cfg_7 =                                                                           \
			{                                                                          \
				.bioz_stby_on = DT_INST_PROP(inst, bioz_stby_on),                  \
				.bioz_q_clk_phase = DT_INST_PROP(inst, bioz_q_clk_phase),          \
				.bioz_i_clk_phase = DT_INST_PROP(inst, bioz_i_clk_phase),          \
				.bioz_ina_chop_en = DT_INST_PROP(inst, bioz_ina_chop_en),          \
				.bioz_ch_f_sel = DT_INST_PROP(inst, bioz_ch_fsel),                 \
			},                                                                         \
	},

#define MAX30009_CONFIG_BIOZ_MUX(inst)                                                             \
	.bioz_mux_cfg = {                                                                          \
		.mux_cfg_1 =                                                                       \
			{                                                                          \
				.bmux_rsel = DT_INST_PROP(inst, bmux_rsel),                        \
				.bmux_bist_en = DT_INST_PROP(inst, bmux_bist_en),                  \
				.connect_cal_only = DT_INST_PROP(inst, connect_cal_only),          \
				.mux_en = DT_INST_PROP(inst, mux_en),                              \
				.cal_en = DT_INST_PROP(inst, cal_en),                              \
			},                                                                         \
		.mux_cfg_2 =                                                                       \
			{                                                                          \
				.bmux_gsr_rsel = DT_INST_PROP(inst, bmux_gsr_rsel),                \
				.gsr_load_en = DT_INST_PROP(inst, gsr_load_en),                    \
				.en_ext_in_load = DT_INST_PROP(inst, en_ext_inload),               \
				.en_int_in_load = DT_INST_PROP(inst, en_int_inload),               \
			},                                                                         \
		.mux_cfg_3 =                                                                       \
			{                                                                          \
				.bip_assign = DT_INST_PROP(inst, bip_assign),                      \
				.bin_assign = DT_INST_PROP(inst, bin_assign),                      \
				.drvp_assign = DT_INST_PROP(inst, drvp_assign),                    \
				.drvn_assign = DT_INST_PROP(inst, drvn_assign),                    \
			},                                                                         \
	},

#define MAX30009_CONFIG_DC_LEADS(inst)                                                             \
	.dc_leads_cfg = {                                                                          \
		.en_lon_det = DT_INST_PROP(inst, en_lon_det),                                      \
		.en_loff_det = DT_INST_PROP(inst, en_loff_det),                                    \
		.en_ext_loff = DT_INST_PROP(inst, en_ext_loff),                                    \
		.en_drv_oor = DT_INST_PROP(inst, en_drv_oor),                                      \
		.loff_ipol = DT_INST_PROP(inst, loff_ipol),                                        \
		.loff_imag = DT_INST_PROP(inst, loff_imag),                                        \
		.loff_thresh = DT_INST_PROP(inst, loff_thresh),                                    \
	},

#define MAX30009_CONFIG_LEAD_BIAS(inst)                                                            \
	.lead_bias_cfg_1 = {                                                                       \
		.rbias_value = DT_INST_PROP(inst, rbias_value),                                    \
		.en_rbias_bip = DT_INST_PROP(inst, en_rbias_bip),                                  \
		.en_rbias_bin = DT_INST_PROP(inst, en_rbias_bin),                                  \
	},

#ifdef CONFIG_MAX30009_TRIGGER
#define MAX30009_CONFIG_TRIGGER(inst) .interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),
#else
#define MAX30009_CONFIG_TRIGGER(inst)
#endif /* CONFIG_MAX30009_TRIGGER */

#define MAX30009_CONFIG_SPI(inst)                                                                  \
	.bus = {.spi = SPI_DT_SPEC_INST_GET(inst, MAX30009_SPI_CFG)},                              \
	.bus_type = MAX30009_BUS_TYPE_SPI, .bus_is_ready = max30009_bus_is_ready_spi,              \
	.reg_access = max30009_reg_access_spi,

#define MAX30009_CONFIG_I2C(inst)                                                                  \
	.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)}, .bus_type = MAX30009_BUS_TYPE_I2C,             \
	.bus_is_ready = max30009_bus_is_ready_i2c, .reg_access = max30009_reg_access_i2c,

#ifdef CONFIG_MAX30009_STREAM
/* clang-format off */
#define MAX30009_RTIO_SPI_DEFINE(inst)						\
	COND_CODE_1(CONFIG_SPI_RTIO,						\
		    (SPI_DT_IODEV_DEFINE(max30009_iodev_##inst,			\
					 DT_DRV_INST(inst),			\
					 MAX30009_SPI_CFG);),			\
		    ())

#define MAX30009_RTIO_I2C_DEFINE(inst)						\
	COND_CODE_1(CONFIG_I2C_RTIO,						\
		    (I2C_DT_IODEV_DEFINE(max30009_iodev_##inst,			\
					 DT_DRV_INST(inst));), ())

#define MAX30009_RTIO_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),					\
		    (MAX30009_RTIO_SPI_DEFINE(inst)), ())			\
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),					\
		    (MAX30009_RTIO_I2C_DEFINE(inst)), ())			\
	RTIO_DEFINE(max30009_rtio_ctx_##inst, 8, 8);
/* clang-format on */
#else
#define MAX30009_RTIO_DEFINE(inst)
#endif /* CONFIG_MAX30009_STREAM */

/* clang-format off */
#define MAX30009_DEFINE(inst)								\
	PM_DEVICE_DT_INST_DEFINE(inst, max30009_pm_action);				\
	MAX30009_RTIO_DEFINE(inst)							\
	static struct max30009_data max30009_data_##inst = {				\
		IF_ENABLED(CONFIG_MAX30009_STREAM,					\
			   (.rtio_ctx = &max30009_rtio_ctx_##inst,			\
			    .iodev = &max30009_iodev_##inst,))				\
	};										\
	static const struct max30009_dev_config max30009_config_##inst = {		\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),					\
			    (MAX30009_CONFIG_SPI(inst)), (MAX30009_CONFIG_I2C(inst)))	\
		MAX30009_CONFIG_CLK(inst)						\
		MAX30009_CONFIG_FIFO(inst)						\
		MAX30009_CONFIG_BIOZ_THRESH(inst)					\
		MAX30009_CONFIG_BIOZ(inst)						\
		MAX30009_CONFIG_BIOZ_MUX(inst)						\
		MAX30009_CONFIG_DC_LEADS(inst)						\
		MAX30009_CONFIG_LEAD_BIAS(inst)						\
		MAX30009_CONFIG_TRIGGER(inst)						\
	};										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, max30009_init, PM_DEVICE_DT_INST_GET(inst),	\
				     &max30009_data_##inst, &max30009_config_##inst,	\
				     POST_KERNEL, CONFIG_MAX30009_INIT_PRIORITY,	\
				     &max30009_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(MAX30009_DEFINE)
/* clang-format on */
