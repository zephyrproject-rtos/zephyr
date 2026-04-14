/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 * Copyright (c) 2026 William Markezana
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <zephyr/dt-bindings/pinctrl/bflb-common-pinctrl.h>

#if defined(CONFIG_SOC_SERIES_BL60X)
#include <zephyr/dt-bindings/pinctrl/bl60x-pinctrl.h>
#define UART_SIG_SLOTS	8
#elif defined(CONFIG_SOC_SERIES_BL70X)
#include <zephyr/dt-bindings/pinctrl/bl70x-pinctrl.h>
#define UART_SIG_SLOTS	8
#elif defined(CONFIG_SOC_SERIES_BL70XL)
#include <zephyr/dt-bindings/pinctrl/bl70xl-pinctrl.h>
/* BL70XL has 4 UART signal routing slots (vs 8 on BL602 andBL702) */
#define UART_SIG_SLOTS	4
/* On BL70xL, XTAL32K pins are not dedicated */
#define BL70XL_XTAL32K_PIN_XI	30
#define BL70XL_XTAL32K_PIN_XO	31
#else
#error "Unsupported Platform"
#endif

#define UART_SIG_DISABLE		0x0f
/* Per-slot mask for UART signal selection (each slot is GLB_UART_SIG_0_SEL_LEN bits) */
#define UART_SIG_SLOT_MSK		(GLB_UART_SIG_0_SEL_MSK >> GLB_UART_SIG_0_SEL_POS)

/* GPIO function select values */
#define GLB_GPIO_FUNC_ANALOG		10
#define GLB_GPIO_FUNC_SWGPIO		11
#define GLB_GPIO_FUNC_KEY_SCAN_IN	21
#define GLB_GPIO_FUNC_KEY_SCAN_DRV	22

#define GPIO_BFLB_BL70X_PSRAM_START 23
#define GPIO_BFLB_BL70X_PSRAM_END   28
#define GPIO_BFLB_BL70X_PIN_OFFSET  9

void pinctrl_bflb_configure_uart(uint8_t pin, uint8_t func)
{
	uint32_t regval;
	uint8_t sig;
	uint8_t sig_pos;

	regval = sys_read32(GLB_BASE + GLB_UART_SIG_SEL_0_OFFSET);

	sig = pin % UART_SIG_SLOTS;
	sig_pos = sig * GLB_UART_SIG_0_SEL_LEN;

	regval &= (~(UART_SIG_SLOT_MSK << sig_pos));
	regval |= (func << sig_pos);

	for (uint8_t i = 0; i < UART_SIG_SLOTS; i++) {
		/* reset other sigs which are the same with uart_func */
		sig_pos = i * GLB_UART_SIG_0_SEL_LEN;
		if (((regval & (UART_SIG_SLOT_MSK << sig_pos)) == (func << sig_pos))
			&& (i != sig) && (func != UART_SIG_DISABLE)) {
			regval &= (~(UART_SIG_SLOT_MSK << sig_pos));
			regval |= (UART_SIG_DISABLE << sig_pos);
		}
	}

	sys_write32(regval, GLB_BASE + GLB_UART_SIG_SEL_0_OFFSET);
}

#if defined(CONFIG_SOC_SERIES_BL60X)
/* On BL60x, 3 pads:
 * SF1: Embedded pad, No swap.
 * SF2: External pad, No swap.
 * SF3: External pad, No swap.
 */
void pinctrl_bflb_configure_sf_pads(pinctrl_soc_pin_t pin)
{
	/* Nothing to do */
}
#elif defined(CONFIG_SOC_SERIES_BL70X)
/* On BL70x, 3 pads:
 * SF1: External pad, No swap.
 * SF2: Embedded and External pad, io3 to io0 and io2 to cs swaps. Do not use when using embedded.
 * SF3: Embedded pad, No swap.
 */
void pinctrl_bflb_configure_sf_pads(pinctrl_soc_pin_t pin)
{
	uint8_t funinst = (pin >> BFLB_PINMUX_FUN_INST_POS) & BFLB_PINMUX_FUN_INST_MASK;
	uint8_t sig = BFLB_PINMUX_GET_SIGNAL(pin);
	uint8_t real_pin = BFLB_PINMUX_GET_PIN(pin);
	uint32_t tmp;

	if (funinst == BFLB_PINMUX_FUN_INST_sf2) {
		if (sig == BFLB_PINMUX_SIGNAL_d0 && (real_pin == 28 || real_pin == 26)) {
			tmp = sys_read32(GLB_BASE + GLB_PARM_OFFSET);
			if (real_pin == 28) {
				tmp &= ~GLB_CFG_SFLASH2_SWAP_IO0_IO3_MSK;
			} else {
				tmp |= GLB_CFG_SFLASH2_SWAP_IO0_IO3_MSK;
			}
			sys_write32(tmp, GLB_BASE + GLB_PARM_OFFSET);
		}
		if (sig == BFLB_PINMUX_SIGNAL_cs && (real_pin == 25 || real_pin == 23)) {
			tmp = sys_read32(GLB_BASE + GLB_PARM_OFFSET);
			if (real_pin == 25) {
				tmp &= ~GLB_CFG_SFLASH2_SWAP_CS_IO2_MSK;
			} else {
				tmp |= GLB_CFG_SFLASH2_SWAP_CS_IO2_MSK;
			}
			sys_write32(tmp, GLB_BASE + GLB_PARM_OFFSET);
		}
	}
}
#elif defined(CONFIG_SOC_SERIES_BL70XL)
/* On BL70xL, 2 pads:
 * SF1: Embedded pad, io3 to io0 and io2 to cs swaps, not provided, keep as default
 * SF2: External pad, No swap.
 */
void pinctrl_bflb_configure_sf_pads(pinctrl_soc_pin_t pin)
{
	/* Nothing to do */
}
#endif

void pinctrl_bflb_init_pin(pinctrl_soc_pin_t pin)
{
	uint8_t drive;
	uint8_t function;
	uint16_t mode;
	uint32_t regval;
	uint8_t real_pin;
	uint8_t is_odd;
	uint32_t cfg;
	uint32_t cfg_address;
	uint8_t pull_up;
	uint8_t pull_down;
	uint8_t funinst;
	uint8_t sig;

	real_pin = BFLB_PINMUX_GET_PIN(pin);
	function = BFLB_PINMUX_GET_FUN(pin);
	mode = BFLB_PINMUX_GET_MODE(pin);
	drive = BFLB_PINMUX_GET_DRIVER_STRENGTH(pin);
	pull_up = BFLB_PINMUX_GET_PULL_UP(pin);
	pull_down = BFLB_PINMUX_GET_PULL_DOWN(pin);
	funinst = (pin >> BFLB_PINMUX_FUN_INST_POS) & BFLB_PINMUX_FUN_INST_MASK;
	sig = BFLB_PINMUX_GET_SIGNAL(pin);

#if defined(CONFIG_SOC_SERIES_BL70XL)
	/* disable muxed to be xtal32k */
	if (real_pin == BL70XL_XTAL32K_PIN_XI || real_pin == BL70XL_XTAL32K_PIN_XO) {
		regval = sys_read32(HBN_BASE + HBN_PAD_CTRL_0_OFFSET);
		regval &= ~(1U << (real_pin - 5));
		sys_write32(regval, HBN_BASE + HBN_PAD_CTRL_0_OFFSET);
	}
#endif

	/* Disable output anyway */
	regval = sys_read32(GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));
	regval &= ~(1U << (real_pin & 0x1f));
	sys_write32(regval, GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
	if (real_pin >= GPIO_BFLB_BL70X_PSRAM_START && real_pin <= GPIO_BFLB_BL70X_PSRAM_END) {
		/* Pin directed inside ? */
		if ((sys_read32(GLB_BASE + GLB_GPIO_USE_PSRAM__IO_OFFSET)
			& (1U << (real_pin - GPIO_BFLB_BL70X_PSRAM_START))) != 0) {
			real_pin += GPIO_BFLB_BL70X_PIN_OFFSET;
		}
	}
#endif

	is_odd = real_pin & 1U;

	cfg_address = GLB_BASE + GLB_GPIO_CFGCTL0_OFFSET + (real_pin / 2 * 4);
	cfg = sys_read32(cfg_address);
	cfg &= ~(0xffff << (16 * is_odd));

	regval = sys_read32(GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));

	if (mode == BFLB_PINMUX_MODE_analog) {
		regval &= ~(1U << (real_pin & 0x1f));
		function = GLB_GPIO_FUNC_ANALOG;
	} else if (mode == BFLB_PINMUX_MODE_periph) {
		cfg |= (1U << (is_odd * 16 + GLB_REG_GPIO_0_IE_POS));
		regval &= ~(1U << (real_pin & 0x1f));
#if defined(CONFIG_SOC_SERIES_BL70XL)
		/* BL702L: key scan drive function needs IE disabled */
		if (function == GLB_GPIO_FUNC_KEY_SCAN_DRV) {
			cfg &= ~(1U << (is_odd * 16 + GLB_REG_GPIO_0_IE_POS));
		}
#endif
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
		/* For SF2 In/Out Muxing CLK and CS must be outputs in both entries */
		if (funinst == BFLB_PINMUX_FUN_INST_sf2
		    && (sig == BFLB_PINMUX_SIGNAL_cs || sig == BFLB_PINMUX_SIGNAL_clk)) {
			cfg &= ~(1U << (is_odd * 16 + GLB_REG_GPIO_0_IE_POS));
			regval |= (1U << (real_pin & 0x1f));
		}
#endif
	} else {
		function = GLB_GPIO_FUNC_SWGPIO;

		if (mode == BFLB_PINMUX_MODE_input) {
			cfg |= (1U << (is_odd * 16 + GLB_REG_GPIO_0_IE_POS));
		}

		if (mode == BFLB_PINMUX_MODE_output) {
			regval |= (1U << (real_pin & 0x1f));
		}
	}

	sys_write32(regval, GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));

	if (pull_up != 0) {
		cfg |= (1U << (is_odd * 16 + GLB_REG_GPIO_0_PU_POS));
	} else if (pull_down != 0) {
		cfg |= (1U << (is_odd * 16 + GLB_REG_GPIO_0_PD_POS));
	} else {
		/* Do nothing, register cleared earlier */
	}

	if (BFLB_PINMUX_GET_SMT(pin) != 0) {
		cfg |= (1U << (is_odd * 16 + GLB_REG_GPIO_0_SMT_POS));
	}

	cfg |= (drive << (is_odd * 16 + GLB_REG_GPIO_0_DRV_POS));

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
	if (real_pin !=  BFLB_PINMUX_GET_PIN(pin)) {
		real_pin = BFLB_PINMUX_GET_PIN(pin);
		is_odd = real_pin & 1U;
		regval = sys_read32(GLB_BASE + GLB_GPIO_CFGCTL0_OFFSET + (real_pin / 2 * 4));
		regval |= (function << (is_odd * 16 + GLB_REG_GPIO_0_FUNC_SEL_POS));
		sys_write32(regval, GLB_BASE + GLB_GPIO_CFGCTL0_OFFSET + (real_pin / 2 * 4));
		/* For SF2 In/Out Muxing CLK and CS must be outputs in both entries */
		if (sig == BFLB_PINMUX_SIGNAL_cs || sig == BFLB_PINMUX_SIGNAL_clk) {
			regval = sys_read32(GLB_BASE
				+ GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));
			regval |= (1U << (real_pin & 0x1f));
			sys_write32(regval,
				    GLB_BASE + GLB_GPIO_CFGCTL34_OFFSET + ((real_pin >> 5) << 2));
		}
	}
#endif
	cfg |= (function << (is_odd * 16 + GLB_REG_GPIO_0_FUNC_SEL_POS));
	sys_write32(cfg, cfg_address);
}
