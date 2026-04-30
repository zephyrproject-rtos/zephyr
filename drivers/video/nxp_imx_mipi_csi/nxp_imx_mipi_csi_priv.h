/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_NXP_IMX_MIPI_CSI_PRIV_H_
#define ZEPHYR_DRIVERS_VIDEO_NXP_IMX_MIPI_CSI_PRIV_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* CSIS (Synopsys) registers (subset) */
#define MIPI_CSI2RX_VERSION                 0x00
#define MIPI_CSI2RX_N_LANES                 0x04
#define MIPI_CSI2RX_HOST_RESETN             0x08
#define MIPI_CSI2RX_DPHY_STOPSTATE          0x4C
#define MIPI_CSI2RX_N_LANES_MASK            GENMASK(2, 0)
#define MIPI_CSI2RX_HOST_RESETN_ENABLE      BIT(0)
#define MIPI_CSI2RX_DPHY_STOPSTATE_CLK_LANE BIT(16)

#define CSI_RESET_DELAY_US          10
#define CSI_STOPSTATE_TIMEOUT_US    50000
#define CSI_STOPSTATE_POLL_DELAY_US 50

/* DPHY control via CSIS registers */
#define CSIS_N_LANES         0x04
#define CSIS_DPHY_SHUTDOWNZ  0x40
#define CSIS_DPHY_RSTZ       0x44
#define CSIS_DPHY_STOPSTATE  0x4C
#define CSIS_DPHY_TEST_CTRL0 0x50
#define CSIS_DPHY_TEST_CTRL1 0x54

#define PHY_TESTCLR BIT(0)

#define N_LANES(x) FIELD_PREP(GENMASK(2, 0), ((x) - 1))

/* DPHY CSR (i.MX95) */
#define CSR_PHY_MODE_CTRL      0x00
#define CSR_PHY_FREQ_CTRL      0x04
#define CSR_PHY_TEST_MODE_CTRL 0x08

#define MHZ_TO_HZ               1000000ULL
#define DPHY_MIN_DATA_RATE_MBPS 80
#define DPHY_MAX_DATA_RATE_MBPS 2500

enum dphy_reg_id {
	DPHY_RX_CFGCLKFREQRANGE = 0,
	DPHY_RX_HSFREQRANGE,
	DPHY_RX_DATA_LANE_EN,
	DPHY_RX_DATA_LANE_BASEDIR,
	DPHY_RX_DATA_LANE_FORCERXMODE,
	DPHY_RX_ENABLE_CLK_EXT,
	DPHY_RX_PHY_ENABLE_BYP,
};

struct dw_dphy_reg {
	uint16_t offset;
	uint8_t mask;
	uint8_t shift;
};

#define PHY_REG(_offset, _width, _shift)                                                           \
	{.offset = _offset, .mask = BIT(_width) - 1, .shift = _shift}

struct dphy_mbps_hsfreqrange_map {
	uint16_t mbps;
	uint16_t hsfreqrange;
};

struct csi_pix_format {
	uint32_t pixelformat;
	uint8_t data_type;
	uint8_t bpp;
};

extern const struct csi_pix_format csi_formats[];

struct nxp_imx_dphy_drv_data;

struct nxp_imx_mipi_csi_config {
	mem_addr_t csis_base;
	mem_addr_t dphy_base;
	const struct device *dphy_clock_dev;
	clock_control_subsys_t dphy_clock_subsys;
	const struct device *sensor_dev;
	uint8_t num_lanes;

	/* Selected PHY backend (based on phys node compatible) */
	const struct nxp_imx_dphy_drv_data *dphy_drv_data;
};

struct nxp_imx_mipi_csi_data {
	mem_addr_t csis_regs;
	mem_addr_t dphy_regs;

	struct k_mutex lock;

	struct video_format fmt;
	struct video_frmival frmival;
	struct csi_pix_format csi_fmt;

	uint16_t hsfreqrange;
	uint16_t cfgclkfreqrange;
	bool frmival_set;
	bool format_set;

	bool streaming;
};

struct nxp_imx_dphy_config_ops {
	/**
	 * SoC/PHY-specific DPHY CSR configuration after common freqrange setup.
	 */
	int (*config)(struct nxp_imx_mipi_csi_data *d, const struct nxp_imx_mipi_csi_config *cfg);
};

struct nxp_imx_dphy_drv_data {
	const struct dw_dphy_reg *regs;
	uint32_t regs_size;
	const struct dphy_mbps_hsfreqrange_map *hsfreq_tbl;
	uint8_t max_lanes;
	uint32_t max_data_rate_mbps;
	const struct nxp_imx_dphy_config_ops *ops;
};

int nxp_imx_dphy_write(struct nxp_imx_mipi_csi_data *d, const struct nxp_imx_dphy_drv_data *drv,
		       unsigned int index, uint32_t val);

/* i.MX95 DPHY backend */
extern const struct nxp_imx_dphy_drv_data nxp_imx95_dphy_drv_data;

int nxp_imx_dphy_enable(struct nxp_imx_mipi_csi_data *d, const struct nxp_imx_mipi_csi_config *cfg);
int nxp_imx_dphy_disable(struct nxp_imx_mipi_csi_data *d,
			 const struct nxp_imx_mipi_csi_config *cfg);

#endif /* ZEPHYR_DRIVERS_VIDEO_NXP_IMX_MIPI_CSI_PRIV_H_ */
