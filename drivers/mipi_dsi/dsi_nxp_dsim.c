/*
 * Copyright (C) 2026, Savoir-faire Linux
 * Author: Paolo Wattebled <paolo.wattebled@savoirfairelinux.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Samsung DSIM MIPI-DSI host driver for NXP i.MX8MP.
 *
 * Controls the Samsung DSIM IP block. The PLL divider values (P, M, S)
 * are computed at attach time from the PLL reference frequency and the
 * target burst clock; the resulting bit clock then drives the D-PHY
 * timing computation using MIPI D-PHY specification formulas.
 *
 * Prerequisites handled by earlier boot-stage drivers:
 *   - GPC:           MEDIMIX and MIPI_PHY1 power domains enabled
 *   - CCM:           pixel and DSI reference clocks enabled
 *   - media_blk_ctrl: soft resets released, clocks gated on
 *   - MIPI PHY:      lane resets released
 */

#define DT_DRV_COMPAT nxp_mipi_dsim

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#include <fsl_device_registers.h>

LOG_MODULE_REGISTER(dsi_nxp_dsim, CONFIG_MIPI_DSI_LOG_LEVEL);

#define DEV_CFG(_dev) ((const struct dsim_config *)(_dev)->config)

/* NXP header exposes only the combined PMS field; split into P/M/S. */
#define DSIM_PLLCTRL_P(x) (((uint32_t)(x) << 14) & 0xFC000U) /* bits 19:14 */
#define DSIM_PLLCTRL_M(x) (((uint32_t)(x) << 4) & 0x3FF0U)   /* bits 13:4  */
#define DSIM_PLLCTRL_S(x) (((uint32_t)(x) << 1) & 0xEU)      /* bits 3:1   */

/* PLL divider constraints */
#define DSIM_PLL_FIN_MIN_HZ  2000000U
#define DSIM_PLL_FIN_MAX_HZ  30000000U
#define DSIM_PLL_FVCO_MIN_HZ 1050000000ULL
#define DSIM_PLL_FVCO_MAX_HZ 2100000000ULL
#define DSIM_PLL_M_MIN       64U
#define DSIM_PLL_M_MAX       1023U
#define DSIM_PLL_P_MAX       63U
#define DSIM_PLL_S_MAX       5U

/* Register-poll timeouts and settle delays */
#define DSIM_PLL_STABLE_TIMEOUT_US 500000U
#define DSIM_SWRST_TIMEOUT_US      100000U
#define DSIM_STOP_STATE_TIMEOUT_US 100000U
#define DSIM_FIFO_TIMEOUT_US       250000U
#define DSIM_FIFO_RESET_US         10000U
#define DSIM_LP_SETTLE_US          10000U

#define DSI_BLANK_PKT_OVERHEAD 6U
#define DSIM_MAX_TX_BUF        128U

/* DSI data identifier: MIPI DSI spec §8.5.1 */
#define DSIM_DI_VC_SHIFT 6U
#define DSIM_DI_DT_MASK  0x3fU
#define DSIM_DI_VC_MAX   3U

#define DSIM_MAX_DATA_LANES  4U
#define DSIM_ESC_CLK_MAX_KHZ 20000U /* Samsung DSIM escape-clock ceiling */

/* Register-field constants with no symbolic name in the SDK header */
#define DSIM_PLL_TIMER           0x8000U
#define DSIM_STOP_STATE_CNT      0xfU
#define DSIM_BTA_TIMEOUT         0xffU
#define DSIM_LP_RX_TIMEOUT       0xffffU
#define DSIM_FIFOCTRL_RESET_MASK 0x1fU

/*
 * D-PHY timing helpers (MIPI D-PHY spec v2.0 §6.9). Values are in picoseconds;
 * PS_TO_CYCLE converts to byte-clock cycles (round-closest).
 */
#define PSEC_PER_SEC 1000000000000ULL

#define PS_TO_CYCLE(ps, byte_clk_hz)                                                               \
	((uint32_t)(((uint64_t)(ps) * (byte_clk_hz) + PSEC_PER_SEC / 2U) / PSEC_PER_SEC))

/** D-PHY timing parameters in picoseconds. */
struct dphy_timing_ps {
	uint64_t lpx;
	uint64_t hs_exit;
	uint64_t hs_prepare;
	uint64_t hs_zero;
	uint64_t hs_trail;
	uint64_t clk_prepare;
	uint64_t clk_zero;
	uint64_t clk_post;
	uint64_t clk_trail;
};

/** Compile-time DSIM configuration from devicetree. */
struct dsim_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;       /* phy_ref (D-PHY PLL reference) */
	clock_control_subsys_t pixel_clock_subsys; /* pixel (DISP1_PIX, for blanking math) */
	uint32_t burst_clk_hz;                     /* samsung,burst-clock-frequency */
	uint32_t esc_clk_hz;                       /* samsung,esc-clock-frequency */
};

/** Runtime DSIM state, populated at attach time. */
struct dsim_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	uint32_t pix_clk; /* kHz */
	uint32_t bit_clk; /* kHz */
	uint32_t lanes;
	uint32_t channel;
	uint32_t pixfmt;
	uint32_t mode_flags;
	uint32_t pll_p; /* computed at attach */
	uint32_t pll_m;
	uint32_t pll_s;
	bool cmd_lpm;
	struct {
		uint32_t hactive, hfp, hbp, hsync;
		uint32_t vactive, vfp, vbp, vsync;
	} vm;
};

struct dsim_pll_match {
	uint32_t p;
	uint32_t m;
	uint32_t s;
	uint32_t diff;
};

/**
 * @brief Write a packet header to the DSIM packet-header FIFO.
 *
 * @param dsi DSIM register block.
 * @param vc Virtual channel (0-3).
 * @param data_type MIPI DSI data type (lower 6 bits of the data identifier).
 * @param data0 First header byte (word-count LSB for long packets).
 * @param data1 Second header byte (word-count MSB for long packets).
 */
static void dsim_wr_tx_header(MIPI_DSI_Type *dsi, uint8_t vc, uint8_t data_type, uint8_t data0,
			      uint8_t data1)
{
	uint8_t di = (uint8_t)(((uint32_t)vc << DSIM_DI_VC_SHIFT) | (data_type & DSIM_DI_DT_MASK));
	uint32_t reg = ((uint32_t)data1 << 16U) | ((uint32_t)data0 << 8U) | (uint32_t)di;

	dsi->DSI_PKTHDR = reg;
}

/**
 * @brief Push one 32-bit word into the DSIM payload FIFO.
 *
 * @param dsi DSIM register block.
 * @param tx_data Payload word.
 */
static void dsim_wr_tx_data(MIPI_DSI_Type *dsi, uint32_t tx_data)
{
	dsi->DSI_PAYLOAD = tx_data;
}

/**
 * @brief Write a long-packet payload to the FIFO, packing bytes into words.
 *
 * @param dsi DSIM register block.
 * @param data Payload bytes.
 * @param len Payload length in bytes (must be > 0).
 */
static void dsim_long_data_wr(MIPI_DSI_Type *dsi, const uint8_t *data, uint32_t len)
{
	__ASSERT(len > 0, "%s called with len=0", __func__);

	for (uint32_t i = 0U; i < len; i += 4U) {
		uint32_t rem = len - i;
		uint32_t word;

		if (rem >= 4U) {
			word = (uint32_t)data[i] | ((uint32_t)data[i + 1U] << 8U) |
			       ((uint32_t)data[i + 2U] << 16U) | ((uint32_t)data[i + 3U] << 24U);
		} else {
			word = (uint32_t)data[i];
			if (rem > 1U) {
				word |= (uint32_t)data[i + 1U] << 8U;
			}
			if (rem > 2U) {
				word |= (uint32_t)data[i + 2U] << 16U;
			}
		}

		dsim_wr_tx_data(dsi, word);
	}
}

/**
 * @brief Wait for the payload FIFO to drain, then clear its status bit.
 *
 * @param dsi DSIM register block.
 * @return 0 on success, -ETIMEDOUT on timeout.
 */
static int dsim_wait_for_pkt_done(MIPI_DSI_Type *dsi)
{
	if (!WAIT_FOR(dsi->DSI_INTSRC & MIPI_DSI_DSI_INTSRC_SFRPLFIFOEMPTY_MASK,
		      DSIM_FIFO_TIMEOUT_US, k_busy_wait(10U))) {
		return -ETIMEDOUT;
	}

	dsi->DSI_INTSRC = MIPI_DSI_DSI_INTSRC_SFRPLFIFOEMPTY_MASK;
	return 0;
}

/**
 * @brief Wait for the header FIFO to drain, then clear its status bit.
 *
 * @param dsi DSIM register block.
 * @return 0 on success, -ETIMEDOUT on timeout.
 */
static int dsim_wait_for_hdr_done(MIPI_DSI_Type *dsi)
{
	if (!WAIT_FOR(dsi->DSI_INTSRC & MIPI_DSI_DSI_INTSRC_SFRPHFIFOEMPTY_MASK,
		      DSIM_FIFO_TIMEOUT_US, k_busy_wait(10U))) {
		return -ETIMEDOUT;
	}

	dsi->DSI_INTSRC = MIPI_DSI_DSI_INTSRC_SFRPHFIFOEMPTY_MASK;
	return 0;
}

/**
 * @brief Select low-power or high-speed command transmission (CMDLPDT).
 *
 * @param dsi DSIM register block.
 * @param enable True for low-power command mode, false for high-speed.
 */
static void dsim_config_cmd_lpm(MIPI_DSI_Type *dsi, bool enable)
{
	uint32_t escmode = dsi->DSI_ESCMODE;

	if (enable) {
		escmode |= MIPI_DSI_DSI_ESCMODE_CMDLPDT_MASK;
	} else {
		escmode &= ~MIPI_DSI_DSI_ESCMODE_CMDLPDT_MASK;
	}

	dsi->DSI_ESCMODE = escmode;
}

static bool dsim_pll_fvco_valid(uint64_t fvco)
{
	return fvco >= DSIM_PLL_FVCO_MIN_HZ && fvco <= DSIM_PLL_FVCO_MAX_HZ;
}

static bool dsim_pll_fin_valid(uint32_t fin)
{
	return fin >= DSIM_PLL_FIN_MIN_HZ && fin <= DSIM_PLL_FIN_MAX_HZ;
}

static bool dsim_pll_m_valid(uint32_t m)
{
	return m >= DSIM_PLL_M_MIN && m <= DSIM_PLL_M_MAX;
}

static uint32_t dsim_abs_diff(uint32_t a, uint32_t b)
{
	return a > b ? a - b : b - a;
}

static bool dsim_pll_try_m(uint32_t ref_hz, uint32_t burst_hz, uint32_t p, uint32_t s, uint32_t m,
			   struct dsim_pll_match *best)
{
	uint32_t actual;
	uint32_t diff;

	if (!dsim_pll_m_valid(m)) {
		return false;
	}

	actual = (uint32_t)(((uint64_t)ref_hz * m / p) >> s);
	diff = dsim_abs_diff(actual, burst_hz);
	if (diff >= best->diff) {
		return false;
	}

	best->p = p;
	best->m = m;
	best->s = s;
	best->diff = diff;

	return diff == 0U;
}

static bool dsim_pll_try_p(uint32_t ref_hz, uint32_t burst_hz, uint64_t fvco, uint32_t p,
			   uint32_t s, struct dsim_pll_match *best)
{
	uint32_t fin = ref_hz / p;
	uint32_t m_floor;

	if (!dsim_pll_fin_valid(fin)) {
		return false;
	}

	m_floor = (uint32_t)(fvco / fin);
	if (dsim_pll_try_m(ref_hz, burst_hz, p, s, m_floor, best)) {
		return true;
	}

	return dsim_pll_try_m(ref_hz, burst_hz, p, s, m_floor + 1U, best);
}

static void dsim_pll_store_match(const struct dsim_pll_match *match, uint32_t *out_p,
				 uint32_t *out_m, uint32_t *out_s)
{
	*out_p = match->p;
	*out_m = match->m;
	*out_s = match->s;
}

/**
 * @brief Find PLL dividers P, M, S minimizing error to the burst clock.
 *
 * Fout = ref * M / P / 2^S. Tries floor and ceil M per (S, P) since fvco is
 * rarely divisible by fin. Bounds are enforced via the DSIM_PLL_* macros.
 *
 * @param ref_hz PLL reference clock (Hz).
 * @param burst_hz Target burst/bit clock (Hz).
 * @param out_p Result P divider.
 * @param out_m Result M divider.
 * @param out_s Result S divider.
 * @return 0 on success, -ERANGE if no in-range combination exists.
 */
static int dsim_pll_find_pms(uint32_t ref_hz, uint32_t burst_hz, uint32_t *out_p, uint32_t *out_m,
			     uint32_t *out_s)
{
	struct dsim_pll_match best = {
		.diff = UINT32_MAX,
	};

	for (uint32_t s = 0U; s <= DSIM_PLL_S_MAX; s++) {
		uint64_t fvco = (uint64_t)burst_hz << s;

		if (!dsim_pll_fvco_valid(fvco)) {
			continue;
		}

		for (uint32_t p = 1U; p <= DSIM_PLL_P_MAX; p++) {
			if (dsim_pll_try_p(ref_hz, burst_hz, fvco, p, s, &best)) {
				dsim_pll_store_match(&best, out_p, out_m, out_s);
				return 0;
			}
		}
	}

	if (best.p == 0U) {
		return -ERANGE;
	}

	dsim_pll_store_match(&best, out_p, out_m, out_s);
	return 0;
}

/**
 * @brief Program the PLL dividers, enable it, and wait for lock.
 *
 * Must run before SWRST: the synchronous reset needs the byte clock to release.
 *
 * @param dsi DSIM register block.
 * @param data Driver data holding the computed P/M/S.
 * @return 0 on success, -ETIMEDOUT if the PLL fails to lock.
 */
static int dsim_enable_pll(MIPI_DSI_Type *dsi, const struct dsim_data *data)
{
	uint32_t pllctrl = 0U;

	dsi->DSI_PLLTMR = DSIM_PLL_TIMER;

	pllctrl |= DSIM_PLLCTRL_P(data->pll_p) | DSIM_PLLCTRL_M(data->pll_m) |
		   DSIM_PLLCTRL_S(data->pll_s) | MIPI_DSI_DSI_PLLCTRL_PLLEN_MASK;
	dsi->DSI_PLLCTRL = pllctrl;

	if (!WAIT_FOR(dsi->DSI_STATUS & MIPI_DSI_DSI_STATUS_PLLSTABLE_MASK,
		      DSIM_PLL_STABLE_TIMEOUT_US, k_busy_wait(1000U))) {
		LOG_ERR("PLL stable timeout, STATUS=0x%08x", dsi->DSI_STATUS);
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 * @brief Wait for clock + data lanes to reach stop state, then set STOPSTATE_CNT.
 *
 * Must run after DSI_CONFIG has enabled the lanes.
 *
 * @param dsi DSIM register block.
 * @param lanes Number of active data lanes.
 * @return 0 on success, -EIO if the lanes never reach stop state.
 */
static int dsim_wait_stop_state(MIPI_DSI_Type *dsi, uint32_t lanes)
{
	uint32_t data_lanes_en = (0x1U << lanes) - 1U;
	uint32_t escmode;

	if (!WAIT_FOR((dsi->DSI_STATUS & MIPI_DSI_DSI_STATUS_STOPSTATECLK_MASK) &&
			      (((dsi->DSI_STATUS & MIPI_DSI_DSI_STATUS_STOPSTATEDAT_MASK) >>
				MIPI_DSI_DSI_STATUS_STOPSTATEDAT_SHIFT) &
			       data_lanes_en) == data_lanes_en,
		      DSIM_STOP_STATE_TIMEOUT_US, k_busy_wait(100U))) {
		LOG_ERR("lanes not in stop state (status=0x%08x lanes_en=0x%x)", dsi->DSI_STATUS,
			data_lanes_en);
		return -EIO;
	}

	escmode = dsi->DSI_ESCMODE;
	escmode &= ~MIPI_DSI_DSI_ESCMODE_STOPSTATE_CNT_MASK;
	escmode |= MIPI_DSI_DSI_ESCMODE_STOPSTATE_CNT(DSIM_STOP_STATE_CNT);
	dsi->DSI_ESCMODE = escmode;

	return 0;
}

/**
 * @brief Program resolution, porch and sync timing registers for video mode.
 *
 * @param dsi DSIM register block.
 * @param data Driver data holding the video timings and clocks.
 * @return 0 on success, -EINVAL if a porch word count underflows the overhead.
 */
static int dsim_set_main_mode(MIPI_DSI_Type *dsi, struct dsim_data *data)
{
	uint32_t mdresol = 0U, mvporch = 0U, mhporch = 0U, msync = 0U;
	uint32_t byte_clk_khz = data->bit_clk / BITS_PER_BYTE;
	uint32_t pix_clk_khz = data->pix_clk;
	uint32_t overhead;
	uint32_t hfp_wc, hbp_wc, hsa_wc;

	mdresol |= MIPI_DSI_DSI_MDRESOL_MAINVRESOL(data->vm.vactive) |
		   MIPI_DSI_DSI_MDRESOL_MAINHRESOL(data->vm.hactive);
	dsi->DSI_MDRESOL = mdresol;

	mvporch |= MIPI_DSI_DSI_MVPORCH_MAINVBP(data->vm.vbp) |
		   MIPI_DSI_DSI_MVPORCH_STABLEVFP(data->vm.vfp) |
		   MIPI_DSI_DSI_MVPORCH_CMDALLOW(0xFU);
	dsi->DSI_MVPORCH = mvporch;

	/*
	 * Non-burst WC excludes the 4B header + 2B CRC; burst WC covers the
	 * full porch with no overhead.
	 */
	overhead = (data->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) ? 0U : DSI_BLANK_PKT_OVERHEAD;

	hfp_wc = (uint32_t)DIV_ROUND_UP((uint64_t)data->vm.hfp * byte_clk_khz, pix_clk_khz);
	hbp_wc = (uint32_t)DIV_ROUND_UP((uint64_t)data->vm.hbp * byte_clk_khz, pix_clk_khz);
	hsa_wc = (uint32_t)DIV_ROUND_UP((uint64_t)data->vm.hsync * byte_clk_khz, pix_clk_khz);

	/*
	 * Guard the subtraction: a porch smaller than the packet overhead would
	 * underflow the unsigned WC and corrupt the timing registers.
	 */
	if (hfp_wc < overhead || hbp_wc < overhead || hsa_wc < overhead) {
		LOG_ERR("porch word count below packet overhead (hfp=%u hbp=%u hsa=%u ovh=%u)",
			hfp_wc, hbp_wc, hsa_wc, overhead);
		return -EINVAL;
	}
	hfp_wc -= overhead;
	hbp_wc -= overhead;
	hsa_wc -= overhead;

	mhporch |= MIPI_DSI_DSI_MHPORCH_MAINHFP(hfp_wc) | MIPI_DSI_DSI_MHPORCH_MAINHBP(hbp_wc);
	dsi->DSI_MHPORCH = mhporch;

	msync |= MIPI_DSI_DSI_MSYNC_MAINVSA(data->vm.vsync) | MIPI_DSI_DSI_MSYNC_MAINHSA(hsa_wc);
	dsi->DSI_MSYNC = msync;

	LOG_DBG("hfp_wc=%u hbp_wc=%u hsa_wc=%u", hfp_wc, hbp_wc, hsa_wc);
	return 0;
}

/**
 * @brief Program DSI_CONFIG (video mode, mode flags, pixel format, lane enable).
 *
 * @param dsi DSIM register block.
 * @param data Driver data holding mode flags, pixel format and lane count.
 * @return 0 on success, -ENOTSUP for an unsupported pixel format.
 */
static int dsim_config_dpi(MIPI_DSI_Type *dsi, struct dsim_data *data)
{
	uint32_t config = 0U, data_lanes_en;

	if (data->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) {
		config |= MIPI_DSI_DSI_CONFIG_NON_CONTINUOUS_CLOCK_LANE_MASK |
			  MIPI_DSI_DSI_CONFIG_CLKLANE_STOP_START_MASK;
	}

	if (data->mode_flags & MIPI_DSI_MODE_EOT_PACKET) {
		config |= MIPI_DSI_DSI_CONFIG_EOT_R03_MASK;
	}

	if (data->mode_flags & MIPI_DSI_MODE_VIDEO) {
		config |= MIPI_DSI_DSI_CONFIG_VIDEOMODE_MASK;

		if (data->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
			config |= MIPI_DSI_DSI_CONFIG_BURSTMODE_MASK;
		} else if (data->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
			config |= MIPI_DSI_DSI_CONFIG_SYNCINFORM_MASK;
		}

		if (data->mode_flags & MIPI_DSI_MODE_VIDEO_AUTO_VERT) {
			config |= MIPI_DSI_DSI_CONFIG_AUTOMODE_MASK;
		}
		if (data->mode_flags & MIPI_DSI_MODE_VIDEO_HSE) {
			config |= MIPI_DSI_DSI_CONFIG_HSEDISABLEMODE_MASK;
		}
		if (data->mode_flags & MIPI_DSI_MODE_VIDEO_HFP) {
			config |= MIPI_DSI_DSI_CONFIG_HFPDISABLEMODE_MASK;
		}
		if (data->mode_flags & MIPI_DSI_MODE_VIDEO_HBP) {
			config |= MIPI_DSI_DSI_CONFIG_HBPDISABLEMODE_MASK;
		}
		if (data->mode_flags & MIPI_DSI_MODE_VIDEO_HSA) {
			config |= MIPI_DSI_DSI_CONFIG_HSADISABLEMODE_MASK;
		}

		switch (data->pixfmt) {
		case MIPI_DSI_PIXFMT_RGB565:
			config |= MIPI_DSI_DSI_CONFIG_MAINPIXFORMAT(0x4U);
			break;
		case MIPI_DSI_PIXFMT_RGB666_PACKED:
			config |= MIPI_DSI_DSI_CONFIG_MAINPIXFORMAT(0x5U);
			break;
		case MIPI_DSI_PIXFMT_RGB666:
			config |= MIPI_DSI_DSI_CONFIG_MAINPIXFORMAT(0x6U);
			break;
		case MIPI_DSI_PIXFMT_RGB888:
			config |= MIPI_DSI_DSI_CONFIG_MAINPIXFORMAT(0x7U);
			break;
		default:
			LOG_ERR("unsupported pixel format: %u", data->pixfmt);
			return -ENOTSUP;
		}
	}

	data_lanes_en = (0x1U << data->lanes) - 1U;
	config |= MIPI_DSI_DSI_CONFIG_NUMOFDATLANE(data->lanes - 1U);
	config |= MIPI_DSI_DSI_CONFIG_LANEEN(0x1U | (data_lanes_en << 1U));
	dsi->DSI_CONFIG = config;

	LOG_DBG("DSIM CONFIG=0x%08x", config);
	return 0;
}

/**
 * @brief Compute D-PHY HS timing parameters (picoseconds) from the bit clock.
 *
 * Per MIPI D-PHY v1.2 §6.9 (matches Linux phy-core-mipi-dphy.c).
 *
 * @param hs_clk_hz HS bit clock (Hz).
 * @param t Output timing parameters.
 */
static void dphy_calc_timing(uint64_t hs_clk_hz, struct dphy_timing_ps *t)
{
	uint64_t ui = DIV_ROUND_UP(PSEC_PER_SEC, hs_clk_hz);

	t->lpx = 50000ULL;
	t->hs_exit = 100000ULL;
	t->clk_prepare = 65000ULL;
	t->clk_zero = 300000ULL - t->clk_prepare;
	t->clk_post = 60000ULL + 52ULL * ui;
	t->clk_trail = 60000ULL;
	t->hs_prepare = 60000ULL + 4ULL * ui;
	t->hs_zero = (145000ULL + 10ULL * ui) - t->hs_prepare;
	t->hs_trail = MAX(32ULL * ui, 60000ULL + 16ULL * ui);
}

/**
 * @brief Program the PHYTIMING* and TIMEOUT registers from the bit clock.
 *
 * @param dsi DSIM register block.
 * @param bit_clk_khz HS bit clock (kHz).
 */
static void dsim_config_dphy(MIPI_DSI_Type *dsi, uint32_t bit_clk_khz)
{
	uint64_t hs_clk_hz = (uint64_t)bit_clk_khz * 1000U;
	uint64_t byte_clk_hz = hs_clk_hz / 8U;
	struct dphy_timing_ps t;

	dphy_calc_timing(hs_clk_hz, &t);

	uint32_t phytiming =
		MIPI_DSI_DSI_PHYTIMING_M_TLPXCTL(PS_TO_CYCLE(t.lpx, byte_clk_hz)) |
		MIPI_DSI_DSI_PHYTIMING_M_THSEXITCTL(PS_TO_CYCLE(t.hs_exit, byte_clk_hz));

	uint32_t phytiming1 =
		MIPI_DSI_DSI_PHYTIMING1_M_TCLKPRPRCTL(PS_TO_CYCLE(t.clk_prepare, byte_clk_hz)) |
		MIPI_DSI_DSI_PHYTIMING1_M_TCLKZEROCTL(PS_TO_CYCLE(t.clk_zero, byte_clk_hz)) |
		MIPI_DSI_DSI_PHYTIMING1_M_TCLKPOSTCTL(PS_TO_CYCLE(t.clk_post, byte_clk_hz)) |
		MIPI_DSI_DSI_PHYTIMING1_M_TCLKTRAILCTL(PS_TO_CYCLE(t.clk_trail, byte_clk_hz));

	uint32_t phytiming2 =
		MIPI_DSI_DSI_PHYTIMING2_M_THSPRPRCTL(PS_TO_CYCLE(t.hs_prepare, byte_clk_hz)) |
		MIPI_DSI_DSI_PHYTIMING2_M_THSZEROCTL(PS_TO_CYCLE(t.hs_zero, byte_clk_hz)) |
		MIPI_DSI_DSI_PHYTIMING2_M_THSTRAILCTL(PS_TO_CYCLE(t.hs_trail, byte_clk_hz));

	uint32_t timeout = MIPI_DSI_DSI_TIMEOUT_BTATOUT(DSIM_BTA_TIMEOUT) |
			   MIPI_DSI_DSI_TIMEOUT_LPDRTOUT(DSIM_LP_RX_TIMEOUT);

	LOG_DBG("PHYTIMING=0x%08x PHYTIMING1=0x%08x PHYTIMING2=0x%08x", phytiming, phytiming1,
		phytiming2);

	dsi->DSI_PHYTIMING = phytiming;
	dsi->DSI_PHYTIMING1 = phytiming1;
	dsi->DSI_PHYTIMING2 = phytiming2;
	dsi->DSI_TIMEOUT = timeout;
}

/**
 * @brief Phase 1 clock bring-up: enable the ESC and BYTE clocks for LP transfers.
 *
 * The HS clock request (TXREQUESTHSCLK) is deferred to phase 2.
 *
 * @param dsi DSIM register block.
 * @param lanes Number of active data lanes.
 * @param bit_clk_khz HS bit clock (kHz).
 * @param esc_clk_khz Target escape clock (kHz).
 */
static void dsim_config_clkctrl_phase1(MIPI_DSI_Type *dsi, uint32_t lanes, uint32_t bit_clk_khz,
				       uint32_t esc_clk_khz)
{
	uint32_t clkctrl = 0U;
	uint32_t byte_clk, esc_prescaler;
	uint32_t data_lanes_en = (0x1U << lanes) - 1U;

	clkctrl |= MIPI_DSI_DSI_CLKCTRL_ESCCLKEN_MASK;
	clkctrl |= MIPI_DSI_DSI_CLKCTRL_BYTECLKSRC(0U);
	clkctrl |= MIPI_DSI_DSI_CLKCTRL_BYTECLKEN_MASK;
	clkctrl |= MIPI_DSI_DSI_CLKCTRL_LANEESCCLKEN(0x1U | (data_lanes_en << 1U));

	byte_clk = bit_clk_khz >> 3U;

	/*
	 * Escape clock must stay <= 20 MHz (Samsung DSIM limit): take the larger
	 * prescaler of the requested esc clock and the 20 MHz cap.
	 */
	esc_prescaler = MAX(DIV_ROUND_UP(byte_clk, esc_clk_khz),
			    DIV_ROUND_UP(byte_clk, DSIM_ESC_CLK_MAX_KHZ));
	clkctrl |= MIPI_DSI_DSI_CLKCTRL_ESCPRESCALER(esc_prescaler);

	dsi->DSI_CLKCTRL = clkctrl;
}

/**
 * @brief Phase 2 clock bring-up: assert TXREQUESTHSCLK to enter HS mode.
 *
 * @param dsi DSIM register block.
 */
static void dsim_config_clkctrl_phase2(MIPI_DSI_Type *dsi)
{
	uint32_t clkctrl = dsi->DSI_CLKCTRL;

	clkctrl |= MIPI_DSI_DSI_CLKCTRL_TXREQUESTHSCLK_MASK;
	dsi->DSI_CLKCTRL = clkctrl;
}

/**
 * @brief Enable or disable main video output through MAINSTANDBY.
 *
 * @param dsi DSIM register block.
 * @param enable True to start video output, false to stop it.
 */
static void dsim_set_display_enable(MIPI_DSI_Type *dsi, bool enable)
{
	uint32_t mdresol = dsi->DSI_MDRESOL;

	if (enable) {
		mdresol |= MIPI_DSI_DSI_MDRESOL_MAINSTANDBY_MASK;
	} else {
		mdresol &= ~MIPI_DSI_DSI_MDRESOL_MAINSTANDBY_MASK;
	}

	dsi->DSI_MDRESOL = mdresol;
}

/**
 * @brief Disable the HS request, ESC and BYTE clocks.
 *
 * @param dsi DSIM register block.
 */
static void dsim_disable_clkctrl(MIPI_DSI_Type *dsi)
{
	uint32_t clkctrl = dsi->DSI_CLKCTRL;

	clkctrl &= ~MIPI_DSI_DSI_CLKCTRL_TXREQUESTHSCLK_MASK;
	clkctrl &= ~MIPI_DSI_DSI_CLKCTRL_ESCCLKEN_MASK;
	clkctrl &= ~MIPI_DSI_DSI_CLKCTRL_BYTECLKEN_MASK;
	dsi->DSI_CLKCTRL = clkctrl;
}

/**
 * @brief Disable the DSIM PLL.
 *
 * @param dsi DSIM register block.
 */
static void dsim_disable_pll(MIPI_DSI_Type *dsi)
{
	uint32_t pllctrl = dsi->DSI_PLLCTRL;

	pllctrl &= ~MIPI_DSI_DSI_PLLCTRL_PLLEN_MASK;
	dsi->DSI_PLLCTRL = pllctrl;
}

/**
 * @brief Write one DSI packet: short via header FIFO, long via payload + header.
 *
 * @param dsi DSIM register block.
 * @param channel Virtual channel (0-3).
 * @param data_type MIPI DSI data type.
 * @param buf data0/data1 for a short packet, or the full payload for a long one.
 * @param len Payload length: 0 selects a short packet, >0 a long packet.
 * @return 0 on success, -ETIMEDOUT on FIFO timeout.
 */
static int dsim_pkt_write(MIPI_DSI_Type *dsi, uint8_t channel, uint8_t data_type,
			  const uint8_t *buf, uint32_t len)
{
	int ret;

	if (len == 0U) {
		/*
		 * Short packets use the header FIFO only — poll SFRPHFIFOEMPTY,
		 * not SFRPLFIFOEMPTY which tracks the payload FIFO.
		 */
		dsim_wr_tx_header(dsi, channel, data_type, buf[0], buf[1]);
		ret = dsim_wait_for_hdr_done(dsi);
		if (ret) {
			LOG_ERR("TX header FIFO timeout");
			return -ETIMEDOUT;
		}
	} else {
		dsim_long_data_wr(dsi, buf, len);
		dsim_wr_tx_header(dsi, channel, data_type, (uint8_t)(len & 0xffU),
				  (uint8_t)((len >> 8U) & 0xffU));

		ret = dsim_wait_for_pkt_done(dsi);
		if (ret) {
			LOG_ERR("TX FIFO timeout");
			return -ETIMEDOUT;
		}
	}

	/*
	 * FIFO-empty only means the SFR drained, not that the LP burst finished
	 * on the wire; without this gap large vendor writes corrupt each other.
	 */
	k_busy_wait(DSIM_LP_SETTLE_US);
	return 0;
}

static bool dsim_prepare_transfer_lpm(MIPI_DSI_Type *dsi, struct dsim_data *data,
				      uint32_t msg_flags)
{
	bool desired_lpm = (data->mode_flags & MIPI_DSI_MODE_LPM) != 0U;

	if (msg_flags & MIPI_DSI_MSG_USE_LPM) {
		desired_lpm = true;
	}

	if (desired_lpm == data->cmd_lpm) {
		return false;
	}

	dsim_config_cmd_lpm(dsi, desired_lpm);
	data->cmd_lpm = desired_lpm;
	return true;
}

static void dsim_restore_default_lpm(MIPI_DSI_Type *dsi, struct dsim_data *data)
{
	bool default_lpm = (data->mode_flags & MIPI_DSI_MODE_LPM) != 0U;

	if (data->cmd_lpm == default_lpm) {
		return;
	}

	dsim_config_cmd_lpm(dsi, default_lpm);
	data->cmd_lpm = default_lpm;
}

static int dsim_validate_tx_buf(const struct mipi_dsi_msg *msg, uint32_t min_len)
{
	if (msg->tx_len < min_len || msg->tx_buf == NULL) {
		return -EINVAL;
	}

	return 0;
}

static ssize_t dsim_write_dcs_short(MIPI_DSI_Type *dsi, uint8_t channel,
				    const struct mipi_dsi_msg *msg)
{
	uint8_t buf[2];
	int ret;

	buf[0] = msg->cmd;
	buf[1] = 0U;
	ret = dsim_pkt_write(dsi, channel, msg->type, buf, 0U);

	return ret ? ret : 0;
}

static ssize_t dsim_write_dcs_short_param(MIPI_DSI_Type *dsi, uint8_t channel,
					  const struct mipi_dsi_msg *msg)
{
	const uint8_t *tx = msg->tx_buf;
	uint8_t buf[2];
	int ret;

	ret = dsim_validate_tx_buf(msg, 1U);
	if (ret) {
		return ret;
	}

	buf[0] = msg->cmd;
	buf[1] = tx[0];
	ret = dsim_pkt_write(dsi, channel, msg->type, buf, 0U);

	return ret ? ret : (ssize_t)msg->tx_len;
}

static ssize_t dsim_write_dcs_long(MIPI_DSI_Type *dsi, uint8_t channel,
				   const struct mipi_dsi_msg *msg)
{
	uint8_t buf[DSIM_MAX_TX_BUF];
	int ret;

	/* Compare without +1 so a near-SIZE_MAX tx_len cannot wrap past the check. */
	if (msg->tx_len >= DSIM_MAX_TX_BUF || (msg->tx_len > 0U && msg->tx_buf == NULL)) {
		LOG_ERR("DCS long write invalid: len=%zu", msg->tx_len);
		return -EINVAL;
	}

	buf[0] = msg->cmd;
	if (msg->tx_len > 0U) {
		memcpy(&buf[1], msg->tx_buf, msg->tx_len);
	}

	ret = dsim_pkt_write(dsi, channel, msg->type, buf, (uint32_t)(1U + msg->tx_len));
	return ret ? ret : (ssize_t)msg->tx_len;
}

static ssize_t dsim_write_generic_short(MIPI_DSI_Type *dsi, uint8_t channel,
					const struct mipi_dsi_msg *msg, uint32_t min_len)
{
	const uint8_t *tx = msg->tx_buf;
	uint8_t buf[2] = {0U, 0U};
	uint32_t accepted = 0U;
	int ret;

	if (min_len > 0U) {
		ret = dsim_validate_tx_buf(msg, min_len);
		if (ret) {
			return ret;
		}

		buf[0] = tx[0];
		if (min_len > 1U) {
			buf[1] = tx[1];
		}

		accepted = (uint32_t)msg->tx_len;
	}

	ret = dsim_pkt_write(dsi, channel, msg->type, buf, 0U);
	if (ret) {
		return ret;
	}

	return (ssize_t)accepted;
}

static ssize_t dsim_write_generic_long(MIPI_DSI_Type *dsi, uint8_t channel,
				       const struct mipi_dsi_msg *msg)
{
	uint8_t buf[DSIM_MAX_TX_BUF];
	int ret;

	if (msg->tx_len == 0U || msg->tx_len > DSIM_MAX_TX_BUF || msg->tx_buf == NULL) {
		LOG_ERR("generic long write invalid: len=%zu", msg->tx_len);
		return -EINVAL;
	}

	memcpy(buf, msg->tx_buf, msg->tx_len);
	ret = dsim_pkt_write(dsi, channel, msg->type, buf, (uint32_t)msg->tx_len);

	return ret ? ret : (ssize_t)msg->tx_len;
}

static ssize_t dsim_write_dcs(MIPI_DSI_Type *dsi, uint8_t channel, const struct mipi_dsi_msg *msg)
{
	switch (msg->type) {
	case MIPI_DSI_DCS_SHORT_WRITE:
		return dsim_write_dcs_short(dsi, channel, msg);
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		return dsim_write_dcs_short_param(dsi, channel, msg);
	case MIPI_DSI_DCS_LONG_WRITE:
		return dsim_write_dcs_long(dsi, channel, msg);
	default:
		LOG_ERR("DCS read not yet implemented or used");
		return -ENOTSUP;
	}
}

static ssize_t dsim_write_generic(MIPI_DSI_Type *dsi, uint8_t channel,
				  const struct mipi_dsi_msg *msg)
{
	switch (msg->type) {
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		return dsim_write_generic_short(dsi, channel, msg, 0U);
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		return dsim_write_generic_short(dsi, channel, msg, 1U);
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		return dsim_write_generic_short(dsi, channel, msg, 2U);
	case MIPI_DSI_GENERIC_LONG_WRITE:
		return dsim_write_generic_long(dsi, channel, msg);
	default:
		LOG_ERR("generic read not yet implemented or used");
		return -ENOTSUP;
	}
}

/**
 * @brief Bring up the DSIM host: compute the PLL, reset, configure, and enable.
 *
 * @param dev DSIM device.
 * @param channel Virtual channel (stored for reference).
 * @param mdev Attaching peripheral (lanes, pixel format, mode flags, timings).
 * @return 0 on success, negative errno on failure.
 */
static int dsim_attach(const struct device *dev, uint8_t channel,
		       const struct mipi_dsi_device *mdev)
{
	const struct dsim_config *cfg = DEV_CFG(dev);
	struct dsim_data *data = dev->data;
	MIPI_DSI_Type *dsi = (MIPI_DSI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t fifoctrl;
	uint32_t computed_bit_clk_hz;
	uint32_t pll_ref_freq;
	uint32_t pix_freq_hz;
	int ret;

	if (mdev->data_lanes < 1U || mdev->data_lanes > DSIM_MAX_DATA_LANES) {
		LOG_ERR("invalid data lane count: %u", mdev->data_lanes);
		return -EINVAL;
	}
	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		LOG_ERR("unsupported DSI mode (video mode required)");
		return -EINVAL;
	}

	ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &pll_ref_freq);
	if (ret) {
		LOG_ERR("failed to get PHY ref clock rate: %d", ret);
		return ret;
	}

	data->lanes = mdev->data_lanes;
	data->channel = channel;
	data->pixfmt = mdev->pixfmt;
	data->mode_flags = mdev->mode_flags;

	data->vm.hactive = mdev->timings.hactive;
	data->vm.hfp = mdev->timings.hfp;
	data->vm.hbp = mdev->timings.hbp;
	data->vm.hsync = mdev->timings.hsync;
	data->vm.vactive = mdev->timings.vactive;
	data->vm.vfp = mdev->timings.vfp;
	data->vm.vbp = mdev->timings.vbp;
	data->vm.vsync = mdev->timings.vsync;

	ret = dsim_pll_find_pms(pll_ref_freq, cfg->burst_clk_hz, &data->pll_p, &data->pll_m,
				&data->pll_s);
	if (ret) {
		LOG_ERR("No valid PLL P/M/S for ref=%u burst=%u", pll_ref_freq, cfg->burst_clk_hz);
		return ret;
	}
	computed_bit_clk_hz = (uint32_t)(((uint64_t)pll_ref_freq * data->pll_m) / data->pll_p /
					 (1U << data->pll_s));
	data->bit_clk = computed_bit_clk_hz / 1000U;

	ret = clock_control_get_rate(cfg->clock_dev, cfg->pixel_clock_subsys, &pix_freq_hz);
	if (ret || pix_freq_hz == 0U) {
		LOG_ERR("failed to get pixel clock rate: %d", ret);
		return ret ? ret : -EINVAL;
	}
	data->pix_clk = pix_freq_hz / 1000U;

	LOG_DBG("PLL P=%u M=%u S=%u bit_clk=%u kHz pix_clk=%u kHz", data->pll_p, data->pll_m,
		data->pll_s, data->bit_clk, data->pix_clk);

	/*
	 * Manual reset sequence: clocks + PLL must run before SWRST (synchronous
	 * reset won't release without a clock). Reset, wait for release, then
	 * configure the functional registers.
	 */
	dsim_config_clkctrl_phase1(dsi, data->lanes, data->bit_clk, cfg->esc_clk_hz / 1000U);

	ret = dsim_enable_pll(dsi, data);
	if (ret) {
		LOG_ERR("DSIM PLL enable failed (%d)", ret);
		return ret;
	}

	dsi->DSI_SWRST = 1U;

	if (!WAIT_FOR(dsi->DSI_STATUS & MIPI_DSI_DSI_STATUS_SWRSTRLS_MASK, DSIM_SWRST_TIMEOUT_US,
		      k_busy_wait(1000U))) {
		LOG_ERR("software reset did not release (SWRSTRLS)");
		ret = -ETIMEDOUT;
		goto err_disable;
	}

	data->cmd_lpm = false;

	/*
	 * Re-configure all registers after reset, per the manual. CLKCTRL is
	 * re-applied because SWRST clears the functional register file.
	 */
	dsim_config_clkctrl_phase1(dsi, data->lanes, data->bit_clk, cfg->esc_clk_hz / 1000U);

	/*
	 * FIFO pointer reset: clear then restore bits[4:0]; stale FIFO state
	 * otherwise produces display noise.
	 */
	fifoctrl = dsi->DSI_FIFOCTRL;
	fifoctrl &= ~DSIM_FIFOCTRL_RESET_MASK;
	dsi->DSI_FIFOCTRL = fifoctrl;
	k_busy_wait(DSIM_FIFO_RESET_US);
	fifoctrl |= DSIM_FIFOCTRL_RESET_MASK;
	dsi->DSI_FIFOCTRL = fifoctrl;
	k_busy_wait(DSIM_FIFO_RESET_US);

	ret = dsim_set_main_mode(dsi, data);
	if (ret) {
		goto err_disable;
	}
	ret = dsim_config_dpi(dsi, data);
	if (ret) {
		goto err_disable;
	}
	dsim_config_dphy(dsi, data->bit_clk);

	ret = dsim_wait_stop_state(dsi, data->lanes);
	if (ret) {
		LOG_ERR("DSIM lane stop-state failed (%d)", ret);
		goto err_disable;
	}

	dsim_config_clkctrl_phase2(dsi);
	dsim_set_display_enable(dsi, true);

	LOG_INF("DSIM attached: %u lanes ch=%u pixfmt=%u mode_flags=0x%x", data->lanes,
		data->channel, data->pixfmt, data->mode_flags);
	return 0;

err_disable:
	dsim_disable_clkctrl(dsi);
	dsim_disable_pll(dsi);
	return ret;
}

/**
 * @brief Send a DSI command packet (DCS / generic short and long writes).
 *
 * Reads are not implemented. DCS packets use msg->cmd; generic packets carry
 * their payload in msg->tx_buf with no command byte.
 *
 * @param dev DSIM device.
 * @param channel Virtual channel (0-3).
 * @param msg Message describing the packet to send.
 * @return Number of bytes accepted on success, negative errno on failure.
 */
static ssize_t dsim_transfer(const struct device *dev, uint8_t channel, struct mipi_dsi_msg *msg)
{
	struct dsim_data *data = dev->data;
	MIPI_DSI_Type *dsi = (MIPI_DSI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	ssize_t ret;
	bool lpm_changed;

	if (channel > DSIM_DI_VC_MAX) {
		LOG_ERR("invalid virtual channel: %u", channel);
		return -EINVAL;
	}

	lpm_changed = dsim_prepare_transfer_lpm(dsi, data, msg->flags);

	switch (msg->type) {
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
	case MIPI_DSI_DCS_LONG_WRITE:
	case MIPI_DSI_DCS_READ:
		ret = dsim_write_dcs(dsi, channel, msg);
		break;

	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
	case MIPI_DSI_GENERIC_LONG_WRITE:
	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
		ret = dsim_write_generic(dsi, channel, msg);
		break;

	default:
		LOG_ERR("unsupported DSI msg type: 0x%02x", msg->type);
		ret = -ENOTSUP;
		break;
	}

	if (lpm_changed) {
		dsim_restore_default_lpm(dsi, data);
	}

	return ret;
}

/**
 * @brief Stop video output and disable the DSIM clocks and PLL.
 *
 * @param dev DSIM device.
 * @param channel Unused.
 * @param mdev Unused.
 * @return 0 always.
 */
static int dsim_detach(const struct device *dev, uint8_t channel,
		       const struct mipi_dsi_device *mdev)
{
	MIPI_DSI_Type *dsi = (MIPI_DSI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t intsrc;

	ARG_UNUSED(channel);
	ARG_UNUSED(mdev);

	dsim_set_display_enable(dsi, false);
	dsim_disable_clkctrl(dsi);
	dsim_disable_pll(dsi);

	/* Write-1-to-clear: read the pending sources back to acknowledge them all. */
	intsrc = dsi->DSI_INTSRC;
	dsi->DSI_INTSRC = intsrc;

	LOG_INF("DSIM detached");
	return 0;
}

static DEVICE_API(mipi_dsi, dsim_api) = {
	.attach = dsim_attach,
	.transfer = dsim_transfer,
	.detach = dsim_detach,
};

/**
 * @brief Map registers, enable the DSI clock, and probe the IP version.
 *
 * @param dev DSIM device.
 * @return 0 on success, negative errno on failure.
 */
static int dsim_init(const struct device *dev)
{
	const struct dsim_config *cfg = DEV_CFG(dev);
	MIPI_DSI_Type *dsi;
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock controller not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("failed to enable DSI clock: %d", ret);
		return ret;
	}

	dsi = (MIPI_DSI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	LOG_INF("DSIM VERSION=0x%08x", dsi->DSI_VERSION);

	return 0;
}

#define DSIM_INIT(inst)                                                                            \
	static const struct dsim_config dsim_config_##inst = {                                     \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(inst)),                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, phy_ref)),            \
		.clock_subsys = (clock_control_subsys_t)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(    \
			inst, phy_ref, name),                                                      \
		.pixel_clock_subsys =                                                              \
			(clock_control_subsys_t)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(            \
				inst, pixel, name),                                                \
		.burst_clk_hz = DT_INST_PROP(inst, samsung_burst_clock_frequency),                 \
		.esc_clk_hz = DT_INST_PROP(inst, samsung_esc_clock_frequency),                     \
	};                                                                                         \
                                                                                                   \
	static struct dsim_data dsim_data_##inst;                                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dsim_init, NULL, &dsim_data_##inst, &dsim_config_##inst,       \
			      POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY, &dsim_api);

DT_INST_FOREACH_STATUS_OKAY(DSIM_INIT)
