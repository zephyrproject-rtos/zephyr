/**
 * @file
 * @brief Display Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2025 Abderrahmane JARMOUNI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_DISPLAY_H_
#define ZEPHYR_INCLUDE_DEVICETREE_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-display Devicetree Display API
 * @ingroup devicetree
 * @ingroup display_interface
 * @{
 */

/**
 * @brief Get display node identifier by logical index from "displays" property of node with
 * compatible "zephyr,displays"
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	displays_node: my-displays {
 *		compatible = "zephyr,displays";
 *		displays = <&n2 &n3>;
 *		status = "okay";
 *	};
 *
 *     n2: node-2 { ... };
 *     n3: node-3 { ... };
 * @endcode
 *
 * Above, displays property has two elements:
 *
 * - index 0 has phandle `&n2`, which is `node-2`'s phandle
 * - index 1 has phandle `&n3`, which is `node-3`'s phandle
 *
 * Example usage:
 *
 * @code{.c}
 *
 *     DT_ZEPHYR_DISPLAY(0) // node identifier for display node node-2
 *     DT_ZEPHYR_DISPLAY(1) // node identifier for display node node-3
 * @endcode
 *
 * @param idx logical index of display node's phandle in "displays" property
 *
 * @return display node identifier, @ref DT_INVALID_NODE otherwise
 */
#define DT_ZEPHYR_DISPLAY(idx)                                                                     \
	DT_PHANDLE_BY_IDX(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_displays), displays, idx)

/**
 * @brief Get number of zephyr displays
 *
 * @return number of displays designated by "displays" property of "zephyr,displays" compatible
 * node, if it exists, otherwise 1 if "zephyr,display" chosen property exists, 0 otherwise
 */
#define DT_ZEPHYR_DISPLAYS_COUNT                                                                   \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays),                                    \
		    (DT_PROP_LEN(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_displays), displays)),       \
		    (DT_HAS_CHOSEN(zephyr_display)))
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_DMAS_H_ */
