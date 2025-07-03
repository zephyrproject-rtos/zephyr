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
 * @brief Convert a devicetree GPIO phandle+specifier to GPIOTE instance number.
 *
 * Some of nRF SoCs may have more instances of GPIOTE.
 * To handle this, we use the "gpiote-instance" property of the GPIO node.
 *
 * This macro converts a devicetree GPIO phandle array value
 * "<&gpioX pin ...>" to a GPIOTE instance number.
 *
 * Examples:
 *
 *     &gpiote0 {
 *             instance = <0>;
 *     };
 *
 *     &gpiote20 {
 *             instance = <20>;
 *     };
 *
 *     &gpio0 {
 *             gpiote-instance = <&gpiote0>;
 *     }
 *
 *     &gpio1 {
 *             gpiote-instance = <&gpiote20>;
 *     }
 *
 *     foo: my-node {
 *             tx-gpios = <&gpio0 4 ...>;
 *             rx-gpios = <&gpio0 5 ...>, <&gpio1 5 ...>;
 *     };
 *
 *     NRF_DT_GPIOTE_INST_BY_IDX(DT_NODELABEL(foo), tx_gpios, 0) // = 0
 *     NRF_DT_GPIOTE_INST_BY_IDX(DT_NODELABEL(foo), rx_gpios, 1) // = 20
 */
#define NRF_DT_GPIOTE_INST_BY_IDX(node_id, prop, idx)			\
	DT_PROP(DT_PHANDLE(DT_GPIO_CTLR_BY_IDX(node_id, prop, idx),	\
			   gpiote_instance),				\
		instance)

/**
 * @brief Equivalent to NRF_DT_GPIOTE_INST_BY_IDX(node_id, prop, 0)
 */
#define NRF_DT_GPIOTE_INST(node_id, prop)				\
	NRF_DT_GPIOTE_INST_BY_IDX(node_id, prop, 0)

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

/**
 * Error out the build if CONFIG_HAS_NORDIC_DMM=y and memory-regions property is not defined
 * or the status of the selected memory region is not "okay"
 *
 * @param node Devicetree node.
 */
#define NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(node_id)					 \
	IF_ENABLED(CONFIG_HAS_NORDIC_DMM,							 \
		(BUILD_ASSERT((									 \
			DT_NODE_HAS_PROP(node_id, memory_regions) &&				 \
			DT_NODE_HAS_STATUS_OKAY(DT_PHANDLE_BY_IDX(node_id, memory_regions, 0))), \
		DT_NODE_PATH(node_id) " defined without memory regions")))

/** @brief Get clock frequency that is used for the given node.
 *
 * Macro checks if node has clock property and if yes then if clock has clock_frequency property
 * then it is returned. If it has supported_clock_frequency property with the list of supported
 * frequencies then the last one is returned with assumption that they are ordered and the last
 * one is the highest. If node does not have clock then 16 MHz is returned which is the default
 * frequency.
 *
 * @param node Devicetree node.
 *
 * @return Frequency of the clock that is used for the node.
 */
#define NRF_PERIPH_GET_FREQUENCY(node) \
	COND_CODE_1(DT_CLOCKS_HAS_IDX(node, 0),							\
		(COND_CODE_1(DT_NODE_HAS_PROP(DT_CLOCKS_CTLR(node), clock_frequency),		\
			     (DT_PROP(DT_CLOCKS_CTLR(node), clock_frequency)),			\
			     (DT_PROP_LAST(DT_CLOCKS_CTLR(node), supported_clock_frequency)))),	\
		(NRFX_MHZ_TO_HZ(16)))

/**
 * @brief Utility macro to check if instance is fast by node, expands to 1 or 0.
 *
 * @param node_id Node identifier.
 */
#define NRF_DT_IS_FAST(node_id)									\
	COND_CODE_1(										\
		UTIL_AND(									\
			DT_NODE_EXISTS(DT_PHANDLE(node_id, power_domains)),			\
			DT_NODE_EXISTS(DT_NODELABEL(gdpwr_fast_active_1))			\
		),										\
		(										\
			DT_SAME_NODE(								\
				DT_PHANDLE(node_id, power_domains),				\
				DT_NODELABEL(gdpwr_fast_active_1)				\
			)									\
		),										\
		(0)										\
	)

/**
 * @brief Utility macro to check if instance is fast by DT_DRV_INST, expands to 1 or 0.
 *
 * @param inst Driver instance
 */
#define NRF_DT_INST_IS_FAST(inst) \
	NRF_DT_IS_FAST(DT_DRV_INST(inst))

/**
 * @brief Utility macro to check if instance is fast by DT_DRV_INST, expands to 1 or empty.
 *
 * @param inst Driver instance
 */
#define NRF_DT_INST_IS_FAST_OR_EMPTY(inst) \
	IF_ENABLED(NRF_DT_INST_IS_FAST(inst), 1)

/**
 * @brief Utility macro to check if any instance with compat is fast. Expands to 1 or 0.
 */
#define NRF_DT_INST_ANY_IS_FAST									\
	COND_CODE_0(										\
		IS_EMPTY(DT_INST_FOREACH_STATUS_OKAY(NRF_DT_INST_IS_FAST_OR_EMPTY)),		\
		(1),										\
		(0)										\
	)

#endif /* !_ASMLANGUAGE */

#endif
