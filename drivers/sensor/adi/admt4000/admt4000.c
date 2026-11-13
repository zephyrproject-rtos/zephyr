/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_admt4000

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#include <zephyr/drivers/sensor/admt4000.h>

LOG_MODULE_REGISTER(admt4000, CONFIG_SENSOR_LOG_LEVEL);

/* ECC control registers for ADMT4000 */
static const uint8_t admt4000_ecc_control_registers[] = {
	ADMT4000_02_REG_GENERAL, ADMT4000_02_REG_DIGIOEN, ADMT4000_02_REG_ANGLECK,
	ADMT4000_02_REG_H1MAG,   ADMT4000_02_REG_H1PH,    ADMT4000_02_REG_H2MAG,
	ADMT4000_02_REG_H2PH,    ADMT4000_02_REG_H3MAG,   ADMT4000_02_REG_H3PH,
	ADMT4000_02_REG_H8MAG,   ADMT4000_02_REG_H8PH,
};

static int admt4000_compute_crc(long reg_addr, uint16_t reg_data, uint8_t excess, bool is_write,
				uint8_t *crc_ret)
{
	int crc[] = {1, 1, 1, 1, 1};
	int poly, xor;
	uint32_t data_in;

	if (reg_addr > ADMT4000_02_REG_ECCDIS) {
		return -EINVAL;
	}

	if (is_write) {
		reg_addr = (reg_addr & ADMT4000_RW_MASK) | ADMT4000_WR_EN;
		data_in = ((reg_addr << 16) | reg_data) << 3;
	} else {
		reg_addr = (reg_addr & ADMT4000_RW_MASK);
		data_in = ((reg_addr << 16) | reg_data) << 3 | excess;
	}

	for (int i = 25; i >= 0; i--) {
		xor = ((data_in >> i) & 0x1) ^ crc[4];
		poly = crc[1] ^ xor;

		crc[4] = crc[3];
		crc[3] = crc[2];
		crc[2] = poly;
		crc[1] = crc[0];
		crc[0] = xor;
	}

	*crc_ret = (uint8_t)(16 * crc[4] + 8 * crc[3] + 4 * crc[2] + 2 * crc[1] + crc[0]);

	return 0;
}

static int admt4000_hamming_calc(uint8_t position, uint8_t code_length, uint8_t *code)
{
	int count = 0;
	int i = position - 1;
	int j;

	while (i < code_length) {
		for (j = i; j < i + position && j < code_length; j++) {
			uint8_t byte = code[j / 8];
			uint8_t bit = FIELD_GET(BIT(j & 0x7), byte);

			if (bit) {
				count++;
			}
		}
		i += (2 * position);
	}

	return (count % 2) ? 1 : 0;
}

static int admt4000_ecc_encode(uint8_t *parity_num, uint8_t *code_length, uint8_t *code,
			       uint8_t *input, uint8_t size_code, uint8_t size_input, uint8_t *ecc)
{
	int i = 0, j = 0, k = 0;
	int eff_pos, value;
	uint8_t position, xtract;

	*ecc = 0;

	if (size_code > 16 || size_input > 16) {
		return -EINVAL;
	}

	/* Compute number of parity bits needed */
	*parity_num = 0;
	while ((size_input * 8) > ((1 << i) - (i + 1))) {
		(*parity_num)++;
		i++;
	}

	*code_length = *parity_num + (size_input * 8);

	/* Fill code array with data and placeholder parity bits */
	for (i = 0; i < *code_length; i++) {
		if (i == ((1 << k) - 1)) {
			code[i / 8] &= ~BIT(i % 8);
			k++;
		} else {
			code[i / 8] &= ~BIT(i % 8);
			xtract = FIELD_GET(BIT(j % 8), input[j / 8]);
			code[i / 8] |= FIELD_PREP(BIT(i % 8), xtract);
			j++;
		}
	}

	/* Calculate parity bits and update ECC */
	for (i = 0; i < *parity_num; i++) {
		position = 1 << i;
		value = admt4000_hamming_calc(position, *code_length, code);
		eff_pos = position - 1;

		code[eff_pos / 8] &= ~BIT(eff_pos % 8);
		code[eff_pos / 8] |= FIELD_PREP(BIT(eff_pos % 8), value);

		*ecc |= (value << i);
	}

	/* Calculate overall parity bit */
	value = 0;

	for (i = 0; i < *code_length; i++) {
		uint8_t bit = FIELD_GET(BIT(i % 8), code[i / 8]);

		if (bit) {
			value++;
		}
	}

	value &= 0x1;
	*ecc |= (value << *parity_num);

	return 0;
}

static int admt4000_read(const struct device *dev, uint8_t reg_addr, uint16_t *reg_data,
			 uint8_t *verif)
{
	const struct admt4000_dev_config *config = dev->config;
	int ret;
	uint8_t tx_buf[4] = {0};
	uint8_t rx_buf[4] = {0};
	uint8_t excess, received_crc, crc_expected;

	if (reg_addr > ADMT4000_02_REG_ECCDIS) {
		return -EINVAL;
	}

	/* Construct read command: bit 7 = 0 for read */
	tx_buf[0] = reg_addr & ADMT4000_RW_MASK;

	const struct spi_buf tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
	const struct spi_buf rx = {.buf = rx_buf, .len = sizeof(rx_buf)};
	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx, .count = 1};

	ret = spi_transceive_dt(&config->spi, &tx_set, &rx_set);
	if (ret < 0) {
		return ret;
	}

	/* Parse received data: rx[1] and rx[2] = 16-bit big-endian data */
	*reg_data = ((uint16_t)rx_buf[1] << 8) | rx_buf[2];

	/* Status flags (upper 3 bits), CRC (lower 5 bits) */
	excess = rx_buf[3] >> 5;
	received_crc = rx_buf[3] & 0x1F;

	/* Compute expected CRC */
	ret = admt4000_compute_crc(reg_addr, *reg_data, excess, false, &crc_expected);
	if (ret < 0) {
		return ret;
	}

	/* Validate CRC */
	if (received_crc != crc_expected) {
		LOG_ERR("CRC mismatch - expected 0x%02X, got 0x%02X", crc_expected, received_crc);
		return -EBADMSG;
	}

	if (verif) {
		*verif = rx_buf[3];
	}

	return 0;
}

static int admt4000_write(const struct device *dev, uint8_t reg_addr, uint16_t reg_data)
{
	const struct admt4000_dev_config *config = dev->config;
	int ret;
	uint8_t buf[4];
	uint8_t verif;

	if (reg_addr > ADMT4000_02_REG_ECCDIS) {
		return -EINVAL;
	}

	ret = admt4000_compute_crc(reg_addr, reg_data, 0, true, &verif);
	if (ret) {
		return ret;
	}

	buf[0] = (reg_addr & ADMT4000_RW_MASK) | ADMT4000_WR_EN;
	buf[1] = FIELD_GET(ADMT4000_HI_BYTE, reg_data);
	buf[2] = FIELD_GET(ADMT4000_LOW_BYTE, reg_data);
	buf[3] = verif;

	const struct spi_buf tx = {.buf = buf, .len = sizeof(buf)};
	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};

	return spi_write_dt(&config->spi, &tx_set);
}

static int admt4000_reg_update(const struct device *dev, uint8_t reg_addr, uint16_t update_mask,
			       uint16_t update_val)
{
	int ret;
	uint16_t temp;
	uint8_t verif;

	ret = admt4000_read(dev, reg_addr, &temp, &verif);
	if (ret) {
		return ret;
	}

	temp = (temp & ~update_mask) | update_val;

	return admt4000_write(dev, reg_addr, temp);
}

static int admt4000_set_page(const struct device *dev, bool is_page_zero)
{
	int ret;
	uint16_t page_val;
	struct admt4000_data *data = dev->data;

	/* Avoid unnecessary SPI write if already on desired page */
	if (data->is_page_zero == is_page_zero) {
		return 0;
	}

	page_val = FIELD_PREP(ADMT4000_PAGE_MASK, is_page_zero ? 0x00 : 0x02);

	ret = admt4000_reg_update(dev, ADMT4000_AGP_REG_CNVPAGE, ADMT4000_PAGE_MASK, page_val);
	if (ret) {
		return ret;
	}

	data->is_page_zero = is_page_zero;

	return 0;
}

static int admt4000_ecc_config(const struct device *dev, bool is_en)
{
	int ret;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	uint16_t value = is_en ? ADMT4000_ECC_EN_COMM : ADMT4000_ECC_DIS_COMM;

	return admt4000_write(dev, ADMT4000_02_REG_ECCDIS, value);
}

static int admt4000_update_ecc(const struct device *dev, uint16_t *ecc_val)
{
	int ret;
	uint8_t for_encode[15] = {0}, ecc[2] = {0};
	uint8_t parity_num, code_len, encoded[16] = {0};
	uint16_t temp;

	/* ECC1 (needs padding) */
	for (int i = 7; i < 11; i++) {
		ret = admt4000_read(dev, admt4000_ecc_control_registers[i], &temp, NULL);
		if (ret) {
			LOG_ERR("ECC1 read failed at index %d: %d", i, ret);
			return ret;
		}

		if (i != 7) {
			sys_put_le16(temp, &for_encode[2 * (i - 8) + 1]);
		} else {
			for_encode[14] = FIELD_GET(ADMT4000_HI_BYTE, temp);
		}
	}

	ret = admt4000_ecc_encode(&parity_num, &code_len, encoded, for_encode, 16, 15, &ecc[0]);
	if (ret) {
		LOG_ERR("ECC1 encoding failed: %d", ret);
		return ret;
	}

	/* ECC0 (no padding) */
	for (int i = 0; i < 8; i++) {
		ret = admt4000_read(dev, admt4000_ecc_control_registers[i], &temp, NULL);
		if (ret) {
			LOG_ERR("ECC0 read failed at index %d: %d", i, ret);
			return ret;
		}

		if (i != 7) {
			sys_put_le16(temp, &for_encode[2 * i]);
		} else {
			for_encode[14] = FIELD_GET(ADMT4000_LOW_BYTE, temp);
		}
	}

	ret = admt4000_ecc_encode(&parity_num, &code_len, encoded, for_encode, 16, 15, &ecc[1]);
	if (ret) {
		LOG_ERR("ECC0 encoding failed: %d", ret);
		return ret;
	}

	/* Format ECC data to write in actual register */
	temp = sys_get_be16(ecc);

	if (ecc_val) {
		*ecc_val = temp;
		LOG_DBG("ECC value updated: 0x%04X", temp);
	} else {
		LOG_DBG("ECC value pointer is NULL, skipping update");
	}

	ret = admt4000_set_page(dev, false);
	if (ret) {
		LOG_ERR("Set page failed: %d", ret);
		return ret;
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		LOG_ERR("ECC config (disable) failed: %d", ret);
		return ret;
	}

	ret = admt4000_write(dev, ADMT4000_02_REG_ECCEDC, temp);
	if (ret) {
		LOG_ERR("ECC write failed: %d", ret);
		return ret;
	}

	ret = admt4000_ecc_config(dev, true);
	if (ret) {
		LOG_ERR("ECC config (enable) failed: %d", ret);
		return ret;
	}

	return ret;
}

static int admt4000_get_page(const struct device *dev, bool *is_page_zero)
{
	int ret;
	uint16_t reg_val;
	uint8_t page;
	struct admt4000_data *data = dev->data;

	if (!is_page_zero) {
		return -EINVAL;
	}

	ret = admt4000_read(dev, ADMT4000_AGP_REG_CNVPAGE, &reg_val, NULL);
	if (ret) {
		return ret;
	}

	page = FIELD_GET(ADMT4000_PAGE_MASK, reg_val);

	if (page == 0x00) {
		*is_page_zero = true;
		data->is_page_zero = true;
	} else if (page == 0x02) {
		*is_page_zero = false;
		data->is_page_zero = false;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int admt4000_set_cnv(const struct device *dev, bool is_rising)
{
	int ret;
	uint16_t cnv_val;
	struct admt4000_data *data = dev->data;

	cnv_val = FIELD_PREP(ADMT4000_CNV_EDGE_MASK,
			     is_rising ? ADMT4000_RISING_EDGE : ADMT4000_FALLING_EDGE);

	ret = admt4000_reg_update(dev, ADMT4000_AGP_REG_CNVPAGE, ADMT4000_CNV_EDGE_MASK, cnv_val);
	if (ret) {
		return ret;
	}

	data->is_rising_edge = is_rising;

	return 0;
}

static int admt4000_toggle_cnv(const struct device *dev)
{
	int ret;

	/* Simulate rising edge */
	ret = admt4000_set_cnv(dev, true);
	if (ret) {
		return ret;
	}

	/* Simulate falling edge */
	ret = admt4000_set_cnv(dev, false);
	if (ret) {
		return ret;
	}

	return 0;
}

static int admt4000_raw_angle_read(const struct device *dev, uint16_t *angle_data)
{
	uint8_t angle_regs[] = {ADMT4000_AGP_REG_ABSANGLE, ADMT4000_AGP_REG_ANGLE};
	int ret;

	for (int i = 0; i < 2; i++) {
		ret = admt4000_read(dev, angle_regs[i], &angle_data[i], NULL);
		if (ret < 0) {
			LOG_ERR("Failed to read register 0x%02X, error %d", angle_regs[i], ret);
			return ret;
		}
	}

	return 0;
}

static int admt4000_get_raw_turns_and_angle(const struct device *dev, uint8_t *turns,
					    uint16_t *angle)
{
	struct admt4000_data *data = dev->data;
	int ret;
	uint16_t raw_angles[2] = {0};

	ret = admt4000_raw_angle_read(dev, raw_angles);
	if (ret) {
		LOG_ERR("Failed to read raw angles: %d", ret);
		return ret;
	}

	uint8_t turn_count = (raw_angles[0] >> 8) & 0xFF;

	data->turns = turn_count;

	/*
	 * Decode the signed quarter-turn count: the 8-bit raw field is a
	 * two's-complement value, so counts above the maximum represent
	 * negative turns.
	 */
	if (turn_count > ADMT4000_QUARTER_TURNS_MAX) {
		data->quarter_turns = (int16_t)turn_count - ADMT4000_QUARTER_TURNS_RES;
	} else {
		data->quarter_turns = (int16_t)turn_count;
	}

	if (turns) {
		*turns = turn_count;
	}

	uint8_t coarse_angle = raw_angles[0] & 0xFF;
	uint16_t fine_angle = raw_angles[1];

	data->angle[0] = coarse_angle;
	data->angle[1] = fine_angle;

	if (angle) {
		*angle = fine_angle;
	}

	return 0;
}

static int admt4000_get_cos(const struct device *dev, int16_t *val)
{
	struct admt4000_data *data = dev->data;
	int ret;
	uint16_t raw_val;
	uint8_t verif;

	if (!data->is_page_zero) {
		ret = admt4000_set_page(dev, true);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_read(dev, ADMT4000_00_REG_COSINE, &raw_val, &verif);
	if (ret) {
		return ret;
	}

	*val = sign_extend(FIELD_GET(ADMT4000_RAW_COSINE_MASK, raw_val), 13);
	data->cos_val = *val;

	return 0;
}

static int admt4000_get_sin(const struct device *dev, int16_t *val)
{
	struct admt4000_data *data = dev->data;
	int ret;
	uint16_t raw_val;
	uint8_t verif;

	if (!data->is_page_zero) {
		ret = admt4000_set_page(dev, true);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_read(dev, ADMT4000_00_REG_SINE, &raw_val, &verif);
	if (ret) {
		return ret;
	}

	*val = sign_extend(FIELD_GET(ADMT4000_RAW_SINE_MASK, raw_val), 13);
	data->sin_val = *val;

	return 0;
}

static int admt4000_clear_all_faults(const struct device *dev)
{
	return admt4000_reg_update(dev, ADMT4000_AGP_REG_FAULT, ADMT4000_ALL_FAULTS, 0);
}

static int admt4000_get_radius(const struct device *dev, uint16_t *radius)
{
	struct admt4000_data *data = dev->data;
	int ret;
	uint16_t raw_temp;
	uint8_t verif;

	if (!data->is_page_zero) {
		ret = admt4000_set_page(dev, true);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_read(dev, ADMT4000_00_REG_RADIUS, &raw_temp, &verif);
	if (ret) {
		return ret;
	}

	*radius = FIELD_GET(ADMT4000_RADIUS_MASK, raw_temp);

	return 0;
}

static int admt4000_get_converted_radius(const struct device *dev, uint32_t *radius)
{
	uint16_t raw_temp;
	int ret;

	ret = admt4000_get_radius(dev, &raw_temp);
	if (ret) {
		return ret;
	}

	*radius = raw_temp * ADMT4000_RADIUS_RES;

	return 0;
}

static int admt4000_get_temp(const struct device *dev, uint16_t *temp, bool is_primary)
{
	int ret;
	uint16_t raw_temp;
	uint8_t verif;
	struct admt4000_data *data = dev->data;

	if (!data->is_page_zero) {
		ret = admt4000_set_page(dev, true);
		if (ret) {
			LOG_ERR("Failed to set page 0: %d", ret);
			return ret;
		}
	}

	uint8_t temp_reg = is_primary ? ADMT4000_00_REG_TMP0 : ADMT4000_00_REG_TMP1;

	ret = admt4000_read(dev, temp_reg, &raw_temp, &verif);
	if (ret) {
		LOG_ERR("admt4000_read temp reg failed: %d", ret);
		return ret;
	}

	*temp = FIELD_GET(ADMT4000_TEMP_MASK, raw_temp);

	LOG_DBG("Read raw temp: 0x%04X, masked temp: %u, verif: 0x%02X", raw_temp, *temp, verif);

	return 0;
}

static int admt4000_get_converted_temp(const struct device *dev, int32_t *temp, bool is_primary)
{
	struct admt4000_data *data = dev->data;
	int32_t raw_temp = (int32_t)data->temp;
	int32_t offset;
	int32_t slope_x100;

	if (data->vdd_variant == ADMT4000_3P3V && !is_primary) {
		offset = 1208;
		slope_x100 = 1361;
	} else if (data->vdd_variant == ADMT4000_3P3V && is_primary) {
		offset = 1150;
		slope_x100 = 1632;
	} else if (data->vdd_variant == ADMT4000_5V && !is_primary) {
		offset = 1238;
		slope_x100 = 1345;
	} else if (data->vdd_variant == ADMT4000_5V && is_primary) {
		offset = 1145;
		slope_x100 = 1627;
	} else {
		return -EINVAL;
	}

	*temp = (int32_t)(((int64_t)(raw_temp - offset) * ADMT4000_SF) / slope_x100);

	return 0;
}

static int admt4000_get_angle_filt(const struct device *dev, bool *is_filtered)
{
	int ret;
	uint16_t temp;
	uint8_t verif;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_read(dev, ADMT4000_02_REG_GENERAL, &temp, &verif);
	if (ret) {
		return ret;
	}

	*is_filtered = FIELD_GET(ADMT4000_ANGL_FILT_MASK, temp);
	data->angle_filter_enabled = *is_filtered;

	return 0;
}

static int admt4000_set_angle_filt(const struct device *dev, bool is_filtered)
{
	int ret;
	uint16_t temp;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		return ret;
	}

	temp = FIELD_PREP(ADMT4000_ANGL_FILT_MASK, (uint8_t)is_filtered);

	ret = admt4000_reg_update(dev, ADMT4000_02_REG_GENERAL, ADMT4000_ANGL_FILT_MASK, temp);
	if (ret) {
		return ret;
	}

	data->angle_filter_enabled = is_filtered;

	return admt4000_update_ecc(dev, NULL);
}

static int admt4000_get_h8_ctrl(const struct device *dev, enum admt4000_harmonic_corr_src *source)
{
	int ret;
	uint16_t temp;
	uint8_t verif;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_read(dev, ADMT4000_02_REG_GENERAL, &temp, &verif);
	if (ret) {
		return ret;
	}

	*source = (enum admt4000_harmonic_corr_src)FIELD_GET(ADMT4000_H8_CTRL_MASK, temp);
	data->h8_corr_src = *source;

	return 0;
}

static int admt4000_set_h8_ctrl(const struct device *dev, enum admt4000_harmonic_corr_src source)
{
	int ret;
	uint16_t temp;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		return ret;
	}

	temp = FIELD_PREP(ADMT4000_H8_CTRL_MASK, (uint8_t)source);

	ret = admt4000_reg_update(dev, ADMT4000_02_REG_GENERAL, ADMT4000_H8_CTRL_MASK, temp);
	if (ret) {
		return ret;
	}

	data->h8_corr_src = source;

	return admt4000_update_ecc(dev, NULL);
}

static int admt4000_get_conv_sync_mode(const struct device *dev, enum admt4000_conv_sync_mode *mode)
{
	int ret;
	uint16_t temp;
	uint8_t verif;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_read(dev, ADMT4000_02_REG_GENERAL, &temp, &verif);
	if (ret) {
		return ret;
	}

	*mode = (enum admt4000_conv_sync_mode)FIELD_GET(ADMT4000_CONV_SYNC_MODE_MASK, temp);
	data->conv_sync_mode = *mode;

	return 0;
}

static int admt4000_set_conv_sync_mode(const struct device *dev, enum admt4000_conv_sync_mode mode)
{
	int ret;
	uint16_t temp;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		return ret;
	}

	temp = FIELD_PREP(ADMT4000_CONV_SYNC_MODE_MASK, mode);

	ret = admt4000_reg_update(dev, ADMT4000_02_REG_GENERAL, ADMT4000_CONV_SYNC_MODE_MASK, temp);
	if (ret) {
		return ret;
	}

	data->conv_sync_mode = mode;

	return admt4000_update_ecc(dev, NULL);
}

static int admt4000_get_conv_mode(const struct device *dev, bool *is_one_shot)
{
	int ret;
	uint16_t temp;
	uint8_t verif;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_read(dev, ADMT4000_02_REG_GENERAL, &temp, &verif);
	if (ret) {
		return ret;
	}

	*is_one_shot = FIELD_GET(ADMT4000_CNV_MODE_MASK, temp);
	data->is_one_shot = *is_one_shot;

	return 0;
}

static int admt4000_set_conv_mode(const struct device *dev, bool is_one_shot)
{
	int ret;
	uint16_t temp;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		return ret;
	}

	temp = FIELD_PREP(ADMT4000_CNV_MODE_MASK, (uint8_t)is_one_shot);

	ret = admt4000_reg_update(dev, ADMT4000_02_REG_GENERAL, ADMT4000_CNV_MODE_MASK, temp);
	if (ret) {
		return ret;
	}

	data->is_one_shot = is_one_shot;

	return admt4000_update_ecc(dev, NULL);
}

static int admt4000_io_en(const struct device *dev, uint8_t gpio, bool is_en)
{
	int ret;
	struct admt4000_data *data = dev->data;

	if (gpio > ADMT4000_MAX_GPIO_INDEX) {
		return -EINVAL;
	}

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		return ret;
	}

	ret = admt4000_reg_update(dev, ADMT4000_02_REG_DIGIOEN, ADMT4000_DIG_IO_EN(gpio),
				  is_en ? ADMT4000_DIG_IO_EN(gpio) : 0);
	if (ret) {
		return ret;
	}

	return admt4000_update_ecc(dev, NULL);
}

static int admt4000_gpio_func(const struct device *dev, uint8_t gpio, bool is_alt_func)
{
	int ret;
	struct admt4000_data *data = dev->data;

	if (gpio > ADMT4000_MAX_GPIO_INDEX) {
		LOG_ERR("Invalid GPIO index: %d", gpio);
		return -EINVAL;
	}

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			LOG_ERR("admt4000_set_page failed: %d", ret);
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		LOG_ERR("admt4000_ecc_config failed: %d", ret);
		return ret;
	}

	ret = admt4000_reg_update(dev, ADMT4000_02_REG_DIGIOEN, ADMT4000_GPIO_FUNC(gpio),
				  is_alt_func ? 0 : ADMT4000_GPIO_FUNC(gpio));
	if (ret) {
		LOG_ERR("admt4000_reg_update failed: %d", ret);
		return ret;
	}

	data->gpios[gpio].is_alt_pin = is_alt_func;
	LOG_DBG("GPIO %d alt function set to: %d", gpio, is_alt_func);

	ret = admt4000_update_ecc(dev, NULL);
	return ret;
}

static int admt4000_get_hmag_config(const struct device *dev, uint8_t hmag, uint16_t *mag)
{
	int ret;
	uint16_t temp;
	uint8_t verif;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	switch (hmag) {
	case 1:
		ret = admt4000_read(dev, ADMT4000_02_REG_H1MAG, &temp, &verif);
		break;
	case 2:
		ret = admt4000_read(dev, ADMT4000_02_REG_H2MAG, &temp, &verif);
		break;
	case 3:
		ret = admt4000_read(dev, ADMT4000_02_REG_H3MAG, &temp, &verif);
		break;
	case 8:
		ret = admt4000_read(dev, ADMT4000_02_REG_H8MAG, &temp, &verif);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		return ret;
	}

	if (hmag < 8) {
		*mag = FIELD_GET(ADMT4000_H_11BIT_MAG_MASK, temp);
	} else {
		*mag = FIELD_GET(ADMT4000_H_8BIT_MAG_MASK, temp);
	}

	return 0;
}

static int admt4000_set_hmag_config(const struct device *dev, uint8_t hmag, uint16_t mag)
{
	int ret;
	uint16_t mask;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		return ret;
	}

	switch (hmag) {
	case 1:
	case 2:
		if (mag > ADMT4000_11BIT_MAX) {
			return -EINVAL;
		}
		mask = ADMT4000_H_11BIT_MAG_MASK;
		break;
	case 3:
	case 8:
		if (mag > ADMT4000_8BIT_MAX) {
			return -EINVAL;
		}
		mask = ADMT4000_H_8BIT_MAG_MASK;
		break;
	default:
		return -EINVAL;
	}

	uint8_t reg = (hmag == 1)   ? ADMT4000_02_REG_H1MAG
		      : (hmag == 2) ? ADMT4000_02_REG_H2MAG
		      : (hmag == 3) ? ADMT4000_02_REG_H3MAG
				    : ADMT4000_02_REG_H8MAG;

	ret = admt4000_reg_update(dev, reg, mask, FIELD_PREP(mask, mag));
	if (ret) {
		return ret;
	}

	return admt4000_update_ecc(dev, NULL);
}

static int admt4000_set_converted_hmag_config(const struct device *dev, uint8_t hmag, uint32_t mag)
{
	uint16_t raw;

	if (mag > (ADMT4000_HMAG_RES * ADMT4000_11BIT_MAX)) {
		return -EINVAL;
	}

	raw = (mag * ADMT4000_CORDIC_SCALER) / ADMT4000_HMAG_RES;

	return admt4000_set_hmag_config(dev, hmag, raw);
}

static int admt4000_get_hphase_config(const struct device *dev, uint8_t hpha, uint16_t *pha)
{
	int ret;
	uint16_t temp;
	uint8_t verif;
	struct admt4000_data *data = dev->data;

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	switch (hpha) {
	case 1:
		ret = admt4000_read(dev, ADMT4000_02_REG_H1PH, &temp, &verif);
		break;
	case 2:
		ret = admt4000_read(dev, ADMT4000_02_REG_H2PH, &temp, &verif);
		break;
	case 3:
		ret = admt4000_read(dev, ADMT4000_02_REG_H3PH, &temp, &verif);
		break;
	case 8:
		ret = admt4000_read(dev, ADMT4000_02_REG_H8PH, &temp, &verif);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		return ret;
	}

	*pha = FIELD_GET(ADMT4000_H_12BIT_PHA_MASK, temp);

	return 0;
}

static int admt4000_set_hphase_config(const struct device *dev, uint8_t hpha, uint16_t pha)
{
	int ret;
	struct admt4000_data *data = dev->data;

	if (pha > ADMT4000_12BIT_MAX) {
		return -EINVAL;
	}

	if (data->is_page_zero) {
		ret = admt4000_set_page(dev, false);
		if (ret) {
			return ret;
		}
	}

	ret = admt4000_ecc_config(dev, false);
	if (ret) {
		return ret;
	}

	uint8_t reg = (hpha == 1)   ? ADMT4000_02_REG_H1PH
		      : (hpha == 2) ? ADMT4000_02_REG_H2PH
		      : (hpha == 3) ? ADMT4000_02_REG_H3PH
		      : (hpha == 8) ? ADMT4000_02_REG_H8PH
				    : 0;

	if (reg == 0) {
		return -EINVAL;
	}

	ret = admt4000_reg_update(dev, reg, ADMT4000_H_12BIT_PHA_MASK,
				  FIELD_PREP(ADMT4000_H_12BIT_PHA_MASK, pha));
	if (ret) {
		return ret;
	}

	return admt4000_update_ecc(dev, NULL);
}

static int admt4000_set_converted_hphase_config(const struct device *dev, uint8_t hpha,
						uint32_t pha)
{
	uint16_t raw;

	raw = (pha * ADMT4000_CORDIC_SCALER) / ADMT4000_HPHA_RES;

	return admt4000_set_hphase_config(dev, hpha, raw);
}

static int admt4000_sdp_pulse_coil_rs(const struct device *dev)
{
	int ret;
	const struct admt4000_dev_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->gpio_coil_rs)) {
		return -ENODEV;
	}

	ret = admt4000_set_cnv(dev, false);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_set_dt(&config->gpio_coil_rs, 1);
	if (ret) {
		return ret;
	}

	k_msleep(5);

	ret = gpio_pin_set_dt(&config->gpio_coil_rs, 0);
	if (ret) {
		return ret;
	}

	/* Restart the acquisition sequence after the magnetic reset. */
	return admt4000_toggle_cnv(dev);
}

int admt4000_sample_fetch_helper(const struct device *dev, enum sensor_channel chan,
				 struct admt4000_data *data)
{
	int ret;

	LOG_DBG("Starting sample fetch");

	ret = admt4000_get_conv_mode(dev, &data->is_one_shot);
	if (ret < 0) {
		LOG_ERR("Failed to get conversion mode: %d", ret);
		return ret;
	}

	LOG_DBG("Conversion mode: %s", data->is_one_shot ? "one-shot" : "continuous");

	if (data->is_one_shot) {
		ret = admt4000_toggle_cnv(dev);
		if (ret < 0) {
			LOG_ERR("Failed to toggle CNV: %d", ret);
			return ret;
		}
		LOG_DBG("CNV toggled");
	}

	ret = admt4000_get_raw_turns_and_angle(dev, &data->turns, &data->angle[1]);
	if (ret < 0) {
		LOG_ERR("Failed to get turns and angle: %d", ret);
		return ret;
	}
	LOG_DBG("Turns: %d, Angle: %d", data->turns, data->angle[1]);

	ret = admt4000_get_temp(dev, &data->temp, true);
	if (ret < 0) {
		LOG_ERR("Failed to get temperature: %d", ret);
		return ret;
	}
	LOG_DBG("Temperature: %d", data->temp);

	/* Pre-compute converted temperature for RTIO decoder use */
	ret = admt4000_get_converted_temp(dev, &data->converted_temp, true);
	if (ret < 0) {
		LOG_ERR("Failed to get converted temperature: %d", ret);
		return ret;
	}

	ret = admt4000_get_cos(dev, &data->cos_val);
	if (ret < 0) {
		LOG_ERR("Failed to get cosine value: %d", ret);
		return ret;
	}
	LOG_DBG("Cosine: %d", data->cos_val);

	ret = admt4000_get_sin(dev, &data->sin_val);
	if (ret < 0) {
		LOG_ERR("Failed to get sine value: %d", ret);
		return ret;
	}
	LOG_DBG("Sine: %d", data->sin_val);

	/* Pre-compute converted radius for channel_get / RTIO decoder use */
	ret = admt4000_get_converted_radius(dev, &data->converted_radius);
	if (ret < 0) {
		LOG_ERR("Failed to get converted radius: %d", ret);
		return ret;
	}
	LOG_DBG("Radius: %u", data->converted_radius);

	LOG_DBG("Sample fetch completed successfully");
	return 0;
}

static int admt4000_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct admt4000_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_ROTATION && (int)chan != SENSOR_CHAN_ADMT4000_ANGLE &&
	    (int)chan != SENSOR_CHAN_ADMT4000_COS && (int)chan != SENSOR_CHAN_ADMT4000_SIN &&
	    (int)chan != SENSOR_CHAN_ADMT4000_RADIUS) {
		return -ENOTSUP;
	}

	return admt4000_sample_fetch_helper(dev, chan, data);
}

static int admt4000_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct admt4000_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_ADMT4000_ANGLE: {
		uint16_t raw = FIELD_GET(ADMT4000_ANGLE_MASK, data->angle[1]);
		uint64_t micro_deg = (uint64_t)raw * 360ULL * 1000000ULL / 4096ULL;

		val->val1 = (int32_t)(micro_deg / 1000000ULL);
		val->val2 = (int32_t)(micro_deg % 1000000ULL);
		return 0;
	}

	case SENSOR_CHAN_ROTATION:
		/*
		 * Report full turns from the signed quarter-turn count. val1
		 * holds the whole turns and val2 carries the fractional turn
		 * (quarter-turn steps) in millionths.
		 */
		val->val1 = data->quarter_turns / 4;
		val->val2 = (data->quarter_turns % 4) * 250000;
		return 0;

	case SENSOR_CHAN_AMBIENT_TEMP: {
		int32_t converted_temp;
		int ret;

		ret = admt4000_get_converted_temp(dev, &converted_temp, true);
		if (ret) {
			LOG_ERR("Failed to get converted temperature: %d", ret);
			return ret;
		}

		val->val1 = converted_temp / 100000;
		val->val2 = (converted_temp % 100000) * 10;

		return 0;
	}

	case SENSOR_CHAN_ADMT4000_COS: {
		/* Normalize raw CORDIC count to [-1, 1) to match the decoder. */
		int64_t micro = (int64_t)data->cos_val * 1000000 / ADMT4000_CORDIC_FULL_SCALE;

		val->val1 = (int32_t)(micro / 1000000);
		val->val2 = (int32_t)(micro % 1000000);
		return 0;
	}

	case SENSOR_CHAN_ADMT4000_SIN: {
		/* Normalize raw CORDIC count to [-1, 1) to match the decoder. */
		int64_t micro = (int64_t)data->sin_val * 1000000 / ADMT4000_CORDIC_FULL_SCALE;

		val->val1 = (int32_t)(micro / 1000000);
		val->val2 = (int32_t)(micro % 1000000);
		return 0;
	}

	case SENSOR_CHAN_ADMT4000_RADIUS:
		/* converted_radius is in 10^-7 mV/V units */
		val->val1 = data->converted_radius / 10000000;
		val->val2 = (data->converted_radius % 10000000) / 10;
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int admt4000_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct admt4000_data *data = dev->data;

	switch ((int)attr) {
	case SENSOR_ATTR_ADMT4000_CONV_MODE:
		data->is_one_shot = val->val1;
		return admt4000_set_conv_mode(dev, val->val1);

	case SENSOR_ATTR_ADMT4000_SYNC_MODE:
		data->conv_sync_mode = val->val1;
		return admt4000_set_conv_sync_mode(dev, val->val1);

	case SENSOR_ATTR_ADMT4000_FILTER_ENABLE:
		data->angle_filter_enabled = val->val1;
		return admt4000_set_angle_filt(dev, val->val1);

	case SENSOR_ATTR_ADMT4000_ECK_TYPE:
		data->eck_type = val->val1;
		return 0;

	case SENSOR_ATTR_ADMT4000_H8_CORR_SRC:
		data->h8_corr_src = val->val1;
		return admt4000_set_h8_ctrl(dev, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_1:
		data->hmc[0] = val->val1;
		return admt4000_set_converted_hmag_config(dev, 1, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_2:
		data->hmc[1] = val->val1;
		return admt4000_set_converted_hmag_config(dev, 2, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_3:
		data->hmc[2] = val->val1;
		return admt4000_set_converted_hmag_config(dev, 3, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_1:
		data->hpc[0] = val->val1;
		return admt4000_set_converted_hphase_config(dev, 1, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_2:
		data->hpc[1] = val->val1;
		return admt4000_set_converted_hphase_config(dev, 2, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_3:
		data->hpc[2] = val->val1;
		return admt4000_set_converted_hphase_config(dev, 3, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_8:
		data->hmc8 = val->val1;
		return admt4000_set_converted_hmag_config(dev, 8, val->val1);

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_8:
		data->hpc8 = val->val1;
		return admt4000_set_converted_hphase_config(dev, 8, val->val1);

	case SENSOR_ATTR_ADMT4000_COIL_RESET:
		return admt4000_sdp_pulse_coil_rs(dev);

	default:
		return -ENOTSUP;
	}
}

static int admt4000_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_ADMT4000_CONV_MODE: {
		bool is_one_shot;

		ret = admt4000_get_conv_mode(dev, &is_one_shot);
		if (ret) {
			return ret;
		}
		val->val1 = is_one_shot;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_SYNC_MODE: {
		enum admt4000_conv_sync_mode mode;

		ret = admt4000_get_conv_sync_mode(dev, &mode);
		if (ret) {
			return ret;
		}
		val->val1 = mode;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_FILTER_ENABLE: {
		bool is_filtered;

		ret = admt4000_get_angle_filt(dev, &is_filtered);
		if (ret) {
			return ret;
		}
		val->val1 = is_filtered;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_ECK_TYPE: {
		struct admt4000_data *data = dev->data;

		val->val1 = data->eck_type;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_H8_CORR_SRC: {
		enum admt4000_harmonic_corr_src source;

		ret = admt4000_get_h8_ctrl(dev, &source);
		if (ret) {
			return ret;
		}
		val->val1 = source;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_1: {
		uint16_t mag;

		ret = admt4000_get_hmag_config(dev, 1, &mag);
		if (ret) {
			return ret;
		}
		val->val1 = mag;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_2: {
		uint16_t mag;

		ret = admt4000_get_hmag_config(dev, 2, &mag);
		if (ret) {
			return ret;
		}
		val->val1 = mag;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_3: {
		uint16_t mag;

		ret = admt4000_get_hmag_config(dev, 3, &mag);
		if (ret) {
			return ret;
		}
		val->val1 = mag;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_1: {
		uint16_t pha;

		ret = admt4000_get_hphase_config(dev, 1, &pha);
		if (ret) {
			return ret;
		}
		val->val1 = pha;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_2: {
		uint16_t pha;

		ret = admt4000_get_hphase_config(dev, 2, &pha);
		if (ret) {
			return ret;
		}
		val->val1 = pha;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_3: {
		uint16_t pha;

		ret = admt4000_get_hphase_config(dev, 3, &pha);
		if (ret) {
			return ret;
		}
		val->val1 = pha;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_MAG_8: {
		uint16_t mag;

		ret = admt4000_get_hmag_config(dev, 8, &mag);
		if (ret) {
			return ret;
		}
		val->val1 = mag;
		val->val2 = 0;
		return 0;
	}

	case SENSOR_ATTR_ADMT4000_HARMONIC_PH_8: {
		uint16_t pha;

		ret = admt4000_get_hphase_config(dev, 8, &pha);
		if (ret) {
			return ret;
		}
		val->val1 = pha;
		val->val2 = 0;
		return 0;
	}

	default:
		return -ENOTSUP;
	}
}

static void admt4000_handle_interrupt(const struct device *dev)
{
	struct admt4000_data *data = dev->data;

	if (data->handler) {
		data->handler(dev, &data->trigger);
	}
}

static void admt4000_gpio_callback(const struct device *port, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct admt4000_data *data = CONTAINER_OF(cb, struct admt4000_data, gpio_cb);

	admt4000_handle_interrupt(data->dev);
}

static int admt4000_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	struct admt4000_data *data = dev->data;
	const struct admt4000_dev_config *config = dev->config;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if (!device_is_ready(config->gpio_acalc.port)) {
		return -ENODEV;
	}

	data->handler = handler;
	data->trigger = *trig;

	return gpio_pin_interrupt_configure_dt(
		&config->gpio_acalc, handler ? GPIO_INT_EDGE_TO_INACTIVE : GPIO_INT_DISABLE);
}

static DEVICE_API(sensor, admt4000_driver_api) = {
	.attr_set = admt4000_attr_set,
	.attr_get = admt4000_attr_get,
	.sample_fetch = admt4000_sample_fetch,
	.channel_get = admt4000_channel_get,
	.trigger_set = admt4000_trigger_set,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = admt4000_submit,
	.get_decoder = admt4000_get_decoder,
#endif
};

static int admt4000_init(const struct device *dev)
{
	const struct admt4000_dev_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	struct admt4000_data *data = dev->data;
	int ret;
	bool conv_mode;

	/* Validate VDD variant */
	if (config->vdd_variant != ADMT4000_3P3V && config->vdd_variant != ADMT4000_5V) {
		LOG_ERR("Invalid VDD variant: %d", config->vdd_variant);
		return -EINVAL;
	}

	data->vdd_variant = config->vdd_variant;

	/* Set page to 1 */
	ret = admt4000_set_page(dev, false);
	if (ret) {
		LOG_ERR("Failed to set initial page: %d", ret);
		return ret;
	}

	/* Configure GPIO_BUSY */
	if (device_is_ready(config->gpio_busy.port)) {
		ret = gpio_pin_configure_dt(&config->gpio_busy, GPIO_INPUT);
		if (ret) {
			LOG_ERR("Failed to configure GPIO BUSY: %d", ret);
			return ret;
		}
		ret = admt4000_gpio_func(dev, 0, true);
		if (ret) {
			return ret;
		}
		ret = admt4000_io_en(dev, 0, true);
		if (ret) {
			return ret;
		}
	}

	/* Configure GPIO_ACALC */
	if (device_is_ready(config->gpio_acalc.port)) {
		ret = gpio_pin_configure_dt(&config->gpio_acalc, GPIO_INPUT);
		if (ret) {
			return ret;
		}

		ret = admt4000_gpio_func(dev, 3, true);
		if (ret) {
			return ret;
		}

		ret = admt4000_io_en(dev, 3, true);
		if (ret) {
			return ret;
		}

		gpio_init_callback(&data->gpio_cb, admt4000_gpio_callback,
				   BIT(config->gpio_acalc.pin));
		ret = gpio_add_callback(config->gpio_acalc.port, &data->gpio_cb);
		if (ret) {
			return ret;
		}
	}

	/* Optional: Configure GPIO_COIL_RS */
	if (device_is_ready(config->gpio_coil_rs.port)) {
		ret = gpio_pin_configure_dt(&config->gpio_coil_rs, GPIO_OUTPUT_LOW);
		if (ret) {
			return ret;
		}
	}

	/* Optional: Configure GPIO_CNV */
	if (device_is_ready(config->gpio_cnv.port)) {
		ret = gpio_pin_configure_dt(&config->gpio_cnv, GPIO_INPUT);
		if (ret) {
			return ret;
		}
	}

	k_msleep(10);

	ret = admt4000_clear_all_faults(dev);
	if (ret) {
		return ret;
	}

	/* Set fixed conversion factor based on VDD */
	data->fixed_conv_factor_mv = (config->vdd_variant == ADMT4000_3P3V) ? 412500 : 300000;
	LOG_DBG("Fixed conversion factor: %d mV", data->fixed_conv_factor_mv);

	ret = admt4000_get_conv_mode(dev, &conv_mode);
	if (ret) {
		LOG_ERR("Failed to get conversion mode: %d", ret);
		return ret;
	}

	data->is_one_shot = conv_mode;

	ret = admt4000_set_page(dev, true);
	if (ret) {
		LOG_ERR("Failed to set page: %d", ret);
		return ret;
	}

	ret = admt4000_get_page(dev, &conv_mode);
	if (ret) {
		LOG_ERR("Failed to get current page: %d", ret);
		return ret;
	}

	data->is_page_zero = conv_mode;

	LOG_INF("ADMT4000 initialized");
	return 0;
}

#define ADMT4000_DEVICE_INIT(inst)                                                                 \
	static struct admt4000_data admt4000_data_##inst;                                          \
	static const struct admt4000_dev_config admt4000_config_##inst = {                         \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB),             \
		.vdd_variant = DT_INST_PROP(inst, adi_vdd_variant),                                \
		.gpio_busy = GPIO_DT_SPEC_INST_GET_OR(inst, busy_gpios, {0}),                      \
		.gpio_acalc = GPIO_DT_SPEC_INST_GET_OR(inst, acalc_gpios, {0}),                    \
		.gpio_coil_rs = GPIO_DT_SPEC_INST_GET_OR(inst, coil_rs_gpios, {0}),                \
		.gpio_cnv = GPIO_DT_SPEC_INST_GET_OR(inst, cnv_gpios, {0}),                        \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, admt4000_init, NULL, &admt4000_data_##inst,             \
				     &admt4000_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &admt4000_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADMT4000_DEVICE_INIT)
