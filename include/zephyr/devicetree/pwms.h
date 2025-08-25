/**
 * @file
 * @brief PWMs Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_PWMS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_PWMS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-pwms Devicetree PWMs API
 * @ingroup devicetree
 * @ingroup pwm_interface
 * @{
 */

/**
 * @brief Get the node identifier for the PWM controller from a
 *        pwms property at an index
 *
 * Example devicetree fragment:
 *
 *     pwm1: pwm-controller@... { ... };
 *
 *     pwm2: pwm-controller@... { ... };
 *
 *     n: node {
 *             pwms = <&pwm1 1 PWM_POLARITY_NORMAL>,
 *                    <&pwm2 3 PWM_POLARITY_INVERTED>;
 *     };
 *
 * Example usage:
 *
 *     DT_PWMS_CTLR_BY_IDX(DT_NODELABEL(n), 0) // DT_NODELABEL(pwm1)
 *     DT_PWMS_CTLR_BY_IDX(DT_NODELABEL(n), 1) // DT_NODELABEL(pwm2)
 *
 * @param node_id node identifier for a node with a pwms property
 * @param idx logical index into pwms property
 * @return the node identifier for the PWM controller referenced at
 *         index "idx"
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_PWMS_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, pwms, idx)

/**
 * @brief Get the node identifier for the PWM controller from a
 *        pwms property by name
 *
 * Example devicetree fragment:
 *
 *     pwm1: pwm-controller@... { ... };
 *
 *    pwm2: pwm-controller@... { ... };
 *
 *     n: node {
 *             pwms = <&pwm1 1 PWM_POLARITY_NORMAL>,
 *                    <&pwm2 3 PWM_POLARITY_INVERTED>;
 *             pwm-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_PWMS_CTLR_BY_NAME(DT_NODELABEL(n), alpha) // DT_NODELABEL(pwm1)
 *     DT_PWMS_CTLR_BY_NAME(DT_NODELABEL(n), beta)  // DT_NODELABEL(pwm2)
 *
 * @param node_id node identifier for a node with a pwms property
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the node identifier for the PWM controller in the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_PWMS_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, pwms, name)

/**
 * @brief Equivalent to DT_PWMS_CTLR_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a pwms property
 * @return the node identifier for the PWM controller at index 0
 *         in the node's "pwms" property
 * @see DT_PWMS_CTLR_BY_IDX()
 */
#define DT_PWMS_CTLR(node_id) DT_PWMS_CTLR_BY_IDX(node_id, 0)

/**
 * @brief Get PWM specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     pwm1: pwm-controller@... {
 *             compatible = "vnd,pwm";
 *             #pwm-cells = <2>;
 *     };
 *
 *     pwm2: pwm-controller@... {
 *             compatible = "vnd,pwm";
 *             #pwm-cells = <2>;
 *     };
 *
 *     n: node {
 *             pwms = <&pwm1 1 200000 PWM_POLARITY_NORMAL>,
 *                    <&pwm2 3 100000 PWM_POLARITY_INVERTED>;
 *     };
 *
 * Bindings fragment for the "vnd,pwm" compatible:
 *
 *     pwm-cells:
 *       - channel
 *       - period
 *       - flags
 *
 * Example usage:
 *
 *     DT_PWMS_CELL_BY_IDX(DT_NODELABEL(n), 0, channel) // 1
 *     DT_PWMS_CELL_BY_IDX(DT_NODELABEL(n), 1, channel) // 3
 *     DT_PWMS_CELL_BY_IDX(DT_NODELABEL(n), 0, period)  // 200000
 *     DT_PWMS_CELL_BY_IDX(DT_NODELABEL(n), 1, period)  // 100000
 *     DT_PWMS_CELL_BY_IDX(DT_NODELABEL(n), 0, flags)   // PWM_POLARITY_NORMAL
 *     DT_PWMS_CELL_BY_IDX(DT_NODELABEL(n), 1, flags)   // PWM_POLARITY_INVERTED
 *
 * @param node_id node identifier for a node with a pwms property
 * @param idx logical index into pwms property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_PWMS_CELL_BY_IDX(node_id, idx, cell) \
	DT_PHA_BY_IDX(node_id, pwms, idx, cell)

/**
 * @brief Get a PWM specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     pwm1: pwm-controller@... {
 *             compatible = "vnd,pwm";
 *             #pwm-cells = <2>;
 *     };
 *
 *     pwm2: pwm-controller@... {
 *             compatible = "vnd,pwm";
 *             #pwm-cells = <2>;
 *     };
 *
 *     n: node {
 *             pwms = <&pwm1 1 200000 PWM_POLARITY_NORMAL>,
 *                    <&pwm2 3 100000 PWM_POLARITY_INVERTED>;
 *             pwm-names = "alpha", "beta";
 *     };
 *
 * Bindings fragment for the "vnd,pwm" compatible:
 *
 *     pwm-cells:
 *       - channel
 *       - period
 *       - flags
 *
 * Example usage:
 *
 *     DT_PWMS_CELL_BY_NAME(DT_NODELABEL(n), alpha, channel) // 1
 *     DT_PWMS_CELL_BY_NAME(DT_NODELABEL(n), beta, channel)  // 3
 *     DT_PWMS_CELL_BY_NAME(DT_NODELABEL(n), alpha, period)  // 200000
 *     DT_PWMS_CELL_BY_NAME(DT_NODELABEL(n), beta, period)   // 100000
 *     DT_PWMS_CELL_BY_NAME(DT_NODELABEL(n), alpha, flags)   // PWM_POLARITY_NORMAL
 *     DT_PWMS_CELL_BY_NAME(DT_NODELABEL(n), beta, flags)    // PWM_POLARITY_INVERTED
 *
 * @param node_id node identifier for a node with a pwms property
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_PWMS_CELL_BY_NAME(node_id, name, cell) \
	DT_PHA_BY_NAME(node_id, pwms, name, cell)

/**
 * @brief Equivalent to DT_PWMS_CELL_BY_IDX(node_id, 0, cell)
 * @param node_id node identifier for a node with a pwms property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_PWMS_CELL_BY_IDX()
 */
#define DT_PWMS_CELL(node_id, cell) DT_PWMS_CELL_BY_IDX(node_id, 0, cell)

/**
 * @brief Get a PWM specifier's channel cell value at an index
 *
 * This macro only works for PWM specifiers with cells named "channel".
 * Refer to the node's binding to check if necessary.
 *
 * This is equivalent to DT_PWMS_CELL_BY_IDX(node_id, idx, channel).
 *
 * @param node_id node identifier for a node with a pwms property
 * @param idx logical index into pwms property
 * @return the channel cell value at index "idx"
 * @see DT_PWMS_CELL_BY_IDX()
 */
#define DT_PWMS_CHANNEL_BY_IDX(node_id, idx) \
	DT_PWMS_CELL_BY_IDX(node_id, idx, channel)

/**
 * @brief Get a PWM specifier's channel cell value by name
 *
 * This macro only works for PWM specifiers with cells named "channel".
 * Refer to the node's binding to check if necessary.
 *
 * This is equivalent to DT_PWMS_CELL_BY_NAME(node_id, name, channel).
 *
 * @param node_id node identifier for a node with a pwms property
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the channel cell value in the specifier at the named element
 * @see DT_PWMS_CELL_BY_NAME()
 */
#define DT_PWMS_CHANNEL_BY_NAME(node_id, name) \
	DT_PWMS_CELL_BY_NAME(node_id, name, channel)

/**
 * @brief Equivalent to DT_PWMS_CHANNEL_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a pwms property
 * @return the channel cell value at index 0
 * @see DT_PWMS_CHANNEL_BY_IDX()
 */
#define DT_PWMS_CHANNEL(node_id) DT_PWMS_CHANNEL_BY_IDX(node_id, 0)

/**
 * @brief Get PWM specifier's period cell value at an index
 *
 * This macro only works for PWM specifiers with cells named "period".
 * Refer to the node's binding to check if necessary.
 *
 * This is equivalent to DT_PWMS_CELL_BY_IDX(node_id, idx, period).
 *
 * @param node_id node identifier for a node with a pwms property
 * @param idx logical index into pwms property
 * @return the period cell value at index "idx"
 * @see DT_PWMS_CELL_BY_IDX()
 */
#define DT_PWMS_PERIOD_BY_IDX(node_id, idx) \
	DT_PWMS_CELL_BY_IDX(node_id, idx, period)

/**
 * @brief Get a PWM specifier's period cell value by name
 *
 * This macro only works for PWM specifiers with cells named "period".
 * Refer to the node's binding to check if necessary.
 *
 * This is equivalent to DT_PWMS_CELL_BY_NAME(node_id, name, period).
 *
 * @param node_id node identifier for a node with a pwms property
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the period cell value in the specifier at the named element
 * @see DT_PWMS_CELL_BY_NAME()
 */
#define DT_PWMS_PERIOD_BY_NAME(node_id, name) \
	DT_PWMS_CELL_BY_NAME(node_id, name, period)

/**
 * @brief Equivalent to DT_PWMS_PERIOD_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a pwms property
 * @return the period cell value at index 0
 * @see DT_PWMS_PERIOD_BY_IDX()
 */
#define DT_PWMS_PERIOD(node_id) DT_PWMS_PERIOD_BY_IDX(node_id, 0)

/**
 * @brief Get a PWM specifier's flags cell value at an index
 *
 * This macro expects PWM specifiers with cells named "flags".
 * If there is no "flags" cell in the PWM specifier, zero is returned.
 * Refer to the node's binding to check specifier cell names if necessary.
 *
 * This is equivalent to DT_PWMS_CELL_BY_IDX(node_id, idx, flags).
 *
 * @param node_id node identifier for a node with a pwms property
 * @param idx logical index into pwms property
 * @return the flags cell value at index "idx", or zero if there is none
 * @see DT_PWMS_CELL_BY_IDX()
 */
#define DT_PWMS_FLAGS_BY_IDX(node_id, idx) \
	DT_PHA_BY_IDX_OR(node_id, pwms, idx, flags, 0)

/**
 * @brief Get a PWM specifier's flags cell value by name
 *
 * This macro expects PWM specifiers with cells named "flags".
 * If there is no "flags" cell in the PWM specifier, zero is returned.
 * Refer to the node's binding to check specifier cell names if necessary.
 *
 * This is equivalent to DT_PWMS_CELL_BY_NAME(node_id, name, flags) if
 * there is a flags cell, but expands to zero if there is none.
 *
 * @param node_id node identifier for a node with a pwms property
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the flags cell value in the specifier at the named element,
 *         or zero if there is none
 * @see DT_PWMS_CELL_BY_NAME()
 */
#define DT_PWMS_FLAGS_BY_NAME(node_id, name) \
	DT_PHA_BY_NAME_OR(node_id, pwms, name, flags, 0)

/**
 * @brief Equivalent to DT_PWMS_FLAGS_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a pwms property
 * @return the flags cell value at index 0, or zero if there is none
 * @see DT_PWMS_FLAGS_BY_IDX()
 */
#define DT_PWMS_FLAGS(node_id) DT_PWMS_FLAGS_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the PWM controller from a
 *        DT_DRV_COMPAT instance's pwms property at an index
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into pwms property
 * @return the node identifier for the PWM controller referenced at
 *         index "idx"
 * @see DT_PWMS_CTLR_BY_IDX()
 */
#define DT_INST_PWMS_CTLR_BY_IDX(inst, idx) \
	DT_PWMS_CTLR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get the node identifier for the PWM controller from a
 *        DT_DRV_COMPAT instance's pwms property by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the node identifier for the PWM controller in the named element
 * @see DT_PWMS_CTLR_BY_NAME()
 */
#define DT_INST_PWMS_CTLR_BY_NAME(inst, name) \
	DT_PWMS_CTLR_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_PWMS_CTLR_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the node identifier for the PWM controller at index 0
 *         in the instance's "pwms" property
 * @see DT_PWMS_CTLR_BY_IDX()
 */
#define DT_INST_PWMS_CTLR(inst) DT_INST_PWMS_CTLR_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's PWM specifier's cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into pwms property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 */
#define DT_INST_PWMS_CELL_BY_IDX(inst, idx, cell) \
	DT_PWMS_CELL_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT instance's PWM specifier's cell value by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PWMS_CELL_BY_NAME()
 */
#define DT_INST_PWMS_CELL_BY_NAME(inst, name, cell) \
	DT_PWMS_CELL_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Equivalent to DT_INST_PWMS_CELL_BY_IDX(inst, 0, cell)
 * @param inst DT_DRV_COMPAT instance number
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 */
#define DT_INST_PWMS_CELL(inst, cell) \
	DT_INST_PWMS_CELL_BY_IDX(inst, 0, cell)

/**
 * @brief Equivalent to DT_INST_PWMS_CELL_BY_IDX(inst, idx, channel)
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into pwms property
 * @return the channel cell value at index "idx"
 * @see DT_INST_PWMS_CELL_BY_IDX()
 */
#define DT_INST_PWMS_CHANNEL_BY_IDX(inst, idx) \
	DT_INST_PWMS_CELL_BY_IDX(inst, idx, channel)

/**
 * @brief Equivalent to DT_INST_PWMS_CELL_BY_NAME(inst, name, channel)
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the channel cell value in the specifier at the named element
 * @see DT_INST_PWMS_CELL_BY_NAME()
 */
#define DT_INST_PWMS_CHANNEL_BY_NAME(inst, name) \
	DT_INST_PWMS_CELL_BY_NAME(inst, name, channel)

/**
 * @brief Equivalent to DT_INST_PWMS_CHANNEL_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the channel cell value at index 0
 * @see DT_INST_PWMS_CHANNEL_BY_IDX()
 */
#define DT_INST_PWMS_CHANNEL(inst) DT_INST_PWMS_CHANNEL_BY_IDX(inst, 0)

/**
 * @brief Equivalent to DT_INST_PWMS_CELL_BY_IDX(inst, idx, period)
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into pwms property
 * @return the period cell value at index "idx"
 * @see DT_INST_PWMS_CELL_BY_IDX()
 */
#define DT_INST_PWMS_PERIOD_BY_IDX(inst, idx) \
	DT_INST_PWMS_CELL_BY_IDX(inst, idx, period)

/**
 * @brief Equivalent to DT_INST_PWMS_CELL_BY_NAME(inst, name, period)
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the period cell value in the specifier at the named element
 * @see DT_INST_PWMS_CELL_BY_NAME()
 */
#define DT_INST_PWMS_PERIOD_BY_NAME(inst, name) \
	DT_INST_PWMS_CELL_BY_NAME(inst, name, period)

/**
 * @brief Equivalent to DT_INST_PWMS_PERIOD_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the period cell value at index 0
 * @see DT_INST_PWMS_PERIOD_BY_IDX()
 */
#define DT_INST_PWMS_PERIOD(inst) DT_INST_PWMS_PERIOD_BY_IDX(inst, 0)

/**
 * @brief Equivalent to DT_INST_PWMS_CELL_BY_IDX(inst, idx, flags)
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into pwms property
 * @return the flags cell value at index "idx", or zero if there is none
 * @see DT_INST_PWMS_CELL_BY_IDX()
 */
#define DT_INST_PWMS_FLAGS_BY_IDX(inst, idx) \
	DT_INST_PWMS_CELL_BY_IDX(inst, idx, flags)

/**
 * @brief Equivalent to DT_INST_PWMS_CELL_BY_NAME(inst, name, flags)
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a pwms element
 *             as defined by the node's pwm-names property
 * @return the flags cell value in the specifier at the named element,
 *         or zero if there is none
 * @see DT_INST_PWMS_CELL_BY_NAME()
 */
#define DT_INST_PWMS_FLAGS_BY_NAME(inst, name) \
	DT_INST_PWMS_CELL_BY_NAME(inst, name, flags)

/**
 * @brief Equivalent to DT_INST_PWMS_FLAGS_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the flags cell value at index 0, or zero if there is none
 * @see DT_INST_PWMS_FLAGS_BY_IDX()
 */
#define DT_INST_PWMS_FLAGS(inst) DT_INST_PWMS_FLAGS_BY_IDX(inst, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_PWMS_H_ */
