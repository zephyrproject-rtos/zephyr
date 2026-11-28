/**
 * @file
 * @brief Counter alarm Devicetree macro public API header file.
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_COUNTER_ALARM_H_
#define ZEPHYR_INCLUDE_DEVICETREE_COUNTER_ALARM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-counter-alarms Devicetree Counter Alarms API
 * @ingroup devicetree
 * @ingroup counter_interface
 * @{
 */

/**
 * @brief Get the node identifier for the counter controller from a
 *        <tt>counter-alarms</tt> property at an index
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... { ... };
 *
 *     counter2: counter-controller@... { ... };
 *
 *     n: node {
 *             counter-alarms = <&counter1 1>, <&counter2 3>;
 *     };
 *
 * Example usage:
 *
 *     DT_COUNTER_ALARMS_CTLR_BY_IDX(DT_NODELABEL(n), counter_alarms, 0)
 *         // DT_NODELABEL(counter1)
 *     DT_COUNTER_ALARMS_CTLR_BY_IDX(DT_NODELABEL(n), counter_alarms, 1)
 *         // DT_NODELABEL(counter2)
 *
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_alarm_pha
 * @return the node identifier for the counter controller referenced at
 *         index @p idx
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_COUNTER_ALARMS_CTLR_BY_IDX(node_id, counter_alarm_pha, idx)                             \
	DT_PHANDLE_BY_IDX(node_id, counter_alarm_pha, idx)

/**
 * @brief Get the node identifier for the counter controller from a
 *        <tt>counter-alarms</tt> property by name
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... { ... };
 *
 *     counter2: counter-controller@... { ... };
 *
 *     n: node {
 *             counter-alarms = <&counter1 1>, <&counter2 3>;
 *             counter-alarm-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_COUNTER_ALARMS_CTLR_BY_NAME(DT_NODELABEL(n), counter_alarms, alpha)
 *         // DT_NODELABEL(counter1)
 *     DT_COUNTER_ALARMS_CTLR_BY_NAME(DT_NODELABEL(n), counter_alarms, beta)
 *         // DT_NODELABEL(counter2)
 *
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-alarms</tt> element
 *             as defined by the node's <tt>counter-alarm-names</tt> property
 * @return the node identifier for the counter controller in the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_COUNTER_ALARMS_CTLR_BY_NAME(node_id, counter_alarm_pha, name)                           \
	DT_PHANDLE_BY_NAME(node_id, counter_alarm_pha, name)

/**
 * @brief Equivalent to DT_COUNTER_ALARMS_CTLR_BY_IDX(node_id, counter_alarm_pha, 0)
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @return the node identifier for the counter controller at index 0
 *         in @p counter_alarm_pha
 * @see DT_COUNTER_ALARMS_CTLR_BY_IDX()
 */
#define DT_COUNTER_ALARMS_CTLR(node_id, counter_alarm_pha)                                         \
	DT_COUNTER_ALARMS_CTLR_BY_IDX(node_id, counter_alarm_pha, 0)

/**
 * @brief Get counter alarm specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... {
 *             compatible = "vnd,counter";
 *             #counter-alarm-cells = <1>;
 *     };
 *
 *     counter2: counter-controller@... {
 *             compatible = "vnd,counter";
 *             #counter-alarm-cells = <1>;
 *     };
 *
 *     n: node {
 *             counter-alarms = <&counter1 1>, <&counter2 3>;
 *     };
 *
 * Bindings fragment for the <tt>"vnd,counter"</tt> compatible:
 *
 *     counter-alarm-cells:
 *       - channel
 *
 * Example usage:
 *
 *     DT_COUNTER_ALARMS_CELL_BY_IDX(DT_NODELABEL(n), counter_alarms, 0, channel) // 1
 *     DT_COUNTER_ALARMS_CELL_BY_IDX(DT_NODELABEL(n), counter_alarms, 1, channel) // 3
 *
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_alarm_pha
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index @p idx
 * @see DT_PHA_BY_IDX()
 */
#define DT_COUNTER_ALARMS_CELL_BY_IDX(node_id, counter_alarm_pha, idx, cell)                       \
	DT_PHA_BY_IDX(node_id, counter_alarm_pha, idx, cell)

/**
 * @brief Get a counter alarm specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... {
 *             compatible = "vnd,counter";
 *             #counter-alarm-cells = <1>;
 *     };
 *
 *     counter2: counter-controller@... {
 *             compatible = "vnd,counter";
 *             #counter-alarm-cells = <1>;
 *     };
 *
 *     n: node {
 *             counter-alarms = <&counter1 1>, <&counter2 3>;
 *             counter-alarm-names = "alpha", "beta";
 *     };
 *
 * Bindings fragment for the <tt>"vnd,counter"</tt> compatible:
 *
 *     counter-alarm-cells:
 *       - channel
 *
 * Example usage:
 *
 *     DT_COUNTER_ALARMS_CELL_BY_NAME(DT_NODELABEL(n), counter_alarms, alpha, channel) // 1
 *     DT_COUNTER_ALARMS_CELL_BY_NAME(DT_NODELABEL(n), counter_alarms, beta, channel) // 3
 *
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-alarms</tt> element
 *             as defined by the node's <tt>counter-alarm-names</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_COUNTER_ALARMS_CELL_BY_NAME(node_id, counter_alarm_pha, name, cell)                     \
	DT_PHA_BY_NAME(node_id, counter_alarm_pha, name, cell)

/**
 * @brief Equivalent to DT_COUNTER_ALARMS_CELL_BY_IDX(node_id, counter_alarm_pha, 0, cell)
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_COUNTER_ALARMS_CELL_BY_IDX()
 */
#define DT_COUNTER_ALARMS_CELL(node_id, counter_alarm_pha, cell)                                   \
	DT_COUNTER_ALARMS_CELL_BY_IDX(node_id, counter_alarm_pha, 0, cell)

/**
 * @brief Get a counter alarm specifier's <tt>channel</tt> cell value at an index
 *
 * This macro only works for counter alarm specifiers with cells named
 * <tt>channel</tt>. Refer to the node's binding to check if necessary.
 *
 * This is equivalent to
 * DT_COUNTER_ALARMS_CELL_BY_IDX(node_id, counter_alarm_pha, idx, channel).
 *
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_alarm_pha
 * @return the <tt>channel</tt> cell value at index @p idx
 * @see DT_COUNTER_ALARMS_CELL_BY_IDX()
 */
#define DT_COUNTER_ALARMS_CHANNEL_BY_IDX(node_id, counter_alarm_pha, idx)                          \
	DT_COUNTER_ALARMS_CELL_BY_IDX(node_id, counter_alarm_pha, idx, channel)

/**
 * @brief Get a counter alarm specifier's <tt>channel</tt> cell value by name
 *
 * This macro only works for counter alarm specifiers with cells named
 * <tt>channel</tt>. Refer to the node's binding to check if necessary.
 *
 * This is equivalent to
 * DT_COUNTER_ALARMS_CELL_BY_NAME(node_id, counter_alarm_pha, name, channel).
 *
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-alarms</tt> element
 *             as defined by the node's <tt>counter-alarm-names</tt> property
 * @return the <tt>channel</tt> cell value in the specifier at the named element
 * @see DT_COUNTER_ALARMS_CELL_BY_NAME()
 */
#define DT_COUNTER_ALARMS_CHANNEL_BY_NAME(node_id, counter_alarm_pha, name)                        \
	DT_COUNTER_ALARMS_CELL_BY_NAME(node_id, counter_alarm_pha, name, channel)

/**
 * @brief Equivalent to DT_COUNTER_ALARMS_CHANNEL_BY_IDX(node_id, counter_alarm_pha, 0)
 * @param node_id node identifier
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @return the <tt>channel</tt> cell value at index 0
 * @see DT_COUNTER_ALARMS_CHANNEL_BY_IDX()
 */
#define DT_COUNTER_ALARMS_CHANNEL(node_id, counter_alarm_pha)                                      \
	DT_COUNTER_ALARMS_CHANNEL_BY_IDX(node_id, counter_alarm_pha, 0)

/**
 * @brief Get the node identifier for the counter controller from a
 *        @c DT_DRV_COMPAT instance's <tt>counter-alarms</tt> property at an index
 *
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_alarm_pha
 * @return the node identifier for the counter controller referenced at
 *         index @p idx
 * @see DT_COUNTER_ALARMS_CTLR_BY_IDX()
 */
#define DT_INST_COUNTER_ALARMS_CTLR_BY_IDX(inst, counter_alarm_pha, idx)                           \
	DT_COUNTER_ALARMS_CTLR_BY_IDX(DT_DRV_INST(inst), counter_alarm_pha, idx)

/**
 * @brief Get the node identifier for the counter controller from a
 *        @c DT_DRV_COMPAT instance's <tt>counter-alarms</tt> property by name
 *
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-alarms</tt> element
 *             as defined by the node's <tt>counter-alarm-names</tt> property
 * @return the node identifier for the counter controller in the named element
 * @see DT_COUNTER_ALARMS_CTLR_BY_NAME()
 */
#define DT_INST_COUNTER_ALARMS_CTLR_BY_NAME(inst, counter_alarm_pha, name)                         \
	DT_COUNTER_ALARMS_CTLR_BY_NAME(DT_DRV_INST(inst), counter_alarm_pha, name)

/**
 * @brief Equivalent to DT_INST_COUNTER_ALARMS_CTLR_BY_IDX(inst, counter_alarm_pha, 0)
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @return the node identifier for the counter controller at index 0
 * @see DT_INST_COUNTER_ALARMS_CTLR_BY_IDX()
 */
#define DT_INST_COUNTER_ALARMS_CTLR(inst, counter_alarm_pha)                                       \
	DT_INST_COUNTER_ALARMS_CTLR_BY_IDX(inst, counter_alarm_pha, 0)

/**
 * @brief Get a @c DT_DRV_COMPAT instance's counter alarm specifier's cell value
 *        at an index
 *
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_alarm_pha
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index @p idx
 * @see DT_COUNTER_ALARMS_CELL_BY_IDX()
 */
#define DT_INST_COUNTER_ALARMS_CELL_BY_IDX(inst, counter_alarm_pha, idx, cell)                     \
	DT_COUNTER_ALARMS_CELL_BY_IDX(DT_DRV_INST(inst), counter_alarm_pha, idx, cell)

/**
 * @brief Get a @c DT_DRV_COMPAT instance's counter alarm specifier's cell value
 *        by name
 *
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-alarms</tt> element
 *             as defined by the node's <tt>counter-alarm-names</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_COUNTER_ALARMS_CELL_BY_NAME()
 */
#define DT_INST_COUNTER_ALARMS_CELL_BY_NAME(inst, counter_alarm_pha, name, cell)                   \
	DT_COUNTER_ALARMS_CELL_BY_NAME(DT_DRV_INST(inst), counter_alarm_pha, name, cell)

/**
 * @brief Equivalent to DT_INST_COUNTER_ALARMS_CELL_BY_IDX(inst, counter_alarm_pha, 0, cell)
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_INST_COUNTER_ALARMS_CELL_BY_IDX()
 */
#define DT_INST_COUNTER_ALARMS_CELL(inst, counter_alarm_pha, cell)                                 \
	DT_INST_COUNTER_ALARMS_CELL_BY_IDX(inst, counter_alarm_pha, 0, cell)

/**
 * @brief Equivalent to
 *        DT_INST_COUNTER_ALARMS_CELL_BY_IDX(inst, counter_alarm_pha, idx, channel)
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_alarm_pha
 * @return the <tt>channel</tt> cell value at index @p idx
 * @see DT_COUNTER_ALARMS_CHANNEL_BY_IDX()
 */
#define DT_INST_COUNTER_ALARMS_CHANNEL_BY_IDX(inst, counter_alarm_pha, idx)                        \
	DT_INST_COUNTER_ALARMS_CELL_BY_IDX(inst, counter_alarm_pha, idx, channel)

/**
 * @brief Equivalent to
 *        DT_INST_COUNTER_ALARMS_CELL_BY_NAME(inst, counter_alarm_pha, name, channel)
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-alarms</tt> element
 *             as defined by the node's <tt>counter-alarm-names</tt> property
 * @return the <tt>channel</tt> cell value in the specifier at the named element
 * @see DT_COUNTER_ALARMS_CHANNEL_BY_NAME()
 */
#define DT_INST_COUNTER_ALARMS_CHANNEL_BY_NAME(inst, counter_alarm_pha, name)                      \
	DT_INST_COUNTER_ALARMS_CELL_BY_NAME(inst, counter_alarm_pha, name, channel)

/**
 * @brief Equivalent to DT_INST_COUNTER_ALARMS_CHANNEL_BY_IDX(inst, counter_alarm_pha, 0)
 * @param inst @c DT_DRV_COMPAT instance number
 * @param counter_alarm_pha lowercase-and-underscores counter alarm
 *        property with type "phandle-array"
 * @return the <tt>channel</tt> cell value at index 0
 * @see DT_INST_COUNTER_ALARMS_CHANNEL_BY_IDX()
 */
#define DT_INST_COUNTER_ALARMS_CHANNEL(inst, counter_alarm_pha)                                    \
	DT_INST_COUNTER_ALARMS_CHANNEL_BY_IDX(inst, counter_alarm_pha, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_COUNTER_ALARM_H_ */
