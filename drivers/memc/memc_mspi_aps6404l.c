/*
 * Copyright (c) 2024, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mspi_aps6404l
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

LOG_MODULE_REGISTER(memc_mspi_aps6404l, CONFIG_MEMC_LOG_LEVEL);

#define APM_VENDOR_ID                  0xD

#define APS6404L_WRITE                 0x02
#define APS6404L_READ                  0x03
#define APS6404L_FAST_READ             0x0B
#define APS6404L_QUAD_MODE_ENTER       0x35
#define APS6404L_QUAD_WRITE            0x38
#define APS6404L_RESET_ENABLE          0x66
#define APS6404L_RESET_MEMORY          0x99
#define APS6404L_READ_ID               0x9F
#define APS6404L_HALF_SLEEP_ENTER      0xC0
#define APS6404L_QUAD_READ             0xEB
#define APS6404L_QUAD_MODE_EXIT        0xF5

struct memc_mspi_aps6404l_config {
	uint32_t                       port;
	uint32_t                       mem_size;

	const struct device            *bus;
	struct mspi_dev_id             dev_id;
	struct mspi_dev_cfg            serial_cfg;
	struct mspi_dev_cfg            quad_cfg;
	struct mspi_dev_cfg            tar_dev_cfg;
	struct mspi_xip_cfg            tar_xip_cfg;
	struct mspi_scramble_cfg       tar_scramble_cfg;

	mspi_timing_cfg                tar_timing_cfg;
	mspi_timing_param              timing_cfg_mask;

	bool                           sw_multi_periph;
};

struct memc_mspi_aps6404l_data {
	struct mspi_dev_cfg            dev_cfg;
	struct mspi_xip_cfg            xip_cfg;
	struct mspi_scramble_cfg       scramble_cfg;
	mspi_timing_cfg                timing_cfg;
	struct mspi_xfer               trans;
	struct mspi_xfer_packet        packet;

	struct k_sem                   lock;
};

static int memc_mspi_aps6404l_command_write(const struct device *psram, uint8_t cmd, uint32_t addr,
					    uint8_t *wdata, uint32_t length)
{
	const struct memc_mspi_aps6404l_config *cfg = psram->config;
	struct memc_mspi_aps6404l_data *data = psram->data;
	int ret;
	uint8_t buffer[16];

	data->packet.dir              = MSPI_TX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = buffer;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.tx_dummy          = 0;
	data->trans.cmd_length        = 1;
	data->trans.addr_length       = 0;
	data->trans.hold_ce           = false;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = 10;

	if (wdata != NULL) {
		memcpy(buffer, wdata, length);
	}

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI write transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static int memc_mspi_aps6404l_command_read(const struct device *psram, uint8_t cmd, uint32_t addr,
					   uint8_t *rdata, uint32_t length)
{
	const struct memc_mspi_aps6404l_config *cfg = psram->config;
	struct memc_mspi_aps6404l_data *data = psram->data;

	int ret;
	uint8_t buffer[16];

	data->packet.dir              = MSPI_RX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = buffer;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.rx_dummy          = 0;
	data->trans.cmd_length        = 1;
	data->trans.addr_length       = 3;
	data->trans.hold_ce           = false;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	memcpy(rdata, buffer, length);
	return ret;
}

#if CONFIG_PM_DEVICE
static void acquire(const struct device *psram)
{
	const struct memc_mspi_aps6404l_config *cfg = psram->config;
	struct memc_mspi_aps6404l_data *data = psram->data;

	k_sem_take(&data->lock, K_FOREVER);

	if (cfg->sw_multi_periph) {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id,
				       MSPI_DEVICE_CONFIG_ALL, &data->dev_cfg)) {
			;
		}
	} else {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id,
				       MSPI_DEVICE_CONFIG_NONE, NULL)) {
			;
		}
	}
}
#endif /* CONFIG_PM_DEVICE */

static void release(const struct device *psram)
{
	const struct memc_mspi_aps6404l_config *cfg = psram->config;
	struct memc_mspi_aps6404l_data *data = psram->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port)) {
		;
	}

	k_sem_give(&data->lock);
}

static int memc_mspi_aps6404l_reset(const struct device *psram)
{
	int ret;

	LOG_DBG("Resetting aps6404l/%u", __LINE__);

	ret = memc_mspi_aps6404l_command_write(psram, APS6404L_RESET_ENABLE, 0, NULL, 0);
	if (ret) {
		return ret;
	}
	ret = memc_mspi_aps6404l_command_write(psram, APS6404L_RESET_MEMORY, 0, NULL, 0);
	if (ret) {
		return ret;
	}
	/** We need to delay 5 ms to allow aps6404L pSRAM to reinitialize */
	k_busy_wait(5000);
	return ret;
}

static int memc_mspi_aps6404l_get_vendor_id(const struct device *psram, uint8_t *vendor_id)
{
	uint16_t buffer = 0;
	int ret;

	ret = memc_mspi_aps6404l_command_read(psram, APS6404L_READ_ID, 0, (uint8_t *)&buffer, 2);
	LOG_DBG("Read ID buff: %x/%u", buffer, __LINE__);
	*vendor_id = buffer & 0xff;

	return ret;
}

#if CONFIG_PM_DEVICE
static int memc_mspi_aps6404l_half_sleep_enter(const struct device *psram)
{
	int ret;

	LOG_DBG("Putting aps6404l to half sleep/%u", __LINE__);
	ret = memc_mspi_aps6404l_command_write(psram, APS6404L_HALF_SLEEP_ENTER, 0, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to enter half sleep/%u", __LINE__);
		return ret;
	}
	/** Minimum half sleep duration tHS time */
	k_busy_wait(4);

	return ret;
}

static int memc_mspi_aps6404l_half_sleep_exit(const struct device *psram)
{
	const struct memc_mspi_aps6404l_config *cfg = psram->config;
	struct memc_mspi_aps6404l_data *data = psram->data;
	struct mspi_dev_cfg bkp = data->dev_cfg;
	int ret = 0;

	data->dev_cfg.freq = 48000000;

	mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_FREQUENCY,
			(const struct mspi_dev_cfg *)&data->dev_cfg);

	LOG_DBG("Waking up aps6404l from half sleep/%u", __LINE__);
	ret = memc_mspi_aps6404l_command_write(psram, 0, 0, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to exit from half sleep/%u", __LINE__);
		return ret;
	}
	/** Minimum half sleep exit CE to CLK setup time  */
	k_busy_wait(100);

	data->dev_cfg = bkp;

	ret = mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_FREQUENCY,
			      (const struct mspi_dev_cfg *)&data->dev_cfg);
	if (ret) {
		LOG_ERR("Failed to reconfigure MSPI after exiting half sleep/%u", __LINE__);
		return ret;
	}

	return ret;
}

static int memc_mspi_aps6404l_pm_action(const struct device *psram, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		acquire(psram);
		memc_mspi_aps6404l_half_sleep_exit(psram);
		release(psram);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		acquire(psram);
		memc_mspi_aps6404l_half_sleep_enter(psram);
		release(psram);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /** IS_ENABLED(CONFIG_PM_DEVICE) */

static int memc_mspi_aps6404l_init(const struct device *psram)
{
	const struct memc_mspi_aps6404l_config *cfg = psram->config;
	struct memc_mspi_aps6404l_data *data = psram->data;
	uint8_t vendor_id;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Controller device not ready/%u", __LINE__);
		return -ENODEV;
	}

	switch (cfg->tar_dev_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_QUAD:
		break;
	default:
		LOG_ERR("Bus mode %d not supported/%u", cfg->tar_dev_cfg.io_mode, __LINE__);
		return -EIO;
	}

	if (data->dev_cfg.io_mode == MSPI_IO_MODE_QUAD) {
		if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL,
				    &cfg->quad_cfg)) {
			LOG_ERR("Failed to config mspi controller/%u", __LINE__);
			return -EIO;
		}
		data->dev_cfg = cfg->quad_cfg;
		if (memc_mspi_aps6404l_reset(psram)) {
			LOG_ERR("Could not reset pSRAM/%u", __LINE__);
			return -EIO;
		}
		if (memc_mspi_aps6404l_command_write(psram, APS6404L_QUAD_MODE_EXIT, 0, NULL, 0)) {
			LOG_ERR("Could not exit quad mode/%u", __LINE__);
			return -EIO;
		}
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &cfg->serial_cfg)) {
		LOG_ERR("Failed to config mspi controller/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->serial_cfg;

	if (memc_mspi_aps6404l_reset(psram)) {
		LOG_ERR("Could not reset pSRAM/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps6404l_get_vendor_id(psram, &vendor_id)) {
		LOG_ERR("Could not read vendor id/%u", __LINE__);
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);
	if (vendor_id != APM_VENDOR_ID) {
		LOG_WRN("Vendor ID does not match expected value of 0x%0x/%u", APM_VENDOR_ID,
			__LINE__);
	}

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_QUAD) {
		if (memc_mspi_aps6404l_command_write(psram, APS6404L_QUAD_MODE_ENTER, 0, NULL, 0)) {
			return -EIO;
		}
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &cfg->tar_dev_cfg)) {
		LOG_ERR("Failed to config mspi controller/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->tar_dev_cfg;

#if CONFIG_MSPI_TIMING
	if (mspi_timing_config(cfg->bus, &cfg->dev_id, cfg->timing_cfg_mask,
			       (void *)&cfg->tar_timing_cfg)) {
		LOG_ERR("Failed to config mspi timing/%u", __LINE__);
		return -EIO;
	}
	data->timing_cfg = cfg->tar_timing_cfg;
#endif /* CONFIG_MSPI_TIMING */

#if CONFIG_MSPI_XIP
	if (cfg->tar_xip_cfg.enable) {
		if (mspi_xip_config(cfg->bus, &cfg->dev_id, &cfg->tar_xip_cfg)) {
			LOG_ERR("Failed to enable XIP/%u", __LINE__);
			return -EIO;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
	}
#endif /* CONFIG_MSPI_XIP */

#if CONFIG_MSPI_SCRAMBLE
	if (cfg->tar_scramble_cfg.enable) {
		if (mspi_scramble_config(cfg->bus, &cfg->dev_id, &cfg->tar_scramble_cfg)) {
			LOG_ERR("Failed to enable scrambling/%u", __LINE__);
			return -EIO;
		}
		data->scramble_cfg = cfg->tar_scramble_cfg;
	}
#endif /* MSPI_SCRAMBLE */

	release(psram);

	return 0;
}

#define MSPI_DEVICE_CONFIG_SERIAL(n)                                                              \
	{                                                                                         \
		.ce_num             = DT_INST_PROP(n, mspi_hardware_ce_num),                      \
		.freq               = 12000000,                                                   \
		.io_mode            = MSPI_IO_MODE_SINGLE,                                        \
		.data_rate          = MSPI_DATA_RATE_SINGLE,                                      \
		.cpp                = MSPI_CPP_MODE_0,                                            \
		.endian             = MSPI_XFER_LITTLE_ENDIAN,                                    \
		.ce_polarity        = MSPI_CE_ACTIVE_LOW,                                         \
		.dqs_enable         = false,                                                      \
		.rx_dummy           = 8,                                                          \
		.tx_dummy           = 0,                                                          \
		.read_cmd           = APS6404L_FAST_READ,                                         \
		.write_cmd          = APS6404L_WRITE,                                             \
		.cmd_length         = 1,                                                          \
		.addr_length        = 3,                                                          \
		.mem_boundary       = 1024,                                                        \
		.time_to_break      = 8,                                                         \
	}

#define MSPI_DEVICE_CONFIG_QUAD(n)                                                                \
	{                                                                                         \
		.ce_num             = DT_INST_PROP(n, mspi_hardware_ce_num),                      \
		.freq               = 24000000,                                                   \
		.io_mode            = MSPI_IO_MODE_SINGLE,                                        \
		.data_rate          = MSPI_DATA_RATE_SINGLE,                                      \
		.cpp                = MSPI_CPP_MODE_0,                                            \
		.endian             = MSPI_XFER_LITTLE_ENDIAN,                                    \
		.ce_polarity        = MSPI_CE_ACTIVE_LOW,                                         \
		.dqs_enable         = false,                                                      \
		.rx_dummy           = 6,                                                          \
		.tx_dummy           = 0,                                                          \
		.read_cmd           = APS6404L_QUAD_READ,                                         \
		.write_cmd          = APS6404L_QUAD_WRITE,                                        \
		.cmd_length         = 1,                                                          \
		.addr_length        = 3,                                                          \
		.mem_boundary       = 1024,                                                        \
		.time_to_break      = 4,                                                         \
	}

#if CONFIG_SOC_FAMILY_AMBIQ
#define MSPI_TIMING_CONFIG(n)                                                                     \
	{                                                                                         \
		.ui8WriteLatency    = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 0),             \
		.ui8TurnAround      = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 1),             \
		.bTxNeg             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 2),             \
		.bRxNeg             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 3),             \
		.bRxCap             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 4),             \
		.ui32TxDQSDelay     = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 5),             \
		.ui32RxDQSDelay     = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 6),             \
		.ui32RXDQSDelayEXT  = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 7),             \
	}
#define MSPI_TIMING_CONFIG_MASK(n) DT_INST_PROP(n, ambiq_timing_config_mask)
#else
#define MSPI_TIMING_CONFIG(n) {}
#define MSPI_TIMING_CONFIG_MASK(n) MSPI_TIMING_PARAM_DUMMY
#define MSPI_PORT(n) 0
#endif

#define MEMC_MSPI_APS6404L(n)                                                                     \
	static const struct memc_mspi_aps6404l_config                                             \
	memc_mspi_aps6404l_config_##n = {                                                         \
		.port               = MSPI_PORT(n),                                               \
		.mem_size           = DT_INST_PROP(n, size) / 8,                                  \
		.bus                = DEVICE_DT_GET(DT_INST_BUS(n)),                              \
		.dev_id             = MSPI_DEVICE_ID_DT_INST(n),                                  \
		.serial_cfg         = MSPI_DEVICE_CONFIG_SERIAL(n),                               \
		.quad_cfg           = MSPI_DEVICE_CONFIG_QUAD(n),                                 \
		.tar_dev_cfg        = MSPI_DEVICE_CONFIG_DT_INST(n),                              \
		.tar_xip_cfg        = MSPI_XIP_CONFIG_DT_INST(n),                                 \
		.tar_scramble_cfg   = MSPI_SCRAMBLE_CONFIG_DT_INST(n),                            \
		.tar_timing_cfg     = MSPI_TIMING_CONFIG(n),                                      \
		.timing_cfg_mask    = MSPI_TIMING_CONFIG_MASK(n),                                 \
		.sw_multi_periph    = DT_PROP(DT_INST_BUS(n), software_multiperipheral)           \
	};                                                                                        \
	static struct memc_mspi_aps6404l_data                                                     \
		memc_mspi_aps6404l_data_##n = {                                                   \
		.lock = Z_SEM_INITIALIZER(memc_mspi_aps6404l_data_##n.lock, 0, 1),                \
	};                                                                                        \
	PM_DEVICE_DT_INST_DEFINE(n, memc_mspi_aps6404l_pm_action);                                \
	DEVICE_DT_INST_DEFINE(n,                                                                  \
			      memc_mspi_aps6404l_init,                                            \
			      PM_DEVICE_DT_INST_GET(n),                                           \
			      &memc_mspi_aps6404l_data_##n,                                       \
			      &memc_mspi_aps6404l_config_##n,                                     \
			      POST_KERNEL,                                                        \
			      CONFIG_MEMC_INIT_PRIORITY,                                          \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MSPI_APS6404L)
