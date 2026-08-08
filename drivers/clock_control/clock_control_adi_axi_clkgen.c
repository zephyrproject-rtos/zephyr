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
#define AXI_REG_FPGA_INFO		0x001c
#define AXI_REG_FPGA_VOLTAGE		0x0140

#define AXI_CLKGEN_REG_RESETN		0x40
#define AXI_CLKGEN_MMCM_RESETN		BIT(1)
#define AXI_CLKGEN_RESETN		BIT(0)

#define AXI_CLKGEN_REG_STATUS		0x5c
#define AXI_CLKGEN_STATUS_LOCKED	BIT(0)

#define AXI_CLKGEN_REG_DRP_CNTRL	0x70
#define AXI_CLKGEN_DRP_CNTRL_SEL	BIT(29)
#define AXI_CLKGEN_DRP_CNTRL_READ	BIT(28)

#define AXI_CLKGEN_REG_DRP_STATUS	0x74
#define AXI_CLKGEN_DRP_STATUS_BUSY	BIT(16)

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

#define AXI_INFO_FPGA_TECH(info)	((info) >> 24)
#define AXI_INFO_FPGA_FAMILY(info)	(((info) >> 16) & 0xff)
#define AXI_INFO_FPGA_SPEED(info)	(((info) >> 8) & 0xff)
#define AXI_INFO_FPGA_VOLTAGE(val)	((val) & 0xffff)

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

static int mmcm_read(const struct device *dev, uint32_t reg, uint32_t *val)
{
	uint32_t timeout = 1000000;
	uint32_t status;

	do {
		status = reg_read(dev, AXI_CLKGEN_REG_DRP_STATUS);
	} while ((status & AXI_CLKGEN_DRP_STATUS_BUSY) && --timeout);

	if (timeout == 0) {
		return -ETIMEDOUT;
	}

	reg_write(dev, AXI_CLKGEN_REG_DRP_CNTRL,
		  AXI_CLKGEN_DRP_CNTRL_SEL | AXI_CLKGEN_DRP_CNTRL_READ |
		  (reg << 16));

	timeout = 1000000;
	do {
		*val = reg_read(dev, AXI_CLKGEN_REG_DRP_STATUS);
	} while ((*val & AXI_CLKGEN_DRP_STATUS_BUSY) && --timeout);

	if (timeout == 0) {
		return -ETIMEDOUT;
	}

	*val &= 0xffff;
	return 0;
}

static int mmcm_write(const struct device *dev, uint32_t reg,
		      uint32_t val, uint32_t mask)
{
	uint32_t timeout = 1000000;
	uint32_t status;
	uint32_t reg_val;

	do {
		status = reg_read(dev, AXI_CLKGEN_REG_DRP_STATUS);
	} while ((status & AXI_CLKGEN_DRP_STATUS_BUSY) && --timeout);

	if (timeout == 0) {
		return -ETIMEDOUT;
	}

	if (mask != 0xffff) {
		int ret = mmcm_read(dev, reg, &reg_val);

		if (ret) {
			return ret;
		}
		reg_val &= ~mask;
	} else {
		reg_val = 0;
	}

	reg_val |= AXI_CLKGEN_DRP_CNTRL_SEL | (reg << 16) | (val & mask);
	reg_write(dev, AXI_CLKGEN_REG_DRP_CNTRL, reg_val);

	return 0;
}

static uint32_t lookup_filter(uint32_t m)
{
	if (m < ARRAY_SIZE(filter_table)) {
		return filter_table[m];
	}
	return 0x08008090;
}

static uint32_t lookup_lock(uint32_t m)
{
	if (m < ARRAY_SIZE(lock_table)) {
		return lock_table[m];
	}
	return 0x1f1f00fa;
}

static void setup_ranges(const struct device *dev,
			 uint32_t *fpfd_min, uint32_t *fpfd_max,
			 uint32_t *fvco_min, uint32_t *fvco_max)
{
	uint32_t info = reg_read(dev, AXI_REG_FPGA_INFO);
	uint32_t tech = AXI_INFO_FPGA_TECH(info);
	uint32_t family = AXI_INFO_FPGA_FAMILY(info);
	uint32_t speed = AXI_INFO_FPGA_SPEED(info);

	uint32_t voltage_reg = reg_read(dev, AXI_REG_FPGA_VOLTAGE);
	uint32_t voltage = AXI_INFO_FPGA_VOLTAGE(voltage_reg);

	switch (speed) {
	case FPGA_SPEED_1 ... FPGA_SPEED_1LV:
		*fvco_max = 1200000;
		*fpfd_max = 450000;
		break;
	case FPGA_SPEED_2 ... FPGA_SPEED_2LV:
		*fvco_max = 1440000;
		*fpfd_max = 500000;
		if ((family == FPGA_FAMILY_KINTEX) ||
		    (family == FPGA_FAMILY_ARTIX)) {
			if (voltage < 950) {
				*fvco_max = 1200000;
				*fpfd_max = 450000;
			}
		}
		break;
	case FPGA_SPEED_3:
		*fvco_max = 1600000;
		*fpfd_max = 550000;
		break;
	default:
		break;
	}

	if (tech == FPGA_TECH_ULTRASCALE_PLUS) {
		*fvco_max = 1600000;
		*fvco_min = 800000;
	}
}

static void calc_params(const struct device *dev,
			uint32_t fin, uint32_t fout,
			uint32_t *best_d, uint32_t *best_m,
			uint32_t *best_dout)
{
	uint32_t fpfd_min = 10000;
	uint32_t fpfd_max = 300000;
	uint32_t fvco_min = 600000;
	uint32_t fvco_max = 1200000;
	int32_t best_f = INT32_MAX;
	uint32_t version;

	version = reg_read(dev, AXI_REG_VERSION);
	if ((version >> 16) > 4) {
		setup_ranges(dev, &fpfd_min, &fpfd_max, &fvco_min, &fvco_max);
	}

	fin /= 1000;
	fout /= 1000;

	*best_d = 0;
	*best_m = 0;
	*best_dout = 0;

	uint32_t d_min = MAX(DIV_ROUND_UP(fin, fpfd_max), 1);
	uint32_t d_max = MIN(fin / fpfd_min, 80);
	uint32_t m_min = MAX(DIV_ROUND_UP(fvco_min, fin) * d_min, 1);
	uint32_t m_max = MIN(fvco_max * d_max / fin, 64);

	for (uint32_t m = m_min; m <= m_max; m++) {
		uint32_t _d_min = MAX(d_min, DIV_ROUND_UP(fin * m, fvco_max));
		uint32_t _d_max = MIN(d_max, fin * m / fvco_min);

		for (uint32_t d = _d_min; d <= _d_max; d++) {
			uint32_t fvco = fin * m / d;
			uint32_t dout = DIV_ROUND_CLOSEST(fvco, fout);

			dout = CLAMP(dout, 1, 128);
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
	uint32_t status;

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

	calc_clk_params(dout, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLKOUT0_1, (high << 6) | low, 0xefff);
	mmcm_write(dev, MMCM_REG_CLKOUT0_2, (edge << 7) | (nocount << 6),
		   0x03ff);

	uint32_t dout1 = dout * 4;

	calc_clk_params(dout1, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLKOUT1_1, (high << 6) | low, 0xefff);
	mmcm_write(dev, MMCM_REG_CLKOUT1_2, (edge << 7) | (nocount << 6),
		   0x03ff);

	calc_clk_params(d, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLK_DIV,
		   (edge << 13) | (nocount << 12) | (high << 6) | low, 0x3fff);

	calc_clk_params(m, &low, &high, &edge, &nocount);
	mmcm_write(dev, MMCM_REG_CLK_FB1, (high << 6) | low, 0xefff);
	mmcm_write(dev, MMCM_REG_CLK_FB2, (edge << 7) | (nocount << 6),
		   0x03ff);

	mmcm_write(dev, MMCM_REG_LOCK1, lock & 0x3ff, 0x3ff);
	mmcm_write(dev, MMCM_REG_LOCK2,
		   (((lock >> 16) & 0x1f) << 10) | 0x1, 0x7fff);
	mmcm_write(dev, MMCM_REG_LOCK3,
		   (((lock >> 24) & 0x1f) << 10) | 0x3e9, 0x7fff);
	mmcm_write(dev, MMCM_REG_FILTER1, filter >> 16, 0x9900);
	mmcm_write(dev, MMCM_REG_FILTER2, filter, 0x9900);

	mmcm_enable(dev, true);

	k_msleep(10);

	status = reg_read(dev, AXI_CLKGEN_REG_STATUS);
	if (!(status & AXI_CLKGEN_STATUS_LOCKED)) {
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

	if (mmcm_read(dev, MMCM_REG_CLKOUT0_1, &reg)) {
		return -ETIMEDOUT;
	}
	dout = (reg & 0x3f) + ((reg >> 6) & 0x3f);

	if (mmcm_read(dev, MMCM_REG_CLK_DIV, &reg)) {
		return -ETIMEDOUT;
	}
	d = (reg & 0x3f) + ((reg >> 6) & 0x3f);

	if (mmcm_read(dev, MMCM_REG_CLK_FB1, &reg)) {
		return -ETIMEDOUT;
	}
	m = (reg & 0x3f) + ((reg >> 6) & 0x3f);

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

	LOG_INF("AXI CLKGEN v%d.%d.%c, parent clock %u Hz",
		version >> 16, (version >> 8) & 0xff, version & 0xff,
		cfg->parent_rate);

	return 0;
}

#define AXI_CLKGEN_INIT(n)						\
	static struct axi_clkgen_data axi_clkgen_data_##n;		\
	static const struct axi_clkgen_config axi_clkgen_config_##n = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),			\
		.parent_rate = DT_INST_PROP(n, adi_parent_clock_hz),	\
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
