/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FLASH_MSPI_NOR_QUIRKS_INFINEON_H__
#define __FLASH_MSPI_NOR_QUIRKS_INFINEON_H__

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/mspi_cadence.h>
#include "flash_mspi_nor.h"
#include "spi_nor_s28hx512t.h"

#define S28HX512T_OCMD_READ_DDR (0xEE)
#define S28HX512T_OCMD_READ_SDR (0xEC)

#if defined(CONFIG_FLASH_MSPI_INFINEON_S28HX512T_EARLY_FIXUP_RESET) && !defined(WITH_SOFT_RESET)
#error "Early Fixup Reset requires software reset to be enabled, see initial-soft-reset"
#endif

static bool infineon_s28hx512t_is_register_volatile(uint32_t reg)
{
	return reg >= S28HX512T_SPI_NOR_STR1V_ADDR;
}

static int infineon_s28hx512t_read_register(const struct device *dev, uint32_t reg, uint8_t *value)
{
	struct flash_mspi_nor_data *dev_data = dev->data;

	set_up_xfer_with_addr(dev, MSPI_RX, reg);

	if (infineon_s28hx512t_is_register_volatile(reg)) {
		dev_data->xfer.rx_dummy = dev_data->cmd_info.rdsr_dummy;
	} else {
		uint8_t read_dummy_cycles = dev_data->cmd_info.read_dummy_cycles;

		dev_data->xfer.rx_dummy = (read_dummy_cycles != 0) ? read_dummy_cycles : 8;
	}

	dev_data->packet.num_bytes = 1;
	dev_data->packet.data_buf = value;

	return perform_xfer(dev, S28HX512T_SPI_NOR_CMD_RREG);
}

static int infineon_s28hx512t_write_register(const struct device *dev, uint32_t reg, uint8_t value)
{
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rv = 0;

	rv = cmd_wren(dev);
	if (rv < 0) {
		return rv;
	}

	set_up_xfer_with_addr(dev, MSPI_TX, reg);
	dev_data->packet.data_buf = &value;
	dev_data->packet.num_bytes = 1;

	return perform_xfer(dev, S28HX512T_SPI_NOR_CMD_WR_WRARG);
}

static int infineon_s28hx512t_disable_hybrid_sector(const struct device *dev)
{
	uint8_t conf3 = 0;
	int rc = 0;

	rc = infineon_s28hx512t_read_register(dev, S28HX512T_SPI_NOR_CFR3V_ADDR, &conf3);
	if (rc < 0) {
		LOG_ERR("Error reading volatile configuration register 3");
		return rc;
	}

	if ((conf3 & BIT(3)) == 0) {
		LOG_INF("Flash is in hybrid sector mode. Changing non-volatile config to correct "
			"this");

		conf3 |= BIT(3);

		rc = infineon_s28hx512t_write_register(dev, S28HX512T_SPI_NOR_CFR3N_ADDR, conf3);
		if (rc < 0) {
			LOG_ERR("Error changing non-volatile configuration of flash");
			return rc;
		}

		rc = wait_until_ready(dev, K_MSEC(S28HX512T_SPI_NOR_NV_WRITE_MAX_MSEC));
		if (rc < 0) {
			LOG_ERR("Error waiting for flash to enter idle after disabling hybrid "
				"sector mode by writing non volatile register");
			return rc;
		}
	}

	return 0;
}

#ifdef CONFIG_FLASH_MSPI_INFINEON_S28HX512T_EARLY_FIXUP_RESET
static int infineon_s28hx512t_early_fixup(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc;
	struct mspi_dev_cfg boot_cfg = {
		.io_mode = MSPI_IO_MODE_OCTAL,
		.data_rate = MSPI_DATA_RATE_DUAL,
		.cmd_length = 2,
	};

	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			     MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CMD_LEN |
				     MSPI_DEVICE_CONFIG_DATA_RATE,
			     &boot_cfg);
	if (rc < 0) {
		LOG_ERR("%s: dev_config() failed: %d", __func__, rc);
		return rc;
	}
	dev_data->last_applied_cfg = &boot_cfg;
	dev_data->cmd_info.cmd_extension = CMD_EXTENSION_SAME;

	rc = soft_reset_66_99(dev);
	if (rc < 0) {
		return rc;
	}

	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id,
			     MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CMD_LEN |
				     MSPI_DEVICE_CONFIG_DATA_RATE,
			     &dev_config->mspi_control_cfg);
	if (rc < 0) {
		LOG_ERR("%s: dev_config() failed: %d", __func__, rc);
		return rc;
	}
	dev_data->last_applied_cfg = &dev_config->mspi_control_cfg;
	dev_data->cmd_info.cmd_extension = CMD_EXTENSION_NONE;
	dev_data->cmd_info.uses_4byte_addr = false;

	return 0;
}
#endif /* CONFIG_FLASH_MSPI_INFINEON_S28HX512T_EARLY_FIXUP_RESET */

static int infineon_s28hx512t_pre_init(const struct device *dev)
{
	int rc;
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	rc = infineon_s28hx512t_disable_hybrid_sector(dev);
	if (rc < 0) {
		return rc;
	}

	/* 256k sector erase type */
	memset(dev_data->erase_types, 0, sizeof(dev_data->erase_types));
	dev_data->erase_types[0] = (struct jesd216_erase_type){
		.cmd = 0xDC,
		.exp = 18,
	};

	/* enter 4 byte addressing mode if configured to use 4 byte addressing */
	if (dev_config->mspi_nor_cfg.io_mode == MSPI_IO_MODE_SINGLE) {
		if (dev_config->mspi_nor_cfg.addr_length == 4) {
			dev_data->cmd_info.uses_4byte_addr = true;
		}

		if (dev_data->cmd_info.uses_4byte_addr == true) {
			dev_data->switch_info.enter_4byte_addr = ENTER_4BYTE_ADDR_B7;
		}
	}

	/* set cmd extension for 2 byte opcodes for octal mode */
	dev_data->cmd_info.cmd_extension = CMD_EXTENSION_SAME;

	return 0;
}

static int infineon_s28hx512t_switch_octal(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	const struct mspi_dev_cfg *mspi_nor_cfg = &dev_config->mspi_nor_cfg;
	struct flash_mspi_nor_data *dev_data = dev->data;
	const uint8_t memlat[16] = {5, 6, 8, 10, 12, 14, 16, 18, 20, 22, 23, 24, 25, 26, 27, 28};
	const uint8_t vrglat[4] = {3, 4, 5, 6};
	uint8_t read_dummy = 0;
	uint8_t cmd_dummy = 0;
	uint8_t cfg_reg;
	int rc = 0;

	if (mspi_nor_cfg->cmd_length != 2) {
		LOG_ERR("Octal mode requires 2 byte command length");
		return -EINVAL;
	}

	rc = infineon_s28hx512t_read_register(dev, S28HX512T_SPI_NOR_CFR2V_ADDR, &cfg_reg);
	if (rc != 0) {
		return rc;
	}
	read_dummy = memlat[FIELD_GET(GENMASK(3, 0), cfg_reg)];

	rc = infineon_s28hx512t_read_register(dev, S28HX512T_SPI_NOR_CFR3V_ADDR, &cfg_reg);
	if (rc != 0) {
		return rc;
	}
	cmd_dummy = vrglat[FIELD_GET(GENMASK(7, 6), cfg_reg)];

	rc = infineon_s28hx512t_read_register(dev, S28HX512T_SPI_NOR_CFR5V_ADDR, &cfg_reg);
	if (rc != 0) {
		return rc;
	}

	cfg_reg |= S28HX512T_SPI_NOR_CFR5X_OPI_IT;
	if (mspi_nor_cfg->data_rate == MSPI_DATA_RATE_SINGLE) {
		cfg_reg &= ~S28HX512T_SPI_NOR_CFR5X_SDRDDR;
		dev_data->cmd_info.read_cmd = S28HX512T_OCMD_READ_SDR;
	} else if (mspi_nor_cfg->data_rate == MSPI_DATA_RATE_DUAL) {
		cfg_reg |= S28HX512T_SPI_NOR_CFR5X_SDRDDR;
		dev_data->cmd_info.read_cmd = S28HX512T_OCMD_READ_DDR;
	} else {
		LOG_ERR("data rate not supported");
		return -ENOTSUP;
	}

	rc = infineon_s28hx512t_write_register(dev, S28HX512T_SPI_NOR_CFR5V_ADDR, cfg_reg);
	if (rc != 0) {
		return rc;
	}

	dev_data->cmd_info.pp_cmd = SPI_NOR_CMD_PP_4B;
	dev_data->cmd_info.uses_4byte_addr = true;
	dev_data->cmd_info.read_mode_bit_cycles = 0;
	dev_data->cmd_info.read_dummy_cycles = read_dummy;
	dev_data->cmd_info.rdid_dummy = cmd_dummy;
	dev_data->cmd_info.rdid_addr_4 = true;
	dev_data->cmd_info.rdsr_dummy = cmd_dummy;
	dev_data->cmd_info.rdsr_addr_4 = true;
	dev_data->cmd_info.sfdp_addr_4 = true;

	/* configure dual byte opcode on the controller explicitly*/
	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id, MSPI_DEVICE_CONFIG_CMD_LEN,
			     mspi_nor_cfg);
	if (rc < 0) {
		LOG_ERR("failed to configure MSPI controller for command length");
		return rc;
	}

	return 0;
}

static int infineon_s28hx512t_post_switch(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;

	switch (dev_config->mspi_nor_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE: {
		/* Opcodes 0x03 and 0x13 only read for speeds <= 50 MHz, and use 0 read dummy
		 * cycles. Opcodes 0x0B and 0x0C read for speeds > 50 MHz, and use 8 read dummy
		 * cycles by default.
		 */
		if (dev_config->mspi_nor_cfg.freq <= MHZ(50)) {
			dev_data->cmd_info.read_cmd = dev_data->cmd_info.uses_4byte_addr
							      ? SPI_NOR_CMD_READ_4B
							      : SPI_NOR_CMD_READ;
			dev_data->cmd_info.read_dummy_cycles = 0;
		}

		if (dev_data->cmd_info.uses_4byte_addr == false) {
			LOG_WRN("page programming is only supported for 4byte addressing mode");
		}

		return 0;
	}
	case MSPI_IO_MODE_OCTAL: {
		return infineon_s28hx512t_switch_octal(dev);
	}
	default: {
		return -EINVAL;
	}
	}

	return 0;
}

#define FLASH_HAS_CADENCE_PARENT(node, ...)                                                        \
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_PARENT(node), cdns_mspi_controller), (1))

#define INFINEON_HAS_CADENCE_PARENT                                                                \
	UTIL_NOT(IS_EMPTY(                                                                         \
		DT_FOREACH_STATUS_OKAY_VARGS(infineon_s28hx512t, FLASH_HAS_CADENCE_PARENT)))

#if INFINEON_HAS_CADENCE_PARENT
static int infineon_s28hx512t_configure_rd_delay_cadence_mspi(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	uint8_t id[JESD216_READ_ID_LEN] = {0};
	uint8_t read_delay = 0;
	uint8_t max = 0xff;
	uint8_t min = 0x00;
	int rc = 0;

	for (read_delay = 0xf; read_delay >= 0x0; read_delay--) {

		rc = mspi_cadence_configure_read_delay(dev_config->bus, read_delay);
		if (rc < 0) {
			LOG_ERR("failed to set read delay");
			return rc;
		}

		rc = read_jedec_id(dev, id);
		if (rc < 0) {
			LOG_ERR("failed to read JEDEC ID: %d", rc);
			return rc;
		}

		if (memcmp(id, dev_config->jedec_id, sizeof(id)) == 0) {
			if (max == 0xff) {
				max = read_delay;
			}
			min = read_delay;
		} else if (max != 0xff) {
			break;
		}
	}

	if (max == 0xff) {
		LOG_ERR("could not find a suitable value to set as read delay");
		return -ENODEV;
	}

	read_delay = (max + min) / 2;
	LOG_INF("setting read delay as 0x%x", read_delay);

	rc = mspi_cadence_configure_read_delay(dev_config->bus, read_delay);
	if (rc < 0) {
		LOG_ERR("failed to set read delay");
		return rc;
	}

	return 0;
}

static int infineon_s28hx512t_post_switch_cadence_mspi(const struct device *dev)
{
	const struct flash_mspi_nor_config *dev_config = dev->config;
	struct flash_mspi_nor_data *dev_data = dev->data;
	int rc = 0;

	rc = infineon_s28hx512t_post_switch(dev);
	if (rc != 0) {
		return rc;
	}

	rc = mspi_dev_config(dev_config->bus, &dev_config->mspi_id, NON_XIP_DEV_CFG_MASK,
			     &dev_config->mspi_nor_cfg);
	if (rc < 0) {
		return rc;
	}

	dev_data->last_applied_cfg = &dev_config->mspi_nor_cfg;

	return infineon_s28hx512t_configure_rd_delay_cadence_mspi(dev);
}
#endif /* INFINEON_HAS_CADENCE_PARENT */

#define FLASH_INFINEON_S28HX512T_INST(node)                                                        \
	struct flash_mspi_nor_quirks flash_quirks_infineon_s28hx512t_##node = {                    \
		.pre_init = infineon_s28hx512t_pre_init,					   \
		IF_ENABLED(CONFIG_FLASH_MSPI_INFINEON_S28HX512T_EARLY_FIXUP_RESET,		   \
		(.soft_reset = infineon_s28hx512t_early_fixup,))				   \
		COND_CODE_1(FLASH_HAS_CADENCE_PARENT(node),					   \
		(.post_switch_mode = infineon_s28hx512t_post_switch_cadence_mspi,),		   \
		(.post_switch_mode = infineon_s28hx512t_post_switch,)) };

DT_FOREACH_STATUS_OKAY(infineon_s28hx512t, FLASH_INFINEON_S28HX512T_INST)

#define FLASH_QUIRKS_INFINEON_S28HX512T(node) (&flash_quirks_infineon_s28hx512t_##node)

#endif /*__FLASH_MSPI_NOR_QUIRKS_INFINEON_H__*/
