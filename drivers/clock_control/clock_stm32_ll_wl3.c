/*
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2026 Anders Frandsen <anfran@anfran.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

/* Driver definitions */
#define RCC_REG(offset) (DT_REG_ADDR(STM32_CLOCK_CONTROL_NODE) + (offset))

/* RF HSE capacitor bank tuning. */
#define STM32_WL3_HSE_CAPACITOR_TUNE	32

/* Device tree node definitions */
#define DT_RCC_SLOWCLK_NODE	DT_PHANDLE(STM32_CLOCK_CONTROL_NODE, slow_clock)

/* Device tree properties definitions */
#define STM32_WL3_CLKSYS_PRESCALER		\
	DT_PROP(STM32_CLOCK_CONTROL_NODE, clksys_prescaler)

#if DT_NODE_HAS_PROP(STM32_CLOCK_CONTROL_NODE, slow_clock)

#	if !DT_NODE_HAS_STATUS_OKAY(DT_RCC_SLOWCLK_NODE)
#		error slow-clock source is not enabled
#	endif

#	if DT_SAME_NODE(DT_RCC_SLOWCLK_NODE, DT_NODELABEL(clk_lsi))
#		define STM32_WL3_SLOWCLK_SRC	LL_RCC_LSCO_CLKSOURCE_LSI
#	elif DT_SAME_NODE(DT_RCC_SLOWCLK_NODE, DT_NODELABEL(clk_lse))
#		define STM32_WL3_SLOWCLK_SRC	LL_RCC_LSCO_CLKSOURCE_LSE
#	elif DT_SAME_NODE(DT_RCC_SLOWCLK_NODE, DT_NODELABEL(clk_16mhz_div512))
#		define STM32_WL3_SLOWCLK_SRC	LL_RCC_LSCO_CLKSOURCE_HSI64M_DIV2048
#	else
#		error Invalid device selected as slow-clock
#	endif

#endif /* DT_NODE_HAS_PROP(STM32_CLOCK_CONTROL_NODE, slow_clock) */

/* Verify device tree properties are correct */
#if defined(STM32_SYSCLK_SRC_HSE)
#define STM32_WL3_CLKROOT_FREQ	STM32_HSE_FREQ
#define LL_PRESCALER(x)		_CONCAT(LL_RCC_DIRECT_HSE_DIV_, x)
#elif defined(STM32_SYSCLK_SRC_HSI) || defined(STM32_SYSCLK_SRC_PLL)
#define STM32_WL3_CLKROOT_FREQ	STM32_HSI_FREQ
#define LL_PRESCALER(x)		_CONCAT(LL_RCC_RC64MPLL_DIV_, x)
BUILD_ASSERT(IS_POWER_OF_TWO(STM32_WL3_CLKSYS_PRESCALER),
	"clksys-prescaler must be a power of two when SYSCLK source is the RC64MPLL block");
#else
#error Invalid device selected as SYSCLK source: use clk_hsi, pll or clk_hse
#endif

BUILD_ASSERT(STM32_HCLK_FREQUENCY * STM32_WL3_CLKSYS_PRESCALER == STM32_WL3_CLKROOT_FREQ,
	"clock-frequency must be the SYSCLK source frequency divided by clksys-prescaler");

#if defined(STM32_SYSCLK_SRC_PLL) || defined(STM32_SYSCLK_SRC_HSE)
BUILD_ASSERT(IS_ENABLED(STM32_HSE_ENABLED),
	"STM32WL3 PLL and Direct HSE modes require HSE to be enabled");
#endif

#if defined(STM32_SYSCLK_SRC_PLL)
/* The RC64MPLL PLL output is HSE x 4/3 (RM0511 section 6.2) */
BUILD_ASSERT(DT_PROP(DT_NODELABEL(pll), clock_frequency) == (STM32_HSE_FREQ * 4U / 3U),
	"pll clock-frequency must be the HSE frequency multiplied by 4/3");
#endif

/** @brief Verifies if provided domain clock is currently active */
static int enabled_clock(uint32_t src_clk)
{
	switch (src_clk) {
	case STM32_SRC_SYSCLK:
	case STM32_SRC_CLKSLOWMUX:
	case STM32_SRC_CLK16MHZ:
	case STM32_SRC_CLKSYS:
	case STM32_SRC_CLKROOT_DIV2:
		return 0;
	case STM32_SRC_LSE:
		if (!IS_ENABLED(STM32_LSE_ENABLED)) {
			return -ENOTSUP;
		}
		return 0;
	case STM32_SRC_LSI:
		if (!IS_ENABLED(STM32_LSI_ENABLED)) {
			return -ENOTSUP;
		}
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int stm32_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	const mem_addr_t reg = RCC_REG(pclken->bus);
	volatile uint32_t temp;

	ARG_UNUSED(dev);
	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempting to change domain clock */
		return -ENOTSUP;
	}

	sys_set_bits(reg, pclken->enr);

	/* Read back register to be blocked by RCC
	 * until peripheral clock enabling is complete
	 */
	temp = sys_read32(reg);
	UNUSED(temp);

	return 0;
}

static int stm32_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	const mem_addr_t reg = RCC_REG(pclken->bus);

	ARG_UNUSED(dev);
	if (!IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Attempting to change domain clock */
		return -ENOTSUP;
	}

	sys_clear_bits(reg, pclken->enr);

	return 0;
}

static int stm32_clock_control_configure(const struct device *dev,
					 clock_control_subsys_t sub_system,
					 void *data)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;
	uint32_t enr = pclken->enr;
	uint32_t reg = STM32_DT_CLKSEL_REG_GET(enr);
	uint32_t shift = STM32_DT_CLKSEL_SHIFT_GET(enr);
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	err = enabled_clock(pclken->bus);
	if (err < 0) {
		/* Attempting to configure an unavailable or invalid clock */
		return err;
	}

	if (pclken->enr == NO_SEL) {
		/* Domain clock is fixed. Nothing to set. Exit */
		return 0;
	}

	stm32_reg_modify_bits((uint32_t *)(DT_REG_ADDR(DT_NODELABEL(rcc)) + reg),
			      STM32_DT_CLKSEL_MASK_GET(enr) << shift,
			      STM32_DT_CLKSEL_VAL_GET(enr) << shift);

	return 0;
}

/** @brief Returns the CLK_ROOT frequency. */
static uint32_t get_clk_root_freq(void)
{
	return LL_RCC_DIRECT_HSE_IsEnabled() ? STM32_HSE_FREQ : STM32_HSI_FREQ;
}

/** @brief Returns the CLK_SYS frequency (CPU/AHB/APB) by reading RCC_CFGR->CLKSYSDIV */
static uint32_t get_clk_sys_freq(void)
{
	/* Both LL prescaler getters return the raw CLKSYSDIV field, still shifted */
	uint32_t clksysdiv = LL_RCC_GetRC64MPLLPrescaler() >> RCC_CFGR_CLKSYSDIV_Pos;

	if (LL_RCC_DIRECT_HSE_IsEnabled()) {
		/* DIRECT_HSE dividers are not powers of two */
		static const uint8_t hse_div[] = {1, 2, 3, 6, 12, 24, 48};

		if (clksysdiv >= ARRAY_SIZE(hse_div)) {
			return 0;
		}

		return STM32_HSE_FREQ / hse_div[clksysdiv];
	}

	return STM32_HSI_FREQ >> clksysdiv;
}

/** @brief Returns the slow-clock frequency (RTC/WDG/LCSC/LCDC) by reading RCC_CFGR->CLKSLOWSEL */
static uint32_t get_slow_clk_freq(void)
{
	switch (LL_RCC_LSCO_GetSource()) {
	case LL_RCC_LSCO_CLKSOURCE_LSE:
		return STM32_LSE_FREQ;
	case LL_RCC_LSCO_CLKSOURCE_LSI:
		return STM32_LSI_FREQ;
	case LL_RCC_LSCO_CLKSOURCE_HSI64M_DIV2048:
		return STM32_HSI_FREQ / 2048U;
	default:
		return 0;
	}
}

static int stm32_clock_control_get_subsys_rate(const struct device *dev,
					       clock_control_subsys_t sub_system,
					       uint32_t *rate)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	switch (pclken->bus) {
	case STM32_SRC_SYSCLK:
		*rate = get_clk_root_freq();
		break;
	case STM32_SRC_LSE:
		*rate = STM32_LSE_FREQ;
		break;
	case STM32_SRC_LSI:
		*rate = STM32_LSI_FREQ;
		break;
	case STM32_SRC_CLKSLOWMUX:
		*rate = get_slow_clk_freq();
		break;
	case STM32_SRC_CLK16MHZ:
		*rate = MHZ(16);
		break;
	case STM32_SRC_CLKSYS:
		*rate = get_clk_sys_freq();
		break;
	case STM32_SRC_CLKROOT_DIV2:
		*rate = get_clk_root_freq() / 2;
		break;
	case STM32_CLOCK_BUS_AHB0:
	case STM32_CLOCK_BUS_APB0:
	case STM32_CLOCK_BUS_APB1:
		*rate = get_clk_sys_freq();
		break;
	/* APB2 (radio subsystem) is not supported yet. */
	default:
		return -ENOTSUP;
	}

	if (pclken->div) {
		*rate /= (pclken->div + 1);
	}

	return 0;
}

static enum clock_control_status stm32_clock_control_get_status(const struct device *dev,
								clock_control_subsys_t sub_system)
{
	struct stm32_pclken *pclken = (struct stm32_pclken *)sub_system;

	ARG_UNUSED(dev);

	if (IN_RANGE(pclken->bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
		/* Bus / gated clock */
		if ((sys_read32(RCC_REG(pclken->bus)) & pclken->enr) == pclken->enr) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	} else {
		/* Domain clock */
		if (enabled_clock(pclken->bus) == 0) {
			return CLOCK_CONTROL_STATUS_ON;
		} else {
			return CLOCK_CONTROL_STATUS_OFF;
		}
	}
}

static DEVICE_API(clock_control, stm32_clock_control_api) = {
	.on = stm32_clock_control_on,
	.off = stm32_clock_control_off,
	.get_rate = stm32_clock_control_get_subsys_rate,
	.get_status = stm32_clock_control_get_status,
	.configure = stm32_clock_control_configure,
};

static void set_up_fixed_clock_sources(void)
{
	if (IS_ENABLED(STM32_HSE_ENABLED)) {
		/* Crystal oscillator settings. These match the HSE setup
		 * set by HAL_RCC_OscConfig() in stm32wl3x_hal_rcc.c.
		 */
		LL_RCC_HSE_SetCapacitorTuning(STM32_WL3_HSE_CAPACITOR_TUNE);
		LL_RCC_HSE_SetStartupCurrent(0);
		LL_RCC_HSE_SetAmplitudeThreshold(0);
		LL_RCC_HSE_SetCurrentControl(40);

		LL_RCC_HSE_Enable();
		while (!LL_RCC_HSE_IsReady()) {
			/* Wait for HSE ready */
		}
	}

	if (IS_ENABLED(STM32_LSI_ENABLED)) {
		LL_RCC_LSI_Enable();
		while (!LL_RCC_LSI_IsReady()) {
			/* Wait for LSI ready */
		}
	}

	if (IS_ENABLED(STM32_LSE_ENABLED)) {
#if STM32_LSE_DRIVING
		/* Configure driving capability */
		LL_RCC_LSE_SetDriveCapability(STM32_LSE_DRIVING << RCC_CSSWCR_LSEDRV_Pos);
#endif
		/* Unconditionally disable pull-up & pull-down on LSE pins */
		LL_PWR_SetNoPullB(LL_PWR_GPIO_BIT_12 | LL_PWR_GPIO_BIT_13);

		if (IS_ENABLED(STM32_LSE_BYPASS)) {
			/* Configure LSE bypass */
			LL_RCC_LSE_EnableBypass();
		}

		/* Enable LSE Oscillator (32.768 kHz) */
		LL_RCC_LSE_Enable();
		while (!LL_RCC_LSE_IsReady()) {
			/* Wait for LSE ready */
		}
	}
}

static int stm32_clock_control_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Set flash latency according to target CLK_SYS frequency:
	 *  - 1 wait state when CLK_SYS > 32MHz (i.e., 64MHz configuration)
	 *  - 0 wait states otherwise (CLK_SYS <= 32MHz)
	 */
	if (STM32_HCLK_FREQUENCY > MHZ(32)) {
		LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
	} else {
		LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
	}

	/* Enable SYSCFG clock. */
	LL_APB0_GRP1_EnableClock(LL_APB0_GRP1_PERIPH_SYSCFG);

	/* Set up individual enabled clocks */
	set_up_fixed_clock_sources();

	/* Set up the slow clock mux */
#if defined(STM32_WL3_SLOWCLK_SRC)
	LL_RCC_LSCO_SetSource(STM32_WL3_SLOWCLK_SRC);
#endif

#if defined(STM32_SYSCLK_SRC_HSE)
	/* Set HSE prescaler */
	LL_RCC_SetDirectHSEPrescaler(LL_PRESCALER(STM32_WL3_CLKSYS_PRESCALER));

	/* Select Direct HSE as SYSCLK source */
	LL_RCC_DIRECT_HSE_Enable();

	while (!LL_RCC_DIRECT_HSE_IsEnabled()) {
		/* Wait until Direct HSE is ready */
	}
#else
	if (IS_ENABLED(STM32_SYSCLK_SRC_PLL)) {
		/* Turn on the PLL part of RC64MPLL block */
		LL_RCC_RC64MPLL_Enable();
		while (!LL_RCC_RC64MPLL_IsReady()) {
			/* Wait until PLL is locked */
		}
	} else {
		/* Leave the RC64MPLL block free-running on its internal RC */
		LL_RCC_RC64MPLL_Disable();
	}

	LL_RCC_SetRC64MPLLPrescaler(LL_PRESCALER(STM32_WL3_CLKSYS_PRESCALER));

	/* Select the RC64MPLL block as SYSCLK source */
	LL_RCC_DIRECT_HSE_Disable();

	while (LL_RCC_DIRECT_HSE_IsEnabled()) {
		/* Wait until the switch away from Direct HSE has completed */
	}
#endif /* STM32_SYSCLK_SRC_HSE */

	if (STM32_HCLK_FREQUENCY <= MHZ(32)) {
		LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
	}

	SystemCoreClock = STM32_HCLK_FREQUENCY;

	return 0;
}

DEVICE_DT_DEFINE(STM32_CLOCK_CONTROL_NODE,
		 &stm32_clock_control_init,
		 NULL, NULL, NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &stm32_clock_control_api);
