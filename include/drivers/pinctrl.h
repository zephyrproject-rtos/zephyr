/*
 * Copyright (c) 2021 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for PINCTRL drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_

#include <zephyr/types.h>
#include <toolchain.h>
#include <soc_pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PINCTRL Driver APIs
 * @defgroup pinctrl_interface PINCTRL Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @typedef pinctrl_soc_pins_t
 *
 * @brief Provides a type to hold PINCTRL information specified in devicetree
 *
 * This type is sufficient to hold a PINCTRL device pointer, pin number,
 * and the subset of the flags used to control PINCTRL configuration
 * which may be given in devicetree.
 */

struct pinctrl_state {
	const pinctrl_soc_pins_t *pins;
	size_t num_pins;
};

/** Pin controller configuration for a given device. */
struct pinctrl_config {
	/** List of states. */
	const struct pinctrl_state *states;
	/** Number of states. */
	size_t states_cnt;
};

/**
 * @brief Obtain the variable name storing pinctrl config for the given DT node
 * identifier.
 *
 * @param node Node identifier.
 */

/**
 * @def PINCTRL_DT_SPEC_GET(node, n)
 *
 * @brief Static initializer for a @p pinctrl_dt_pin_spec
 *
 * This returns a static initializer for a @p pinctrl_dt_pin_spec structure
 * given a devicetree node identifier and a state ID.
 *
 * Example devicetree fragment:
 *
 *	nodelabel: node {
 *		pinctrl-0 = <&pincfg_a>, <&pincfg_b>, <&pincfg_c>;
 *		pinctrl-1 = <&pincfg_ax>, <&pincfg_bx>;
 *	}
 *
 * Example usage:
 *
 * const pinctrl_soc_pins_t spec[] =
 *	PINCTRL_DT_SPEC_GET(DT_NODELABEL(nodelabel), 0);
 *
 * It is an error to use this macro unless the node exists, has the given
 * property, and that property specifies a list of valid phandles to pin
 * configuration node.
 *
 * @param node devicetree node identifier
 * @param n an integer id that corresponds to pinctrl-n property.
 * @return static initializer for an array of `pinctrl_soc_pins_t`
 */

/**
 * @brief Configure a single pin from a @p pinctrl_dt_pin_spec.
 *
 * @param pin_spec Pin configuration obtained from the device tree.
 *
 * @retval 0 If successful.
 */
__syscall int pinctrl_pin_configure(const pinctrl_soc_pins_t *pin_spec);

int z_impl_pinctrl_pin_configure(const pinctrl_soc_pins_t *pin_spec);

/**
 * @brief Configure pin list from a @p pinctrl_dt_pin_spec array.
 *
 * @param pin_spec An array of pin spec elements obtained from the device tree.
 * @param pin_spec_length number of elements in pin_spec array.
 *
 * @retval 0 If successful.
 */
static inline int pinctrl_pin_list_configure(
		const pinctrl_soc_pins_t pin_spec[],
		size_t pin_spec_length)
{
	int ret = 0;

	for (size_t i = 0; i < pin_spec_length; i++) {
		ret = pinctrl_pin_configure(&pin_spec[i]);
		if (ret != 0) {
			break;
		}
	}

	return ret;
}

static inline int pinctrl_set_pin_state(const struct pinctrl_state pins)
{
	return pinctrl_pin_list_configure(pins.pins, pins.num_pins);
}

#define Z_PINCTRL_DT_PER_PIN(node, prop, idx) 				\
	PINCTRL_SOC_PINS_ELEM_INIT(DT_PROP_BY_IDX(node, prop, idx))

/* Allow soc_pinctrl.h to override Z_PINCTRL_DT_SPEC_GET */
#ifndef Z_PINCTRL_DT_SPEC_GET
#define Z_PINCTRL_DT_SPEC_GET(node, n) 					\
{ DT_FOREACH_PROP_ELEM(node, pinctrl_##n, Z_PINCTRL_DT_PER_PIN) }
#endif

/* Name of form soc_pins_<PIN_IDX>_dts_ord_<DEP_ORD> */
#define Z_PINCTRL_DT_SOC_PINS_NAME(pin_idx, node) 			\
	_CONCAT(soc_pins_, _CONCAT(pin_idx, DEVICE_DT_NAME_GET(node)))

#define Z_PINCTRL_DT_SOC_PINS_DEFINE(pin_idx, node)			\
static const pinctrl_soc_pins_t Z_PINCTRL_DT_SOC_PINS_NAME(pin_idx, node)[] = \
	Z_PINCTRL_DT_SPEC_GET(node, pin_idx);

#define Z_PINCTRL_DT_SOC_PINS_INIT(pin_idx, node)			\
{									\
	.num_pins = ARRAY_SIZE(Z_PINCTRL_DT_SOC_PINS_NAME(pin_idx, node)),\
	.pins = Z_PINCTRL_DT_SOC_PINS_NAME(pin_idx, node),		\
},

#define Z_PINCTRL_DT_STATES_NAME(node) 					\
	_CONCAT(__pinctrl_states, DEVICE_DT_NAME_GET(node))

#define Z_PINCTRL_DT_STATES_DEFINE(node)				\
	UTIL_LISTIFY(DT_NUM_PINCTRL_STATES(node),			\
		     Z_PINCTRL_DT_SOC_PINS_DEFINE, node)		\
	static const struct pinctrl_state				\
	Z_PINCTRL_DT_STATES_NAME(node)[] = {				\
		UTIL_LISTIFY(DT_NUM_PINCTRL_STATES(node),		\
			     Z_PINCTRL_DT_SOC_PINS_INIT, node)		\
	}

#define PINCTRL_DT_CONFIG_NAME(node)					\
	_CONCAT(__pinctrl_config, DEVICE_DT_NAME_GET(node))

#define PINCTRL_DT_INST_CONFIG_NAME(inst) 				\
	PINCTRL_DT_CONFIG_NAME(DT_DRV_INST(inst))

#define PINCTRL_DT_CONFIG_DEFINE(node)					\
	Z_PINCTRL_DT_STATES_DEFINE(node);				\
	static const struct pinctrl_config PINCTRL_DT_CONFIG_NAME(node) = \
	{								\
		.states = Z_PINCTRL_DT_STATES_NAME(node),		\
		.states_cnt = DT_NUM_PINCTRL_STATES(node),		\
	}

#define PINCTRL_DT_INST_CONFIG_DEFINE(inst)				\
	PINCTRL_DT_CONFIG_DEFINE(DT_DRV_INST(inst))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/pinctrl.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_ */
