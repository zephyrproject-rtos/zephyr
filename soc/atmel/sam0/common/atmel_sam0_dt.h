/*
 * Copyright (c) 2020, Linaro Ltd.
 * Copyright (c) 2021-2024, Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM0 MCU family devicetree helper macros
 */

#ifndef _ATMEL_SAM0_DT_H_
#define _ATMEL_SAM0_DT_H_

/* clang-format off */

#define ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, cell)		\
	(volatile uint32_t *)						\
	(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, cell)) +	\
	 DT_INST_CLOCKS_CELL_BY_NAME(n, cell, offset))

#define ATMEL_SAM0_DT_INST_CELL_PERIPH_MASK(n, name, cell)		\
	BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, name, cell))

/* Helper macro to get register address that control peripheral clock
 * enable bit.
 */
#define ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, pm)))

/* Helper macro to get peripheral clock bit mask.
 */
#define ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, cell)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_PERIPH_MASK(n, mclk, cell)),	\
		(ATMEL_SAM0_DT_INST_CELL_PERIPH_MASK(n, pm, cell)))

/* Helper macros for use with ATMEL SAM0 DMAC controller
 * return 0xff as default value if there is no 'dmas' property
 */
#define ATMEL_SAM0_DT_INST_DMA_CELL(n, name, cell)		\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),	\
		    (0xff))
#define ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, name) \
	ATMEL_SAM0_DT_INST_DMA_CELL(n, name, trigsrc)
#define ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, name) \
	ATMEL_SAM0_DT_INST_DMA_CELL(n, name, channel)
#define ATMEL_SAM0_DT_INST_DMA_CTLR(n, name)			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CTLR_BY_NAME(n, name)),	\
		    (DT_INVALID_NODE))


/* Use to check if a sercom 'n' is enabled for a given 'compat' */
#define ATMEL_SAM0_DT_SERCOM_CHECK(n, compat) \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sercom##n), compat, okay)

/* Use to check if TCC 'n' is enabled for a given 'compat' */
#define ATMEL_SAM0_DT_TCC_CHECK(n, compat) \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(tcc##n), compat, okay)

/* Common macro for use to set HCLK_FREQ_HZ */
#define ATMEL_SAM0_DT_CPU_CLK_FREQ_HZ \
	DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

/**
 * @brief Test if a node has a atmel,assigned-clocks phandle-array property at
 * a given index.
 *
 * This expands to 1 if the given index is valid atmel,assigned-clocks property
 * phandle-array index. Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             atmel,assigned-clocks = <...>, <...>;
 *     };
 *
 *     n2: node-2 {
 *             atmel,assigned-clocks = <...>;
 *     };
 *
 * Example usage:
 *
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_IDX(DT_NODELABEL(n1), 0) // 1
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_IDX(DT_NODELABEL(n1), 1) // 1
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_IDX(DT_NODELABEL(n1), 2) // 0
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_IDX(DT_NODELABEL(n2), 1) // 0
 *
 * @param node_id node identifier; may or may not have any atmel,assigned-clocks
 *		  property
 * @param idx index of a atmel,assigned-clocks property phandle-array whose
 *            existence to check
 * @return 1 if the index exists, 0 otherwise
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_IDX(node_id, idx) \
	DT_PROP_HAS_IDX(node_id, atmel_assigned_clocks, idx)

/**
 * @brief Test if a node has a clock-names array property holds a given name
 *
 * This expands to 1 if the name is available as atmel,assigned-clocks-name array
 * property cell. Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             atmel,assigned-clocks = <...>, <...>;
 *             atmel,assigned-clock-names = "alpha", "beta";
 *     };
 *
 *     n2: node-2 {
 *             atmel,assigned-clocks = <...>;
 *             atmel,assigned-clock-names = "alpha";
 *     };
 *
 * Example usage:
 *
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_NAME(DT_NODELABEL(n1), alpha) // 1
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_NAME(DT_NODELABEL(n1), beta)  // 1
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_NAME(DT_NODELABEL(n2), beta)  // 0
 *
 * @param node_id node identifier; may or may not have any clock-names property.
 * @param name lowercase-and-underscores clock-names cell value name to check
 * @return 1 if the atmel,assigned-clock name exists, 0 otherwise
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_NAME(node_id, name) \
	DT_PROP_HAS_NAME(node_id, atmel_assigned_clocks, name)

/**
 * @brief Get the number of elements in a atmel,assigned-clocks property
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             atmel,assigned-clocks = <&foo>, <&bar>;
 *     };
 *
 *     n2: node-2 {
 *             atmel,assigned-clocks = <&foo>;
 *     };
 *
 * Example usage:
 *
 *     ATMEL_SAM0_DT_NUM_ASSIGNED_CLOCKS(DT_NODELABEL(n1)) // 2
 *     ATMEL_SAM0_DT_NUM_ASSIGNED_CLOCKS(DT_NODELABEL(n2)) // 1
 *
 * @param node_id node identifier with a atmel,assigned-clocks property
 * @return number of elements in the property
 */
#define ATMEL_SAM0_DT_NUM_ASSIGNED_CLOCKS(node_id) \
	DT_PROP_LEN(node_id, atmel_assigned_clocks)


/**
 * @brief Get the node identifier for the controller phandle from a
 *        "atmel,assigned-clocks" phandle-array property at an index
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... { ... };
 *
 *     clk2: clock-controller@... { ... };
 *
 *     n: node {
 *             atmel,assigned-clocks = <&clk1 10 20>, <&clk2 30 40>;
 *     };
 *
 * Example usage:
 *
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(n), 0)) // DT_NODELABEL(clk1)
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(n), 1)) // DT_NODELABEL(clk2)
 *
 * @param node_id node identifier
 * @param idx logical index into "atmel,assigned-clocks"
 * @return the node identifier for the clock controller referenced at index "idx"
 * @see DT_PHANDLE_BY_IDX()
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, atmel_assigned_clocks, idx)

/**
 * @brief Equivalent to ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return a node identifier for the atmel,assigned-clocks controller at index 0
 *         in "atmel,assigned-clocks"
 * @see ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX()
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR(node_id) \
	ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        atmel,assigned-clocks phandle-array property by name
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... { ... };
 *
 *     clk2: clock-controller@... { ... };
 *
 *     n: node {
 *             atmel,assigned-clocks = <&clk1 10 20>, <&clk2 30 40>;
 *             atmel,assigned-clock-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_NAME(DT_NODELABEL(n), beta) // DT_NODELABEL(clk2)
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores name of a atmel,assigned-clocks element
 *             as defined by the node's clock-names property
 * @return the node identifier for the clock controller referenced by name
 * @see DT_PHANDLE_BY_NAME()
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, atmel_assigned_clocks, name)

/**
 * @brief Get a atmel,assigned-clock specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... {
 *             compatible = "vnd,clock";
 *             #atmel,assigned-clock-cells = < 2 >;
 *     };
 *
 *     n: node {
 *             atmel,assigned-clocks = < &clk1 10 20 >, < &clk1 30 40 >;
 *     };
 *
 * Bindings fragment for the vnd,clock compatible:
 *
 *     atmel,assigned-clock-cells:
 *       - bus
 *       - bits
 *
 * Example usage:
 *
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX(DT_NODELABEL(n), 0, bus) // 10
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX(DT_NODELABEL(n), 1, bits) // 40
 *
 * @param node_id node identifier for a node with a atmel,assigned-clocks property
 * @param idx logical index into atmel,assigned-clocks property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX(node_id, idx, cell) \
	DT_PHA_BY_IDX(node_id, atmel_assigned_clocks, idx, cell)

/**
 * @brief Get a atmel,assigned-clock specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... {
 *             compatible = "vnd,clock";
 *             #atmel,assigned-clock-cells = < 2 >;
 *     };
 *
 *     n: node {
 *             atmel,assigned-clocks = < &clk1 10 20 >, < &clk1 30 40 >;
 *             clock-names = "alpha", "beta";
 *     };
 *
 * Bindings fragment for the vnd,clock compatible:
 *
 *     atmel,assigned-clock-cells:
 *       - bus
 *       - bits
 *
 * Example usage:
 *
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_NAME(DT_NODELABEL(n), alpha, bus) // 10
 *     ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_NAME(DT_NODELABEL(n), beta, bits) // 40
 *
 * @param node_id node identifier for a node with a atmel,assigned-clocks property
 * @param name lowercase-and-underscores name of a atmel,assigned-clocks element
 *             as defined by the node's clock-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_NAME(node_id, name, cell) \
	DT_PHA_BY_NAME(node_id, atmel_assigned_clocks, name, cell)

/**
 * @brief Equivalent to ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX(node_id, 0, cell)
 * @param node_id node identifier for a node with a atmel,assigned-clocks property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX()
 */
#define ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL(node_id, cell) \
	ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX(node_id, 0, cell)

/**
 * @brief Equivalent to ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_IDX(DT_DRV_INST(inst), idx)
 * @param inst DT_DRV_COMPAT instance number; may or may not have any
 *             atmel,assigned-clocks property
 * @param idx index of a atmel,assigned-clocks property phandle-array whose existence
 *            to check
 * @return 1 if the index exists, 0 otherwise
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_HAS_IDX(inst, idx) \
	DT_ASSIGNED_CLOCKS_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_CLOCK_HAS_NAME(DT_DRV_INST(inst), name)
 * @param inst DT_DRV_COMPAT instance number; may or may not have any
 *             atmel,clock-names property.
 * @param name lowercase-and-underscores clock-names cell value name to check
 * @return 1 if the atmel,assigned-clock name exists, 0 otherwise
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_HAS_NAME(inst, name) \
	ATMEL_SAM0_DT_ASSIGNED_CLOCKS_HAS_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to ATMEL_SAM0_DT_NUM_ASSIGNED_CLOCKS(DT_DRV_INST(inst))
 * @param inst instance number
 * @return number of elements in the atmel,assigned-clocks property
 */
#define ATMEL_SAM0_DT_INST_NUM_ASSIGNED_CLOCKS(inst) \
	ATMEL_SAM0_DT_NUM_ASSIGNED_CLOCKS(DT_DRV_INST(inst))

/**
 * @brief Get the node identifier for the controller phandle from a
 *        "atmel,assigned-clocks" phandle-array property at an index
 *
 * @param inst instance number
 * @param idx logical index into "atmel,assigned-clocks"
 * @return the node identifier for the clock controller referenced at
 *         index "idx"
 * @see ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX()
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CTLR_BY_IDX(inst, idx) \
	ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CTLR_BY_IDX(inst, 0)
 * @param inst instance number
 * @return a node identifier for the atmel,assigned-clocks controller at index 0
 *         in "atmel,assigned-clocks"
 * @see ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR()
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CTLR(inst) \
	ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CTLR_BY_IDX(inst, 0)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        atmel,assigned-clocks phandle-array property by name
 *
 * @param inst instance number
 * @param name lowercase-and-underscores name of a atmel,assigned-clocks element
 *             as defined by the node's atmel,assigned-clock-names property
 * @return the node identifier for the clock controller referenced by
 *         the named element
 * @see ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_NAME()
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CTLR_BY_NAME(inst, name) \
	ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CTLR_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a DT_DRV_COMPAT instance's atmel,assigned-clock specifier's cell
 *        value at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into atmel,assigned-clocks property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX()
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_IDX(inst, idx, cell) \
	ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT instance's atmel,assigned-clock specifier's cell
 *        value by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a atmel,assigned-clocks element
 *             as defined by the node's atmel,assigned-clock-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_NAME()
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_NAME(inst, name, cell) \
	ATMEL_SAM0_DT_ASSIGNED_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Equivalent to ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_IDX(inst, 0, cell)
 * @param inst DT_DRV_COMPAT instance number
 * @param cell lowercase-and-underscores cell name
 * @return the value of the cell inside the specifier at index 0
 */
#define ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL(inst, cell) \
	ATMEL_SAM0_DT_INST_ASSIGNED_CLOCKS_CELL_BY_IDX(inst, 0, cell)

/* clang-format on */

#endif /* _ATMEL_SAM0_SOC_DT_H_ */
