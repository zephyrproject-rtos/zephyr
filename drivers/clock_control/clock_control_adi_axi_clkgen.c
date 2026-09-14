/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Analog Devices AXI Clock Generator core.
 * Based on the no-OS reference driver by Analog Devices.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>

#define DT_DRV_COMPAT adi_axi_clkgen

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adi_axi_clkgen, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define AXI_REG_VERSION			0x0000
#define AXI_VERSION_MAJOR		GENMASK(31, 16)
#define AXI_VERSION_MINOR		GENMASK(15, 8)
#define AXI_VERSION_PATCH		GENMASK(7, 0)

#define AXI_REG_FPGA_INFO		0x001c
#define AXI_INFO_FPGA_TECH		GENMASK(31, 24)
#define AXI_INFO_FPGA_FAMILY		GENMASK(23, 16)
#define AXI_INFO_FPGA_SPEED		GENMASK(15, 8)

#define AXI_REG_FPGA_VOLTAGE		0x0140
#define AXI_INFO_FPGA_VOLTAGE		GENMASK(15, 0)

#define AXI_CLKGEN_REG_RESETN		0x40
#define AXI_CLKGEN_MMCM_RESETN		BIT(1)
#define AXI_CLKGEN_RESETN		BIT(0)

#define AXI_CLKGEN_REG_STATUS		0x5c
#define AXI_CLKGEN_STATUS_LOCKED	BIT(0)

#define AXI_CLKGEN_REG_DRP_CNTRL	0x70
#define AXI_CLKGEN_DRP_CNTRL_SEL	BIT(29)
#define AXI_CLKGEN_DRP_CNTRL_READ	BIT(28)
#define AXI_CLKGEN_DRP_CNTRL_ADDR	GENMASK(23, 16)
#define AXI_CLKGEN_DRP_CNTRL_DATA	GENMASK(15, 0)

#define AXI_CLKGEN_REG_DRP_STATUS	0x74
#define AXI_CLKGEN_DRP_STATUS_BUSY	BIT(16)
#define AXI_CLKGEN_DRP_STATUS_DATA	GENMASK(15, 0)

/*
 * The DRP port is a slow serial-ish shim into the MMCM: one transfer takes a
 * few parent-clock cycles, so the busy flag clears in well under a
 * microsecond. The budget below is a stuck-fabric backstop, not a real wait,
 * and is deliberately far longer than any plausible transfer.
 */
#define MMCM_DRP_TIMEOUT_US		10000

/*
 * MMCM lock time after reset release. UG472 (7 series) / UG572 (UltraScale)
 * specify a maximum lock time of 100 us for the widest divider settings; 10 ms
 * is the same conservative budget the vendor drivers use.
 */
#define MMCM_LOCK_TIMEOUT_US		10000

#define MMCM_REG_CLKOUT0_1		0x08
#define MMCM_REG_CLKOUT0_2		0x09
#define MMCM_REG_CLKOUT1_1		0x0a
#define MMCM_REG_CLKOUT1_2		0x0b
#define MMCM_REG_CLK_FB1		0x14
#define MMCM_REG_CLK_FB2		0x15
#define MMCM_REG_CLK_DIV		0x16
#define MMCM_REG_LOCK1			0x18
#define MMCM_REG_LOCK2			0x19
#define MMCM_REG_LOCK3			0x1a
#define MMCM_REG_FILTER1		0x4e
#define MMCM_REG_FILTER2		0x4f

/*
 * Core revision from which FPGA_INFO/FPGA_VOLTAGE (and therefore per-speed-
 * grade PLL limits) are implemented.
 */
#define AXI_CLKGEN_VERSION_FPGA_INFO	4

/*
 * MMCM operating limits, in kHz (calc_params() works in kHz to keep the search
 * in 32-bit arithmetic). fpfd is the phase-detector frequency fin/D, fvco the
 * VCO frequency fin*M/D. The defaults are the intersection of all supported
 * families; the per-speed-grade values come from the 7 series and UltraScale
 * datasheets (DS181/DS182/DS183, DS922/DS925) and are refined at runtime by
 * setup_ranges() when the core reports its FPGA info.
 */
#define MMCM_FPFD_MIN_KHZ		10000
#define MMCM_FPFD_MAX_DEFAULT_KHZ	300000
#define MMCM_FVCO_MIN_DEFAULT_KHZ	600000
#define MMCM_FVCO_MAX_DEFAULT_KHZ	1200000
#define MMCM_FPFD_MAX_SPEED_1_KHZ	450000
#define MMCM_FVCO_MAX_SPEED_1_KHZ	1200000
#define MMCM_FPFD_MAX_SPEED_2_KHZ	500000
#define MMCM_FVCO_MAX_SPEED_2_KHZ	1440000
#define MMCM_FPFD_MAX_SPEED_3_KHZ	550000
#define MMCM_FVCO_MAX_SPEED_3_KHZ	1600000
#define MMCM_FVCO_MIN_USP_KHZ		800000
#define MMCM_FVCO_MAX_USP_KHZ		1600000

/* Core supply in mV below which -2 parts derate to the -1 limits. */
#define FPGA_VOLTAGE_DERATE_MV		950

/* Counter ranges: D (DIVCLK), M (CLKFBOUT_MULT), CLKOUTn_DIVIDE. */
#define MMCM_DIVCLK_DIVIDE_MAX		80
#define MMCM_CLKFBOUT_MULT_MAX		64
#define MMCM_CLKOUT_DIVIDE_MAX		128

/* CLKOUT1 is wired as CLKOUT0/4 inside the AXI core. */
#define MMCM_CLKOUT1_RATIO		4

/*
 * MMCM counter register layout, per Xilinx XAPP888 ("MMCM and PLL Dynamic
 * Reconfiguration"). Every output counter is a pair of registers: ClkReg1
 * holds the high/low duty counts, ClkReg2 the half-period edge and the
 * bypass ("no count") bit. The DIVCLK counter packs all four into one word.
 *
 * The *_WMASK values are the bits this driver owns; mmcm_write() does a
 * read-modify-write and leaves everything outside the mask alone, because the
 * remaining bits hold fractional-divide and phase-mux state that the AXI
 * core sets up at synthesis time and we must not clobber.
 */
#define MMCM_CLK_REG1_HIGH_TIME		GENMASK(11, 6)
#define MMCM_CLK_REG1_LOW_TIME		GENMASK(5, 0)
/* bit 12 is reserved and must be preserved; bits 15:13 are the phase mux */
#define MMCM_CLK_REG1_WMASK		0xefff

#define MMCM_CLK_REG2_EDGE		BIT(7)
#define MMCM_CLK_REG2_NOCOUNT		BIT(6)
#define MMCM_CLK_REG2_DELAY_TIME	GENMASK(5, 0)
#define MMCM_CLK_REG2_WMASK		0x03ff

#define MMCM_DIV_REG_EDGE		BIT(13)
#define MMCM_DIV_REG_NOCOUNT		BIT(12)
#define MMCM_DIV_REG_HIGH_TIME		GENMASK(11, 6)
#define MMCM_DIV_REG_LOW_TIME		GENMASK(5, 0)
#define MMCM_DIV_REG_WMASK		0x3fff

/* Field layout of one lock_table[] entry (see XAPP888 Table 3). */
#define MMCM_LOCK_TBL_CNT		GENMASK(9, 0)
#define MMCM_LOCK_TBL_SAT_HIGH		GENMASK(20, 16)
#define MMCM_LOCK_TBL_REF_DLY		GENMASK(28, 24)

#define MMCM_LOCK_REG_CNT		GENMASK(9, 0)
#define MMCM_LOCK_REG_DLY		GENMASK(14, 10)
#define MMCM_LOCK_REG1_WMASK		0x03ff
#define MMCM_LOCK_REG23_WMASK		0x7fff
/* Fixed unlock/lock counts prescribed by XAPP888 for the Lock_2/Lock_3 regs */
#define MMCM_LOCK_REG2_CNT		0x001
#define MMCM_LOCK_REG3_CNT		0x3e9

/*
 * A filter_table[] entry is an opaque, silicon-characterised bit pattern: the
 * upper half-word goes to Filter_1, the lower to Filter_2, and only the four
 * bits in the mask are actual filter controls.
 */
#define MMCM_FILTER_REG_WMASK		0x9900
#define MMCM_FILTER_HIGH		GENMASK(31, 16)

#define MMCM_DATA_MASK			0xffff

enum {
	FPGA_FAMILY_UNKNOWN = 0,
	FPGA_FAMILY_ARTIX,
	FPGA_FAMILY_KINTEX,
	FPGA_FAMILY_VIRTEX,
	FPGA_FAMILY_ZYNQ,
};

enum {
	FPGA_SPEED_UNKNOWN = 0,
	FPGA_SPEED_1  = 10,
	FPGA_SPEED_1L = 11,
	FPGA_SPEED_1H = 12,
	FPGA_SPEED_1HV = 13,
	FPGA_SPEED_1LV = 14,
	FPGA_SPEED_2  = 20,
	FPGA_SPEED_2L = 21,
	FPGA_SPEED_2LV = 22,
	FPGA_SPEED_3  = 30,
};

enum {
	FPGA_TECH_UNKNOWN = 0,
	FPGA_TECH_7SERIES,
	FPGA_TECH_ULTRASCALE,
	FPGA_TECH_ULTRASCALE_PLUS,
};

/*
 * Loop-filter and lock-detect settings indexed by the feedback divider M-1.
 * Both tables are lifted verbatim from Xilinx XAPP888; the entries are
 * silicon-characterised patterns, not computable values. Dividers past the end
 * of a table all use the same terminal entry.
 */
#define MMCM_FILTER_DEFAULT		0x08008090
#define MMCM_LOCK_DEFAULT		0x1f1f00fa

static const uint32_t filter_table[] = {
	0x01001990, 0x01001190, 0x01009890, 0x01001890,
	0x01008890, 0x01009090, 0x01009090, 0x01009090,
	0x01009090, 0x01000890, 0x01000890, 0x01000890,
	0x08009090, 0x01001090, 0x01001090, 0x01001090,
	0x01001090, 0x01001090, 0x01001090, 0x01001090,
	0x01001090, 0x01001090, 0x01001090, 0x01008090,
	0x01008090, 0x01008090, 0x01008090, 0x01008090,
	0x01008090, 0x01008090, 0x01008090, 0x01008090,
	0x01008090, 0x01008090, 0x01008090, 0x01008090,
	0x01008090, 0x08001090, 0x08001090, 0x08001090,
	0x08001090, 0x08001090, 0x08001090, 0x08001090,
	0x08001090, 0x08001090, 0x08001090,
};

static const uint32_t lock_table[] = {
	0x060603e8, 0x060603e8, 0x080803e8, 0x0b0b03e8,
	0x0e0e03e8, 0x111103e8, 0x131303e8, 0x161603e8,
	0x191903e8, 0x1c1c03e8, 0x1f1f0384, 0x1f1f0339,
	0x1f1f02ee, 0x1f1f02bc, 0x1f1f028a, 0x1f1f0271,
	0x1f1f023f, 0x1f1f0226, 0x1f1f020d, 0x1f1f01f4,
	0x1f1f01db, 0x1f1f01c2, 0x1f1f01a9, 0x1f1f0190,
	0x1f1f0190, 0x1f1f0177, 0x1f1f015e, 0x1f1f015e,
	0x1f1f0145, 0x1f1f0145, 0x1f1f012c, 0x1f1f012c,
	0x1f1f012c, 0x1f1f0113, 0x1f1f0113, 0x1f1f0113,
};

struct axi_clkgen_config {
	DEVICE_MMIO_ROM;
	uint32_t parent_rate;
};

struct axi_clkgen_data {
	DEVICE_MMIO_RAM;
	uint32_t rate;
};

static inline uint32_t reg_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void reg_write(const struct device *dev, uint32_t reg,
			     uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

static bool mmcm_drp_idle(const struct device *dev, uint32_t *status)
{
	*status = reg_read(dev, AXI_CLKGEN_REG_DRP_STATUS);

	return (*status & AXI_CLKGEN_DRP_STATUS_BUSY) == 0;
}

/* Wait for the DRP port to go idle, returning the final status word. */
static int mmcm_drp_wait(const struct device *dev, uint32_t *status)
{
	uint32_t val = 0;

	if (!WAIT_FOR(mmcm_drp_idle(dev, &val), MMCM_DRP_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	if (status != NULL) {
		*status = val;
	}

	return 0;
}

static int mmcm_read(const struct device *dev, uint32_t reg, uint32_t *val)
{
	int ret;

	ret = mmcm_drp_wait(dev, NULL);
	if (ret) {
		return ret;
	}

	reg_write(dev, AXI_CLKGEN_REG_DRP_CNTRL,
		  AXI_CLKGEN_DRP_CNTRL_SEL | AXI_CLKGEN_DRP_CNTRL_READ |
			  FIELD_PREP(AXI_CLKGEN_DRP_CNTRL_ADDR, reg));

	ret = mmcm_drp_wait(dev, val);
	if (ret) {
		return ret;
	}

	*val = FIELD_GET(AXI_CLKGEN_DRP_STATUS_DATA, *val);

	return 0;
}

static int mmcm_write(const struct device *dev, uint32_t reg,
		      uint32_t val, uint32_t mask)
{
	uint32_t reg_val;
	int ret;

	ret = mmcm_drp_wait(dev, NULL);
	if (ret) {
		return ret;
	}

	if (mask != MMCM_DATA_MASK) {
		ret = mmcm_read(dev, reg, &reg_val);
		if (ret) {
			return ret;
		}
		reg_val &= ~mask;
	} else {
		reg_val = 0;
	}

	reg_val |= AXI_CLKGEN_DRP_CNTRL_SEL |
		   FIELD_PREP(AXI_CLKGEN_DRP_CNTRL_ADDR, reg) | (val & mask);
	reg_write(dev, AXI_CLKGEN_REG_DRP_CNTRL, reg_val);

	return 0;
}

static uint32_t lookup_filter(uint32_t m)
{
	if (m < ARRAY_SIZE(filter_table)) {
		return filter_table[m];
	}
	return MMCM_FILTER_DEFAULT;
}

static uint32_t lookup_lock(uint32_t m)
{
	if (m < ARRAY_SIZE(lock_table)) {
		return lock_table[m];
	}
	return MMCM_LOCK_DEFAULT;
}

static void setup_ranges(const struct device *dev,
			 uint32_t *fpfd_min, uint32_t *fpfd_max,
			 uint32_t *fvco_min, uint32_t *fvco_max)
{
	uint32_t info = reg_read(dev, AXI_REG_FPGA_INFO);
	uint32_t tech = FIELD_GET(AXI_INFO_FPGA_TECH, info);
	uint32_t family = FIELD_GET(AXI_INFO_FPGA_FAMILY, info);
	uint32_t speed = FIELD_GET(AXI_INFO_FPGA_SPEED, info);

	uint32_t voltage_reg = reg_read(dev, AXI_REG_FPGA_VOLTAGE);
	uint32_t voltage = FIELD_GET(AXI_INFO_FPGA_VOLTAGE, voltage_reg);

	switch (speed) {
	case FPGA_SPEED_1 ... FPGA_SPEED_1LV:
		*fvco_max = MMCM_FVCO_MAX_SPEED_1_KHZ;
		*fpfd_max = MMCM_FPFD_MAX_SPEED_1_KHZ;
		break;
	case FPGA_SPEED_2 ... FPGA_SPEED_2LV:
		*fvco_max = MMCM_FVCO_MAX_SPEED_2_KHZ;
		*fpfd_max = MMCM_FPFD_MAX_SPEED_2_KHZ;
		/*
		 * Kintex/Artix -2 parts are only rated for the -2 maxima at the
		 * nominal 0.95 V core supply; below that they derate to the -1
		 * numbers (7 series datasheets DS181/DS182).
		 */
		if ((family == FPGA_FAMILY_KINTEX) ||
		    (family == FPGA_FAMILY_ARTIX)) {
			if (voltage < FPGA_VOLTAGE_DERATE_MV) {
				*fvco_max = MMCM_FVCO_MAX_SPEED_1_KHZ;
				*fpfd_max = MMCM_FPFD_MAX_SPEED_1_KHZ;
			}
		}
		break;
	case FPGA_SPEED_3:
		*fvco_max = MMCM_FVCO_MAX_SPEED_3_KHZ;
		*fpfd_max = MMCM_FPFD_MAX_SPEED_3_KHZ;
		break;
	default:
		break;
	}

	if (tech == FPGA_TECH_ULTRASCALE_PLUS) {
		*fvco_max = MMCM_FVCO_MAX_USP_KHZ;
		*fvco_min = MMCM_FVCO_MIN_USP_KHZ;
	}
}

static void calc_params(const struct device *dev,
			uint32_t fin, uint32_t fout,
			uint32_t *best_d, uint32_t *best_m,
			uint32_t *best_dout)
{
	uint32_t fpfd_min = MMCM_FPFD_MIN_KHZ;
	uint32_t fpfd_max = MMCM_FPFD_MAX_DEFAULT_KHZ;
	uint32_t fvco_min = MMCM_FVCO_MIN_DEFAULT_KHZ;
	uint32_t fvco_max = MMCM_FVCO_MAX_DEFAULT_KHZ;
	int32_t best_f = INT32_MAX;
	uint32_t version;

	/*
	 * Only v5 and later of the core expose the FPGA_INFO/FPGA_VOLTAGE
	 * registers needed to pick per-speed-grade limits; on older cores the
	 * conservative defaults above are all we can safely assume.
	 */
	version = reg_read(dev, AXI_REG_VERSION);
	if (FIELD_GET(AXI_VERSION_MAJOR, version) > AXI_CLKGEN_VERSION_FPGA_INFO) {
		setup_ranges(dev, &fpfd_min, &fpfd_max, &fvco_min, &fvco_max);
	}

	fin /= 1000;
	fout /= 1000;

	*best_d = 0;
	*best_m = 0;
	*best_dout = 0;

	uint32_t d_min = MAX(DIV_ROUND_UP(fin, fpfd_max), 1);
	uint32_t d_max = MIN(fin / fpfd_min, MMCM_DIVCLK_DIVIDE_MAX);
	uint32_t m_min = MAX(DIV_ROUND_UP(fvco_min, fin) * d_min, 1);
	uint32_t m_max = MIN(fvco_max * d_max / fin, MMCM_CLKFBOUT_MULT_MAX);

	for (uint32_t m = m_min; m <= m_max; m++) {
		uint32_t _d_min = MAX(d_min, DIV_ROUND_UP(fin * m, fvco_max));
		uint32_t _d_max = MIN(d_max, fin * m / fvco_min);

		for (uint32_t d = _d_min; d <= _d_max; d++) {
			uint32_t fvco = fin * m / d;
			uint32_t dout = DIV_ROUND_CLOSEST(fvco, fout);

			dout = CLAMP(dout, 1, MMCM_CLKOUT_DIVIDE_MAX);
			int32_t f = fvco / dout;

			if (abs(f - (int32_t)fout) < abs(best_f - (int32_t)fout)) {
				best_f = f;
				*best_d = d;
				*best_m = m;
				*best_dout = dout;
				if (best_f == (int32_t)fout) {
					return;
				}
			}
		}
	}
}

static void calc_clk_params(uint32_t divider, uint32_t *low, uint32_t *high,
			    uint32_t *edge, uint32_t *nocount)
{
	if (divider == 1) {
		*nocount = 1;
	} else {
		*nocount = 0;
	}
	*high = divider / 2;
	*edge = divider % 2;
	*low = divider - *high;
}

static void mmcm_enable(const struct device *dev, bool enable)
{
	uint32_t val = AXI_CLKGEN_RESETN;

	if (enable) {
		val |= AXI_CLKGEN_MMCM_RESETN;
	}

	reg_write(dev, AXI_CLKGEN_REG_RESETN, val);
}

static int axi_clkgen_do_set_rate(const struct device *dev, uint32_t rate)
{
	const struct axi_clkgen_config *cfg = dev->config;
	struct axi_clkgen_data *data = dev->data;
	uint32_t d, m, dout;
	uint32_t nocount, high, edge, low;
	uint32_t filter, lock;

	if (cfg->parent_rate == 0 || rate == 0) {
		return -EINVAL;
	}

	/* calc_params divides by (rate / 1000); any rate below 1 kHz truncates
	 * to zero and causes a data abort. MMCM minimum output is ~4.7 MHz.
	 */
	if (rate < 1000) {
		return -EINVAL;
	}

	calc_params(dev, cfg->parent_rate, rate, &d, &m, &dout);

	if (d == 0 || m == 0 || dout == 0) {
		LOG_ERR("no valid MMCM parameters for %u Hz", rate);
		return -EINVAL;
	}

	filter = lookup_filter(m - 1);
	lock = lookup_lock(m - 1);

	mmcm_enable(dev, false);

	/* CLKOUT0 is the clock handed to the rest of the design. */
	calc_clk_params(dout, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLKOUT0_1,
		   FIELD_PREP(MMCM_CLK_REG1_HIGH_TIME, high) |
			   FIELD_PREP(MMCM_CLK_REG1_LOW_TIME, low),
		   MMCM_CLK_REG1_WMASK);
	mmcm_write(dev, MMCM_REG_CLKOUT0_2,
		   FIELD_PREP(MMCM_CLK_REG2_EDGE, edge) |
			   FIELD_PREP(MMCM_CLK_REG2_NOCOUNT, nocount),
		   MMCM_CLK_REG2_WMASK);

	/*
	 * CLKOUT1 is the AXI core's internal divide-by-4 of CLKOUT0, used for
	 * the DRP/status logic, so its counter must track CLKOUT0.
	 */
	calc_clk_params(dout * MMCM_CLKOUT1_RATIO, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLKOUT1_1,
		   FIELD_PREP(MMCM_CLK_REG1_HIGH_TIME, high) |
			   FIELD_PREP(MMCM_CLK_REG1_LOW_TIME, low),
		   MMCM_CLK_REG1_WMASK);
	mmcm_write(dev, MMCM_REG_CLKOUT1_2,
		   FIELD_PREP(MMCM_CLK_REG2_EDGE, edge) |
			   FIELD_PREP(MMCM_CLK_REG2_NOCOUNT, nocount),
		   MMCM_CLK_REG2_WMASK);

	/* Input divider D (DIVCLK_DIVIDE). */
	calc_clk_params(d, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLK_DIV,
		   FIELD_PREP(MMCM_DIV_REG_EDGE, edge) |
			   FIELD_PREP(MMCM_DIV_REG_NOCOUNT, nocount) |
			   FIELD_PREP(MMCM_DIV_REG_HIGH_TIME, high) |
			   FIELD_PREP(MMCM_DIV_REG_LOW_TIME, low),
		   MMCM_DIV_REG_WMASK);

	/* Feedback multiplier M (CLKFBOUT_MULT). */
	calc_clk_params(m, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLK_FB1,
		   FIELD_PREP(MMCM_CLK_REG1_HIGH_TIME, high) |
			   FIELD_PREP(MMCM_CLK_REG1_LOW_TIME, low),
		   MMCM_CLK_REG1_WMASK);
	mmcm_write(dev, MMCM_REG_CLK_FB2,
		   FIELD_PREP(MMCM_CLK_REG2_EDGE, edge) |
			   FIELD_PREP(MMCM_CLK_REG2_NOCOUNT, nocount),
		   MMCM_CLK_REG2_WMASK);

	mmcm_write(dev, MMCM_REG_LOCK1,
		   FIELD_PREP(MMCM_LOCK_REG_CNT,
			      FIELD_GET(MMCM_LOCK_TBL_CNT, lock)),
		   MMCM_LOCK_REG1_WMASK);
	mmcm_write(dev, MMCM_REG_LOCK2,
		   FIELD_PREP(MMCM_LOCK_REG_DLY,
			      FIELD_GET(MMCM_LOCK_TBL_SAT_HIGH, lock)) |
			   FIELD_PREP(MMCM_LOCK_REG_CNT, MMCM_LOCK_REG2_CNT),
		   MMCM_LOCK_REG23_WMASK);
	mmcm_write(dev, MMCM_REG_LOCK3,
		   FIELD_PREP(MMCM_LOCK_REG_DLY,
			      FIELD_GET(MMCM_LOCK_TBL_REF_DLY, lock)) |
			   FIELD_PREP(MMCM_LOCK_REG_CNT, MMCM_LOCK_REG3_CNT),
		   MMCM_LOCK_REG23_WMASK);
	mmcm_write(dev, MMCM_REG_FILTER1, FIELD_GET(MMCM_FILTER_HIGH, filter),
		   MMCM_FILTER_REG_WMASK);
	mmcm_write(dev, MMCM_REG_FILTER2, filter, MMCM_FILTER_REG_WMASK);

	mmcm_enable(dev, true);

	/*
	 * The MMCM re-acquires lock after reset release; nothing downstream may
	 * use the output until it does, so poll rather than assume a fixed
	 * settling delay.
	 */
	if (!WAIT_FOR(reg_read(dev, AXI_CLKGEN_REG_STATUS) &
		      AXI_CLKGEN_STATUS_LOCKED,
		      MMCM_LOCK_TIMEOUT_US, k_busy_wait(10))) {
		LOG_ERR("MMCM not locked at %u Hz", rate);
		return -EIO;
	}

	data->rate = rate;
	LOG_INF("MMCM locked at %u Hz", rate);

	return 0;
}

static int axi_clkgen_set_rate(const struct device *dev,
			       clock_control_subsys_t sys,
			       clock_control_subsys_rate_t rate)
{
	ARG_UNUSED(sys);

	return axi_clkgen_do_set_rate(dev, (uint32_t)(uintptr_t)rate);
}

static int axi_clkgen_get_rate(const struct device *dev,
			       clock_control_subsys_t sys, uint32_t *rate)
{
	const struct axi_clkgen_config *cfg = dev->config;
	uint32_t d, m, dout;
	uint32_t reg;
	uint64_t tmp;

	ARG_UNUSED(sys);

	if (rate == NULL) {
		return -EINVAL;
	}

	if (mmcm_read(dev, MMCM_REG_CLKOUT0_1, &reg)) {
		return -ETIMEDOUT;
	}
	dout = FIELD_GET(MMCM_CLK_REG1_LOW_TIME, reg) +
		    FIELD_GET(MMCM_CLK_REG1_HIGH_TIME, reg);

	if (mmcm_read(dev, MMCM_REG_CLK_DIV, &reg)) {
		return -ETIMEDOUT;
	}
	d = FIELD_GET(MMCM_CLK_REG1_LOW_TIME, reg) +
		    FIELD_GET(MMCM_CLK_REG1_HIGH_TIME, reg);

	if (mmcm_read(dev, MMCM_REG_CLK_FB1, &reg)) {
		return -ETIMEDOUT;
	}
	m = FIELD_GET(MMCM_CLK_REG1_LOW_TIME, reg) +
		    FIELD_GET(MMCM_CLK_REG1_HIGH_TIME, reg);

	if (d == 0 || dout == 0) {
		*rate = 0;
		return 0;
	}

	tmp = (uint64_t)(cfg->parent_rate / d) * m;
	tmp = tmp / dout;

	*rate = (tmp > UINT32_MAX) ? UINT32_MAX : (uint32_t)tmp;

	return 0;
}

static int axi_clkgen_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	mmcm_enable(dev, true);

	return 0;
}

static int axi_clkgen_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	mmcm_enable(dev, false);

	return 0;
}

static enum clock_control_status axi_clkgen_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	uint32_t status;

	ARG_UNUSED(sys);

	status = reg_read(dev, AXI_CLKGEN_REG_STATUS);
	if (status & AXI_CLKGEN_STATUS_LOCKED) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, axi_clkgen_api) = {
	.on = axi_clkgen_on,
	.off = axi_clkgen_off,
	.get_rate = axi_clkgen_get_rate,
	.get_status = axi_clkgen_get_status,
	.set_rate = axi_clkgen_set_rate,
};

static int axi_clkgen_init(const struct device *dev)
{
	const struct axi_clkgen_config *cfg = dev->config;
	uint32_t version;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	version = reg_read(dev, AXI_REG_VERSION);

	LOG_INF("AXI CLKGEN v%u.%u.%c, parent clock %u Hz",
		(unsigned int)FIELD_GET(AXI_VERSION_MAJOR, version),
		(unsigned int)FIELD_GET(AXI_VERSION_MINOR, version),
		(char)FIELD_GET(AXI_VERSION_PATCH, version),
		cfg->parent_rate);

	return 0;
}

#define AXI_CLKGEN_INIT(n)						\
	static struct axi_clkgen_data axi_clkgen_data_##n;		\
	static const struct axi_clkgen_config axi_clkgen_config_##n = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),			\
		.parent_rate = DT_INST_PROP(n, clock_frequency),		\
	};								\
	DEVICE_DT_INST_DEFINE(n,					\
			      axi_clkgen_init,				\
			      NULL,					\
			      &axi_clkgen_data_##n,			\
			      &axi_clkgen_config_##n,			\
			      POST_KERNEL,				\
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,	\
			      &axi_clkgen_api);

DT_INST_FOREACH_STATUS_OKAY(AXI_CLKGEN_INIT)
