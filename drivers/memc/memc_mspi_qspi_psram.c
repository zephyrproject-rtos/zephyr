/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mspi_qspi_psram

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/devicetree.h>
#include <zephyr/pm/device_runtime.h>
#include "memc_mspi_qspi_psram.h"

#if CONFIG_SOC_FAMILY_AMBIQ
#include "mspi_ambiq.h"
typedef struct mspi_ambiq_timing_cfg mspi_timing_cfg;
typedef enum mspi_ambiq_timing_param mspi_timing_param;
#elif CONFIG_SOC_FAMILY_STM32
#include "mspi_stm32.h"
typedef struct mspi_stm32_timing_cfg mspi_timing_cfg;
typedef enum mspi_stm32_timing_param mspi_timing_param;
#else
typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;
#endif

LOG_MODULE_REGISTER(memc_mspi_qspi_psram, CONFIG_MEMC_LOG_LEVEL);

/*
 * Chip parameter table — indexed by enum qspi_psram_variant (0..N-1, not GENERIC).
 * Values come from device datasheets and must not be changed without
 * verifying against the relevant datasheet revision.
 */
static const struct qspi_psram_chip_params chip_table[] = {
	[QSPI_PSRAM_VARIANT_ESP64H] = {
		/* Espressif ESP-PSRAM64H, 64 Mbit (8 MB), 3.3 V / 1.8 V    */
		/* Datasheet: tCEM = 8 μs, row boundary = 1 KB               */
		.enter_qpi_cmd      = 0x35,
		.exit_qpi_cmd       = 0xF5,
		.read_cmd           = 0xEB,
		.write_cmd          = 0x38,
		.reset_en_cmd       = 0x66,
		.reset_cmd          = 0x99,
		.read_id_cmd        = 0x9F,
		.kgd_value          = 0x5D,
		.cmd_length         = QSPI_PSRAM_CMD_LEN_1BYTE,
		.addr_length        = QSPI_PSRAM_ADDR_LEN_3BYTE,
		.default_rx_dummy   = 6,
		.default_tx_dummy   = 0,
		.ce_max_burst_bytes = 1024,
		.ce_refresh_us      = 8,
	},
	[QSPI_PSRAM_VARIANT_IS66WVS4M8BLL] = {
		/* ISSI IS66WVS4M8BLL, 32 Mbit (4 MB)                        */
		/* Datasheet: tCEM = 8 μs, row boundary = 1 KB               */
		.enter_qpi_cmd      = 0x35,
		.exit_qpi_cmd       = 0xF5,
		.read_cmd           = 0xEB,
		.write_cmd          = 0x38,
		.reset_en_cmd       = 0x66,
		.reset_cmd          = 0x99,
		.read_id_cmd        = 0x9F,
		.kgd_value          = 0x5D,
		.cmd_length         = QSPI_PSRAM_CMD_LEN_1BYTE,
		.addr_length        = QSPI_PSRAM_ADDR_LEN_3BYTE,
		.default_rx_dummy   = 6,
		.default_tx_dummy   = 0,
		.ce_max_burst_bytes = 1024,
		.ce_refresh_us      = 4,
	},
	[QSPI_PSRAM_VARIANT_IS66WVS8M8BLL] = {
		/* ISSI IS66WVS8M8BLL, 64 Mbit (8 MB)                        */
		/* Datasheet: tCEM = 8 μs, row boundary = 1 KB               */
		.enter_qpi_cmd      = 0x35,
		.exit_qpi_cmd       = 0xF5,
		.read_cmd           = 0xEB,
		.write_cmd          = 0x38,
		.reset_en_cmd       = 0x66,
		.reset_cmd          = 0x99,
		.read_id_cmd        = 0x9F,
		.kgd_value          = 0x5D,
		.cmd_length         = QSPI_PSRAM_CMD_LEN_1BYTE,
		.addr_length        = QSPI_PSRAM_ADDR_LEN_3BYTE,
		.default_rx_dummy   = 6,
		.default_tx_dummy   = 0,
		.ce_max_burst_bytes = 1024,
		.ce_refresh_us      = 4,
	},
};

struct memc_mspi_qspi_psram_config {
	uint32_t                    port;
	uint32_t                    mem_size;

	const struct device        *bus;
	struct mspi_dev_id          dev_id;
	struct mspi_dev_cfg         spi_init_cfg;
	struct mspi_dev_cfg         tar_dev_cfg;

	MSPI_XIP_CFG_STRUCT_DECLARE(tar_xip_cfg)
	MSPI_SCRAMBLE_CFG_STRUCT_DECLARE(tar_scramble_cfg)
	MSPI_TIMING_CFG_STRUCT_DECLARE(tar_timing_cfg)
	MSPI_TIMING_PARAM_DECLARE(timing_cfg_mask)
	MSPI_XIP_BASE_ADDR_DECLARE(xip_base_addr)

	bool                        sw_multi_periph;
	bool                        pm_dev_rt_auto;
	bool                        has_chip_variant; /* false → generic mode, use DT params */
	enum qspi_psram_variant     variant;          /* valid only when has_chip_variant */

	/* Generic-mode init commands (from DT, with standard defaults) */
	uint8_t                     enter_qpi_cmd;
	uint8_t                     exit_qpi_cmd;
	uint8_t                     reset_en_cmd;
	uint8_t                     reset_cmd;
	uint8_t                     read_id_cmd;
};

struct memc_mspi_qspi_psram_data {
	struct mspi_dev_cfg         dev_cfg;
	struct mspi_xip_cfg         xip_cfg;
	struct mspi_scramble_cfg    scramble_cfg;
#if defined(CONFIG_MSPI_TIMING)
	mspi_timing_cfg             timing_cfg;
#endif
	struct mspi_xfer            trans;
	struct mspi_xfer_packet     packet;

	struct k_sem                lock;
	uint16_t                    dummy;
};

static int qspi_psram_command_write(const struct device *psram, uint8_t cmd,
				    uint32_t addr, uint8_t *wdata, uint32_t length)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;
	int ret;

	data->packet.dir        = MSPI_TX;
	data->packet.cmd        = cmd;
	data->packet.address    = addr;
	data->packet.data_buf   = wdata;
	data->packet.num_bytes  = length;

	data->trans.async       = false;
	data->trans.xfer_mode   = MSPI_PIO;
	data->trans.tx_dummy    = 0;
	data->trans.rx_dummy    = 0;
	data->trans.cmd_length  = data->dev_cfg.cmd_length;
	data->trans.addr_length = data->dev_cfg.addr_length;
	data->trans.hold_ce     = false;
	data->trans.packets     = &data->packet;
	data->trans.num_packet  = 1;
	data->trans.timeout     = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id,
			      (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI write transaction failed: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return 0;
}

static int qspi_psram_command_read(const struct device *psram, uint8_t cmd,
				   uint32_t addr, uint8_t *rdata, uint32_t length)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;
	int ret;

	data->packet.dir        = MSPI_RX;
	data->packet.cmd        = cmd;
	data->packet.address    = addr;
	data->packet.data_buf   = rdata;
	data->packet.num_bytes  = length;

	data->trans.async       = false;
	data->trans.xfer_mode   = MSPI_PIO;
	data->trans.tx_dummy    = data->dev_cfg.tx_dummy;
	data->trans.rx_dummy    = data->dev_cfg.rx_dummy;
	data->trans.cmd_length  = data->dev_cfg.cmd_length;
	data->trans.addr_length = data->dev_cfg.addr_length;
	data->trans.hold_ce     = false;
	data->trans.packets     = &data->packet;
	data->trans.num_packet  = 1;
	data->trans.timeout     = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id,
			      (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI read transaction failed: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return 0;
}

static int qspi_psram_reset(const struct device *psram,
			    const struct qspi_psram_chip_params *chip)
{
	struct memc_mspi_qspi_psram_data *data = psram->data;
	int ret;

	ret = qspi_psram_command_write(psram, chip->reset_en_cmd, 0,
				       (uint8_t *)&data->dummy, 0);
	if (ret) {
		LOG_ERR("Failed to send Reset Enable/%u", __LINE__);
		return ret;
	}

	ret = qspi_psram_command_write(psram, chip->reset_cmd, 0,
				       (uint8_t *)&data->dummy, 0);
	if (ret) {
		LOG_ERR("Failed to send Reset/%u", __LINE__);
		return ret;
	}

	k_busy_wait(QSPI_PSRAM_RESET_DELAY_US);
	return 0;
}

static int qspi_psram_verify_id(const struct device *psram,
				const struct qspi_psram_chip_params *chip)
{
	struct memc_mspi_qspi_psram_data *data = psram->data;
	uint8_t id[3] = {0};
	uint8_t saved_addr_length;
	int ret;

	/*
	 * Read ID (0x9F) requires a 24-bit address phase in SPI mode for all
	 * supported chip variants (IS66WVS4M8BLL, ESP-PSRAM64H, IS66WVS8M8BLL).
	 * spi_init_cfg has addr_length = 0 (ADDR_DISABLED) for reset commands
	 * which need no address, so override it here for this transaction only.
	 */
	saved_addr_length = data->dev_cfg.addr_length;
	data->dev_cfg.addr_length = 3; /* ADDR_3_BYTE */

	ret = qspi_psram_command_read(psram, chip->read_id_cmd, 0, id, sizeof(id));

	data->dev_cfg.addr_length = saved_addr_length;

	if (ret) {
		LOG_ERR("Failed to read chip ID/%u", __LINE__);
		return ret;
	}

	LOG_DBG("PSRAM ID: MF=0x%02X KGD=0x%02X EID=0x%02X", id[0], id[1], id[2]);

	if (chip->kgd_value == 0) {
		/* Generic mode: no expected KGD, log and continue */
		LOG_INF("Generic PSRAM: skipping KGD check (ID MF=0x%02X KGD=0x%02X EID=0x%02X)",
			id[0], id[1], id[2]);
		return 0;
	}

	if (id[1] != chip->kgd_value) {
		LOG_ERR("KGD mismatch: expected 0x%02X got 0x%02X/%u",
			chip->kgd_value, id[1], __LINE__);
		return -EIO;
	}

	return 0;
}

static void qspi_psram_release(const struct device *psram)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port)) {
		;
	}

	k_sem_give(&data->lock);
}

#if defined(CONFIG_PM_DEVICE)
static void qspi_psram_acquire(const struct device *psram)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;

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

static int qspi_psram_pm_action(const struct device *psram,
				enum pm_device_action action)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		qspi_psram_acquire(psram);
#if CONFIG_MSPI_XIP
		if (data->xip_cfg.enable) {
			ret = mspi_xip_config(cfg->bus, &cfg->dev_id, &data->xip_cfg);
			if (ret) {
				LOG_ERR("Failed to re-enable XIP/%u", __LINE__);
			}
		}
#endif
		break;

	case PM_DEVICE_ACTION_SUSPEND:
#if CONFIG_MSPI_XIP
		if (data->xip_cfg.enable) {
			sys_cache_data_flush_and_invd_all();
			struct mspi_xip_cfg xip_off = data->xip_cfg;

			xip_off.enable = false;
			ret = mspi_xip_config(cfg->bus, &cfg->dev_id, &xip_off);
			if (ret) {
				LOG_ERR("Failed to disable XIP/%u", __LINE__);
				break;
			}
		}
#endif
		qspi_psram_release(psram);
		break;

	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/*
 * Attempt to bring the chip back to SPI mode unconditionally.
 * Required on warm boot when a previous init left the chip in QPI mode:
 * SPI-mode reset commands would not be recognised by a chip in QPI mode,
 * causing Read ID to return all-zeros.
 * Sequence mirrors the working legacy driver (psram_force_spi_mode):
 *   1. Exit QPI (0xF5) sent in 4-line mode
 *   2. Reset Enable + Reset sent in 4-line mode (clean up any QPI state)
 *   3. Restore 1-line SPI mode — subsequent SPI reset/ID commands are clean
 * Errors in the QPI phase are intentionally ignored: if the chip is already
 * in SPI mode the QPI-mode commands produce garbage on the bus but the chip
 * is unaffected.
 */
static int qspi_psram_force_spi_mode(const struct device *psram,
				     const struct qspi_psram_chip_params *chip)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;
	struct mspi_dev_cfg qpi_cfg = cfg->spi_init_cfg;

	qpi_cfg.io_mode = MSPI_IO_MODE_QUAD;

	if (!mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &qpi_cfg)) {
		data->dev_cfg = qpi_cfg;

		/* Exit QPI → chip returns to SPI mode if it was in QPI */
		qspi_psram_command_write(psram, chip->exit_qpi_cmd, 0,
					 (uint8_t *)&data->dummy, 0);

		/* Reset while still in QPI (handles chips in inconsistent state) */
		qspi_psram_command_write(psram, chip->reset_en_cmd, 0,
					 (uint8_t *)&data->dummy, 0);
		qspi_psram_command_write(psram, chip->reset_cmd, 0,
					 (uint8_t *)&data->dummy, 0);
		k_busy_wait(QSPI_PSRAM_RESET_DELAY_US);
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id,
			    MSPI_DEVICE_CONFIG_ALL, &cfg->spi_init_cfg)) {
		LOG_ERR("Failed to restore SPI mode after QPI exit/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->spi_init_cfg;
	return 0;
}

static int memc_mspi_qspi_psram_init(const struct device *psram)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;
	struct qspi_psram_chip_params generic_params;
	const struct qspi_psram_chip_params *chip;
	int ret;

	if (cfg->has_chip_variant) {
		chip = &chip_table[cfg->variant];
	} else {
		/*
		 * Build chip params from DT-configurable fields; transfer
		 * commands and CE timing are filled later from tar_dev_cfg.
		 */
		generic_params = (struct qspi_psram_chip_params){
			.enter_qpi_cmd = cfg->enter_qpi_cmd,
			.exit_qpi_cmd  = cfg->exit_qpi_cmd,
			.reset_en_cmd  = cfg->reset_en_cmd,
			.reset_cmd     = cfg->reset_cmd,
			.read_id_cmd   = cfg->read_id_cmd,
			.kgd_value     = 0, /* skip KGD verification */
		};
		chip = &generic_params;
	}

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("MSPI controller not ready/%u", __LINE__);
		return -ENODEV;
	}

	if (cfg->tar_dev_cfg.io_mode != MSPI_IO_MODE_QUAD) {
		LOG_ERR("Only MSPI_IO_MODE_QUAD supported, got %d/%u",
			cfg->tar_dev_cfg.io_mode, __LINE__);
		return -EIO;
	}

	if (!cfg->has_chip_variant) {
		if (cfg->tar_dev_cfg.read_cmd == 0 || cfg->tar_dev_cfg.write_cmd == 0) {
			LOG_ERR("Generic mode requires read-command and write-command in DT/%u",
				__LINE__);
			return -EINVAL;
		}
	}

	/* Configure controller in single SPI mode for init-phase commands */
	ret = mspi_dev_config(cfg->bus, &cfg->dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &cfg->spi_init_cfg);
	if (ret) {
		LOG_ERR("Failed to set SPI init mode/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->spi_init_cfg;

	/* Exit QPI mode in case the chip was left there by a previous boot */
	ret = qspi_psram_force_spi_mode(psram, chip);
	if (ret) {
		LOG_ERR("Failed to force SPI mode/%u", __LINE__);
		return ret;
	}

	ret = qspi_psram_reset(psram, chip);
	if (ret) {
		LOG_ERR("Failed to reset PSRAM/%u", __LINE__);
		return -EIO;
	}

	ret = qspi_psram_verify_id(psram, chip);
	if (ret) {
		LOG_ERR("PSRAM ID verification failed/%u", __LINE__);
		return -EIO;
	}

	/* Switch chip to QPI mode (SPI command, single-wire) */
	ret = qspi_psram_command_write(psram, chip->enter_qpi_cmd, 0,
				       (uint8_t *)&data->dummy, 0);
	k_busy_wait(100); /* wait for chip to change mode */
	if (ret) {
		LOG_ERR("Failed to enter QPI mode/%u", __LINE__);
		return -EIO;
	}

	/*
	 * Build the runtime device config from DT target values.
	 * For known chip variants: override read/write commands and CE timing
	 * with datasheet values from chip_table, ignoring whatever was in DT.
	 * For generic mode: all transfer parameters come from DT as-is.
	 */
	data->dev_cfg = cfg->tar_dev_cfg;
	if (cfg->has_chip_variant) {
		data->dev_cfg.read_cmd      = chip->read_cmd;
		data->dev_cfg.write_cmd     = chip->write_cmd;
		data->dev_cfg.cmd_length    = chip->cmd_length;
		data->dev_cfg.addr_length   = chip->addr_length;
		data->dev_cfg.rx_dummy      = chip->default_rx_dummy;
		data->dev_cfg.tx_dummy      = chip->default_tx_dummy;
		data->dev_cfg.mem_boundary  = chip->ce_max_burst_bytes;
		data->dev_cfg.time_to_break = chip->ce_refresh_us;
	}

	ret = mspi_dev_config(cfg->bus, &cfg->dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &data->dev_cfg);
	if (ret) {
		LOG_ERR("Failed to set QPI target mode/%u", __LINE__);
		return -EIO;
	}

#if CONFIG_MSPI_TIMING
	ret = mspi_timing_config(cfg->bus, &cfg->dev_id,
				 cfg->timing_cfg_mask,
				 (void *)&cfg->tar_timing_cfg);
	if (ret) {
		LOG_ERR("Failed to configure MSPI timing/%u", __LINE__);
		return -EIO;
	}
	data->timing_cfg = cfg->tar_timing_cfg;
#endif

#if CONFIG_MSPI_XIP
	if (cfg->tar_xip_cfg.enable) {
		ret = mspi_xip_config(cfg->bus, &cfg->dev_id, &cfg->tar_xip_cfg);
		if (ret) {
			LOG_ERR("Failed to enable XIP/%u", __LINE__);
			return -EIO;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
	}
#endif

#if CONFIG_MSPI_SCRAMBLE
	if (cfg->tar_scramble_cfg.enable) {
		ret = mspi_scramble_config(cfg->bus, &cfg->dev_id,
					   &cfg->tar_scramble_cfg);
		if (ret) {
			LOG_ERR("Failed to enable scrambling/%u", __LINE__);
			return -EIO;
		}
		data->scramble_cfg = cfg->tar_scramble_cfg;
	}
#endif

	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || !cfg->pm_dev_rt_auto) {
		qspi_psram_release(psram);
	}

	return 0;
}

/*
 * Per-platform timing config helpers.
 * MSPI_STM32_TIMING_CONFIG reads the st,timing-config DT array property
 * (turnaround cycles). On other platforms a no-op struct is used.
 */
#define QSPI_PSRAM_TIMING_CONFIG(n)                                          \
	COND_CODE_1(CONFIG_SOC_FAMILY_STM32,                                 \
		(MSPI_STM32_TIMING_CONFIG(n)), ({}))

#define QSPI_PSRAM_TIMING_CONFIG_MASK(n)                                     \
	COND_CODE_1(CONFIG_SOC_FAMILY_STM32,                                 \
		(MSPI_TURNAROUND_CYCLES_CFG), (MSPI_TIMING_PARAM_DUMMY))

/*
 * Fixed SPI-mode config used only during the chip initialisation sequence.
 * addr_length = 0 (ADDR_DISABLED) so no address bytes are emitted for
 * Reset Enable, Reset, Read ID and Enter QPI commands.
 */
#define QSPI_PSRAM_SPI_INIT_CFG(n)                                           \
	{                                                                    \
		.ce_num        = DT_INST_PROP(n, mspi_hardware_ce_num),      \
		.freq          = QSPI_PSRAM_SPI_INIT_FREQ,                   \
		.io_mode       = MSPI_IO_MODE_SINGLE,                        \
		.data_rate     = MSPI_DATA_RATE_SINGLE,                      \
		.cpp           = MSPI_CPP_MODE_0,                            \
		.endian        = MSPI_XFER_LITTLE_ENDIAN,                    \
		.ce_polarity   = MSPI_CE_ACTIVE_LOW,                         \
		.dqs_enable    = false,                                      \
		.rx_dummy      = 0,                                          \
		.tx_dummy      = 0,                                          \
		.read_cmd      = 0,                                          \
		.write_cmd     = 0,                                          \
		.cmd_length    = 1,   /* INSTR_1_BYTE */                     \
		.addr_length   = 0,   /* ADDR_DISABLED */                    \
		.mem_boundary  = 0,                                          \
		.time_to_break = 0,                                          \
	}

#define MEMC_MSPI_QSPI_PSRAM(n)                                                          \
	static const struct memc_mspi_qspi_psram_config                                  \
	memc_mspi_qspi_psram_config_##n = {                                              \
		.port            = 0,                                                    \
		.mem_size        = DT_INST_PROP(n, size) / 8,                            \
		.bus             = DEVICE_DT_GET(DT_INST_BUS(n)),                        \
		.dev_id          = MSPI_DEVICE_ID_DT_INST(n),                            \
		.spi_init_cfg    = QSPI_PSRAM_SPI_INIT_CFG(n),                           \
		.tar_dev_cfg     = MSPI_DEVICE_CONFIG_DT_INST(n),                        \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_XIP,                           \
					      tar_xip_cfg,                               \
					      MSPI_XIP_CONFIG_DT_INST(n))                \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_SCRAMBLE,                      \
					      tar_scramble_cfg,                          \
					      MSPI_SCRAMBLE_CONFIG_DT_INST(n))           \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_TIMING,                        \
					      tar_timing_cfg,                            \
					      QSPI_PSRAM_TIMING_CONFIG(n))               \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_TIMING,                        \
					      timing_cfg_mask,                           \
					      QSPI_PSRAM_TIMING_CONFIG_MASK(n))          \
		MSPI_XIP_BASE_ADDR_INIT(xip_base_addr, DT_INST_BUS(n))                   \
		.sw_multi_periph    = DT_PROP_OR(DT_INST_BUS(n),                         \
						 software_multiperipheral, false),       \
		.pm_dev_rt_auto     = DT_INST_PROP(n, zephyr_pm_device_runtime_auto),    \
		.has_chip_variant   = DT_INST_NODE_HAS_PROP(n, chip_variant),            \
		.variant            = DT_INST_ENUM_IDX_OR(n, chip_variant,               \
						QSPI_PSRAM_VARIANT_GENERIC),             \
		.enter_qpi_cmd      = DT_INST_PROP(n, enter_qpi_cmd),                    \
		.exit_qpi_cmd       = DT_INST_PROP(n, exit_qpi_cmd),                     \
		.reset_en_cmd       = DT_INST_PROP(n, reset_en_cmd),                     \
		.reset_cmd          = DT_INST_PROP(n, reset_cmd),                        \
		.read_id_cmd        = DT_INST_PROP(n, read_id_cmd),                      \
	};                                                                               \
	static struct memc_mspi_qspi_psram_data                                          \
	memc_mspi_qspi_psram_data_##n = {                                                \
		.lock  = Z_SEM_INITIALIZER(                                              \
				memc_mspi_qspi_psram_data_##n.lock, 0, 1),               \
		.dummy = 0,                                                              \
	};                                                                               \
	PM_DEVICE_DT_INST_DEFINE(n, qspi_psram_pm_action);                               \
	DEVICE_DT_INST_DEFINE(n,                                                         \
			      memc_mspi_qspi_psram_init,                                 \
			      PM_DEVICE_DT_INST_GET(n),                                  \
			      &memc_mspi_qspi_psram_data_##n,                            \
			      &memc_mspi_qspi_psram_config_##n,                          \
			      POST_KERNEL,                                               \
			      CONFIG_MEMC_INIT_PRIORITY,                                 \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MSPI_QSPI_PSRAM)
