/*
 * Copyright (c) 2024, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mspi_aps256n

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#if CONFIG_SOC_FAMILY_AMBIQ
#include "mspi_ambiq.h"
typedef struct mspi_ambiq_timing_cfg mspi_timing_cfg;
typedef enum mspi_ambiq_timing_param mspi_timing_param;

#else
typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;
#endif

LOG_MODULE_REGISTER(memc_mspi_aps256, CONFIG_MEMC_LOG_LEVEL);

#define APS256N_VENDOR_ID 0x0D
#define APS256N_DEVICE_ID 0x03

#define MSPI_PSRAM_DDR_GLOBAL_RESET   0xFFFF
#define MSPI_PSRAM_DDR_READ           0x2020
#define MSPI_PSRAM_DDR_WRITE          0xA0A0
#define MSPI_PSRAM_DDR_READ_REGISTER  0x4040
#define MSPI_PSRAM_DDR_WRITE_REGISTER 0xC0C0

enum aps256n_dummy_clock {
	APS256N_DC_8,
	APS256N_DC_10,
	APS256N_DC_12,
	APS256N_DC_14,
	APS256N_DC_16,
	APS256N_DC_18,
	APS256N_DC_20,
	APS256N_DC_22,
};

struct memc_mspi_aps256n_config {
	uint32_t port;
	uint32_t mem_size;

	const struct device *bus;
	struct mspi_dev_id dev_id;
	struct mspi_dev_cfg tar_dev_cfg;
	struct mspi_xip_cfg tar_xip_cfg;
	struct mspi_scramble_cfg tar_scramble_cfg;

	mspi_timing_cfg tar_timing_cfg;
	mspi_timing_param timing_cfg_mask;

	bool sw_multi_periph;
};

struct memc_mspi_aps256n_data {
	struct mspi_dev_cfg dev_cfg;
	struct mspi_xip_cfg xip_cfg;
	struct mspi_scramble_cfg scramble_cfg;
	mspi_timing_cfg timing_cfg;
	struct mspi_xfer trans;
	struct mspi_xfer_packet packet;

	struct k_sem lock;
	uint16_t vendor_device_id;
	uint32_t device_size_kb;
};

static int memc_mspi_aps256n_command_write(const struct device *psram, uint16_t cmd, uint32_t addr,
					   uint16_t addr_len, uint32_t rx_dummy, uint32_t tx_dummy,
					   uint8_t *wdata, uint32_t length)
{
	const struct memc_mspi_aps256n_config *cfg = psram->config;
	struct memc_mspi_aps256n_data *data = psram->data;
	int ret;

	data->packet.dir = MSPI_TX;
	data->packet.cmd = cmd;
	data->packet.address = addr;
	data->packet.data_buf = wdata;
	data->packet.num_bytes = length;

	data->trans.async = false;
	data->trans.xfer_mode = MSPI_PIO;
	data->trans.rx_dummy = rx_dummy;
	data->trans.tx_dummy = tx_dummy;
	data->trans.cmd_length = 1;
	data->trans.addr_length = addr_len;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI write transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static int memc_mspi_aps256n_command_read(const struct device *psram, uint16_t cmd, uint32_t addr,
					  uint16_t addr_len, uint32_t rx_dummy, uint32_t tx_dummy,
					  uint8_t *rdata, uint32_t length)
{
	const struct memc_mspi_aps256n_config *cfg = psram->config;
	struct memc_mspi_aps256n_data *data = psram->data;
	int ret;

	data->packet.dir = MSPI_RX;
	data->packet.cmd = cmd;
	data->packet.address = addr;
	data->packet.data_buf = rdata;
	data->packet.num_bytes = length;

	data->trans.async = false;
	data->trans.xfer_mode = MSPI_PIO;
	data->trans.rx_dummy = rx_dummy;
	data->trans.tx_dummy = tx_dummy;
	data->trans.cmd_length = 1;
	data->trans.addr_length = addr_len;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static void acquire(const struct device *psram)
{
	const struct memc_mspi_aps256n_config *cfg = psram->config;
	struct memc_mspi_aps256n_data *data = psram->data;

	k_sem_take(&data->lock, K_FOREVER);

	if (cfg->sw_multi_periph) {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL,
				       &data->dev_cfg))
			;
	} else {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_NONE, NULL))
			;
	}
}

static void release(const struct device *psram)
{
	const struct memc_mspi_aps256n_config *cfg = psram->config;
	struct memc_mspi_aps256n_data *data = psram->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port))
		;

	k_sem_give(&data->lock);
}

static int memc_mspi_aps256n_reset(const struct device *psram)
{
	int ret;
	uint32_t pio_buffer = 0;

	LOG_DBG("Return to default mode");
	ret = memc_mspi_aps256n_command_write(psram, MSPI_PSRAM_DDR_GLOBAL_RESET, 0, 4, 0, 0,
					      (uint8_t *)&pio_buffer, 2);

	return ret;
}

static int memc_mspi_aps256n_enter_hex_mode(const struct device *psram)
{
	uint8_t io_mode_reg = 0;
	uint32_t raw_data;
	int ret;

	ret = memc_mspi_aps256n_command_read(psram, MSPI_PSRAM_DDR_READ_REGISTER, 8, 4, 6, 6,
					     (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM MR8 register!\n");
		goto err;
	} else {
		io_mode_reg = (uint8_t)raw_data;
		LOG_DBG("PSRAM Register MR8 = 0x%X\n", io_mode_reg);
		LOG_DBG("PSRAM IO mode = 0x%X\n\n", (io_mode_reg & 0x40) >> 6);
	}

	raw_data = io_mode_reg | 0x40;

	ret = memc_mspi_aps256n_command_write(psram, MSPI_PSRAM_DDR_WRITE_REGISTER, 8, 4, 6, 0,
					      (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to set PSRAM into HEX mode!\n");
		goto err;
	} else {
		LOG_DBG("Set PSRAM into HEX mode\n\n");
	}

err:
	return ret;
}

static int memc_mspi_aps256n_device_init(const struct device *psram)
{
	int ret;
	uint32_t raw_data;
	uint8_t vendor_id_reg = 0;
	uint8_t device_id_reg = 0;
	uint8_t device_id = 0;
	uint8_t rlc_reg = 0;
	struct memc_mspi_aps256n_data *data = psram->data;

	/* Read and set PSRAM Register MR0 */
	LOG_DBG("Read PSRAM Register MR0\n");
	ret = memc_mspi_aps256n_command_read(psram, MSPI_PSRAM_DDR_READ_REGISTER, 0, 4, 6, 6,
					     (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM Register MR0!\n");
		goto err;
	} else {
		rlc_reg = (uint8_t)raw_data;
		LOG_DBG("PSRAM Register MR0 = 0x%02X\n", rlc_reg);
		LOG_DBG("PSRAM Read Latency Code = 0x%02X\n", ((rlc_reg & 0x1C) >> 2) + 3);
	}

	rlc_reg &= 0xC0; /* set latency to 3 (0b000) */
	rlc_reg |= 0x01; /* set PSRAM drive strength (0:Full 1:Half(default) 2:Quarter 3: Eighth) */
	if (data->dev_cfg.dqs_enable == false) {
		rlc_reg |= 0x20; /* set LT to fixed */
		LOG_DBG("Set read latency into fixed type in NON-DQS mode!\n");
	}

	raw_data = rlc_reg;

	ret = memc_mspi_aps256n_command_write(psram, MSPI_PSRAM_DDR_WRITE_REGISTER, 0, 4, 0, 0,
					      (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to write PSRAM Register MR0!\n");
		goto err;
	} else {
		LOG_DBG("Set PSRAM Register MR0 into 0x%02X\n", rlc_reg);
	}

	LOG_DBG("Read PSRAM Read Latency Code\n");
	ret = memc_mspi_aps256n_command_read(psram, MSPI_PSRAM_DDR_READ_REGISTER, 0, 4, 6, 6,
					     (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM Read Latency Code!\n");
		goto err;
	} else {
		rlc_reg = (uint8_t)raw_data;
		LOG_DBG("PSRAM Register MR0 = 0x%02X\n", rlc_reg);
		LOG_DBG("PSRAM Read Latency Code = 0x%02X\n\n", ((rlc_reg & 0x1C) >> 2) + 3);
	}

	/* Read and set PSRAM Write Latency Code */
	LOG_DBG("Read PSRAM Write Latency Code\n");
	ret = memc_mspi_aps256n_command_read(psram, MSPI_PSRAM_DDR_READ_REGISTER, 4, 4, 6, 6,
					     (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM Write Latency Code!\n");
		goto err;
	} else {
		rlc_reg = (uint8_t)raw_data;
		LOG_DBG("PSRAM Register MR4 = 0x%02X\n", rlc_reg);
		LOG_DBG("PSRAM Write Latency Code = 0x%02X\n", ((rlc_reg & 0xE0) >> 5) + 3);
	}

	raw_data = rlc_reg & 0x1F;

	ret = memc_mspi_aps256n_command_write(psram, MSPI_PSRAM_DDR_WRITE_REGISTER, 4, 4, 0, 0,
					      (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to write PSRAM Write Latency Code!\n");
		goto err;
	} else {
		LOG_DBG("Set PSRAM Write Latency Code into 3\n");
	}

	LOG_DBG("Read PSRAM Write Latency Code\n");
	ret = memc_mspi_aps256n_command_read(psram, MSPI_PSRAM_DDR_READ_REGISTER, 4, 4, 6, 6,
					     (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM Write Latency Code!\n");
		goto err;
	} else {
		rlc_reg = (uint8_t)raw_data;
		LOG_DBG("PSRAM Register MR4 = 0x%02X\n", rlc_reg);
		LOG_DBG("PSRAM Write Latency Code = 0x%02X\n\n", ((rlc_reg & 0xE0) >> 5) + 3);
	}

	/* Read PSRAM Vendor ID and Device ID and return status. */
	LOG_DBG("Read PSRAM Vendor ID\n");
	ret = memc_mspi_aps256n_command_read(psram, MSPI_PSRAM_DDR_READ_REGISTER, 1, 4, 6, 6,
					     (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM Vendor ID!\n");
		goto err;
	} else {
		vendor_id_reg = (uint8_t)raw_data;
		LOG_DBG("PSRAM Register MR1 = 0x%X\n", vendor_id_reg);
		if ((vendor_id_reg & 0x1F) == 0xD) {
			LOG_DBG("PSRAM Vendor ID =  01101\n\n");
			data->vendor_device_id = ((vendor_id_reg & 0x1F) << 8) & 0xFF00;
		} else {
			LOG_DBG("Fail to get correct PSRAM Vendor ID!\n\n");
			goto err;
		}
	}

	LOG_DBG("Read PSRAM Device ID\n");
	ret = memc_mspi_aps256n_command_read(psram, MSPI_PSRAM_DDR_READ_REGISTER, 2, 4, 6, 6,
					     (uint8_t *)&raw_data, 4);
	if (ret != 0) {
		LOG_ERR("Failed to read PSRAM Device ID!\n");
		goto err;
	} else {
		device_id_reg = (uint8_t)raw_data;
		device_id = (device_id_reg & 0x18) >> 3;
		data->vendor_device_id |= device_id & 0xFF;
		LOG_DBG("PSRAM Register MR2 = 0x%X\n", device_id_reg);
		LOG_DBG("PSRAM Device ID =  Generation %d\n", device_id + 1);
		if ((device_id_reg & 0x7) == 0x1) {
			data->device_size_kb = 32 / 8 * 1024U;
			LOG_DBG("PSRAM Density =  32Mb\n\n");
		} else if ((device_id_reg & 0x7) == 0x3) {
			data->device_size_kb = 64 / 8 * 1024U;
			LOG_DBG("PSRAM Density =  64Mb\n\n");
		} else if ((device_id_reg & 0x7) == 0x5) {
			data->device_size_kb = 128 / 8 * 1024U;
			LOG_DBG("PSRAM Density =  128Mb\n\n");
		} else if ((device_id_reg & 0x7) == 0x7) {
			data->device_size_kb = 256 / 8 * 1024U;
			LOG_DBG("PSRAM Density =  256Mb\n\n");
		}
	}

err:
	return ret;
}

static int memc_mspi_aps256n_init(const struct device *psram)
{
	const struct memc_mspi_aps256n_config *cfg = psram->config;
	struct memc_mspi_aps256n_data *data = psram->data;
	struct mspi_dev_cfg lcl_dev_cfg = cfg->tar_dev_cfg;
	int ret = 0;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Controller device is not ready");
		ret = -ENODEV;
		goto err;
	}

	switch (cfg->tar_dev_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_HEX:
		break;
	default:
		LOG_ERR("bus mode %d not supported/%u", cfg->tar_dev_cfg.io_mode, __LINE__);
		ret = -EIO;
		goto err;
	}

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_HEX) {
		lcl_dev_cfg.io_mode = MSPI_IO_MODE_OCTAL;
	}

	data->dev_cfg = cfg->tar_dev_cfg;

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &lcl_dev_cfg)) {
		LOG_ERR("Failed to config mspi controller/%u", __LINE__);
		ret = -EIO;
		goto err;
	}

	if (cfg->tar_scramble_cfg.enable) {
		if (mspi_scramble_config(cfg->bus, &cfg->dev_id, &cfg->tar_scramble_cfg)) {
			LOG_ERR("Failed to enable scrambling/%u", __LINE__);
			ret = -EIO;
			goto err;
		}
		data->scramble_cfg = cfg->tar_scramble_cfg;
	}

	if (cfg->tar_xip_cfg.enable) {
		if (mspi_xip_config(cfg->bus, &cfg->dev_id, &cfg->tar_xip_cfg)) {
			LOG_ERR("Failed to enable XIP/%u", __LINE__);
			ret = -EIO;
			goto err;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
	}

	if (memc_mspi_aps256n_reset(psram)) {
		LOG_ERR("Could not reset pSRAM/%u", __LINE__);
		ret = -EIO;
		goto err;
	}

	if (memc_mspi_aps256n_device_init(psram)) {
		LOG_ERR("Could not perform device init/%u", __LINE__);
		ret = -EIO;
		goto err;
	}

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_HEX) {
		if (memc_mspi_aps256n_enter_hex_mode(psram)) {
			LOG_ERR("Could not enter hex mode/%u", __LINE__);
			ret = -EIO;
			goto err;
		}

		lcl_dev_cfg.io_mode = MSPI_IO_MODE_HEX;
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_IO_MODE, &lcl_dev_cfg)) {
		LOG_ERR("Failed to config mspi controller to HEX mode/%u", __LINE__);
		ret = -EIO;
		goto err;
	}

	data->timing_cfg = cfg->tar_timing_cfg;

	release(psram);

err:
	return ret;
}

#if defined(CONFIG_PM_DEVICE)
static int memc_mspi_aps256n_pm_action(const struct device *psram, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		acquire(psram);

		release(psram);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		acquire(psram);

		release(psram);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /** IS_ENABLED(CONFIG_PM_DEVICE) */

#if CONFIG_SOC_FAMILY_AMBIQ
#define MSPI_TIMING_CONFIG(n)                                                                      \
	{                                                                                          \
		.ui8WriteLatency = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 0),                 \
		.ui8TurnAround = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 1),                   \
		.bTxNeg = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 2),                          \
		.bRxNeg = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 3),                          \
		.bRxCap = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 4),                          \
		.ui32TxDQSDelay = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 5),                  \
		.ui32RxDQSDelay = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 6),                  \
		.ui32RXDQSDelayEXT = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 7),               \
	}
#define MSPI_TIMING_CONFIG_MASK(n) DT_INST_PROP(n, ambiq_timing_config_mask)
#else
#define MSPI_TIMING_CONFIG(n)
#define MSPI_TIMING_CONFIG_MASK(n)
#endif

#define MEMC_MSPI_APS256N(n)                                                                       \
	static const struct memc_mspi_aps256n_config memc_mspi_aps256n_config_##n = {              \
		.port = MSPI_PORT(n),                                                              \
		.mem_size = DT_INST_PROP(n, size) / 8,                                             \
		.bus = DEVICE_DT_GET(DT_INST_BUS(n)),                                              \
		.dev_id = MSPI_DEVICE_ID_DT_INST(n),                                               \
		.tar_dev_cfg = MSPI_DEVICE_CONFIG_DT_INST(n),                                      \
		.tar_xip_cfg = MSPI_XIP_CONFIG_DT_INST(n),                                         \
		.tar_scramble_cfg = MSPI_SCRAMBLE_CONFIG_DT_INST(n),                               \
		.tar_timing_cfg = MSPI_TIMING_CONFIG(n),                                           \
		.timing_cfg_mask = MSPI_TIMING_CONFIG_MASK(n),                                     \
		.sw_multi_periph = DT_PROP(DT_INST_BUS(n), software_multiperipheral)};             \
	static struct memc_mspi_aps256n_data memc_mspi_aps256n_data_##n = {                        \
		.lock = Z_SEM_INITIALIZER(memc_mspi_aps256n_data_##n.lock, 0, 1),                  \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, memc_mspi_aps256n_pm_action);                                  \
	DEVICE_DT_INST_DEFINE(n, memc_mspi_aps256n_init, PM_DEVICE_DT_INST_GET(n),                 \
			      &memc_mspi_aps256n_data_##n, &memc_mspi_aps256n_config_##n,          \
			      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MSPI_APS256N)
