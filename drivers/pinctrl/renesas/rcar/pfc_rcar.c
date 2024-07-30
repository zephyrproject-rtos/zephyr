/*
 * Copyright (c) 2021-2023 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT renesas_rcar_pfc

#include "pfc_rcar.h"
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/device_mmio.h>

#define PFC_RCAR_PMMR 0x0

/* Gen3 only has one base address, Gen4 has one per GPIO controller */
#if defined(CONFIG_SOC_SERIES_RCAR_GEN3)
#define PFC_RCAR_GPSR 0x100
#define PFC_RCAR_IPSR 0x200
#elif defined(CONFIG_SOC_SERIES_RCAR_GEN4)
#define PFC_RCAR_GPSR 0x040
#define PFC_RCAR_IPSR 0x060
#else
#error Unsupported SoC Series
#endif

/* swap both arguments */
#define PFC_REG_ADDRESS(idx, inst) DT_INST_REG_ADDR_BY_IDX(inst, idx)
static uintptr_t reg_base[] = {
	LISTIFY(DT_NUM_REGS(DT_DRV_INST(0)), PFC_REG_ADDRESS, (,), 0)
};

#define PFC_REG_SIZE(idx, inst) DT_INST_REG_SIZE_BY_IDX(inst, idx)
static const uintptr_t __maybe_unused reg_sizes[] = {
	LISTIFY(DT_NUM_REGS(DT_DRV_INST(0)), PFC_REG_SIZE, (,), 0)
};

#ifdef CONFIG_PINCTRL_RCAR_VOLTAGE_CONTROL
/* POC Control Register can control IO voltage level that is supplied to the pin */
struct pfc_pocctrl_reg {
	uint32_t offset;
	const uint16_t pins[32];
};
#endif /* CONFIG_PINCTRL_RCAR_VOLTAGE_CONTROL */

/*
 * Each drive step is either encoded in 2 or 3 bits.
 * So based on a 24 mA maximum value each step is either
 * 24/4 mA or 24/8 mA.
 */
#define PFC_RCAR_DRIVE_MAX 24U
#define PFC_RCAR_DRIVE_STEP(size) \
	(size == 2 ? PFC_RCAR_DRIVE_MAX / 4 : PFC_RCAR_DRIVE_MAX / 8)

/* Some registers such as IPSR GPSR or DRVCTRL are protected and
 * must be preceded to a write to PMMR with the inverse value.
 */
static void pfc_rcar_write(uintptr_t pfc_base, uint32_t offs, uint32_t val)
{
	sys_write32(~val, pfc_base + PFC_RCAR_PMMR);
	sys_write32(val, pfc_base + offs);
}

/* Set the pin either in gpio or peripheral */
static void pfc_rcar_set_gpsr(uintptr_t pfc_base,
			      uint16_t pin, bool peripheral)
{
#if defined(CONFIG_SOC_SERIES_RCAR_GEN3)
	/* On Gen3 we have multiple GPSR at one base address */
	uint8_t bank = pin / 32;
#elif defined(CONFIG_SOC_SERIES_RCAR_GEN4)
	/* On Gen4 we have one GPSR at multiple base address */
	uint8_t bank = 0;
#endif
	uint8_t bit = pin % 32;
	uint32_t val = sys_read32(pfc_base + PFC_RCAR_GPSR +
				  bank * sizeof(uint32_t));

	if (peripheral) {
		val |= BIT(bit);
	} else {
		val &= ~BIT(bit);
	}
	pfc_rcar_write(pfc_base, PFC_RCAR_GPSR + bank * sizeof(uint32_t), val);
}

/* Set peripheral function */
static void pfc_rcar_set_ipsr(uintptr_t pfc_base,
			      const struct rcar_pin_func *rcar_func)
{
	uint16_t reg_offs = PFC_RCAR_IPSR + rcar_func->bank * sizeof(uint32_t);
	uint32_t val = sys_read32(pfc_base + reg_offs);

	val &= ~(0xFU << rcar_func->shift);
	val |= (rcar_func->func << rcar_func->shift);
	pfc_rcar_write(pfc_base, reg_offs, val);
}

static uint32_t pfc_rcar_get_drive_reg(uint16_t pin, uint8_t *offset,
				       uint8_t *size)
{
	const struct pfc_drive_reg *drive_regs = pfc_rcar_get_drive_regs();

	while (drive_regs->reg != 0U) {
		for (size_t i = 0U; i < ARRAY_SIZE(drive_regs->fields); i++) {
			if (drive_regs->fields[i].pin == pin) {
				*offset = drive_regs->fields[i].offset;
				*size = drive_regs->fields[i].size;
				return drive_regs->reg;
			}
		}
		drive_regs++;
	}

	return 0;
}

/*
 * Maximum drive strength is 24mA. This value can be lowered
 * using DRVCTRLx registers, some pins have 8 steps (3 bits size encoded)
 * some have 4 steps (2 bits size encoded).
 */
static int pfc_rcar_set_drive_strength(uintptr_t pfc_base, uint16_t pin,
				       uint8_t strength)
{
	uint8_t offset, size, step;
	uint32_t reg, val;

	reg = pfc_rcar_get_drive_reg(pin, &offset, &size);
	if (reg == 0U) {
		return -EINVAL;
	}

	step = PFC_RCAR_DRIVE_STEP(size);
	if ((strength < step) || (strength > PFC_RCAR_DRIVE_MAX)) {
		return -EINVAL;
	}

	/* Convert the value from mA based on a full drive strength
	 * value of 24mA.
	 */
	strength = (strength / step) - 1U;
	/* clear previous drive strength value */
	val = sys_read32(pfc_base + reg);
	val &= ~GENMASK(offset + size - 1U, offset);
	val |= strength << offset;

	pfc_rcar_write(pfc_base, reg, val);

	return 0;
}

static const struct pfc_bias_reg *pfc_rcar_get_bias_reg(uint16_t pin,
							uint8_t *bit)
{
	const struct pfc_bias_reg *bias_regs = pfc_rcar_get_bias_regs();

	/* Loop around all the registers to find the bit for a given pin */
	while (bias_regs->puen && bias_regs->pud) {
		for (size_t i = 0U; i < ARRAY_SIZE(bias_regs->pins); i++) {
			if (bias_regs->pins[i] == pin) {
				*bit = i;
				return bias_regs;
			}
		}
		bias_regs++;
	}

	return NULL;
}

int pfc_rcar_set_bias(uintptr_t pfc_base, uint16_t pin, uint16_t flags)
{
	uint32_t val;
	uint8_t bit;
	const struct pfc_bias_reg *bias_reg = pfc_rcar_get_bias_reg(pin, &bit);

	if (bias_reg == NULL) {
		return -EINVAL;
	}

	/* pull enable/disable*/
	val = sys_read32(pfc_base + bias_reg->puen);
	if ((flags & RCAR_PIN_FLAGS_PUEN) == 0U) {
		sys_write32(val & ~BIT(bit), pfc_base + bias_reg->puen);
		return 0;
	}
	sys_write32(val | BIT(bit), pfc_base + bias_reg->puen);

	/* pull - up/down */
	val = sys_read32(pfc_base + bias_reg->pud);
	if (flags & RCAR_PIN_FLAGS_PUD) {
		sys_write32(val | BIT(bit), pfc_base + bias_reg->pud);
	} else {
		sys_write32(val & ~BIT(bit), pfc_base + bias_reg->pud);
	}
	return 0;
}

#ifdef CONFIG_PINCTRL_RCAR_VOLTAGE_CONTROL

const struct pfc_pocctrl_reg pfc_r8a77951_r8a77961_volt_regs[] = {
	{
		.offset = 0x0380,
		.pins = {
			[0]  = RCAR_GP_PIN(3,  0),    /* SD0_CLK  */
			[1]  = RCAR_GP_PIN(3,  1),    /* SD0_CMD  */
			[2]  = RCAR_GP_PIN(3,  2),    /* SD0_DAT0 */
			[3]  = RCAR_GP_PIN(3,  3),    /* SD0_DAT1 */
			[4]  = RCAR_GP_PIN(3,  4),    /* SD0_DAT2 */
			[5]  = RCAR_GP_PIN(3,  5),    /* SD0_DAT3 */
			[6]  = RCAR_GP_PIN(3,  6),    /* SD1_CLK  */
			[7]  = RCAR_GP_PIN(3,  7),    /* SD1_CMD  */
			[8]  = RCAR_GP_PIN(3,  8),    /* SD1_DAT0 */
			[9]  = RCAR_GP_PIN(3,  9),    /* SD1_DAT1 */
			[10] = RCAR_GP_PIN(3,  10),   /* SD1_DAT2 */
			[11] = RCAR_GP_PIN(3,  11),   /* SD1_DAT3 */
			[12] = RCAR_GP_PIN(4,  0),    /* SD2_CLK  */
			[13] = RCAR_GP_PIN(4,  1),    /* SD2_CMD  */
			[14] = RCAR_GP_PIN(4,  2),    /* SD2_DAT0 */
			[15] = RCAR_GP_PIN(4,  3),    /* SD2_DAT1 */
			[16] = RCAR_GP_PIN(4,  4),    /* SD2_DAT2 */
			[17] = RCAR_GP_PIN(4,  5),    /* SD2_DAT3 */
			[18] = RCAR_GP_PIN(4,  6),    /* SD2_DS   */
			[19] = RCAR_GP_PIN(4,  7),    /* SD3_CLK  */
			[20] = RCAR_GP_PIN(4,  8),    /* SD3_CMD  */
			[21] = RCAR_GP_PIN(4,  9),    /* SD3_DAT0 */
			[22] = RCAR_GP_PIN(4,  10),   /* SD3_DAT1 */
			[23] = RCAR_GP_PIN(4,  11),   /* SD3_DAT2 */
			[24] = RCAR_GP_PIN(4,  12),   /* SD3_DAT3 */
			[25] = RCAR_GP_PIN(4,  13),   /* SD3_DAT4 */
			[26] = RCAR_GP_PIN(4,  14),   /* SD3_DAT5 */
			[27] = RCAR_GP_PIN(4,  15),   /* SD3_DAT6 */
			[28] = RCAR_GP_PIN(4,  16),   /* SD3_DAT7 */
			[29] = RCAR_GP_PIN(4,  17),   /* SD3_DS   */
			[30] = -1,
			[31] = -1,
		}
	},
	{ /* sentinel */ },
};

static const struct pfc_pocctrl_reg *pfc_rcar_get_io_voltage_regs(void)
{
	return pfc_r8a77951_r8a77961_volt_regs;
}

static const struct pfc_pocctrl_reg *pfc_rcar_get_pocctrl_reg(uint16_t pin, uint8_t *bit)
{
	const struct pfc_pocctrl_reg *voltage_regs = pfc_rcar_get_io_voltage_regs();

	BUILD_ASSERT(ARRAY_SIZE(voltage_regs->pins) < UINT8_MAX);

	/* Loop around all the registers to find the bit for a given pin */
	while (voltage_regs && voltage_regs->offset) {
		uint8_t i;

		for (i = 0U; i < ARRAY_SIZE(voltage_regs->pins); i++) {
			if (voltage_regs->pins[i] == pin) {
				*bit = i;
				return voltage_regs;
			}
		}
		voltage_regs++;
	}

	return NULL;
}

static void pfc_rcar_set_voltage(uintptr_t pfc_base, uint16_t pin, uint16_t voltage)
{
	uint32_t val;
	uint8_t bit;
	const struct pfc_pocctrl_reg *voltage_reg;

	voltage_reg = pfc_rcar_get_pocctrl_reg(pin, &bit);
	if (!voltage_reg) {
		return;
	}

	val = sys_read32(pfc_base + voltage_reg->offset);

	switch (voltage) {
	case PIN_VOLTAGE_1P8V:
		if (!(val & BIT(bit))) {
			return;
		}
		val &= ~BIT(bit);
		break;
	case PIN_VOLTAGE_3P3V:
		if (val & BIT(bit)) {
			return;
		}
		val |= BIT(bit);
		break;
	default:
		break;
	}

	pfc_rcar_write(pfc_base, voltage_reg->offset, val);
}
#endif /* CONFIG_PINCTRL_RCAR_VOLTAGE_CONTROL */

int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	int ret = 0;
	uint8_t reg_index;
	uintptr_t pfc_base;

	ret = pfc_rcar_get_reg_index(pin->pin, &reg_index);
	if (ret) {
		return ret;
	}

	if (reg_index >= ARRAY_SIZE(reg_base)) {
		return -EINVAL;
	}

	pfc_base = reg_base[reg_index];

	/* Set pin as GPIO if capable */
	if (RCAR_IS_GP_PIN(pin->pin)) {
		pfc_rcar_set_gpsr(pfc_base, pin->pin, false);
	} else if ((pin->flags & RCAR_PIN_FLAGS_FUNC_SET) == 0U) {
		/* A function must be set for non GPIO capable pin */
		return -EINVAL;
	}

#ifdef CONFIG_PINCTRL_RCAR_VOLTAGE_CONTROL
	if (pin->voltage != PIN_VOLTAGE_NONE) {
		pfc_rcar_set_voltage(pfc_base, pin->pin, pin->voltage);
	}
#endif

	/* Select function for pin */
	if ((pin->flags & RCAR_PIN_FLAGS_FUNC_SET) != 0U) {

		if ((pin->flags & RCAR_PIN_FLAGS_FUNC_DUMMY) == 0U) {
			pfc_rcar_set_ipsr(pfc_base, &pin->func);
		}

		if (RCAR_IS_GP_PIN(pin->pin)) {
			pfc_rcar_set_gpsr(pfc_base, pin->pin, true);
		}

		if ((pin->flags & RCAR_PIN_FLAGS_PULL_SET) != 0U) {
			ret = pfc_rcar_set_bias(pfc_base, pin->pin, pin->flags);
			if (ret < 0) {
				return ret;
			}
		}
	}

	if (pin->drive_strength != 0U) {
		ret = pfc_rcar_set_drive_strength(pfc_base, pin->pin,
						  pin->drive_strength);
	}

	return ret;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	int ret = 0;

	ARG_UNUSED(reg);
	while (pin_cnt-- > 0U) {
		ret = pinctrl_configure_pin(pins++);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

#if defined(DEVICE_MMIO_IS_IN_RAM)
__boot_func static int pfc_rcar_driver_init(void)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(reg_base); i++) {
		device_map(reg_base + i, reg_base[i], reg_sizes[i], K_MEM_CACHE_NONE);
	}
	return 0;
}

SYS_INIT(pfc_rcar_driver_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
