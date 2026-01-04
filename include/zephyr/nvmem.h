/*
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header file for NVMEM API.
 * @ingroup nvmem_interface
 */

#ifndef ZEPHYR_INCLUDE_NVMEM_H_
#define ZEPHYR_INCLUDE_NVMEM_H_

/**
 * @brief Interfaces for NVMEM cells.
 * @defgroup nvmem_interface NVMEM
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/nvmem.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-Volatile Memory cell representation.
 */
struct nvmem_cell {
	/** NVMEM parent controller device instance. */
	const struct device *dev;
	/** Offset of the NVMEM cell relative to the parent controller's base address */
	off_t offset;
	/** Size of the NVMEM cell */
	size_t size;
	/** Indicator if the NVMEM cell is read-write or read-only */
	bool read_only;
};

/**
 * @brief Static initializer for a struct nvmem_cell.
 *
 * This returns a static initializer for a struct nvmem_cell given a devicetree
 * node identifier.
 *
 * @note This is a helper macro for other NVMEM_CELL_GET macros to initialize the
 *       nvmem_cell struct.
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	const struct nvmem_cell cell = NVMEM_CELL_INIT(DT_NODELABEL(mac_address));
 *
 *	// Initializes 'cell' to:
 *	// {
 *	//	.dev = DEVICE_DT_GET(DT_NODELABEL(mac_eeprom)),
 *	//	.offset = 0xfa,
 *	//	.size = 6,
 *	//	.read_only = false,
 *	// }
 * @endcode
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for a struct nvmem_cell
 */
#define NVMEM_CELL_INIT(node_id)                                                                   \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_MTD_FROM_NVMEM_CELL(node_id)),                             \
		.offset = DT_REG_ADDR(node_id),                                                    \
		.size = DT_REG_SIZE(node_id),                                                      \
		.read_only = DT_PROP(node_id, read_only),                                          \
	}

/**
 * @brief Static initializer for a struct nvmem_cell.
 *
 * This returns a static initializer for a struct nvmem_cell given a devicetree
 * node identifier and a name.
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
 *	const struct nvmem_cell cell =
 *		NVMEM_CELL_GET_BY_NAME(DT_NODELABEL(eth), mac_address);
 *
 *	// Initializes 'cell' to:
 *	// {
 *	//	.dev = DEVICE_DT_GET(DT_NODELABEL(mac_eeprom)),
 *	//	.offset = 0xfa,
 *	//	.size = 6,
 *	//	.read_only = false,
 *	// }
 * @endcode
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined by
 *             the node's nvmem-cell-names property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_INST_GET_BY_NAME
 */
#define NVMEM_CELL_GET_BY_NAME(node_id, name) NVMEM_CELL_INIT(DT_NVMEM_CELL_BY_NAME(node_id, name))

/**
 * @brief Static initializer for a struct nvmem_cell from a DT_DRV_COMPAT
 *        instance.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined by
 *             the node's nvmem-cell-names property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_GET_BY_NAME
 */
#define NVMEM_CELL_INST_GET_BY_NAME(inst, name) NVMEM_CELL_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Like NVMEM_CELL_GET_BY_NAME(), with a fallback to a default value.
 *
 * If the devicetree node identifier 'node_id' refers to a node with a property
 * 'nvmem-cells', this expands to <tt>NVMEM_CELL_GET_BY_NAME(node_id, name)</tt>. The
 * @p default_value parameter is not expanded in this case. Otherwise, this
 * expands to @p default_value.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined by
 *             the node's nvmem-cell-names property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct nvmem_cell for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see NVMEM_CELL_INST_GET_BY_NAME_OR
 */
#define NVMEM_CELL_GET_BY_NAME_OR(node_id, name, default_value)                                    \
	COND_CODE_1(DT_PROP_HAS_NAME(node_id, nvmem_cells, name),                                  \
		    (NVMEM_CELL_GET_BY_NAME(node_id, name)),                                       \
		    (default_value))

/**
 * @brief Like NVMEM_CELL_INST_GET_BY_NAME(), with a fallback to a default
 *        value.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined by
 *             the node's nvmem-cell-names property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct nvmem_cell for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see NVMEM_CELL_GET_BY_NAME_OR
 */
#define NVMEM_CELL_INST_GET_BY_NAME_OR(inst, name, default_value)                                  \
	NVMEM_CELL_GET_BY_NAME_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Static initializer for a struct nvmem_cell.
 *
 * This returns a static initializer for a struct nvmem_cell given a devicetree
 * node identifier and an index.
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
 *	const struct nvmem_cell cell =
 *		NVMEM_CELL_GET_BY_IDX(DT_NODELABEL(eth), 0);
 *
 *	// Initializes 'cell' to:
 *	// {
 *	//	.dev = DEVICE_DT_GET(DT_NODELABEL(mac_eeprom)),
 *	//	.offset = 0xfa,
 *	//	.size = 6,
 *	//	.read_only = false,
 *	// }
 * @endcode
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into 'nvmem-cells' property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_INST_GET_BY_IDX
 */
#define NVMEM_CELL_GET_BY_IDX(node_id, idx) NVMEM_CELL_INIT(DT_NVMEM_CELL_BY_IDX(node_id, idx))

/**
 * @brief Static initializer for a struct nvmem_cell from a DT_DRV_COMPAT
 *        instance.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Logical index into 'nvmem-cells' property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_GET_BY_IDX
 */
#define NVMEM_CELL_INST_GET_BY_IDX(inst, idx) NVMEM_CELL_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Like NVMEM_CELL_GET_BY_IDX(), with a fallback to a default value.
 *
 * If the devicetree node identifier 'node_id' refers to a node with a property
 * 'nvmem-cells', this expands to <tt>NVMEM_CELL_GET_BY_IDX(node_id, idx)</tt>. The
 * @p default_value parameter is not expanded in this case. Otherwise, this
 * expands to @p default_value.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into 'nvmem-cells' property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct nvmem_cell for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see NVMEM_CELL_INST_GET_BY_IDX_OR
 */
#define NVMEM_CELL_GET_BY_IDX_OR(node_id, idx, default_value)                                      \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, nvmem_cells, idx),                                    \
		    (NVMEM_CELL_GET_BY_IDX(node_id, idx)),                                         \
		    (default_value))

/**
 * @brief Like NVMEM_CELL_INST_GET_BY_IDX(), with a fallback to a default
 *        value.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Logical index into 'nvmem-cells' property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct nvmem_cell for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see NVMEM_CELL_GET_BY_IDX_OR
 */
#define NVMEM_CELL_INST_GET_BY_IDX_OR(inst, idx, default_value)                                    \
	NVMEM_CELL_GET_BY_IDX_OR(DT_DRV_INST(inst), idx, default_value)

/**
 * @brief Read data from an NVMEM cell.
 *
 * @param cell The NVMEM cell.
 * @param data Buffer to store read data.
 * @param off The offset to start reading from.
 * @param len Number of bytes to read.
 *
 * @kconfig_dep{CONFIG_NVMEM}
 *
 * @retval -EINVAL Invalid offset or length arguments.
 * @retval -ENODEV The controller device is not ready.
 * @retval -ENXIO  No runtime device API available.
 * @return the result of the underlying device API call.
 */
int nvmem_cell_read(const struct nvmem_cell *cell, void *data, off_t off, size_t len);

/**
 * @brief Write data to an NVMEM cell.
 *
 * @param cell The NVMEM cell.
 * @param data Buffer with data to write.
 * @param off The offset to start writing to.
 * @param len Number of bytes to write.
 *
 * @kconfig_dep{CONFIG_NVMEM}
 *
 * @retval -EINVAL Invalid offset or length arguments.
 * @retval -EROFS  Writing to a read-only NVMEM Cell.
 * @retval -ENODEV The controller device is not ready.
 * @retval -ENXIO  No runtime device API available.
 * @return the result of the underlying device API call.
 */
int nvmem_cell_write(const struct nvmem_cell *cell, const void *data, off_t off, size_t len);

/**
 * @brief Validate that the NVMEM cell is ready.
 *
 * @param cell The NVMEM cell.
 *
 * @return true if the NVMEM cell is ready for use and false otherwise.
 */
static inline bool nvmem_cell_is_ready(const struct nvmem_cell *cell)
{
	return cell != NULL && device_is_ready(cell->dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NVMEM_H_ */
