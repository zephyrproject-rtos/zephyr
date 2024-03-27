/*
 * Copyright (c) 2023 Kickmaker
 * Copyright (c) 2021 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_st25dv

#include <assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/eeprom.h>
#if defined(CONFIG_EEPROM_ST25DV_ENABLE_ADVANCED_FEATURES)
#include <zephyr/drivers/eeprom/eeprom_st25dv.h>
#endif
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_st25dv, CONFIG_EEPROM_LOG_LEVEL);

/* ST25DV registers */

#define ST25DV_REG_GPO         0x0000
#define ST25DV_REG_IT_TIME     0x0001
#define ST25DV_REG_EH_MODE     0x0002
#define ST25DV_REG_RF_MNGT     0x0003
#define ST25DV_REG_RFA1SS      0x0004
#define ST25DV_REG_ENDA1       0x0005
#define ST25DV_REG_RFA2SS      0x0006
#define ST25DV_REG_ENDA2       0x0007
#define ST25DV_REG_RFA3SS      0x0008
#define ST25DV_REG_ENDA3       0x0009
#define ST25DV_REG_RFA4SS      0x000a
#define ST25DV_REG_I2CSS       0x000b
#define ST25DV_REG_LOCK_CC     0x000c
#define ST25DV_REG_MB_MODE     0x000d
#define ST25DV_REG_MB_WDG      0x000e
#define ST25DV_REG_LOCK_CFG    0x000f
#define ST25DV_REG_LOCK_DSF_ID 0x0010
#define ST25DV_REG_LOCK_AFI    0x0011
#define ST25DV_REG_DSFID       0x0012
#define ST25DV_REG_AFI         0x0013
#define ST25DV_REG_MEM_SIZE    0x0014
#define ST25DV_REG_BLK_SIZE    0x0016
#define ST25DV_REG_IC_REF      0x0017
#define ST25DV_REG_UUID        0x0018
#define ST25DV_REG_IC_REV      0x0020
#define ST25DV_REG_I2C_PASSWD  0x0900
#define ST25DV_REG_I2C_SSO_DYN 0x2004
#define ST25DV_REG_MB_CTRL_DYN 0x2006
#define ST25DV_REG_MBLEN_DYN   0x2007
#define ST25DV_REG_MAILBOX_RAM 0x2008

/** @brief Length of the MEM_SIZE field of the ST25DV */
#define ST25DV_MEM_SIZE_LENGTH    2U
/** @brief Length of the BLK_SIZE field of the ST25DV */
#define ST25DV_BLK_SIZE_LENGTH    1U
/** @brief Length of the IC_REV field of the ST25DV */
#define ST25DV_IC_REV_LENGTH      1U
/** @brief Length of ENDA registers */
#define ST25DV_ENDA_LENGTH        1U
/** @brief Length of I2C_PASSWD message */
#define ST25DV_I2C_PASSWD_LENGTH  17U
/** @brief Length of RFAxSS registers */
#define ST25DV_RFASS_LENGTH       1U
/** @brief Length of MB_MODE register */
#define ST25DV_MB_MODE_LENGTH     1U
/** @brief Length of MB_CTRL_DYN register */
#define ST25DV_MB_CTRL_DYN_LENGTH 1U
/** @brief Length of MB_LEN_DYN register */
#define ST25DV_MB_LEN_DYN_LENGTH  1U

/**
 * @brief I2C write timeout (us)
 *
 * @details Max value (us): t_write + t_write * (max write bytes) / (internal page write)
 * 5 + 5*(256/16)
 */
#define EEPROM_ST25DV_WRITE_TIMEOUT_US 85U

/* I2C Security Session Open I2C_SSO_Dyn */
#define EEPROM_ST25DV_I2C_SSO_DYN_I2CSSO_SHIFT 0
#define EEPROM_ST25DV_I2C_SSO_DYN_I2CSSO_FIELD 0xFE
#define EEPROM_ST25DV_I2C_SSO_DYN_I2CSSO_MASK  0x01

/* RF X Access Protection RFAxSS */
#define EEPROM_ST25DV_RFAXSS_PWD_CTRL_SHIFT 0
#define EEPROM_ST25DV_RFAXSS_PWD_CTRL_FIELD 0xFC
#define EEPROM_ST25DV_RFAXSS_PWD_CTRL_MASK  0x03
#define EEPROM_ST25DV_RFAXSS_RW_PROT_SHIFT  2U
#define EEPROM_ST25DV_RFAXSS_RW_PROT_FIELD  0xF3
#define EEPROM_ST25DV_RFAXSS_RW_PROT_MASK   0x0C
#define EEPROM_ST25DV_RFAXSS_ALL_SHIFT      0
#define EEPROM_ST25DV_RFAXSS_ALL_MASK       0x0F

/* I2C Access Protection I2CSS */
#define EEPROM_ST25DV_I2CSS_RW_PROT_A1_SHIFT 0
#define EEPROM_ST25DV_I2CSS_RW_PROT_A1_FIELD 0xFC
#define EEPROM_ST25DV_I2CSS_RW_PROT_A1_MASK  0x03
#define EEPROM_ST25DV_I2CSS_RW_PROT_A2_SHIFT 2U
#define EEPROM_ST25DV_I2CSS_RW_PROT_A2_FIELD 0xF3
#define EEPROM_ST25DV_I2CSS_RW_PROT_A2_MASK  0x0C
#define EEPROM_ST25DV_I2CSS_RW_PROT_A3_SHIFT 4U
#define EEPROM_ST25DV_I2CSS_RW_PROT_A3_FIELD 0xCF
#define EEPROM_ST25DV_I2CSS_RW_PROT_A3_MASK  0x30
#define EEPROM_ST25DV_I2CSS_RW_PROT_A4_SHIFT 6U
#define EEPROM_ST25DV_I2CSS_RW_PROT_A4_FIELD 0x3F
#define EEPROM_ST25DV_I2CSS_RW_PROT_A4_MASK  0xC0

/* Fast transfer mode control and status */
#define EEPROM_ST25DV_MB_CTRL_DYN_MBEN_SHIFT        0u
#define EEPROM_ST25DV_MB_CTRL_DYN_MBEN_FIELD        0xFE
#define EEPROM_ST25DV_MB_CTRL_DYN_MBEN_MASK         0x01
#define EEPROM_ST25DV_MB_CTRL_DYN_HOSTPUTMSG_SHIFT  1u
#define EEPROM_ST25DV_MB_CTRL_DYN_HOSTPUTMSG_FIELD  0xFD
#define EEPROM_ST25DV_MB_CTRL_DYN_HOSTPUTMSG_MASK   0x02
#define EEPROM_ST25DV_MB_CTRL_DYN_RFPUTMSG_SHIFT    2u
#define EEPROM_ST25DV_MB_CTRL_DYN_RFPUTMSG_FIELD    0xFB
#define EEPROM_ST25DV_MB_CTRL_DYN_RFPUTMSG_MASK     0x04
#define EEPROM_ST25DV_MB_CTRL_DYN_STRESERVED_SHIFT  3u
#define EEPROM_ST25DV_MB_CTRL_DYN_STRESERVED_FIELD  0xF7
#define EEPROM_ST25DV_MB_CTRL_DYN_STRESERVED_MASK   0x08
#define EEPROM_ST25DV_MB_CTRL_DYN_HOSTMISSMSG_SHIFT 4u
#define EEPROM_ST25DV_MB_CTRL_DYN_HOSTMISSMSG_FIELD 0xEF
#define EEPROM_ST25DV_MB_CTRL_DYN_HOSTMISSMSG_MASK  0x10
#define EEPROM_ST25DV_MB_CTRL_DYN_RFMISSMSG_SHIFT   5u
#define EEPROM_ST25DV_MB_CTRL_DYN_RFMISSMSG_FIELD   0xDF
#define EEPROM_ST25DV_MB_CTRL_DYN_RFMISSMSG_MASK    0x20
#define EEPROM_ST25DV_MB_CTRL_DYN_CURRENTMSG_SHIFT  6u
#define EEPROM_ST25DV_MB_CTRL_DYN_CURRENTMSG_FIELD  0x3F
#define EEPROM_ST25DV_MB_CTRL_DYN_CURRENTMSG_MASK   0xC0

/** Sleep interval for polling (in ms) */
#define EEPROM_ST25DV_POLL_INTERVAL_MS 10u

/** Limit for I2C transfer if i2c-transfer-is-limited property is set */
#define EEPROM_ST25DV_I2C_LIMIT_MAX 255u

/** Split for I2C transfer too long for a single transfer when exceeding EEPROM_ST25DV_I2C_LIMIT_MAX
 */
#define EEPROM_ST25DV_I2C_LIMIT_SPLIT_TRANSFER 128u

struct eeprom_st25dv_config {
	struct i2c_dt_spec bus;
	uint16_t addr;
	uint8_t power_state;
	size_t size;
	bool i2c_transfer_is_limited;
};

struct eeprom_st25dv_data {
	struct k_mutex lock;
};

static int _st25dv_read(const struct eeprom_st25dv_config *config, uint16_t addr, off_t offset,
			uint8_t *pdata, uint32_t count)
{
	int ret = 0;
	int64_t timeout;
	uint8_t wr_data[2] = {offset >> 8, offset & 0xff};
	/*
	 * A write cycle may be in progress so reads must be attempted
	 * until the current write cycle should be completed.
	 */
	timeout = k_uptime_get() + EEPROM_ST25DV_WRITE_TIMEOUT_US;
	while (1) {
		int64_t now = k_uptime_get();

		ret = i2c_write_read(config->bus.bus, addr, wr_data, 2, pdata, count);

		if (!ret || now > timeout) {
			break;
		}
		k_sleep(K_MSEC(1));
	}
	return ret;
}

static int _st25dv_write(const struct eeprom_st25dv_config *config, uint16_t addr, off_t offset,
			 const uint8_t *pdata, uint32_t count)
{
	int ret = 0;
	int64_t timeout;
	uint8_t buffer[count + 2];

	buffer[0] = offset >> 8;
	buffer[1] = offset;
	memcpy(&buffer[2], pdata, count);

	/*
	 * A write cycle may already be in progress so writes must be
	 * attempted until the previous write cycle should be
	 * completed.
	 */
	timeout = k_uptime_get() + EEPROM_ST25DV_WRITE_TIMEOUT_US;
	while (1) {
		int64_t now = k_uptime_get();

		ret = i2c_write(config->bus.bus, buffer, count + 2, addr);

		if (!ret || now > timeout) {
			break;
		}
		k_sleep(K_MSEC(1));
	}
	return ret;
}

static int _st25dv_write_user(const struct eeprom_st25dv_config *config, off_t offset,
			      const uint8_t *pdata, uint32_t count)
{
	return _st25dv_write(config, config->addr, offset, pdata, count);
}

static int _st25dv_read_user(const struct eeprom_st25dv_config *config, off_t offset,
			     uint8_t *pdata, uint32_t count)
{
	return _st25dv_read(config, config->addr, offset, pdata, count);
}

#if defined(CONFIG_EEPROM_ST25DV_ENABLE_ADVANCED_FEATURES)

static int _st25dv_read_conf(const struct eeprom_st25dv_config *config, off_t offset,
			     uint8_t *pdata, uint32_t count)
{
	return _st25dv_read(config, config->addr | 4, offset, pdata, count);
}

static int _st25dv_write_conf(const struct eeprom_st25dv_config *config, off_t offset,
			      uint8_t *pdata, uint32_t count)
{
	return _st25dv_write(config, config->addr | 4, offset, pdata, count);
}

static int _st25dv_read_dyn_conf(const struct eeprom_st25dv_config *config, off_t offset,
				 uint8_t *pdata, uint32_t count)
{
	return _st25dv_read(config, config->addr, offset, pdata, count);
}

static int _st25dv_write_dyn_conf(const struct eeprom_st25dv_config *config, off_t offset,
				  uint8_t *pdata, uint32_t count)
{
	return _st25dv_write(config, config->addr, offset, pdata, count);
}

#endif /* defined(CONFIG_EEPROM_ST25DV_ENABLE_ADVANCED_FEATURES) */

static int eeprom_st25dv_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	if (!len) {
		return 0;
	}

	if (config->i2c_transfer_is_limited && len > EEPROM_ST25DV_I2C_LIMIT_MAX) {
		LOG_ERR("attempt to read mode bytes than possible");
		return -EINVAL;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_user(config, offset, buf, len);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int eeprom_st25dv_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_write_user(config, offset, buf, len);
	k_mutex_unlock(&data->lock);

	return ret;
}

static size_t eeprom_st25dv_size(const struct device *dev)
{
	const struct eeprom_st25dv_config *config = dev->config;

	return config->size;
}

static int eeprom_st25dv_init(const struct device *dev)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&(config->bus))) {
		LOG_ERR("parent bus device not ready");
		return -EINVAL;
	}
	return 0;
}

#if defined(CONFIG_EEPROM_ST25DV_ENABLE_ADVANCED_FEATURES)

/**
 * @brief  Read ENDA1 register
 * @param[in] config ST25DV config structure
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _get_enda1(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	assert(value != NULL);

	return _st25dv_read_conf(config, ST25DV_REG_ENDA1, value, ST25DV_ENDA_LENGTH);
}

/**
 * @brief  Write ENDA1 register
 * @param[in] config ST25DV config structure
 * @param[in] value data pointer to write to register
 * @return 0 in case of success, an error code otherwise
 */
static int _set_enda1(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	assert(value != NULL);

	return _st25dv_write_conf(config, ST25DV_REG_ENDA1, value, ST25DV_ENDA_LENGTH);
}

/**
 * @brief  Read ENDA2 register
 * @param[in] config ST25DV config structure
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _get_enda2(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	assert(value != NULL);

	return _st25dv_read_conf(config, ST25DV_REG_ENDA2, value, ST25DV_ENDA_LENGTH);
}

/**
 * @brief  Write ENDA2 register
 * @param[in] config ST25DV config structure
 * @param[in] value data pointer to write to register
 * @return 0 in case of success, an error code otherwise
 */
static int _set_enda2(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	assert(value != NULL);

	return _st25dv_write_conf(config, ST25DV_REG_ENDA2, value, ST25DV_ENDA_LENGTH);
}

/**
 * @brief  Read ENDA3 register
 * @param[in] config ST25DV config structure
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _get_enda3(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	assert(value != NULL);

	return _st25dv_read_conf(config, ST25DV_REG_ENDA3, value, ST25DV_ENDA_LENGTH);
}

/**
 * @brief  Write ENDA3 register
 * @param[in] config ST25DV config structure
 * @param[in] value data pointer to write to register
 * @return 0 in case of success, an error code otherwise
 */
static int _set_enda3(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	assert(value != NULL);

	return _st25dv_write_conf(config, ST25DV_REG_ENDA3, value, ST25DV_ENDA_LENGTH);
}

/**
 * @brief  Read I2C_SSO_DYN_I2CSSO register
 * @param[in] config structure containing context driver
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _read_i2c_sso_dyn(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	int32_t ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_I2C_SSO_DYN, (uint8_t *)value, 1);
	if (ret == 0) {
		*value &= (EEPROM_ST25DV_I2C_SSO_DYN_I2CSSO_MASK);
		*value = *value >> (EEPROM_ST25DV_I2C_SSO_DYN_I2CSSO_SHIFT);
	}

	return ret;
}

/**
 * @brief  Read RFA1SS_ALL register
 * @param[in] config structure containing context driver
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _get_rfa1ss(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA1SS, (uint8_t *)value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		*value &= (EEPROM_ST25DV_RFAXSS_ALL_MASK);
		*value = *value >> (EEPROM_ST25DV_RFAXSS_ALL_SHIFT);
	}

	return ret;
}

/**
 * @brief  Write RFA1SS_ALL register
 * @param[in] config structure containing context driver
 * @param[in] value data pointer to write to register
 * @return 0 in case of success, an error code otherwise
 */
static int _set_rfa1ss(const struct eeprom_st25dv_config *config, const uint8_t *const value)
{
	uint8_t reg_value = 0;
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA1SS, &reg_value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		reg_value = ((*value << (EEPROM_ST25DV_RFAXSS_ALL_SHIFT)) &
			     (EEPROM_ST25DV_RFAXSS_ALL_MASK)) |
			    (reg_value & ~(EEPROM_ST25DV_RFAXSS_ALL_MASK));

		ret = _st25dv_write_conf(config, ST25DV_REG_RFA1SS, &reg_value,
					 ST25DV_RFASS_LENGTH);
	}

	return ret;
}

/**
 * @brief  Read RFA2SS_ALL register
 * @param[in] config structure containing context driver
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _get_rfa2ss(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA2SS, (uint8_t *)value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		*value &= (EEPROM_ST25DV_RFAXSS_ALL_MASK);
		*value = *value >> (EEPROM_ST25DV_RFAXSS_ALL_SHIFT);
	}

	return ret;
}

/**
 * @brief  Write RFA2SS_ALL register
 * @param[in] config structure containing context driver
 * @param[in] value data pointer to write to register
 * @return 0 in case of success, an error code otherwise
 */
static int _set_rfa2ss(const struct eeprom_st25dv_config *config, const uint8_t *const value)
{
	uint8_t reg_value = 0;
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA2SS, &reg_value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		reg_value = ((*value << (EEPROM_ST25DV_RFAXSS_ALL_SHIFT)) &
			     (EEPROM_ST25DV_RFAXSS_ALL_MASK)) |
			    (reg_value & ~(EEPROM_ST25DV_RFAXSS_ALL_MASK));

		ret = _st25dv_write_conf(config, ST25DV_REG_RFA2SS, &reg_value,
					 ST25DV_RFASS_LENGTH);
	}

	return ret;
}

/**
 * @brief  Read RFA3SS_ALL register
 * @param[in] config structure containing context driver
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _get_rfa3ss(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA3SS, (uint8_t *)value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		*value &= (EEPROM_ST25DV_RFAXSS_ALL_MASK);
		*value = *value >> (EEPROM_ST25DV_RFAXSS_ALL_SHIFT);
	}

	return ret;
}

/**
 * @brief  Write RFA3SS_ALL register
 * @param[in] config structure containing context driver
 * @param[in] value data pointer to write to register
 * @return 0 in case of success, an error code otherwise
 */
static int _set_rfa3ss(const struct eeprom_st25dv_config *config, const uint8_t *const value)
{
	uint8_t reg_value = 0;
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA3SS, &reg_value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		reg_value = ((*value << (EEPROM_ST25DV_RFAXSS_ALL_SHIFT)) &
			     (EEPROM_ST25DV_RFAXSS_ALL_MASK)) |
			    (reg_value & ~(EEPROM_ST25DV_RFAXSS_ALL_MASK));

		ret = _st25dv_write_conf(config, ST25DV_REG_RFA3SS, &reg_value,
					 ST25DV_RFASS_LENGTH);
	}

	return ret;
}

/**
 * @brief  Read RFA4SS_ALL register
 * @param[in] config structure containing context driver
 * @param[out] value data pointer to store register content
 * @return 0 in case of success, an error code otherwise
 */
static int _get_rfa4ss(const struct eeprom_st25dv_config *config, uint8_t *const value)
{
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA4SS, (uint8_t *)value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		*value &= (EEPROM_ST25DV_RFAXSS_ALL_MASK);
		*value = *value >> (EEPROM_ST25DV_RFAXSS_ALL_SHIFT);
	}

	return ret;
}

/**
 * @brief  Write RFA4SS_ALL register
 * @param[in] config structure containing context driver
 * @param[in] value data pointer to write to register
 * @return 0 in case of success, an error code otherwise
 */
static int _set_rfa4ss(const struct eeprom_st25dv_config *config, const uint8_t *const value)
{
	uint8_t reg_value = 0;
	int ret;

	ret = _st25dv_read_conf(config, ST25DV_REG_RFA4SS, &reg_value, ST25DV_RFASS_LENGTH);
	if (ret == 0) {
		reg_value = ((*value << (EEPROM_ST25DV_RFAXSS_ALL_SHIFT)) &
			     (EEPROM_ST25DV_RFAXSS_ALL_MASK)) |
			    (reg_value & ~(EEPROM_ST25DV_RFAXSS_ALL_MASK));

		ret = _st25dv_write_conf(config, ST25DV_REG_RFA4SS, &reg_value,
					 ST25DV_RFASS_LENGTH);
	}

	return ret;
}

int eeprom_st25dv_read_uuid(const struct device *dev, uint8_t uuid[EEPROM_ST25DV_UUID_LENGTH])
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	if (uuid == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_conf(config, ST25DV_REG_UUID, uuid, EEPROM_ST25DV_UUID_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_read_ic_rev(const struct device *dev, uint8_t *ic_rev)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	if (ic_rev == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_conf(config, ST25DV_REG_IC_REV, ic_rev, 1U);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_read_ic_ref(const struct device *dev, uint8_t *ic_ref)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	if (ic_ref == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_conf(config, ST25DV_REG_IC_REF, ic_ref, ST25DV_IC_REV_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_read_mem_size(const struct device *dev, eeprom_st25dv_mem_size_t *mem_size)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;
	uint8_t mem_size_field[ST25DV_MEM_SIZE_LENGTH] = {0};

	if (mem_size == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_conf(config, ST25DV_REG_MEM_SIZE, mem_size_field,
				ST25DV_MEM_SIZE_LENGTH);
	if (ret == 0) {
		mem_size->mem_size = sys_get_le16(mem_size_field);
		ret = _st25dv_read_conf(config, ST25DV_REG_BLK_SIZE, &(mem_size->block_size),
					ST25DV_BLK_SIZE_LENGTH);
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_read_end_zone(const struct device *dev, const eeprom_st25dv_end_zone_t end_zone,
				uint8_t *end_zone_addr)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	*end_zone_addr = 0x00;

	/* Read the corresponding End zone */
	k_mutex_lock(&data->lock, K_FOREVER);
	switch (end_zone) {
	case EEPROM_ST25DV_END_ZONE1:
		ret = _get_enda1(config, end_zone_addr);
		break;
	case EEPROM_ST25DV_END_ZONE2:
		ret = _get_enda2(config, end_zone_addr);
		break;
	case EEPROM_ST25DV_END_ZONE3:
		ret = _get_enda3(config, end_zone_addr);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

int eeprom_st25dv_write_end_zone(const struct device *dev, const eeprom_st25dv_end_zone_t end_zone,
				 uint8_t end_zone_addr)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	switch (end_zone) {
	case EEPROM_ST25DV_END_ZONE1:
		ret = _set_enda1(config, &end_zone_addr);
		break;
	case EEPROM_ST25DV_END_ZONE2:
		ret = _set_enda2(config, &end_zone_addr);
		break;
	case EEPROM_ST25DV_END_ZONE3:
		ret = _set_enda3(config, &end_zone_addr);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

int eeprom_st25dv_read_mb_mode(const struct device *dev, uint8_t *mb_mode)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	if (mb_mode == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_conf(config, ST25DV_REG_MB_MODE, mb_mode, ST25DV_MB_MODE_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_write_mb_mode(const struct device *dev, uint8_t mb_mode)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_write_conf(config, ST25DV_REG_MB_MODE, &mb_mode, ST25DV_MB_MODE_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_read_mb_ctrl_dyn(const struct device *dev, uint8_t *mb_ctrl_dyn)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_dyn_conf(config, ST25DV_REG_MB_CTRL_DYN, mb_ctrl_dyn,
				    ST25DV_MB_CTRL_DYN_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_write_mb_ctrl_dyn(const struct device *dev, uint8_t mb_ctrl_dyn)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_write_dyn_conf(config, ST25DV_REG_MB_CTRL_DYN, &mb_ctrl_dyn,
				     ST25DV_MB_CTRL_DYN_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_read_mb_len_dyn(const struct device *dev, uint8_t *mb_len_dyn)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_read_dyn_conf(config, ST25DV_REG_MBLEN_DYN, mb_len_dyn,
				    ST25DV_MB_LEN_DYN_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_write_mb_len_dyn(const struct device *dev, uint8_t mb_len_dyn)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_write_dyn_conf(config, ST25DV_REG_MBLEN_DYN, &mb_len_dyn,
				     ST25DV_MB_LEN_DYN_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_present_i2c_password(const struct device *dev,
				       const eeprom_st25dv_passwd_t password)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	uint8_t ai2c_message[ST25DV_I2C_PASSWD_LENGTH] = {0};

	/* Build I2C Message with Password + Validation code 0x09 + Password */
	ai2c_message[8] = 0x09U;
	sys_put_be64(password, &(ai2c_message[0]));
	sys_put_be64(password, &(ai2c_message[9]));

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_write_conf(config, ST25DV_REG_I2C_PASSWD, ai2c_message,
				 ST25DV_I2C_PASSWD_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_write_i2c_password(const struct device *dev,
				     const eeprom_st25dv_passwd_t password)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;

	uint8_t ai2c_message[ST25DV_I2C_PASSWD_LENGTH] = {0};

	/* Build I2C Message with Password + Validation code 0x07 + Password */
	ai2c_message[8] = 0x07U;
	sys_put_be64(password, &(ai2c_message[0]));
	sys_put_be64(password, &(ai2c_message[9]));

	/* Write new password in I2CPASSWD register */
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _st25dv_write_conf(config, ST25DV_REG_I2C_PASSWD, ai2c_message,
				 ST25DV_I2C_PASSWD_LENGTH);
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_read_i2c_security_session_dyn(const struct device *dev,
						eeprom_st25dv_i2c_sso_status_t *const pSession)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;
	uint8_t reg_value;

	*pSession = EEPROM_ST25DV_I2C_SESSION_CLOSED;

	/* Read actual value of I2C_SSO_DYN register */
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = _read_i2c_sso_dyn(config, &reg_value);
	k_mutex_unlock(&data->lock);
	if (ret == 0) {
		/* Extract Open session information */
		if (reg_value != 0U) {
			*pSession = EEPROM_ST25DV_I2C_SESSION_OPEN;
		} else {
			*pSession = EEPROM_ST25DV_I2C_SESSION_CLOSED;
		}
	}
	return ret;
}

int eeprom_st25dv_read_rfzss(const struct device *dev, const eeprom_st25dv_protection_zone_t zone,
			     eeprom_st25dv_rf_prot_zone_t *const rf_prot_zone)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;
	uint8_t reg_value = 0;

	rf_prot_zone->passwd_ctrl = EEPROM_ST25DV_NOT_PROTECTED;
	rf_prot_zone->rw_protection = EEPROM_ST25DV_NO_PROT;

	/* Read actual value of Sector Security Status register */
	k_mutex_lock(&data->lock, K_FOREVER);
	switch (zone) {
	case EEPROM_ST25DV_PROT_ZONE1:
		ret = _get_rfa1ss(config, &reg_value);
		break;
	case EEPROM_ST25DV_PROT_ZONE2:
		ret = _get_rfa2ss(config, &reg_value);
		break;
	case EEPROM_ST25DV_PROT_ZONE3:
		ret = _get_rfa3ss(config, &reg_value);
		break;
	case EEPROM_ST25DV_PROT_ZONE4:
		ret = _get_rfa4ss(config, &reg_value);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	k_mutex_unlock(&data->lock);

	if (ret == 0) {
		/* Extract Sector Security Status configuration */
		switch ((reg_value & EEPROM_ST25DV_RFAXSS_PWD_CTRL_MASK) >>
			EEPROM_ST25DV_RFAXSS_PWD_CTRL_SHIFT) {
		case EEPROM_ST25DV_NOT_PROTECTED:
			rf_prot_zone->passwd_ctrl = EEPROM_ST25DV_NOT_PROTECTED;
			break;
		case EEPROM_ST25DV_PROT_PASSWD1:
			rf_prot_zone->passwd_ctrl = EEPROM_ST25DV_PROT_PASSWD1;
			break;
		case EEPROM_ST25DV_PROT_PASSWD2:
			rf_prot_zone->passwd_ctrl = EEPROM_ST25DV_PROT_PASSWD2;
			break;
		case EEPROM_ST25DV_PROT_PASSWD3:
			rf_prot_zone->passwd_ctrl = EEPROM_ST25DV_PROT_PASSWD3;
			break;

		default:
			ret = -EIO;
			break;
		}

		switch ((reg_value & EEPROM_ST25DV_RFAXSS_PWD_CTRL_MASK) >>
			EEPROM_ST25DV_RFAXSS_PWD_CTRL_SHIFT) {
		case EEPROM_ST25DV_NO_PROT:
			rf_prot_zone->rw_protection = EEPROM_ST25DV_NO_PROT;
			break;
		case EEPROM_ST25DV_WRITE_PROT:
			rf_prot_zone->rw_protection = EEPROM_ST25DV_WRITE_PROT;
			break;
		case EEPROM_ST25DV_READ_PROT:
			rf_prot_zone->rw_protection = EEPROM_ST25DV_READ_PROT;
			break;
		case EEPROM_ST25DV_READWRITE_PROT:
			rf_prot_zone->rw_protection = EEPROM_ST25DV_READWRITE_PROT;
			break;

		default:
			ret = -EIO;
			break;
		}
	}
	return ret;
}

int eeprom_st25dv_write_rfzss(const struct device *dev, const eeprom_st25dv_protection_zone_t zone,
			      const eeprom_st25dv_rf_prot_zone_t *rf_prot_zone)
{
	const struct eeprom_st25dv_config *config = dev->config;
	struct eeprom_st25dv_data *data = dev->data;
	int ret;
	uint8_t reg_value;

	/* Update Sector Security Status */
	reg_value = ((uint8_t)rf_prot_zone->rw_protection << EEPROM_ST25DV_RFAXSS_RW_PROT_SHIFT) &
		    EEPROM_ST25DV_RFAXSS_RW_PROT_MASK;
	reg_value |= (((uint8_t)rf_prot_zone->passwd_ctrl << EEPROM_ST25DV_RFAXSS_PWD_CTRL_SHIFT) &
		      EEPROM_ST25DV_RFAXSS_PWD_CTRL_MASK);

	/* Write Sector Security register */
	k_mutex_lock(&data->lock, K_FOREVER);
	switch (zone) {
	case EEPROM_ST25DV_PROT_ZONE1:
		ret = _set_rfa1ss(config, &reg_value);
		break;
	case EEPROM_ST25DV_PROT_ZONE2:
		ret = _set_rfa2ss(config, &reg_value);
		break;
	case EEPROM_ST25DV_PROT_ZONE3:
		ret = _set_rfa3ss(config, &reg_value);
		break;
	case EEPROM_ST25DV_PROT_ZONE4:
		ret = _set_rfa4ss(config, &reg_value);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

int eeprom_st25dv_init_end_zone(const struct device *dev)
{
	uint8_t end_value;
	uint32_t max_mem_length;
	eeprom_st25dv_mem_size_t memsize;
	int ret;

	memsize.mem_size = 0;
	memsize.block_size = 0;

	/* Get EEPROM mem size */
	(void)eeprom_st25dv_read_mem_size(dev, &memsize);
	max_mem_length = ((uint32_t)memsize.mem_size + 1U) * ((uint32_t)memsize.block_size + 1U);

	/* Compute Max value for endzone register */
	end_value = (uint8_t)((max_mem_length / 32U) - 1U);

	(void)eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE1, 0x00U);
	(void)eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE2, 0x01U);

	/* Write EndZone value to ST25DVxxKC registers */
	ret = eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE3, end_value);
	if (ret == 0) {
		ret = eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE2, end_value);
	}

	if (ret == 0) {
		ret = eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE1, end_value);
	}

	return ret;
}

int eeprom_st25dv_create_user_zone(const struct device *dev, const uint16_t zone1_length,
				   const uint16_t zone2_length, const uint16_t zone3_length,
				   const uint16_t zone4_length)
{
	uint8_t end_value;
	uint16_t zone1_length_tmp = zone1_length;
	uint16_t zone2_length_tmp = zone2_length;
	uint16_t zone3_length_tmp = zone3_length;
	uint16_t max_mem_length;
	int ret = 0;
	eeprom_st25dv_mem_size_t memsize;

	memsize.mem_size = 0;
	memsize.block_size = 0;

	/* Get EEPROM mem size */
	(void)eeprom_st25dv_read_mem_size(dev, &memsize);
	max_mem_length = ((uint32_t)memsize.mem_size + 1U) * ((uint32_t)memsize.block_size + 1U);

	/* Checks that values of different zones are in bounds */
	if ((zone1_length < 32U) || (zone1_length > max_mem_length) ||
	    (zone2_length > (max_mem_length - 32U)) || (zone3_length > (max_mem_length - 64U)) ||
	    (zone4_length > (max_mem_length - 96U))) {
		ret = -EINVAL;
	}

	/* Checks that the total is less than the authorised maximum */
	if ((zone1_length + zone2_length + zone3_length + zone4_length) > max_mem_length) {
		ret = -EINVAL;
	}

	/* if the value for each Length is not a multiple of 32 correct it. */
	if ((zone1_length % 32U) != 0U) {
		zone1_length_tmp = zone1_length - (zone1_length % 32U);
	}

	if ((zone2_length % 32U) != 0U) {
		zone2_length_tmp = zone2_length - (zone2_length % 32U);
	}

	if ((zone3_length % 32U) != 0U) {
		zone3_length_tmp = zone3_length - (zone3_length % 32U);
	}

	if (ret == 0) {
		/* First right 0xFF in each Endx value */
		ret = eeprom_st25dv_init_end_zone(dev);
	}

	if (ret == 0) {
		/* Then Write corresponding value for each zone */
		end_value = (uint8_t)((zone1_length_tmp / 32U) - 1U);
		ret = eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE1, end_value);
	}

	if (ret == 0) {
		end_value = (uint8_t)(((zone1_length_tmp + zone2_length_tmp) / 32U) - 1U);
		ret = eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE2, end_value);
	}

	if (ret == 0) {
		end_value =
			(uint8_t)(((zone1_length_tmp + zone2_length_tmp + zone3_length_tmp) / 32U) -
				  1U);
		ret = eeprom_st25dv_write_end_zone(dev, EEPROM_ST25DV_END_ZONE3, end_value);
	}

	return ret;
}

int eeprom_st25dv_set_ftm(const struct device *dev, bool enable)
{
	int ret = 0;
	/* Present password to open security session */
	ret = eeprom_st25dv_present_i2c_password(dev, 0);
	if (ret < 0) {
		LOG_ERR("Error presenting I2C password");
		return ret;
	}

	if (enable) {
		/* Authorize FTM mode by setting MB_MODE */
		ret = eeprom_st25dv_write_mb_mode(dev, 0x01);
		if (ret < 0) {
			LOG_ERR("Error write mb_mode %d", ret);
			return ret;
		}

		/* Enable FTM: read, set MB_EN bit and write */
		uint8_t mb_ctrl_dyn = 0;

		ret = eeprom_st25dv_read_mb_ctrl_dyn(dev, &mb_ctrl_dyn);
		if (ret < 0) {
			LOG_ERR("Error read mb_ctrl_dyn %d", ret);
			return ret;
		}

		mb_ctrl_dyn |= (1u << EEPROM_ST25DV_MB_CTRL_DYN_MBEN_SHIFT);
		ret = eeprom_st25dv_write_mb_ctrl_dyn(dev, mb_ctrl_dyn);
		if (ret < 0) {
			LOG_ERR("Error write mb_ctrl_dyn %d", ret);
			return ret;
		}
	} else {
		/* Disable FTM: read, clear MB_EN bit and write */
		uint8_t mb_ctrl_dyn = 0;

		ret = eeprom_st25dv_read_mb_ctrl_dyn(dev, &mb_ctrl_dyn);
		if (ret < 0) {
			LOG_ERR("Error read mb_ctrl_dyn %d", ret);
			return ret;
		}

		mb_ctrl_dyn &= EEPROM_ST25DV_MB_CTRL_DYN_MBEN_MASK;
		ret = eeprom_st25dv_write_mb_ctrl_dyn(dev, mb_ctrl_dyn);
		if (ret < 0) {
			LOG_ERR("Error write mb_ctrl_dyn %d", ret);
			return ret;
		}

		/* Forbid FTM mode by clearing MB_MODE */
		ret = eeprom_st25dv_write_mb_mode(dev, 0x00);
		if (ret < 0) {
			LOG_ERR("Error write mb_mode %d", ret);
			return ret;
		}
	}

	return ret;
}

int eeprom_st25dv_mailbox_poll_status(const struct device *dev,
				      eeprom_st25dv_mb_status_poll_t status, bool poll_for_set,
				      k_timeout_t timeout)
{
	uint8_t mb_ctrl_dyn = 0;
	uint64_t start_time;
	uint8_t mask;
	uint8_t compare_mask_value;
	int ret;

	start_time = k_uptime_ticks();

	/* Set mask according to status to poll */
	switch (status) {
	case EEPROM_ST25DV_MB_STATUS_POLL_RF_PUT:
		mask = EEPROM_ST25DV_MB_CTRL_DYN_RFPUTMSG_MASK;
		break;

	case EEPROM_ST25DV_MB_STATUS_POLL_HOST_PUT:
		mask = EEPROM_ST25DV_MB_CTRL_DYN_HOSTPUTMSG_MASK;
		break;

	case EEPROM_ST25DV_MB_STATUS_POLL_BOTH:
		mask = EEPROM_ST25DV_MB_CTRL_DYN_HOSTPUTMSG_MASK |
		       EEPROM_ST25DV_MB_CTRL_DYN_RFPUTMSG_MASK;
		break;

	default:
		return -EINVAL;
	}
	if (poll_for_set) {
		compare_mask_value = mask;
	} else {
		compare_mask_value = 0;
	}

	/* Poll status field until register matches expected value */
	do {
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    k_uptime_ticks() - start_time >= timeout.ticks) {
			ret = -EAGAIN;
			break;
		}

		ret = eeprom_st25dv_read_mb_ctrl_dyn(dev, &mb_ctrl_dyn);
		k_msleep(EEPROM_ST25DV_POLL_INTERVAL_MS);

	} while (ret == 0 && (mb_ctrl_dyn & mask) != compare_mask_value);

	if (ret == 0 && (mb_ctrl_dyn & mask) == compare_mask_value) {
		return 0;
	} else {
		return -EIO;
	}
}

int eeprom_st25dv_mailbox_read(const struct device *dev, uint8_t *buffer, size_t buffer_length)
{
	const struct eeprom_st25dv_config *config = dev->config;
	int ret = 0;
	uint8_t mb_len;
	/*
	 * MB_LEN contains length of message -1
	 * Increment by one to get the actual length of the message in mailbox
	 */
	ret = eeprom_st25dv_read_mb_len_dyn(dev, &mb_len);
	uint16_t msg_length = (uint16_t)mb_len + 1;
	/* Check buffer is big enough to store mailbox message */
	if (buffer_length < msg_length) {
		return -EINVAL;
	}

	/*
	 * Some hardware may not allow reading 256 bytes in a single read
	 * Use device properties to know if such limitation is applicable or not.
	 * If so, split in two reads of 128 bytes
	 */
	if (config->i2c_transfer_is_limited && msg_length > EEPROM_ST25DV_I2C_LIMIT_MAX) {
		ret = eeprom_st25dv_read(dev, ST25DV_REG_MAILBOX_RAM, buffer,
					 EEPROM_ST25DV_I2C_LIMIT_SPLIT_TRANSFER);
		if (ret == 0) {
			ret = eeprom_st25dv_read(
				dev,
				ST25DV_REG_MAILBOX_RAM + EEPROM_ST25DV_I2C_LIMIT_SPLIT_TRANSFER,
				&(buffer[EEPROM_ST25DV_I2C_LIMIT_SPLIT_TRANSFER]),
				msg_length - EEPROM_ST25DV_I2C_LIMIT_SPLIT_TRANSFER);
			if (ret == 0) {
				ret = msg_length;
			}
		}
	} else {
		ret = eeprom_st25dv_read(dev, ST25DV_REG_MAILBOX_RAM, buffer, msg_length);
		if (ret == 0) {
			ret = msg_length;
		}
	}
	return ret;
}

int eeprom_st25dv_mailbox_write(const struct device *dev, uint8_t *buffer, size_t length)
{
	if (length > 256) {
		return -EINVAL;
	}

	return eeprom_st25dv_write(dev, ST25DV_REG_MAILBOX_RAM, buffer, length);
}

#endif /* defined(CONFIG_EEPROM_ST25DV_ENABLE_ADVANCED_FEATURES) */

static const struct eeprom_driver_api eeprom_st25dv_api = {
	.read = eeprom_st25dv_read,
	.write = eeprom_st25dv_write,
	.size = eeprom_st25dv_size,
};

#define ST25DV_DEVICE_INIT(inst)                                                                   \
	static const struct eeprom_st25dv_config _st25dv_config_##inst = {                         \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.addr = DT_INST_REG_ADDR(inst),                                                    \
		.size = DT_INST_PROP(inst, size),                                                  \
		.i2c_transfer_is_limited = DT_INST_PROP(inst, i2c_transfer_is_limited),            \
	};                                                                                         \
	static struct eeprom_st25dv_data _st25dv_data_##inst;                                      \
	DEVICE_DT_INST_DEFINE(inst, eeprom_st25dv_init, eeprom_st25dv_pm, &_st25dv_data_##inst,    \
			      &_st25dv_config_##inst, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &eeprom_st25dv_api);

DT_INST_FOREACH_STATUS_OKAY(ST25DV_DEVICE_INIT)
