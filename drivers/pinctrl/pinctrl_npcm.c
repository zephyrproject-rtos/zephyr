/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_pinctrl

#include <assert.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/npcm-pinctrl.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pinctrl_npcm, CONFIG_PINCTRL_LOG_LEVEL);

/*
 * Multi-function pinmux definitions.
 *
 * The table is used to get the pinmux register field and alt function config for supported alt
 * functions. Each pin should be defined in the device tree. The alt function config is a bit field,
 * and each bit represents the alt function selection of the pinmux register field. The bit value is
 * defined in the order of the alt functions macro names.
 *
 * The GPIO must be the 'first' alt function in the alt function config for the GPIO driver to set
 * the pin as a GPIO pin. The rest of the alt functions should be defined in the order of the alt
 * functions macro names.
 */
#define NPCM_PINMUX_REG_SIZE   5
#define NPCM_PINMUX_GROUP_SIZE 6

struct npcm_pinmux_config {
	uint16_t pin_reg[NPCM_PINMUX_REG_SIZE];
	uint8_t alt[NPCM_PINMUX_GROUP_SIZE]; /* Bit n in alt represents reg[n] bit value */
} __packed;

#define NPCM_PINMUX_GROUP_RDATA(id, reg, alt_group)                                                \
	[id] = {                                                                                   \
		.pin_reg = reg,                                                                    \
		.alt = alt_group,                                                                  \
	},

#define NPCM_PINMUX_ID(node_id, prop) DT_PROP_BY_IDX(node_id, prop, 0)
#define NPCM_PINMUX_PIN_REG(node_id, prop)                                                         \
	{DT_PROP_BY_IDX(node_id, prop, 4), DT_PROP_BY_IDX(node_id, prop, 3),                       \
	 DT_PROP_BY_IDX(node_id, prop, 2), DT_PROP_BY_IDX(node_id, prop, 1),                       \
	 DT_PROP_BY_IDX(node_id, prop, 0)}
#define NPCM_PINMUX_ALT(node_id, prop)                                                             \
	{DT_PROP_BY_IDX(node_id, prop, 0), DT_PROP_BY_IDX(node_id, prop, 1),                       \
	 DT_PROP_BY_IDX(node_id, prop, 2), DT_PROP_BY_IDX(node_id, prop, 3),                       \
	 DT_PROP_BY_IDX(node_id, prop, 4), DT_PROP_BY_IDX(node_id, prop, 5)}

#define NPCM_PINMUX_INIT(node_id, prop)                                                            \
	IF_ENABLED( \
		DT_PROP(node_id, nuvoton_pin_hw_def), \
		(NPCM_PINMUX_GROUP_RDATA( \
				NPCM_PINMUX_ID(node_id, nuvoton_id), \
				NPCM_PINMUX_PIN_REG(node_id, nuvoton_pin_reg), \
				NPCM_PINMUX_ALT(node_id, nuvoton_alt) \
				) \
		))

static const struct npcm_pinmux_config npcm_pinmux_table[] = {
	DT_INST_FOREACH_CHILD_VARGS(0, NPCM_PINMUX_INIT)};
static const int npcm_pinmux_table_size = ARRAY_SIZE(npcm_pinmux_table);
BUILD_ASSERT(ARRAY_SIZE(npcm_pinmux_table) <= UINT8_MAX,
	     "Pinmux table size should be less than 256");

/*
 * Pin peripheral pull-up/down definitions.
 *
 * The table is used to get pull-up/down property of register filed and config for supported alt
 * functions.
 */
struct npcm_periph_pupd_cfg {
	uint16_t id: 7;         /* Pin number */
	uint16_t pupd_sup: 6;   /* Bit N represents the support of pull-up/down of pin group N. */
	uint16_t pupd_type: 2;  /* [0]=pull-up, [1]=pull-down */
	uint16_t pupd_reg0: 11; /* Pull-up/down config reg field index0 */
	uint16_t pupd_reg1: 11; /* Pull-up/down config reg field index1 */
} __packed;

#define NPCM_PERIPH_PUPD_TABLE_REG0_IDX 0
#define NPCM_PERIPH_PUPD_TABLE_REG1_IDX 1

#define NPCM_PIN_PERIPH_PUPD_GROUP_RDATA(pin, sup, type, reg0, reg1)                               \
	{                                                                                          \
		.id = pin,                                                                         \
		.pupd_sup = sup,                                                                   \
		.pupd_type = type,                                                                 \
		.pupd_reg0 = reg0,                                                                 \
		.pupd_reg1 = reg1,                                                                 \
	},
#define NPCM_PIN_PERIPH_PUPD_SUP(node_id, prop)  DT_PROP_BY_IDX(node_id, prop, 0)
#define NPCM_PIN_PERIPH_PUPD_TYPE(node_id)       Z_PINCTRL_NPCM_BIAS_TYPE(node_id)
#define NPCM_PIN_PERIPH_PUPD_REG0(node_id, prop) DT_PROP_BY_IDX(node_id, prop, 0)
#define NPCM_PIN_PERIPH_PUPD_REG1(node_id, prop) DT_PROP_BY_IDX(node_id, prop, 1)
#define NPCM_PIN_PERIPH_PUPD_GROUP_INIT(node_id)                                                   \
	NPCM_PIN_PERIPH_PUPD_GROUP_RDATA(                                                          \
		NPCM_PINMUX_ID(node_id, nuvoton_id),                                               \
		NPCM_PIN_PERIPH_PUPD_SUP(node_id, nuvoton_pull_up_down_sup),                       \
		NPCM_PIN_PERIPH_PUPD_TYPE(node_id),                                                \
		NPCM_PIN_PERIPH_PUPD_REG0(node_id, nuvoton_pull_up_down_reg), NPCM_PIN_REG_NULL)
#define NPCM_PIN_PERIPH_PUPD_GROUP_EXT_INIT(node_id)                                               \
	NPCM_PIN_PERIPH_PUPD_GROUP_RDATA(                                                          \
		NPCM_PINMUX_ID(node_id, nuvoton_id),                                               \
		NPCM_PIN_PERIPH_PUPD_SUP(node_id, nuvoton_pull_up_down_sup),                       \
		NPCM_PIN_PERIPH_PUPD_TYPE(node_id),                                                \
		NPCM_PIN_PERIPH_PUPD_REG0(node_id, nuvoton_pull_up_down_reg),                      \
		NPCM_PIN_PERIPH_PUPD_REG1(node_id, nuvoton_pull_up_down_reg))

#define NPCM_PIN_PERIPH_PUPD_INIT(node_id, prop)                                                   \
	IF_ENABLED( \
		DT_PROP(node_id, nuvoton_pin_hw_def), \
		(IF_ENABLED( \
			DT_NODE_HAS_PROP(node_id, nuvoton_pull_up_down_reg), \
			(COND_CODE_1( \
				DT_PROP(node_id, nuvoton_pull_up_down_config_extend), \
				(NPCM_PIN_PERIPH_PUPD_GROUP_EXT_INIT(node_id)), \
				(NPCM_PIN_PERIPH_PUPD_GROUP_INIT(node_id)) \
				) \
			)) \
		))

static const struct npcm_periph_pupd_cfg npcm_periph_pupd_table[] = {
	DT_INST_FOREACH_CHILD_VARGS(0, NPCM_PIN_PERIPH_PUPD_INIT)};
static const int npcm_periph_pupd_table_size = ARRAY_SIZE(npcm_periph_pupd_table);
BUILD_ASSERT(ARRAY_SIZE(npcm_periph_pupd_table) <= UINT8_MAX,
	     "Pupd table size should be less than 256");

/*
 * Pin voltage level definitions.
 *
 * The table is used to check the current pin voltage level. Not all alt functions in a pin have
 * low-voltage support. The recorded status is only for supported alt functions.
 */
struct npcm_pin_lv_cfg {
	uint16_t id: 7;      /* Pin number */
	uint16_t lv_reg: 11; /* Low-voltage config reg field */
	uint16_t lv_sup: 6;  /* The bit N value means group N supports low-voltage level. */
	uint16_t level: 12;  /* Support normal 3.3V or low-voltage 1.8V */
} __packed;

#define NPCM_PIN_LV_GROUP_RDATA(pin, reg, sup, lv)                                                 \
	{                                                                                          \
		.id = pin,                                                                         \
		.lv_reg = reg,                                                                     \
		.lv_sup = sup,                                                                     \
		.level = lv,                                                                       \
	},
#define NPCM_PIN_LV_REG(node_id, prop)       DT_PROP_BY_IDX(node_id, prop, 0)
#define NPCM_PIN_LV_SUP(node_id, prop)       DT_PROP_BY_IDX(node_id, prop, 0)
#define NPCM_PIN_LV_LOW_LEVEL(node_id, prop) DT_PROP_BY_IDX(node_id, prop, 0)

#define NPCM_PIN_LV_INIT(node_id, prop)                                                            \
	IF_ENABLED( \
		DT_PROP(node_id, nuvoton_pin_hw_def), \
		(IF_ENABLED( \
			DT_NODE_HAS_PROP(node_id, nuvoton_low_voltage_reg), \
			(NPCM_PIN_LV_GROUP_RDATA( \
				NPCM_PINMUX_ID(node_id, nuvoton_id), \
				NPCM_PIN_LV_REG(node_id, nuvoton_low_voltage_reg), \
				NPCM_PIN_LV_SUP(node_id, nuvoton_low_voltage_sup), \
				NPCM_PIN_LV_LOW_LEVEL(node_id, nuvoton_low_voltage_def_mv) \
				) \
			)) \
		))

static struct npcm_pin_lv_cfg npcm_pin_lv_table[] = {
	DT_INST_FOREACH_CHILD_VARGS(0, NPCM_PIN_LV_INIT)};
static const int npcm_pin_lv_table_size = ARRAY_SIZE(npcm_pin_lv_table);
BUILD_ASSERT(ARRAY_SIZE(npcm_pin_lv_table) <= UINT8_MAX,
	     "Pin low-voltage table size should be less than 256");

/* Driver config */
struct npcm_pinctrl_config {
	/* Device base address used for pinctrl driver */
	uintptr_t base_scfg;
	uintptr_t base_glue;
};

static const struct npcm_pinctrl_config npcm_pinctrl_cfg = {
	.base_scfg = DT_INST_REG_ADDR_BY_NAME(0, scfg),
	.base_glue = DT_INST_REG_ADDR_BY_NAME(0, glue),
};

/* SCFG multi-registers */
#define NPCM_SCFG_OFFSET(n)     (((n) >> NPCM_PINCTRL_SHIFT) & NPCM_PINCTRL_MASK)
#define NPCM_SCFG(base, n)      (*(volatile uint8_t *)(base + NPCM_SCFG_OFFSET(n)))
#define NPCM_SCFG_BIT_OFFSET(n) ((n) & NPCM_PINCTRL_BIT_MASK)
#define NPCM_SCFG_SET(base, n)  (NPCM_SCFG(base, n) |= BIT(NPCM_SCFG_BIT_OFFSET(n)))
#define NPCM_SCFG_CLR(base, n)  (NPCM_SCFG(base, n) &= ~BIT(NPCM_SCFG_BIT_OFFSET(n)))

/* Pin number access helper */
#define NPCM_PINCTRL_GET_PORT(pin) (((pin) >> NPCM_PINCTRL_PORT_SHIFT) & NPCM_PINCTRL_PORT_MASK)
#define NPCM_PINCTRL_GET_PIN(pin)  (((pin) >> 0) & NPCM_PINCTRL_PIN_MASK)

/* Voltage level constants */
#define NPCM_VOLT_LV_3V3 3300
#define NPCM_VOLT_LV_1V8 1800

/* Local functions */
static int npcm_periph_pinmux_configure(const struct npcm_pinctrl *pin)
{
	const uintptr_t scfg_base = npcm_pinctrl_cfg.base_scfg;
	uint8_t cfg_value;
	uint16_t reg_value;
	uint8_t i;

	/* Make sure the pin config exists */
	if ((pin->props.id >= npcm_pinmux_table_size) ||
	    (pin->props.group >= NPCM_PINMUX_GROUP_SIZE)) {
		LOG_ERR("No pinmux define of io%X%X", NPCM_PINCTRL_GET_PORT(pin->props.id),
			NPCM_PINCTRL_GET_PIN(pin->props.id));
		return -ENOTSUP;
	}

	/* Check for empty config */
	cfg_value = npcm_pinmux_table[pin->props.id].alt[pin->props.group];
	if (cfg_value == NPCM_PIN_CFG_NULL) {
		return 0;
	}

	/* Set bit value in alt */
	for (i = 0; i < NPCM_PINMUX_REG_SIZE; i++) {
		reg_value = npcm_pinmux_table[pin->props.id].pin_reg[i];
		if (reg_value == NPCM_PIN_REG_NULL) {
			continue;
		}

		if (cfg_value & BIT(i)) {
			NPCM_SCFG_SET(scfg_base, reg_value);
		} else {
			NPCM_SCFG_CLR(scfg_base, reg_value);
		}
	}

	return 0;
}

static void npcm_periph_pupd_reg_set(uint16_t bias_type_set, uint16_t io, uint8_t reg_idx)
{
	const uintptr_t scfg_base = npcm_pinctrl_cfg.base_scfg;
	uint16_t pupd_reg;
	uint16_t pupd_type;

	/* Get pupd register and type */
	pupd_type = npcm_periph_pupd_table[io].pupd_type;
	if (reg_idx == NPCM_PERIPH_PUPD_TABLE_REG0_IDX) {
		pupd_reg = npcm_periph_pupd_table[io].pupd_reg0;
	} else if (reg_idx == NPCM_PERIPH_PUPD_TABLE_REG1_IDX) {
		pupd_reg = npcm_periph_pupd_table[io].pupd_reg1;
	} else {
		/* Invalid index */
		LOG_ERR("Pupd reg index %d is invalid", reg_idx);
		return;
	}

	/* Set register */
	if (pupd_reg != NPCM_PIN_REG_NULL) {
		if (bias_type_set == NPCM_BIAS_TYPE_NONE) {
			NPCM_SCFG_CLR(scfg_base, pupd_reg);
		} else if (bias_type_set == pupd_type) {
			NPCM_SCFG_SET(scfg_base, pupd_reg);
		} else {
			LOG_WRN("Pupd type %d is invalid", bias_type_set);
		}
	}
}

static int npcm_periph_pupd_configure(const struct npcm_pinctrl *pin)
{
	const struct npcm_periph_pupd *pupd = &pin->cfg.pupd;
	uint8_t sup_idx = 0;
	uint8_t i;
	uint8_t j;

	/* Check if pin supports low-voltage */
	for (i = 0; i < npcm_periph_pupd_table_size; i++) {
		if (npcm_periph_pupd_table[i].id == pin->props.id) {
			break;
		}
	}
	if (i == npcm_periph_pupd_table_size) {
		LOG_WRN("No pupd support of io%X%X", NPCM_PINCTRL_GET_PORT(pin->props.id),
			NPCM_PINCTRL_GET_PIN(pin->props.id));
		return -ENOTSUP;
	}

	/* Check the config index of pupd and set */
	for (j = 0; j < NPCM_PINMUX_GROUP_SIZE; j++) {
		if (npcm_periph_pupd_table[i].pupd_sup & BIT(j)) {
			if (j == pin->props.group) {
				npcm_periph_pupd_reg_set(pupd->io_bias_type, i, sup_idx);
				return 0;
			}
			sup_idx++; /* The index of pupd_reg in pupd table  */
		}
	}

	LOG_WRN("No pupd support of io%X%X pinmux group%d", pin->props.group,
		NPCM_PINCTRL_GET_PORT(pin->props.id), NPCM_PINCTRL_GET_PIN(pin->props.id));
	return -ENOTSUP;
}

static int npcm_device_control_configure(const struct npcm_pinctrl *pin)
{
	const uintptr_t scfg_base = npcm_pinctrl_cfg.base_scfg;
	const struct npcm_dev_ctl *ctrl = &pin->cfg.dev_ctl;

	if (ctrl->is_set == 0) {
		NPCM_SCFG_CLR(scfg_base, ctrl->reg_id);
	} else {
		NPCM_SCFG_SET(scfg_base, ctrl->reg_id);
	}

	return 0;
}

static int npcm_volt_level_configure(const struct npcm_pinctrl *pin)
{
	const struct npcm_lvol *lvol = &pin->cfg.lvol;
	const uintptr_t scfg_base = npcm_pinctrl_cfg.base_scfg;
	uint16_t level = lvol->level;
	uint8_t i;

	/* Check voltage level is the supported 3.3V or 1.8V */
	if ((level != NPCM_VOLT_LV_3V3) && (level != NPCM_VOLT_LV_1V8)) {
		LOG_WRN("Invalid voltage level %d of io%X%X", level,
			NPCM_PINCTRL_GET_PORT(pin->props.id), NPCM_PINCTRL_GET_PIN(pin->props.id));
		return -EINVAL;
	}

	/* Check if pin supports low-voltage */
	for (i = 0; i < npcm_pin_lv_table_size; i++) {
		if (npcm_pin_lv_table[i].id == pin->props.id) {
			break;
		}
	}
	if (i == npcm_pin_lv_table_size) {
		LOG_WRN("No lvol support of io%X%X", NPCM_PINCTRL_GET_PORT(pin->props.id),
			NPCM_PINCTRL_GET_PIN(pin->props.id));
		return -ENOTSUP;
	}

	/* Only configure low-voltage supported alt functions of the pin */
	if (npcm_pin_lv_table[i].lv_sup & BIT(pin->props.group)) {
		/* Set different level */
		if (npcm_pin_lv_table[i].level != level) {
			if (level == NPCM_VOLT_LV_1V8) {
				NPCM_SCFG_SET(scfg_base, npcm_pin_lv_table[i].lv_reg);
			} else {
				NPCM_SCFG_CLR(scfg_base, npcm_pin_lv_table[i].lv_reg);
			}

			/* Record pin level */
			npcm_pin_lv_table[i].level = level;
		}
	}

	return 0;
}

/* Pinctrl API implementation */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	uint8_t i;
	int ret = 0;

	/* Configure all peripheral devices' properties here. */
	for (i = 0; i < pin_cnt; i++) {
		switch (pins[i].props.type) {
		case NPCM_PINCTRL_TYPE_PERIPH_PINMUX:
			/* Configure peripheral device's pinmux functionality */
			ret = npcm_periph_pinmux_configure(&pins[i]);
			break;

		case NPCM_PINCTRL_TYPE_PERIPH_PUPD:
			/* Configure peripheral device's internal PU/PD */
			ret = npcm_periph_pupd_configure(&pins[i]);
			break;

		case NPCM_PINCTRL_TYPE_DEVICE_CTRL:
			/* Configure device's IO characteristics */
			ret = npcm_device_control_configure(&pins[i]);
			break;

		case NPCM_PINCTRL_TYPE_LVOL:
			/* Configure device's IO level */
			ret = npcm_volt_level_configure(&pins[i]);
			break;

		default:
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}
