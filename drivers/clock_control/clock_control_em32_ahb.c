/*
 * SPDX-FileCopyrightText: 2026 ELAN Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/em32_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/syscon.h>
LOG_MODULE_REGISTER(em32_ahb, CONFIG_LOG_DEFAULT_LEVEL);

#include <soc_clkctrl.h>
#include <soc_infoctrl.h>
#include <soc_sysctrl.h>

/* Syscon must be initialized before the AHB clock controller. */
BUILD_ASSERT(CONFIG_SYSCON_INIT_PRIORITY < CONFIG_CLOCK_CONTROL_EM32_AHB_INIT_PRIORITY,
	     "AHB clock controller must initialize after syscon");

#define DWT_UNLOCK_KEY 0xC5ACCE55U
#define EM32_GATE_MAX  63U

/* Clock gate register updates are serialized by syscon_update_bits() internal spinlock. */

struct elan_em32_ahb_clock_control_config {
	const struct device *sysctrl_syscon;
	const struct device *clkctrl_syscon;
	const struct device *infoctrl_syscon;
	uint32_t clock_source;
	uint32_t clock_frequency;
	uint32_t clock_divider;
};

/*
 * Configurations
 */
static uint32_t ahb_count_khz = 12000;
static bool g_dwt_ok;

static inline void early_delay_us(uint32_t us)
{
	if (g_dwt_ok) {
		/*
		 * Use DWT cycle counter for early busy-wait delays.
		 *
		 * This requires the effective core frequency (SystemCoreClock or an
		 * equivalent) to be in sync with the active clock configuration.
		 * Accuracy depends on that value being correct.
		 */
		uint64_t hz = (uint64_t)ahb_count_khz * 1000U;
		uint32_t cycles = (uint32_t)((hz * us) / 1000000ULL);
		uint32_t start = DWT->CYCCNT;

		while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
			/* Busy-wait */
		}
	} else {
		/*
		 * Fallback implementation using a minimal NOP loop.
		 *
		 * Intended only to add a tiny gap between back-to-back register
		 * writes when DWT is unavailable. This is not an accurate µs delay.
		 */
		for (uint32_t i = 0; i < (ahb_count_khz / 1000U) * (us / 10U ? us / 10U : 1U);
		     i++) {
			arch_nop();
		}
	}
}

/*
 * Late-stage delay using Zephyr timing services.
 *
 * After the kernel time base is ready, k_busy_wait() provides a calibrated,
 * clock-aware busy wait and should be preferred to ad-hoc loops.
 */
static void late_delay_us(uint32_t us)
{
	k_busy_wait(us);
}

/*
 * Delay backend indirection.
 *
 * Default to the early implementation until the kernel timing subsystem
 * becomes available; then switch to the late path.
 */
static void (*delay_us_impl)(uint32_t us) = early_delay_us;

/**
 * @brief Busy-wait for at least @p us microseconds.
 *
 * Abstracts the underlying delay mechanism. During early boot it uses a
 * DWT-based implementation and switches to k_busy_wait() after kernel init.
 *
 * Notes:
 * - Suitable for early clock/power sequencing.
 * - Acceptable for short delays in ISRs.
 * - Avoid long busy-waits; prefer polling with timeout or
 *   k_sleep()/k_msleep() when scheduling is possible.
 */
static inline void delay_us(uint32_t us)
{
	delay_us_impl(us);
}

/* Syscon-based helpers for SYSCTRL, CLKCTRL and INFOCTRL register access */
static inline int ahb_em32_syscon_read_field(const struct device *syscon, uint32_t offset,
					     uint32_t mask, uint32_t *value)
{
	uint32_t reg;
	int ret;

	ret = syscon_read_reg(syscon, offset, &reg);
	if (ret < 0) {
		return ret;
	}
	*value = FIELD_GET(mask, reg);
	return 0;
}

/* Read both factory trim fields from one INFOCTRL register access. */
static int ahb_em32_syscon_read_mirc_trim(const struct device *syscon, uint32_t offset,
					  uint32_t *tall, uint32_t *tv12)
{
	uint32_t reg;
	int ret;

	ret = syscon_read_reg(syscon, offset, &reg);
	if (ret < 0) {
		return ret;
	}

	*tall = FIELD_GET(MIRC_TALL_MASK, reg);
	*tv12 = FIELD_GET(MIRC_TV12_MASK, reg);

	return 0;
}

static inline int ahb_em32_syscon_write_field(const struct device *syscon, uint32_t offset,
					      uint32_t mask, uint32_t value)
{
	__ASSERT(mask != 0U, "mask must not be zero");

	/* Optional: check value range */
	if ((value << __builtin_ctz(mask)) & ~mask) {
		LOG_ERR("Value 0x%x exceeds field mask 0x%x", value, mask);
		return -EINVAL;
	}

	return syscon_update_bits(syscon, offset, mask, value << __builtin_ctz(mask));
}

/* Gate value semantics for readability. */
enum em32_gate_val {
	EM32_GATE_OPEN = 0u,   /* clear bit -> clock enabled */
	EM32_GATE_CLOSED = 1u, /* set bit   -> clock gated   */
};

static inline bool em32_gate_is_all(uint32_t gate_idx)
{
	return (gate_idx == (uint32_t)EM32_GATE_PCLKG_ALL);
}

static inline bool em32_gate_is_valid(uint32_t gate_idx)
{
	/* Valid when in [0..63] or the ALL marker is used. */
	return (gate_idx <= EM32_GATE_MAX) || em32_gate_is_all(gate_idx);
}

/*
 * Write a single gate bit using the unified field RMW helper.
 * For ALL, only EM32_GATE_OPEN is accepted (open all clocks).
 * Closing ALL clocks is rejected as unsafe.
 *
 * Clock gate register updates are serialized by syscon_update_bits()
 * internal spinlock, so no additional locking is needed here.
 */
static inline int em32_clk_gate_write(const struct device *syscon, uint32_t gate_idx,
				      enum em32_gate_val val)
{
	int ret;

	if (!em32_gate_is_valid(gate_idx)) {
		LOG_ERR("Gate index %u out of range", gate_idx);
		return -EINVAL;
	}

	if (em32_gate_is_all(gate_idx)) {
		if (val == EM32_GATE_OPEN) {
			ret = syscon_write_reg(syscon, SYSCTRL_CLK_GATE_REG_OFF,
					       (uint32_t)EM32_GATE_OPEN);
			if (ret == 0) {
				ret = syscon_write_reg(syscon, SYSCTRL_CLK_GATE_REG2_OFF,
						       (uint32_t)EM32_GATE_OPEN);
			}
		} else {
			/* Reject closing all gates to avoid system shutdown. */
			LOG_WRN("Closing ALL gates is not supported");
			return -ENOTSUP;
		}
	} else {
		uint32_t bit = (gate_idx <= 31u) ? gate_idx : (gate_idx - 32u);
		uint32_t off =
			(gate_idx <= 31u) ? SYSCTRL_CLK_GATE_REG_OFF : SYSCTRL_CLK_GATE_REG2_OFF;

		ret = syscon_update_bits(syscon, off, BIT(bit), (uint32_t)val);
	}

	if (ret < 0) {
		LOG_ERR("Clock gate update failed: %d", ret);
	}

	return ret;
}

static inline int em32_clk_gate_open(const struct device *syscon, uint32_t gate_idx)
{
	return em32_clk_gate_write(syscon, gate_idx, EM32_GATE_OPEN);
}

static inline int em32_clk_gate_close(const struct device *syscon, uint32_t gate_idx)
{
	return em32_clk_gate_write(syscon, gate_idx, EM32_GATE_CLOSED);
}

static int elan_em32_get_ahb_freq(const struct device *dev, uint32_t *freq)
{
	const struct elan_em32_ahb_clock_control_config *config = dev->config;
	const struct device *sysctrl_syscon = config->sysctrl_syscon;
	const struct device *clkctrl_syscon = config->clkctrl_syscon;
	uint32_t irc_freq_khz;
	uint32_t irc_pll_freq_khz;
	uint32_t main_freq_khz;
	uint32_t ahb_freq_khz;

	uint32_t mirc_rcm;
	int ret;

	ret = ahb_em32_syscon_read_field(clkctrl_syscon, CLKCTRL_MIRC_CTRL_OFF,
					 CLKCTRL_MIRC_RCM_MASK, &mirc_rcm);
	if (ret < 0) {
		return ret;
	}
	switch (mirc_rcm) {
	case 0x00:
		irc_freq_khz = 12000;
		irc_pll_freq_khz = 12000 * 16 / 2;
		break; /* 12M/120M */
	case 0x01:
		irc_freq_khz = 16000;
		irc_pll_freq_khz = 16000 * 16 / 4;
		break; /* 16M/80M */
	case 0x02:
		irc_freq_khz = 20000;
		irc_pll_freq_khz = 20000 * 16 / 4;
		break; /* 20M/100M */
	case 0x03:
		irc_freq_khz = 24000;
		irc_pll_freq_khz = 24000 * 16 / 4;
		break; /* 24M/120M */
	case 0x04:
		irc_freq_khz = 28000;
		irc_pll_freq_khz = 28000 * 16 / 6;
		break; /* 28M/93M*/
	case 0x05:
		irc_freq_khz = 32000;
		irc_pll_freq_khz = 32000 * 16 / 6;
		break; /* 32M/107M */
	default:
		LOG_ERR("Unsupported MIRC_RCM value %u", mirc_rcm);
		return -EINVAL;
	}

	uint32_t sysctrl_reg;
	uint32_t hclk_sel;

	ret = syscon_read_reg(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF, &sysctrl_reg);
	if (ret < 0) {
		return ret;
	}

	hclk_sel = FIELD_GET(SYSCTRL_HCLK_SEL_MASK, sysctrl_reg);
	switch (hclk_sel) {
	case 0x00:
		main_freq_khz = irc_freq_khz;
		break;

	case 0x01:
		if (FIELD_GET(SYSCTRL_XTAL_HIRC_SEL, sysctrl_reg)) {
			main_freq_khz = 24000 * 5;
		} else {
			main_freq_khz = irc_pll_freq_khz;
		}
		break;

	case 0x02:
		main_freq_khz = 0xffffffffU;
		break;

	default:
		main_freq_khz = 0;
		break;
	}

	main_freq_khz = main_freq_khz >> FIELD_GET(SYSCTRL_HCLK_DIV_MASK, sysctrl_reg);
	ahb_freq_khz = main_freq_khz;

	*freq = ahb_freq_khz;
	return 0;
}

static int elan_em32_set_ahb_freq(const struct device *dev)
{
	const struct elan_em32_ahb_clock_control_config *config = dev->config;
	const struct device *sysctrl_syscon = config->sysctrl_syscon;
	const struct device *clkctrl_syscon = config->clkctrl_syscon;
	const struct device *infoctrl_syscon = config->infoctrl_syscon;
	uint32_t clk_src = config->clock_source;
	uint32_t freq_src = config->clock_frequency;
	uint32_t pre_div = config->clock_divider;
	bool b_pll;
	int ret;

	ret = em32_clk_gate_open(sysctrl_syscon, EM32_GATE_PCLKG_AIP);
	if (ret < 0) {
		return ret;
	}

	if (freq_src == EM32_CLK_FREQ_IRCLOW12 /* irc_freq_src */) {
		return ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
						   SYSCTRL_HCLK_DIV_MASK, pre_div);
	}

	ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_MISC_REG_CTRL_OFF,
					  SYSCTRL_WAIT_COUNT_PASS_MASK, 0x0a);
	if (ret < 0) {
		return ret;
	}
	ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_MISC_REG_CTRL_OFF,
					  SYSCTRL_WAIT_COUNT_MASK, 0x03);
	if (ret < 0) {
		return ret;
	}
	ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_MISC_REG_CTRL_OFF,
					  SYSCTRL_WAIT_COUNT_SET, 0x01);
	if (ret < 0) {
		return ret;
	}

	uint32_t hclk_sel;

	ret = ahb_em32_syscon_read_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
					 SYSCTRL_HCLK_SEL_MASK, &hclk_sel);
	if (ret < 0) {
		return ret;
	}
	if (hclk_sel == 0x01) {
		ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
						  SYSCTRL_HCLK_SEL_MASK, 0x00);
		if (ret < 0) {
			return ret;
		}
		delay_us(100);
		ret = ahb_em32_syscon_write_field(clkctrl_syscon, CLKCTRL_SYS_PLL_CTRL_OFF,
						  CLKCTRL_SYS_PLL_PD, 0x01);
		if (ret < 0) {
			return ret;
		}
		delay_us(1);
	}

	if (clk_src == EM32_CLK_SRC_EXTERNAL1) {
		ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
						  SYSCTRL_HCLK_SEL_MASK, 0x02);
		if (ret < 0) {
			return ret;
		}
	} else {
		if (freq_src >> 4) {
			b_pll = true;
		} else {
			b_pll = false;
		}

		uint32_t mirc_trim_offset;
		uint32_t mirc_tall, mirc_tv12;
		bool apply_mirc_trim = true;

		switch (freq_src) {
		case EM32_CLK_FREQ_IRCLOW12:
			mirc_trim_offset = MIRC_12M_R_2_OFF;
			break;

		case EM32_CLK_FREQ_IRCLOW16:
		case EM32_CLK_FREQ_IRCHIGH64:
			mirc_trim_offset = MIRC_16M_2_OFF;
			break;

		case EM32_CLK_FREQ_IRCLOW20:
		case EM32_CLK_FREQ_IRCHIGH80:
			mirc_trim_offset = MIRC_20M_2_OFF;
			break;

		case EM32_CLK_FREQ_IRCLOW24:
		case EM32_CLK_FREQ_IRCHIGH96:
			mirc_trim_offset = MIRC_24M_2_OFF;
			break;

		case EM32_CLK_FREQ_IRCLOW28:
		case EM32_CLK_FREQ_IRCHIGH112:
			mirc_trim_offset = MIRC_28M_2_OFF;
			break;

		case EM32_CLK_FREQ_IRCLOW32:
		case EM32_CLK_FREQ_IRCHIGH128:
			mirc_trim_offset = MIRC_32M_2_OFF;
			break;

		default:
			apply_mirc_trim = false;
			break;
		}

		if (apply_mirc_trim) {
			ret = ahb_em32_syscon_read_mirc_trim(infoctrl_syscon, mirc_trim_offset,
							     &mirc_tall, &mirc_tv12);
			if (ret < 0) {
				return ret;
			}

			ret = syscon_update_bits(
				clkctrl_syscon, CLKCTRL_MIRC_CTRL2_OFF,
				CLKCTRL_MIRC2_TALL_MASK | CLKCTRL_MIRC2_TV12_MASK,
				FIELD_PREP(CLKCTRL_MIRC2_TALL_MASK, mirc_tall & 0x3FF) |
					FIELD_PREP(CLKCTRL_MIRC2_TV12_MASK, ~mirc_tv12 & 0x7));
			if (ret < 0) {
				return ret;
			}
		}

		delay_us(100);
		ret = ahb_em32_syscon_write_field(clkctrl_syscon, CLKCTRL_MIRC_CTRL_OFF,
						  CLKCTRL_MIRC_RCM_MASK, freq_src & 0x0f);
		if (ret < 0) {
			return ret;
		}
		ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
						  SYSCTRL_XTAL_HIRC_SEL, 0x00);
		if (ret < 0) {
			return ret;
		}

		if (b_pll) {
			switch (freq_src) {
			case EM32_CLK_FREQ_IRCHIGH64:
				ret = ahb_em32_syscon_write_field(clkctrl_syscon,
								  CLKCTRL_SYS_PLL_CTRL_OFF,
								  CLKCTRL_SYS_PLL_FSET_MASK, 0x00);
				if (ret < 0) {
					return ret;
				}
				break;
			case EM32_CLK_FREQ_IRCHIGH80:
				ret = ahb_em32_syscon_write_field(clkctrl_syscon,
								  CLKCTRL_SYS_PLL_CTRL_OFF,
								  CLKCTRL_SYS_PLL_FSET_MASK, 0x01);
				if (ret < 0) {
					return ret;
				}
				break;
			case EM32_CLK_FREQ_IRCHIGH96:
				ret = ahb_em32_syscon_write_field(clkctrl_syscon,
								  CLKCTRL_SYS_PLL_CTRL_OFF,
								  CLKCTRL_SYS_PLL_FSET_MASK, 0x02);
				if (ret < 0) {
					return ret;
				}
				break;
			case EM32_CLK_FREQ_IRCHIGH112:
				ret = ahb_em32_syscon_write_field(clkctrl_syscon,
								  CLKCTRL_SYS_PLL_CTRL_OFF,
								  CLKCTRL_SYS_PLL_FSET_MASK, 0x03);
				if (ret < 0) {
					return ret;
				}
				break;
			case EM32_CLK_FREQ_IRCHIGH128:
				ret = ahb_em32_syscon_write_field(clkctrl_syscon,
								  CLKCTRL_SYS_PLL_CTRL_OFF,
								  CLKCTRL_SYS_PLL_FSET_MASK, 0x03);
				if (ret < 0) {
					return ret;
				}
				break;
			default:
				break;
			}

			ret = ahb_em32_syscon_write_field(clkctrl_syscon, CLKCTRL_LDO_PLL_OFF,
							  CLKCTRL_PLL_LDO_PD, 0x00);
			if (ret < 0) {
				return ret;
			}
			delay_us(1);
			ret = ahb_em32_syscon_write_field(clkctrl_syscon, CLKCTRL_LDO_PLL_OFF,
							  CLKCTRL_PLL_LDO_VP_SEL, 0x00);
			if (ret < 0) {
				return ret;
			}
			delay_us(10);
			ret = ahb_em32_syscon_write_field(clkctrl_syscon, CLKCTRL_SYS_PLL_CTRL_OFF,
							  CLKCTRL_SYS_PLL_PD, 0x00);
			if (ret < 0) {
				return ret;
			}
			delay_us(1);
			uint32_t pll_stable;

			do {
				ret = ahb_em32_syscon_read_field(
					clkctrl_syscon, CLKCTRL_SYS_PLL_CTRL_OFF,
					CLKCTRL_SYS_PLL_STABLE, &pll_stable);
				if (ret < 0) {
					return ret;
				}
			} while (pll_stable == 0);
			delay_us(1);
			ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
							  SYSCTRL_HCLK_SEL_MASK, 0x01);
			if (ret < 0) {
				return ret;
			}
			delay_us(1);
		} else {
			ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
							  SYSCTRL_HCLK_SEL_MASK, 0x00);
			if (ret < 0) {
				return ret;
			}
			delay_us(100);
			ret = ahb_em32_syscon_write_field(clkctrl_syscon, CLKCTRL_SYS_PLL_CTRL_OFF,
							  CLKCTRL_SYS_PLL_PD, 0x01);
			if (ret < 0) {
				return ret;
			}
		}
	}

	if (pre_div == EM32_AHB_CLK_DIV128) {
		ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
						  SYSCTRL_HCLK_DIV_MASK, pre_div - 1);
	} else {
		ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
						  SYSCTRL_HCLK_DIV_MASK, pre_div + 1);
	}
	if (ret < 0) {
		return ret;
	}

	ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_MISC_REG_CTRL_OFF,
					  SYSCTRL_WAIT_COUNT_SET, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_MISC_REG_CTRL_OFF,
					  SYSCTRL_WAIT_COUNT_PASS_MASK, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = ahb_em32_syscon_write_field(sysctrl_syscon, SYSCTRL_SYS_REG_CTRL_OFF,
					  SYSCTRL_HCLK_DIV_MASK, pre_div);
	if (ret < 0) {
		return ret;
	}

	ret = elan_em32_get_ahb_freq(dev, &ahb_count_khz);
	if (ret) {
		return ret;
	}

	return 0;
}

static int elan_em32_ahb_clock_control_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct elan_em32_ahb_clock_control_config *cfg = dev->config;
	uint32_t clk_grp = POINTER_TO_UINT(sys);

	/* API-level "ALL" */
	if (sys == CLOCK_CONTROL_SUBSYS_ALL || clk_grp == EM32_GATE_PCLKG_ALL) {
		/* Enabling all clock == open gate. */
		return em32_clk_gate_open(cfg->sysctrl_syscon, EM32_GATE_PCLKG_ALL);
	}

	if (clk_grp == EM32_GATE_NONE) {
		/* No hardware gate: parent reference only */
		return 0;
	}

	/* Accept known indices and the ALL marker. */
	if ((clk_grp < EM32_GATE_HCLKG_DMA) || (clk_grp > EM32_GATE_PCLKG_SSP1)) {
		LOG_ERR("Unknown clock group #%u", clk_grp);
		return -EINVAL;
	}

	/* Enabling a clock == open gate (clear the bit). */
	return em32_clk_gate_open(cfg->sysctrl_syscon, clk_grp);
}

static int elan_em32_ahb_clock_control_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct elan_em32_ahb_clock_control_config *cfg = dev->config;
	uint32_t clk_grp = POINTER_TO_UINT(sys);

	/* Do not support closing ALL clocks; reject explicitly. */
	if (sys == CLOCK_CONTROL_SUBSYS_ALL || clk_grp == EM32_GATE_PCLKG_ALL) {
		return -ENOTSUP;
	}

	if (clk_grp == EM32_GATE_NONE) {
		/* No hardware gate: parent reference only */
		return 0;
	}

	if ((clk_grp < EM32_GATE_HCLKG_DMA) || (clk_grp > EM32_GATE_PCLKG_SSP1)) {
		LOG_ERR("Unknown clock group #%u", clk_grp);
		return -EINVAL;
	}

	/* Disabling a clock == close gate (set the bit). */
	return em32_clk_gate_close(cfg->sysctrl_syscon, clk_grp);
}

static int elan_em32_ahb_clock_control_get_rate(const struct device *dev,
						clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	/* elan_em32_get_ahb_freq(dev) returns kHz; convert to Hz. */
	uint32_t ahb_khz;
	int ret;

	ret = elan_em32_get_ahb_freq(dev, &ahb_khz);
	if (ret) {
		return ret;
	}

	*rate = ahb_khz * 1000U;

	return 0;
}

static DEVICE_API(clock_control, elan_em32_ahb_clock_control_api) = {
	.on = elan_em32_ahb_clock_control_on,
	.off = elan_em32_ahb_clock_control_off,
	.get_rate = elan_em32_ahb_clock_control_get_rate,
};

static bool dwt_try_enable(void)
{
	/*
	 * Enable trace unit access required by DWT. This is architecture
	 * standard for Cortex-M where CYCCNT lives in DWT.
	 */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	/*
	 * Unlock DWT if a Lock Access Register is present.
	 */
#ifdef DWT_LAR
	DWT->LAR = DWT_UNLOCK_KEY;
#endif

	/*
	 * Enable the cycle counter.
	 */
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	/*
	 * Sanity-check that CYCCNT increments.
	 */
	uint32_t c0 = DWT->CYCCNT;

	for (volatile int i = 0; i < 1000; i++) {
		arch_nop();
	}

	uint32_t c1 = DWT->CYCCNT;

	return (c1 != c0);
}

static int delay_switch_to_late_post_init(void)
{
	/*
	 * Switch delay backend to the kernel-aware implementation after
	 * the system initialization has completed.
	 */
	delay_us_impl = late_delay_us;

	return 0;
}

/* Enforce ordering: switch must run after system clock is initialized. */
BUILD_ASSERT(CONFIG_EM32_DELAY_SWITCH_PRIORITY > CONFIG_SYSTEM_CLOCK_INIT_PRIORITY,
	     "delay switch priority must be greater than system clock priority");

/*
 * Switch the delay backend at PRE_KERNEL_2 so that k_busy_wait() and other
 * kernel primitives are available and calibrated.
 */
SYS_INIT(delay_switch_to_late_post_init, PRE_KERNEL_2, CONFIG_EM32_DELAY_SWITCH_PRIORITY);

static int elan_em32_ahb_clock_control_init(const struct device *dev)
{
	/*
	 * Attempt to enable DWT early to provide precise busy-wait delays
	 * during clock configuration.
	 */
	g_dwt_ok = dwt_try_enable();

	/*
	 * Configure AHB frequency and update internal clock state used by
	 * the early delay path.
	 */
	int ret;

	const struct elan_em32_ahb_clock_control_config *config = dev->config;

	if (!device_is_ready(config->sysctrl_syscon) || !device_is_ready(config->clkctrl_syscon) ||
	    !device_is_ready(config->infoctrl_syscon)) {
		LOG_ERR("SYSCTRL, CLKCTRL or INFOCTRL syscon device is not ready");
		return -ENODEV;
	}

	ret = elan_em32_set_ahb_freq(dev);
	if (ret) {
		LOG_ERR("AHB clock setup failed: %d", ret);
		return ret;
	}

	return 0;
}

static const struct elan_em32_ahb_clock_control_config em32_ahb_config = {
	.sysctrl_syscon = DEVICE_DT_GET(DT_NODELABEL(sysctrl)),
	.clkctrl_syscon = DEVICE_DT_GET(DT_NODELABEL(clkctrl)),
	.infoctrl_syscon = DEVICE_DT_GET(DT_NODELABEL(infoctrl)),
	.clock_source = DT_PROP(DT_NODELABEL(clk_ahb), clock_source),
	.clock_frequency = DT_PROP(DT_NODELABEL(clk_ahb), clock_frequency),
	.clock_divider = DT_PROP(DT_NODELABEL(clk_ahb), clock_divider),
};

DEVICE_DT_DEFINE(DT_NODELABEL(clk_ahb), elan_em32_ahb_clock_control_init, NULL, NULL,
		 &em32_ahb_config, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_EM32_AHB_INIT_PRIORITY,
		 &elan_em32_ahb_clock_control_api);
