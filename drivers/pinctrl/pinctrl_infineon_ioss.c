/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_ioss_pinctrl

#include <errno.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#define IFX_IOSS_HSIOM_BASE		DT_INST_REG_ADDR_BY_NAME(0, hsiom)
#define IFX_IOSS_GPIO_BASE		DT_INST_REG_ADDR_BY_NAME(0, gpio)

#define IFX_IOSS_HSIOM_PRT_SIZE		0x10U
#define IFX_IOSS_HSIOM_PORT_SEL(pin)	(((pin) / 4U) * 4U)
#define IFX_IOSS_HSIOM_SEL_SHIFT(pin)	(((pin) % 4U) * 8U)
#define IFX_IOSS_HSIOM_SEL_MSK		GENMASK(4, 0)
#define IFX_IOSS_HSIOM_NONSEC		0x1000U

#define IFX_IOSS_GPIO_PRT_SIZE		0x80U
#define IFX_IOSS_GPIO_PRT_OUT_CLR	0x04U
#define IFX_IOSS_GPIO_PRT_OUT_SET	0x08U
#define IFX_IOSS_GPIO_PRT_CFG		0x44U
#define IFX_IOSS_GPIO_PRT_CFG_OUT	0x4CU
#define IFX_IOSS_GPIO_PRT_CFG_SLEW_EXT	0x64U
#define IFX_IOSS_GPIO_PRT_CFG_DRIVE_EXT	0x68U
#define IFX_IOSS_GPIO_PRT_CFG_OUT3	0x70U

#define IFX_IOSS_GPIO_CFG_SHIFT(pin)	((pin) * 4U)
#define IFX_IOSS_GPIO_CFG_MSK		GENMASK(3, 0)

#define IFX_IOSS_GPIO_SLOW(pin)		BIT(pin)
#define IFX_IOSS_GPIO_DRIVE_SHIFT(pin)	(16U + ((pin) * 2U))
#define IFX_IOSS_GPIO_DRIVE_MSK		GENMASK(1, 0)

#if defined(CONFIG_SOC_SERIES_PSC3)
#define IFX_IOSS_GPIO_DRIVE_SEL_FULL	0U
#define IFX_IOSS_GPIO_DRIVE_SEL_HALF	1U
#define IFX_IOSS_GPIO_DRIVE_SEL_QUARTER	2U
#define IFX_IOSS_GPIO_DRIVE_SEL_EIGHTH	3U
#else
#define IFX_IOSS_GPIO_DRIVE_SEL_FULL	0U
#define IFX_IOSS_GPIO_DRIVE_SEL_HALF	2U
#define IFX_IOSS_GPIO_DRIVE_SEL_QUARTER	3U
#endif /* CONFIG_SOC_SERIES_PSC3 */

#define IFX_IOSS_GPIO_SLEW_EXT(pin)		BIT((pin) * 4U)
#define IFX_IOSS_GPIO_DRIVE_EXT(pin)		(((pin) / 4U) * 4U)
#define IFX_IOSS_GPIO_DRIVE_EXT_SHIFT(pin)	(((pin) % 4U) * 8U)
#define IFX_IOSS_GPIO_DRIVE_EXT_MSK		GENMASK(2, 0)
#define IFX_IOSS_GPIO_DRIVE_EXT_FULL		7U
#define IFX_IOSS_GPIO_CFG_OUT3_MSK		GENMASK(3, 0)

static struct k_spinlock ifx_ioss_lock;

static inline int ifx_ioss_drive_sel(uint32_t drive, uint32_t *sel)
{
	if (IS_ENABLED(CONFIG_SOC_SERIES_PSE84)) {
		*sel = IFX_IOSS_GPIO_DRIVE_EXT_FULL - drive;
		return 0;
	}

	switch (drive) {
	case IFX_IOSS_DRIVE_FULL:
		*sel = IFX_IOSS_GPIO_DRIVE_SEL_FULL;
		return 0;
	case IFX_IOSS_DRIVE_HALF:
		*sel = IFX_IOSS_GPIO_DRIVE_SEL_HALF;
		return 0;
	case IFX_IOSS_DRIVE_ONE_FOURTH:
		*sel = IFX_IOSS_GPIO_DRIVE_SEL_QUARTER;
		return 0;
#if defined(CONFIG_SOC_SERIES_PSC3)
	case IFX_IOSS_DRIVE_ONE_EIGHTH:
		*sel = IFX_IOSS_GPIO_DRIVE_SEL_EIGHTH;
		return 0;
#endif /* CONFIG_SOC_SERIES_PSC3 */
	default:
		return -ENOTSUP;
	}
}

static inline void ifx_ioss_rmw(mem_addr_t reg, uint32_t clr, uint32_t set)
{
	uint32_t val = sys_read32(reg);

	val &= ~clr;
	val |= set;
	sys_write32(val, reg);
}

#if defined(CONFIG_PINCTRL_INFINEON_IOSS_EXT)
static void ifx_ioss_configure_ext(mem_addr_t gpio, uint32_t pin_num,
				   uint32_t pincfg, uint32_t drive)
{
	uint32_t drive_shift = IFX_IOSS_GPIO_DRIVE_EXT_SHIFT(pin_num);
	uint32_t slew = 0U;

	if ((pincfg & IFX_IOSS_PINCFG_SLEW_EN) != 0U) {
		if ((pincfg & IFX_IOSS_PINCFG_SLEW_SLOW) != 0U) {
			slew = IFX_IOSS_GPIO_SLEW_EXT(pin_num);
		}

		ifx_ioss_rmw(gpio + IFX_IOSS_GPIO_PRT_CFG_SLEW_EXT,
			     IFX_IOSS_GPIO_SLEW_EXT(pin_num), slew);
	}

	if ((pincfg & IFX_IOSS_PINCFG_DRIVE_EN) != 0U) {
		ifx_ioss_rmw(gpio + IFX_IOSS_GPIO_PRT_CFG_DRIVE_EXT +
			     IFX_IOSS_GPIO_DRIVE_EXT(pin_num),
			     IFX_IOSS_GPIO_DRIVE_EXT_MSK << drive_shift,
			     drive << drive_shift);
	}
}
#else
static void ifx_ioss_configure_cfg_out(mem_addr_t gpio, uint32_t pin_num,
				       uint32_t pincfg, uint32_t drive)
{
	uint32_t drive_shift = IFX_IOSS_GPIO_DRIVE_SHIFT(pin_num);
	uint32_t val = sys_read32(gpio + IFX_IOSS_GPIO_PRT_CFG_OUT);

	if ((pincfg & IFX_IOSS_PINCFG_SLEW_EN) != 0U) {
		val &= ~IFX_IOSS_GPIO_SLOW(pin_num);

		if ((pincfg & IFX_IOSS_PINCFG_SLEW_SLOW) != 0U) {
			val |= IFX_IOSS_GPIO_SLOW(pin_num);
		}
	}

	if ((pincfg & IFX_IOSS_PINCFG_DRIVE_EN) != 0U) {
		val &= ~(IFX_IOSS_GPIO_DRIVE_MSK << drive_shift);
		val |= drive << drive_shift;
	}

	sys_write32(val, gpio + IFX_IOSS_GPIO_PRT_CFG_OUT);
}
#endif /* CONFIG_PINCTRL_INFINEON_IOSS_EXT */

static void ifx_ioss_configure_out(mem_addr_t gpio, uint32_t pin_num,
				   uint32_t pincfg, uint32_t drive)
{
	uint32_t out_en = IFX_IOSS_PINCFG_SLEW_EN | IFX_IOSS_PINCFG_DRIVE_EN;

	if ((pincfg & out_en) == 0U) {
		return;
	}

#if defined(CONFIG_PINCTRL_INFINEON_IOSS_EXT)
	ifx_ioss_configure_ext(gpio, pin_num, pincfg, drive);
#else
	ifx_ioss_configure_cfg_out(gpio, pin_num, pincfg, drive);
#endif /* CONFIG_PINCTRL_INFINEON_IOSS_EXT */
}

static int ifx_ioss_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t port = IFX_IOSS_PINMUX_PORT(pin->pinmux);
	uint32_t pin_num = IFX_IOSS_PINMUX_PIN(pin->pinmux);
	uint32_t hsiom = IFX_IOSS_PINMUX_HSIOM(pin->pinmux);
	uint32_t cfg = pin->pincfg & IFX_IOSS_GPIO_CFG_MSK;
	uint32_t cfg_shift = IFX_IOSS_GPIO_CFG_SHIFT(pin_num);
	uint32_t sel_shift = IFX_IOSS_HSIOM_SEL_SHIFT(pin_num);
	mem_addr_t gpio = IFX_IOSS_GPIO_BASE + (port * IFX_IOSS_GPIO_PRT_SIZE);
	mem_addr_t hsiom_prt = IFX_IOSS_HSIOM_BASE +
			       (port * IFX_IOSS_HSIOM_PRT_SIZE);
	k_spinlock_key_t key;
	uint32_t drive = 0U;
	int ret;

	if ((pin->pincfg & IFX_IOSS_PINCFG_DRIVE_EN) != 0U) {
		ret = ifx_ioss_drive_sel(IFX_IOSS_PINCFG_DRIVE(pin->pincfg),
					 &drive);
		if (ret != 0) {
			return ret;
		}
	}

	key = k_spin_lock(&ifx_ioss_lock);

	if (IS_ENABLED(CONFIG_TRUSTED_EXECUTION_SECURE)) {
		sys_clear_bit(hsiom_prt + IFX_IOSS_HSIOM_NONSEC, pin_num);
	}

	if (IFX_IOSS_PINCFG_DM(pin->pincfg) == IFX_IOSS_DM_PULLDOWN) {
		sys_write32(BIT(pin_num), gpio + IFX_IOSS_GPIO_PRT_OUT_CLR);
	} else {
		sys_write32(BIT(pin_num), gpio + IFX_IOSS_GPIO_PRT_OUT_SET);
	}

	ifx_ioss_configure_out(gpio, pin_num, pin->pincfg, drive);

	if (IS_ENABLED(CONFIG_SOC_SERIES_PSE84) &&
	    (IFX_IOSS_PINCFG_DM(pin->pincfg) == IFX_IOSS_DM_HIGHZ)) {
		ifx_ioss_rmw(gpio + IFX_IOSS_GPIO_PRT_CFG_OUT3,
			     IFX_IOSS_GPIO_CFG_OUT3_MSK << cfg_shift, 0U);
	}

	ifx_ioss_rmw(gpio + IFX_IOSS_GPIO_PRT_CFG,
		     IFX_IOSS_GPIO_CFG_MSK << cfg_shift, cfg << cfg_shift);

	ifx_ioss_rmw(hsiom_prt + IFX_IOSS_HSIOM_PORT_SEL(pin_num),
		     IFX_IOSS_HSIOM_SEL_MSK << sel_shift,
		     (hsiom & IFX_IOSS_HSIOM_SEL_MSK) << sel_shift);

	k_spin_unlock(&ifx_ioss_lock, key);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		ret = ifx_ioss_configure_pin(&pins[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}
