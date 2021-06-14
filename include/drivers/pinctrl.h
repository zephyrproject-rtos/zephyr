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
 * @typedef pinctrl_dt_pin_spec_t
 *
 * @brief Provides a type to hold PINCTRL information specified in devicetree
 *
 * This type is sufficient to hold a PINCTRL device pointer, pin number,
 * and the subset of the flags used to control PINCTRL configuration
 * which may be given in devicetree.
 */

/**
 * @def PINCTRL_DT_SPEC_GET(node_id, n)
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
 * const pinctrl_dt_pin_spec_t spec[] =
 *	PINCTRL_DT_SPEC_GET(DT_NODELABEL(nodelabel), 0);
 *
 * It is an error to use this macro unless the node exists, has the given
 * property, and that property specifies a list of valid phandles to pin
 * configuration node.
 *
 * @param node_id devicetree node identifier
 * @param n an integer id that corresponds to pinctrl-n property.
 * @return static initializer for an array of `pinctrl_dt_pin_spec_t`
 */

/**
 * @brief Configure a single pin from a @p pinctrl_dt_pin_spec.
 *
 * @param pin_spec Pin configuration obtained from the device tree.
 *
 * @retval 0 If successful.
 */
__syscall int pinctrl_pin_configure(const pinctrl_dt_pin_spec_t *pin_spec);

int z_impl_pinctrl_pin_configure(const pinctrl_dt_pin_spec_t *pin_spec);

/**
 * @brief Configure pin list from a @p pinctrl_dt_pin_spec array.
 *
 * @param pin_spec An array of pin spec elements obtained from the device tree.
 * @param pin_spec_length number of elements in pin_spec array.
 *
 * @retval 0 If successful.
 */
static inline int pinctrl_pin_list_configure(
		const pinctrl_dt_pin_spec_t pin_spec[],
		unsigned int pin_spec_length)
{
	int ret = 0;

	for (int i = 0; i < pin_spec_length; i++) {
		ret = pinctrl_pin_configure(&pin_spec[i]);
		if (ret != 0) {
			break;
		}
	}

	return ret;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/pinctrl.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_ */
