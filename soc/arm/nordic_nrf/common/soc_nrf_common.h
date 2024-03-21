/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Common soc.h include for Nordic nRF5 SoCs.
 */

#ifndef _ZEPHYR_SOC_ARM_NORDIC_NRF_SOC_NRF_COMMON_H_
#define _ZEPHYR_SOC_ARM_NORDIC_NRF_SOC_NRF_COMMON_H_

#ifndef _ASMLANGUAGE
#include <nrfx.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

/**
 * @brief Get a PSEL value out of a foo-gpios or foo-pin devicetree property
 *
 * Many Nordic bindings have 'foo-pin' properties to specify a pin
 * configuration as a PSEL value directly instead of using a 'foo-gpios'
 * <&gpioX Y flags> style controller phandle + GPIO specifier.
 *
 * It would be better to use 'foo-gpios' properties instead. This type
 * of property is more in line with the recommended DT encoding for GPIOs.
 *
 * To allow for a smooth migration from 'foo-pin' to 'foo-gpios', this
 * helper macro can be used to get a PSEL value out of the devicetree
 * using whichever one of 'foo-gpios' or 'foo-pin' is in the DTS.
 *
 * Note that you can also use:
 *
 *   - NRF_DT_PSEL_CHECK_*() to check the property configuration at build time
 *   - NRF_DT_GPIOS_TO_PSEL() if you only have a 'foo-gpios'
 *
 * @param node_id node identifier
 * @param psel_prop lowercase-and-underscores old-style 'foo-pin' property
 * @param gpios_prop new-style 'foo-gpios' property
 * @param default_val the value returned if neither is set
 * @return PSEL register value taken from psel_prop or gpios_prop, whichever
 *         is present in the DTS. If gpios_prop is present, it is converted
 *         to a PSEL register value first.
 */
#define NRF_DT_PSEL(node_id, psel_prop, gpios_prop, default_val)	\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, psel_prop),		\
		    (DT_PROP(node_id, psel_prop)),			\
		    (COND_CODE_1(					\
			    DT_NODE_HAS_PROP(node_id, gpios_prop),	\
			    (NRF_DT_GPIOS_TO_PSEL(node_id,		\
						  gpios_prop)),		\
			    (default_val))))

/**
 * Error out the build if the devicetree node with identifier
 * 'node_id' has both a legacy psel-style property and a gpios
 * property.
 *
 * Otherwise, do nothing.
 *
 * @param node_id node identifier
 * @param psel_prop lowercase-and-underscores PSEL style property
 * @param psel_prop_name human-readable string name of psel_prop
 * @param gpios_prop lowercase-and-underscores foo-gpios style property
 * @param gpio_prop_name human-readable string name of gpios_prop
 */
#define NRF_DT_PSEL_CHECK_NOT_BOTH(node_id, psel_prop, psel_prop_name,	\
				   gpios_prop, gpios_prop_name)		\
	BUILD_ASSERT(							\
		!(DT_NODE_HAS_PROP(node_id, psel_prop) &&		\
		  DT_NODE_HAS_PROP(node_id, gpios_prop)),		\
		"Devicetree node " DT_NODE_PATH(node_id)		\
		" has both of the " psel_prop_name			\
		" and " gpios_prop_name					\
		" properties set; you must remove one. "		\
		"Note: you can use /delete-property/ to delete properties.")

/**
 * Like NRF_DT_PSEL_CHECK_NOT_BOTH, but instead checks that exactly one
 * of the properties is set.
 */
#define NRF_DT_PSEL_CHECK_EXACTLY_ONE(node_id,				\
				      psel_prop, psel_prop_name,	\
				      gpios_prop, gpios_prop_name)	\
	BUILD_ASSERT(							\
		(DT_NODE_HAS_PROP(node_id, psel_prop) ^			\
		 DT_NODE_HAS_PROP(node_id, gpios_prop)),		\
		"Devicetree node " DT_NODE_PATH(node_id)		\
		" must have exactly one of the " psel_prop_name		\
		" and " gpios_prop_name					\
		" properties set. "					\
		"Note: you can use /delete-property/ to delete properties.")

/**
 * @brief Convert a devicetree GPIO phandle+specifier to PSEL value
 *
 * Various peripherals in nRF SoCs have pin select registers, which
 * usually have PSEL in their names. The low bits of these registers
 * generally look like this in the register map description:
 *
 *     Bit number     5 4 3 2 1 0
 *     ID             B A A A A A
 *
 *     ID  Field  Value    Description
 *     A   PIN    [0..31]  Pin number
 *     B   PORT   [0..1]   Port number
 *
 * Examples:
 *
 * - pin P0.4 has "PSEL value" 4 (B=0 and A=4)
 * - pin P1.5 has "PSEL value" 37 (B=1 and A=5)
 *
 * This macro converts a devicetree GPIO phandle array value
 * "<&gpioX pin ...>" to a "PSEL value".
 *
 * Note: in Nordic SoC devicetrees, "gpio0" means P0, and "gpio1"
 * means P1. This is encoded in the "port" property of each GPIO node.
 *
 * Examples:
 *
 *     foo: my-node {
 *             tx-gpios = <&gpio0 4 ...>;
 *             rx-gpios = <&gpio0 5 ...>, <&gpio1 5 ...>;
 *     };
 *
 *     NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_NODELABEL(foo), tx_gpios, 0) // 0 + 4 = 4
 *     NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_NODELABEL(foo), rx_gpios, 1) // 32 + 5 = 37
 */
#define NRF_DT_GPIOS_TO_PSEL_BY_IDX(node_id, prop, idx)			\
	((DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, port) << 5) |	\
	 (DT_GPIO_PIN_BY_IDX(node_id, prop, idx) & 0x1F))


/**
 * @brief Equivalent to NRF_DT_GPIOS_TO_PSEL_BY_IDX(node_id, prop, 0)
 */
#define NRF_DT_GPIOS_TO_PSEL(node_id, prop)			\
	NRF_DT_GPIOS_TO_PSEL_BY_IDX(node_id, prop, 0)

/**
 * If the node has the property, expands to
 * NRF_DT_GPIOS_TO_PSEL(node_id, prop). The default_value argument is
 * not expanded in this case.
 *
 * Otherwise, expands to default_value.
 */
#define NRF_DT_GPIOS_TO_PSEL_OR(node_id, prop, default_value)	\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),		\
		    (NRF_DT_GPIOS_TO_PSEL(node_id, prop)),	\
		    (default_value))

/**
 * Error out the build if 'prop' is set on node 'node_id' and
 * DT_GPIO_CTLR(node_id, prop) is not an SoC GPIO controller,
 * i.e. a node with compatible "nordic,nrf-gpio".
 *
 * Otherwise, do nothing.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores PSEL style property
 * @param prop_name human-readable string name for 'prop'
 */
#define NRF_DT_CHECK_GPIO_CTLR_IS_SOC(node_id, prop, prop_name)		\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),			\
		   (BUILD_ASSERT(DT_NODE_HAS_COMPAT(			\
					 DT_GPIO_CTLR(node_id, prop),	\
					 nordic_nrf_gpio),		\
				 "Devicetree node "			\
				 DT_NODE_PATH(node_id)			\
				 " property " prop_name			\
				 " must refer to a GPIO controller "	\
				 "with compatible nordic,nrf-gpio; "	\
				 "got "					\
				 DT_NODE_PATH(DT_GPIO_CTLR(node_id,	\
							   prop))	\
				 ", which does not have this "		\
				 "compatible")),			\
		    (BUILD_ASSERT(1,					\
				  "NRF_DT_CHECK_GPIO_CTLR_IS_SOC: OK")))
/* Note: allow a trailing ";" either way */

/**
 * Error out the build if CONFIG_PM_DEVICE=y and pinctrl-1 state (sleep) is not
 * defined.
 *
 * @param node_id node identifier
 */
#define NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(node_id)			       \
	BUILD_ASSERT(!IS_ENABLED(CONFIG_PM_DEVICE) ||			       \
		     DT_PINCTRL_HAS_NAME(node_id, sleep),		       \
		     DT_NODE_PATH(node_id) " defined without sleep state")

#endif /* !_ASMLANGUAGE */

#endif
