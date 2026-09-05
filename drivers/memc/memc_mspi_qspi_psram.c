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
 * AUTO makes the driver identify the chip from its Read ID response and
 * take the parameters from the ID match table.
 * GENERIC is the value used when no chip-variant is specified in DT; the
 * driver then uses standard QPI init commands but reads all transfer
 * parameters (read/write command, rx-dummy, cmd/addr length, CE timing)
 * from DT properties.
 */
enum qspi_psram_variant {
	QSPI_PSRAM_VARIANT_ESP64H,
	QSPI_PSRAM_VARIANT_IS66WVS4M8BLL,
	QSPI_PSRAM_VARIANT_IS66WVS8M8BLL,
	QSPI_PSRAM_VARIANT_AUTO,
	QSPI_PSRAM_VARIANT_GENERIC, /* must be last */
};

/*
 * DT enum indices for command-length and address-length (mspi-device.yaml).
 * Used to populate chip_params without depending on the DT binding headers.
 */
#define QSPI_PSRAM_CMD_LEN_1BYTE   1   /* "INSTR_1_BYTE" */
#define QSPI_PSRAM_ADDR_LEN_3BYTE  3   /* "ADDR_3_BYTE"  */

/*
 * Read ID response layout is MF ID, KGD, EID[47:0]; five bytes cover every
 * bit that is stable for a given part (the density code sits in EID[47:45],
 * the rest of the EID is per-die manufacturing data or reserved).
 */
#define QSPI_PSRAM_ID_LEN          5
#define QSPI_PSRAM_KGD_PASS        0x5D
#define QSPI_PSRAM_KGD_FAIL        0x55

/* The two ID families on the market; every clone answers with one of these */
#define QSPI_PSRAM_MF_AP           0x0D  /* AP Memory, Espressif, IPUS, ... */
#define QSPI_PSRAM_MF_ISSI         0x9D

/*
 * Worst-grade tCEM per ID family, applied in AUTO mode: the temperature
 * grade is not readable from the ID (AP: 3 us for the 105 C parts,
 * ISSI: 1 us above 85 C).
 */
#define QSPI_PSRAM_WORST_TCEM_US(mf) (((mf) == QSPI_PSRAM_MF_ISSI) ? 1 : 3)

/**
 * @brief Per-chip parameters sourced from the device datasheet.
 *
 * These values are fixed for a given chip and are not user-configurable.
 * The driver applies them during initialization, overriding the matching
 * DT properties (read/write command, cmd/addr length, dummy cycles and
 * ce-break-config), so none of them has to be given in devicetree.
 *
 * Entries are held in one table indexed by enum qspi_psram_variant, of which
 * a build only carries the parts its devicetree names; the AUTO variant
 * carries them all and matches the chip against id/mask at init.
 *
 * A Read ID response matches an entry when every bit selected by mask is
 * equal in id and in the bytes read from the chip. Bits cleared in the mask
 * are ignored: EID[44:0] is per-die manufacturing data on AP-family parts
 * and reserved on ISSI, so two chips of the same model differ there.
 */
struct qspi_psram_chip_params {
	const char *name;             /* Part or family name for log output    */
	uint8_t  id[QSPI_PSRAM_ID_LEN];   /* Read ID pattern for AUTO matching */
	uint8_t  mask[QSPI_PSRAM_ID_LEN]; /* Significant ID bits; a hole left  */
					  /* by conditional compilation has an */
					  /* all-zero mask, never matched      */
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

/* 1 when at least one enabled node selects the AUTO chip variant */
#define QSPI_PSRAM_AUTO_USED QSPI_PSRAM_VARIANT_USED(auto)

/* 1 when any enabled node needs the chip table: a named part or AUTO */
#define QSPI_PSRAM_TABLE_USED                                                     \
	(QSPI_PSRAM_VARIANT_USED(esp64h) ||                                       \
	 QSPI_PSRAM_VARIANT_USED(is66wvs4m8bll) ||                                \
	 QSPI_PSRAM_VARIANT_USED(is66wvs8m8bll) ||                                \
	 QSPI_PSRAM_AUTO_USED)

/*
 * De-facto standard QPI PSRAM command set and phase layout, shared by both
 * ID families on the market; the dummy-cycle values are the common datasheet
 * numbers for operation up to 104 MHz.
 */
#define QSPI_PSRAM_STD_QPI_PARAMS                                                 \
	.enter_qpi_cmd      = 0x35,                                               \
	.exit_qpi_cmd       = 0xF5,                                               \
	.qspi_read_cmd      = 0xEB,                                               \
	.qspi_write_cmd     = 0x38,                                               \
	.spi_read_cmd       = 0x0B,                                               \
	.spi_write_cmd      = 0x02,                                               \
	.reset_en_cmd       = 0x66,                                               \
	.reset_cmd          = 0x99,                                               \
	.read_id_cmd        = 0x9F,                                               \
	.kgd_value          = QSPI_PSRAM_KGD_PASS,                                \
	.cmd_length         = QSPI_PSRAM_CMD_LEN_1BYTE,                           \
	.addr_length        = QSPI_PSRAM_ADDR_LEN_3BYTE,                          \
	.qspi_rx_dummy      = 6,                                                  \
	.spi_rx_dummy       = 8,                                                  \
	.default_tx_dummy   = 0,                                                  \
	.ce_max_burst_bytes = 1024

/*
 * A family row for AUTO matching: the only ID bits that are stable for a
 * given part are MF ID, KGD and the density code in EID[47:45] (the top
 * three bits of ID byte 2), hence the fixed FF FF E0 00 00 mask.
 * The grade of a detected chip is unknown by definition, so these rows
 * carry the worst-grade tCEM of their family.
 */
#define QSPI_PSRAM_AUTO_ENTRY(fam, mf, density, mbits)                            \
{                                                                                 \
	.name = fam " " #mbits " Mbit",                                           \
	.id   = { (mf), QSPI_PSRAM_KGD_PASS, (density) << 5, 0x00, 0x00 },        \
	.mask = { 0xFF, 0xFF, 0xE0, 0x00, 0x00 },                                 \
	.size_bits = (mbits) * 1024U * 1024U,                                     \
	QSPI_PSRAM_STD_QPI_PARAMS,                                                \
	.ce_refresh_us = QSPI_PSRAM_WORST_TCEM_US(mf),                            \
}

#if QSPI_PSRAM_TABLE_USED
/*
 * Chip parameter table, one entry per supported part. Values come from device
 * datasheets and must not be changed without verifying against the relevant
 * datasheet revision.
 *
 * Only the parts named in devicetree are compiled in; the AUTO variant pulls
 * in every entry, and a build that describes the device in devicetree instead
 * drops the table entirely.
 */
static const struct qspi_psram_chip_params chip_table[] = {
#if QSPI_PSRAM_VARIANT_USED(esp64h) || QSPI_PSRAM_AUTO_USED
	[QSPI_PSRAM_VARIANT_ESP64H] = {
		/* Espressif ESP-PSRAM64H, 64 Mbit (8 MB), 3.3 V / 1.8 V     */
		/* Datasheet: tCEM = 8 us, page size = 1 KB, tCPH = 50 ns    */
		.name               = "ESP-PSRAM64H",
		.id                 = { QSPI_PSRAM_MF_AP, QSPI_PSRAM_KGD_PASS,
					0x40, 0x00, 0x00 },
		.mask               = { 0xFF, 0xFF, 0xE0, 0x00, 0x00 },
		.size_bits          = 64U * 1024U * 1024U,
		QSPI_PSRAM_STD_QPI_PARAMS,
		.ce_refresh_us      = 8,
	},
#endif
#if QSPI_PSRAM_VARIANT_USED(is66wvs4m8bll) || QSPI_PSRAM_AUTO_USED
	[QSPI_PSRAM_VARIANT_IS66WVS4M8BLL] = {
		/* ISSI IS66WVS4M8BLL, 32 Mbit (4 MB)                        */
		/* Datasheet: tCEM = 4 us up to 85 C, page size = 1 KB.      */
		/* Parts run above 85 C need 1 us and must set ce-break-     */
		/* config in devicetree instead of using this variant.       */
		.name               = "IS66WVS4M8BLL",
		.id                 = { QSPI_PSRAM_MF_ISSI, QSPI_PSRAM_KGD_PASS,
					0x40, 0x00, 0x00 },
		.mask               = { 0xFF, 0xFF, 0xE0, 0x00, 0x00 },
		.size_bits          = 32U * 1024U * 1024U,
		QSPI_PSRAM_STD_QPI_PARAMS,
		.ce_refresh_us      = 4,
	},
#endif
#if QSPI_PSRAM_VARIANT_USED(is66wvs8m8bll) || QSPI_PSRAM_AUTO_USED
	[QSPI_PSRAM_VARIANT_IS66WVS8M8BLL] = {
		/* ISSI IS66WVS8M8BLL, 64 Mbit (8 MB)                        */
		/* Datasheet: tCEM = 4 us up to 85 C, page size = 1 KB.      */
		/* Parts run above 85 C need 1 us and must set ce-break-     */
		/* config in devicetree instead of using this variant.       */
		.name               = "IS66WVS8M8BLL",
		.id                 = { QSPI_PSRAM_MF_ISSI, QSPI_PSRAM_KGD_PASS,
					0x60, 0x00, 0x00 },
		.mask               = { 0xFF, 0xFF, 0xE0, 0x00, 0x00 },
		.size_bits          = 64U * 1024U * 1024U,
		QSPI_PSRAM_STD_QPI_PARAMS,
		.ce_refresh_us      = 4,
	},
#endif
#if QSPI_PSRAM_AUTO_USED
	/*
	 * Family rows for the densities no named variant covers, reachable
	 * only through ID matching.
	 */
	QSPI_PSRAM_AUTO_ENTRY("AP-family", QSPI_PSRAM_MF_AP,   0x0, 16),
	QSPI_PSRAM_AUTO_ENTRY("AP-family", QSPI_PSRAM_MF_AP,   0x1, 32),
	/* Density code of the 128 Mbit APS12804O is unverified on silicon */
	QSPI_PSRAM_AUTO_ENTRY("AP-family", QSPI_PSRAM_MF_AP,   0x3, 128),
	QSPI_PSRAM_AUTO_ENTRY("ISSI",      QSPI_PSRAM_MF_ISSI, 0x0, 8),
	QSPI_PSRAM_AUTO_ENTRY("ISSI",      QSPI_PSRAM_MF_ISSI, 0x1, 16),
	QSPI_PSRAM_AUTO_ENTRY("ISSI",      QSPI_PSRAM_MF_ISSI, 0x4, 128),
#endif
};

#if !QSPI_PSRAM_AUTO_USED
BUILD_ASSERT(ARRAY_SIZE(chip_table) <= QSPI_PSRAM_VARIANT_AUTO,
	     "chip_table must only hold entries for concrete chip variants");
#endif
#endif /* QSPI_PSRAM_TABLE_USED */

/*
 * Resolve a node to its table entry at build time, so an instance that
 * describes the device in devicetree never pulls the table into the image.
 * AUTO resolves to NULL: its parameters come from the ID match at run time.
 */
#define QSPI_PSRAM_CHIP_PARAMS(n)                                                 \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, chip_variant, auto), (NULL),        \
		(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, chip_variant),              \
			     (&chip_table[DT_INST_ENUM_IDX(n, chip_variant)]),    \
			     (NULL))))

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

	/* Identify the chip from its Read ID response at init */
	bool                        auto_detect;

	/* Table entry of the chip named in DT, NULL in generic and AUTO modes */
	const struct qspi_psram_chip_params *chip;

	/*
	 * Init-phase commands used in generic and AUTO modes, taken from the
	 * binding defaults unless devicetree overrides them.
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

/**
 * @brief Issue Read ID and fill @p id with the first @p len response bytes.
 *
 * Read ID (0x9F) requires a 24-bit don't-care address phase in SPI mode on
 * every supported chip. The init-phase config has addr_length = 0
 * (ADDR_DISABLED) for the reset commands which need no address, so it is
 * overridden for this transaction only.
 */
static int qspi_psram_read_id(const struct device *psram, uint8_t read_id_cmd,
			      uint8_t *id, uint32_t len)
{
	struct memc_mspi_qspi_psram_data *data = psram->data;
	uint8_t saved_addr_length;
	int ret;

	saved_addr_length = data->dev_cfg.addr_length;
	data->dev_cfg.addr_length = QSPI_PSRAM_ADDR_LEN_3BYTE;

	ret = qspi_psram_command_read(psram, read_id_cmd, 0, id, len);

	data->dev_cfg.addr_length = saved_addr_length;

	if (ret) {
		LOG_ERR("Failed to read chip ID");
	}
	return ret;
}

/**
 * @brief Compare the Read ID response with the part named in devicetree.
 *
 * Purely informational: a KGD or identity mismatch is only logged, and the
 * driver carries on with the devicetree choice. Fitting a chip that fails
 * these checks may be deliberate, so the decision stays with the user.
 * Only a failed Read ID transaction is an error.
 */
static int qspi_psram_verify_id(const struct device *psram,
				const struct qspi_psram_chip_params *chip)
{
	uint8_t id[QSPI_PSRAM_ID_LEN] = {0};
	bool identity_ok = true;
	int ret;

	ret = qspi_psram_read_id(psram, chip->read_id_cmd, id, sizeof(id));
	if (ret) {
		return ret;
	}

	LOG_DBG("PSRAM ID: %02X %02X %02X %02X %02X", id[0], id[1], id[2], id[3], id[4]);

	if (chip->kgd_value == 0) {
		/* Generic mode: nothing to compare against, log and continue */
		LOG_INF("Generic PSRAM: skipping KGD check (ID MF=0x%02X KGD=0x%02X EID=0x%02X)",
			id[0], id[1], id[2]);
		return 0;
	}

	if (id[1] != chip->kgd_value) {
		LOG_WRN("KGD 0x%02X differs from the expected 0x%02X: the die may have "
			"failed the factory test", id[1], chip->kgd_value);
	}

	/* KGD is reported above; compare the remaining significant ID bits */
	for (size_t i = 0; i < QSPI_PSRAM_ID_LEN; i++) {
		if (i != 1 && (id[i] & chip->mask[i]) != (chip->id[i] & chip->mask[i])) {
			identity_ok = false;
		}
	}
	if (!identity_ok) {
		LOG_WRN("chip-variant says %s (%02X %02X %02X) but the chip answers "
			"%02X %02X %02X; continuing with the devicetree choice",
			chip->name, chip->id[0], chip->id[1], chip->id[2],
			id[0], id[1], id[2]);
	}

	return 0;
}

#if QSPI_PSRAM_AUTO_USED
/**
 * @brief Identify the chip from its Read ID response.
 *
 * Must run in SPI mode after the reset sequence: AP-family parts only
 * guarantee Read ID as a power-up initialization step, and a chip left in
 * QPI mode does not decode the command at all.
 *
 * On a match @p chip_out points at the parameters of the matched entry.
 */
static int qspi_psram_auto_detect(const struct device *psram, uint8_t read_id_cmd,
				  const struct qspi_psram_chip_params **chip_out)
{
	uint8_t id[QSPI_PSRAM_ID_LEN] = {0};
	bool all_zero = true;
	bool all_ones = true;
	int ret;

	ret = qspi_psram_read_id(psram, read_id_cmd, id, sizeof(id));
	if (ret) {
		return ret;
	}

	for (size_t e = 0; e < ARRAY_SIZE(chip_table); e++) {
		const struct qspi_psram_chip_params *entry = &chip_table[e];
		bool match = true;

		/*
		 * A hole left in the table by conditional compilation has an
		 * all-zero mask and would match any response; the MF bits are
		 * always significant in a real entry.
		 */
		if (entry->mask[0] == 0) {
			continue;
		}

		for (size_t i = 0; i < QSPI_PSRAM_ID_LEN; i++) {
			if ((id[i] & entry->mask[i]) !=
			    (entry->id[i] & entry->mask[i])) {
				match = false;
				break;
			}
		}

		if (match) {
			LOG_INF("Detected %s (ID %02X %02X %02X %02X %02X)",
				entry->name, id[0], id[1], id[2], id[3], id[4]);
			*chip_out = entry;
			return 0;
		}
	}

	for (size_t i = 0; i < QSPI_PSRAM_ID_LEN; i++) {
		all_zero = all_zero && (id[i] == 0x00);
		all_ones = all_ones && (id[i] == 0xFF);
	}

	if (all_zero || all_ones) {
		LOG_ERR("No response from the PSRAM (bus reads 0x%02X)", id[0]);
	} else if (id[1] == QSPI_PSRAM_KGD_FAIL) {
		LOG_ERR("PSRAM die failed the factory test (KGD 0x%02X)", id[1]);
	} else {
		LOG_ERR("Unknown PSRAM ID: %02X %02X %02X %02X %02X",
			id[0], id[1], id[2], id[3], id[4]);
	}

	return -ENODEV;
}
#endif /* QSPI_PSRAM_AUTO_USED */

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

	/*
	 * Everything below comes from the chip table for a known variant, or
	 * from the ID match in AUTO mode.
	 */
	if (cfg->chip != NULL || cfg->auto_detect) {
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
	bool from_table = (cfg->chip != NULL);
	uint32_t mem_size;
	int ret;

	if (cfg->chip != NULL) {
		chip = cfg->chip;
	} else {
		/*
		 * Build chip params from DT-configurable fields; in generic
		 * mode the transfer commands and CE timing are filled later
		 * from tar_dev_cfg, in AUTO mode the whole struct is replaced
		 * by the matched table entry below.
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

	if (cfg->auto_detect) {
#if QSPI_PSRAM_AUTO_USED
		ret = qspi_psram_auto_detect(psram, chip->read_id_cmd, &chip);
#else
		ret = -ENOTSUP;
#endif
		if (ret) {
			LOG_ERR("PSRAM auto detection failed");
			return ret;
		}
		from_table = true;
	} else {
		ret = qspi_psram_verify_id(psram, chip);
		if (ret) {
			LOG_ERR("Failed to read the PSRAM ID");
			return -EIO;
		}
	}

	/*
	 * The capacity of a known or detected chip is a property of the part,
	 * so it comes from the table; generic mode has to be told in
	 * devicetree.
	 */
	mem_size = from_table ? (chip->size_bits / 8) : cfg->mem_size;
	if (mem_size == 0) {
		LOG_ERR("Generic mode requires size in DT");
		return -EINVAL;
	}
	if (cfg->auto_detect && cfg->mem_size != 0 && cfg->mem_size != mem_size) {
		LOG_WRN("DT size %u B differs from the detected %u B; using the detected size",
			cfg->mem_size, mem_size);
	}
	data->mem_size = mem_size;

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
	 * For known and detected chips: override read/write commands and CE
	 * timing with the datasheet values for the selected bus width,
	 * ignoring whatever was in DT. For generic mode: all transfer
	 * parameters come from DT as-is.
	 */
	data->dev_cfg = cfg->tar_dev_cfg;
	if (from_table) {
		data->dev_cfg.read_cmd      = quad ? chip->qspi_read_cmd : chip->spi_read_cmd;
		data->dev_cfg.write_cmd     = quad ? chip->qspi_write_cmd : chip->spi_write_cmd;
		data->dev_cfg.cmd_length    = chip->cmd_length;
		data->dev_cfg.addr_length   = chip->addr_length;
		data->dev_cfg.rx_dummy      = quad ? chip->qspi_rx_dummy : chip->spi_rx_dummy;
		data->dev_cfg.tx_dummy      = chip->default_tx_dummy;
		data->dev_cfg.mem_boundary  = chip->ce_max_burst_bytes;
		data->dev_cfg.time_to_break = chip->ce_refresh_us;

		/*
		 * The ID does not reveal the temperature grade, so AUTO
		 * replaces the matched entry's standard-grade CE limit with
		 * the worst grade of its family. A board that knows its part
		 * may relax the limits via ce-break-config; each cell is
		 * taken only when set, so a zero cell keeps the table value
		 * instead of disabling that protection.
		 */
		if (cfg->auto_detect) {
			data->dev_cfg.time_to_break =
				QSPI_PSRAM_WORST_TCEM_US(chip->id[0]);
			if (cfg->tar_dev_cfg.mem_boundary != 0) {
				data->dev_cfg.mem_boundary = cfg->tar_dev_cfg.mem_boundary;
			}
			if (cfg->tar_dev_cfg.time_to_break != 0) {
				data->dev_cfg.time_to_break = cfg->tar_dev_cfg.time_to_break;
			}
		}
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
		.auto_detect        = DT_INST_ENUM_HAS_VALUE(n, chip_variant, auto),     \
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
