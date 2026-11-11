/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_i2c

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#else
#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#endif

#include "i2c-priv.h"
#include "i2c_bitbang.h"

LOG_MODULE_REGISTER(i2c_infineon_scb, CONFIG_I2C_LOG_LEVEL);

#define IFX_SCB_CTRL             0x000U
#define IFX_SCB_I2C_CTRL         0x060U
#define IFX_SCB_I2C_STATUS       0x064U
#define IFX_SCB_I2C_M_CMD        0x068U
#define IFX_SCB_I2C_S_CMD        0x06CU
#define IFX_SCB_I2C_CFG          0x070U
#define IFX_SCB_TX_CTRL          0x200U
#define IFX_SCB_TX_FIFO_CTRL     0x204U
#define IFX_SCB_TX_FIFO_STATUS   0x208U
#define IFX_SCB_TX_FIFO_WR       0x240U
#define IFX_SCB_RX_CTRL          0x300U
#define IFX_SCB_RX_FIFO_CTRL     0x304U
#define IFX_SCB_RX_MATCH         0x310U
#define IFX_SCB_RX_FIFO_RD       0x340U
#define IFX_SCB_INTR_CAUSE       0xE00U
#define IFX_SCB_INTR_I2C_EC      0xE80U
#define IFX_SCB_INTR_I2C_EC_MASK 0xE88U
#define IFX_SCB_INTR_M           0xF00U
#define IFX_SCB_INTR_M_MASK      0xF08U
#define IFX_SCB_INTR_M_MASKED    0xF0CU
#define IFX_SCB_INTR_S           0xF40U
#define IFX_SCB_INTR_S_MASK      0xF48U
#define IFX_SCB_INTR_S_MASKED    0xF4CU
#define IFX_SCB_INTR_TX          0xF80U
#define IFX_SCB_INTR_TX_MASK     0xF88U
#define IFX_SCB_INTR_RX          0xFC0U
#define IFX_SCB_INTR_RX_MASK     0xFC8U

#define IFX_SCB_CTRL_EC_AM_MODE BIT(8)
#define IFX_SCB_CTRL_MEM_WIDTH  GENMASK(15, 14)
#define IFX_SCB_CTRL_MODE       GENMASK(25, 24)
#define IFX_SCB_CTRL_ENABLED    BIT(31)
#define IFX_SCB_CTRL_I2C_BYTE                                                                      \
	(FIELD_PREP(IFX_SCB_CTRL_MODE, 0U) | FIELD_PREP(IFX_SCB_CTRL_MEM_WIDTH, 0U))

#define IFX_SCB_I2C_CTRL_HIGH_PHASE       GENMASK(3, 0)
#define IFX_SCB_I2C_CTRL_LOW_PHASE        GENMASK(7, 4)
#define IFX_SCB_I2C_CTRL_S_GENERAL_IGNORE BIT(11)
#define IFX_SCB_I2C_CTRL_SLAVE_MODE       BIT(30)
#define IFX_SCB_I2C_CTRL_MASTER_MODE      BIT(31)

#define IFX_SCB_I2C_STATUS_S_READ BIT(4)

#define IFX_SCB_M_CMD_START         BIT(0)
#define IFX_SCB_M_CMD_START_ON_IDLE BIT(1)
#define IFX_SCB_M_CMD_ACK           BIT(2)
#define IFX_SCB_M_CMD_NACK          BIT(3)
#define IFX_SCB_M_CMD_STOP          BIT(4)

#define IFX_SCB_S_CMD_ACK  BIT(0)
#define IFX_SCB_S_CMD_NACK BIT(1)

#define IFX_SCB_I2C_CFG_CLK_ENABLE       BIT(1)
#define IFX_SCB_I2C_CFG_SDA_IN_FILT_SEL  BIT(4)
#define IFX_SCB_I2C_CFG_SCL_IN_FILT_SEL  BIT(12)
#define IFX_SCB_I2C_CFG_SDA_OUT_FILT_SEL GENMASK(29, 28)
#define IFX_SCB_I2C_CFG_FILT_SEL                                                                   \
	(IFX_SCB_I2C_CFG_SDA_IN_FILT_SEL | IFX_SCB_I2C_CFG_SCL_IN_FILT_SEL |                       \
	 IFX_SCB_I2C_CFG_SDA_OUT_FILT_SEL)
#define IFX_SCB_I2C_CFG_ANALOG_IN                                                                  \
	(IFX_SCB_I2C_CFG_SDA_IN_FILT_SEL | IFX_SCB_I2C_CFG_SCL_IN_FILT_SEL)

#define IFX_SCB_FRAME_DATA_WIDTH GENMASK(4, 0)
#define IFX_SCB_FRAME_MSB_FIRST  BIT(8)
#define IFX_SCB_FRAME_8BIT     (FIELD_PREP(IFX_SCB_FRAME_DATA_WIDTH, 7U) | IFX_SCB_FRAME_MSB_FIRST)
#define IFX_SCB_FRAME_MASK     (IFX_SCB_FRAME_DATA_WIDTH | IFX_SCB_FRAME_MSB_FIRST)
#define IFX_SCB_RX_CTRL_MEDIAN BIT(9)

#define IFX_SCB_FIFO_CTRL_TRIGGER_LEVEL GENMASK(7, 0)
#define IFX_SCB_FIFO_CTRL_CLEAR         BIT(16)

#define IFX_SCB_TX_FIFO_STATUS_USED     GENMASK(8, 0)
#define IFX_SCB_TX_FIFO_STATUS_SR_VALID BIT(15)

#define IFX_SCB_RX_MATCH_ADDR GENMASK(7, 0)
#define IFX_SCB_RX_MATCH_MASK GENMASK(23, 16)
#define IFX_SCB_RX_MATCH_TARGET(addr)                                                              \
	(FIELD_PREP(IFX_SCB_RX_MATCH_ADDR, (uint32_t)(addr) << 1) |                                \
	 FIELD_PREP(IFX_SCB_RX_MATCH_MASK, IFX_SCB_I2C_ADDR_MASK))

#define IFX_SCB_INTR_CAUSE_M      BIT(0)
#define IFX_SCB_INTR_CAUSE_S      BIT(1)
#define IFX_SCB_INTR_CAUSE_TX     BIT(2)
#define IFX_SCB_INTR_CAUSE_RX     BIT(3)
#define IFX_SCB_INTR_CAUSE_I2C_EC BIT(4)

#define IFX_SCB_INTR_I2C_EC_WAKE_UP BIT(0)

#define IFX_SCB_INTR_M_ARB_LOST  BIT(0)
#define IFX_SCB_INTR_M_NACK      BIT(1)
#define IFX_SCB_INTR_M_ACK       BIT(2)
#define IFX_SCB_INTR_M_STOP      BIT(4)
#define IFX_SCB_INTR_M_BUS_ERROR BIT(8)
#define IFX_SCB_INTR_M_EVENTS                                                                      \
	(IFX_SCB_INTR_M_ARB_LOST | IFX_SCB_INTR_M_NACK | IFX_SCB_INTR_M_STOP |                     \
	 IFX_SCB_INTR_M_BUS_ERROR)

#define IFX_SCB_INTR_S_ARB_LOST   BIT(0)
#define IFX_SCB_INTR_S_NACK       BIT(1)
#define IFX_SCB_INTR_S_STOP       BIT(4)
#define IFX_SCB_INTR_S_ADDR_MATCH BIT(6)
#define IFX_SCB_INTR_S_BUS_ERROR  BIT(8)
#define IFX_SCB_INTR_S_ERRORS     (IFX_SCB_INTR_S_ARB_LOST | IFX_SCB_INTR_S_BUS_ERROR)
#define IFX_SCB_INTR_S_EVENTS                                                                      \
	(IFX_SCB_INTR_S_ERRORS | IFX_SCB_INTR_S_NACK | IFX_SCB_INTR_S_STOP |                       \
	 IFX_SCB_INTR_S_ADDR_MATCH)

#define IFX_SCB_INTR_TX_TRIGGER   BIT(0)
#define IFX_SCB_INTR_TX_EMPTY     BIT(4)
#define IFX_SCB_INTR_TX_UNDERFLOW BIT(6)

#define IFX_SCB_INTR_RX_NOT_EMPTY BIT(2)

#define IFX_SCB_I2C_PHASE_MAX      16U
#define IFX_SCB_I2C_LOW_PHASE_MIN  7U
#define IFX_SCB_I2C_HIGH_PHASE_MIN 5U
#define IFX_SCB_I2C_ADDR_MAX       0x7FU
#define IFX_SCB_I2C_ADDR_MASK      0xFEU
#define IFX_SCB_I2C_ADDR_READ      BIT(0)
#define IFX_SCB_I2C_FILL_BYTE      0xFFU

#define IFX_SCB_I2C_MEDIAN_KHZ (I2C_BITRATE_FAST / 1000U)

#define IFX_SCB_I2C_SM_CLK_MIN_KHZ  1550U
#define IFX_SCB_I2C_SM_CLK_MAX_KHZ  3200U
#define IFX_SCB_I2C_SM_LOW_NS       (4700U + 300U)
#define IFX_SCB_I2C_SM_HIGH_NS      (4000U + 1000U)
#define IFX_SCB_I2C_FM_CLK_MIN_KHZ  7820U
#define IFX_SCB_I2C_FM_CLK_MAX_KHZ  10000U
#define IFX_SCB_I2C_FM_LOW_NS       (1300U + 300U)
#define IFX_SCB_I2C_FM_HIGH_NS      (600U + 300U)
#define IFX_SCB_I2C_FMP_CLK_MIN_KHZ 15840U
#define IFX_SCB_I2C_FMP_CLK_MAX_KHZ 25800U
#define IFX_SCB_I2C_FMP_LOW_NS      (500U + 120U)
#define IFX_SCB_I2C_FMP_HIGH_NS     (260U + 120U)

#define IFX_SCB_I2C_FRAC_DIV_STEPS 32U

enum ifx_scb_i2c_state {
	IFX_SCB_I2C_IDLE,
	IFX_SCB_I2C_CTRL_START,
	IFX_SCB_I2C_CTRL_ADDR,
	IFX_SCB_I2C_CTRL_WRITE,
	IFX_SCB_I2C_CTRL_DRAIN,
	IFX_SCB_I2C_CTRL_READ,
	IFX_SCB_I2C_CTRL_STOP,
	IFX_SCB_I2C_TGT_WRITE,
	IFX_SCB_I2C_TGT_READ,
	IFX_SCB_I2C_TGT_NACK,
};

struct ifx_scb_i2c_timing {
	uint16_t bitrate_khz;
	uint16_t clk_min_khz;
	uint16_t clk_max_khz;
	uint16_t low_ns;
	uint16_t high_ns;
};

struct ifx_scb_i2c_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
	const struct device *clk_dev;
	struct ifx_clk_peri clk;
#else
	struct ifx_cat1_clock clk;
	en_clk_dst_t clk_dst;
#endif
	uint8_t fifo_depth;
	uint32_t bitrate;
	k_timeout_t timeout;
	void (*irq_config)(void);
#ifdef CONFIG_I2C_INFINEON_BUS_RECOVERY
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
#endif
};

struct ifx_scb_i2c_data {
	struct k_mutex bus_lock;
	struct k_sem done;
	struct k_spinlock lock;
	struct i2c_target_config *target;
	struct i2c_msg *msgs;
	uint32_t dev_config;
	uint32_t seg_left;
	uint32_t msg_pos;
	int result;
	uint16_t addr;
	uint8_t num_msgs;
	uint8_t msg_idx;
	uint8_t seg_last;
	uint8_t state;
};

static inline bool ifx_scb_i2c_is_ctrl(uint8_t state)
{
	return (state >= IFX_SCB_I2C_CTRL_START) && (state <= IFX_SCB_I2C_CTRL_STOP);
}

static void ifx_scb_i2c_enable(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	uint32_t ctrl = IFX_SCB_CTRL_I2C_BYTE |
			(pm_device_wakeup_is_enabled(dev) ? IFX_SCB_CTRL_EC_AM_MODE : 0U);

	sys_write32(ctrl, config->base + IFX_SCB_CTRL);
	sys_write32(ctrl | IFX_SCB_CTRL_ENABLED, config->base + IFX_SCB_CTRL);
}

static void ifx_scb_i2c_fifo_clear(mem_addr_t base)
{
	uint32_t tx = sys_read32(base + IFX_SCB_TX_FIFO_CTRL);
	uint32_t rx = sys_read32(base + IFX_SCB_RX_FIFO_CTRL);

	sys_write32(tx | IFX_SCB_FIFO_CTRL_CLEAR, base + IFX_SCB_TX_FIFO_CTRL);
	sys_write32(rx | IFX_SCB_FIFO_CTRL_CLEAR, base + IFX_SCB_RX_FIFO_CTRL);
	sys_write32(tx, base + IFX_SCB_TX_FIFO_CTRL);
	sys_write32(rx, base + IFX_SCB_RX_FIFO_CTRL);
}

static void ifx_scb_i2c_mask(mem_addr_t base, uint32_t m, uint32_t tx, uint32_t rx)
{
	sys_write32(m, base + IFX_SCB_INTR_M_MASK);
	sys_write32(tx, base + IFX_SCB_INTR_TX_MASK);
	sys_write32(rx, base + IFX_SCB_INTR_RX_MASK);
}

static uint32_t ifx_scb_i2c_phases(const struct ifx_scb_i2c_timing *timing, uint32_t clk,
				   uint32_t *low, uint32_t *high)
{
	uint32_t clk_khz = DIV_ROUND_UP(clk, 1000U);
	uint32_t median = (timing->bitrate_khz > IFX_SCB_I2C_MEDIAN_KHZ) ? 1U : 0U;
	uint32_t lo = MAX(IFX_SCB_I2C_LOW_PHASE_MIN + median,
			  DIV_ROUND_UP(timing->low_ns * clk_khz, USEC_PER_SEC));
	uint32_t hi = MAX(IFX_SCB_I2C_HIGH_PHASE_MIN + median,
			  DIV_ROUND_UP(timing->high_ns * clk_khz, USEC_PER_SEC));
	uint32_t total = MAX(lo + hi, DIV_ROUND_UP(clk_khz, timing->bitrate_khz));
	uint32_t extra;

	if ((lo > IFX_SCB_I2C_PHASE_MAX) || (hi > IFX_SCB_I2C_PHASE_MAX) ||
	    (total > (2U * IFX_SCB_I2C_PHASE_MAX))) {
		return 0U;
	}

	extra = total - lo - hi;
	lo += DIV_ROUND_UP(extra, 2U);
	hi += extra / 2U;

	if (lo > IFX_SCB_I2C_PHASE_MAX) {
		hi += lo - IFX_SCB_I2C_PHASE_MAX;
		lo = IFX_SCB_I2C_PHASE_MAX;
	}

	if (hi > IFX_SCB_I2C_PHASE_MAX) {
		lo += hi - IFX_SCB_I2C_PHASE_MAX;
		hi = IFX_SCB_I2C_PHASE_MAX;
	}

	*low = lo;
	*high = hi;

	return clk / total;
}

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
static int ifx_scb_i2c_clk_init(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;

	if (!device_is_ready(config->clk_dev)) {
		return -ENODEV;
	}

	return 0;
}

static int ifx_scb_i2c_clk_set(const struct device *dev, const struct ifx_scb_i2c_timing *timing,
			       uint32_t *rate)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	int ret;

	*rate = timing->clk_min_khz * 1000U;
	ret = clock_control_set_rate(config->clk_dev, (clock_control_subsys_t)&config->clk,
				     (clock_control_subsys_rate_t)rate);
	if (ret != 0) {
		return ret;
	}

	return clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&config->clk, rate);
}
#else
static int ifx_scb_i2c_clk_init(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;

	if (ifx_cat1_utils_peri_pclk_assign_divider(config->clk_dst, &config->clk) !=
	    CY_RSLT_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static uint32_t ifx_scb_i2c_divider(const struct ifx_scb_i2c_timing *timing, uint32_t src)
{
	uint32_t last = src / (timing->clk_min_khz * 1000U);
	uint32_t best_rate = 0U;
	uint32_t best_div = 0U;
	uint32_t low;
	uint32_t high;

	for (uint32_t divider = MAX(DIV_ROUND_UP(src, timing->clk_max_khz * 1000U), 1U);
	     divider <= last; divider++) {
		uint32_t rate = ifx_scb_i2c_phases(timing, src / divider, &low, &high);

		if (rate > best_rate) {
			best_rate = rate;
			best_div = divider;
		}
	}

	return best_div;
}

static int ifx_scb_i2c_clk_src(const struct device *dev, uint32_t *rate)
{
#ifdef CONFIG_SOC_FAMILY_INFINEON_PSOC4
	ARG_UNUSED(dev);

	*rate = Cy_SysClk_ClkHfGetFrequency();
#else
	const struct ifx_scb_i2c_config *config = dev->config;
	uint32_t freq = ifx_cat1_utils_peri_pclk_get_frequency(config->clk_dst, &config->clk);
	uint32_t div_int;
	uint32_t div_frac = 0U;

	if (ifx_cat1_utils_peri_is_fract_div(&config->clk)) {
		ifx_cat1_utils_peri_pclk_get_frac_divider(config->clk_dst, &config->clk, &div_int,
							  &div_frac);
	} else {
		div_int = ifx_cat1_utils_peri_pclk_get_divider(config->clk_dst, &config->clk);
	}

	*rate = (freq * (div_int + 1U)) + ((freq / IFX_SCB_I2C_FRAC_DIV_STEPS) * div_frac);
#endif

	return (*rate == 0U) ? -EIO : 0;
}

static int ifx_scb_i2c_clk_set(const struct device *dev, const struct ifx_scb_i2c_timing *timing,
			       uint32_t *rate)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	cy_rslt_t status;
	uint32_t divider;
	int ret;

	ret = ifx_scb_i2c_clk_src(dev, rate);
	if (ret != 0) {
		return ret;
	}

	divider = ifx_scb_i2c_divider(timing, *rate);
	if (divider == 0U) {
		LOG_ERR("No SCB clock for %u kHz from %u Hz", timing->bitrate_khz, *rate);
		return -ENOTSUP;
	}

	status = ifx_cat1_utils_peri_pclk_disable_divider(config->clk_dst, &config->clk);
	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	if (ifx_cat1_utils_peri_is_fract_div(&config->clk)) {
		status = ifx_cat1_utils_peri_pclk_set_frac_divider(config->clk_dst, &config->clk,
								   divider - 1U, 0U);
	} else {
		status = ifx_cat1_utils_peri_pclk_set_divider(config->clk_dst, &config->clk,
							      divider - 1U);
	}

	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	status = ifx_cat1_utils_peri_pclk_enable_divider(config->clk_dst, &config->clk);
	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	*rate = ifx_cat1_utils_peri_pclk_get_frequency(config->clk_dst, &config->clk);

	return 0;
}
#endif

static int ifx_scb_i2c_set_speed(const struct device *dev, uint32_t speed)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_timing t;
	uint32_t rate;
	uint32_t low;
	uint32_t high;
	uint32_t reg;
	int ret;

	switch (speed) {
	case I2C_SPEED_STANDARD:
		t.bitrate_khz = I2C_BITRATE_STANDARD / 1000U;
		t.clk_min_khz = IFX_SCB_I2C_SM_CLK_MIN_KHZ;
		t.clk_max_khz = IFX_SCB_I2C_SM_CLK_MAX_KHZ;
		t.low_ns = IFX_SCB_I2C_SM_LOW_NS;
		t.high_ns = IFX_SCB_I2C_SM_HIGH_NS;
		break;
	case I2C_SPEED_FAST:
		t.bitrate_khz = I2C_BITRATE_FAST / 1000U;
		t.clk_min_khz = IFX_SCB_I2C_FM_CLK_MIN_KHZ;
		t.clk_max_khz = IFX_SCB_I2C_FM_CLK_MAX_KHZ;
		t.low_ns = IFX_SCB_I2C_FM_LOW_NS;
		t.high_ns = IFX_SCB_I2C_FM_HIGH_NS;
		break;
	case I2C_SPEED_FAST_PLUS:
		t.bitrate_khz = I2C_BITRATE_FAST_PLUS / 1000U;
		t.clk_min_khz = IFX_SCB_I2C_FMP_CLK_MIN_KHZ;
		t.clk_max_khz = IFX_SCB_I2C_FMP_CLK_MAX_KHZ;
		t.low_ns = IFX_SCB_I2C_FMP_LOW_NS;
		t.high_ns = IFX_SCB_I2C_FMP_HIGH_NS;
		break;
	default:
		return -ENOTSUP;
	}

	ret = ifx_scb_i2c_clk_set(dev, &t, &rate);
	if (ret != 0) {
		return ret;
	}

	if (ifx_scb_i2c_phases(&t, rate, &low, &high) == 0U) {
		LOG_ERR("SCB clock %u Hz unusable for %u kHz", rate, t.bitrate_khz);
		return -EIO;
	}

	reg = sys_read32(config->base + IFX_SCB_I2C_CTRL) &
	      ~(IFX_SCB_I2C_CTRL_LOW_PHASE | IFX_SCB_I2C_CTRL_HIGH_PHASE);
	sys_write32(reg | FIELD_PREP(IFX_SCB_I2C_CTRL_LOW_PHASE, low - 1U) |
			    FIELD_PREP(IFX_SCB_I2C_CTRL_HIGH_PHASE, high - 1U),
		    config->base + IFX_SCB_I2C_CTRL);

	if (t.bitrate_khz > IFX_SCB_I2C_MEDIAN_KHZ) {
		sys_set_bits(config->base + IFX_SCB_RX_CTRL, IFX_SCB_RX_CTRL_MEDIAN);
		sys_clear_bits(config->base + IFX_SCB_I2C_CFG, IFX_SCB_I2C_CFG_FILT_SEL);
	} else {
		sys_clear_bits(config->base + IFX_SCB_RX_CTRL, IFX_SCB_RX_CTRL_MEDIAN);
		reg = sys_read32(config->base + IFX_SCB_I2C_CFG) & ~IFX_SCB_I2C_CFG_FILT_SEL;
		sys_write32(reg | IFX_SCB_I2C_CFG_ANALOG_IN, config->base + IFX_SCB_I2C_CFG);
	}

	return 0;
}

static void ifx_scb_i2c_quiesce(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;

	sys_clear_bits(config->base + IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED);
	ifx_scb_i2c_mask(config->base, 0U, 0U, 0U);
	sys_write32(0U, config->base + IFX_SCB_INTR_S_MASK);
	sys_write32(0U, config->base + IFX_SCB_INTR_I2C_EC_MASK);
}

static uint32_t ifx_scb_i2c_role(const struct device *dev)
{
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t role = 0U;

	if ((data->dev_config & I2C_MODE_CONTROLLER) != 0U) {
		role |= IFX_SCB_I2C_CTRL_MASTER_MODE;
	}

	if (data->target != NULL) {
		role |= IFX_SCB_I2C_CTRL_SLAVE_MODE;
	}

	return role;
}

static int ifx_scb_i2c_hw_init(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t reg;
	int ret;

	ifx_scb_i2c_quiesce(dev);

	reg = sys_read32(config->base + IFX_SCB_TX_CTRL) & ~IFX_SCB_FRAME_MASK;
	sys_write32(reg | IFX_SCB_FRAME_8BIT, config->base + IFX_SCB_TX_CTRL);
	sys_write32(IFX_SCB_FRAME_8BIT, config->base + IFX_SCB_RX_CTRL);
	sys_write32(FIELD_PREP(IFX_SCB_FIFO_CTRL_TRIGGER_LEVEL, config->fifo_depth / 2U),
		    config->base + IFX_SCB_TX_FIFO_CTRL);
	sys_write32(0U, config->base + IFX_SCB_RX_FIFO_CTRL);

	sys_write32((data->target != NULL) ? IFX_SCB_RX_MATCH_TARGET(data->target->address) : 0U,
		    config->base + IFX_SCB_RX_MATCH);

	sys_write32(IFX_SCB_I2C_CTRL_S_GENERAL_IGNORE | ifx_scb_i2c_role(dev),
		    config->base + IFX_SCB_I2C_CTRL);

	ret = ifx_scb_i2c_set_speed(dev, I2C_SPEED_GET(data->dev_config));
	if (ret != 0) {
		return ret;
	}

	if (data->target != NULL) {
		sys_write32(IFX_SCB_INTR_S_EVENTS, config->base + IFX_SCB_INTR_S_MASK);
	}

	data->state = IFX_SCB_I2C_IDLE;
	ifx_scb_i2c_enable(dev);

	return 0;
}

static bool ifx_scb_i2c_joined(const struct i2c_msg *msgs, uint8_t idx)
{
	return ((msgs[idx - 1U].flags & I2C_MSG_STOP) == 0U) &&
	       ((msgs[idx].flags & (I2C_MSG_READ | I2C_MSG_RESTART)) ==
		(msgs[idx - 1U].flags & I2C_MSG_READ));
}

static bool ifx_scb_i2c_seg_stop(const struct ifx_scb_i2c_data *data)
{
	return ((data->msgs[data->seg_last].flags & I2C_MSG_STOP) != 0U) ||
	       ((data->seg_last + 1U) == data->num_msgs);
}

static uint8_t *ifx_scb_i2c_cursor(struct ifx_scb_i2c_data *data)
{
	while (data->msg_pos == data->msgs[data->msg_idx].len) {
		data->msg_idx++;
		data->msg_pos = 0U;
	}

	data->seg_left--;

	return &data->msgs[data->msg_idx].buf[data->msg_pos++];
}

static uint32_t ifx_scb_i2c_segment(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t addr = (uint32_t)data->addr << 1;

	data->seg_last = data->msg_idx;
	data->seg_left = data->msgs[data->msg_idx].len;
	data->msg_pos = 0U;

	while (((data->seg_last + 1U) < data->num_msgs) &&
	       ifx_scb_i2c_joined(data->msgs, data->seg_last + 1U)) {
		data->seg_last++;
		data->seg_left += data->msgs[data->seg_last].len;
	}

	sys_write32(IFX_SCB_INTR_M_ACK, config->base + IFX_SCB_INTR_M);

	if ((data->msgs[data->msg_idx].flags & I2C_MSG_READ) != 0U) {
		addr |= IFX_SCB_I2C_ADDR_READ;
		sys_write32(IFX_SCB_INTR_RX_NOT_EMPTY, config->base + IFX_SCB_INTR_RX);
		ifx_scb_i2c_mask(config->base, IFX_SCB_INTR_M_EVENTS, 0U,
				 IFX_SCB_INTR_RX_NOT_EMPTY);
	} else {
		ifx_scb_i2c_mask(config->base, IFX_SCB_INTR_M_EVENTS | IFX_SCB_INTR_M_ACK, 0U, 0U);
	}

	return addr;
}

static void ifx_scb_i2c_start(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t addr;

	ifx_scb_i2c_fifo_clear(config->base);
	sys_write32(IFX_SCB_INTR_M_EVENTS | IFX_SCB_INTR_M_ACK, config->base + IFX_SCB_INTR_M);

	addr = ifx_scb_i2c_segment(dev);
	data->state = IFX_SCB_I2C_CTRL_START;
	sys_write32(addr, config->base + IFX_SCB_TX_FIFO_WR);
	sys_write32(IFX_SCB_M_CMD_START_ON_IDLE, config->base + IFX_SCB_I2C_M_CMD);
}

static void ifx_scb_i2c_restart(const struct device *dev, uint32_t cmd)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t addr;

	data->msg_idx = data->seg_last + 1U;
	addr = ifx_scb_i2c_segment(dev);
	data->state = ((addr & IFX_SCB_I2C_ADDR_READ) != 0U) ? IFX_SCB_I2C_CTRL_READ
							     : IFX_SCB_I2C_CTRL_ADDR;
	sys_write32(IFX_SCB_M_CMD_START | cmd, config->base + IFX_SCB_I2C_M_CMD);
	sys_write32(addr, config->base + IFX_SCB_TX_FIFO_WR);
}

static void ifx_scb_i2c_stop(const struct device *dev, uint32_t cmd)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;

	data->msg_idx = data->seg_last + 1U;
	data->state = IFX_SCB_I2C_CTRL_STOP;
	sys_write32(IFX_SCB_M_CMD_STOP | cmd, config->base + IFX_SCB_I2C_M_CMD);
}

static void ifx_scb_i2c_finish(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;

	ifx_scb_i2c_mask(config->base, 0U, 0U, 0U);
	data->state = IFX_SCB_I2C_IDLE;
	data->msgs = NULL;
	k_sem_give(&data->done);
}

static void ifx_scb_i2c_drain(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	uint32_t status = sys_read32(config->base + IFX_SCB_TX_FIFO_STATUS);

	if ((status & (IFX_SCB_TX_FIFO_STATUS_USED | IFX_SCB_TX_FIFO_STATUS_SR_VALID)) != 0U) {
		return;
	}

	sys_write32(0U, config->base + IFX_SCB_INTR_TX_MASK);

	if (ifx_scb_i2c_seg_stop(dev->data)) {
		ifx_scb_i2c_stop(dev, 0U);
	} else {
		ifx_scb_i2c_restart(dev, 0U);
	}
}

static void ifx_scb_i2c_fill(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t used = FIELD_GET(IFX_SCB_TX_FIFO_STATUS_USED,
				  sys_read32(config->base + IFX_SCB_TX_FIFO_STATUS));
	uint32_t room = config->fifo_depth - MIN(used, config->fifo_depth);

	while ((room > 0U) && (data->seg_left > 0U)) {
		sys_write32(*ifx_scb_i2c_cursor(data), config->base + IFX_SCB_TX_FIFO_WR);
		room--;
	}

	if (data->seg_left > 0U) {
		data->state = IFX_SCB_I2C_CTRL_WRITE;
		sys_write32(IFX_SCB_INTR_TX_TRIGGER, config->base + IFX_SCB_INTR_TX);
		sys_write32(IFX_SCB_INTR_TX_TRIGGER, config->base + IFX_SCB_INTR_TX_MASK);
		return;
	}

	data->state = IFX_SCB_I2C_CTRL_DRAIN;
	sys_write32(IFX_SCB_INTR_TX_UNDERFLOW, config->base + IFX_SCB_INTR_TX);
	sys_write32(IFX_SCB_INTR_TX_UNDERFLOW, config->base + IFX_SCB_INTR_TX_MASK);
	ifx_scb_i2c_drain(dev);
}

static void ifx_scb_i2c_recv(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;

	*ifx_scb_i2c_cursor(data) = (uint8_t)sys_read32(config->base + IFX_SCB_RX_FIFO_RD);
	sys_write32(IFX_SCB_INTR_RX_NOT_EMPTY, config->base + IFX_SCB_INTR_RX);

	if (data->seg_left > 0U) {
		sys_write32(IFX_SCB_M_CMD_ACK, config->base + IFX_SCB_I2C_M_CMD);
		return;
	}

	sys_write32(0U, config->base + IFX_SCB_INTR_RX_MASK);

	if (ifx_scb_i2c_seg_stop(data)) {
		ifx_scb_i2c_stop(dev, IFX_SCB_M_CMD_NACK);
	} else {
		ifx_scb_i2c_restart(dev, IFX_SCB_M_CMD_NACK);
	}
}

static void ifx_scb_i2c_ctrl_isr(const struct device *dev, uint32_t cause)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t intr = 0U;

	if ((cause & IFX_SCB_INTR_CAUSE_M) != 0U) {
		intr = sys_read32(config->base + IFX_SCB_INTR_M_MASKED);
		sys_write32(intr, config->base + IFX_SCB_INTR_M);
	}

	if ((intr & (IFX_SCB_INTR_M_ARB_LOST | IFX_SCB_INTR_M_BUS_ERROR)) != 0U) {
		data->result = ((intr & IFX_SCB_INTR_M_BUS_ERROR) != 0U) ? -EIO : -EAGAIN;

		if ((data->result == -EIO) || (data->target == NULL) ||
		    (data->state != IFX_SCB_I2C_CTRL_START)) {
			ifx_scb_i2c_enable(dev);
		}

		ifx_scb_i2c_finish(dev);
		return;
	}

	if ((intr & IFX_SCB_INTR_M_NACK) != 0U) {
		data->result = -EIO;
		ifx_scb_i2c_mask(config->base, IFX_SCB_INTR_M_EVENTS, 0U, 0U);

		if (data->state != IFX_SCB_I2C_CTRL_STOP) {
			data->state = IFX_SCB_I2C_CTRL_STOP;
			sys_write32(IFX_SCB_M_CMD_NACK | IFX_SCB_M_CMD_STOP,
				    config->base + IFX_SCB_I2C_M_CMD);
		}
	}

	switch (data->state) {
	case IFX_SCB_I2C_CTRL_START:
	case IFX_SCB_I2C_CTRL_ADDR:
		if ((intr & IFX_SCB_INTR_M_ACK) != 0U) {
			sys_write32(IFX_SCB_INTR_M_EVENTS, config->base + IFX_SCB_INTR_M_MASK);
			ifx_scb_i2c_fill(dev);
		} else if ((cause & IFX_SCB_INTR_CAUSE_RX) != 0U) {
			data->state = IFX_SCB_I2C_CTRL_READ;
			ifx_scb_i2c_recv(dev);
		}
		break;
	case IFX_SCB_I2C_CTRL_WRITE:
		if ((cause & IFX_SCB_INTR_CAUSE_TX) != 0U) {
			ifx_scb_i2c_fill(dev);
		}
		break;
	case IFX_SCB_I2C_CTRL_DRAIN:
		if ((cause & IFX_SCB_INTR_CAUSE_TX) != 0U) {
			sys_write32(IFX_SCB_INTR_TX_UNDERFLOW, config->base + IFX_SCB_INTR_TX);
			ifx_scb_i2c_drain(dev);
		}
		break;
	case IFX_SCB_I2C_CTRL_READ:
		if ((cause & IFX_SCB_INTR_CAUSE_RX) != 0U) {
			ifx_scb_i2c_recv(dev);
		}
		break;
	default:
		break;
	}

	if ((intr & IFX_SCB_INTR_M_STOP) == 0U) {
		return;
	}

	if (data->state != IFX_SCB_I2C_CTRL_STOP) {
		data->result = -EIO;
	} else if ((data->result == 0) && (data->msg_idx < data->num_msgs)) {
		ifx_scb_i2c_start(dev);
		return;
	}

	ifx_scb_i2c_finish(dev);
}

static void ifx_scb_i2c_target_end(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;

	sys_write32(0U, config->base + IFX_SCB_INTR_TX_MASK);
	sys_write32(0U, config->base + IFX_SCB_INTR_RX_MASK);
	ifx_scb_i2c_fifo_clear(config->base);
	data->state = IFX_SCB_I2C_IDLE;
}

static void ifx_scb_i2c_target_error(const struct device *dev, uint32_t intr)
{
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;

	if (ifx_scb_i2c_is_ctrl(data->state)) {
		return;
	}

	if (data->state >= IFX_SCB_I2C_TGT_WRITE) {
		ifx_scb_i2c_target_end(dev);

		if (cb->error != NULL) {
			cb->error(data->target, ((intr & IFX_SCB_INTR_S_ARB_LOST) != 0U)
							? I2C_ERROR_ARBITRATION
							: I2C_ERROR_GENERIC);
		}
	}

	ifx_scb_i2c_enable(dev);
}

static void ifx_scb_i2c_target_recv(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint8_t val = (uint8_t)sys_read32(config->base + IFX_SCB_RX_FIFO_RD);

	sys_write32(IFX_SCB_INTR_RX_NOT_EMPTY, config->base + IFX_SCB_INTR_RX);

	if ((data->state == IFX_SCB_I2C_TGT_WRITE) && (cb->write_received != NULL) &&
	    (cb->write_received(data->target, val) != 0)) {
		data->state = IFX_SCB_I2C_TGT_NACK;
	}

	sys_write32((data->state == IFX_SCB_I2C_TGT_NACK) ? IFX_SCB_S_CMD_NACK : IFX_SCB_S_CMD_ACK,
		    config->base + IFX_SCB_I2C_S_CMD);
}

static void ifx_scb_i2c_target_send(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint8_t val = IFX_SCB_I2C_FILL_BYTE;

	if ((data->state == IFX_SCB_I2C_TGT_READ) &&
	    ((cb->read_processed == NULL) || (cb->read_processed(data->target, &val) != 0))) {
		data->state = IFX_SCB_I2C_TGT_NACK;
		val = IFX_SCB_I2C_FILL_BYTE;
	}

	sys_write32(val, config->base + IFX_SCB_TX_FIFO_WR);
	sys_write32(IFX_SCB_INTR_TX_EMPTY, config->base + IFX_SCB_INTR_TX);
}

static void ifx_scb_i2c_target_addr(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint8_t val = IFX_SCB_I2C_FILL_BYTE;
	int ret;

	if (ifx_scb_i2c_is_ctrl(data->state)) {
		sys_write32(0U, config->base + IFX_SCB_I2C_M_CMD);
		ifx_scb_i2c_mask(config->base, 0U, 0U, 0U);
		ifx_scb_i2c_fifo_clear(config->base);
	} else if (data->state >= IFX_SCB_I2C_TGT_WRITE) {
		ifx_scb_i2c_target_end(dev);
	}

	if (IS_ENABLED(CONFIG_PM)) {
		sys_write32(IFX_SCB_INTR_I2C_EC_WAKE_UP, config->base + IFX_SCB_INTR_I2C_EC);
	}

	if ((sys_read32(config->base + IFX_SCB_I2C_STATUS) & IFX_SCB_I2C_STATUS_S_READ) != 0U) {
		data->state = IFX_SCB_I2C_TGT_READ;
		ret = (cb->read_requested != NULL) ? cb->read_requested(data->target, &val)
						   : -ENOTSUP;
	} else {
		data->state = IFX_SCB_I2C_TGT_WRITE;
		ret = (cb->write_requested != NULL) ? cb->write_requested(data->target) : 0;
	}

	if (ret != 0) {
		data->state = IFX_SCB_I2C_IDLE;
		sys_write32(IFX_SCB_S_CMD_NACK, config->base + IFX_SCB_I2C_S_CMD);
		return;
	}

	if (data->state == IFX_SCB_I2C_TGT_READ) {
		sys_write32(val, config->base + IFX_SCB_TX_FIFO_WR);
		sys_write32(IFX_SCB_INTR_TX_EMPTY, config->base + IFX_SCB_INTR_TX);
		sys_write32(IFX_SCB_INTR_TX_EMPTY, config->base + IFX_SCB_INTR_TX_MASK);
	} else {
		sys_write32(IFX_SCB_INTR_RX_NOT_EMPTY, config->base + IFX_SCB_INTR_RX);
		sys_write32(IFX_SCB_INTR_RX_NOT_EMPTY, config->base + IFX_SCB_INTR_RX_MASK);
	}

	sys_write32(IFX_SCB_S_CMD_ACK, config->base + IFX_SCB_I2C_S_CMD);
}

static void ifx_scb_i2c_target_isr(const struct device *dev, uint32_t cause)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	const struct i2c_target_callbacks *cb = data->target->callbacks;
	uint32_t intr = 0U;

	if ((cause & IFX_SCB_INTR_CAUSE_S) != 0U) {
		intr = sys_read32(config->base + IFX_SCB_INTR_S_MASKED);
		sys_write32(intr, config->base + IFX_SCB_INTR_S);
	}

	if ((intr & IFX_SCB_INTR_S_ERRORS) != 0U) {
		ifx_scb_i2c_target_error(dev, intr);
		return;
	}

	if (data->state >= IFX_SCB_I2C_TGT_WRITE) {
		if ((cause & IFX_SCB_INTR_CAUSE_RX) != 0U) {
			ifx_scb_i2c_target_recv(dev);
		}

		if ((intr & IFX_SCB_INTR_S_NACK) != 0U) {
			sys_write32(0U, config->base + IFX_SCB_INTR_TX_MASK);
		} else if ((cause & IFX_SCB_INTR_CAUSE_TX) != 0U) {
			ifx_scb_i2c_target_send(dev);
		}
	}

	if (((intr & IFX_SCB_INTR_S_STOP) != 0U) && (data->state >= IFX_SCB_I2C_TGT_WRITE)) {
		ifx_scb_i2c_target_end(dev);

		if (cb->stop != NULL) {
			(void)cb->stop(data->target);
		}
	}

	if ((intr & IFX_SCB_INTR_S_ADDR_MATCH) != 0U) {
		ifx_scb_i2c_target_addr(dev);
	}
}

static void ifx_scb_i2c_wake_restore(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;

	if (sys_read32(config->base + IFX_SCB_INTR_I2C_EC_MASK) == 0U) {
		return;
	}

	sys_set_bits(config->base + IFX_SCB_I2C_CFG, IFX_SCB_I2C_CFG_CLK_ENABLE);
	sys_write32(0U, config->base + IFX_SCB_INTR_I2C_EC_MASK);
	sys_write32((data->target != NULL) ? IFX_SCB_INTR_S_EVENTS : 0U,
		    config->base + IFX_SCB_INTR_S_MASK);
}

static void ifx_scb_i2c_isr(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t cause = sys_read32(config->base + IFX_SCB_INTR_CAUSE);

	if (IS_ENABLED(CONFIG_PM) && ((cause & IFX_SCB_INTR_CAUSE_I2C_EC) != 0U)) {
		sys_write32(IFX_SCB_INTR_I2C_EC_WAKE_UP, config->base + IFX_SCB_INTR_I2C_EC);
		ifx_scb_i2c_wake_restore(dev);
		cause |= IFX_SCB_INTR_CAUSE_S;
	}

	if (ifx_scb_i2c_is_ctrl(data->state)) {
		ifx_scb_i2c_ctrl_isr(dev, cause);
	}

	if (IS_ENABLED(CONFIG_I2C_TARGET) && (data->target != NULL)) {
		ifx_scb_i2c_target_isr(dev, cause);
	}

	if ((data->state == IFX_SCB_I2C_IDLE) && (data->msgs != NULL)) {
		ifx_scb_i2c_start(dev);
	}

	k_spin_unlock(&data->lock, key);
}

static int ifx_scb_i2c_halt(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = (data->state == IFX_SCB_I2C_IDLE) ? 0 : -EBUSY;

	if (ret == 0) {
		sys_clear_bits(config->base + IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_scb_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	uint32_t speed = I2C_SPEED_GET(dev_config);
	uint32_t reg;
	int ret;

	if (((dev_config & I2C_ADDR_10_BITS) != 0U) || (speed < I2C_SPEED_STANDARD) ||
	    (speed > I2C_SPEED_FAST_PLUS)) {
		return -EIO;
	}

	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	ret = ifx_scb_i2c_halt(dev);
	if (ret != 0) {
		goto out;
	}

	ret = ifx_scb_i2c_set_speed(dev, speed);
	if (ret != 0) {
		(void)ifx_scb_i2c_set_speed(dev, I2C_SPEED_GET(data->dev_config));
	} else {
		data->dev_config = dev_config;
	}

	reg = sys_read32(config->base + IFX_SCB_I2C_CTRL) &
	      ~(IFX_SCB_I2C_CTRL_MASTER_MODE | IFX_SCB_I2C_CTRL_SLAVE_MODE);
	sys_write32(reg | ifx_scb_i2c_role(dev), config->base + IFX_SCB_I2C_CTRL);
	ifx_scb_i2c_enable(dev);

out:
	k_mutex_unlock(&data->bus_lock);
	(void)pm_device_runtime_put(dev);

	return (ret != 0) ? -EIO : 0;
}

static int ifx_scb_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct ifx_scb_i2c_data *data = dev->data;

	*dev_config = data->dev_config;

	return 0;
}

static int ifx_scb_i2c_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t len = 0U;
	int ret;

	if (num_msgs == 0U) {
		return 0;
	}

	if (addr > IFX_SCB_I2C_ADDR_MAX) {
		return -EIO;
	}

	for (uint8_t i = 0U; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_ADDR_10_BITS) != 0U) {
			return -EIO;
		}

		if ((msgs[i].len > 0U) && (msgs[i].buf == NULL)) {
			return -EIO;
		}

		len = ((i > 0U) && ifx_scb_i2c_joined(msgs, i)) ? (len + msgs[i].len) : msgs[i].len;

		if (((msgs[i].flags & I2C_MSG_READ) != 0U) && (len == 0U) &&
		    (((i + 1U) == num_msgs) || !ifx_scb_i2c_joined(msgs, i + 1U))) {
			return -EIO;
		}
	}

	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	if (((data->dev_config & I2C_MODE_CONTROLLER) == 0U) ||
	    ((data->target != NULL) && (data->target->address == addr))) {
		ret = -EIO;
		goto out;
	}

	pm_policy_device_power_lock_get(dev);
	k_sem_reset(&data->done);

	key = k_spin_lock(&data->lock);
	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->addr = addr;
	data->msg_idx = 0U;
	data->result = 0;

	if (data->state == IFX_SCB_I2C_IDLE) {
		ifx_scb_i2c_start(dev);
	}
	k_spin_unlock(&data->lock, key);

	(void)k_sem_take(&data->done, config->timeout);

	key = k_spin_lock(&data->lock);
	ret = (data->msgs == NULL) ? data->result : -ETIMEDOUT;
	data->msgs = NULL;

	if (ifx_scb_i2c_is_ctrl(data->state)) {
		ifx_scb_i2c_mask(config->base, 0U, 0U, 0U);
		ifx_scb_i2c_enable(dev);
		data->state = IFX_SCB_I2C_IDLE;
	}
	k_spin_unlock(&data->lock, key);

	pm_policy_device_power_lock_put(dev);

	if (ret == -ETIMEDOUT) {
		LOG_ERR("Transfer to 0x%02x timed out", addr);
	}

out:
	k_mutex_unlock(&data->bus_lock);
	(void)pm_device_runtime_put(dev);

	return (ret != 0) ? -EIO : 0;
}

#ifdef CONFIG_I2C_TARGET
static int ifx_scb_i2c_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if ((cfg == NULL) || (cfg->callbacks == NULL) ||
	    ((cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) != 0U) ||
	    (cfg->address > IFX_SCB_I2C_ADDR_MAX)) {
		return -EINVAL;
	}

	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	key = k_spin_lock(&data->lock);

	if (data->target != NULL) {
		ret = -EIO;
	} else {
		sys_clear_bits(config->base + IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED);
		sys_write32(IFX_SCB_RX_MATCH_TARGET(cfg->address), config->base + IFX_SCB_RX_MATCH);
		sys_set_bits(config->base + IFX_SCB_I2C_CTRL, IFX_SCB_I2C_CTRL_SLAVE_MODE);
		sys_write32(IFX_SCB_INTR_S_EVENTS, config->base + IFX_SCB_INTR_S_MASK);
		data->target = cfg;
		ifx_scb_i2c_enable(dev);
	}

	k_spin_unlock(&data->lock, key);
	k_mutex_unlock(&data->bus_lock);

	if (ret != 0) {
		(void)pm_device_runtime_put(dev);
	}

	return ret;
}

static int ifx_scb_i2c_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	k_mutex_lock(&data->bus_lock, K_FOREVER);
	key = k_spin_lock(&data->lock);

	if ((cfg == NULL) || (data->target != cfg) || (data->state >= IFX_SCB_I2C_TGT_WRITE)) {
		ret = -EINVAL;
	} else {
		sys_clear_bits(config->base + IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED);
		sys_write32(0U, config->base + IFX_SCB_INTR_S_MASK);
		sys_clear_bits(config->base + IFX_SCB_I2C_CTRL, IFX_SCB_I2C_CTRL_SLAVE_MODE);
		sys_write32(0U, config->base + IFX_SCB_RX_MATCH);
		data->target = NULL;
		ifx_scb_i2c_enable(dev);
	}

	k_spin_unlock(&data->lock, key);
	k_mutex_unlock(&data->bus_lock);

	if (ret == 0) {
		(void)pm_device_runtime_put(dev);
	}

	return ret;
}
#endif

#ifdef CONFIG_I2C_INFINEON_BUS_RECOVERY
static void ifx_scb_i2c_set_scl(void *io_context, int state)
{
	const struct ifx_scb_i2c_config *config = io_context;

	(void)gpio_pin_set_dt(&config->scl, state);
}

static void ifx_scb_i2c_set_sda(void *io_context, int state)
{
	const struct ifx_scb_i2c_config *config = io_context;

	(void)gpio_pin_set_dt(&config->sda, state);
}

static int ifx_scb_i2c_get_sda(void *io_context)
{
	const struct ifx_scb_i2c_config *config = io_context;

	return (gpio_pin_get_dt(&config->sda) > 0) ? 1 : 0;
}

static int ifx_scb_i2c_recover_bus(const struct device *dev)
{
	const struct i2c_bitbang_io io = {
		.set_scl = ifx_scb_i2c_set_scl,
		.set_sda = ifx_scb_i2c_set_sda,
		.get_sda = ifx_scb_i2c_get_sda,
	};
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	gpio_flags_t flags = GPIO_INPUT | GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN;
	struct i2c_bitbang bitbang;
	int restore;
	int ret;

	if ((config->scl.port == NULL) || (config->sda.port == NULL)) {
		return -ENOSYS;
	}

	if (pm_device_runtime_get(dev) < 0) {
		return -EIO;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	if (ifx_scb_i2c_halt(dev) != 0) {
		ret = -EIO;
		goto out;
	}

	ret = gpio_pin_configure_dt(&config->scl, flags);
	if (ret == 0) {
		ret = gpio_pin_configure_dt(&config->sda, flags);
	}

	if (ret == 0) {
		i2c_bitbang_init(&bitbang, &io, (void *)config);
		ret = i2c_bitbang_configure(&bitbang, I2C_MODE_CONTROLLER |
							      I2C_SPEED_SET(I2C_SPEED_STANDARD));
	}

	ret = (ret == 0) ? i2c_bitbang_recover_bus(&bitbang) : -EIO;

	restore = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	ifx_scb_i2c_enable(dev);

	if ((ret == 0) && (restore != 0)) {
		ret = -EIO;
	}

out:
	k_mutex_unlock(&data->bus_lock);
	(void)pm_device_runtime_put(dev);

	return ret;
}
#endif

static int ifx_scb_i2c_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if ((sys_read32(config->base + IFX_SCB_CTRL) & IFX_SCB_CTRL_ENABLED) != 0U) {
			return 0;
		}

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			return ret;
		}

		return ifx_scb_i2c_hw_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		if (k_mutex_lock(&data->bus_lock, K_NO_WAIT) != 0) {
			return -EBUSY;
		}

		if (pm_device_wakeup_is_enabled(dev)) {
			ret = (data->state == IFX_SCB_I2C_IDLE) ? 0 : -EBUSY;
			k_mutex_unlock(&data->bus_lock);
			return ret;
		}

		ret = ifx_scb_i2c_halt(dev);
		k_mutex_unlock(&data->bus_lock);
		if (ret != 0) {
			return ret;
		}

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);

		return (ret == -ENOENT) ? 0 : ret;
	case PM_DEVICE_ACTION_TURN_ON:
		return ifx_scb_i2c_clk_init(dev);
	default:
		return -ENOTSUP;
	}
}

static int ifx_scb_i2c_init(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	int ret;

	k_mutex_init(&data->bus_lock);
	k_sem_init(&data->done, 0, 1);
	data->dev_config = I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->bitrate);

	ifx_scb_i2c_quiesce(dev);
	config->irq_config();

	ret = pm_device_driver_init(dev, ifx_scb_i2c_pm_action);
	if (ret != 0) {
		return ret;
	}

	return pm_device_runtime_enable(dev);
}

static DEVICE_API(i2c, ifx_scb_i2c_api) = {
	.configure = ifx_scb_i2c_configure,
	.get_config = ifx_scb_i2c_get_config,
	.transfer = ifx_scb_i2c_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = ifx_scb_i2c_target_register,
	.target_unregister = ifx_scb_i2c_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
#ifdef CONFIG_I2C_INFINEON_BUS_RECOVERY
	.recover_bus = ifx_scb_i2c_recover_bus,
#endif
};

#define IFX_SCB_I2C_DIV(n) DT_INST_CLOCKS_CTLR(n)

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#define IFX_SCB_I2C_CLOCK_INIT(n)                                                                  \
	.clk_dev = DEVICE_DT_GET(DT_PARENT(IFX_SCB_I2C_DIV(n))),                                   \
	.clk = IFX_CLK_PERI_DT_INST_SPEC_GET(n),
#else
#define IFX_SCB_I2C_DIV_TYPE(n) DT_PROP(IFX_SCB_I2C_DIV(n), div_type)
#ifdef CONFIG_SOC_FAMILY_INFINEON_EDGE
#define IFX_SCB_I2C_CLOCK_BLOCK(n)                                                                 \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(IFX_SCB_I2C_DIV(n), peri_group, 0),        \
					 DT_PROP_BY_IDX(IFX_SCB_I2C_DIV(n), peri_group, 1),        \
					 IFX_SCB_I2C_DIV_TYPE(n))
#else
#define IFX_SCB_I2C_CLOCK_BLOCK(n)                                                                 \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(IFX_SCB_I2C_DIV(n), peri_group, 1),        \
					 IFX_SCB_I2C_DIV_TYPE(n))
#endif
#define IFX_SCB_I2C_CLOCK_INIT(n)                                                                  \
	.clk = {                                                                                   \
		.block = IFX_SCB_I2C_CLOCK_BLOCK(n),                                               \
		.channel = DT_PROP(IFX_SCB_I2C_DIV(n), channel),                                   \
	},                                                                                         \
	.clk_dst = (en_clk_dst_t)DT_INST_PROP(n, clk_dst),
#endif

#ifdef CONFIG_I2C_INFINEON_BUS_RECOVERY
#define IFX_SCB_I2C_RECOVERY_INIT(n)                                                               \
	.scl = GPIO_DT_SPEC_INST_GET_OR(n, scl_gpios, {0}),                                        \
	.sda = GPIO_DT_SPEC_INST_GET_OR(n, sda_gpios, {0}),
#else
#define IFX_SCB_I2C_RECOVERY_INIT(n)
#endif

#define IFX_SCB_I2C_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void ifx_scb_i2c_irq_config_##n(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ifx_scb_i2c_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ifx_scb_i2c_config ifx_scb_i2c_config_##n = {                          \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		IFX_SCB_I2C_CLOCK_INIT(n)                                                          \
		.bitrate = DT_INST_PROP_OR(n, clock_frequency, I2C_BITRATE_STANDARD),              \
		.fifo_depth = DT_INST_PROP(n, fifo_depth),                                         \
		.timeout = I2C_DT_INST_TRANSFER_TIMEOUT(n),                                        \
		.irq_config = ifx_scb_i2c_irq_config_##n,                                          \
		IFX_SCB_I2C_RECOVERY_INIT(n)                                                       \
	};                                                                                         \
                                                                                                   \
	static struct ifx_scb_i2c_data ifx_scb_i2c_data_##n;                                       \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, ifx_scb_i2c_pm_action);                                        \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(n, ifx_scb_i2c_init, PM_DEVICE_DT_INST_GET(n),                   \
				  &ifx_scb_i2c_data_##n, &ifx_scb_i2c_config_##n, POST_KERNEL,     \
				  CONFIG_I2C_INIT_PRIORITY, &ifx_scb_i2c_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SCB_I2C_INIT)

#ifdef CONFIG_PM
#define IFX_SCB_I2C_DEV_GET(n) DEVICE_DT_INST_GET(n),

static const struct device *const ifx_scb_i2c_devs[] = {
	DT_INST_FOREACH_STATUS_OKAY(IFX_SCB_I2C_DEV_GET)};

static void ifx_scb_i2c_sleep_enter(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t ctrl = sys_read32(config->base + IFX_SCB_CTRL);

	if (((ctrl & IFX_SCB_CTRL_ENABLED) == 0U) || (data->state != IFX_SCB_I2C_IDLE)) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if ((ctrl & IFX_SCB_CTRL_EC_AM_MODE) != 0U) {
		sys_write32((data->target != NULL)
				    ? (IFX_SCB_INTR_S_EVENTS & ~IFX_SCB_INTR_S_ADDR_MATCH)
				    : 0U,
			    config->base + IFX_SCB_INTR_S_MASK);
		sys_write32(IFX_SCB_INTR_I2C_EC_WAKE_UP, config->base + IFX_SCB_INTR_I2C_EC_MASK);
		sys_clear_bits(config->base + IFX_SCB_I2C_CFG, IFX_SCB_I2C_CFG_CLK_ENABLE);
	} else if (!IS_ENABLED(CONFIG_PM_DEVICE)) {
		sys_clear_bits(config->base + IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED);
	}

	k_spin_unlock(&data->lock, key);
}

static void ifx_scb_i2c_sleep_exit(const struct device *dev)
{
	const struct ifx_scb_i2c_config *config = dev->config;
	struct ifx_scb_i2c_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ifx_scb_i2c_wake_restore(dev);

	if (!IS_ENABLED(CONFIG_PM_DEVICE) &&
	    ((sys_read32(config->base + IFX_SCB_CTRL) & IFX_SCB_CTRL_ENABLED) == 0U)) {
		ifx_scb_i2c_enable(dev);
	}

	k_spin_unlock(&data->lock, key);
}

static void ifx_scb_i2c_for_each(enum pm_state state, void (*fn)(const struct device *dev))
{
	if (state == PM_STATE_RUNTIME_IDLE) {
		return;
	}

	ARRAY_FOR_EACH(ifx_scb_i2c_devs, i) {
		if (device_is_ready(ifx_scb_i2c_devs[i]) &&
		    pm_device_is_powered(ifx_scb_i2c_devs[i])) {
			fn(ifx_scb_i2c_devs[i]);
		}
	}
}

static void ifx_scb_i2c_state_entry(enum pm_state state)
{
	ifx_scb_i2c_for_each(state, ifx_scb_i2c_sleep_enter);
}

static void ifx_scb_i2c_state_exit(enum pm_state state)
{
	ifx_scb_i2c_for_each(state, ifx_scb_i2c_sleep_exit);
}

static struct pm_notifier ifx_scb_i2c_notifier = {
	.state_entry = ifx_scb_i2c_state_entry,
	.state_exit = ifx_scb_i2c_state_exit,
};

static int ifx_scb_i2c_notifier_init(void)
{
	pm_notifier_register(&ifx_scb_i2c_notifier);

	return 0;
}

SYS_INIT(ifx_scb_i2c_notifier_init, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY);
#endif
