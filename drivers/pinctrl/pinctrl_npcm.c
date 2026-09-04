/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pinctrl_npcm, CONFIG_PINCTRL_LOG_LEVEL);

/*
 * The pin function table is indexed by the child order of the
 * nuvoton,npcm-pinfunc node. A pin configuration selects an entry through
 * a pinmux phandle. Each entry describes the SCFG bits used to select the
 * function and its optional pull and low-voltage controls.
 */
#define NPCM_PINFUNC_NODE      DT_COMPAT_GET_ANY_STATUS_OKAY(nuvoton_npcm_pinfunc)
#define NPCM_PINFUNC_ALT_CELLS 15 /* up to five <reg bit value> triplets */

struct npcm_pinfunc {
	uint8_t alts[NPCM_PINFUNC_ALT_CELLS]; /* <reg bit value> triplets */
	uint8_t nalts;                        /* cells used in alts */
	uint8_t pupd[2];                      /* <reg bit> of the pull enable */
	uint8_t pupd_dir;                     /* NPCM_BIAS_PULL_UP/DOWN, NONE if absent */
	uint8_t lvol[2];                      /* <reg bit> of the low-voltage select */
	uint8_t has_lvol;
};

/*
 * Every SCFG offset the devicetree names is stored in a uint8_t, so a wider
 * value would silently address a different register of the same block.
 */
#define NPCM_CELL_CHECK(node_id, prop, idx)                                                        \
	BUILD_ASSERT(DT_PROP_BY_IDX(node_id, prop, idx) <= UINT8_MAX,                              \
		     DT_NODE_PATH(node_id) ": " #prop " cell does not fit in 8 bits");
#define NPCM_PROP_CELLS_CHECK(node_id, prop)                                                       \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, prop),                                                \
		   (DT_FOREACH_PROP_ELEM(node_id, prop, NPCM_CELL_CHECK)))

#define NPCM_PINFUNC_CHECK(node_id)                                                                \
	BUILD_ASSERT(DT_PROP_LEN_OR(node_id, nuvoton_alts, 0) % 3 == 0,                            \
		     DT_NODE_PATH(node_id) ": nuvoton,alts must be <reg bit value> triplets");     \
	BUILD_ASSERT(DT_PROP_LEN_OR(node_id, nuvoton_alts, 0) <= NPCM_PINFUNC_ALT_CELLS,           \
		     DT_NODE_PATH(node_id) ": too many nuvoton,alts cells");                       \
	BUILD_ASSERT(!(DT_NODE_HAS_PROP(node_id, nuvoton_pull_up_reg) &&                           \
		       DT_NODE_HAS_PROP(node_id, nuvoton_pull_down_reg)),                          \
		     DT_NODE_PATH(node_id) ": a function pulls one way only");                     \
	NPCM_PROP_CELLS_CHECK(node_id, nuvoton_alts)                                               \
	NPCM_PROP_CELLS_CHECK(node_id, nuvoton_pull_up_reg)                                        \
	NPCM_PROP_CELLS_CHECK(node_id, nuvoton_pull_down_reg)                                      \
	NPCM_PROP_CELLS_CHECK(node_id, nuvoton_low_voltage_reg)

/* Initialize optional fields only when the corresponding property is present. */
#define NPCM_PINFUNC_ALTS(node_id)                                                                 \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, nuvoton_alts),                                        \
		   (.alts = DT_PROP(node_id, nuvoton_alts),))
#define NPCM_PINFUNC_PUPD(node_id, prop, dir)                                                      \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, prop),                                                \
		   (.pupd = DT_PROP(node_id, prop), .pupd_dir = dir,))
#define NPCM_PINFUNC_LVOL(node_id)                                                                 \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, nuvoton_low_voltage_reg),                             \
		   (.lvol = DT_PROP(node_id, nuvoton_low_voltage_reg), .has_lvol = 1,))

/* clang-format off */
#define NPCM_PINFUNC_ENTRY(node_id)                                                                \
	[DT_NODE_CHILD_IDX(node_id)] = {                                                           \
		.nalts = DT_PROP_LEN_OR(node_id, nuvoton_alts, 0),                                 \
		NPCM_PINFUNC_ALTS(node_id)                                                         \
		NPCM_PINFUNC_PUPD(node_id, nuvoton_pull_up_reg, NPCM_BIAS_PULL_UP)                 \
		NPCM_PINFUNC_PUPD(node_id, nuvoton_pull_down_reg, NPCM_BIAS_PULL_DOWN)             \
		NPCM_PINFUNC_LVOL(node_id)                                                         \
	},
/* clang-format on */

DT_FOREACH_CHILD(NPCM_PINFUNC_NODE, NPCM_PINFUNC_CHECK)

static const struct npcm_pinfunc npcm_pinfunc_table[] = {
	DT_FOREACH_CHILD(NPCM_PINFUNC_NODE, NPCM_PINFUNC_ENTRY)};

BUILD_ASSERT(ARRAY_SIZE(npcm_pinfunc_table) <= NPCM_PINCTRL_MAX_FUNCS,
	     "Too many pin functions for the pinctrl_soc_pin_t index field");

/* A pin group can also name an SCFG bit of its own, held in the same widths. */
#define NPCM_DEVCTL_CHECK(node_id)                                                                 \
	NPCM_PROP_CELLS_CHECK(node_id, nuvoton_device_control)                                     \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, nuvoton_device_control),                              \
		   (BUILD_ASSERT(DT_PROP_BY_IDX(node_id, nuvoton_device_control, 1) < 8,           \
				 DT_NODE_PATH(node_id) ": device-control bit is out of range");))
#define NPCM_PINGROUP_CHECK(node_id) DT_FOREACH_CHILD(node_id, NPCM_DEVCTL_CHECK)

/* Nested walks must use distinct macros; the preprocessor will not re-enter one. */
DT_INST_FOREACH_CHILD_SEP(0, NPCM_PINGROUP_CHECK, ())

/* Base address of the System Configuration (SCFG) register block. */
static const uintptr_t npcm_scfg_base = DT_REG_ADDR(DT_INST_PHANDLE(0, nuvoton_scfg));

static void npcm_scfg_write_bit(uint8_t reg, uint8_t bit, bool set)
{
	mm_reg_t addr = npcm_scfg_base + reg;
	uint8_t val = sys_read8(addr);

	sys_write8(set ? (val | BIT(bit)) : (val & ~BIT(bit)), addr);
}

static int npcm_pinfunc_configure(const struct npcm_pinctrl *pin)
{
	const struct npcm_pinfunc *fn = &npcm_pinfunc_table[pin->fn];

	/* Program every selector in the function's row. */
	for (uint8_t i = 0; i < fn->nalts; i += 3) {
		npcm_scfg_write_bit(fn->alts[i], fn->alts[i + 1], fn->alts[i + 2]);
	}

	if (pin->bias != NPCM_BIAS_NONE) {
		if (fn->pupd_dir == NPCM_BIAS_NONE) {
			LOG_ERR("pin function %u has no pull resistor", pin->fn);
			return -ENOTSUP;
		}
		if (pin->bias != NPCM_BIAS_DISABLE && pin->bias != fn->pupd_dir) {
			LOG_ERR("pin function %u resistor pulls the other way", pin->fn);
			return -ENOTSUP;
		}
		npcm_scfg_write_bit(fn->pupd[0], fn->pupd[1], pin->bias != NPCM_BIAS_DISABLE);
	}

	if (pin->volt != NPCM_VOLT_NONE) {
		if (!fn->has_lvol) {
			LOG_ERR("pin function %u has no low-voltage select", pin->fn);
			return -ENOTSUP;
		}
		/*
		 * A 1.8 V pin requires its internal pull to be disabled, which
		 * is the reset state of PxPULL.
		 */
		npcm_scfg_write_bit(fn->lvol[0], fn->lvol[1], pin->volt == NPCM_VOLT_1V8);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		int ret;

		if (pins[i].type == NPCM_PINCTRL_TYPE_DEVCTL) {
			npcm_scfg_write_bit(pins[i].ctl_reg, pins[i].ctl_bit, pins[i].ctl_val);
			continue;
		}

		ret = npcm_pinfunc_configure(&pins[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
