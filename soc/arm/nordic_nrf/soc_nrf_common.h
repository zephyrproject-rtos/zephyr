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
#include <devicetree.h>
#include <toolchain.h>

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
 *             rx-gpios = <&gpio1 5 ...>;
 *     };
 *
 *     NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(foo), tx_gpios) // 0 + 4 = 4
 *     NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(foo), rx_gpios) // 32 + 5 = 37
 */
#define NRF_DT_GPIOS_TO_PSEL(node_id, prop)				\
	(DT_GPIO_PIN(node_id, prop) +					\
	 (DT_PROP_BY_PHANDLE(node_id, prop, port) << 5))

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
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node_id, prop) ||		\
		     DT_NODE_HAS_COMPAT(DT_GPIO_CTLR(node_id, prop),	\
					nordic_nrf_gpio),		\
		     "Devicetree node " DT_NODE_PATH(node_id)		\
		     " property " prop_name " must refer to a GPIO "	\
		     " controller with compatible nordic,nrf-gpio; "	\
		     "got " DT_NODE_PATH(DT_GPIO_CTLR(node_id, prop))	\
		     ", which does not have this compatible")

#endif /* !_ASMLANGUAGE */

#endif
