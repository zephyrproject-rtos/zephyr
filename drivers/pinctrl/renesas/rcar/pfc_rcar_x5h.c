/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT renesas_rcar_pfc_x5h

#include "pfc_rcar.h"
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/firmware/scmi/pinctrl.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rcar-common.h>
#include <zephyr/init.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#ifndef CONFIG_PINCTRL_RCAR_SCMI

/* Swap both arguments */
#define PFC_REG_ADDRESS(idx, inst) DT_INST_REG_ADDR_BY_IDX(inst, idx)
static uintptr_t reg_base[] = {
	LISTIFY(DT_NUM_REGS(DT_DRV_INST(0)), PFC_REG_ADDRESS, (,), 0)
};

#define PFC_REG_SIZE(idx, inst) DT_INST_REG_SIZE_BY_IDX(inst, idx)
static const uintptr_t __maybe_unused reg_sizes[] = {
	LISTIFY(DT_NUM_REGS(DT_DRV_INST(0)), PFC_REG_SIZE, (,), 0)
};

/* Gen5 has one GPIO group at multiple base address */
#define PFC_RCAR_PMMR 0x0 /* Multiplexed Pin Setting Mask Register */
#define PFC_RCAR_GPSR 0x40 /* GPIO/Peripheral Function Select Register */

#define PFC_RCAR_ALTSEL0 0x60
#define PFC_RCAR_ALTSEL(i) (PFC_RCAR_ALTSEL0 + (i) * sizeof(uint32_t))

#define PFC_RCAR_ALTSEL_REG_CNT 4

#define PFC_RCAR_DRVCTRL_REG_CNT 3

/*
 * Writing a value to any register from among the GPSR, ALTSELm, DRVCTRLm,
 * PUEN, or PUD registers is enabled by writing the inverse of the value to
 * the PPMR register.
 */
static void pfc_rcar_write(uintptr_t pfc_base, uint32_t offset, uint32_t val)
{
	sys_write32(~val, pfc_base + PFC_RCAR_PMMR);
	sys_write32(val, pfc_base + offset);
}

/* Set pin mode, either GPIO or Peripheral */
static void pfc_rcar_set_mode(uintptr_t pfc_base,
			      uint8_t bit, bool peripheral)
{
	uint32_t val = sys_read32(pfc_base + PFC_RCAR_GPSR);

	if (peripheral) {
		val |= BIT(bit);
	} else {
		val &= ~BIT(bit);
	}

	pfc_rcar_write(pfc_base, PFC_RCAR_GPSR, val);
}

/* Set peripheral function */
static void pfc_rcar_set_func(uintptr_t pfc_base,
			      const struct rcar_pin_func *rcar_func)
{
	uint32_t reg, val;

	for (uint8_t i = 0; i < PFC_RCAR_ALTSEL_REG_CNT; i++) {
		reg = PFC_RCAR_ALTSEL(i);

		val = sys_read32(pfc_base + reg);
		if (rcar_func->func & BIT(i)) {
			val |= BIT(rcar_func->bit);
		} else {
			val &= ~BIT(rcar_func->bit);
		}
		pfc_rcar_write(pfc_base, reg, val);
	}
}

static const struct pfc_drive_reg *pfc_rcar_get_drive_reg(uint16_t pin,
							  uint8_t *bit)
{
	const struct pfc_drive_reg *drive_regs = pfc_rcar_get_drive_regs();

	/* Loop around all the registers to find the bit for a given pin */
	while (drive_regs->drvctrl0 && drive_regs->drvctrl1 && drive_regs->drvctrl2) {
		for (size_t i = 0; i < ARRAY_SIZE(drive_regs->pins); i++) {
			if (drive_regs->pins[i] == pin) {
				*bit = i;
				return drive_regs;
			}
		}
		drive_regs++;
	}

	return NULL;
}

/* Convert drive strength from uA to a 3-bit encoded value */
static int pfc_rcar_strength_microamp_to_raw(uint16_t microamp, uint8_t *raw)
{
	int ret = 0;

	switch (microamp) {
	case 7500:
		*raw = 1;
		break;
	case 9500:
		*raw = 2;
		break;
	case 10300:
		*raw = 3;
		break;
	case 10600:
		*raw = 4;
		break;
	case 10800:
		*raw = 5;
		break;
	case 11000:
		*raw = 6;
		break;
	case 11200:
		*raw = 7;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int pfc_rcar_set_drive_strength(uintptr_t pfc_base, uint16_t pin,
				       uint16_t strength_microamp)
{
	const struct pfc_drive_reg *drive_reg;
	uint32_t reg, val;
	uint8_t bit, strength_raw = 0;
	int ret;

	drive_reg = pfc_rcar_get_drive_reg(pin, &bit);
	if (drive_reg == NULL) {
		return -EINVAL;
	}

	ret = pfc_rcar_strength_microamp_to_raw(strength_microamp, &strength_raw);
	if (ret) {
		return ret;
	}

	for (uint8_t i = 0; i < PFC_RCAR_DRVCTRL_REG_CNT; i++) {
		switch (i) {
		case 0:
			reg = drive_reg->drvctrl0;
			break;
		case 1:
			reg = drive_reg->drvctrl1;
			break;
		default:
			reg = drive_reg->drvctrl2;
		}

		val = sys_read32(pfc_base + reg);
		if (strength_raw & BIT(i)) {
			val |= BIT(bit);
		} else {
			val &= ~BIT(bit);
		}
		pfc_rcar_write(pfc_base, reg, val);
	}

	return 0;
}

static const struct pfc_bias_reg *pfc_rcar_get_bias_reg(uint16_t pin,
							uint8_t *bit)
{
	const struct pfc_bias_reg *bias_regs = pfc_rcar_get_bias_regs();

	/* Loop around all the registers to find the bit for a given pin */
	while (bias_regs->puen && bias_regs->pud) {
		for (size_t i = 0; i < ARRAY_SIZE(bias_regs->pins); i++) {
			if (bias_regs->pins[i] == pin) {
				*bit = i;
				return bias_regs;
			}
		}
		bias_regs++;
	}

	return NULL;
}

static int pfc_rcar_set_bias(uintptr_t pfc_base, uint16_t pin, uint16_t flags)
{
	const struct pfc_bias_reg *bias_reg;
	uint32_t val;
	uint8_t bit;

	bias_reg = pfc_rcar_get_bias_reg(pin, &bit);
	if (bias_reg == NULL) {
		return -EINVAL;
	}

	/* Enable/disable pull resistor */
	val = sys_read32(pfc_base + bias_reg->puen);
	if (!(flags & RCAR_PIN_FLAGS_PUEN)) {
		pfc_rcar_write(pfc_base, bias_reg->puen, val & ~BIT(bit));
		return 0;
	}
	pfc_rcar_write(pfc_base, bias_reg->puen, val | BIT(bit));

	/* Select either pull-up or pull-down */
	val = sys_read32(pfc_base + bias_reg->pud);
	if (flags & RCAR_PIN_FLAGS_PUD) {
		val |= BIT(bit);
	} else {
		val &= ~BIT(bit);
	}
	pfc_rcar_write(pfc_base, bias_reg->pud, val);

	return 0;
}

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uintptr_t pfc_base;
	uint16_t pin_id = pin->pin;
	uint8_t reg_index, bit;
	int ret;

	/* Get GPIO group base address */
	ret = pfc_rcar_get_reg_index(pin_id, &reg_index);
	if (ret) {
		return ret;
	}

	if (reg_index >= ARRAY_SIZE(reg_base)) {
		return -EINVAL;
	}

	pfc_base = reg_base[reg_index];
	bit = pin_id % 32;

	/* Set pin mode and function */
	if (pin->flags & RCAR_PIN_FLAGS_FUNC_SET) {
		pfc_rcar_set_mode(pfc_base, bit, true);
		pfc_rcar_set_func(pfc_base, &pin->func);
	} else {
		pfc_rcar_set_mode(pfc_base, bit, false);
	}

	/* Set bias */
	if (pin->flags & RCAR_PIN_FLAGS_PULL_SET) {
		ret = pfc_rcar_set_bias(pfc_base, pin_id, pin->flags);
		if (ret < 0) {
			return ret;
		}
	}

	/* Set drive strength */
	if (pin->drive_strength_microamp != 0) {
		ret = pfc_rcar_set_drive_strength(pfc_base, pin_id,
						  pin->drive_strength_microamp);
	}

	return ret;
}

#ifdef DEVICE_MMIO_IS_IN_RAM
__boot_func static int pfc_rcar_driver_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(reg_base); i++) {
		device_map(reg_base + i, reg_base[i], reg_sizes[i], K_MEM_CACHE_NONE);
	}

	return 0;
}

SYS_INIT(pfc_rcar_driver_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

#else

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	struct scmi_pinctrl_settings settings;
	uint8_t config_cnt = 0;

	/* Pin ID */
	settings.id = pin->pin;

	/* Peripheral function */
	settings.function = pin->func.func;

	if (pin->flags & RCAR_PIN_FLAGS_PULL_SET) {
		/* Bias enable/disable */
		settings.config[config_cnt++] = SCMI_PINCTRL_BIAS_DISABLE;
		settings.config[config_cnt++] = (pin->flags & RCAR_PIN_FLAGS_PUEN) ? 0 : 1;

		/* Bias pull-up or pull-down */
		settings.config[config_cnt++] = SCMI_PINCTRL_BIAS_PULL_DEFAULT;
		settings.config[config_cnt++] = (pin->flags & RCAR_PIN_FLAGS_PUD) ? 1 : 0;
	}

	/* Drive strength, in uA */
	if (pin->drive_strength_microamp != 0) {
		settings.config[config_cnt++] = SCMI_PCINTRL_DRIVE_STRENGTH;
		settings.config[config_cnt++] = pin->drive_strength_microamp;
	}

	/* Attributes, containing mode info (either GPIO or Peripheral) */
	settings.attributes =
		SCMI_PINCTRL_CONFIG_ATTRIBUTES((pin->flags & RCAR_PIN_FLAGS_FUNC_SET) ? 1 : 0,
					       config_cnt >> 1,
					       SCMI_PINCTRL_SELECTOR_PIN);

	return scmi_pinctrl_settings_configure(&settings);
}

#endif /* CONFIG_PINCTRL_RCAR_SCMI */

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		ret = pinctrl_configure_pin(pins++);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
