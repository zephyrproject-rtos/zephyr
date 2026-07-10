/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDIO Device Implementation
 *
 * This file implements SDIO device-side functionality including device
 * initialization, capability detection, and function management.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/sd_dev/sdio_dev.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/sd_dev.h>

LOG_MODULE_REGISTER(sdio_dev, LOG_LEVEL_INF);

/**
 * @name SDIO Function Initialization
 * @{
 */

/**
 * @brief Initialize an SDIO function
 *
 * This function initializes a single SDIO function by reading its CIS
 * address and initializing its receive FIFO.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval 0 on success
 * @retval -EINVAL if func is NULL
 */
int sdio_dev_func_init(struct sdio_dev_func *func)
{
	func->cis_addr = sdio_dev_read_cis_addr(func);
	if (func->cis_addr == 0) {
		LOG_WRN("func%d: CIS addr is 0 (check device)", func->fn);
	}

	k_fifo_init(&func->rx_fifo);
	/* clear pending interrupt */
	sdio_dev_func_intr_is_pending(func);

	return 0;
}

/**
 * @brief Deinitialize SDIO function
 *
 * This function deinitializes a single SDIO function by cleaning up
 * the RX FIFO and resetting function-specific parameters.
 *
 * @param func Pointer to the SDIO function structure
 *
 * @retval 0 on success
 * @retval -EINVAL if func pointer is NULL
 */
int sdio_dev_func_deinit(struct sdio_dev_func *func)
{
	sd_dev_pkt_t *rx_pkt = 0;

	if (!func) {
		LOG_ERR("%s: func is NULL", __func__);
		return -EINVAL;
	}

	/* Flush and clean up RX FIFO */
	while ((rx_pkt = k_fifo_get(&func->rx_fifo, K_NO_WAIT)) != NULL) {
		/* Free any remaining data in the FIFO */
		sd_dev_pkt_free(rx_pkt);
	}

	/* Reset function parameters */
	func->cis_addr = 0;
	func->block_size = 0;
	func->func_code = 0;

	LOG_DBG("func%d deinitialized", func->fn);

	return 0;
}

/**
 * @}
 */

/**
 * @name SDIO Device Initialization
 * @{
 */

/**
 * @brief Apply an SDIO configuration field.
 *
 * If the field value is not @ref SDIO_CFG_UNSET, the value is applied
 * using the specified setter function.
 *
 * When the setter returns -EACCES, the current device value is obtained
 * through the getter function and stored back into the configuration
 * structure.
 *
 * @param _cfg Pointer to SDIO device configuration.
 * @param _sdio SDIO device instance.
 * @param _ret Temporary variable used to store return value.
 * @param _field Configuration field name.
 * @param _set Setter function.
 * @param _get Getter function.
 * @param _name Field name used for logging.
 */
#define SDIO_CFG_SET(_cfg, _sdio, _ret, _field, _set, _get, _name) \
	if ((_cfg)->_field != SDIO_CFG_UNSET) { \
		(_ret) = _set((_sdio), (_cfg)->_field); \
		if ((_ret) == -EACCES) { \
			(_cfg)->_field = _get(_sdio); \
			LOG_WRN("No permission to set %s, using default: %d", \
				_name, (_cfg)->_field); \
			(_ret) = 0; \
		} else if ((_ret) < 0) { \
			LOG_ERR("Failed to set %s: %d", _name, (_ret)); \
		} else { \
			LOG_DBG("%s set to: %d", _name, (_cfg)->_field); \
		} \
	}

/**
 * @brief Initialize SDIO configuration
 *
 * This function initializes the SDIO device configuration by setting
 * various parameters if they are specified (non-negative values).
 *
 * @param sdio Pointer to SDIO device structure
 * @param cfg Pointer to SDIO device configuration
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio or cfg is NULL
 * @retval negative errno code on failure
 */
static int sdio_dev_cfg_init(struct sdio_dev *sdio)
{
	struct sdio_dev_cfg *cfg;
	int ret;

	if ((sdio == NULL) || (sdio->cfg == NULL)) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	cfg = sdio->cfg;

	SDIO_CFG_SET(cfg, sdio, ret, cccr_version, sdio_dev_set_cccr_version,
		     sdio_dev_get_cccr_version, "CCCR version");

	SDIO_CFG_SET(cfg, sdio, ret, sdio_version, sdio_dev_set_sdio_version,
		     sdio_dev_get_sdio_version, "SDIO version");

	SDIO_CFG_SET(cfg, sdio, ret, spec_version, sdio_dev_set_spec_version,
		     sdio_dev_get_spec_version, "SD spec version");

	SDIO_CFG_SET(cfg, sdio, ret, bus_width, sdio_dev_set_bus_width, sdio_dev_get_bus_width,
		     "Bus width");

	SDIO_CFG_SET(cfg, sdio, ret, cspi_int, sdio_dev_set_cspi_int, sdio_dev_get_cspi_int,
		     "CSPI interrupt");

	SDIO_CFG_SET(cfg, sdio, ret, multi_block, sdio_dev_set_multi_block,
		     sdio_dev_get_multi_block, "Multi-block");

	SDIO_CFG_SET(cfg, sdio, ret, low_speed, sdio_dev_set_low_speed, sdio_dev_get_low_speed,
		     "Low-speed");

	SDIO_CFG_SET(cfg, sdio, ret, high_speed, sdio_dev_set_high_speed,
		     sdio_dev_support_high_speed, "High-speed");

	SDIO_CFG_SET(cfg, sdio, ret, uhs_mode, sdio_dev_set_uhs, sdio_dev_get_uhs, "UHS mode");

	SDIO_CFG_SET(cfg, sdio, ret, strength, sdio_dev_set_strength, sdio_dev_get_strength,
		     "Drive strength");

	SDIO_CFG_SET(cfg, sdio, ret, async_intr_support, sdio_dev_set_aync_intr,
		     sdio_dev_get_aync_intr, "Async interrupt");

	LOG_INF("SDIO configuration initialized successfully");

	return 0;
}

/**
 * @brief Initialize SDIO function configuration
 *
 * This function initializes the SDIO function configuration by setting
 * various parameters if they are specified (non-zero values).
 *
 * @param func Pointer to SDIO function structure
 * @param func_cfg Pointer to SDIO function configuration
 *
 * @retval 0 on success
 * @retval -EINVAL if func or func_cfg is NULL
 * @retval negative errno code on failure
 */
int sdio_dev_func_cfg_init(struct sdio_dev_func *func, struct sdio_func_cfg *func_cfg)
{
	int ret;
	uint8_t default_fn_code;
	int default_block_size;

	if (func == NULL || func_cfg == NULL) {
		LOG_ERR("Invalid parameters: func=%p, func_cfg=%p", func, func_cfg);
		return -EINVAL;
	}

	/* Set function code if specified */
	if (func_cfg->fn_code > 0) {
		ret = sdio_dev_set_fn_code(func, func_cfg->fn_code);
		if (ret < 0) {
			if (ret == -EACCES) {
				default_fn_code = sdio_dev_get_fn_code(func);
				LOG_WRN("No permission to set function code for fn%d, "
					"using default: 0x%02x",
					func_cfg->fn, default_fn_code);
				func_cfg->fn_code = default_fn_code;
				func->func_code = default_fn_code;
			} else {
				LOG_ERR("Failed to set function code for fn%d: %d", func_cfg->fn,
					ret);
				return ret;
			}
		} else {
			LOG_DBG("Function code for fn%d set to: 0x%02x", func_cfg->fn,
				func_cfg->fn_code);
		}
	}

	/* Set block size if specified */
	if (func_cfg->block_size > 0) {
		ret = sdio_dev_set_block_size(func, func_cfg->block_size);
		if (ret < 0) {
			if (ret == -EACCES) {
				default_block_size = sdio_dev_get_block_size(func);
				LOG_WRN("No permission to set block size for fn%d, "
					"using default: %d",
					func_cfg->fn, default_block_size);
				func_cfg->block_size = default_block_size;
				func->block_size = default_block_size;
			} else {
				LOG_ERR("Failed to set block size for fn%d: %d", func_cfg->fn, ret);
				return ret;
			}
		} else {
			LOG_DBG("Block size for fn%d set to: %d", func_cfg->fn,
				func_cfg->block_size);
		}
	}

	LOG_INF("SDIO function %d configuration initialized successfully", func_cfg->fn);
	return 0;
}

/**
 * @brief Initialize SDIO device
 *
 * This function initializes the SDIO device with the provided configuration.
 * It allocates and initializes all enabled functions, reads device capabilities,
 * and configures the CIS table.
 *
 * @param sdio Pointer to SDIO device structure
 * @param cfg Pointer to SDIO device configuration
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio or cfg is NULL
 * @retval -ENOMEM if memory allocation fails
 * @retval negative errno code on other failures
 */
int sdio_dev_init(struct sdio_dev *sdio, const struct sdio_dev_cfg *cfg)
{
	int ret = 0;
	struct sd_dev_card *card = 0;

	if (!sdio || !cfg) {
		LOG_ERR("%s: sdio or cfg is NULL", __func__);
		return -EINVAL;
	}

	card = sdio->card;

	sdio->cccr_addr = cfg->cccr_addr;
	sdio->num_funcs = 0;
	sdio->cfg = sd_dev_heap_alloc(sizeof(struct sdio_dev_cfg));
	memset(sdio->cfg, 0, sizeof(struct sdio_dev_cfg));
	*sdio->cfg = *cfg;
	sdio_dev_cfg_init(sdio);

	for (int i = 0; i < CONFIG_SDIO_DEV_MAX_FUNCS; i++) {
		if (cfg->func_bitmap & (1 << i)) {
			sdio->num_funcs++;
		}
	}

	for (int i = 0; i < CONFIG_SDIO_DEV_MAX_FUNCS; i++) {
		struct sdio_func_cfg *fcfg = &sdio->cfg->funcs[i];

		if (fcfg->fn == 0) {
			continue;
		}

		if (fcfg->fn < 1 || fcfg->fn > 7) {
			LOG_WRN("%s: invalid fn=%d", __func__, fcfg->fn);
			continue;
		}

		if (!(cfg->func_bitmap & (1 << (fcfg->fn - 1)))) {
			continue;
		}

		struct sdio_dev_func *func = sd_dev_heap_alloc(sizeof(*func));

		if (!func) {
			LOG_ERR("%s: alloc func failed", __func__);
			return -ENOMEM;
		}

		memset(func, 0, sizeof(*func));

		sdio->funcs[fcfg->fn] = func;

		func->sdio = sdio;
		func->fn = fcfg->fn;
		func->fbr_addr = fcfg->fbr_addr;
		sdio_dev_func_cfg_init(func, fcfg);

		LOG_INF("sdio func%d defaule value: block_size=%d func_code=%d", func->fn,
			func->block_size, func->func_code);

		ret = sdio_dev_func_init(func);
		if (ret) {
			LOG_ERR("func%d init failed (%d)", func->fn, ret);
			return ret;
		}
	}

	sdio_dev_cis_table_configurate(sdio);

	return 0;
}

/**
 * @brief Deinitialize SDIO device
 *
 * This function deinitializes the SDIO device by cleaning up all allocated
 * functions, releasing memory, and resetting device state to initial values.
 *
 * @param sdio Pointer to the SDIO device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if sdio pointer is NULL
 */
int sdio_dev_deinit(struct sdio_dev *sdio)
{
	int ret = 0;

	if (!sdio) {
		LOG_ERR("%s: sdio is NULL", __func__);
		return -EINVAL;
	}

	/* Deinitialize all functions */
	for (int i = 0; i < CONFIG_SDIO_DEV_MAX_FUNCS; i++) {
		struct sdio_dev_func *func = sdio->funcs[i];

		if (!func) {
			continue;
		}

		/* Call function-specific deinit if exists */
		ret = sdio_dev_func_deinit(func);
		if (ret) {
			LOG_WRN("func%d deinit failed (%d)", func->fn, ret);
			/* Continue cleanup even if one fails */
		}

		/* Free the function structure */
		sd_dev_heap_free(func);
		sdio->funcs[i] = NULL;
	}

	/* Clear SDIO device fields */
	sdio->priv = NULL;
	sdio->cccr_addr = 0;
	sdio->num_funcs = 0;
	sd_dev_heap_free(sdio->cfg);
	sdio->cfg = 0;

	LOG_INF("sdio device deinitialized");

	return 0;
}

/**
 * @brief Check if SDIO device is enumerated
 *
 * This function checks if any SDIO function is ready, indicating that
 * the device has been enumerated by the host.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if device is enumerated (at least one function is ready)
 * @retval 0 if device is not enumerated or sdio is NULL
 */
int sdio_dev_is_enumed(struct sdio_dev *sdio)
{
	if (!sdio) {
		return 0;
	}

	for (int fn = 0; fn < sdio->num_funcs; fn++) {
		struct sdio_dev_func *func = sdio->funcs[fn + 1];

		if (!func) {
			continue;
		}

		if (sdio_dev_func_is_ready(func)) {
			return 1;
		}
	}

	return 0;
}

/**
 * @}
 */

/**
 * @name SDIO Device APIs
 * @{
 */

/**
 * @brief Configure CIS table for SDIO device
 *
 * This function configures the Card Information Structure (CIS) table
 * for the SDIO device.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int sdio_dev_cis_table_configurate(struct sdio_dev *sdio)
{
	const struct device *dev = sdio->card->dev;

	return sd_dev_cis_table_configurate(dev);
}

/*
 * SDIO Device CCCR (Card Common Control Registers) Management Functions
 */

/**
 * @brief Get CCCR version from SDIO device
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Positive CCCR version number on success
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_cccr_version(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, &val);
	if (ret) {
		return ret;
	}

	return (val & SDIO_CCCR_CCCR_REV_MASK) >> SDIO_CCCR_CCCR_REV_SHIFT;
}

/**
 * @brief Set CCCR version for SDIO device
 *
 * @param sdio Pointer to SDIO device structure
 * @param version CCCR version to set (0-15)
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_cccr_version(struct sdio_dev *sdio, uint8_t version)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	if (version > 0x0F) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, &val);
	if (ret) {
		return ret;
	}

	val &= ~SDIO_CCCR_CCCR_REV_MASK;
	val |= (version << SDIO_CCCR_CCCR_REV_SHIFT) & SDIO_CCCR_CCCR_REV_MASK;

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, &val);
	if (ret) {
		return ret;
	}

	if (((val & SDIO_CCCR_CCCR_REV_MASK) >> SDIO_CCCR_CCCR_REV_SHIFT) != version) {
		return -EACCES;
	}

	return 0;
}

/**
 * @brief Get SDIO specification version
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Positive SDIO version number on success
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_sdio_version(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, &val);
	if (ret) {
		return ret;
	}

	return (val >> 4) & 0x0F;
}

/**
 * @brief Set SDIO specification version
 *
 * @param sdio Pointer to SDIO device structure
 * @param version SDIO version to set (0-15)
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_sdio_version(struct sdio_dev *sdio, int version)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	if (version < 0 || version > 0x0F) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, &val);
	if (ret) {
		return ret;
	}

	val &= 0x0F;
	val |= (version << 4);

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CCCR, &val);
	if (ret) {
		return ret;
	}

	if (((val >> 4) & 0x0F) != version) {
		return -EACCES;
	}

	return 0;
}

/**
 * @brief Set SD specification version
 *
 * @param sdio Pointer to SDIO device structure
 * @param version SD spec version to set (0-15)
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_spec_version(struct sdio_dev *sdio, uint8_t version)
{
	const struct device *dev;
	uint32_t addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	if (version > 0x0F) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	addr = sdio->cccr_addr + SDIO_CCCR_SD;

	ret = sd_dev_read_reg8(dev, addr, &val);
	if (ret) {
		return ret;
	}

	val &= ~0x0F;
	val |= (version & 0x0F);

	ret = sd_dev_write_reg8(dev, addr, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, addr, &val);
	if (ret) {
		return ret;
	}

	if ((val & 0x0F) != version) {
		return -EACCES;
	}

	return 0;
}

/**
 * @brief Get SD specification version
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Positive SD spec version on success
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_spec_version(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	addr = sdio->cccr_addr + SDIO_CCCR_SD;

	ret = sd_dev_read_reg8(dev, addr, &val);
	if (ret) {
		return ret;
	}

	return val & 0x0F;
}

/**
 * @brief Get current bus width configuration
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval SDIO_CCCR_BUS_IF_WIDTH_1_BIT for 1-bit mode
 * @retval SDIO_CCCR_BUS_IF_WIDTH_4_BIT for 4-bit mode
 * @retval SDIO_CCCR_BUS_IF_WIDTH_8_BIT for 8-bit mode
 * @retval -1 on error
 */
int sdio_dev_get_bus_width(struct sdio_dev *sdio)
{
	const struct device *dev = sdio->card->dev;
	uint32_t cccr_addr = sdio->cccr_addr;
	uint32_t read_addr = cccr_addr + SDIO_CCCR_BUS_IF;
	uint8_t read_value = 0;

	sd_dev_read_reg8(dev, read_addr, &read_value);

	switch (read_value & SDIO_CCCR_BUS_IF_WIDTH_MASK) {
	case 0x00:
		return SDIO_CCCR_BUS_IF_WIDTH_1_BIT;
	case 0x02:
		return SDIO_CCCR_BUS_IF_WIDTH_4_BIT;
	default:
		return SDIO_CCCR_BUS_IF_WIDTH_8_BIT;
	}
}

/**
 * @brief Check if master interrupt is enabled
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval true if master interrupt is enabled
 * @retval false if disabled or on error
 */
bool sdio_dev_master_intr_is_enable(const struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return false;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_INT_EN, &val);
	if (ret) {
		return false;
	}

	return (val & BIT(0)) ? true : false;
}

/**
 * @brief Set bus width for SDIO device
 *
 * @param sdio Pointer to SDIO device structure
 * @param width Bus width (1 or 4 bits)
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_bus_width(struct sdio_dev *sdio, uint8_t width)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val, new_width;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	switch (width) {
	case 1:
		new_width = SDIO_CCCR_BUS_IF_WIDTH_1_BIT;
		break;
	case 4:
		new_width = SDIO_CCCR_BUS_IF_WIDTH_4_BIT;
		break;
	default:
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_BUS_IF, &val);
	if (ret) {
		return ret;
	}

	val &= ~SDIO_CCCR_BUS_IF_WIDTH_MASK;
	val |= new_width;

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_BUS_IF, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_BUS_IF, &val);
	if (ret) {
		return ret;
	}

	if ((val & SDIO_CCCR_BUS_IF_WIDTH_MASK) != new_width) {
		return -EACCES;
	}

	return 0;
}

/**
 * @brief Set CSPI interrupt mode
 *
 * @param sdio Pointer to SDIO device structure
 * @param cspi_int Enable (non-zero) or disable (0) CSPI interrupt
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_cspi_int(struct sdio_dev *sdio, int cspi_int)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_BUS_IF, &val);
	if (ret) {
		return ret;
	}

	if (cspi_int) {
		val |= SDIO_CCCR_BUS_IF_ECSI;
	} else {
		val &= ~SDIO_CCCR_BUS_IF_ECSI;
	}

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_BUS_IF, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_BUS_IF, &val);
	if (ret) {
		return ret;
	}

	if (cspi_int) {
		if (!(val & SDIO_CCCR_BUS_IF_ECSI)) {
			return -EACCES;
		}
	} else {
		if (val & SDIO_CCCR_BUS_IF_ECSI) {
			return -EACCES;
		}
	}

	return 0;
}

/**
 * @brief Get CSPI interrupt mode status
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if CSPI interrupt is enabled
 * @retval 0 if disabled
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_cspi_int(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_BUS_IF, &val);
	if (ret) {
		return ret;
	}

	return (val & SDIO_CCCR_BUS_IF_ECSI) ? 1 : 0;
}

/**
 * @brief Get multi-block support capability
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if multi-block is supported
 * @retval 0 if not supported
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_multi_block(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, &val);
	if (ret) {
		return ret;
	}

	return (val & SDIO_CCCR_CAPS_SMB) ? 1 : 0;
}

/**
 * @brief Set multi-block transfer mode
 *
 * @param sdio Pointer to SDIO device structure
 * @param multi_block Enable (non-zero) or disable (0) multi-block
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_multi_block(struct sdio_dev *sdio, int multi_block)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, &val);
	if (ret) {
		return ret;
	}

	if (multi_block) {
		val |= SDIO_CCCR_CAPS_SMB;
	} else {
		val &= ~SDIO_CCCR_CAPS_SMB;
	}

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, &val);
	if (ret) {
		return ret;
	}

	if (multi_block) {
		if (!(val & SDIO_CCCR_CAPS_SMB)) {
			return -EACCES;
		}
	} else {
		if (val & SDIO_CCCR_CAPS_SMB) {
			return -EACCES;
		}
	}

	return 0;
}

/**
 * @brief Get low speed card capability
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if low speed card
 * @retval 0 if not low speed
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_low_speed(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, &val);
	if (ret) {
		return ret;
	}

	return (val & SDIO_CCCR_CAPS_LSC) ? 1 : 0;
}

/**
 * @brief Set low speed card mode
 *
 * @param sdio Pointer to SDIO device structure
 * @param low_speed Enable (non-zero) or disable (0) low speed mode
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_low_speed(struct sdio_dev *sdio, int low_speed)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, &val);
	if (ret) {
		return ret;
	}

	if (low_speed) {
		val |= SDIO_CCCR_CAPS_LSC;
	} else {
		val &= ~SDIO_CCCR_CAPS_LSC;
	}

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_CAPS, &val);
	if (ret) {
		return ret;
	}

	if (low_speed) {
		if (!(val & SDIO_CCCR_CAPS_LSC)) {
			return -EACCES;
		}
	} else {
		if (val & SDIO_CCCR_CAPS_LSC) {
			return -EACCES;
		}
	}

	return 0;
}

/**
 * @brief Check if high speed is supported
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if high speed is supported
 * @retval 0 if not supported
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_support_high_speed(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_SPEED, &val);
	if (ret) {
		return ret;
	}

	return (val & SDIO_CCCR_SPEED_SHS) ? 1 : 0;
}

/**
 * @brief Set high speed mode
 *
 * @param sdio Pointer to SDIO device structure
 * @param high_speed High speed mode value (0-7)
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_high_speed(struct sdio_dev *sdio, uint8_t high_speed)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	if (high_speed > 0x07) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_SPEED, &val);
	if (ret) {
		return ret;
	}

	val &= ~SDIO_CCCR_SPEED_MASK;
	val |= (high_speed << SDIO_CCCR_SPEED_SHIFT) & SDIO_CCCR_SPEED_MASK;

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_SPEED, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_SPEED, &val);
	if (ret) {
		return ret;
	}

	if (((val & SDIO_CCCR_SPEED_MASK) >> SDIO_CCCR_SPEED_SHIFT) != high_speed) {
		return -EACCES;
	}

	return 0;
}

/**
 * @brief Get UHS mode
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Positive UHS mode value on success
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_uhs(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_UHS, &val);
	if (ret) {
		return ret;
	}

	return val & 0x07;
}

/**
 * @brief Set UHS mode
 *
 * @param sdio Pointer to SDIO device structure
 * @param uhs_mode UHS mode value (0-7)
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed
 * @retval Negative error code on failure
 */
int sdio_dev_set_uhs(struct sdio_dev *sdio, uint8_t uhs_mode)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	if (uhs_mode > 7) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_UHS, &val);
	if (ret) {
		return ret;
	}

	val &= ~0x07;
	val |= (uhs_mode & 0x07);

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_UHS, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_UHS, &val);
	if (ret) {
		return ret;
	}

	if ((val & 0x07) != uhs_mode) {
		return -EACCES;
	}

	return 0;
}

/**
 * @brief Get current bus speed
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Bus speed selection value
 */
int sdio_dev_get_bus_speed(struct sdio_dev *sdio)
{
	const struct device *dev = sdio->card->dev;
	uint32_t addr = sdio->cccr_addr + SDIO_CCCR_SPEED;
	uint8_t val;
	uint8_t bss;

	if (sd_dev_read_reg8(dev, addr, &val)) {
		return SDIO_CCCR_SPEED_SHS;
	}

	if (!(val & SDIO_CCCR_SPEED_SHS)) {
		return SDIO_CCCR_SPEED_SHS;
	}

	bss = (val & SDIO_CCCR_SPEED_MASK) >> SDIO_CCCR_SPEED_SHIFT;

	return bss;
}

/**
 * @brief Get driver strength setting
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval Positive driver strength value on success
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on failure
 */
int sdio_dev_get_strength(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_DRIVE_STRENGTH, &val);
	if (ret) {
		return ret;
	}

	return val & 0x07;
}

/**
 * @brief Set driver strength
 *
 * This function configures the driver strength for the SDIO device by
 * writing to the CCCR Drive Strength register and verifying the setting.
 *
 * @param sdio Pointer to SDIO device structure
 * @param strength Driver strength value (0-7)
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters or strength value out of range
 * @retval -EACCES if verification failed after write
 * @retval Negative error code on register access failure
 */
int sdio_dev_set_strength(struct sdio_dev *sdio, uint8_t strength)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	if (strength > 7) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_DRIVE_STRENGTH, &val);
	if (ret) {
		return ret;
	}

	val &= ~0x07;
	val |= strength;

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_DRIVE_STRENGTH, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_DRIVE_STRENGTH, &val);
	if (ret) {
		return ret;
	}

	if ((val & 0x07) != strength) {
		return -EACCES;
	}

	return 0;
}

/**
 * @brief Set asynchronous interrupt mode
 *
 * This function enables or disables asynchronous interrupt support for
 * the SDIO device by configuring the CCCR Interrupt Extension register.
 *
 * @param sdio Pointer to SDIO device structure
 * @param aync_int Enable (non-zero) or disable (0) asynchronous interrupt
 *
 * @retval 0 on success
 * @retval -EINVAL if invalid parameters
 * @retval -EACCES if verification failed after write
 * @retval Negative error code on register access failure
 */
int sdio_dev_set_aync_intr(struct sdio_dev *sdio, int aync_int)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_INT_EXT, &val);
	if (ret) {
		return ret;
	}

	if (aync_int) {
		val |= SDIO_CCCR_INT_EXT_EAI;
	} else {
		val &= ~SDIO_CCCR_INT_EXT_EAI;
	}

	ret = sd_dev_write_reg8(dev, cccr_addr + SDIO_CCCR_INT_EXT, val);
	if (ret) {
		return ret;
	}

	/* Verify the write operation */
	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_INT_EXT, &val);
	if (ret) {
		return ret;
	}

	if (aync_int) {
		if (!(val & SDIO_CCCR_INT_EXT_EAI)) {
			return -EACCES;
		}
	} else {
		if (val & SDIO_CCCR_INT_EXT_EAI) {
			return -EACCES;
		}
	}

	return 0;
}

/**
 * @brief Get asynchronous interrupt support status
 *
 * This function checks if asynchronous interrupt is supported by reading
 * the CCCR Interrupt Extension register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval 1 if asynchronous interrupt is supported
 * @retval 0 if not supported
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on register access failure
 */
int sdio_dev_get_aync_intr(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return -EINVAL;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_INT_EXT, &val);
	if (ret) {
		return ret;
	}

	return (val & SDIO_CCCR_INT_EXT_SAI) ? 1 : 0;
}

/**
 * @brief Check if asynchronous interrupt is enabled
 *
 * This function checks if asynchronous interrupt mode is currently enabled
 * by reading the CCCR Interrupt Extension register.
 *
 * @param sdio Pointer to SDIO device structure
 *
 * @retval true if asynchronous interrupt is enabled
 * @retval false if disabled or on error
 */
bool sdio_dev_aync_intr_is_enabled(struct sdio_dev *sdio)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!sdio || !sdio->card || !sdio->card->dev) {
		return false;
	}

	dev = sdio->card->dev;
	cccr_addr = sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_INT_EXT, &val);
	if (ret) {
		return false;
	}

	return (val & SDIO_CCCR_INT_EXT_EAI) ? true : false;
}

/**
 * @}
 */

/**
 * @name SDIO Function APIs
 * @{
 */

/**
 * @brief Check if SDIO function is enabled
 *
 * This function checks the I/O Enable register in CCCR to determine
 * if the specified function is enabled.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval true if function is enabled
 * @retval false if function is disabled or on error
 */
bool sdio_dev_func_is_enabled(const struct sdio_dev_func *func)
{
	const struct device *dev = func->sdio->card->dev;
	uint8_t fn = func->fn;
	uint32_t cccr_addr = func->sdio->cccr_addr;
	uint32_t enable_addr = cccr_addr + SDIO_CCCR_IO_EN;
	uint8_t val;
	int ret;

	ret = sd_dev_read_reg8(dev, enable_addr, &val);
	if (ret) {
		return false;
	}

	if (val & (1U << fn)) {
		return true;
	}

	return false;
}

/**
 * @brief Check if SDIO function is ready
 *
 * This function checks the I/O Ready register in CCCR to determine
 * if the specified function is ready for I/O operations.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval true if function is ready
 * @retval false if function is not ready or on error
 */
bool sdio_dev_func_is_ready(const struct sdio_dev_func *func)
{
	const struct device *dev = func->sdio->card->dev;
	uint8_t fn = func->fn;
	uint32_t cccr_addr = func->sdio->cccr_addr;
	uint32_t ready_addr = cccr_addr + SDIO_CCCR_IO_RD;
	uint8_t val;
	int ret;

	ret = sd_dev_read_reg8(dev, ready_addr, &val);
	if (ret) {
		return false;
	}

	if (val & (1U << fn)) {
		return true;
	}

	return false;
}

/**
 * @brief Check if SDIO function interrupt is enabled
 *
 * This function checks the CCCR Interrupt Enable register to determine
 * if interrupts are enabled for the specified function.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval 1 if function interrupt is enabled
 * @retval 0 if disabled
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on register access failure
 */
int sdio_dev_func_intr_is_enable(struct sdio_dev_func *func)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!func || !func->sdio || !func->sdio->card || !func->sdio->card->dev) {
		return -EINVAL;
	}

	dev = func->sdio->card->dev;
	cccr_addr = func->sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_INT_EN, &val);
	if (ret) {
		return ret;
	}

	return (val & BIT(func->fn)) ? 1 : 0;
}

/**
 * @brief Check if SDIO function interrupt is pending
 *
 * This function checks the CCCR Interrupt Pending register to determine
 * if there is a pending interrupt for the specified function.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval 1 if function interrupt is pending
 * @retval 0 if no interrupt pending
 * @retval -EINVAL if invalid parameters
 * @retval Negative error code on register access failure
 */
int sdio_dev_func_intr_is_pending(struct sdio_dev_func *func)
{
	const struct device *dev;
	uint32_t cccr_addr;
	uint8_t val;
	int ret;

	if (!func || !func->sdio || !func->sdio->card || !func->sdio->card->dev) {
		return -EINVAL;
	}

	dev = func->sdio->card->dev;
	cccr_addr = func->sdio->cccr_addr;

	ret = sd_dev_read_reg8(dev, cccr_addr + SDIO_CCCR_INT_P, &val);
	if (ret) {
		return ret;
	}

	return (val & BIT(func->fn)) ? 1 : 0;
}

/**
 * @brief Get function block size
 *
 * This function reads the block size for the specified function from
 * the FBR (Function Basic Register).
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval Block size in bytes on success
 * @retval -EINVAL if dev is NULL
 * @retval negative errno code on other failures
 */
int sdio_dev_get_block_size(struct sdio_dev_func *func)
{
	const struct device *dev = func->sdio->card->dev;
	uint32_t fbr_addr = func->fbr_addr;
	uint32_t addr = fbr_addr + SDIO_FBR_BLK_SIZE;
	uint8_t lsb;
	uint8_t msb;
	int ret = 0;
	int block_size = 0;

	if (!dev) {
		return -EINVAL;
	}

	ret = sd_dev_read_reg8(dev, addr, &lsb);
	if (ret) {
		return ret;
	}

	ret = sd_dev_read_reg8(dev, addr + 1, &msb);
	if (ret) {
		return ret;
	}

	block_size = ((uint16_t)msb << 8) | lsb;

	return block_size;
}

/**
 * @brief Set function block size
 *
 * This function sets the block size for the specified function in
 * the FBR (Function Basic Register) and verifies the change.
 *
 * @param func Pointer to SDIO function structure
 * @param block_size Block size to set (in bytes)
 *
 * @retval 0 on success
 * @retval -EINVAL if func is NULL or dev is invalid
 * @retval -EACCES if verification fails
 * @retval negative errno code on other failures
 */
int sdio_dev_set_block_size(struct sdio_dev_func *func, uint16_t block_size)
{
	const struct device *dev;
	uint32_t fbr_addr;
	uint32_t addr;
	uint8_t lsb;
	uint8_t msb;
	int ret;
	int verify_size;

	if (func == NULL || func->sdio == NULL || func->sdio->card == NULL) {
		return -EINVAL;
	}

	dev = func->sdio->card->dev;
	if (!dev) {
		return -EINVAL;
	}

	fbr_addr = func->fbr_addr;
	addr = fbr_addr + SDIO_FBR_BLK_SIZE;

	/* Split block_size into LSB and MSB */
	lsb = block_size & 0xFF;
	msb = (block_size >> 8) & 0xFF;

	/* Write LSB */
	ret = sd_dev_write_reg8(dev, addr, lsb);
	if (ret) {
		return ret;
	}

	/* Write MSB */
	ret = sd_dev_write_reg8(dev, addr + 1, msb);
	if (ret) {
		return ret;
	}

	/* Verify the written value */
	verify_size = sdio_dev_get_block_size(func);
	if (verify_size < 0) {
		return verify_size;
	}

	if (verify_size != block_size) {
		return -EACCES;
	}

	/* Update the function structure */
	func->block_size = block_size;

	return 0;
}

/**
 * @brief Set function code
 *
 * This function writes the function code (type identifier) to the
 * FBR register. Function 0 cannot have its code set.
 *
 * @param func Pointer to SDIO function structure
 * @param fn_code Function code to set
 *
 * @retval 0 on success
 * @retval -EINVAL if func->fn is 0
 * @retval negative errno code on write failure
 */
int sdio_dev_set_fn_code(struct sdio_dev_func *func, uint8_t fn_code)
{
	const struct device *dev = func->sdio->card->dev;
	uint32_t addr = func->fbr_addr + SDIO_FBR_CODE;
	int ret;

	if (func->fn == 0) {
		return -EINVAL;
	}

	ret = sd_dev_write_reg8(dev, addr, fn_code);
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * @brief Read function code
 *
 * This function reads the function code (type identifier) from the
 * FBR register.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval Function code value on success
 * @retval 0 on failure
 */
uint8_t sdio_dev_get_fn_code(const struct sdio_dev_func *func)
{
	const struct device *dev = func->sdio->card->dev;
	uint32_t addr = func->fbr_addr + SDIO_FBR_CODE;
	uint8_t val;

	if (sd_dev_read_reg8(dev, addr, &val)) {
		return 0;
	}

	return val;
}

/**
 * @brief Read CIS address
 *
 * This function reads the 24-bit Card Information Structure (CIS)
 * pointer from the FBR registers.
 *
 * @param func Pointer to SDIO function structure
 *
 * @retval CIS address on success
 * @retval Error code on failure (returned as uint32_t)
 */
uint32_t sdio_dev_read_cis_addr(struct sdio_dev_func *func)
{
	const struct device *dev = func->sdio->card->dev;
	uint32_t base = func->fbr_addr;
	uint8_t lsb, mid, msb;
	int ret;
	uint32_t cis_addr = 0;

	ret = sd_dev_read_reg8(dev, base + SDIO_FBR_CIS, &lsb);
	if (ret) {
		return ret;
	}

	ret = sd_dev_read_reg8(dev, base + SDIO_FBR_CIS + 1, &mid);
	if (ret) {
		return ret;
	}

	ret = sd_dev_read_reg8(dev, base + SDIO_FBR_CIS + 2, &msb);
	if (ret) {
		return ret;
	}

	cis_addr = ((uint32_t)msb << 16) | ((uint32_t)mid << 8) | (uint32_t)lsb;

	return cis_addr;
}

/**
 * @}
 */
