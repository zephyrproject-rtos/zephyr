/**
 * @file
 * @brief MBOX Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MBOX_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MBOX_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-mbox Devicetree MBOX API
 * @ingroup devicetree
 * @ingroup mbox_interface
 * @{
 */

/**
 * @brief Get the node identifier for the MBOX controller from a mboxes
 *	  property by name
 *
 * Example devicetree fragment:
 *
 *     mbox1: mbox-controller@... { ... };
 *
 *     n: node {
 *             mboxes = <&mbox1 8>,
 *                      <&mbox1 9>;
 *             mbox-names = "tx", "rx";
 *     };
 *
 * Example usage:
 *
 *     DT_MBOX_CTLR_BY_NAME(DT_NODELABEL(n), tx) // DT_NODELABEL(mbox1)
 *     DT_MBOX_CTLR_BY_NAME(DT_NODELABEL(n), rx) // DT_NODELABEL(mbox1)
 *
 * @param node_id node identifier for a node with a mboxes property
 * @param name lowercase-and-underscores name of a mboxes element
 *             as defined by the node's mbox-names property
 *
 * @return the node identifier for the MBOX controller in the named element
 *
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_MBOX_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, mboxes, name)

/**
 * @brief Get a MBOX channel value by name
 *
 * Example devicetree fragment:
 *
 *     mbox1: mbox@... {
 *             #mbox-cells = <1>;
 *     };
 *
 *     n: node {
 *		mboxes = <&mbox1 1>,
 *		         <&mbox1 6>;
 *		mbox-names = "tx", "rx";
 *     };
 *
 * Bindings fragment for the mbox compatible:
 *
 *     mbox-cells:
 *       - channel
 *
 * Example usage:
 *
 *     DT_MBOX_CHANNEL_BY_NAME(DT_NODELABEL(n), tx) // 1
 *     DT_MBOX_CHANNEL_BY_NAME(DT_NODELABEL(n), rx) // 6
 *
 * @param node_id node identifier for a node with a mboxes property
 * @param name lowercase-and-underscores name of a mboxes element
 *             as defined by the node's mbox-names property
 *
 * @return the channel value in the specifier at the named element or 0 if no
 *         channels are supported
 *
 * @see DT_PHA_BY_NAME_OR()
 */
#define DT_MBOX_CHANNEL_BY_NAME(node_id, name) \
	DT_PHA_BY_NAME_OR(node_id, mboxes, name, channel, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_MBOX_H_ */
