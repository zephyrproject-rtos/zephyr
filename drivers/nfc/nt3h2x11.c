/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_nt3h2x11

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/nfc/nt3h2x11.h>

#include <zephyr/nfc/nfc_tag.h>

#include <zephyr/logging/log.h>

#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(nt3h2x11, CONFIG_NT3H2X11_LOG_LEVEL);

/******************* DEVICE STRUCTURE *******************/

struct nt3h2x11_cfg {
	/* i2c parameters */
	struct i2c_dt_spec i2c;
	/* irq DTS settings */
	const struct gpio_dt_spec irq_gpio;
	uint8_t irq_pin;
	/* internal (1) or external driver (0) */
	uint8_t internal;
};

struct nt3h2x11_data {
	const struct device *parent;
	const struct device *dev_i2c;
	const struct device *dev_irq_external;
	struct gpio_callback dev_irq_external_cb;
	struct k_work worker_irq;
	nt3h2x11_irq_callback_t app_irq_cb;
	uint8_t initialized;
	/* FD pin edge detection */
	uint8_t flag_fd_pin;
	/* NFC subsys data */
	nfc_tag_cb_t nfc_tag_cb;
	enum nfc_tag_type tag_type;
};

/**
 * @brief Macro-helper to set bit in session/config-block
 *
 * Depends on NT3H2X11_REG_xx and NT3H2X11_MSK_xx definitions
 */
#define NT3H2X11_CSREG_SET_EN(_d, _csreg, _r, _m, _v)                                              \
	_nt3h2x11_write_csreg_register_enable(_d, _csreg, NT3H2X11_REG_##_r,                       \
					      NT3H2X11_MSK_##_r##_##_m, _v)

/**
 * @brief Macro-helper to read bit in session/config-block
 *
 * Depends on NT3H2X11_REG_xx and NT3H2X11_MSK_xx definitions
 */
#define NT3H2X11_CSREG_GET_EN(_d, _csreg, _r, _m, _v)                                              \
	_nt3h2x11_read_csreg_register_enable(_d, _csreg, NT3H2X11_REG_##_r,                        \
					     NT3H2X11_MSK_##_r##_##_m, _v)

/**
 * @brief Macro-helper to set multi-bit value in session/config-block
 *
 * Depends on NT3H2X11_REG_xx and NT3H2X11_MSK_xx definitions
 */
#define NT3H2X11_CSREG_SET_VAL(_d, _csreg, _r, _m, _v)                                             \
	_nt3h2x11_write_csreg_register_value(_d, _csreg, NT3H2X11_REG_##_r,                        \
					     NT3H2X11_MSK_##_r##_##_m, _v)

/**
 * @brief Macro-helper to read multi-bit value in session/config-block
 *
 * Depends on NT3H2X11_REG_xx and NT3H2X11_MSK_xx definitions
 */
#define NT3H2X11_CSREG_GET_VAL(_d, _csreg, _r, _m, _v)                                             \
	_nt3h2x11_read_csreg_register_value(_d, _csreg, NT3H2X11_REG_##_r,                         \
					    NT3H2X11_MSK_##_r##_##_m, _v)

/******************* HELPER FUNCTIONS *******************/

/**
 * @brief Read a single-byte value from register in the SESSION or CONFIG block
 *
 * @param[in]  *dev      : Pointer to nt3h2x11 device
 * @param[in]   csreg    : SESSION or CONFIG register-block
 * @param[in]   reg_addr : Register-address to read
 * @param[out] *val      : Pointer to value-buffer
 * @return               : 0 on success, negative upon error.
 */
int _nt3h2x11_read_csreg_register(const struct device *dev, enum nt3h2x11_csreg csreg,
				  uint8_t reg_addr, uint8_t *val)
{
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;
	const struct nt3h2x11_cfg *cfg = (const struct nt3h2x11_cfg *)dev->config;

	if (data->dev_i2c == NULL) {
		return -ENODEV;
	}

	uint8_t wbuf[2];

	wbuf[0] = csreg;
	wbuf[1] = reg_addr;

	int rv = i2c_write_read(data->dev_i2c, cfg->i2c.addr, wbuf, 2u, val, 1u);

	if (rv != 0) {
		LOG_ERR("I2C write_read error: %d", rv);
	}

	return rv;
}

/**
 * @brief Write a single-byte value to a register in the SESSION or CONFIG block
 *
 * @param[in] *dev      : Pointer to nt3h2x11 device
 * @param[in]  csreg    : SESSION or CONFIG register-block
 * @param[in]  reg_addr : Register-address to write to
 * @param[in]  mask     : Mask of value within register
 * @param[in]  val      : Value to write
 * @return              : 0 on success, negative upon error.
 */
static int _nt3h2x11_write_csreg_register(const struct device *dev, enum nt3h2x11_csreg csreg,
					  uint8_t reg_addr, uint8_t mask, uint8_t val)
{
	int rv = 0;
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;
	const struct nt3h2x11_cfg *cfg = (const struct nt3h2x11_cfg *)dev->config;

	if (data->dev_i2c == NULL) {
		return -ENODEV;
	}

	uint8_t wbuf[4];

	wbuf[0] = csreg;
	wbuf[1] = reg_addr;
	wbuf[2] = mask;
	wbuf[3] = val;

	rv = i2c_write(data->dev_i2c, wbuf, 4u, cfg->i2c.addr);
	if (rv != 0) {
		LOG_ERR("I2C write error: %d", rv);
		return rv;
	}

	return 0;
}

/**
 * @brief Write and validate a single-bit value to a register in the SESSION or
 *        CONFIG block
 *
 * @param[in] *dev      : Pointer to nt3h2x11 device
 * @param[in]  csreg    : SESSION or CONFIG register-block
 * @param[in]  reg_addr : Register-address to write to
 * @param[in]  mask     : Mask of value within register
 * @param[in]  val      : Value to write
 * @return              : 0 on success, negative upon error.
 */
static int _nt3h2x11_write_csreg_register_enable(const struct device *dev,
						 enum nt3h2x11_csreg csreg, uint8_t reg,
						 uint8_t mask, uint8_t val)
{
	uint8_t mask_val = (val != 0) ? mask : 0u;

	return _nt3h2x11_write_csreg_register(dev, csreg, reg, mask, mask_val);
}

/**
 * @brief Read and validate a single-bit value to a register in the SESSION or
 *        CONFIG block
 *
 * @param[in]  *dev      : Pointer to nt3h2x11 device
 * @param[in]   csreg    : SESSION or CONFIG register-block
 * @param[in]   reg_addr : Register-address to write to
 * @param[in]   mask     : Mask of value within register
 * @param[out] *val      : Pointer to value where result is stored
 * @return               : 0 on success, negative upon error.
 */
static int _nt3h2x11_read_csreg_register_enable(const struct device *dev, enum nt3h2x11_csreg csreg,
						uint8_t reg, uint8_t mask, uint8_t *val)
{
	uint8_t regval = 0u;
	int rv = _nt3h2x11_read_csreg_register(dev, csreg, reg, &regval);

	if (rv == 0) {
		*val = ((regval & mask) != 0) ? 1u : 0u;
	}

	return rv;
}

/**
 * @brief Write and validate a multi-bit value to a register in the SESSION or
 *        CONFIG block
 *
 * @param[in] *dev      : Pointer to nt3h2x11 device
 * @param[in]  csreg    : SESSION or CONFIG register-block
 * @param[in]  reg_addr : Register-address to write to
 * @param[in]  mask     : Mask of value within register
 * @param[in]  val      : Value to write
 * @return              : 0 on success, negative upon error.
 */
static int _nt3h2x11_write_csreg_register_value(const struct device *dev, enum nt3h2x11_csreg csreg,
						uint8_t reg, uint8_t mask, uint8_t val)
{
	return _nt3h2x11_write_csreg_register(dev, csreg, reg, mask, val);
}

/**
 * @brief Read and validate a multi-bit value to a register in the SESSION or
 *        CONFIG block
 *
 * @param[in]  *dev      : Pointer to nt3h2x11 device
 * @param[in]  csreg     : SESSION or CONFIG register-block
 * @param[in]   reg_addr : Register-address to write to
 * @param[in]   mask     : Mask of value within register
 * @param[out] *val      : Pointer to value where result is stored
 * @return               : 0 on success, negative upon error.
 */
static int _nt3h2x11_read_csreg_register_value(const struct device *dev, enum nt3h2x11_csreg csreg,
					       uint8_t reg, uint8_t mask, uint8_t *val)
{
	uint8_t regval = 0u;
	int rv = _nt3h2x11_read_csreg_register(dev, csreg, reg, &regval);

	if (rv == 0) {
		*val = (regval & mask);
	}

	return rv;
}

/******************* DRIVER FUNCTIONS *******************/

int nt3h2x11_set_device(const struct device *dev, struct nt3h2x11_device *dblk)
{
	return nt3h2x11_write_blocks(dev, NT3H2X11_BLK_DEVICE, (uint8_t *)dblk, 1u);
}

int nt3h2x11_get_device(const struct device *dev, struct nt3h2x11_device *dblk)
{
	return nt3h2x11_read_blocks(dev, NT3H2X11_BLK_DEVICE, (uint8_t *)dblk, 1u);
}

int nt3h2x11_set_cfg_auth(const struct device *dev, struct nt3h2x11_cfg_auth *cfg_auth)
{
	int rv = 0;
	uint8_t blk_la[NT3H2X11_BYTES_BLK]; /* lock&auth */
	uint8_t blk_ap[NT3H2X11_BYTES_BLK]; /* access&pwd */

	/* read current authentication */
	rv = nt3h2x11_read_blocks(dev, NT3H2X11_BLK_LOCK_AUTH, blk_la, 1u);
	if (rv != 0) {
		return rv;
	}
	rv = nt3h2x11_read_blocks(dev, NT3H2X11_BLK_ACCESS_PWD, blk_ap, 1u);
	if (rv != 0) {
		return rv;
	}

	/* update authentication */
	(void)memcpy(&blk_la[NT3H2X11_REG_DYNAMIC_LOCK], cfg_auth->dyn_lock,
		     NT3H2X11_BYTES_DYNAMIC_LOCK);
	(void)memcpy(&blk_la[NT3H2X11_REG_AUTH0], cfg_auth->auth0, NT3H2X11_BYTES_AUTH0);
	(void)memcpy(&blk_ap[NT3H2X11_REG_ACCESS], cfg_auth->access, NT3H2X11_BYTES_ACCESS);
	(void)memcpy(&blk_ap[NT3H2X11_REG_PWD], cfg_auth->pwd, NT3H2X11_BYTES_PWD);
	(void)memcpy(&blk_ap[NT3H2X11_REG_PACK], cfg_auth->pack, NT3H2X11_BYTES_PACK);
	(void)memcpy(&blk_ap[NT3H2X11_REG_PT_I2C], cfg_auth->pt_i2c, NT3H2X11_BYTES_PT_I2C);

	/* write updated authentication */
	rv = nt3h2x11_write_blocks(dev, NT3H2X11_BLK_LOCK_AUTH, blk_la, 1u);
	if (rv != 0) {
		return rv;
	}
	rv = nt3h2x11_write_blocks(dev, NT3H2X11_BLK_ACCESS_PWD, blk_ap, 1u);
	if (rv != 0) {
		return rv;
	}

	return 0;
}

int nt3h2x11_get_cfg_auth(const struct device *dev, struct nt3h2x11_cfg_auth *cfg_auth)
{
	int rv = 0;
	uint8_t blk_la[NT3H2X11_BYTES_BLK]; /* lock&auth */
	uint8_t blk_ap[NT3H2X11_BYTES_BLK]; /* access&pwd */

	rv = nt3h2x11_read_blocks(dev, NT3H2X11_BLK_LOCK_AUTH, blk_la, 1u);
	if (rv != 0) {
		return rv;
	}
	rv = nt3h2x11_read_blocks(dev, NT3H2X11_BLK_ACCESS_PWD, blk_ap, 1u);
	if (rv != 0) {
		return rv;
	}

	(void)memcpy(cfg_auth->dyn_lock, &blk_la[NT3H2X11_REG_DYNAMIC_LOCK],
		     NT3H2X11_BYTES_DYNAMIC_LOCK);
	(void)memcpy(cfg_auth->auth0, &blk_la[NT3H2X11_REG_AUTH0], NT3H2X11_BYTES_AUTH0);
	(void)memcpy(cfg_auth->access, &blk_ap[NT3H2X11_REG_ACCESS], NT3H2X11_BYTES_ACCESS);
	(void)memcpy(cfg_auth->pwd, &blk_ap[NT3H2X11_REG_PWD], NT3H2X11_BYTES_PWD);
	(void)memcpy(cfg_auth->pack, &blk_ap[NT3H2X11_REG_PACK], NT3H2X11_BYTES_PACK);
	(void)memcpy(cfg_auth->pt_i2c, &blk_ap[NT3H2X11_REG_PT_I2C], NT3H2X11_BYTES_PT_I2C);

	return rv;
}

/******************* SESSION AND CONFIG OPERATIONS *******************/

int nt3h2x11_set_softreset_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable)
{
	return NT3H2X11_CSREG_SET_EN(dev, csreg, NC_REG, RST_ON_OFF, enable);
}

int nt3h2x11_get_softreset_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *enable)
{
	return NT3H2X11_CSREG_GET_EN(dev, csreg, NC_REG, RST_ON_OFF, enable);
}

int nt3h2x11_set_pthru_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable)
{
	return NT3H2X11_CSREG_SET_EN(dev, csreg, NC_REG, PTHRU_ON_OFF, enable);
}

int nt3h2x11_get_pthru_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *enable)
{
	return NT3H2X11_CSREG_GET_EN(dev, csreg, NC_REG, PTHRU_ON_OFF, enable);
}

int nt3h2x11_set_fd_off(const struct device *dev, enum nt3h2x11_csreg csreg,
			enum nt3h2x11_fd_off fd_off)
{
	return NT3H2X11_CSREG_SET_VAL(dev, csreg, NC_REG, FD_OFF, fd_off);
}

int nt3h2x11_get_fd_off(const struct device *dev, enum nt3h2x11_csreg csreg,
			enum nt3h2x11_fd_off *fd_off)
{
	return NT3H2X11_CSREG_GET_VAL(dev, csreg, NC_REG, FD_OFF, (uint8_t *)fd_off);
}

int nt3h2x11_set_fd_on(const struct device *dev, enum nt3h2x11_csreg csreg,
		       enum nt3h2x11_fd_on fd_on)
{
	return NT3H2X11_CSREG_SET_VAL(dev, csreg, NC_REG, FD_ON, fd_on);
}

int nt3h2x11_get_fd_on(const struct device *dev, enum nt3h2x11_csreg csreg,
		       enum nt3h2x11_fd_on *fd_on)
{
	return NT3H2X11_CSREG_GET_VAL(dev, csreg, NC_REG, FD_ON, (uint8_t *)fd_on);
}

int nt3h2x11_set_srammirror_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable)
{
	return NT3H2X11_CSREG_SET_EN(dev, csreg, NC_REG, SRAM_MIRROR_ON_OFF, enable);
}

int nt3h2x11_get_srammirror_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *enable)
{
	return NT3H2X11_CSREG_GET_EN(dev, csreg, NC_REG, SRAM_MIRROR_ON_OFF, enable);
}

int nt3h2x11_set_transfer_dir(const struct device *dev, enum nt3h2x11_csreg csreg,
			      enum nt3h2x11_transfer_dir dir)
{
	int rv = 0;
	uint8_t regval = 0u;

	/* Read current setting */
	rv = _nt3h2x11_read_csreg_register(dev, csreg, NT3H2X11_REG_NC_REG, &regval);
	if (rv != 0) {
		return rv;
	}

	/* only update if direction is different from current setting */
	if (dir != (regval & NT3H2X11_MSK_NC_REG_TRANSFER_DIR)) {
		/* turn pthru on/off when active */
		if ((regval & NT3H2X11_MSK_NC_REG_TRANSFER_DIR) != 0u) {

			rv = NT3H2X11_CSREG_SET_EN(dev, csreg, NC_REG, PTHRU_ON_OFF, 1u);
			if (rv != 0) {
				return rv;
			}
			rv = NT3H2X11_CSREG_SET_VAL(dev, csreg, NC_REG, TRANSFER_DIR, dir);
			if (rv != 0) {
				return rv;
			}
			rv = NT3H2X11_CSREG_SET_EN(dev, csreg, NC_REG, PTHRU_ON_OFF, 0u);
			if (rv != 0) {
				return rv;
			}

		} else {
			rv = NT3H2X11_CSREG_SET_VAL(dev, csreg, NC_REG, TRANSFER_DIR, dir);
			if (rv != 0) {
				return rv;
			}
		}
	}
	return 0;
}

int nt3h2x11_get_transfer_dir(const struct device *dev, enum nt3h2x11_csreg csreg,
			      enum nt3h2x11_transfer_dir *dir)
{
	return NT3H2X11_CSREG_GET_VAL(dev, csreg, NC_REG, TRANSFER_DIR, (uint8_t *)dir);
}

int nt3h2x11_set_last_ndef_blk(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t block)
{
	if (block > NT3H2X11_LAST_NDEF_BLOCK_MAX) {
		return -EINVAL;
	}
	return _nt3h2x11_write_csreg_register(dev, csreg, NT3H2X11_REG_LAST_NDEF_BLOCK, 0xFFu,
					      block);
}

int nt3h2x11_get_last_ndef_blk(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *block)
{
	return _nt3h2x11_read_csreg_register(dev, csreg, NT3H2X11_REG_LAST_NDEF_BLOCK, block);
}

int nt3h2x11_set_srammirror_blk(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t block)
{
	return _nt3h2x11_write_csreg_register(dev, csreg, NT3H2X11_REG_SRAM_MIRROR_BLOCK, 0xFFu,
					      block);
}

int nt3h2x11_get_srammirror_blk(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *block)
{
	return _nt3h2x11_read_csreg_register(dev, csreg, NT3H2X11_REG_SRAM_MIRROR_BLOCK, block);
}

int nt3h2x11_set_wdt(const struct device *dev, enum nt3h2x11_csreg csreg, uint16_t time)
{
	int rv = 0;
	uint8_t val = 0u;

	/* Write LSB */
	val = (time >> 0u) & 0xFFu;
	rv = _nt3h2x11_write_csreg_register(dev, csreg, NT3H2X11_REG_WDT_LS, 0xFFu, val);
	if (rv != 0) {
		return rv;
	}

	/* Write MSB */
	val = (time >> 8u) & 0xFFu;
	rv = _nt3h2x11_write_csreg_register(dev, csreg, NT3H2X11_REG_WDT_MS, 0xFFu, val);
	if (rv != 0) {
		return rv;
	}

	return 0;
}

int nt3h2x11_get_wdt(const struct device *dev, enum nt3h2x11_csreg csreg, uint16_t *wdt)
{
	int rv = 0;
	uint8_t regval = 0u;

	rv = _nt3h2x11_read_csreg_register(dev, csreg, NT3H2X11_REG_WDT_LS, &regval);
	if (rv != 0) {
		return rv;
	}

	*wdt |= (((uint16_t)regval) << 0u) & 0x00FFu;

	rv = _nt3h2x11_read_csreg_register(dev, csreg, NT3H2X11_REG_WDT_MS, &regval);
	if (rv != 0) {
		return rv;
	}

	*wdt |= (((uint16_t)regval) << 8u) & 0xFF00u;

	return 0;
}

int nt3h2x11_set_i2c_clkstr_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t enable)
{
	return NT3H2X11_CSREG_SET_EN(dev, csreg, I2C_CLOCK_STR, CLOCK_STR, enable);
}

int nt3h2x11_get_i2c_clkstr_en(const struct device *dev, enum nt3h2x11_csreg csreg, uint8_t *enable)
{
	return NT3H2X11_CSREG_GET_EN(dev, csreg, I2C_CLOCK_STR, CLOCK_STR, (uint8_t *)enable);
}

/******************* CONFIG ONLY OPERATIONS *******************/

int nt3h2x11_get_i2c_lock_config(const struct device *dev, uint8_t *locked)
{
	return NT3H2X11_CSREG_GET_EN(dev, NT3H2X11_CONFIG, CFG_REG_LOCK, LOCK_I2C, locked);
}

int nt3h2x11_get_nfc_lock_config(const struct device *dev, uint8_t *locked)
{
	return NT3H2X11_CSREG_GET_EN(dev, NT3H2X11_CONFIG, CFG_REG_LOCK, LOCK_NFC, locked);
}

/******************* SESSION ONLY OPERATIONS *******************/

int nt3h2x11_get_auth_en(const struct device *dev, uint8_t *enable)
{
	return NT3H2X11_CSREG_GET_EN(dev, NT3H2X11_SESSION, I2C_CLOCK_STR, NEG_AUTH, enable);
}

int nt3h2x11_get_nsreg(const struct device *dev, uint8_t *nsreg)
{
	return _nt3h2x11_read_csreg_register(dev, NT3H2X11_SESSION, NT3H2X11_REG_NS_REG, nsreg);
}

/******************* GENERIC READ/WRITE *******************/

int nt3h2x11_read_blocks(const struct device *dev, uint8_t block, uint8_t *buf, uint8_t count)
{
	int rv = 0u;
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;
	const struct nt3h2x11_cfg *cfg = (const struct nt3h2x11_cfg *)dev->config;

	if (data->dev_i2c == NULL) {
		return -ENODEV;
	}

	uint16_t buf_idx = 0u;
	uint8_t block_idx = block;

	/* read blocks + store directly in buf[] */
	for (uint8_t i = 0u; i < count; i++) {

		rv = i2c_write_read(data->dev_i2c, cfg->i2c.addr, &block_idx, sizeof(block_idx),
				    &buf[buf_idx], NT3H2X11_BYTES_BLK);
		if (rv != 0u) {
			LOG_ERR("I2C write_read error: %d", rv);
			return rv;
		}

		/* shift number of bytes in buf ; shift 1 in block-idx */
		buf_idx += NT3H2X11_BYTES_BLK;
		block_idx += 1;
	}

	return 0;
}

int nt3h2x11_write_blocks(const struct device *dev, uint8_t block, uint8_t *buf, uint8_t count)
{
	int rv = 0;
	uint32_t timeout = 0u;
	uint8_t nsreg = 0u;
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;
	const struct nt3h2x11_cfg *cfg = (const struct nt3h2x11_cfg *)dev->config;

	if (data->dev_i2c == NULL) {
		return -ENODEV;
	}

	uint16_t buf_idx = 0u;
	uint8_t block_idx = block;
	uint8_t wbuf[NT3H2X11_BYTES_BLK + 1];
	(void)memset(wbuf, 0u, sizeof(wbuf));

	/*
	 * First byte in first block is i2c addr.
	 * Changing the address needs to be done outside this driver.
	 * Bitshift is required as LSB is used for RW-details
	 */
	if (block == 0) {
		buf[0] = 0xFFu & (cfg->i2c.addr << 1);
	}

	for (uint8_t i = 0u; i < count; i++) {

		/* First byte in write-buffer has to be block address */
		wbuf[0] = block_idx;
		(void)memcpy(&wbuf[1], &buf[buf_idx], NT3H2X11_BYTES_BLK);

		/* Write block data */
		rv = i2c_write(data->dev_i2c, wbuf, NT3H2X11_BYTES_BLK + 1, cfg->i2c.addr);
		if (rv != 0) {
			LOG_ERR("I2C write_read error: %d", rv);
			return rv;
		}

		/*
		 * If address is within EEPROM memory region, wait for completion
		 * ==> Completion bit is written in SESSION block, NS_REG.
		 *     Detection is done with NT3H2X11_MSK_NS_REG_EEPROM_WR_BUSY
		 */
		if ((block <= NT3H2X11_BLK_SRAM_START) ||
		    (block >
		     (NT3H2X11_BLK_SRAM_START + (NT3H2X11_BYTES_SRAM / NT3H2X11_BYTES_BLK)))) {

			/* number of timeouts to wait */
			timeout = (CONFIG_NFC_NT3H2X11_MAX_WRITE_DELAY / 5u) + 1u;
			do {
				(void)k_sleep(K_MSEC(5));
				rv = _nt3h2x11_read_csreg_register(dev, NT3H2X11_SESSION,
								   NT3H2X11_REG_NS_REG, &nsreg);
				if (rv != 0) {
					LOG_ERR("I2C read error: %d", rv);
					return rv;
				}
				timeout--;
			} while ((timeout != 0u) &&
				 ((nsreg & NT3H2X11_MSK_NS_REG_EEPROM_WR_BUSY) != 0u));

			if (timeout == 0u) {
				return -ETIMEDOUT;
			}
		}

		/* shift number of bytes in buf ; shift 1 in block-idx */
		buf_idx += NT3H2X11_BYTES_BLK;
		block_idx += 1;
	}

	return 0;
}

int nt3h2x11_read_bytes(const struct device *dev, uint16_t addr, uint8_t *buf, uint16_t buf_len)
{
	int rv;

	/* Offset of addr within a block */
	uint8_t offset = addr % NT3H2X11_BYTES_BLK;
	/* Remaining (unused)bytes in a block after buf_len */
	uint8_t remain = NT3H2X11_BYTES_BLK - ((buf_len + offset) % NT3H2X11_BYTES_BLK);
	/* Start Block */
	uint8_t block = (addr - offset) / NT3H2X11_BYTES_BLK;
	/* Number of Blocks */
	uint8_t blocks = (offset + buf_len + remain) / NT3H2X11_BYTES_BLK;

	/* Read blocks */
	uint8_t rbuf[NT3H2X11_BYTES_BLK];
	uint32_t idx_buf = 0u; /* index within `buf` to which we copy */
	uint32_t len_copy;     /* number of bytes to copy per iteration */

	for (uint8_t i = 0u; i <= blocks; i++) {
		rv = nt3h2x11_read_blocks(dev, block, rbuf, 1u);
		if (rv != 0) {
			return rv;
		}

		/* Copy data to rbuf */
		len_copy = MIN(NT3H2X11_BYTES_BLK - offset, buf_len - idx_buf);
		(void)memcpy(&buf[idx_buf], &rbuf[offset], len_copy);

		block++;     /* copy next block */
		offset = 0u; /* copy from start of next block */
		idx_buf += len_copy;
	}

	return 0;
}

int nt3h2x11_write_bytes(const struct device *dev, uint16_t addr, uint8_t *buf, uint16_t buf_len)
{
	int rv;

	/* Offset of addr within a block */
	uint8_t offset = addr % NT3H2X11_BYTES_BLK;
	/* Remaining (unused)bytes in a block after buf_len */
	uint8_t remain = NT3H2X11_BYTES_BLK - ((buf_len + offset) % NT3H2X11_BYTES_BLK);
	/* Start Block */
	uint8_t block = (addr - offset) / NT3H2X11_BYTES_BLK;
	/* Number of Blocks */
	uint8_t blocks = (offset + buf_len + remain) / NT3H2X11_BYTES_BLK;

	/* Read blocks */
	uint8_t rbuf[NT3H2X11_BYTES_BLK];
	uint32_t idx_buf = 0u; /* index within `buf` from which we copy */
	uint32_t len_copy;     /* number of bytes to copy per iteration */

	for (uint8_t i = 0u; i <= blocks; i++) {
		rv = nt3h2x11_read_blocks(dev, block, rbuf, 1u);
		if (rv != 0) {
			return rv;
		}

		/* Copy data to rbuf */
		len_copy = MIN(NT3H2X11_BYTES_BLK - offset, buf_len - idx_buf);
		(void)memcpy(&rbuf[offset], &buf[idx_buf], len_copy);

		/* Write blocks */
		rv = nt3h2x11_write_blocks(dev, block, rbuf, 1u);
		if (rv != 0) {
			return rv;
		}

		block++;     /* copy next block */
		offset = 0u; /* read from start of next block */
		idx_buf += len_copy;
	}

	return 0;
}

int nt3h2x11_set_i2c_addr(const struct device *dev, uint8_t addr_old, uint8_t addr_new)
{
	int rv = 0;
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;

	uint8_t buf[NT3H2X11_BYTES_BLK + 1];
	uint8_t block_idx = 0u;

	rv = i2c_write_read(data->dev_i2c, addr_old, &block_idx, sizeof(block_idx), &buf[1],
			    NT3H2X11_BYTES_BLK);
	if (rv != 0) {
		LOG_DBG("Can't update I2C (%02x => %02x). Read Error: %d", addr_old, addr_new, rv);
		return rv;
	}

	/* First byte in write-buffer has to be block address */
	buf[0] = 0u;

	/* First byte of first block is i2c addr */
	/* bitshift is required as LSB is used for RW-details */
	buf[1] = 0xFFu & (addr_new << 1);

	/* Write block data */
	rv = i2c_write(data->dev_i2c, buf, NT3H2X11_BYTES_BLK + 1, addr_old);
	if (rv != 0) {
		LOG_DBG("Can't change I2C (%02x => %02x). Write Error: %d", addr_old, addr_new, rv);
		return rv;
	}
	printk("\n!! - I2C addr has changed : %02x => %02x - !!\n", addr_old, addr_new);
#if CONFIG_REBOOT
	printk("\n!! - REBOOTING.. - !!\n");
	k_sleep(K_MSEC(250));
	sys_reboot(SYS_REBOOT_COLD);
#else
	printk("\n!! - DEVICE RESET REQUIRED - !!\n");
	while (true) {
	}
#endif /* CONFIG_REBOOT */
	return 0;
}

/******************* IRQ HANDLING *******************/

static enum nt3h2x11_event _nt3h2x11_reg2event(const struct device *dev, uint8_t nc_reg,
					       uint8_t ns_reg)
{
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;
	enum nt3h2x11_event event = NT3H2X11_EVENT_NONE;

	/* process registers */
	enum nt3h2x11_fd_on fd_on = nc_reg & NT3H2X11_MSK_NC_REG_FD_ON;
	enum nt3h2x11_fd_off fd_off = nc_reg & NT3H2X11_MSK_NC_REG_FD_OFF;
	enum nt3h2x11_transfer_dir dir = nc_reg & NT3H2X11_MSK_NC_REG_TRANSFER_DIR;
	uint8_t flag_pthru = nc_reg & NT3H2X11_MSK_NC_REG_PTHRU_ON_OFF;

	uint8_t flag_ndef_read = ns_reg & NT3H2X11_MSK_NS_REG_NDEF_DATA_READ;
	uint8_t flag_i2c_locked = ns_reg & NT3H2X11_MSK_NS_REG_I2C_LOCKED;
	uint8_t flag_nfc_locked = ns_reg & NT3H2X11_MSK_NS_REG_RF_LOCKED;
	uint8_t flag_i2c_sram_ready = ns_reg & NT3H2X11_MSK_NS_REG_SRAM_I2C_READY;
	uint8_t flag_nfc_sram_ready = ns_reg & NT3H2X11_MSK_NS_REG_SRAM_RF_READY;
	uint8_t flag_fd_pin = ns_reg & NT3H2X11_MSK_NS_REG_RF_FIELD_PRESENT;

	/* Check FD-edge to discriminate field-on/off from other events */
	uint8_t fd_edge = (data->flag_fd_pin != flag_fd_pin) ? 1u : 0u;

	data->flag_fd_pin = flag_fd_pin;

	/*
	 * Section 8.4, p34 of NT3H2x11 user manualL:
	 * REMARK: When FD_ON is configured to trigger on NFC field presence (00b),
	 * FD will be pulled low again, when host is reading the NDEF_DATA_READ bit
	 * of NS_REG session register from I2C perspective.
	 *
	 * TODO: This might trigger additional "NFC_TAG_EVENT_FIELD_ON" callbacks.
	 * We should probably need to add a filter?
	 */

	/* Field Detect = On */
	switch (fd_on) {

	/* Event upon which the signal output on the FD pin is pulled low
	 *  00b: if the field is switched on
	 */
	case NT3H2X11_FD_ON_RF_ON:
		if ((fd_edge != 0u) && (flag_fd_pin != 0u)) {
			event = NT3H2X11_EVENT_FD_ON;
		}
		break;

	/* Event upon which the signal output on the FD pin is pulled low
	 * 01b: by first valid start of communication (SoC)
	 */
	case NT3H2X11_FD_ON_RF_FIRST_VALID:
		if ((fd_edge != 0u) && (flag_fd_pin != 0u)) {
			event = NT3H2X11_EVENT_START_OF_COMM;
		}
		break;

	/* Event upon which the signal output on the FD pin is pulled low
	 * 10b: by selection of the tag
	 */
	case NT3H2X11_FD_ON_TAG_SELECTION:
		if ((fd_edge != 0u) && (flag_fd_pin != 0u)) {
			event = NT3H2X11_EVENT_SELECTED;
		}
		break;

	/* Event upon which the signal output on the FD pin is pulled low
	 * 11b: (pthru:NFC>I2C) if data is ready to be read from I2C
	 * 11b: (pthru:I2C>NFC) if data is read by the NFC interface
	 */
	case NT3H2X11_FD_ON_RF_DATA_READY:
		/* only when pass-through is enabled */
		if (flag_pthru != 0u) {
			/* if data is ready to be read from I2C (pthru:NFC>I2C) */
			if (dir == NT3H2X11_TRANSFER_DIR_RF_TO_I2C) {
				if (flag_i2c_sram_ready != 0u) {
					event = NT3H2X11_EVENT_DATA_READY_I2C;
				}
				/* if data is read by the NFC interface (pthru:I2C>NFC) */
			} else {
				if (flag_nfc_locked == 0u) {
					event = NT3H2X11_EVENT_LAST_DATA_READ_NFC;
				}
			}
		}
		break;

	default:
		break;
	}

	/* Field Detect = Off */
	if (event == NT3H2X11_EVENT_NONE) {
		switch (fd_off) {

		/* Event upon which the signal output on the FD pin is released
		 * 00b: if the field is switched off
		 */
		case NT3H2X11_FD_OFF_RF_OFF:
			if ((fd_edge != 0u) && (flag_fd_pin == 0u)) {
				event = NT3H2X11_EVENT_FD_OFF;
			}
			break;

		/* Event upon which the signal output on the FD pin is released
		 * 01b: if the field is switched off
		 *      or the tag is set to the HALT state
		 */
		case NT3H2X11_FD_OFF_RF_OFF_OR_HALT:
			if ((fd_edge != 0u) && (flag_fd_pin == 0u)) {
				event = NT3H2X11_EVENT_FD_OFF;
			} else {
				/* By elimination of event possibilities: we are halted */
				/* ==> FD_on is checked first, so no other events possible */
				event = NT3H2X11_EVENT_HALTED;
			}
			break;

		/* Event upon which the signal output on the FD pin is released
		 * 10b: if the field is switched off
		 *      or the last page of the NDEF message has been read
		 */
		case NT3H2X11_FD_OFF_RF_OFF_OR_LAST_NDEF_READ:
			if ((fd_edge != 0u) && (flag_fd_pin == 0u)) {
				event = NT3H2X11_EVENT_FD_OFF;
			} else if (flag_ndef_read != 0u) {
				event = NT3H2X11_EVENT_LAST_NDEF_READ;
			} else {
				/* should not happen */
			}
			break;

		/* Event upon which the signal output on the FD pin is released
		 * 11b: (if FD_ON = 11b) if the field is switched off
		 *      or if last data is read by I2C (pthru:NFC>I2C)
		 *      or last data is written by I2C (pthru:I2C>NFC)
		 * 11b: (if FD_ON = 00b or 01b or 10b) if the field is switched off
		 */
		case NT3H2X11_FD_OFF_RF_OFF_OR_LAST_DATA_RW:
			if ((fd_edge != 0u) && (flag_fd_pin == 0u)) {
				event = NT3H2X11_EVENT_FD_OFF;

			} else if (flag_pthru != 0u) {
				/* Only when pass-through is enabled */

				/* 11b: (if FD_ON = 11b) .. */
				if (fd_on == NT3H2X11_FD_ON_RF_ON) {
					if ((dir == NT3H2X11_TRANSFER_DIR_RF_TO_I2C) &&
					    (flag_i2c_locked == 0u)) {
						/* .. if last data is read (pthru:NFC>I2C) */
						event = NT3H2X11_EVENT_LAST_DATA_READ_I2C;

					} else if ((dir != NT3H2X11_TRANSFER_DIR_RF_TO_I2C) &&
						   (flag_nfc_sram_ready != 0u)) {
						/* .. or last data is written (pthru:I2C>NFC) */
						event = NT3H2X11_EVENT_LAST_DATA_WRITTEN_I2C;
					} else {
						/* .. */
					}
				}

			} else {
				/* .. */
			}
			break;

		default:
			break;
		}
	}

	return event;
}

int nt3h2x11_irq_set_callback(const struct device *dev, nt3h2x11_irq_callback_t cb)
{
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;

	data->app_irq_cb = cb;
	return 0;
}

static void _nt3h2x11_irq_cb_worker(struct k_work *worker)
{
	struct nt3h2x11_data *data = CONTAINER_OF(worker, struct nt3h2x11_data, worker_irq);
	const struct device *dev = data->parent;

	uint8_t ns_reg, nc_reg;
	(void)_nt3h2x11_read_csreg_register(dev, NT3H2X11_SESSION, NT3H2X11_REG_NC_REG, &nc_reg);
	(void)_nt3h2x11_read_csreg_register(dev, NT3H2X11_SESSION, NT3H2X11_REG_NS_REG, &ns_reg);
	enum nt3h2x11_event event = _nt3h2x11_reg2event(dev, nc_reg, ns_reg);

	if (data->app_irq_cb != NULL) {
		data->app_irq_cb(dev, event, nc_reg, ns_reg);
	}
}

static void _nt3h2x11_irq_external_cb(const struct device *dev, struct gpio_callback *cb,
				      uint32_t pins)
{
	struct nt3h2x11_data *data = CONTAINER_OF(cb, struct nt3h2x11_data, dev_irq_external_cb);

	/* Push handling to worker */
	if (data->initialized == 1u) {
		k_work_submit(&data->worker_irq);
	}
}

static void _nt3h2x11_irq_internal_cb(const void *param)
{
	const struct device *dev = (const struct device *)param;
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;

#ifdef CONFIG_SOC_SERIES_K32
	/* Toggle INT_INVERT bit to catch opposite FD transition and avoid immediate ISR reentry */
	ASYNC_SYSCON->NFCTAGPADSCTRL ^= ASYNC_SYSCON_NFCTAGPADSCTRL_INT_INVERT_MASK;
#endif

	/* Push handling to worker */
	if (data->initialized == 1u) {
		k_work_submit(&data->worker_irq);
	}
}

/******************* NFC SUBSYS DRIVER IMPLEMENTATION *******************/

static void nt3h2x11_tag_irq_cb(const struct device *dev, enum nt3h2x11_event event, uint8_t nc_reg,
				uint8_t ns_reg)
{
	ARG_UNUSED(nc_reg);
	ARG_UNUSED(ns_reg);

	struct nt3h2x11_data *data = dev->data;
	enum nfc_tag_event nfc_event = NFC_TAG_EVENT_NONE;

	switch (event) {
	case NT3H2X11_EVENT_NONE:
		nfc_event = NFC_TAG_EVENT_NONE;
		break;

	case NT3H2X11_EVENT_FD_OFF:
		nfc_event = NFC_TAG_EVENT_FIELD_OFF;
		break;

	case NT3H2X11_EVENT_FD_ON:
	case NT3H2X11_EVENT_START_OF_COMM:
	case NT3H2X11_EVENT_SELECTED:
		nfc_event = NFC_TAG_EVENT_FIELD_ON;
		break;

	case NT3H2X11_EVENT_HALTED:
		nfc_event = NFC_TAG_EVENT_STOPPED;
		break;

	case NT3H2X11_EVENT_LAST_NDEF_READ:
	case NT3H2X11_EVENT_LAST_DATA_READ_NFC:
		nfc_event = NFC_TAG_EVENT_READ_DONE;
		break;

	case NT3H2X11_EVENT_DATA_READY_I2C:
		nfc_event = NFC_TAG_EVENT_DATA_IND;
		break;

	case NT3H2X11_EVENT_LAST_DATA_READ_I2C:
		nfc_event = NFC_TAG_EVENT_DATA_IND_DONE;
		break;

	case NT3H2X11_EVENT_LAST_DATA_WRITTEN_I2C:
		nfc_event = NFC_TAG_EVENT_DATA_TRANSMITTED;
		break;

	default:
		break;
	}

	if (data->nfc_tag_cb != NULL) {
		data->nfc_tag_cb(dev, nfc_event, NULL, 0u);
	}
}

static int nt3h2x11_tag_init(const struct device *dev, nfc_tag_cb_t cb)
{
	struct nt3h2x11_data *data = dev->data;

	/* overwrite external application callback with NFC-subsystem */
	if (cb != NULL) {
		(void)nt3h2x11_irq_set_callback(dev, nt3h2x11_tag_irq_cb);
		data->nfc_tag_cb = cb;
	}

	/* Default register setup */
	(void)nt3h2x11_set_fd_off(dev, NT3H2X11_SESSION, NT3H2X11_FD_OFF_RF_OFF_OR_LAST_NDEF_READ);
	(void)nt3h2x11_set_fd_on(dev, NT3H2X11_SESSION, NT3H2X11_FD_ON_TAG_SELECTION);

	return 0;
}

static int nt3h2x11_tag_set_type(const struct device *dev, enum nfc_tag_type type)
{
	int rv = 0;
	struct nt3h2x11_data *data = dev->data;

	/* nt3h2x11 only support T2T messages */
	if (type != NFC_TAG_TYPE_T2T) {
		return -ENOTSUP;
	}

	/* Load default settings = T2T */
	struct nt3h2x11_device default_device = NT3H2X11_DEFAULT_DEVICE;

	rv = nt3h2x11_set_device(dev, &default_device);
	if (rv != 0) {
		return rv;
	}

	/* Type is set */
	data->tag_type = type;
	return 0;
}

static int nt3h2x11_tag_get_type(const struct device *dev, enum nfc_tag_type *type)
{
	struct nt3h2x11_data *data = dev->data;
	*type = data->tag_type;
	return 0;
}

static int nt3h2x11_tag_start(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* nt3h2x11 is always active */
	return 0;
}

static int nt3h2x11_tag_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* nt3h2x11 is always active */
	return 0;
}

static int nt3h2x11_tag_set_ndef(const struct device *dev, uint8_t *buf, uint16_t len)
{
	/*
	 * TODO: current implementation assumes a single continues block to write
	 * data to but it is actually split in SECTOR0 and SECTOR1 when using a
	 * 2k config.
	 */

	int rv;
	uint32_t addr = NT3H2X11_BLK_SECTOR0_START * NT3H2X11_BYTES_BLK;

	uint8_t buf_hdr[4];
	uint8_t terminatorTLV = 0xFEu;

	/* write header */
	if (len < 0xFF) {
		buf_hdr[0] = 0x3u;
		buf_hdr[1] = len;
		rv = nt3h2x11_write_bytes(dev, addr, buf_hdr, 2u);
		if (rv != 0) {
			return rv;
		}
		addr += 2u;

	} else {
		buf_hdr[0] = 0x3u;
		buf_hdr[1] = 0xFFu;
		buf_hdr[2] = (uint8_t)((len & 0x0000FF00u) >> 8);
		buf_hdr[3] = (uint8_t)((len & 0x000000FFu) >> 0);
		rv = nt3h2x11_write_bytes(dev, addr, buf_hdr, 4u);
		if (rv != 0) {
			return rv;
		}
		addr += 4u;
	}

	/* write payload */
	rv = nt3h2x11_write_bytes(dev, addr, buf, len);
	if (rv != 0) {
		return rv;
	}
	addr += len;

	/* write terminator */
	rv = nt3h2x11_write_bytes(dev, addr, &terminatorTLV, 1u);
	if (rv != 0) {
		return rv;
	}
	addr += 1u;

	/* update LAST_NDEF_BLOCK */
	uint8_t blk_offset = addr % NT3H2X11_BYTES_BLK;
	uint8_t blk_addr = (addr - blk_offset) / NT3H2X11_BYTES_BLK;

	rv = nt3h2x11_set_last_ndef_blk(dev, NT3H2X11_SESSION, blk_addr);

	return rv;
}

static int nt3h2x11_tag_cmd(const struct device *dev, enum nfc_tag_cmd cmd, uint8_t *buf,
			    uint16_t *buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	return 0;
}

static struct nfc_tag_driver_api _nt3h2x11_driver_api = {
	.init = nt3h2x11_tag_init,
	.set_type = nt3h2x11_tag_set_type,
	.get_type = nt3h2x11_tag_get_type,
	.start = nt3h2x11_tag_start,
	.stop = nt3h2x11_tag_stop,
	.set_ndef = nt3h2x11_tag_set_ndef,
	.cmd = nt3h2x11_tag_cmd,
};

/******************* DEVICE INITIALISATION BY DTS *******************/

/**
 * @brief Initialize NTAG driver as an External IC
 *
 * @param[in] *dev : Pointer to nt3h2x11 device
 * @return         : 0 on success, negative upon error.
 */
static int _nt3h2x11_init_external(const struct device *dev)
{
	int rv = 0;
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;
	const struct nt3h2x11_cfg *cfg = (const struct nt3h2x11_cfg *)dev->config;

	data->dev_irq_external = cfg->irq_gpio.port;
	if (data->dev_irq_external != NULL) {

		rv = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
		if (rv != 0) {
			LOG_ERR("Init IRQ-pin failed, pin:%d, err:%d", cfg->irq_gpio.pin, rv);
			return rv;
		}

		gpio_init_callback(&data->dev_irq_external_cb, _nt3h2x11_irq_external_cb,
				   BIT(cfg->irq_gpio.pin));

		rv = gpio_add_callback(data->dev_irq_external, &data->dev_irq_external_cb);
		if (rv != 0) {
			LOG_ERR("Init IRQ-cb callback, err:%d", rv);
			return rv;
		}

		rv = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_BOTH);
		if (rv != 0) {
			LOG_ERR("Could not configure gpio %d, err: %d", cfg->irq_gpio.pin, rv);
			return rv;
		}

		LOG_DBG("IRQ: GPIO initialised (bus:%s)", cfg->i2c.bus->name);
	}

	return 0;
}

/**
 * @brief Initialize NTAG driver as an Internal IC
 *
 * @param[in] *dev : Pointer to nt3h2x11 device
 * @return         : 0 on success, negative upon error.
 */
static int _nt3h2x11_init_internal(const struct device *dev)
{
	const struct nt3h2x11_cfg *cfg = (const struct nt3h2x11_cfg *)dev->config;

	if (cfg->internal != 0) {
		irq_enable(cfg->irq_gpio.pin);
	}

#ifdef CONFIG_SOC_SERIES_K32
	/* power on nt3h2x11 */
	ASYNC_SYSCON->NFCTAG_VDD = (ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OE_MASK |
				    ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OUT_MASK);
	k_sleep(K_MSEC(300));
#endif

	return 0;
}

/**
 * @brief Initialize NTAG driver IC.
 *
 * @param[in] *dev : Pointer to nt3h2x11 device
 * @return         : 0 on success, negative upon error.
 */
static int _nt3h2x11_init(const struct device *dev)
{
	int rv = 0;
	struct nt3h2x11_data *data = (struct nt3h2x11_data *)dev->data;
	const struct nt3h2x11_cfg *cfg = (const struct nt3h2x11_cfg *)dev->config;

	LOG_DBG("nt3h2x11: init");

	/* setup i2c */
	data->parent = (const struct device *)dev;
	data->dev_i2c = cfg->i2c.bus;

	/* setup FD-edge detection : FD-pin is default low */
	data->flag_fd_pin = 0u;

	if (data->dev_i2c == NULL) {
		LOG_ERR("Init I2C failed, could not bind, %s", cfg->i2c.bus->name);
		return -ENXIO;
	}

	/* setup IO / device as internal or externally connected */
	if (cfg->internal != 0) {
		rv = _nt3h2x11_init_internal(dev);
		if (rv != 0) {
			return rv;
		}
	} else {
		rv = _nt3h2x11_init_external(dev);
		if (rv != 0) {
			return rv;
		}
	}

	/* Init worker to process IRQ-callbacks */
	k_work_init(&data->worker_irq, _nt3h2x11_irq_cb_worker);
	data->initialized = 1u;

	LOG_DBG("nt3h2x11: init OK");

	return rv;
}

#define NT3H2X11_INIT(inst)                                                                        \
	static struct nt3h2x11_data nt3h2x11_data##inst = {0};                                     \
	static const struct nt3h2x11_cfg nt3h2x11_cfg##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupts),                               \
			    (.internal = 1, .irq_gpio = {0},                                       \
			     .irq_pin = DT_INST_IRQ_BY_IDX(inst, 0, irq)),                         \
			    (.internal = 0,                                                        \
			     .irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),           \
			     .irq_pin = 0))};                                                      \
                                                                                                   \
	static int _nt3h2x11_init##inst(const struct device *dev)                                  \
	{                                                                                          \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, interrupts),                                \
			   (IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),                          \
					DT_INST_IRQ_BY_IDX(inst, 0, priority),                     \
					_nt3h2x11_irq_internal_cb, DEVICE_DT_INST_GET(inst), 0);)) \
		return _nt3h2x11_init(dev);                                                        \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, _nt3h2x11_init##inst, NULL, &nt3h2x11_data##inst,              \
			      &nt3h2x11_cfg##inst, POST_KERNEL, CONFIG_NFC_NT3H2X11_INIT_PRIORITY, \
			      &_nt3h2x11_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NT3H2X11_INIT)
