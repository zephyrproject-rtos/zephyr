/**
 * @file
 * @brief NVMEM Devicetree public API header file.
 */

/*
 * Copyright (c) 2024, Andriy Gelman
 * Copyright (c) 2025, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DEVICETREE_NVMEM_H_
#define INCLUDE_ZEPHYR_DEVICETREE_NVMEM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-nvmem Devicetree NVMEM API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Test if a node has an nvmem-cells phandle-array property at a given index
 *
 * This expands to 1 if the given index is a valid nvmem-cells property phandle-array index.
 * Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	eth: ethernet {
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_NVMEM_CELLS_HAS_IDX(DT_NODELABEL(eth), 0) // 1
 *	DT_NVMEM_CELLS_HAS_IDX(DT_NODELABEL(eth), 1) // 0
 * @endcode
 *
 * @param node_id node identifier; may or may not have any nvmem-cells property
 * @param idx index of a nvmem-cells property phandle-array whose existence to check
 *
 * @return 1 if the index exists, 0 otherwise
 */
#define DT_NVMEM_CELLS_HAS_IDX(node_id, idx) DT_PROP_HAS_IDX(node_id, nvmem_cells, idx)

/**
 * @brief Test if a node has an nvmem-cell-names array property hold a given name.
 *
 * This expands to 1 if the name is available as nvmem-cells-name array property cell.
 * Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	eth: ethernet {
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_NVMEM_CELLS_HAS_NAME(DT_NODELABEL(eth), mac_address) // 1
 *	DT_NVMEM_CELLS_HAS_NAME(DT_NODELABEL(eth), bogus)       // 0
 * @endcode
 *
 * @param node_id node identifier; may or may not have any nvmem-cell-names property
 * @param name lowercase-and-underscores nvmem-cell-names cell value name to check
 *
 * @return 1 if the index exists, 0 otherwise
 */
#define DT_NVMEM_CELLS_HAS_NAME(node_id, name) DT_PROP_HAS_NAME(node_id, nvmem_cells, name)

/**
 * @brief Get the number of elements in an nvmem-cells property
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	eth: ethernet {
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_NUM_NVMEM_CELLS(DT_NODELABEL(eth)) // 1
 * @endcode
 *
 * @param node_id node identifier with an nvmem-cells property
 *
 * @return number of elements in the property
 */
#define DT_NUM_NVMEM_CELLS(node_id) DT_PROP_LEN(node_id, nvmem_cells)

/**
 * @brief Get the node identifier for the NVMEM cell from the nvmem-cells property by index.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	mac_eeprom: mac_eeprom@2 {
 *		nvmem-layout {
 *			compatible = "fixed-layout";
 *			#address-cells = <1>;
 *			#size-cells = <1>;
 *
 *			mac_address: mac_address@fa {
 *				reg = <0xfa 0x06>;
 *				#nvmem-cell-cells = <0>;
 *			};
 *		};
 *	};
 *
 *	eth: ethernet {
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_NVMEM_CELL_BY_IDX(DT_NODELABEL(eth), 0) // DT_NODELABEL(mac_address)
 * @endcode
 *
 * @param node_id node identifier for a node with a nvmem-cells property
 * @param idx index into the nvmem-cells property
 *
 * @return the node identifier for the NVMEM cell at index idx
 */
#define DT_NVMEM_CELL_BY_IDX(node_id, idx) DT_PHANDLE_BY_IDX(node_id, nvmem_cells, idx)

/**
 * @brief Equivalent to DT_NVMEM_CELL_BY_IDX(node_id, 0)
 *
 * @param node_id node identifier
 *
 * @return a node identifier for the NVMEM cell at index 0
 *         in "nvmem-cells"
 *
 * @see DT_NVMEM_CELL_BY_IDX()
 */
#define DT_NVMEM_CELL(node_id) DT_NVMEM_CELL_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the NVMEM cell from the nvmem-cells property by name.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	mac_eeprom: mac_eeprom@2 {
 *		nvmem-layout {
 *			compatible = "fixed-layout";
 *			#address-cells = <1>;
 *			#size-cells = <1>;
 *
 *			mac_address: mac_address@fa {
 *				reg = <0xfa 0x06>;
 *				#nvmem-cell-cells = <0>;
 *			};
 *		};
 *	};
 *
 *	eth: ethernet {
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_NVMEM_CELL_BY_NAME(DT_NODELABEL(eth), mac_address) // DT_NODELABEL(mac_address)
 * @endcode
 *
 * @param node_id node identifier for a node with a nvmem-cells property
 * @param name lowercase-and-underscores name of an nvmem-cells element
 *             as defined by the node's nvmem-cell-names property
 *
 * @return the node identifier for the NVMEM cell by name
 */
#define DT_NVMEM_CELL_BY_NAME(node_id, name) DT_PHANDLE_BY_NAME(node_id, nvmem_cells, name)

/**
 * @brief Equivalent to DT_NVMEM_CELLS_HAS_IDX(DT_DRV_INST(inst), idx)
 *
 * @param inst DT_DRV_COMPAT instance number; may or may not have any nvmem-cells property
 * @param idx index of an nvmem-cells property phandle-array whose existence to check
 *
 * @return 1 if the index exists, 0 otherwise
 */
#define DT_INST_NVMEM_CELLS_HAS_IDX(inst, idx) DT_NVMEM_CELLS_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_NVMEM_CELLS_HAS_NAME(DT_DRV_INST(inst), name)
 *
 * @param inst DT_DRV_COMPAT instance number; may or may not have any nvmem-cell-names property.
 * @param name lowercase-and-underscores nvmem-cell-names cell value name to check
 *
 * @return 1 if the nvmem cell name exists, 0 otherwise
 */
#define DT_INST_NVMEM_CELLS_HAS_NAME(inst, name) DT_NVMEM_CELLS_HAS_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_NUM_NVMEM_CELLS(DT_DRV_INST(inst))
 *
 * @param inst instance number
 *
 * @return number of elements in the nvmem-cells property
 */
#define DT_INST_NUM_NVMEM_CELLS(inst) DT_NUM_NVMEM_CELLS(DT_DRV_INST(inst))

/**
 * @brief Get the node identifier for the controller phandle from an
 *        nvmem-cells phandle-array property at an index
 *
 * @param inst instance number
 * @param idx logical index into nvmem-cells
 *
 * @return the node identifier for the nvmem cell referenced at
 *         index "idx"
 *
 * @see DT_NVMEM_CELL_CTLR_BY_IDX()
 */
#define DT_INST_NVMEM_CELL_BY_IDX(inst, idx) DT_NVMEM_CELL_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_INST_NVMEM_CELL_BY_IDX(inst, 0)
 *
 * @param inst instance number
 *
 * @return a node identifier for the nvmem cell at index 0
 *         in nvmem-cells
 *
 * @see DT_NVMEM_CELL()
 */
#define DT_INST_NVMEM_CELL(inst) DT_INST_NVMEM_CELL_BY_IDX(inst, 0)

/**
 * @brief Get the node identifier for the controller phandle from an
 *        nvmem-cells phandle-array property by name
 *
 * @param inst instance number
 * @param name lowercase-and-underscores name of an nvmem-cells element
 *             as defined by the node's nvmem-cell-names property
 *
 * @return the node identifier for the nvmem cell referenced by
 *         the named element
 *
 * @see DT_NVMEM_CELL_BY_NAME()
 */
#define DT_INST_NVMEM_CELL_BY_NAME(inst, name) DT_NVMEM_CELL_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get the node identifier of the memory controller for an nvmem cell.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	mac_eeprom: mac_eeprom@2 {
 *		nvmem-layout {
 *			compatible = "fixed-layout";
 *			#address-cells = <1>;
 *			#size-cells = <1>;
 *
 *			mac_address: mac_address@fa {
 *				reg = <0xfa 0x06>;
 *				#nvmem-cell-cells = <0>;
 *			};
 *		};
 *	};
 *
 *	eth: ethernet {
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_MTD_FROM_NVMEM_CELL(DT_NVMEM_CELL(DT_NODELABEL(eth))) // DT_NODELABEL(mac_eeprom)
 * @endcode
 *
 * @param node_id node identifier for an nvmem cell node
 *
 * @return the node identifier of the Memory Technology Device (MTD) that
 *         contains the nvmem cell node.
 */
#define DT_MTD_FROM_NVMEM_CELL(node_id) DT_GPARENT(node_id)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_DEVICETREE_NVMEM_H_ */
