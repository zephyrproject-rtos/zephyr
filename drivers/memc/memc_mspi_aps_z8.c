/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mspi_aps_z8
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/pm/device_runtime.h>
#include "memc_mspi_aps_z8.h"
#if CONFIG_SOC_FAMILY_AMBIQ
#include "mspi_ambiq.h"
typedef struct mspi_ambiq_timing_cfg mspi_timing_cfg;
typedef enum mspi_ambiq_timing_param mspi_timing_param;
#else
typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;
#endif

LOG_MODULE_REGISTER(memc_mspi_aps_z8, CONFIG_MEMC_LOG_LEVEL);

#define APM_VENDOR_ID                  0xD

#define APS_Z8_SYNC_WRITE              0x80
#define APS_Z8_SYNC_READ               0x00
#define APS_Z8_LINEAR_BURST_WRITE      0xA0
#define APS_Z8_LINEAR_BURST_READ       0x20
#define APS_Z8_GLOBAL_RESET            0xFF
#define APS_Z8_WRITE_REGISTER          0xC0
#define APS_Z8_READ_REGISTER           0x40

struct memc_mspi_aps_z8_config {
	uint32_t                       port;
	uint32_t                       mem_size;

	const struct device            *bus;
	struct mspi_dev_id             dev_id;
	struct mspi_dev_cfg            octal_cfg;
	struct mspi_dev_cfg            tar_dev_cfg;

	MSPI_XIP_CFG_STRUCT_DECLARE(tar_xip_cfg)
	MSPI_SCRAMBLE_CFG_STRUCT_DECLARE(tar_scramble_cfg)
	MSPI_TIMING_CFG_STRUCT_DECLARE(tar_timing_cfg)
	MSPI_TIMING_PARAM_DECLARE(timing_cfg_mask)
	MSPI_XIP_BASE_ADDR_DECLARE(xip_base_addr)

	bool                           sw_multi_periph;
	bool                           pm_dev_rt_auto;
};

struct memc_mspi_aps_z8_data {
	struct memc_mspi_aps_z8_reg    regs;
	struct mspi_dev_cfg            dev_cfg;
	struct mspi_xip_cfg            xip_cfg;
	struct mspi_scramble_cfg       scramble_cfg;
	mspi_timing_cfg                timing_cfg;
	struct mspi_xfer               trans;
	struct mspi_xfer_packet        packet;

	struct k_sem                   lock;
	uint16_t                       dummy;
};

static int memc_mspi_aps_z8_command_write(const struct device *psram, uint8_t cmd,
					  uint32_t addr, uint8_t *wdata, uint32_t length)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;
	int ret;

	data->packet.dir              = MSPI_TX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = wdata;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.tx_dummy          = 0;
	data->trans.rx_dummy          = data->dev_cfg.rx_dummy;
	data->trans.cmd_length        = data->dev_cfg.cmd_length;
	data->trans.addr_length       = data->dev_cfg.addr_length;
	data->trans.hold_ce           = false;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI write transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static int memc_mspi_aps_z8_command_read(const struct device *psram, uint8_t cmd,
					 uint32_t addr, uint8_t *rdata, uint32_t length)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;
	int ret;

	data->packet.dir              = MSPI_RX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = rdata;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.tx_dummy          = data->dev_cfg.tx_dummy;
	data->trans.rx_dummy          = data->dev_cfg.rx_dummy;
	data->trans.cmd_length        = data->dev_cfg.cmd_length;
	data->trans.addr_length       = data->dev_cfg.addr_length;
	data->trans.hold_ce           = false;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static int memc_mspi_aps_z8_enter_command_mode(const struct device *psram)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;
	int ret;

	if (cfg->octal_cfg.io_mode == data->dev_cfg.io_mode) {
		return 0;
	}

	ret = mspi_dev_config(cfg->bus, &cfg->dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &cfg->octal_cfg);
	if (ret) {
		LOG_ERR("Failed to reconfigure MSPI while entering command mode/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->octal_cfg;

	data->regs.MR8_b.IOM = 0x0;
	ret = memc_mspi_aps_z8_command_write(psram, APS_Z8_WRITE_REGISTER, 8, &data->regs.MR8, 1);
	if (ret) {
		LOG_ERR("Failed to exit hex mode/%u", __LINE__);
		return -EIO;
	}
	return 0;
}

static int memc_mspi_aps_z8_exit_command_mode(const struct device *psram)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;
	int ret;

	if (cfg->tar_dev_cfg.io_mode == data->dev_cfg.io_mode) {
		return 0;
	}

	data->regs.MR8_b.IOM = 0x1;
	ret = memc_mspi_aps_z8_command_write(psram, APS_Z8_WRITE_REGISTER, 8, &data->regs.MR8, 1);
	if (ret) {
		LOG_ERR("Failed to enter hex mode/%u", __LINE__);
		return -EIO;
	}

	ret = mspi_dev_config(cfg->bus, &cfg->dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &cfg->tar_dev_cfg);
	if (ret) {
		LOG_ERR("Failed to reconfigure MSPI while exiting command mode/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->tar_dev_cfg;
	return 0;
}

#if CONFIG_PM_DEVICE
static void acquire(const struct device *psram)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;

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
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port)) {
		;
	}

	k_sem_give(&data->lock);
}

static int memc_mspi_aps_z8_reset(const struct device *psram)
{
	struct memc_mspi_aps_z8_data *data = psram->data;
	int ret = 0;

	LOG_DBG("Resetting APS Z8/%u", __LINE__);

	ret = memc_mspi_aps_z8_command_write(psram, APS_Z8_GLOBAL_RESET, 0,
					     (uint8_t *)&data->dummy, 2);
	if (ret) {
		return ret;
	}

	/** We need to delay 2us minimum to allow APS pSRAM to reinitialize */
	k_busy_wait(2);
	return ret;
}

static int memc_mspi_aps_z8_get_vendor_id(const struct device *psram)
{
	struct memc_mspi_aps_z8_data *data = psram->data;
	int ret;

	ret = memc_mspi_aps_z8_command_read(psram, APS_Z8_READ_REGISTER, 1, &data->regs.MR1, 1);
	LOG_DBG("MR1 reg: %x/%u", data->regs.MR1, __LINE__);

	if (data->regs.MR1_b.VID != APM_VENDOR_ID) {
		return -EIO;
	}

	return ret;
}

static int memc_mspi_aps_z8_get_rlc(uint8_t rx_dummy, enum memc_mspi_aps_z8_rlc *rlc)
{
	switch (rx_dummy) {
	case 4:
		*rlc = MEMC_MSPI_APS_Z8_RLC_4;
		break;
	case 5:
		*rlc = MEMC_MSPI_APS_Z8_RLC_5;
		break;
	case 6:
		*rlc = MEMC_MSPI_APS_Z8_RLC_6;
		break;
	case 7:
		*rlc = MEMC_MSPI_APS_Z8_RLC_7;
		break;
	case 8:
		*rlc = MEMC_MSPI_APS_Z8_RLC_8;
		break;
	case 9:
		*rlc = MEMC_MSPI_APS_Z8_RLC_9;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int memc_mspi_aps_z8_get_wlc(uint8_t tx_dummy, enum memc_mspi_aps_z8_wlc *wlc)
{
	switch (tx_dummy) {
	case 5:
		*wlc = MEMC_MSPI_APS_Z8_WLC_5;
		break;
	case 6:
		*wlc = MEMC_MSPI_APS_Z8_WLC_6;
		break;
	case 7:
		*wlc = MEMC_MSPI_APS_Z8_WLC_7;
		break;
	case 8:
		*wlc = MEMC_MSPI_APS_Z8_WLC_8;
		break;
	case 9:
		*wlc = MEMC_MSPI_APS_Z8_WLC_9;
		break;
	case 10:
		*wlc = MEMC_MSPI_APS_Z8_WLC_10;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

#if CONFIG_PM_DEVICE
static int memc_mspi_aps_z8_half_sleep_enter(const struct device *psram)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;
	struct mspi_xip_cfg xip_cfg = data->xip_cfg;
	int ret;

	if (xip_cfg.enable) {
		sys_cache_data_flush_and_invd_all();
	}

#if CONFIG_MSPI_XIP
	xip_cfg.enable = false;
	if (mspi_xip_config(cfg->bus, &cfg->dev_id, &xip_cfg)) {
		LOG_ERR("Failed to disable XIP/%u", __LINE__);
		return -EIO;
	}
#endif /* CONFIG_MSPI_XIP */

	LOG_DBG("Putting APS Z8 to half sleep/%u", __LINE__);
	ret = memc_mspi_aps_z8_enter_command_mode(psram);
	if (ret) {
		return ret;
	}

	data->regs.MR6 = 0xF0;
	ret = memc_mspi_aps_z8_command_write(psram, APS_Z8_WRITE_REGISTER, 6, &data->regs.MR6, 1);
	if (ret) {
		LOG_ERR("Failed to enter half sleep/%u", __LINE__);
		return ret;
	}
	/* Minimum half sleep duration time(tHS) */
	k_busy_wait(150);

	return ret;
}

static int memc_mspi_aps_z8_half_sleep_exit(const struct device *psram)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;
	int ret = 0;

	LOG_DBG("Waking up aps_z8 from half sleep/%u", __LINE__);
	ret = memc_mspi_aps_z8_command_write(psram, 0, 0, (uint8_t *)&data->dummy, 2);
	if (ret) {
		LOG_ERR("Failed to exit from half sleep/%u", __LINE__);
		return ret;
	}
	/* Minimum half sleep exit CE to CLK setup time(tXHS)  */
	k_busy_wait(150);

	ret = memc_mspi_aps_z8_exit_command_mode(psram);
	if (ret) {
		return ret;
	}

#if CONFIG_MSPI_XIP
	if (mspi_xip_config(cfg->bus, &cfg->dev_id, &data->xip_cfg)) {
		LOG_ERR("Failed to enable XIP/%u", __LINE__);
		return -EIO;
	}
#endif /* CONFIG_MSPI_XIP */

	return ret;
}

static int memc_mspi_aps_z8_pm_action(const struct device *psram, enum pm_device_action action)
{
	struct memc_mspi_aps_z8_data *data = psram->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (data->regs.MR1_b.ULP) {
			acquire(psram);
			memc_mspi_aps_z8_half_sleep_exit(psram);
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (data->regs.MR1_b.ULP) {
			memc_mspi_aps_z8_half_sleep_enter(psram);
			release(psram);
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* IS_ENABLED(CONFIG_PM_DEVICE) */

static int memc_mspi_aps_z8_init(const struct device *psram)
{
	const struct memc_mspi_aps_z8_config *cfg = psram->config;
	struct memc_mspi_aps_z8_data *data = psram->data;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Controller device not ready/%u", __LINE__);
		return -ENODEV;
	}

	switch (cfg->tar_dev_cfg.io_mode) {
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_HEX_8_8_16:
		break;
	default:
		LOG_ERR("Bus mode %d not supported/%u", cfg->tar_dev_cfg.io_mode, __LINE__);
		return -EIO;
	}

	if (cfg->tar_dev_cfg.data_rate != MSPI_DATA_RATE_S_D_D) {
		LOG_ERR("Data rate %d not supported/%u", cfg->tar_dev_cfg.data_rate, __LINE__);
		return -EIO;
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &cfg->octal_cfg)) {
		LOG_ERR("Failed to config mspi controller/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->octal_cfg;

	if (memc_mspi_aps_z8_reset(psram)) {
		LOG_ERR("Could not reset pSRAM/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_get_vendor_id(psram)) {
		LOG_ERR("Could not read vendor id/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_command_read(psram, APS_Z8_READ_REGISTER, 0, &data->regs.MR0, 1)) {
		LOG_ERR("Could not read MR0 register/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_command_read(psram, APS_Z8_READ_REGISTER, 2, &data->regs.MR2, 1)) {
		LOG_ERR("Could not read MR2 register/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_command_read(psram, APS_Z8_READ_REGISTER, 3, &data->regs.MR3, 1)) {
		LOG_ERR("Could not read MR3 register/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_command_read(psram, APS_Z8_READ_REGISTER, 4, &data->regs.MR4, 1)) {
		LOG_ERR("Could not read MR4 register/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_command_read(psram, APS_Z8_READ_REGISTER, 6, &data->regs.MR6, 1)) {
		LOG_ERR("Could not read MR2 register/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_command_read(psram, APS_Z8_READ_REGISTER, 8, &data->regs.MR8, 1)) {
		LOG_ERR("Could not read MR8 register/%u", __LINE__);
		return -EIO;
	}

	enum memc_mspi_aps_z8_rlc rlc;

	if (memc_mspi_aps_z8_get_rlc(cfg->tar_dev_cfg.rx_dummy, &rlc)) {
		LOG_ERR("rx_dummy:%d not supported/%u", cfg->tar_dev_cfg.rx_dummy, __LINE__);
		return -EIO;
	}
	data->regs.MR0_b.RLC = rlc;

	enum memc_mspi_aps_z8_wlc wlc;

	if (memc_mspi_aps_z8_get_wlc(cfg->tar_dev_cfg.tx_dummy, &wlc)) {
		LOG_ERR("tx_dummy:%d not supported/%u", cfg->tar_dev_cfg.tx_dummy, __LINE__);
		return -EIO;
	}
	data->regs.MR4_b.WLC = wlc;

	data->regs.MR0_b.LT = !cfg->tar_dev_cfg.dqs_enable;

	if (memc_mspi_aps_z8_command_write(psram, APS_Z8_WRITE_REGISTER, 0, &data->regs.MR0, 1)) {
		LOG_ERR("Could not write MR0 register/%u", __LINE__);
		return -EIO;
	}

	if (memc_mspi_aps_z8_command_write(psram, APS_Z8_WRITE_REGISTER, 4, &data->regs.MR4, 1)) {
		LOG_ERR("Could not write MR4 register/%u", __LINE__);
		return -EIO;
	}

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_HEX_8_8_16) {
		data->regs.MR8_b.IOM = 1;
		if (memc_mspi_aps_z8_command_write(psram, APS_Z8_WRITE_REGISTER, 8,
						   &data->regs.MR8, 1)) {
			LOG_ERR("Could not write MR8 register/%u", __LINE__);
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

	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) ||
	    !cfg->pm_dev_rt_auto) {
		release(psram);
	}

	return 0;
}

#define MSPI_DEVICE_CONFIG_OCTAL(n)                                                               \
	{                                                                                         \
		.ce_num             = DT_INST_PROP(n, mspi_hardware_ce_num),                      \
		.freq               = 24000000,                                                   \
		.io_mode            = MSPI_IO_MODE_OCTAL,                                         \
		.data_rate          = MSPI_DATA_RATE_S_D_D,                                       \
		.cpp                = MSPI_CPP_MODE_0,                                            \
		.endian             = MSPI_XFER_LITTLE_ENDIAN,                                    \
		.ce_polarity        = MSPI_CE_ACTIVE_LOW,                                         \
		.dqs_enable         = DT_INST_PROP(n, mspi_dqs_enable),                           \
		.rx_dummy           = MEMC_MSPI_APS_Z8_RX_DUMMY_DEFAULT,                          \
		.tx_dummy           = MEMC_MSPI_APS_Z8_TX_DUMMY_DEFAULT,                          \
		.read_cmd           = APS_Z8_LINEAR_BURST_READ,                                   \
		.write_cmd          = APS_Z8_LINEAR_BURST_WRITE,                                  \
		.cmd_length         = MEMC_MSPI_APS_Z8_CMD_LENGTH_DEFAULT,                        \
		.addr_length        = MEMC_MSPI_APS_Z8_ADDR_LENGTH_DEFAULT,                       \
		.mem_boundary       = 1024,                                                       \
		.time_to_break      = 4,                                                          \
	}
#define MSPI_TIMING_CONFIG(n)                                                                     \
	COND_CODE_1(CONFIG_SOC_FAMILY_AMBIQ,                                                      \
		(MSPI_AMBIQ_TIMING_CONFIG(n)), ({}))                                              \

#define MSPI_TIMING_CONFIG_MASK(n)                                                                \
	COND_CODE_1(CONFIG_SOC_FAMILY_AMBIQ,                                                      \
		(MSPI_AMBIQ_TIMING_CONFIG_MASK(n)), (MSPI_TIMING_PARAM_DUMMY))                    \

#define MSPI_PORT(n)                                                                              \
	COND_CODE_1(CONFIG_SOC_FAMILY_AMBIQ,                                                      \
		(MSPI_AMBIQ_PORT(n)), (0))                                                        \

#define MEMC_MSPI_APS_Z8(n)                                                                       \
	static const struct memc_mspi_aps_z8_config                                               \
	memc_mspi_aps_z8_config_##n = {                                                           \
		.port               = MSPI_PORT(n),                                               \
		.mem_size           = DT_INST_PROP(n, size) / 8,                                  \
		.bus                = DEVICE_DT_GET(DT_INST_BUS(n)),                              \
		.dev_id             = MSPI_DEVICE_ID_DT_INST(n),                                  \
		.octal_cfg          = MSPI_DEVICE_CONFIG_OCTAL(n),                                \
		.tar_dev_cfg        = MSPI_DEVICE_CONFIG_DT_INST(n),                              \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_XIP,                                    \
					      tar_xip_cfg, MSPI_XIP_CONFIG_DT_INST(n))            \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_SCRAMBLE,                               \
					      tar_scramble_cfg, MSPI_SCRAMBLE_CONFIG_DT_INST(n))  \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_TIMING,                                 \
					      tar_timing_cfg, MSPI_TIMING_CONFIG(n))              \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_TIMING,                                 \
					      timing_cfg_mask, MSPI_TIMING_CONFIG_MASK(n))        \
		MSPI_XIP_BASE_ADDR_INIT(xip_base_addr, DT_INST_BUS(n))                            \
		.sw_multi_periph    = DT_PROP(DT_INST_BUS(n), software_multiperipheral),          \
		.pm_dev_rt_auto     = DT_INST_PROP(n, zephyr_pm_device_runtime_auto)              \
	};                                                                                        \
	static struct memc_mspi_aps_z8_data                                                       \
		memc_mspi_aps_z8_data_##n = {                                                     \
		.lock  = Z_SEM_INITIALIZER(memc_mspi_aps_z8_data_##n.lock, 0, 1),                 \
		.dummy = 0,                                                                       \
	};                                                                                        \
	PM_DEVICE_DT_INST_DEFINE(n, memc_mspi_aps_z8_pm_action);                                  \
	DEVICE_DT_INST_DEFINE(n,                                                                  \
			      memc_mspi_aps_z8_init,                                              \
			      PM_DEVICE_DT_INST_GET(n),                                           \
			      &memc_mspi_aps_z8_data_##n,                                         \
			      &memc_mspi_aps_z8_config_##n,                                       \
			      POST_KERNEL,                                                        \
			      CONFIG_MEMC_INIT_PRIORITY,                                          \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MSPI_APS_Z8)
