/*
 * SPDX-FileCopyrightText: 2026 Seunghoon Park
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Rockchip RK3568/RK3566 SD/MMC host controller (DesignWare Mobile Storage
 * Host Controller).
 *
 * Written from the register description and the programming procedures in
 * the Rockchip RK3568 TRM Part 2, chapter "Mobile Storage Host Controller",
 * and the CRU description in the RK3568 TRM Part 1.
 *
 * Scope: SD cards on a 1-bit or 4-bit bus at 3.3 V, default and high speed
 * (up to 50 MHz). There is no 1.8 V signalling, UHS mode, tuning or eMMC
 * support. The controller is polled and data is moved by the CPU through
 * the FIFO.
 *
 * Pin muxing, the card power supply, the controller bus clock gate and the
 * card clock phase are assumed to have been set up by the boot loader.
 */

#define DT_DRV_COMPAT rockchip_rk3568_dw_mshc

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sdhc_rockchip_dw_mshc, CONFIG_SDHC_LOG_LEVEL);

/* Controller registers (TRM "Register Summary") */
#define MSHC_CTRL    0x000
#define MSHC_PWREN   0x004
#define MSHC_CLKDIV  0x008
#define MSHC_CLKSRC  0x00c
#define MSHC_CLKENA  0x010
#define MSHC_TMOUT   0x014
#define MSHC_CTYPE   0x018
#define MSHC_BLKSIZ  0x01c
#define MSHC_BYTCNT  0x020
#define MSHC_INTMASK 0x024
#define MSHC_CMDARG  0x028
#define MSHC_CMD     0x02c
#define MSHC_RESP0   0x030
#define MSHC_RESP1   0x034
#define MSHC_RESP2   0x038
#define MSHC_RESP3   0x03c
#define MSHC_RINTSTS 0x044
#define MSHC_STATUS  0x048
#define MSHC_FIFOTH  0x04c
#define MSHC_CDETECT 0x050
#define MSHC_UHSREG  0x074
#define MSHC_FIFO    0x200

/* SDMMC_CTRL */
#define CTRL_CONTROLLER_RESET BIT(0)
#define CTRL_FIFO_RESET       BIT(1)
#define CTRL_DMA_RESET        BIT(2)

/* SDMMC_CLKENA */
#define CLKENA_ENABLE    BIT(0)
#define CLKENA_LOW_POWER BIT(16)

/* SDMMC_CTYPE */
#define CTYPE_4BIT BIT(0)
#define CTYPE_8BIT BIT(16)

/* SDMMC_CMD */
#define CMD_INDEX_MASK      GENMASK(5, 0)
#define CMD_RESPONSE_EXPECT BIT(6)
#define CMD_RESPONSE_LONG   BIT(7)
#define CMD_CHECK_RESP_CRC  BIT(8)
#define CMD_DATA_EXPECTED   BIT(9)
#define CMD_WRITE           BIT(10)
#define CMD_SEND_AUTO_STOP  BIT(12)
#define CMD_WAIT_PRVDATA    BIT(13)
#define CMD_STOP_ABORT      BIT(14)
#define CMD_SEND_INIT       BIT(15)
#define CMD_UPDATE_CLOCK    BIT(21)
#define CMD_USE_HOLD_REG    BIT(29)
#define CMD_START           BIT(31)

/* SDMMC_RINTSTS */
#define INT_RE   BIT(1)
#define INT_CD   BIT(2)
#define INT_DTO  BIT(3)
#define INT_TXDR BIT(4)
#define INT_RXDR BIT(5)
#define INT_RCRC BIT(6)
#define INT_DCRC BIT(7)
#define INT_RTO  BIT(8)
#define INT_DRTO BIT(9)
#define INT_HTO  BIT(10)
#define INT_FRUN BIT(11)
#define INT_HLE  BIT(12)
#define INT_SBE  BIT(13)
#define INT_ACD  BIT(14)
#define INT_EBE  BIT(15)
#define INT_ALL  0xffffffffU

#define INT_DATA_ERR (INT_EBE | INT_SBE | INT_HLE | INT_FRUN | INT_DCRC)
#define INT_DATA_TMO (INT_HTO | INT_DRTO)

/* SDMMC_STATUS */
#define STATUS_DATA_BUSY      BIT(9)
#define STATUS_FIFO_COUNT_POS 17
#define STATUS_FIFO_COUNT_MSK GENMASK(29, 17)

/* SDMMC_FIFOTH */
#define FIFOTH_MSIZE_8     (2U << 28)
#define FIFOTH_RX_WMARK(x) (((x) & GENMASK(11, 0)) << 16)
#define FIFOTH_TX_WMARK(x) ((x) & GENMASK(11, 0))

/* SDMMC_CDETECT: 0 means a card is present */
#define CDETECT_NO_CARD BIT(0)

/*
 * CRU_CLKSEL_CON30 (RK3568 TRM Part 1): bits [10:8] select the card clock
 * source of controller 0. The upper 16 bits are write enables for the lower
 * 16 bits.
 */
#define CRU_CLKSEL_CON30      0x178
#define CRU_SDMMC0_SEL_SHIFT  8
#define CRU_SDMMC0_SEL_MASK   (0x7U << CRU_SDMMC0_SEL_SHIFT)
#define CRU_HIWORD(mask, val) (((mask) << 16) | (val))

#define DW_RESET_TIMEOUT_MS      100
#define DW_CMD_ACCEPT_TIMEOUT_MS 100
#define DW_BUSY_TIMEOUT_MS       500
#define DW_DEFAULT_CMD_TMO_MS    1000
#define DW_DEFAULT_DATA_TMO_MS   2000
#define DW_PER_BLOCK_TMO_MS      10

/*
 * Card clock choices. The CRU only offers fixed source frequencies. The
 * "card clock" column assumes that the controller divides the CRU clock by
 * two before it reaches the card clock domain, with SDMMC_CLKDIV left at
 * 0 (bypass). That divide-by-two is an assumption: the TRM does not state
 * it. It fits the TRM's use_hold_reg table (50 MHz card clock with a divider
 * of 0), the 375 kHz identification clock obtained from the 750 kHz source,
 * and a throughput measured on this SoC that can only be reached with a
 * card clock above 25 MHz. The 12 MHz and 25 MHz choices have not been
 * checked on hardware.
 */
struct dw_mshc_clk_choice {
	uint32_t freq;
	uint8_t sel;
};

static const struct dw_mshc_clk_choice dw_mshc_clk_choices[] = {
	{375000, 5},   /* 750 kHz oscillator divider */
	{12000000, 0}, /* 24 MHz oscillator */
	{25000000, 4}, /* 50 MHz PLL branch */
	{50000000, 3}, /* 100 MHz PLL branch */
};

struct dw_mshc_config {
	DEVICE_MMIO_ROM;
	const struct device *cru;
	uint32_t fifo_depth;
	uint32_t min_freq;
	uint32_t max_freq;
	uint32_t power_delay_ms;
	uint32_t max_current_330;
	uint8_t bus_width;
};

struct dw_mshc_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
	/* Clock requested by the last set_io(), not the rate that was picked. */
	uint32_t card_clock_req;
	bool init_clocks_sent;
};

static inline mm_reg_t dw_mshc_base(const struct device *dev)
{
	return DEVICE_MMIO_GET(dev);
}

static inline uint32_t dw_rd(const struct device *dev, uint32_t off)
{
	return sys_read32(dw_mshc_base(dev) + off);
}

static inline void dw_wr(const struct device *dev, uint32_t off, uint32_t val)
{
	sys_write32(val, dw_mshc_base(dev) + off);
}

/*
 * Wait until (register & mask) == want. Spins briefly for the common fast
 * case, then sleeps between polls so long waits do not hog the CPU.
 */
static int dw_wait(const struct device *dev, uint32_t off, uint32_t mask, uint32_t want,
		   int timeout_ms)
{
	if (WAIT_FOR((dw_rd(dev, off) & mask) == want, 200, k_busy_wait(1))) {
		return 0;
	}

	return WAIT_FOR((dw_rd(dev, off) & mask) == want,
			(uint32_t)MAX(timeout_ms, 1) * USEC_PER_MSEC, k_msleep(1))
		       ? 0
		       : -ETIMEDOUT;
}

/* Wait until any bit of mask is set; the register value is returned in *val. */
static int dw_wait_any(const struct device *dev, uint32_t off, uint32_t mask, int timeout_ms,
		       uint32_t *val)
{
	return WAIT_FOR(((*val = dw_rd(dev, off)) & mask) != 0U,
			(uint32_t)MAX(timeout_ms, 1) * USEC_PER_MSEC, k_busy_wait(1))
		       ? 0
		       : -ETIMEDOUT;
}

/* Reset bits in SDMMC_CTRL clear themselves when the reset completes. */
static int dw_reset_bits(const struct device *dev, uint32_t bits)
{
	dw_wr(dev, MSHC_CTRL, dw_rd(dev, MSHC_CTRL) | bits);

	return dw_wait(dev, MSHC_CTRL, bits, 0, DW_RESET_TIMEOUT_MS);
}

/*
 * Write SDMMC_CMD and wait for the controller to take it (start_cmd clears).
 * No command register may be written while start_cmd is set.
 */
static int dw_issue_cmd(const struct device *dev, uint32_t cmd)
{
	dw_wr(dev, MSHC_CMD, cmd | CMD_START);

	return dw_wait(dev, MSHC_CMD, CMD_START, 0, DW_CMD_ACCEPT_TIMEOUT_MS);
}

/* Move SDMMC_CLKDIV, SDMMC_CLKSRC and SDMMC_CLKENA into the card clock domain. */
static int dw_update_clock(const struct device *dev)
{
	return dw_issue_cmd(dev, CMD_UPDATE_CLOCK | CMD_WAIT_PRVDATA);
}

static int dw_card_clock_off(const struct device *dev)
{
	struct dw_mshc_data *data = dev->data;

	dw_wr(dev, MSHC_CLKENA, 0);
	data->card_clock_req = 0;

	return dw_update_clock(dev);
}

/* Set the card clock to the highest supported rate that does not exceed freq. */
static int dw_card_clock_set(const struct device *dev, uint32_t freq)
{
	const struct dw_mshc_config *cfg = dev->config;
	struct dw_mshc_data *data = dev->data;
	const struct dw_mshc_clk_choice *pick = &dw_mshc_clk_choices[0];
	int ret;

	if (freq == 0) {
		return dw_card_clock_off(dev);
	}

	for (size_t i = 0; i < ARRAY_SIZE(dw_mshc_clk_choices); i++) {
		if (dw_mshc_clk_choices[i].freq <= freq &&
		    dw_mshc_clk_choices[i].freq <= cfg->max_freq) {
			pick = &dw_mshc_clk_choices[i];
		}
	}

	/* The clock must be off while the source and divider change. */
	ret = dw_card_clock_off(dev);
	if (ret != 0) {
		return ret;
	}

	ret = syscon_write_reg(
		cfg->cru, CRU_CLKSEL_CON30,
		CRU_HIWORD(CRU_SDMMC0_SEL_MASK, (uint32_t)pick->sel << CRU_SDMMC0_SEL_SHIFT));
	if (ret != 0) {
		LOG_ERR("CRU card clock select failed: %d", ret);
		return ret;
	}

	dw_wr(dev, MSHC_CLKDIV, 0);
	dw_wr(dev, MSHC_CLKSRC, 0);
	dw_wr(dev, MSHC_CLKENA, CLKENA_ENABLE | CLKENA_LOW_POWER);

	ret = dw_update_clock(dev);
	if (ret != 0) {
		return ret;
	}

	data->card_clock_req = freq;
	LOG_DBG("card clock %u Hz (CRU select %u)", pick->freq, pick->sel);

	return 0;
}

static void dw_clear_status(const struct device *dev, uint32_t bits)
{
	/* Writing 1 clears a raw status bit; writing 0 leaves it alone. */
	dw_wr(dev, MSHC_RINTSTS, bits);
}

static int dw_controller_init(const struct device *dev)
{
	const struct dw_mshc_config *cfg = dev->config;
	int ret;

	/*
	 * Switch the internal DMA off whatever the Kconfig choice: the boot loader
	 * may have left it running. The memo gives no order, so stop DMA first
	 * (CTRL bits, then BMOD reset) and only then reset the controller.
	 */
	dw_wr(dev, MSHC_CTRL, dw_rd(dev, MSHC_CTRL) & ~(CTRL_DMA_ENABLE | CTRL_USE_INTERNAL_DMA));
	dw_wr(dev, MSHC_BMOD, BMOD_SOFT_RESET);
	ret = dw_wait(dev, MSHC_BMOD, BMOD_SOFT_RESET, 0, DW_RESET_TIMEOUT_MS);
	if (ret != 0) {
		return ret;
	}
	dw_wr(dev, MSHC_BMOD, 0);
	dw_wr(dev, MSHC_IDSTS, IDSTS_ALL);

	ret = dw_reset_bits(dev, CTRL_CONTROLLER_RESET | CTRL_FIFO_RESET | CTRL_DMA_RESET);
	if (ret != 0) {
		return ret;
	}

	dw_clear_status(dev, INT_ALL);
	/* Polled operation: no interrupt sources are enabled. */
	dw_wr(dev, MSHC_INTMASK, 0);
	/* Maximum data and response timeouts; software timers decide. */
	dw_wr(dev, MSHC_TMOUT, 0xffffffffU);
	dw_wr(dev, MSHC_UHSREG, 0);
	/* Watermarks as recommended: rx = depth / 2 - 1, tx = depth / 2. */
	dw_wr(dev, MSHC_FIFOTH,
	      FIFOTH_MSIZE_8 | FIFOTH_RX_WMARK(cfg->fifo_depth / 2 - 1) |
		      FIFOTH_TX_WMARK(cfg->fifo_depth / 2));

	return 0;
}

static int dw_mshc_reset(const struct device *dev)
{
	struct dw_mshc_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = dw_reset_bits(dev, CTRL_CONTROLLER_RESET | CTRL_FIFO_RESET | CTRL_DMA_RESET);
	if (ret == 0) {
		dw_clear_status(dev, INT_ALL);
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int dw_mshc_get_card_present(const struct device *dev)
{
	return (dw_rd(dev, MSHC_CDETECT) & CDETECT_NO_CARD) == 0U;
}

static int dw_mshc_card_busy(const struct device *dev)
{
	return (dw_rd(dev, MSHC_STATUS) & STATUS_DATA_BUSY) != 0U;
}

static int dw_mshc_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct dw_mshc_config *cfg = dev->config;

	memset(props, 0, sizeof(*props));
	props->f_min = cfg->min_freq;
	props->f_max = cfg->max_freq;
	props->power_delay = cfg->power_delay_ms;
	props->max_current_330 = cfg->max_current_330;
	props->host_caps.vol_330_support = 1;
	props->host_caps.high_spd_support = 1;
	props->bus_4_bit_support = (cfg->bus_width == 4);

	return 0;
}

static int dw_mshc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct dw_mshc_config *cfg = dev->config;
	struct dw_mshc_data *data = dev->data;
	uint32_t ctype;
	int ret = 0;

	if (ios->signal_voltage != 0 && ios->signal_voltage != SD_VOL_3_3_V) {
		LOG_ERR("only 3.3 V signalling is supported");
		return -ENOTSUP;
	}

	switch ((int)ios->timing) {
	case 0:
	case SDHC_TIMING_LEGACY:
	case SDHC_TIMING_HS:
	case SDHC_TIMING_SDR12:
	case SDHC_TIMING_SDR25:
		break;
	default:
		LOG_ERR("unsupported timing mode %d", ios->timing);
		return -ENOTSUP;
	}

	switch ((int)ios->bus_width) {
	case 0:
	case SDHC_BUS_WIDTH1BIT:
		ctype = 0;
		break;
	case SDHC_BUS_WIDTH4BIT:
		if (cfg->bus_width < 4) {
			LOG_ERR("4-bit bus is not wired");
			return -ENOTSUP;
		}
		ctype = CTYPE_4BIT;
		break;
	default:
		LOG_ERR("unsupported bus width %d", ios->bus_width);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (ios->power_mode == SDHC_POWER_OFF) {
		ret = dw_card_clock_off(dev);
		dw_wr(dev, MSHC_PWREN, 0);
		data->init_clocks_sent = false;
		goto out;
	}

	if (ios->power_mode == SDHC_POWER_ON) {
		dw_wr(dev, MSHC_PWREN, 1);
	}

	if (ios->bus_width != 0) {
		dw_wr(dev, MSHC_CTYPE, ctype);
	}

	if (ios->clock != 0 && ios->clock != data->card_clock_req) {
		ret = dw_card_clock_set(dev, ios->clock);
	} else if (ios->clock == 0 && data->card_clock_req != 0) {
		ret = dw_card_clock_off(dev);
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

/* Build the SDMMC_CMD response flags for an SD response type. */
static uint32_t dw_response_flags(uint32_t rsp_type)
{
	switch (rsp_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		return 0;
	case SD_RSP_TYPE_R2:
		return CMD_RESPONSE_EXPECT | CMD_RESPONSE_LONG | CMD_CHECK_RESP_CRC;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		/* These responses carry no valid CRC. */
		return CMD_RESPONSE_EXPECT;
	default:
		return CMD_RESPONSE_EXPECT | CMD_CHECK_RESP_CRC;
	}
}

static bool dw_cmd_timeout_expected(uint32_t opcode)
{
	/* Card type probing: v1 cards ignore CMD8, memory cards ignore SDIO CMD5/CMD52. */
	return opcode == SD_SEND_IF_COND || opcode == SD_APP_CMD || opcode == SDIO_SEND_OP_COND ||
	       opcode == SDIO_RW_DIRECT;
}

/* Abandon a failed transfer: drop pending status and empty the FIFO. */
static void dw_recover(const struct device *dev, bool had_data)
{
	dw_clear_status(dev, INT_ALL);
	if (had_data) {
		(void)dw_reset_bits(dev, CTRL_FIFO_RESET);
	}
}

static int dw_fifo_count(const struct device *dev)
{
	return FIELD_GET(STATUS_FIFO_COUNT_MSK, dw_rd(dev, MSHC_STATUS));
}

/*
 * Move data with the CPU through the FIFO.
 *
 * The data transfer over (DTO) bit can be set together with RXDR/TXDR, and
 * can precede the last words leaving the FIFO. Remember that DTO was seen
 * and only clear RXDR/TXDR on their own, so DTO is never lost and a transfer
 * is complete only when DTO was seen and all words were moved.
 */
static int dw_pio_xfer(const struct device *dev, struct sdhc_data *data, bool is_write)
{
	const struct dw_mshc_config *cfg = dev->config;
	uint8_t *p = data->data;
	size_t words_left = ((size_t)data->block_size * data->blocks) / sizeof(uint32_t);
	int tmo_ms = (data->timeout_ms > 0 ? data->timeout_ms : DW_DEFAULT_DATA_TMO_MS) +
		     DW_PER_BLOCK_TMO_MS * (int)data->blocks;
	int64_t end = k_uptime_ticks() + k_ms_to_ticks_ceil64(tmo_ms);
	bool dto_seen = false;

	for (;;) {
		uint32_t st = dw_rd(dev, MSHC_RINTSTS);
		size_t n = 0;

		if ((st & INT_DATA_ERR) != 0U) {
			LOG_ERR("data error, RINTSTS 0x%08x", st);
			return -EIO;
		}
		if ((st & INT_DATA_TMO) != 0U) {
			LOG_ERR("data timeout, RINTSTS 0x%08x", st);
			return -ETIMEDOUT;
		}

		if ((st & INT_DTO) != 0U) {
			dto_seen = true;
		}

		if (is_write) {
			if ((st & INT_TXDR) != 0U) {
				dw_clear_status(dev, INT_TXDR);
				n = MIN(words_left, cfg->fifo_depth - dw_fifo_count(dev));
				for (size_t i = 0; i < n; i++) {
					dw_wr(dev, MSHC_FIFO, UNALIGNED_GET((uint32_t *)p));
					p += sizeof(uint32_t);
				}
			}
		} else if ((st & (INT_RXDR | INT_DTO)) != 0U || dto_seen) {
			if ((st & INT_RXDR) != 0U) {
				dw_clear_status(dev, INT_RXDR);
			}
			n = MIN(words_left, (size_t)dw_fifo_count(dev));
			for (size_t i = 0; i < n; i++) {
				UNALIGNED_PUT(dw_rd(dev, MSHC_FIFO), (uint32_t *)p);
				p += sizeof(uint32_t);
			}
		}
		words_left -= n;

		if (dto_seen && words_left == 0U) {
			return 0;
		}
		if (dto_seen && n == 0U) {
			/* The card ended the transfer before all words moved. */
			LOG_ERR("transfer ended early, %zu words left, "
				"RINTSTS 0x%08x STATUS 0x%08x CTRL 0x%08x BMOD 0x%08x BYTCNT "
				"0x%08x",
				words_left, dw_rd(dev, MSHC_RINTSTS), dw_rd(dev, MSHC_STATUS),
				dw_rd(dev, MSHC_CTRL), dw_rd(dev, MSHC_BMOD),
				dw_rd(dev, MSHC_BYTCNT));
			return -EIO;
		}
		if (k_uptime_ticks() >= end) {
			LOG_ERR("data transfer timeout, %zu words left, "
				"RINTSTS 0x%08x STATUS 0x%08x CTRL 0x%08x BMOD 0x%08x BYTCNT "
				"0x%08x",
				words_left, dw_rd(dev, MSHC_RINTSTS), dw_rd(dev, MSHC_STATUS),
				dw_rd(dev, MSHC_CTRL), dw_rd(dev, MSHC_BMOD),
				dw_rd(dev, MSHC_BYTCNT));
			return -ETIMEDOUT;
		}
	}
}

static int dw_mshc_request_once(const struct device *dev, struct sdhc_command *cmd,
				struct sdhc_data *data)
{
	struct dw_mshc_data *drv = dev->data;
	uint32_t rsp_type = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;
	uint32_t flags = (cmd->opcode & CMD_INDEX_MASK) | CMD_USE_HOLD_REG;
	bool is_write = false;
	bool auto_stop = false;
	uint32_t st;
	int tmo_ms;
	int ret;

	if (data != NULL) {
		size_t bytes = (size_t)data->block_size * data->blocks;

		if (data->data == NULL || bytes == 0U || (bytes % sizeof(uint32_t)) != 0U ||
		    data->block_size > UINT16_MAX) {
			return -EINVAL;
		}
		is_write = cmd->opcode == SD_WRITE_SINGLE_BLOCK ||
			   cmd->opcode == SD_WRITE_MULTIPLE_BLOCK;
		/* The SD stack never sends CMD12; the host ends multiple block transfers. */
		auto_stop = cmd->opcode == SD_READ_MULTIPLE_BLOCK ||
			    cmd->opcode == SD_WRITE_MULTIPLE_BLOCK;
		flags |= CMD_DATA_EXPECTED;
		if (is_write) {
			flags |= CMD_WRITE;
		}
		if (auto_stop) {
			flags |= CMD_SEND_AUTO_STOP;
		}
	}

	flags |= dw_response_flags(cmd->response_type);
	if (cmd->opcode == SD_STOP_TRANSMISSION) {
		flags |= CMD_STOP_ABORT;
	} else {
		flags |= CMD_WAIT_PRVDATA;
	}

	tmo_ms = cmd->timeout_ms > 0 ? cmd->timeout_ms : DW_DEFAULT_CMD_TMO_MS;

	k_mutex_lock(&drv->lock, K_FOREVER);

	if (!drv->init_clocks_sent || cmd->opcode == SD_GO_IDLE_STATE) {
		/* 80 clocks must precede the first command after power up. */
		flags |= CMD_SEND_INIT;
		drv->init_clocks_sent = true;
	}

	ret = dw_wait(dev, MSHC_STATUS, STATUS_DATA_BUSY, 0, DW_BUSY_TIMEOUT_MS);
	if (ret != 0) {
		LOG_ERR("card busy before CMD%u", cmd->opcode);
		goto out;
	}

	dw_clear_status(dev, INT_ALL);

	if (data != NULL) {
		dw_wr(dev, MSHC_BLKSIZ, data->block_size);
		dw_wr(dev, MSHC_BYTCNT, data->block_size * data->blocks);
		ret = dw_reset_bits(dev, CTRL_FIFO_RESET);
		if (ret != 0) {
			goto out;
		}
	}

	dw_wr(dev, MSHC_CMDARG, cmd->arg);
	ret = dw_issue_cmd(dev, flags);
	if (ret != 0) {
		LOG_ERR("CMD%u was not accepted", cmd->opcode);
		goto fail;
	}

	/* A response timeout ends the command without a response. */
	ret = dw_wait_any(dev, MSHC_RINTSTS, INT_CD | INT_RTO | INT_HLE, tmo_ms, &st);
	if (ret != 0) {
		LOG_ERR("CMD%u done timeout, RINTSTS 0x%08x STATUS 0x%08x", cmd->opcode,
			dw_rd(dev, MSHC_RINTSTS), dw_rd(dev, MSHC_STATUS));
		goto fail;
	}

	if ((st & INT_RTO) != 0U) {
		if (dw_cmd_timeout_expected(cmd->opcode)) {
			LOG_DBG("CMD%u response timeout", cmd->opcode);
		} else {
			LOG_ERR("CMD%u response timeout", cmd->opcode);
		}
		ret = -ETIMEDOUT;
		goto fail;
	}
	if ((st & (INT_HLE | INT_RE)) != 0U ||
	    ((flags & CMD_CHECK_RESP_CRC) != 0U && (st & INT_RCRC) != 0U)) {
		LOG_ERR("CMD%u response error, RINTSTS 0x%08x", cmd->opcode, st);
		ret = -EIO;
		goto fail;
	}

	if (auto_stop) {
		/*
		 * Drop the status of this command, so that an error bit that is
		 * set later belongs to the automatic stop command.
		 */
		dw_clear_status(dev, INT_RTO | INT_RCRC | INT_RE);
	}

	if (rsp_type != SD_RSP_TYPE_NONE) {
		cmd->response[0] = dw_rd(dev, MSHC_RESP0);
		if (rsp_type == SD_RSP_TYPE_R2) {
			cmd->response[1] = dw_rd(dev, MSHC_RESP1);
			cmd->response[2] = dw_rd(dev, MSHC_RESP2);
			cmd->response[3] = dw_rd(dev, MSHC_RESP3);
		}
	}

	if (data != NULL) {
		ret = dw_pio_xfer(dev, data, is_write);
		if (ret != 0) {
			goto fail;
		}

		/*
		 * Clear everything except the auto command done bit and the
		 * response errors of the automatic stop command: they may
		 * already be set and are looked at below.
		 */
		dw_clear_status(dev, INT_ALL & ~(INT_ACD | INT_RTO | INT_RCRC | INT_RE));
		if (auto_stop) {
			/*
			 * The stop command needs a full second at least, whatever
			 * timeout the SD stack gave for the first command.
			 */
			ret = dw_wait_any(dev, MSHC_RINTSTS, INT_ACD | INT_RTO | INT_RCRC | INT_RE,
					  MAX(tmo_ms, DW_DEFAULT_CMD_TMO_MS), &st);
			if (ret != 0) {
				LOG_ERR("auto stop did not complete");
				goto fail;
			}
			if ((st & (INT_RTO | INT_RCRC | INT_RE)) != 0U) {
				LOG_ERR("auto stop error, RINTSTS 0x%08x", st);
				ret = -EIO;
				goto fail;
			}
			dw_clear_status(dev, INT_ACD);
		}
		data->bytes_xfered = data->block_size * data->blocks;
	}

	if (rsp_type == SD_RSP_TYPE_R1b || rsp_type == SD_RSP_TYPE_R5b) {
		ret = dw_wait(dev, MSHC_STATUS, STATUS_DATA_BUSY, 0,
			      MAX(tmo_ms, DW_BUSY_TIMEOUT_MS));
		if (ret != 0) {
			LOG_ERR("card busy after CMD%u", cmd->opcode);
			goto fail;
		}
	}

	dw_clear_status(dev, INT_ALL);
	k_mutex_unlock(&drv->lock);

	return 0;

fail:
	dw_recover(dev, data != NULL);
out:
	k_mutex_unlock(&drv->lock);

	return ret;
}

static int dw_mshc_request(const struct device *dev, struct sdhc_command *cmd,
			   struct sdhc_data *data)
{
	/*
	 * Only data commands are retried, as cmd->retries is set by the SD
	 * stack for them. A command without data that times out is not
	 * repeated: the card type probing relies on the first timeout.
	 */
	uint32_t attempts = data != NULL ? (uint32_t)cmd->retries + 1U : 1U;
	int ret;

	do {
		ret = dw_mshc_request_once(dev, cmd, data);
		if ((ret != -EIO && ret != -ETIMEDOUT) || attempts <= 1U) {
			break;
		}
		LOG_DBG("CMD%u failed (%d), %u attempts left", cmd->opcode, ret, attempts - 1U);
	} while (--attempts > 0U);

	return ret;
}

static int dw_mshc_init(const struct device *dev)
{
	const struct dw_mshc_config *cfg = dev->config;
	struct dw_mshc_data *data = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (!device_is_ready(cfg->cru)) {
		LOG_ERR("CRU syscon device is not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	ret = dw_controller_init(dev);
	if (ret != 0) {
		LOG_ERR("controller reset failed: %d", ret);
		return ret;
	}

	/* Identification clock; set_io() takes over when the SD stack starts. */
	dw_wr(dev, MSHC_PWREN, 1);
	dw_wr(dev, MSHC_CTYPE, 0);

	return dw_card_clock_set(dev, cfg->min_freq);
}

static DEVICE_API(sdhc, dw_mshc_api) = {
	.reset = dw_mshc_reset,
	.request = dw_mshc_request,
	.set_io = dw_mshc_set_io,
	.get_card_present = dw_mshc_get_card_present,
	.card_busy = dw_mshc_card_busy,
	.get_host_props = dw_mshc_get_host_props,
};

#define DW_MSHC_INIT(n)                                                                            \
	BUILD_ASSERT(DT_INST_PROP(n, fifo_depth) >= 16, "fifo-depth is too small");                \
	BUILD_ASSERT(DT_INST_PROP_OR(n, bus_width, 4) == 1 ||                                      \
			     DT_INST_PROP_OR(n, bus_width, 4) == 4,                                \
		     "bus-width must be 1 or 4");                                                  \
	static const struct dw_mshc_config dw_mshc_cfg_##n = {                                     \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.cru = DEVICE_DT_GET(DT_INST_PHANDLE(n, rockchip_cru)),                            \
		.fifo_depth = DT_INST_PROP(n, fifo_depth),                                         \
		.min_freq = DT_INST_PROP(n, min_bus_freq),                                         \
		.max_freq = DT_INST_PROP(n, max_bus_freq),                                         \
		.power_delay_ms = DT_INST_PROP(n, power_delay_ms),                                 \
		.max_current_330 = DT_INST_PROP(n, max_current_330),                               \
		.bus_width = DT_INST_PROP_OR(n, bus_width, 4),                                     \
	};                                                                                         \
	static struct dw_mshc_data dw_mshc_data_##n;                                               \
	DEVICE_DT_INST_DEFINE(n, dw_mshc_init, NULL, &dw_mshc_data_##n, &dw_mshc_cfg_##n,          \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &dw_mshc_api);

DT_INST_FOREACH_STATUS_OKAY(DW_MSHC_INIT)
