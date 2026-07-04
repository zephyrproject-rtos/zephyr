/*
 * Copyright (c) 2026 Fiona Behrens
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(clock_control_wch_rcc, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <hal_ch32fun.h>

#define WCH_RCC_CLOCK_ID_OFFSET(id) (((id) >> 5) & 0xFF)
#define WCH_RCC_CLOCK_ID_BIT(id)    ((id) & 0x1F)

#define HSI_FREQ 8 * 1000 * 1000;

struct wch_rcc_config {
	RCC_TypeDef *regs;
	uint32_t hse_freq;
	uint32_t hse_en: 1;
	uint32_t hse_bpy: 1;
	uint32_t hsi_trim: 5;
	uint32_t hsi_en: 1;
	uint32_t lse_bpy: 1;
	uint32_t lse_en: 1;
	uint32_t lsi_en: 1;
	uint32_t hsi_pre: 1;
	uint32_t hse_pre: 1;
	uint32_t pll_src: 1;
	uint32_t pll_mul: 4;
	uint32_t pll_en: 1;
	uint32_t hpre: 4;
	uint32_t ppre1: 3;
	uint32_t ppre2: 3;
	uint32_t sys_src: 2;
};

struct wch_rcc_data {
	uint32_t sys_freq;
};

static int wch_rcc_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct wch_rcc_config *config = dev->config;
	RCC_TypeDef *regs = config->regs;
	uint8_t id = (uintptr_t)sys;
	uint32_t reg = (uint32_t)(&regs->AHBPCENR + WCH_RCC_CLOCK_ID_OFFSET(id));
	uint32_t val = sys_read32(reg);

	val |= BIT(WCH_RCC_CLOCK_ID_BIT(id));
	sys_write32(val, reg);

	return 0;
}

static uint32_t wch_rcc_get_hpre_rate(uint32_t input, uint32_t div)
{
	if ((div & BIT(3)) == 0) {
		return input;
	}

	if (div < 12) {
		div = 2 << (div & ~BIT(3));
	} else {
		div = 2 << ((div & ~BIT(3)) + 1);
	}
	return input / div;
}

static uint32_t wch_rcc_get_ppre(uint32_t div)
{
	if ((div & BIT(2)) == 0) {
		return 1;
	}
	return 2 << (div & ~BIT(2));
}

static int wch_rcc_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	const struct wch_rcc_config *config = dev->config;
	struct wch_rcc_data *data = dev->data;
	uint32_t id = (uintptr_t)sys;

	if (data->sys_freq == 0) {
		return -EAGAIN;
	}

	uint32_t input = wch_rcc_get_hpre_rate(data->sys_freq, config->hpre);

	switch (WCH_RCC_CLOCK_ID_OFFSET(id)) {
	case 0:
		*rate = input;
		return 0;
	case 1:
		*rate = input / wch_rcc_get_ppre(config->ppre2);
		return 0;
	case 2:
		*rate = input / wch_rcc_get_ppre(config->ppre1);
		return 0;
	default:
		return -EINVAL;
	}
}

static int wch_rcc_configure(const struct device *dev, clock_control_subsys_t sys, void *data)
{
	const struct wch_rcc_config *config = dev->config;
	uint32_t id = (uint32_t)sys;

	/* MCO config */
	if ((id & BIT(31)) != 0) {
		config->regs->CFGR0 = (config->regs->CFGR0 & ~RCC_CFGR0_MCO) |
				      FIELD_PREP(RCC_CFGR0_MCO, (uint32_t)data);
		return 0;
	}

	return -EINVAL;
}

static DEVICE_API(clock_control, wch_rcc_api) = {
	.on = wch_rcc_on,
	.get_rate = wch_rcc_get_rate,
	.configure = wch_rcc_configure,
};

static void clock_control_wch_rcc_setup_flash(void)
{
#if defined(FLASH_ACTLR_LATENCY)
	uint32_t latency;

#if defined(CONFIG_SOC_CH32V003)
	if (WCH_RCC_SYSCLK <= 24000000) {
		latency = FLASH_ACTLR_LATENCY_0;
	} else {
		latency = FLASH_ACTLR_LATENCY_1;
	}
#elif defined(CONFIG_SOC_CH32V006)
	if (WCH_RCC_SYSCLK <= 15000000) {
		latency = FLASH_ACTLR_LATENCY_0;
	} else if (WCH_RCC_SYSCLK <= 24000000) {
		latency = FLASH_ACTLR_LATENCY_1;
	} else {
		latency = FLASH_ACTLR_LATENCY_2;
	}
#else
#error Unrecognised SOC family
#endif
	FLASH->ACTLR = (FLASH->ACTLR & ~FLASH_ACTLR_LATENCY) | latency;
#endif
}

static void wch_rcc_set_hse(const struct device *dev)
{
	const struct wch_rcc_config *config = dev->config;

	config->regs->CTLR &= ~RCC_HSEON;

	config->regs->CTLR = (config->regs->CTLR & ~(CTLR_HSEBYP_Set)) |
			     FIELD_PREP(CTLR_HSEBYP_Set, config->hse_bpy);

	if (config->hse_en == 1) {
		config->regs->CTLR |= RCC_HSEON;
		while ((config->regs->CTLR & RCC_HSERDY) == 0) {
		}
	}
}

static void wch_rcc_set_lse(const struct device *dev)
{
	const struct wch_rcc_config *config = dev->config;

	config->regs->BDCTLR = (config->regs->BDCTLR & ~(RCC_LSEBYP | RCC_LSEON)) |
			       FIELD_PREP(RCC_LSEBYP, config->lse_bpy) |
			       FIELD_PREP(RCC_LSEON, config->lse_en);

	if (config->lse_en == 1) {
		while ((config->regs->BDCTLR & RCC_LSERDY) == 0) {
		}
	}

	if (config->lsi_en == 1) {
		config->regs->RSTSCKR |= RCC_LSION;
		while ((config->regs->RSTSCKR & RCC_LSIRDY) == 0) {
		}
	} else {
		config->regs->RSTSCKR &= ~RCC_LSION;
	}
}

static void wch_rcc_set_pll(const struct device *dev)
{
	const struct wch_rcc_config *config = dev->config;

	config->regs->CTLR &= ~RCC_PLLON;

	EXTEN->EXTEN_CTR = (EXTEN->EXTEN_CTR & ~EXTEN_PLL_HSI_PRE) |
			   FIELD_PREP(EXTEN_PLL_HSI_PRE, config->hsi_pre);

	config->regs->CFGR0 = (config->regs->CFGR0 &
			       ~(CFGR0_PLLXTPRE_Mask | CFGR0_PLLSRC_Mask | CFGR0_PLLMull_Mask)) |
			      FIELD_PREP(CFGR0_PLLXTPRE_Mask, config->hse_pre) |
			      FIELD_PREP(CFGR0_PLLSRC_Mask, config->pll_src) |
			      FIELD_PREP(CFGR0_PLLMull_Mask, config->pll_mul);

	if (config->pll_en == 0) {
		return;
	}

	config->regs->CTLR |= RCC_PLLON;
	while ((config->regs->CTLR & RCC_PLLRDY) == 0) {
	}
}

static uint32_t wch_rcc_get_pll_freq(const struct device *dev)
{
	const struct wch_rcc_config *config = dev->config;
	uint32_t cur = 0;

	if (config->pll_src == 0) {
		cur = HSI_FREQ;
		if (config->hse_pre == 0) {
			cur = cur / 2;
		}
	} else {
		cur = config->hse_freq / (config->hse_pre + 1);
	}

	if (config->pll_mul != 0xf) {
		cur = cur * (config->pll_mul + 2);
	} else {
		cur = cur * 18;
	}

	return cur;
}

static void wch_rcc_set_hbclk(const struct device *dev)
{
	const struct wch_rcc_config *config = dev->config;
	uint32_t reg = config->regs->CFGR0;

	reg |= FIELD_PREP(CFGR0_HPRE_Set_Mask, config->hpre);
	reg |= FIELD_PREP(CFGR0_PPRE1_Set_Mask, config->ppre1) |
	       FIELD_PREP(CFGR0_PPRE2_Set_Mask, config->ppre2);
	config->regs->CFGR0 = reg;
}

static int wch_rcc_init(const struct device *dev)
{
	const struct wch_rcc_config *config = dev->config;
	struct wch_rcc_data *data = dev->data;

	clock_control_wch_rcc_setup_flash();

	/* Set HSI trim */
	config->regs->CTLR = (config->regs->CTLR & ~CTLR_HSITRIM_Mask) |
			     FIELD_PREP(CTLR_HSITRIM_Mask, config->hsi_trim);

	wch_rcc_set_hse(dev);
	wch_rcc_set_lse(dev);

	wch_rcc_set_pll(dev);

	/* Select system clock */
	/* CFGR0_SWS_MSK broken in hal, using hardcoded 3 */
	config->regs->CFGR0 = (config->regs->CFGR0 & ~3) | FIELD_PREP(3, config->sys_src);
	switch (config->sys_src) {
	default:
	case 0:
		data->sys_freq = HSI_FREQ;
		break;
	case 1:
		data->sys_freq = config->hse_freq;
		break;
	case 2:
		data->sys_freq = wch_rcc_get_pll_freq(dev);
		break;
	}

	/* Disable HSI if requested */
	config->regs->CFGR0 =
		(config->regs->CFGR0 & ~RCC_HSION) | FIELD_PREP(RCC_HSION, config->hsi_en);

	wch_rcc_set_hbclk(dev);

	return 0;
}

#define DT_GET_PPRE(n)                                                                             \
	((DT_ENUM_IDX(DT_NODELABEL(hclk), pclk##n##_div) == 0)                                     \
		 ? 0                                                                               \
		 : BIT(2) | (DT_ENUM_IDX(DT_NODELABEL(hclk), pclk##n##_div) - 1))

#define DT_GET_HPRE                                                                                \
	((DT_ENUM_IDX(DT_NODELABEL(hclk), prescaler) == 0)                                         \
		 ? 0                                                                               \
		 : BIT(3) | (DT_ENUM_IDX(DT_NODELABEL(hclk), prescaler) - 1))

#if DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(pll), 0), DT_NODELABEL(clk_hsi))
#define PLL_SRC 0
#elif DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(pll), 0), DT_NODELABEL(clk_hse))
#define PLL_SRC 1
#else
#error "Invalid PLL clock source"
#endif

#if DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(hclk), 0), DT_NODELABEL(clk_hsi))
#define SYS_SRC 0
#elif DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(hclk), 0), DT_NODELABEL(clk_hse))
#define SYS_SRC 1
#elif DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(hclk), 0), DT_NODELABEL(pll))
#define SYS_SRC 2
#else
#error "Invalid sys clock source"
#endif

#define WCH_CH32_203_RCC_DEFINE(node_id)                                                           \
	static const struct wch_rcc_config wch_rcc_config_##node_id = {                            \
		.regs = (RCC_TypeDef *)DT_REG_ADDR(node_id),                                       \
		.hse_freq = DT_PROP_OR(DT_NODELABEL(clk_hse), clock_frequency, 0),                 \
		.hse_en = DT_NODE_HAS_STATUS(DT_NODELABEL(clk_hse), okay),                         \
		.hse_bpy = DT_PROP(DT_NODELABEL(clk_hse), hse_bypass),                             \
		.hsi_trim = DT_PROP_OR(DT_NODELABEL(clk_hsi), trim, 16),                           \
		.hsi_en = DT_NODE_HAS_STATUS(DT_NODELABEL(pll), okay),                             \
		.lse_bpy = DT_PROP(DT_NODELABEL(clk_lse), lse_bypass),                             \
		.lse_en = DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_lse)),                          \
		.lsi_en = DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_lsi)),                          \
		.hsi_pre = DT_ENUM_IDX(DT_NODELABEL(pll), hsi_pre),                                \
		.hse_pre = DT_ENUM_IDX(DT_NODELABEL(pll), hse_pre),                                \
		.pll_src = PLL_SRC,                                                                \
		.pll_mul = DT_ENUM_IDX(DT_NODELABEL(pll), mul),                                    \
		.pll_en = DT_NODE_HAS_STATUS(DT_NODELABEL(pll), okay),                             \
		.hpre = DT_GET_HPRE,                                                               \
		.ppre1 = DT_GET_PPRE(1),                                                           \
		.ppre2 = DT_GET_PPRE(2),                                                           \
		.sys_src = SYS_SRC,                                                                \
	};                                                                                         \
                                                                                                   \
	static struct wch_rcc_data wch_rcc_data_##node_id = {                                      \
		.sys_freq = 0,                                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, wch_rcc_init, NULL, &wch_rcc_data_##node_id,                     \
			 &wch_rcc_config_##node_id, PRE_KERNEL_1,                                  \
			 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &wch_rcc_api);

DT_FOREACH_STATUS_OKAY(wch_ch32_203_rcc, WCH_CH32_203_RCC_DEFINE);
