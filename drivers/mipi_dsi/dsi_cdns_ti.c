/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_j721e_dsi

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/dt-bindings/mipi_dsi/mipi_dsi.h>
#include <zephyr/display/mipi_display.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(dsi_cdns_ti, CONFIG_MIPI_DSI_LOG_LEVEL);

/* Main control */
#define MCTL_MAIN_DATA_CTL      0x04
#define BTA_EN                  BIT(14)
#define READ_EN                 BIT(13)
#define HOST_EOT_GEN            BIT(17)
#define VID_EN                  BIT(5)
#define IF_VID_SELECT(x)        ((x) << 2)
#define IF_VID_SELECT_MASK      GENMASK(3, 2)
#define IF_VID_MODE             BIT(1)
#define LINK_EN                 BIT(0)

#define MCTL_MAIN_PHY_CTL       0x08
#define CLK_CONTINUOUS          BIT(4)
#define DATA_LANE_EN(x)         BIT((x) - 1)  /* lanes 1..N-1 */

#define MCTL_MAIN_EN            0x0C
#define IF_EN(x)                BIT(13 + (x))
#define DATA_LANE_START(x)      BIT(4 + (x))
#define CLK_LANE_EN             BIT(3)
#define PLL_START               BIT(0)

#define MCTL_DPHY_CFG0          0x10
#define DPHY_C_RSTB             BIT(20)
#define DPHY_D_RSTB(x)          GENMASK(15 + (x), 16)
#define DPHY_C_PDN              BIT(8)
#define DPHY_CMN_PDN            BIT(9)
#define DPHY_PLL_PDN            BIT(10)
#define DPHY_ALL_D_PDN          GENMASK(7, 4)
#define DPHY_PLL_PSO            BIT(1)
#define DPHY_CMN_PSO            BIT(0)

#define MCTL_DPHY_TIMEOUT1      0x14
#define HSTX_TIMEOUT(x)         ((x) << 4)
#define HSTX_TIMEOUT_MAX        GENMASK(17, 0)
#define CLK_DIV(x)              (x)
#define CLK_DIV_MAX             GENMASK(3, 0)

#define MCTL_DPHY_TIMEOUT2      0x18
#define LPRX_TIMEOUT(x)         (x)

#define MCTL_ULPOUT_TIME        0x1C
#define DATA_LANE_ULPOUT_TIME(x) ((x) << 9)
#define CLK_LANE_ULPOUT_TIME(x)  (x)

#define MCTL_MAIN_STS           0x24
#define PLL_LOCKED              BIT(0)
#define CLK_LANE_RDY            BIT(1)
#define DATA_LANE_RDY(l)        BIT(2 + (l))

#define MCTL_MAIN_STS_CLR       0x150

/* Direct command path (LP mode DCS transfers) */
#define DIRECT_CMD_SEND         0x80
#define DIRECT_CMD_MAIN_SETTINGS 0x84
#define TRIGGER_VAL(x)          ((x) << 25)
#define CMD_LP_EN               BIT(24)
#define CMD_SIZE(x)             ((x) << 16)
#define CMD_VCHAN_ID(x)         ((x) << 14)
#define CMD_DATATYPE(x)         ((x) << 8)
#define CMD_LONG                BIT(3)
#define READ_CMD                1
#define BTA_REQ                 6

#define DIRECT_CMD_STS          0x88
#define DIRECT_CMD_STS_CTL      0x138
#define DIRECT_CMD_STS_CLR      0x158
#define READ_COMPLETED_WITH_ERR BIT(10)
#define ACK_WITH_ERR_RCVD       BIT(5)
#define ACK_RCVD                BIT(4)
#define READ_COMPLETED          BIT(3)
#define WRITE_COMPLETED         BIT(1)

#define DIRECT_CMD_STOP_READ    0x8C
#define DIRECT_CMD_WRDATA       0x90
#define DIRECT_CMD_FIFO_RST     0x94
#define DIRECT_CMD_RDDATA       0xA0

/* Video mode timing */
#define VID_MAIN_CTL            0xB0
#define VID_IGNORE_MISS_VSYNC   BIT(31)
#define RECOVERY_MODE(x)        ((x) << 25)
#define RECOVERY_MODE_NEXT_HSYNC 0
#define REG_BLKEOL_MODE(x)      ((x) << 23)
#define REG_BLKLINE_MODE(x)     ((x) << 21)
#define REG_BLK_MODE_BLANKING_PKT 1
#define REG_BLK_MODE_LP           2   /* go to LP during blanking */
#define SYNC_PULSE_HORIZONTAL   BIT(20)
#define SYNC_PULSE_ACTIVE       BIT(19)
#define VID_PIXEL_MODE_RGB565        (0 << 14)
#define VID_PIXEL_MODE_RGB666_PACKED (1 << 14)
#define VID_PIXEL_MODE_RGB666        (2 << 14)
#define VID_PIXEL_MODE_RGB888        (3 << 14)
#define VID_DATATYPE(x)         ((x) << 8)

#define VID_VSIZE1              0xB4
#define VFP_LEN(x)              ((x) << 12)
#define VBP_LEN(x)              ((x) << 6)
#define VSA_LEN(x)              (x)

#define VID_VSIZE2              0xB8

#define VID_HSIZE1              0xC0
#define HBP_LEN(x)              ((x) << 16)
#define HSA_LEN(x)              (x)

#define VID_HSIZE2              0xC4
#define HFP_LEN(x)              ((x) << 16)
#define HACT_LEN(x)             (x)

#define VID_BLKSIZE1            0xCC
#define BLK_LINE_EVENT_PKT_LEN(x) (x)

#define VID_BLKSIZE2            0xD0
#define BLK_LINE_PULSE_PKT_LEN(x) (x)

#define VID_VCA_SETTING2        0xF8
#define MAX_LINE_LIMIT(x)       ((x) << 16)

#define VID_DPHY_TIME           0xDC
#define REG_WAKEUP_TIME(x)      ((x) << 17)
#define REG_LINE_DURATION(x)    (x)

/* Horizontal timing conversion overheads (in DSI bytes) */
#define DSI_HBP_FRAME_PULSE_OVERHEAD  12
#define DSI_HBP_FRAME_EVENT_OVERHEAD  16
#define DSI_HSA_FRAME_OVERHEAD        14
#define DSI_HFP_FRAME_OVERHEAD        6
#define DSI_HSS_VSS_VSE_FRAME_OVERHEAD 4
#define DSI_BLANKING_FRAME_OVERHEAD   6
#define DSI_NULL_FRAME_OVERHEAD       6
#define DSI_EOT_PKT_SIZE              4

/*  TI DSI wrapper register offsets */
#define DSI_WRAP_DPI_CONTROL    0x04
#define DSI_WRAP_DPI_0_EN       BIT(0)

/*  Cadence DPHY TX register offsets */
#define DPHY_CMN_SSM            0x20    /* State-machine control */
#define DPHY_CMN_SSM_EN         BIT(0)
#define DPHY_CMN_TX_MODE_EN     BIT(9)

#define DPHY_CMN_PWM            0x40
#define DPHY_CMN_PWM_HIGH(x)    ((x) << 0)
#define DPHY_CMN_PWM_LOW(x)     ((x) << 10)
#define DPHY_CMN_PWM_DIV(x)     ((x) << 20)

/* PCS module */
#define DPHY_BAND_CFG            0xB00   /* DPHY_PCS(0x0) = 0xb00 */
#define DPHY_BAND_CFG_LEFT_BAND  GENMASK(4, 0)
#define DPHY_BAND_CFG_RIGHT_BAND GENMASK(9, 5)

/* WIZ registers (TI-specific) */
#define DPHY_TX_WIZ_PLL_CTRL  0xF04
#define DPHY_TX_WIZ_STATUS    0xF08
#define DPHY_TX_WIZ_RST_CTRL  0xF0C
#define DPHY_TX_WIZ_PSM_FREQ  0xF10

#define DPHY_TX_WIZ_IPDIV     GENMASK(4, 0)
#define DPHY_TX_WIZ_OPDIV     GENMASK(13, 8)
#define DPHY_TX_WIZ_FBDIV     GENMASK(25, 16)
#define DPHY_TX_WIZ_LANE_RSTB BIT(31)
#define DPHY_TX_WIZ_PLL_LOCK  BIT(31)

/*  DPHY HS clock band-control table */
static const uint32_t cdns_dphy_tx_bands[] = {
	80, 100, 120, 160, 200, 240, 320, 390, 450, 510,
	560, 640, 690, 770, 870, 950, 1000, 1200, 1400, 1600,
	1800, 2000, 2200, 2500
};

/*  MIPI DSI pixel format → bits-per-pixel */
static inline int dsi_fmt_to_bpp(uint32_t fmt)
{
	switch (fmt) {
	case MIPI_DSI_PIXFMT_RGB888:
		return 24;
	case MIPI_DSI_PIXFMT_RGB666:
		return 18;
	case MIPI_DSI_PIXFMT_RGB666_PACKED:
		return 18;
	case MIPI_DSI_PIXFMT_RGB565:
		return 16;
	default:
		return -1;
	}
}

/*  Driver structs */
struct dsi_cdns_ti_config {
	DEVICE_MMIO_NAMED_ROM(dsi);     /* Cadence DSI TX core */
	DEVICE_MMIO_NAMED_ROM(wrap);    /* TI DSI wrapper      */
	DEVICE_MMIO_NAMED_ROM(dphy);    /* Cadence DPHY TX     */

	/* DSI TX clocks */
	const struct device *dsi_p_clk_dev;
	clock_control_subsys_t dsi_p_clk_subsys;
	const struct device *dsi_sys_clk_dev;
	clock_control_subsys_t dsi_sys_clk_subsys;

	/* DPHY clocks */
	const struct device *psm_clk_dev;
	clock_control_subsys_t psm_clk_subsys;
	const struct device *pll_ref_clk_dev;
	clock_control_subsys_t pll_ref_clk_subsys;

	uint32_t pll_ref_clk_rate_hz;
	uint32_t data_lanes;           /* 1 – 4 */
};

struct dsi_cdns_ti_data {
	DEVICE_MMIO_NAMED_RAM(dsi);
	DEVICE_MMIO_NAMED_RAM(wrap);
	DEVICE_MMIO_NAMED_RAM(dphy);

	bool link_up;                  /* DSI link + DPHY powered */
	struct k_mutex lock;
};

#define DEV_CFG(dev)  ((const struct dsi_cdns_ti_config *)(dev)->config)
#define DEV_DATA(dev) ((struct dsi_cdns_ti_data *)(dev)->data)

/*  Register accessors */
static inline void dsi_wr(const struct device *dev, uint32_t off, uint32_t v)
{
	sys_write32(v, (uintptr_t)DEVICE_MMIO_NAMED_GET(dev, dsi) + off);
}
static inline uint32_t dsi_rd(const struct device *dev, uint32_t off)
{
	return sys_read32((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, dsi) + off);
}

static inline uint32_t wrap_rd(const struct device *dev, uint32_t off)
{
	return sys_read32((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, wrap) + off);
}

static inline void wrap_wr(const struct device *dev, uint32_t off, uint32_t v)
{
	sys_write32(v, (uintptr_t)DEVICE_MMIO_NAMED_GET(dev, wrap) + off);
}

static inline void dphy_wr(const struct device *dev, uint32_t off, uint32_t v)
{
	sys_write32(v, (uintptr_t)DEVICE_MMIO_NAMED_GET(dev, dphy) + off);
}
static inline uint32_t dphy_rd(const struct device *dev, uint32_t off)
{
	return sys_read32((uintptr_t)DEVICE_MMIO_NAMED_GET(dev, dphy) + off);
}

/*  DPHY helpers */
static uint32_t dphy_calc_pll(uint32_t pll_ref_hz, uint32_t hs_clk_rate,
			      uint8_t *ipdiv_out, uint8_t *opdiv_out,
			      uint16_t *fbdiv_out)
{
	uint8_t ipdiv, opdiv;
	uint16_t fbdiv;
	uint64_t tmp;

	/* IP divider — based on PLL reference frequency */
	if (pll_ref_hz < 19200000U) {
		ipdiv = 1;
	}
	else if (pll_ref_hz < 38400000U) {
		ipdiv = 2;
	}
	else if (pll_ref_hz < 76800000U) {
		ipdiv = 4;
	}
	else {
		ipdiv = 8;
	}

	/* OP divider — based on desired HS clock rate */
	if (hs_clk_rate >= 1250000000U) {
		opdiv = 1;
	}
	else if (hs_clk_rate >= 630000000U) {
		opdiv = 2;
	}
	else if (hs_clk_rate >= 320000000U) {
		opdiv = 4;
	}
	else if (hs_clk_rate >= 160000000U) {
		opdiv = 8;
	}
	else {
		opdiv = 16;
	}

	/* FB divider */
	tmp = (uint64_t)hs_clk_rate * 2ULL * opdiv * ipdiv;
	fbdiv = (uint16_t)DIV_ROUND_UP(tmp, pll_ref_hz);
	if (fbdiv < 16U) {
		fbdiv = 16U;
	} else if (fbdiv > 255U) {
		if (opdiv > 1U) {
			opdiv >>= 1;
			tmp = (uint64_t)hs_clk_rate * 2ULL * opdiv * ipdiv;
			fbdiv = (uint16_t)DIV_ROUND_UP(tmp, pll_ref_hz);
			if (fbdiv < 16U) {
				fbdiv = 16U;
			}
			if (fbdiv > 255U) {
				fbdiv = 255U;
			}
		} else {
			fbdiv = 255U;
		}
	}

	*ipdiv_out = ipdiv;
	*opdiv_out = opdiv;
	*fbdiv_out = fbdiv;

	/* Return actual HS clock rate */
	return (uint32_t)((uint64_t)pll_ref_hz * fbdiv / (2ULL * opdiv * ipdiv));
}

/* Get band-control index for a given hs_clk_rate (Hz). */
static int dphy_get_band_ctrl(uint32_t hs_clk_rate)
{
	uint32_t rate_mhz = hs_clk_rate / 1000000U;
	int n = ARRAY_SIZE(cdns_dphy_tx_bands);
	int i;

	if (rate_mhz < cdns_dphy_tx_bands[0]) {
		return -ERANGE;
	}
	for (i = 0; i < n - 1; i++) {
		if (rate_mhz >= cdns_dphy_tx_bands[i] &&
		    rate_mhz < cdns_dphy_tx_bands[i + 1]) {
			return i;
		}
	}
	if (rate_mhz >= cdns_dphy_tx_bands[n - 1]) {
		return n - 1;
	}
	return -ERANGE;
}

/* Full DPHY power-on sequence for J721E variant. */
static int dphy_power_on(const struct device *dev, uint32_t hs_clk_rate)
{
	const struct dsi_cdns_ti_config *cfg = DEV_CFG(dev);
	uint8_t ipdiv, opdiv;
	uint16_t fbdiv;
	uint32_t actual_rate, psm_clk_hz, psm_div, reg, band;
	uint32_t timeout, status;
	int ret;

	/* 1. Get actual PSM clock rate (clocks already enabled by caller) */
	ret = clock_control_get_rate(cfg->psm_clk_dev, cfg->psm_clk_subsys,
				     &psm_clk_hz);

	if (ret < 0) {
		LOG_WRN("Cannot read psm_clk rate, assuming 50 MHz");
		psm_clk_hz = 50000000U;
	}

	/* 2. Set PSM divider so internal PSM clock ≈ 1 MHz */
	psm_div = DIV_ROUND_CLOSEST(psm_clk_hz, 1000000U);
	if (psm_div < 1U) {
		psm_div = 1U;
	}
	if (psm_div > 255U) {
		psm_div = 255U;
	}
	dphy_wr(dev, DPHY_TX_WIZ_PSM_FREQ, psm_div);

	/* 3. Calculate and program PLL dividers */
	actual_rate = dphy_calc_pll(cfg->pll_ref_clk_rate_hz, hs_clk_rate,
				    &ipdiv, &opdiv, &fbdiv);
	LOG_DBG("DPHY PLL: ref=%u Hz, ipdiv=%u opdiv=%u fbdiv=%u → actual %u Hz",
		cfg->pll_ref_clk_rate_hz, ipdiv, opdiv, fbdiv, actual_rate);

	/* Set PWM (plain write, values from Linux j721e_dphy_ops) */
	dphy_wr(dev, DPHY_CMN_PWM,
		DPHY_CMN_PWM_HIGH(6) | DPHY_CMN_PWM_LOW(0x101) | DPHY_CMN_PWM_DIV(0x8));

	/* Write PLL dividers to J721E WIZ register */
	dphy_wr(dev, DPHY_TX_WIZ_PLL_CTRL,
		(FIELD_PREP(DPHY_TX_WIZ_IPDIV, ipdiv) |
		 FIELD_PREP(DPHY_TX_WIZ_OPDIV, opdiv) |
		 FIELD_PREP(DPHY_TX_WIZ_FBDIV, fbdiv)));

	/* 4. De-assert lane reset */
	dphy_wr(dev, DPHY_TX_WIZ_RST_CTRL, DPHY_TX_WIZ_LANE_RSTB);

	/* 5. Set band control */
	ret = dphy_get_band_ctrl(actual_rate);
	if (ret < 0) {
		LOG_ERR("DPHY: HS rate %u Hz out of supported band", actual_rate);
		return ret;
	}
	band = (uint32_t)ret;
	dphy_wr(dev, DPHY_BAND_CFG,
		FIELD_PREP(DPHY_BAND_CFG_LEFT_BAND, band) |
		FIELD_PREP(DPHY_BAND_CFG_RIGHT_BAND, band));

	/* 6. Start TX state machine (read-modify-write, preserving all bits) */
	reg = dphy_rd(dev, DPHY_CMN_SSM);
	dphy_wr(dev, DPHY_CMN_SSM, reg | DPHY_CMN_SSM_EN | DPHY_CMN_TX_MODE_EN);

	/* 7. Wait for J721E WIZ PLL lock (bit 31 of PLL_CTRL) */
	timeout = 1000U;

	do {
		status = dphy_rd(dev, DPHY_TX_WIZ_PLL_CTRL);
		if (status & DPHY_TX_WIZ_PLL_LOCK) {
			break;
		}
		k_busy_wait(1U);
	} while (--timeout);

	if (!(status & DPHY_TX_WIZ_PLL_LOCK)) {
		LOG_ERR("DPHY: PLL lock timed out (WIZ_PLL_CTRL=0x%08x)", status);
		return -ETIMEDOUT;
	}

	/* 8. Wait for WIZ CMN ready (bit 31 of WIZ_STATUS) */
	timeout = 1000U;
	do {
		status = dphy_rd(dev, DPHY_TX_WIZ_STATUS);
		if (status & DPHY_TX_WIZ_PLL_LOCK) {
			break;
		}
		k_busy_wait(1U);
	} while (--timeout);

	if (!(status & DPHY_TX_WIZ_PLL_LOCK)) {
		LOG_ERR("DPHY: CMN ready timed out");
		LOG_ERR("  WIZ_PLL_CTRL = 0x%08x", dphy_rd(dev, DPHY_TX_WIZ_PLL_CTRL));
		LOG_ERR("  WIZ_STATUS   = 0x%08x", status);
		LOG_ERR("  WIZ_RST_CTRL = 0x%08x", dphy_rd(dev, DPHY_TX_WIZ_RST_CTRL));
		LOG_ERR("  CMN_SSM      = 0x%08x", dphy_rd(dev, DPHY_CMN_SSM));
		return -ETIMEDOUT;
	}

	LOG_INF("DPHY: PLL locked, CMN ready — WIZ_PLL_CTRL=0x%08x WIZ_STATUS=0x%08x HS=%u Hz",
		dphy_rd(dev, DPHY_TX_WIZ_PLL_CTRL), status, actual_rate);
	return (int)actual_rate;
}

/*  DSI TX link init */
static void dsi_init_link(const struct device *dev, uint8_t nlanes,
			  uint32_t dsi_sys_clk_hz)
{
	uint32_t val, ulpout;
	int i;

	/* Enable data lanes 1..N-1 (lane 0 always on) and continuous clock */
	val = CLK_CONTINUOUS;
	for (i = 1; i < nlanes; i++) {
		val |= DATA_LANE_EN(i);
	}
	dsi_wr(dev, MCTL_MAIN_PHY_CTL, val);

	/* ULPOUT: 1 ms in sys_clk cycles */
	ulpout = DIV_ROUND_UP(dsi_sys_clk_hz, 1000U);
	dsi_wr(dev, MCTL_ULPOUT_TIME,
	       CLK_LANE_ULPOUT_TIME(ulpout) | DATA_LANE_ULPOUT_TIME(ulpout));

	/* Enable DSI link */
	dsi_wr(dev, MCTL_MAIN_DATA_CTL, LINK_EN);

	/* Start PLL and all lanes */
	val = CLK_LANE_EN | PLL_START;
	for (i = 0; i < nlanes; i++) {
		val |= DATA_LANE_START(i);
	}
	dsi_wr(dev, MCTL_MAIN_EN, val);
}

/*  DSI TX HS init  (equivalent to cdns_dsi_hs_init) */
static int dsi_hs_init(const struct device *dev, uint8_t nlanes)
{
	uint32_t status, tmp;
	uint32_t timeout;

	/* Power down all DPHY blocks */
	dsi_wr(dev, MCTL_DPHY_CFG0,
	       DPHY_CMN_PSO | DPHY_PLL_PSO | DPHY_ALL_D_PDN |
	       DPHY_C_PDN | DPHY_CMN_PDN | DPHY_PLL_PDN);

	/* Clear PLL_LOCKED status */
	dsi_wr(dev, MCTL_MAIN_STS_CLR, PLL_LOCKED);

	/* Power down (keep resets asserted) */
	dsi_wr(dev, MCTL_DPHY_CFG0,
	       DPHY_CMN_PSO | DPHY_ALL_D_PDN | DPHY_C_PDN | DPHY_CMN_PDN);

	/* Wait for DSI TX PLL lock */
	timeout = 1000U;
	do {
		status = dsi_rd(dev, MCTL_MAIN_STS);
		if (status & PLL_LOCKED) {
			break;
		}
		k_busy_wait(100U);
	} while (--timeout);

	if (!(status & PLL_LOCKED)) {
		LOG_ERR("DSI TX: PLL did not lock (MCTL_MAIN_STS=0x%08x)", status);
		return -ETIMEDOUT;
	}

	/* De-assert data and clock lane resets */
	dsi_wr(dev, MCTL_DPHY_CFG0,
	       DPHY_CMN_PSO | DPHY_ALL_D_PDN | DPHY_C_PDN | DPHY_CMN_PDN |
	       DPHY_D_RSTB(nlanes) | DPHY_C_RSTB);

	/* Wait for CLK and DATA lanes ready */
	tmp = CLK_LANE_RDY;
	for (int i = 0; i < nlanes; i++) {
		tmp |= DATA_LANE_RDY(i);
	}

	timeout = 5000U;
	do {
		status = dsi_rd(dev, MCTL_MAIN_STS);
		if ((status & tmp) == tmp) {
			break;
		}
		k_busy_wait(100U);
	} while (--timeout);

	if ((status & tmp) != tmp) {
		LOG_ERR("DSI TX: lanes not ready — MCTL_MAIN_STS=0x%08x want=0x%08x",
			status, tmp);
		LOG_ERR("  MCTL_DPHY_CFG0  = 0x%08x", dsi_rd(dev, MCTL_DPHY_CFG0));
		LOG_ERR("  MCTL_MAIN_EN    = 0x%08x", dsi_rd(dev, MCTL_MAIN_EN));
		LOG_ERR("  MCTL_MAIN_PHY_CTL=0x%08x", dsi_rd(dev, MCTL_MAIN_PHY_CTL));
		return -ETIMEDOUT;
	}

	LOG_INF("DSI TX: lanes ready — MCTL_MAIN_STS=0x%08x MCTL_MAIN_EN=0x%08x",
		status, dsi_rd(dev, MCTL_MAIN_EN));
	return 0;
}

/*  Horizontal timing conversion: DPI pixels → DSI bytes */
static inline uint32_t dpi_to_dsi_timing(uint32_t dpi_timing,
					 uint32_t bpp,
					 uint32_t overhead)
{
	uint32_t t = DIV_ROUND_UP(dpi_timing * bpp, 8U);

	return (t >= overhead) ? (t - overhead) : 0U;
}

/*  Program DSI TX video timing and enable video output */
static int dsi_set_video_mode(const struct device *dev,
			      const struct mipi_dsi_device *mdev,
			      uint32_t dsi_sys_clk_hz,
			      uint32_t actual_hs_clk_hz)
{
	const struct dsi_cdns_ti_config *cfg = DEV_CFG(dev);
	const struct mipi_dsi_timings *t = &mdev->timings;
	bool sync_pulse = !!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE);
	int bpp = dsi_fmt_to_bpp(mdev->pixfmt);
	uint8_t nlanes = (uint8_t)cfg->data_lanes;

	if (bpp < 0) {
		LOG_ERR("DSI: unsupported pixel format 0x%x", mdev->pixfmt);
		return -EINVAL;
	}

	/* ---- Horizontal DSI byte counts ---- */
	uint32_t hsa, hbp, hfp, hact, htotal;

	if (sync_pulse) {
		hsa = dpi_to_dsi_timing(t->hsync, (uint32_t)bpp,
					DSI_HSA_FRAME_OVERHEAD);
		hbp = dpi_to_dsi_timing(t->hbp,  (uint32_t)bpp,
					DSI_HBP_FRAME_PULSE_OVERHEAD);
	} else {
		hsa = 0U;
		hbp = dpi_to_dsi_timing(t->hbp + t->hsync, (uint32_t)bpp,
					DSI_HBP_FRAME_EVENT_OVERHEAD);
	}
	hact = dpi_to_dsi_timing(t->hactive, (uint32_t)bpp, 0U);
	hfp  = dpi_to_dsi_timing(t->hfp,     (uint32_t)bpp,
				 DSI_HFP_FRAME_OVERHEAD);

	htotal = hact + hfp + DSI_HFP_FRAME_OVERHEAD;
	if (sync_pulse) {
		htotal += hbp + DSI_HBP_FRAME_PULSE_OVERHEAD;
		htotal += hsa + DSI_HSA_FRAME_OVERHEAD;
	} else {
		htotal += hbp + DSI_HBP_FRAME_EVENT_OVERHEAD;
	}

	/* ---- Write horizontal timing registers ---- */
	dsi_wr(dev, VID_HSIZE1, HBP_LEN(hbp) | HSA_LEN(hsa));
	dsi_wr(dev, VID_HSIZE2, HFP_LEN(hfp) | HACT_LEN(hact));

	dsi_wr(dev, VID_VSIZE1,
	       VBP_LEN(t->vbp - 1U) | VFP_LEN(t->vfp - 1U) | VSA_LEN(t->vsync + 1U));
	dsi_wr(dev, VID_VSIZE2, t->vactive);

	/* ---- Blanking packet sizes ---- */
	uint32_t tmp;

	/* Sync-pulse mode: blanking2 = htotal - hsa overhead. */
	tmp = htotal - (hsa + DSI_BLANKING_FRAME_OVERHEAD + DSI_HSA_FRAME_OVERHEAD);
	dsi_wr(dev, VID_BLKSIZE2, BLK_LINE_PULSE_PKT_LEN(tmp));
	if (sync_pulse) {
		dsi_wr(dev, VID_VCA_SETTING2,
		       MAX_LINE_LIMIT(tmp - DSI_NULL_FRAME_OVERHEAD));
	}

	/* Sync-event (non-burst) mode: blanking1 = htotal - VSS/VBP overhead.*/
	tmp = htotal - (DSI_HSS_VSS_VSE_FRAME_OVERHEAD + DSI_BLANKING_FRAME_OVERHEAD);
	dsi_wr(dev, VID_BLKSIZE1, BLK_LINE_EVENT_PKT_LEN(tmp));
	if (!sync_pulse) {
		dsi_wr(dev, VID_VCA_SETTING2,
		       MAX_LINE_LIMIT(tmp - DSI_NULL_FRAME_OVERHEAD));
	}

	/* ---- DPHY line timing ---- */
	tmp = DIV_ROUND_UP(htotal, nlanes) - DIV_ROUND_UP(hsa, nlanes);
	if (!(mdev->mode_flags & MIPI_DSI_MODE_EOT_PACKET)) {
		tmp -= DIV_ROUND_UP(DSI_EOT_PKT_SIZE, nlanes);
	}

	/* Wakeup time: ~1/10 of total line duration in DSI byte-lane cycles. */
	uint32_t reg_wakeup = htotal / nlanes / 10U;

	dsi_wr(dev, VID_DPHY_TIME,
	       REG_WAKEUP_TIME(reg_wakeup) | REG_LINE_DURATION(tmp));

	/* ---- HSTX / LPRX timeouts ---- */
	/* tx_byte_period = 8 ns / hs_clk_rate_GHz = 8e9 / hs_clk_rate (ns) */
	uint64_t tx_byte_period_ns = (uint64_t)8000000000ULL / actual_hs_clk_hz;
	/* Frame period in ns ≈ 1e9 / frame_rate.  Approximate as vbp+vfp+vsync+vactive lines
	 * × htotal_ns.  Use a simpler formula: timeout_cycles = 1e9 / (tx_byte_period_ns) / 60
	 * (assuming 60 fps). */
	uint64_t timeout_cycles = 1000000000ULL / (tx_byte_period_ns * 60U);
	uint32_t div = 0U;

	while (timeout_cycles > (uint64_t)HSTX_TIMEOUT_MAX &&
	       div < (uint32_t)CLK_DIV_MAX) {
		timeout_cycles >>= 1;
		div++;
	}
	if (timeout_cycles > (uint64_t)HSTX_TIMEOUT_MAX) {
		timeout_cycles = HSTX_TIMEOUT_MAX;
	}
	dsi_wr(dev, MCTL_DPHY_TIMEOUT1,
	       CLK_DIV(div) | HSTX_TIMEOUT((uint32_t)timeout_cycles));
	dsi_wr(dev, MCTL_DPHY_TIMEOUT2, LPRX_TIMEOUT((uint32_t)timeout_cycles));

	/* ---- Video main control (format, mode, blanking) ---- */
	uint32_t vid_ctl;

	switch (mdev->pixfmt) {
	case MIPI_DSI_PIXFMT_RGB888:
		vid_ctl = VID_PIXEL_MODE_RGB888 |
			  VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_24);
		break;
	case MIPI_DSI_PIXFMT_RGB666:
		vid_ctl = VID_PIXEL_MODE_RGB666 |
			  VID_DATATYPE(MIPI_DSI_PIXEL_STREAM_3BYTE_18);
		break;
	case MIPI_DSI_PIXFMT_RGB666_PACKED:
		vid_ctl = VID_PIXEL_MODE_RGB666_PACKED |
			  VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_18);
		break;
	case MIPI_DSI_PIXFMT_RGB565:
		vid_ctl = VID_PIXEL_MODE_RGB565 |
			  VID_DATATYPE(MIPI_DSI_PACKED_PIXEL_STREAM_16);
		break;
	default:
		return -EINVAL;
	}

	if (sync_pulse) {
		vid_ctl |= SYNC_PULSE_ACTIVE | SYNC_PULSE_HORIZONTAL;
	}
	vid_ctl |= REG_BLKLINE_MODE(REG_BLK_MODE_BLANKING_PKT) |
		   REG_BLKEOL_MODE(REG_BLK_MODE_BLANKING_PKT) |
		   RECOVERY_MODE(RECOVERY_MODE_NEXT_HSYNC) |
		   VID_IGNORE_MISS_VSYNC;
	dsi_wr(dev, VID_MAIN_CTL, vid_ctl);

	/* ---- Enable video output ---- */
	tmp = dsi_rd(dev, MCTL_MAIN_DATA_CTL);
	tmp &= ~(IF_VID_SELECT_MASK | HOST_EOT_GEN | IF_VID_MODE);

	if (!(mdev->mode_flags & MIPI_DSI_MODE_EOT_PACKET)) {
		tmp |= HOST_EOT_GEN;
	}
	if ((mdev->mode_flags & MIPI_DSI_MODE_VIDEO) &&
	    !(mdev->mode_flags & MIPI_DSI_CDNS_TI_MODE_DEFER_VIDEO)) {
		tmp |= IF_VID_MODE | IF_VID_SELECT(1) | VID_EN;
	}
	dsi_wr(dev, MCTL_MAIN_DATA_CTL, tmp);

	if (!(mdev->mode_flags & MIPI_DSI_CDNS_TI_MODE_DEFER_VIDEO)) {
		tmp = dsi_rd(dev, MCTL_MAIN_EN) | IF_EN(1);
		dsi_wr(dev, MCTL_MAIN_EN, tmp);
	}

	return 0;
}

/*  mipi_dsi_driver_api  —  attach */
static int cdns_ti_dsi_attach(const struct device *dev, uint8_t channel,
			      const struct mipi_dsi_device *mdev)
{
	const struct dsi_cdns_ti_config *cfg = DEV_CFG(dev);
	struct dsi_cdns_ti_data *data = DEV_DATA(dev);
	uint32_t dsi_sys_clk_hz, hs_clk_rate, actual_hs_clk;
	int bpp, ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->link_up) {
		LOG_WRN("DSI: already attached — detach first");
		k_mutex_unlock(&data->lock);
		return -EALREADY;
	}

	bpp = dsi_fmt_to_bpp(mdev->pixfmt);
	if (bpp < 0) {
		LOG_ERR("DSI attach: unsupported pixel format 0x%x", mdev->pixfmt);
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	/* HS bit rate = pixel_clk * bpp / nlanes */
	uint32_t htotal = mdev->timings.hactive + mdev->timings.hfp +
			  mdev->timings.hbp + mdev->timings.hsync;
	uint32_t vtotal = mdev->timings.vactive + mdev->timings.vfp +
			  mdev->timings.vbp + mdev->timings.vsync;
	/* pixel_clk_hz: use 60 fps as baseline; DT property overrides below */
	uint32_t pixel_clk_hz = htotal * vtotal * 60U;

	/*
	 * HS bit rate = pixel_clk × bpp / nlanes — NO margin. */
	hs_clk_rate = (uint32_t)((uint64_t)pixel_clk_hz * (uint32_t)bpp /
				  cfg->data_lanes);

	LOG_INF("DSI attach: %ux%u @60fps, %u lanes, bpp=%d, hs_clk=%u Hz",
		mdev->timings.hactive, mdev->timings.vactive,
		cfg->data_lanes, bpp, hs_clk_rate);

	/* --- Step 1: Enable clocks --- */
	ret = clock_control_on(cfg->psm_clk_dev, cfg->psm_clk_subsys);
	if (ret < 0) { LOG_ERR("psm_clk on: %d", ret); goto err; }

	ret = clock_control_on(cfg->pll_ref_clk_dev, cfg->pll_ref_clk_subsys);
	if (ret < 0) { LOG_ERR("pll_ref_clk on: %d", ret); goto err; }

	/* Set pll_ref_clk to the configured rate (25 MHz on AM62L/AM62P) */
	ret = clock_control_set_rate(cfg->pll_ref_clk_dev,
				     cfg->pll_ref_clk_subsys,
				     (clock_control_subsys_rate_t)(uintptr_t)
				     cfg->pll_ref_clk_rate_hz);
	if (ret < 0) {
		LOG_WRN("pll_ref_clk set_rate failed: %d (continuing)", ret);
	}

	/* --- Step 2: Power on DPHY --- */
	ret = dphy_power_on(dev, hs_clk_rate);
	if (ret < 0) { goto err; }
	actual_hs_clk = (uint32_t)ret;

	/* --- Step 3: Get DSI sys clock rate for ULPOUT calculation --- */
	ret = clock_control_get_rate(cfg->dsi_sys_clk_dev,
				     cfg->dsi_sys_clk_subsys,
				     &dsi_sys_clk_hz);
	if (ret < 0 || dsi_sys_clk_hz == 0U) {
		LOG_WRN("Cannot read dsi_sys_clk rate, assuming 96 MHz");
		dsi_sys_clk_hz = 96000000U;
	}

	/* --- Step 3b: Program MCTL_DPHY_TIMEOUT registers BEFORE LINK_EN --- */
	uint64_t frame_cycles = (uint64_t)actual_hs_clk / (60U * 8U);
	uint32_t div = 0U;

	while (frame_cycles > HSTX_TIMEOUT_MAX &&
		div <= (uint32_t)CLK_DIV_MAX) {
		frame_cycles >>= 1;
		div++;
	}
	if (frame_cycles > HSTX_TIMEOUT_MAX) {
		frame_cycles = HSTX_TIMEOUT_MAX;
	}

	dsi_wr(dev, MCTL_DPHY_TIMEOUT1,
		CLK_DIV(div) | HSTX_TIMEOUT((uint32_t)frame_cycles));
	dsi_wr(dev, MCTL_DPHY_TIMEOUT2,
		LPRX_TIMEOUT((uint32_t)frame_cycles));

	/* --- Step 4: Init DSI link (sets LINK_EN) --- */
	dsi_init_link(dev, (uint8_t)cfg->data_lanes, dsi_sys_clk_hz);

	/* --- Step 5: Bring up HS (DPHY reset release, lane ready poll) --- */
	ret = dsi_hs_init(dev, (uint8_t)cfg->data_lanes);
	if (ret < 0) { goto err; }

	/* --- Step 6: Program video timing (only in video mode) --- */
	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO) {
		ret = dsi_set_video_mode(dev, mdev, dsi_sys_clk_hz, actual_hs_clk);
		if (ret < 0) { goto err; }
	}

	/* Step 7: Connect DSS DPI input to Cadence DSI TX. */
	wrap_wr(dev, DSI_WRAP_DPI_CONTROL, DSI_WRAP_DPI_0_EN);

	data->link_up = true;
	LOG_INF("DSI: link up, HS clk = %u Hz", actual_hs_clk);
	k_mutex_unlock(&data->lock);
	return 0;

err:
	/* Disable clocks on failure */
	clock_control_off(cfg->psm_clk_dev, cfg->psm_clk_subsys);
	clock_control_off(cfg->pll_ref_clk_dev, cfg->pll_ref_clk_subsys);
	k_mutex_unlock(&data->lock);
	return ret;
}

/*  mipi_dsi_driver_api  —  transfer  (DCS command, LP mode) */
static ssize_t cdns_ti_dsi_transfer(const struct device *dev, uint8_t channel,
				    struct mipi_dsi_msg *msg)
{
	struct dsi_cdns_ti_data *data = DEV_DATA(dev);
	uint32_t cmd, sts, val, wait_mask;
	int i, j;
	int tx_len, rx_len;
	int ret = 0;

	if (!data->link_up) {
		return -ENODEV;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Reset command FIFO */
	dsi_wr(dev, DIRECT_CMD_FIFO_RST, 0U);

	tx_len = msg->tx_buf ? (int)msg->tx_len : 0;
	rx_len = msg->rx_buf ? (int)msg->rx_len : 0;

	/*
	 * MIPI DSI packet type bit 5 distinguishes long (1) from short (0).
	 *   0x05 DCS_SHORT_WRITE         → short
	 *   0x15 DCS_SHORT_WRITE_PARAM   → short  (note: 0x15 >= 0x10, old check was wrong)
	 *   0x39 DCS_LONG_WRITE          → long
	 *   0x29 GENERIC_LONG_WRITE      → long
	 */
	bool is_long = !!(msg->type & BIT(5));

	uint8_t fifo_buf[66];  /* 1 cmd byte + up to 64 param bytes + 1 spare */
	int fifo_len;

	if (is_long) {
		if (msg->cmd != 0U) {
			/* DCS long write: prepend command byte */
			fifo_buf[0] = msg->cmd;
			if (tx_len > 0) {
				memcpy(&fifo_buf[1], msg->tx_buf, (size_t)tx_len);
			}
			fifo_len = tx_len + 1;
		} else {
			/* Generic long write: payload is entirely in tx_buf */
			fifo_len = tx_len;
		}
		if (fifo_len > 65) { ret = -ENOTSUP; goto out; }
	} else {
		/* Short packet: always exactly 2 data bytes */
		fifo_buf[0] = msg->cmd;
		fifo_buf[1] = (tx_len > 0) ? ((const uint8_t *)msg->tx_buf)[0] : 0U;
		fifo_len = 2;
	}

	if (rx_len > 16) { ret = -ENOTSUP; goto out; }

	/* Build DIRECT_CMD_MAIN_SETTINGS */
	cmd = CMD_SIZE(fifo_len) |
	      CMD_VCHAN_ID(channel) |
	      CMD_DATATYPE(msg->type);

	if ((msg->flags & MIPI_DSI_MSG_USE_LPM) || !(data->link_up)) {
		cmd |= CMD_LP_EN;
	}

	if (is_long) {
		cmd |= CMD_LONG;
	}

	/* Configure wait mask and BTA if needed */
	if (rx_len > 0) {
		wait_mask = READ_COMPLETED | READ_COMPLETED_WITH_ERR;
		dsi_wr(dev, MCTL_MAIN_DATA_CTL,
		       dsi_rd(dev, MCTL_MAIN_DATA_CTL) | READ_EN | BTA_EN);
	} else {
		wait_mask = WRITE_COMPLETED;
	}

	/* Write payload to FIFO */
	for (i = 0; i < fifo_len; i += 4) {
		val = 0U;
		for (j = 0; j < 4 && (i + j) < fifo_len; j++) {
			val |= (uint32_t)fifo_buf[i + j] << (8U * j);
		}
		dsi_wr(dev, DIRECT_CMD_WRDATA, val);
	}

	/* Clear previous status, arm, send */
	dsi_wr(dev, DIRECT_CMD_STS_CLR, wait_mask);
	dsi_wr(dev, DIRECT_CMD_STS_CTL, wait_mask);
	dsi_wr(dev, DIRECT_CMD_MAIN_SETTINGS, cmd);
	dsi_wr(dev, DIRECT_CMD_SEND, 0U);

	/* Poll for completion (up to 100 ms) */
	uint32_t timeout = 100000U;   /* × 1 µs busy_wait */

	do {
		sts = dsi_rd(dev, DIRECT_CMD_STS);
		if (sts & wait_mask) {
			break;
		}
		k_busy_wait(1U);
	} while (--timeout);

	dsi_wr(dev, DIRECT_CMD_STS_CLR, wait_mask);
	dsi_wr(dev, DIRECT_CMD_STS_CTL, 0U);

	/* Clear BTA_EN / READ_EN if they were set */
	if (rx_len > 0) {
		dsi_wr(dev, MCTL_MAIN_DATA_CTL,
		       dsi_rd(dev, MCTL_MAIN_DATA_CTL) & ~(READ_EN | BTA_EN));
	}

	if (!(sts & wait_mask)) {
		LOG_ERR("DSI transfer timeout (type=0x%02x len=%d)", msg->type, tx_len);
		/* Abort any pending read/BTA to prevent the TX lane SM getting stuck */
		dsi_wr(dev, DIRECT_CMD_STOP_READ, 0U);
		ret = -ETIMEDOUT;
		goto out;
	}

	if (sts & (READ_COMPLETED_WITH_ERR | ACK_WITH_ERR_RCVD)) {
		LOG_ERR("DSI transfer error (STS=0x%08x)", sts);
		ret = -EIO;
		goto out;
	}

	/* Read response */
	for (i = 0; i < rx_len; i += 4) {
		uint8_t *buf = msg->rx_buf;
		int k;

		val = dsi_rd(dev, DIRECT_CMD_RDDATA);
		for (k = 0; k < 4 && (i + k) < rx_len; k++) {
			buf[i + k] = (uint8_t)(val >> (8U * k));
		}
	}

	ret = tx_len;

out:
	k_mutex_unlock(&data->lock);
	return (ssize_t)ret;
}

/*  mipi_dsi_driver_api  —  detach */
static int cdns_ti_dsi_detach(const struct device *dev, uint8_t channel,
			      const struct mipi_dsi_device *mdev)
{
	const struct dsi_cdns_ti_config *cfg = DEV_CFG(dev);
	struct dsi_cdns_ti_data *data = DEV_DATA(dev);
	uint32_t val;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->link_up) {
		k_mutex_unlock(&data->lock);
		return 0;
	}

	/* Disable video output */
	val = dsi_rd(dev, MCTL_MAIN_DATA_CTL);
	val &= ~(IF_VID_SELECT_MASK | IF_VID_MODE | VID_EN |
		 HOST_EOT_GEN | READ_EN | BTA_EN);
	dsi_wr(dev, MCTL_MAIN_DATA_CTL, val);

	/* Disable interface */
	val = dsi_rd(dev, MCTL_MAIN_EN) & ~IF_EN(1);
	dsi_wr(dev, MCTL_MAIN_EN, val);

	/* Disable TI wrapper */
	wrap_wr(dev, DSI_WRAP_DPI_CONTROL, 0U);

	/* Power down DPHY */
	dphy_wr(dev, DPHY_CMN_SSM,
		dphy_rd(dev, DPHY_CMN_SSM) & ~DPHY_CMN_SSM_EN);

	/* Disable clocks */
	clock_control_off(cfg->psm_clk_dev, cfg->psm_clk_subsys);
	clock_control_off(cfg->pll_ref_clk_dev, cfg->pll_ref_clk_subsys);

	data->link_up = false;
	LOG_INF("DSI: detached");
	k_mutex_unlock(&data->lock);
	return 0;
}

static DEVICE_API(mipi_dsi, cdns_ti_dsi_api) = {
	.attach   = cdns_ti_dsi_attach,
	.transfer = cdns_ti_dsi_transfer,
	.detach   = cdns_ti_dsi_detach,
};

/*  Driver init — map MMIO, enable peripheral clocks */
static int cdns_ti_dsi_init(const struct device *dev)
{
	const struct dsi_cdns_ti_config *cfg = DEV_CFG(dev);
	struct dsi_cdns_ti_data *data = DEV_DATA(dev);
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, dsi,  K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, wrap, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, dphy, K_MEM_CACHE_NONE);

	k_mutex_init(&data->lock);
	data->link_up = false;

	/* Enable peripheral access clocks (always on) */
	if (!device_is_ready(cfg->dsi_p_clk_dev)) {
		LOG_ERR("dsi_p_clk device not ready");
		return -ENODEV;
	}
	ret = clock_control_on(cfg->dsi_p_clk_dev, cfg->dsi_p_clk_subsys);
	if (ret < 0) {
		LOG_ERR("dsi_p_clk on: %d", ret);
		return ret;
	}

	if (!device_is_ready(cfg->dsi_sys_clk_dev)) {
		LOG_ERR("dsi_sys_clk device not ready");
		return -ENODEV;
	}
	ret = clock_control_on(cfg->dsi_sys_clk_dev, cfg->dsi_sys_clk_subsys);
	if (ret < 0) {
		LOG_ERR("dsi_sys_clk on: %d", ret);
		return ret;
	}

	LOG_INF("TI/Cadence DSI host ready (%u lanes)", cfg->data_lanes);
	return 0;
}

/*  Per-instance device registration macro */
#define CDNS_TI_DSI_DEVICE(n)                                                   \
	static struct dsi_cdns_ti_data dsi_cdns_ti_data_##n;                    \
                                                                                \
	static const struct dsi_cdns_ti_config dsi_cdns_ti_config_##n = {      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(dsi,  DT_DRV_INST(n)),      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(wrap, DT_DRV_INST(n)),      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(dphy, DT_DRV_INST(n)),      \
		.dsi_p_clk_dev    = DEVICE_DT_GET(                              \
			DT_INST_CLOCKS_CTLR_BY_NAME(n, dsi_p_clk)),            \
		.dsi_p_clk_subsys = (clock_control_subsys_t)                    \
			DT_INST_CLOCKS_CELL_BY_NAME(n, dsi_p_clk, name),       \
		.dsi_sys_clk_dev  = DEVICE_DT_GET(                              \
			DT_INST_CLOCKS_CTLR_BY_NAME(n, dsi_sys_clk)),          \
		.dsi_sys_clk_subsys = (clock_control_subsys_t)                  \
			DT_INST_CLOCKS_CELL_BY_NAME(n, dsi_sys_clk, name),     \
		.psm_clk_dev      = DEVICE_DT_GET(                              \
			DT_INST_CLOCKS_CTLR_BY_NAME(n, psm_clk)),              \
		.psm_clk_subsys   = (clock_control_subsys_t)                    \
			DT_INST_CLOCKS_CELL_BY_NAME(n, psm_clk, name),         \
		.pll_ref_clk_dev  = DEVICE_DT_GET(                              \
			DT_INST_CLOCKS_CTLR_BY_NAME(n, pll_ref_clk)),          \
		.pll_ref_clk_subsys = (clock_control_subsys_t)                  \
			DT_INST_CLOCKS_CELL_BY_NAME(n, pll_ref_clk, name),     \
		.pll_ref_clk_rate_hz =                                          \
			DT_INST_PROP(n, ti_pll_ref_clk_rate_hz),               \
		.data_lanes = DT_INST_PROP(n, data_lanes),                      \
	};                                                                      \
                                                                                \
	DEVICE_DT_INST_DEFINE(n,                                                \
			      cdns_ti_dsi_init,                                  \
			      NULL,                                              \
			      &dsi_cdns_ti_data_##n,                            \
			      &dsi_cdns_ti_config_##n,                          \
			      POST_KERNEL,                                       \
			      CONFIG_MIPI_DSI_INIT_PRIORITY,                    \
			      &cdns_ti_dsi_api);

DT_INST_FOREACH_STATUS_OKAY(CDNS_TI_DSI_DEVICE)
