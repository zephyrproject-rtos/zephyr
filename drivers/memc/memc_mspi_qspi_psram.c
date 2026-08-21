/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qspi_psram

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/devicetree.h>

LOG_MODULE_REGISTER(memc_mspi_qspi_psram, CONFIG_MEMC_LOG_LEVEL);

/**
 * @brief Supported QPI PSRAM chip variants.
 *
 * Indices 0..N must match the chip-variant enum order in the DT binding.
 * GENERIC is the value used when no chip-variant is specified in DT; the
 * driver then uses standard QPI init commands but reads all transfer
 * parameters (read/write command, rx-dummy, cmd/addr length, CE timing)
 * from DT properties.
 */
enum qspi_psram_variant {
	QSPI_PSRAM_VARIANT_ESP64H,
	QSPI_PSRAM_VARIANT_IS66WVS4M8BLL,
	QSPI_PSRAM_VARIANT_IS66WVS8M8BLL,
	QSPI_PSRAM_VARIANT_GENERIC, /* must be last */
};

/*
 * DT enum indices for command-length and address-length (mspi-device.yaml).
 * Used to populate chip_params without depending on the DT binding headers.
 */
#define QSPI_PSRAM_CMD_LEN_1BYTE   1   /* "INSTR_1_BYTE" */
#define QSPI_PSRAM_ADDR_LEN_3BYTE  3   /* "ADDR_3_BYTE"  */

/**
 * @brief Per-chip parameters sourced from the device datasheet.
 *
 * These values are fixed for a given chip and are not user-configurable.
 * The driver applies them during initialization, overriding the matching
 * DT properties (read/write command, cmd/addr length, dummy cycles and
 * ce-break-config), so none of them has to be given in devicetree.
 *
 * Entries are held in one table indexed by enum qspi_psram_variant, of which
 * a build only carries the parts its devicetree names.
 */
struct qspi_psram_chip_params {
	uint32_t size_bits;           /* Capacity in bits, 0 in generic mode   */
	uint8_t  enter_qpi_cmd;       /* SPI command to enter QPI mode         */
	uint8_t  exit_qpi_cmd;        /* QPI command to exit back to SPI mode  */
	uint8_t  qspi_read_cmd;       /* Fast-read command in 4-line QPI mode  */
	uint8_t  qspi_write_cmd;      /* Write command in 4-line QPI mode      */
	uint8_t  spi_read_cmd;        /* Fast-read command in 1-line SPI mode  */
	uint8_t  spi_write_cmd;       /* Write command in 1-line SPI mode      */
	uint8_t  reset_en_cmd;        /* Reset Enable command                  */
	uint8_t  reset_cmd;           /* Reset command                         */
	uint8_t  read_id_cmd;         /* Read ID command (returns MF/KGD/EID)  */
	uint8_t  kgd_value;           /* Expected KGD byte in Read ID response */
	uint8_t  cmd_length;          /* Command length enum index (DT binding) */
	uint8_t  addr_length;         /* Address length enum index (DT binding) */
	uint8_t  qspi_rx_dummy;       /* Read dummy cycles in 4-line QPI mode  */
	uint8_t  spi_rx_dummy;        /* Read dummy cycles in 1-line SPI mode  */
	uint8_t  default_tx_dummy;    /* Recommended write dummy cycles        */
	uint16_t ce_max_burst_bytes;  /* Max bytes per CE cycle (mem_boundary) */
	uint32_t ce_refresh_us;       /* Max CE assertion time in us           */
};

/* Frequency used for initial SPI-mode register access before entering QPI */
#define QSPI_PSRAM_SPI_INIT_FREQ    24000000U

/* Minimum delay after software reset before first access (tRST) */
#define QSPI_PSRAM_RESET_DELAY_US   200U

/* Settling time after the mode switch command before the chip answers in QPI */
#define QSPI_PSRAM_QPI_ENTER_DELAY_US 100U

/* Expands to `1 ||` for every enabled node that selects the given variant */
#define QSPI_PSRAM_VARIANT_USED_OR(n, variant)                                    \
	DT_INST_ENUM_HAS_VALUE(n, chip_variant, variant) ||

/* 1 when at least one enabled node selects this chip variant, 0 otherwise */
#define QSPI_PSRAM_VARIANT_USED(variant)                                          \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(QSPI_PSRAM_VARIANT_USED_OR, variant) 0)

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_variant)
/*
 * Chip parameter table, one entry per supported part. Values come from device
 * datasheets and must not be changed without verifying against the relevant
 * datasheet revision.
 *
 * Only the parts named in devicetree are compiled in; a build that describes
 * the device in devicetree instead drops the table entirely.
 */
static const struct qspi_psram_chip_params chip_table[] = {
#if QSPI_PSRAM_VARIANT_USED(esp64h)
	[QSPI_PSRAM_VARIANT_ESP64H] = {
		/* Espressif ESP-PSRAM64H, 64 Mbit (8 MB), 3.3 V / 1.8 V     */
		/* Datasheet: tCEM = 8 us, page size = 1 KB, tCPH = 50 ns    */
		.size_bits          = 64U * 1024U * 1024U,
		.enter_qpi_cmd      = 0x35,
		.exit_qpi_cmd       = 0xF5,
		.qspi_read_cmd      = 0xEB,
		.qspi_write_cmd     = 0x38,
		.spi_read_cmd       = 0x0B,
		.spi_write_cmd      = 0x02,
		.reset_en_cmd       = 0x66,
		.reset_cmd          = 0x99,
		.read_id_cmd        = 0x9F,
		.kgd_value          = 0x5D,
		.cmd_length         = QSPI_PSRAM_CMD_LEN_1BYTE,
		.addr_length        = QSPI_PSRAM_ADDR_LEN_3BYTE,
		.qspi_rx_dummy      = 6,
		.spi_rx_dummy       = 8,
		.default_tx_dummy   = 0,
		.ce_max_burst_bytes = 1024,
		.ce_refresh_us      = 8,
	},
#endif
#if QSPI_PSRAM_VARIANT_USED(is66wvs4m8bll)
	[QSPI_PSRAM_VARIANT_IS66WVS4M8BLL] = {
		/* ISSI IS66WVS4M8BLL, 32 Mbit (4 MB)                        */
		/* Datasheet: tCEM = 4 us up to 85 C, page size = 1 KB.      */
		/* Parts run above 85 C need 1 us and must set ce-break-     */
		/* config in devicetree instead of using this variant.       */
		.size_bits          = 32U * 1024U * 1024U,
		.enter_qpi_cmd      = 0x35,
		.exit_qpi_cmd       = 0xF5,
		.qspi_read_cmd      = 0xEB,
		.qspi_write_cmd     = 0x38,
		.spi_read_cmd       = 0x0B,
		.spi_write_cmd      = 0x02,
		.reset_en_cmd       = 0x66,
		.reset_cmd          = 0x99,
		.read_id_cmd        = 0x9F,
		.kgd_value          = 0x5D,
		.cmd_length         = QSPI_PSRAM_CMD_LEN_1BYTE,
		.addr_length        = QSPI_PSRAM_ADDR_LEN_3BYTE,
		.qspi_rx_dummy      = 6,
		.spi_rx_dummy       = 8,
		.default_tx_dummy   = 0,
		.ce_max_burst_bytes = 1024,
		.ce_refresh_us      = 4,
	},
#endif
#if QSPI_PSRAM_VARIANT_USED(is66wvs8m8bll)
	[QSPI_PSRAM_VARIANT_IS66WVS8M8BLL] = {
		/* ISSI IS66WVS8M8BLL, 64 Mbit (8 MB)                        */
		/* Datasheet: tCEM = 4 us up to 85 C, page size = 1 KB.      */
		/* Parts run above 85 C need 1 us and must set ce-break-     */
		/* config in devicetree instead of using this variant.       */
		.size_bits          = 64U * 1024U * 1024U,
		.enter_qpi_cmd      = 0x35,
		.exit_qpi_cmd       = 0xF5,
		.qspi_read_cmd      = 0xEB,
		.qspi_write_cmd     = 0x38,
		.spi_read_cmd       = 0x0B,
		.spi_write_cmd      = 0x02,
		.reset_en_cmd       = 0x66,
		.reset_cmd          = 0x99,
		.read_id_cmd        = 0x9F,
		.kgd_value          = 0x5D,
		.cmd_length         = QSPI_PSRAM_CMD_LEN_1BYTE,
		.addr_length        = QSPI_PSRAM_ADDR_LEN_3BYTE,
		.qspi_rx_dummy      = 6,
		.spi_rx_dummy       = 8,
		.default_tx_dummy   = 0,
		.ce_max_burst_bytes = 1024,
		.ce_refresh_us      = 4,
	},
#endif
};

BUILD_ASSERT(ARRAY_SIZE(chip_table) <= QSPI_PSRAM_VARIANT_GENERIC,
	     "chip_table must not hold more entries than the chip-variant enum");
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY(chip_variant) */

/*
 * Resolve a node to its table entry at build time, so an instance that
 * describes the device in devicetree never pulls the table into the image.
 */
#define QSPI_PSRAM_CHIP_PARAMS(n)                                                 \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, chip_variant),                       \
		    (&chip_table[DT_INST_ENUM_IDX(n, chip_variant)]), (NULL))

struct memc_mspi_qspi_psram_config {
	uint32_t                    port;
	uint32_t                    mem_size;

	const struct device        *bus;
	struct mspi_dev_id          dev_id;
	struct mspi_dev_cfg         spi_init_cfg;
	struct mspi_dev_cfg         tar_dev_cfg;

	MSPI_MEMMAP_CFG_STRUCT_DECLARE(tar_memmap_cfg)
	MSPI_SCRAMBLE_CFG_STRUCT_DECLARE(tar_scramble_cfg)
	MSPI_MEMMAP_BASE_ADDR_DECLARE(memmap_base_addr)

	bool                        sw_multi_periph;
	bool                        pm_dev_rt_auto;

	/* Table entry of the chip named in DT, NULL in generic mode */
	const struct qspi_psram_chip_params *chip;

	/*
	 * Generic-mode init commands. Read with a fallback so the driver also
	 * works on nodes whose binding is selected by another compatible, such
	 * as a vendor MSPI device binding listed first.
	 */
	uint8_t                     enter_qpi_cmd;
	uint8_t                     exit_qpi_cmd;
	uint8_t                     reset_en_cmd;
	uint8_t                     reset_cmd;
	uint8_t                     read_id_cmd;
};

struct memc_mspi_qspi_psram_data {
	struct mspi_dev_cfg         dev_cfg;
	struct mspi_memmap_cfg      memmap_cfg;
	struct mspi_xfer            trans;
	struct mspi_xfer_packet     packet;

	struct k_sem                lock;
	uint32_t                    mem_size;
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
		LOG_ERR("MSPI write transaction failed: %d", ret);
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
		LOG_ERR("MSPI read transaction failed: %d", ret);
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
		LOG_ERR("Failed to send Reset Enable");
		return ret;
	}

	ret = qspi_psram_command_write(psram, chip->reset_cmd, 0,
				       (uint8_t *)&data->dummy, 0);
	if (ret) {
		LOG_ERR("Failed to send Reset");
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
	data->dev_cfg.addr_length = QSPI_PSRAM_ADDR_LEN_3BYTE;

	ret = qspi_psram_command_read(psram, chip->read_id_cmd, 0, id, sizeof(id));

	data->dev_cfg.addr_length = saved_addr_length;

	if (ret) {
		LOG_ERR("Failed to read chip ID");
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
		LOG_ERR("KGD mismatch: expected 0x%02X got 0x%02X", chip->kgd_value, id[1]);
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
#if CONFIG_MSPI_MEMMAP
		if (data->memmap_cfg.enable) {
			ret = mspi_memmap_config(cfg->bus, &cfg->dev_id, &data->memmap_cfg);
			if (ret) {
				LOG_ERR("Failed to re-enable memory mapping");
			}
		}
#endif
		break;

	case PM_DEVICE_ACTION_SUSPEND:
#if CONFIG_MSPI_MEMMAP
		if (data->memmap_cfg.enable) {
			sys_cache_data_flush_and_invd_all();
			struct mspi_memmap_cfg memmap_off = data->memmap_cfg;

			memmap_off.enable = false;
			ret = mspi_memmap_config(cfg->bus, &cfg->dev_id, &memmap_off);
			if (ret) {
				LOG_ERR("Failed to disable memory mapping");
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
 * Sequence:
 *   1. Exit QPI (0xF5) sent in 4-line mode
 *   2. Reset Enable + Reset sent in 4-line mode (clean up any QPI state)
 *   3. Restore 1-line SPI mode - subsequent SPI reset/ID commands are clean
 * Failures of the three commands are ignored on purpose: a chip that is
 * already in SPI mode does not decode them, they only put garbage on the bus.
 * A failure to configure the controller is a real error and is returned.
 */
static int qspi_psram_force_spi_mode(const struct device *psram,
				     const struct qspi_psram_chip_params *chip)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;
	struct mspi_dev_cfg qpi_cfg = cfg->spi_init_cfg;

	qpi_cfg.io_mode = MSPI_IO_MODE_QUAD;

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &qpi_cfg)) {
		LOG_ERR("Failed to select 4-line mode for the QPI exit");
		return -EIO;
	}
	data->dev_cfg = qpi_cfg;

	/* Exit QPI -> chip returns to SPI mode if it was in QPI */
	qspi_psram_command_write(psram, chip->exit_qpi_cmd, 0,
				 (uint8_t *)&data->dummy, 0);

	/* Reset while still in QPI (handles chips in inconsistent state) */
	qspi_psram_command_write(psram, chip->reset_en_cmd, 0,
				 (uint8_t *)&data->dummy, 0);
	qspi_psram_command_write(psram, chip->reset_cmd, 0,
				 (uint8_t *)&data->dummy, 0);
	k_busy_wait(QSPI_PSRAM_RESET_DELAY_US);

	if (mspi_dev_config(cfg->bus, &cfg->dev_id,
			    MSPI_DEVICE_CONFIG_ALL, &cfg->spi_init_cfg)) {
		LOG_ERR("Failed to restore SPI mode after QPI exit");
		return -EIO;
	}
	data->dev_cfg = cfg->spi_init_cfg;
	return 0;
}

/**
 * @brief Validate the devicetree description of the device.
 *
 * Parameters the driver cannot guess must be present in generic mode,
 * otherwise the mspi-device defaults (no command phase, no address phase)
 * are programmed into the controller and every access fails silently.
 */
static int qspi_psram_check_dt_cfg(const struct memc_mspi_qspi_psram_config *cfg)
{
	/* These parts have four data lines: 1-1-1 (SPI) or 4-4-4 (QPI), nothing else */
	if (cfg->tar_dev_cfg.io_mode != MSPI_IO_MODE_SINGLE &&
	    cfg->tar_dev_cfg.io_mode != MSPI_IO_MODE_QUAD) {
		LOG_ERR("mspi-io-mode %d not supported, use SINGLE or QUAD",
			cfg->tar_dev_cfg.io_mode);
		return -ENOTSUP;
	}

	if (cfg->tar_dev_cfg.data_rate != MSPI_DATA_RATE_SINGLE) {
		LOG_ERR("Only MSPI_DATA_RATE_SINGLE supported, got %d", cfg->tar_dev_cfg.data_rate);
		return -EIO;
	}

	/* Everything below comes from the chip table for a known variant. */
	if (cfg->chip != NULL) {
		return 0;
	}

	if (cfg->tar_dev_cfg.read_cmd == 0 || cfg->tar_dev_cfg.write_cmd == 0) {
		LOG_ERR("Generic mode requires read-command and write-command in DT");
		return -EINVAL;
	}

	/* 0 means INSTR_DISABLED / ADDR_DISABLED */
	if (cfg->tar_dev_cfg.cmd_length == 0 || cfg->tar_dev_cfg.addr_length == 0) {
		LOG_ERR("Generic mode requires command-length and address-length in DT");
		return -EINVAL;
	}

	if (cfg->tar_dev_cfg.rx_dummy == 0) {
		LOG_WRN("Generic mode: rx-dummy is 0, reads are unlikely to work");
	}

	return 0;
}

static int memc_mspi_qspi_psram_init(const struct device *psram)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;
	struct qspi_psram_chip_params generic_params;
	const struct qspi_psram_chip_params *chip;
	const bool quad = (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_QUAD);
	uint32_t mem_size;
	int ret;

	if (cfg->chip != NULL) {
		chip = cfg->chip;
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
		LOG_ERR("MSPI controller not ready");
		return -ENODEV;
	}

	/*
	 * The capacity of a known chip variant is a property of the part, so it
	 * comes from the chip table; generic mode has to be told in devicetree.
	 */
	mem_size = (cfg->chip != NULL) ? (chip->size_bits / 8) : cfg->mem_size;
	if (mem_size == 0) {
		LOG_ERR("Generic mode requires size in DT");
		return -EINVAL;
	}
	data->mem_size = mem_size;

	ret = qspi_psram_check_dt_cfg(cfg);
	if (ret) {
		return ret;
	}

	/*
	 * Bring the chip to SPI mode in case a previous boot left it in QPI.
	 * This also leaves the controller configured for the 1-line init-phase
	 * commands that follow.
	 */
	ret = qspi_psram_force_spi_mode(psram, chip);
	if (ret) {
		LOG_ERR("Failed to force SPI mode");
		return ret;
	}

	ret = qspi_psram_reset(psram, chip);
	if (ret) {
		LOG_ERR("Failed to reset PSRAM");
		return -EIO;
	}

	ret = qspi_psram_verify_id(psram, chip);
	if (ret) {
		LOG_ERR("PSRAM ID verification failed");
		return -EIO;
	}

	/*
	 * In SPI mode the chip stays as it comes out of reset; QPI needs the
	 * mode switch command, sent single-wire.
	 */
	if (quad) {
		ret = qspi_psram_command_write(psram, chip->enter_qpi_cmd, 0,
					       (uint8_t *)&data->dummy, 0);
		k_busy_wait(QSPI_PSRAM_QPI_ENTER_DELAY_US);
		if (ret) {
			LOG_ERR("Failed to enter QPI mode");
			return -EIO;
		}
	}

	/*
	 * Build the runtime device config from DT target values.
	 * For known chip variants: override read/write commands and CE timing
	 * with the datasheet values for the selected bus width, ignoring
	 * whatever was in DT. For generic mode: all transfer parameters come
	 * from DT as-is.
	 */
	data->dev_cfg = cfg->tar_dev_cfg;
	if (cfg->chip != NULL) {
		data->dev_cfg.read_cmd      = quad ? chip->qspi_read_cmd : chip->spi_read_cmd;
		data->dev_cfg.write_cmd     = quad ? chip->qspi_write_cmd : chip->spi_write_cmd;
		data->dev_cfg.cmd_length    = chip->cmd_length;
		data->dev_cfg.addr_length   = chip->addr_length;
		data->dev_cfg.rx_dummy      = quad ? chip->qspi_rx_dummy : chip->spi_rx_dummy;
		data->dev_cfg.tx_dummy      = chip->default_tx_dummy;
		data->dev_cfg.mem_boundary  = chip->ce_max_burst_bytes;
		data->dev_cfg.time_to_break = chip->ce_refresh_us;
	}

	ret = mspi_dev_config(cfg->bus, &cfg->dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &data->dev_cfg);
	if (ret) {
		LOG_ERR("Failed to apply the target device config");
		return -EIO;
	}

#if CONFIG_MSPI_MEMMAP
	if (cfg->tar_memmap_cfg.enable) {
		if (cfg->tar_memmap_cfg.size > mem_size) {
			LOG_ERR("memmap-config size %u exceeds device size %u",
				cfg->tar_memmap_cfg.size, mem_size);
			return -EINVAL;
		}

		ret = mspi_memmap_config(cfg->bus, &cfg->dev_id, &cfg->tar_memmap_cfg);
		if (ret) {
			LOG_ERR("Failed to enable memory mapping");
			return -EIO;
		}
		data->memmap_cfg = cfg->tar_memmap_cfg;
	}
#endif

#if CONFIG_MSPI_SCRAMBLE
	if (cfg->tar_scramble_cfg.enable) {
		ret = mspi_scramble_config(cfg->bus, &cfg->dev_id,
					   &cfg->tar_scramble_cfg);
		if (ret) {
			LOG_ERR("Failed to enable scrambling");
			return -EIO;
		}
	}
#endif

	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME) || !cfg->pm_dev_rt_auto) {
		qspi_psram_release(psram);
	}

	LOG_INF("PSRAM initialised in %s mode, %u KB", quad ? "QPI" : "SPI", mem_size / 1024);

	return 0;
}

static int memc_mspi_qspi_psram_get_size(const struct device *psram, uint64_t *size)
{
	struct memc_mspi_qspi_psram_data *data = psram->data;

	*size = data->mem_size;
	return 0;
}

#if CONFIG_MSPI_MEMMAP
static void *memc_mspi_qspi_psram_get_mem_base(const struct device *psram)
{
	const struct memc_mspi_qspi_psram_config *cfg = psram->config;
	struct memc_mspi_qspi_psram_data *data = psram->data;

	/* The window only holds the device once memory mapping is configured */
	return data->memmap_cfg.enable ? (void *)cfg->memmap_base_addr : NULL;
}
#endif

static DEVICE_API(memc, memc_mspi_qspi_psram_api) = {
	.get_size = memc_mspi_qspi_psram_get_size,
#if CONFIG_MSPI_MEMMAP
	.get_mem_base = memc_mspi_qspi_psram_get_mem_base,
#endif
};

/*
 * Fixed SPI-mode config used only during the chip initialisation sequence.
 * addr_length = 0 (ADDR_DISABLED) so no address bytes are emitted for
 * Reset Enable, Reset, Read ID and Enter QPI commands.
 */
#define QSPI_PSRAM_SPI_INIT_CFG(n)                                           \
	{                                                                    \
		.ce_num        = DT_INST_PROP_OR(n, mspi_hardware_ce_num, 0), \
		.freq          = QSPI_PSRAM_SPI_INIT_FREQ,                   \
		.io_mode       = MSPI_IO_MODE_SINGLE,                        \
		.data_rate     = MSPI_DATA_RATE_SINGLE,                      \
		.cpp           = DT_INST_ENUM_IDX_OR(n, mspi_cpp_mode,       \
						     MSPI_CPP_MODE_0),       \
		.endian        = DT_INST_ENUM_IDX_OR(n, mspi_endian,         \
						MSPI_XFER_LITTLE_ENDIAN),    \
		.ce_polarity   = DT_INST_ENUM_IDX_OR(n, mspi_ce_polarity,    \
						     MSPI_CE_ACTIVE_LOW),    \
		.dqs_enable    = false, /* no DQS in the 1-line init phase */ \
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
		.mem_size        = DT_INST_PROP_OR(n, size, 0) / 8,                            \
		.bus             = DEVICE_DT_GET(DT_INST_BUS(n)),                        \
		.dev_id          = MSPI_DEVICE_ID_DT_INST(n),                            \
		.spi_init_cfg    = QSPI_PSRAM_SPI_INIT_CFG(n),                           \
		.tar_dev_cfg     = MSPI_DEVICE_CONFIG_DT_INST(n),                        \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_MEMMAP,                        \
					      tar_memmap_cfg,                            \
					      MSPI_MEMMAP_CONFIG_DT_INST(n))             \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_SCRAMBLE,                      \
					      tar_scramble_cfg,                          \
					      MSPI_SCRAMBLE_CONFIG_DT_INST(n))           \
		MSPI_MEMMAP_BASE_ADDR_INIT(memmap_base_addr, DT_INST_BUS(n))             \
		.sw_multi_periph    = DT_PROP_OR(DT_INST_BUS(n),                         \
						 software_multiperipheral, false),       \
		.pm_dev_rt_auto     = DT_INST_PROP(n, zephyr_pm_device_runtime_auto),    \
		.chip               = QSPI_PSRAM_CHIP_PARAMS(n),                         \
		.enter_qpi_cmd      = DT_INST_PROP(n, enter_qpi_cmd),                    \
		.exit_qpi_cmd       = DT_INST_PROP(n, exit_qpi_cmd),                     \
		.reset_en_cmd       = DT_INST_PROP(n, reset_en_cmd),                     \
		.reset_cmd          = DT_INST_PROP(n, reset_cmd),                        \
		.read_id_cmd        = DT_INST_PROP(n, read_id_cmd),                      \
	};                                                                               \
	                                                                                 \
	static struct memc_mspi_qspi_psram_data                                          \
	memc_mspi_qspi_psram_data_##n = {                                                \
		.lock  = Z_SEM_INITIALIZER(                                              \
				memc_mspi_qspi_psram_data_##n.lock, 0, 1),               \
		.dummy = 0,                                                              \
	};                                                                               \
	                                                                                 \
	PM_DEVICE_DT_INST_DEFINE(n, qspi_psram_pm_action);                               \
	                                                                                 \
	DEVICE_DT_INST_DEFINE(n,                                                         \
			      memc_mspi_qspi_psram_init,                                 \
			      PM_DEVICE_DT_INST_GET(n),                                  \
			      &memc_mspi_qspi_psram_data_##n,                            \
			      &memc_mspi_qspi_psram_config_##n,                          \
			      POST_KERNEL,                                               \
			      CONFIG_MEMC_INIT_PRIORITY,                                 \
			      &memc_mspi_qspi_psram_api);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MSPI_QSPI_PSRAM)
