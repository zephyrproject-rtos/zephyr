/* Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc84x_clock

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/clock/lpc84x-clock.h>

#include <fsl_clock.h>
#include <fsl_power.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(clock_control_lpc84x, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static const clock_fro_osc_freq_t lpc84x_fro_freq_map[] = {
	kCLOCK_FroOscOut18M,
	kCLOCK_FroOscOut24M,
	kCLOCK_FroOscOut30M,
};

static const clock_main_clk_src_t lpc84x_mainclk_src_map[] = {
	kCLOCK_MainClkSrcFro,
	kCLOCK_MainClkSrcExtClk,
	kCLOCK_MainClkSrcWdtOsc,
	kCLOCK_MainClkSrcFroDiv,
};

struct lpc84x_clock_config {
	DEVICE_MMIO_ROM;
};

struct lpc84x_clock_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
};

/**
 * @name FCLK Mux Instance Indices
 * @brief Selection indices for SYSCON->FCLKSEL[n] registers
 * @{
 */
#define LPC84X_FCLK_UART0 0U  /**< FCLK index for UART0 */
#define LPC84X_FCLK_UART1 1U  /**< FCLK index for UART1 */
#define LPC84X_FCLK_UART2 2U  /**< FCLK index for UART2 */
#define LPC84X_FCLK_UART3 3U  /**< FCLK index for UART3 */
#define LPC84X_FCLK_UART4 4U  /**< FCLK index for UART4 */
#define LPC84X_FCLK_I2C0  5U  /**< FCLK index for I2C0 */
#define LPC84X_FCLK_I2C1  6U  /**< FCLK index for I2C1 */
#define LPC84X_FCLK_I2C2  7U  /**< FCLK index for I2C2 */
#define LPC84X_FCLK_I2C3  8U  /**< FCLK index for I2C3 */
#define LPC84X_FCLK_SPI0  9U  /**< FCLK index for SPI0 */
#define LPC84X_FCLK_SPI1  10U /**< FCLK index for SPI1 */

/**
 * @name FCLK Source Selection
 * @brief Bit values for clock source selection (Ref Manual: Table 184)
 * @{
 */
#define LPC84X_FCLK_SRC_FRO      0U /**< FRO oscillator */
#define LPC84X_FCLK_SRC_MAIN_CLK 1U /**< Main system clock */
#define LPC84X_FCLK_SRC_FRG0     2U /**< Fractional Rate Generator 0 */
#define LPC84X_FCLK_SRC_FRG1     3U /**< Fractional Rate Generator 1 */
#define LPC84X_FCLK_SRC_FRO_DIV  4U /**< FRO divided by 2 */

static uint32_t lpc84x_get_fclk_freq(uint32_t fclk_inst)
{
	uint32_t freq = 0U;

	uint32_t sel = SYSCON->FCLKSEL[fclk_inst] & SYSCON_FCLKSEL_SEL_MASK;

	switch (sel) {
	case LPC84X_FCLK_SRC_FRO:
		freq = CLOCK_GetFroFreq();
		break;
	case LPC84X_FCLK_SRC_MAIN_CLK:
		freq = CLOCK_GetMainClkFreq();
		break;
	case LPC84X_FCLK_SRC_FRG0:
		freq = CLOCK_GetFRG0ClkFreq();
		break;
	case LPC84X_FCLK_SRC_FRG1:
		freq = CLOCK_GetFRG1ClkFreq();
		break;
	case LPC84X_FCLK_SRC_FRO_DIV:
		freq = CLOCK_GetFroFreq() >> 1U;
		break;
	default:
		freq = 0U;
		break;
	}

	return freq;
}

#define PERIPH_CLK_MUX_SEL(n) DT_ENUM_IDX_OR(n, clock_source, 0)

#define PERIPH_CLK_SELECT(label, fclk_inst)                                                        \
	do {                                                                                       \
		if (DT_NODE_HAS_STATUS(DT_NODELABEL(label), okay)) {                               \
			CLOCK_Select(CLK_MUX_DEFINE(FCLKSEL[fclk_inst],                            \
						    PERIPH_CLK_MUX_SEL(DT_NODELABEL(label))));     \
		}                                                                                  \
	} while (0)

static int lpc84x_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct lpc84x_clock_data *data = dev->data;
	uint32_t clk_id = (uint32_t)sub_system;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (clk_id) {
	/* Peripherals with FCLK muxing */
	case LPC84X_CLK_UART0:
		PERIPH_CLK_SELECT(uart0, LPC84X_FCLK_UART0);
		break;
	case LPC84X_CLK_UART1:
		PERIPH_CLK_SELECT(uart1, LPC84X_FCLK_UART1);
		break;
	case LPC84X_CLK_UART2:
		PERIPH_CLK_SELECT(uart2, LPC84X_FCLK_UART2);
		break;
	case LPC84X_CLK_UART3:
		PERIPH_CLK_SELECT(uart3, LPC84X_FCLK_UART3);
		break;
	case LPC84X_CLK_UART4:
		PERIPH_CLK_SELECT(uart4, LPC84X_FCLK_UART4);
		break;
	case LPC84X_CLK_I2C0:
		PERIPH_CLK_SELECT(i2c0, LPC84X_FCLK_I2C0);
		break;
	case LPC84X_CLK_I2C1:
		PERIPH_CLK_SELECT(i2c1, LPC84X_FCLK_I2C1);
		break;
	case LPC84X_CLK_I2C2:
		PERIPH_CLK_SELECT(i2c2, LPC84X_FCLK_I2C2);
		break;
	case LPC84X_CLK_I2C3:
		PERIPH_CLK_SELECT(i2c3, LPC84X_FCLK_I2C3);
		break;
	case LPC84X_CLK_SPI0:
		PERIPH_CLK_SELECT(spi0, LPC84X_FCLK_SPI0);
		break;
	case LPC84X_CLK_SPI1:
		PERIPH_CLK_SELECT(spi1, LPC84X_FCLK_SPI1);
		break;

	/* Peripherals without FCLK muxing */
	case LPC84X_CLK_GPIO0:
	case LPC84X_CLK_GPIO1:
	case LPC84X_CLK_GPIOINT:
	case LPC84X_CLK_SWM:
	case LPC84X_CLK_IOCON:
		break;

	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0) {
		CLOCK_EnableClock((clock_ip_name_t)clk_id);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int lpc84x_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct lpc84x_clock_data *data = dev->data;
	uint32_t clk_id = (uint32_t)sub_system;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (clk_id) {
	case LPC84X_CLK_GPIO0:
	case LPC84X_CLK_GPIO1:
	case LPC84X_CLK_GPIOINT:
	case LPC84X_CLK_SWM:
	case LPC84X_CLK_IOCON:
	case LPC84X_CLK_UART0:
	case LPC84X_CLK_UART1:
	case LPC84X_CLK_UART2:
	case LPC84X_CLK_UART3:
	case LPC84X_CLK_UART4:
	case LPC84X_CLK_I2C0:
	case LPC84X_CLK_I2C1:
	case LPC84X_CLK_I2C2:
	case LPC84X_CLK_I2C3:
	case LPC84X_CLK_SPI0:
	case LPC84X_CLK_SPI1:
		CLOCK_DisableClock((clock_ip_name_t)clk_id);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int lpc84x_clock_control_get_rate(const struct device *dev,
					 clock_control_subsys_t sub_system, uint32_t *rate)
{
	ARG_UNUSED(dev);

	if (rate == NULL) {
		return -EINVAL;
	}

	uint32_t clock_id = (clock_ip_name_t)sub_system;

	switch (clock_id) {
	case LPC84X_CLK_UART0:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_UART0);
		break;
	case LPC84X_CLK_UART1:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_UART1);
		break;
	case LPC84X_CLK_UART2:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_UART2);
		break;
	case LPC84X_CLK_UART3:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_UART3);
		break;
	case LPC84X_CLK_UART4:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_UART4);
		break;
	case LPC84X_CLK_I2C0:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_I2C0);
		break;
	case LPC84X_CLK_I2C1:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_I2C1);
		break;
	case LPC84X_CLK_I2C2:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_I2C2);
		break;
	case LPC84X_CLK_I2C3:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_I2C3);
		break;
	case LPC84X_CLK_SPI0:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_SPI0);
		break;
	case LPC84X_CLK_SPI1:
		*rate = lpc84x_get_fclk_freq(LPC84X_FCLK_SPI1);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(clock_control, lpc84x_clock_api) = {
	.on = lpc84x_clock_control_on,
	.off = lpc84x_clock_control_off,
	.get_rate = lpc84x_clock_control_get_rate,
};

#define LPC84X_CLOCK_CONTROL_INIT(inst)                                                            \
	static int lpc84x_clock_init_##inst(const struct device *dev)                              \
	{                                                                                          \
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);                                            \
		struct lpc84x_clock_data *data = dev->data;                                        \
		SYSCON_Type *base = (SYSCON_Type *)DEVICE_MMIO_GET(dev);                           \
		k_mutex_init(&data->lock);                                                         \
                                                                                                   \
		/* ---- FRO Initialization ---- */                                                 \
		if (DT_INST_PROP(inst, enable_fro)) {                                              \
			POWER_DisablePD(kPDRUNCFG_PD_FRO);                                         \
                                                                                                   \
			uint32_t idx = DT_INST_ENUM_IDX(inst, fro_freq);                           \
			if (idx >= ARRAY_SIZE(lpc84x_fro_freq_map)) {                              \
				return -EINVAL;                                                    \
			}                                                                          \
                                                                                                   \
			CLOCK_SetFroOscFreq(lpc84x_fro_freq_map[idx]);                             \
                                                                                                   \
			if (DT_INST_PROP(inst, fro_low_power_boot)) {                              \
				CLOCK_SetFroOutClkSrc(kCLOCK_FroSrcLpwrBootValue);                 \
			} else {                                                                   \
				CLOCK_SetFroOutClkSrc(kCLOCK_FroSrcFroOsc);                        \
			}                                                                          \
		}                                                                                  \
		/* ---- FRO OUT Initialization ---- */                                             \
		IF_ENABLED(DT_INST_PROP(inst, enable_froout), (					\
					POWER_DisablePD(kPDRUNCFG_PD_FRO);			\
					POWER_DisablePD(kPDRUNCFG_PD_FRO_OUT);			\
					))                              \
                                                                                                   \
		/* ---- System Oscillator Initialization ---- */                                   \
		IF_ENABLED(DT_INST_PROP(inst, enable_sysosc), (			\
			BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, sysosc_freq),	\
				"sysosc-freq is required when enable-sysosc is set"); \
			CLOCK_InitSysOsc(DT_INST_PROP(inst, sysosc_freq));	\
		))                                \
                                                                                                   \
		/* ---- External Clock source choice ---- */                                       \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, extclk_src), (		\
			COND_CODE_1(IS_EQ(DT_INST_ENUM_IDX(inst, extclk_src), 0), ( \
				/* sysosc: handled by InitSysOsc if enabled */	\
				base->EXTCLKSEL &= ~SYSCON_EXTCLKSEL_SEL_MASK; \
			), (							\
				/* clkin: needs extclk-freq */			\
				BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, extclk_freq), \
					"extclk-freq required for clkin");	\
				CLOCK_InitExtClkin(DT_INST_PROP(inst, extclk_freq)); \
			));							\
		))                           \
                                                                                                   \
		/* ---- Main Clock selection ---- */                                               \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, mainclk_src), (		\
			CLOCK_SetMainClkSrc(					\
				lpc84x_mainclk_src_map[DT_INST_ENUM_IDX(inst, mainclk_src)]); \
		))                          \
                                                                                                   \
		/* ---- FRG0 Initialization ---- */                                                \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, frg0_clk_src), ( \
			base->FRG[0].FRGCLKSEL = DT_INST_ENUM_IDX(inst, frg0_clk_src); \
			IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, frg0_mult), ( \
				base->FRG[0].FRGDIV = 0xFF; \
				base->FRG[0].FRGMULT = DT_INST_PROP(inst, frg0_mult); \
			)) \
		))                          \
                                                                                                   \
		/* ---- FRG1 Initialization ---- */                                                \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, frg1_clk_src), ( \
			base->FRG[1].FRGCLKSEL = DT_INST_ENUM_IDX(inst, frg1_clk_src); \
			IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, frg1_mult), ( \
				base->FRG[1].FRGDIV = 0xFF; \
				base->FRG[1].FRGMULT = DT_INST_PROP(inst, frg1_mult); \
			)) \
		))                          \
                                                                                                   \
		CLOCK_SetCoreSysClkDiv(DT_INST_PROP(inst, ahb_clk_divider));                       \
                                                                                                   \
		CLOCK_SetFLASHAccessCyclesForFreq(CLOCK_GetMainClkFreq());                         \
		SystemCoreClockUpdate();                                                           \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	static const struct lpc84x_clock_config cfg_##inst = {                                     \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
	};                                                                                         \
                                                                                                   \
	static struct lpc84x_clock_data data_##inst;                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, lpc84x_clock_init_##inst, NULL, &data_##inst, &cfg_##inst,     \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                    \
			      &lpc84x_clock_api);

DT_INST_FOREACH_STATUS_OKAY(LPC84X_CLOCK_CONTROL_INIT)
