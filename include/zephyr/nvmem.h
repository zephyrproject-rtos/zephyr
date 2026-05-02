/*
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public NVMEM header file.
 * @ingroup nvmem_interface
 */

#ifndef ZEPHYR_INCLUDE_NVMEM_H_
#define ZEPHYR_INCLUDE_NVMEM_H_

/**
 * @defgroup nvmem_interface NVMEM
 * @brief Non-volatile memory cells.
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/nvmem.h>
#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-Volatile Memory cell representation.
 */
struct nvmem_cell {
	/* @cond INTERNAL_HIDDEN */

	/** NVMEM parent controller device instance. */
	const struct device *dev;
	/** Offset of the NVMEM cell relative to the parent controller's base address */
	off_t offset;
	/** Size of the NVMEM cell */
	size_t size;
	/** Indicator if the NVMEM cell is read-write or read-only */
	bool read_only;

	/* @endcond */
};

/**
 * @brief Get a static initializer for a struct nvmem_cell.
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
 * @param node_id Node identifier for an NVMEM cell.
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
 * @brief Get a static initializer for a struct nvmem_cell by name.
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
 * @param node_id Node identifier for a node with an nvmem-cells property.
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined by
 *             the node's nvmem-cell-names property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_INST_GET_BY_NAME
 */
#define NVMEM_CELL_GET_BY_NAME(node_id, name) NVMEM_CELL_INIT(DT_NVMEM_CELL_BY_NAME(node_id, name))

/**
 * @brief Get a static initializer for a struct nvmem_cell from a DT_DRV_COMPAT
 *        instance by name.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined by
 *             the node's nvmem-cell-names property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_GET_BY_NAME
 */
#define NVMEM_CELL_INST_GET_BY_NAME(inst, name) NVMEM_CELL_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a static initializer for a struct nvmem_cell by name, with a fallback.
 *
 * If the devicetree node identifier 'node_id' refers to a node with a property
 * 'nvmem-cells', this expands to <tt>NVMEM_CELL_GET_BY_NAME(node_id, name)</tt>. The
 * @p default_value parameter is not expanded in this case. Otherwise, this
 * expands to @p default_value.
 *
 * @param node_id Node identifier for a node that may have an nvmem-cells property.
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
 * @brief Get a static initializer for a struct nvmem_cell from a DT_DRV_COMPAT
 *        instance by name, with a fallback.
 *
 * @param inst DT_DRV_COMPAT instance number.
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
 * @brief Get a static initializer for a struct nvmem_cell by index.
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
 * @param node_id Node identifier for a node with an nvmem-cells property.
 * @param idx Logical index into 'nvmem-cells' property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_INST_GET_BY_IDX
 */
#define NVMEM_CELL_GET_BY_IDX(node_id, idx) NVMEM_CELL_INIT(DT_NVMEM_CELL_BY_IDX(node_id, idx))

/**
 * @brief Get a static initializer for a struct nvmem_cell from a DT_DRV_COMPAT
 *        instance by index.
 *
 * @param inst DT_DRV_COMPAT instance number.
 * @param idx Logical index into 'nvmem-cells' property.
 *
 * @return Static initializer for a struct nvmem_cell for the property.
 *
 * @see NVMEM_CELL_GET_BY_IDX
 */
#define NVMEM_CELL_INST_GET_BY_IDX(inst, idx) NVMEM_CELL_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a static initializer for a struct nvmem_cell by index, with a fallback.
 *
 * If the devicetree node identifier 'node_id' refers to a node with a property
 * 'nvmem-cells', this expands to <tt>NVMEM_CELL_GET_BY_IDX(node_id, idx)</tt>. The
 * @p default_value parameter is not expanded in this case. Otherwise, this
 * expands to @p default_value.
 *
 * @param node_id Node identifier for a node that may have an nvmem-cells property.
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
 * @brief Get a static initializer for a struct nvmem_cell from a DT_DRV_COMPAT
 *        instance by index, with a fallback.
 *
 * @param inst DT_DRV_COMPAT instance number.
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
 * @param cell NVMEM cell to read from.
 * @param[out] data Buffer to store the read data.
 *                  Must be at least @p len bytes.
 * @param off Offset within the cell to start reading from, in bytes.
 *            Must be less than the cell size.
 * @param len Number of bytes to read.
 *            @p off + @p len must not exceed the cell size.
 *
 * @kconfig_dep{CONFIG_NVMEM}
 *
 * @retval -EINVAL Invalid offset or length arguments.
 * @retval -ENODEV The controller device is not ready.
 * @retval -ENXIO  No runtime device API available.
 * @return The result of the underlying device API call.
 */
int nvmem_cell_read(const struct nvmem_cell *cell, void *data, off_t off, size_t len);

/**
 * @brief Write data to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 *             Must not be read-only.
 * @param data Buffer containing data to write.
 *             Must be at least @p len bytes.
 * @param off Offset within the cell to start writing to, in bytes.
 *            Must be less than the cell size.
 * @param len Number of bytes to write.
 *            @p off + @p len must not exceed the cell size.
 *
 * @kconfig_dep{CONFIG_NVMEM}
 *
 * @retval -EINVAL Invalid offset or length arguments.
 * @retval -EROFS  Writing to a read-only NVMEM Cell.
 * @retval -ENODEV The controller device is not ready.
 * @retval -ENXIO  No runtime device API available.
 * @return The result of the underlying device API call.
 */
int nvmem_cell_write(const struct nvmem_cell *cell, const void *data, off_t off, size_t len);

/**
 * @brief Check if an NVMEM cell is ready.
 *
 * @param cell NVMEM cell to check. May be NULL.
 *
 * @return True if the NVMEM cell is ready for use and false otherwise.
 */
static inline bool nvmem_cell_is_ready(const struct nvmem_cell *cell)
{
	return cell != NULL && device_is_ready(cell->dev);
}

/**
 * @brief Check if an NVMEM cell is read-only.
 *
 * @param cell NVMEM cell to check. Can't be NULL.
 *
 * @return True if the NVMEM cell is read-only and false otherwise.
 */
static inline bool nvmem_cell_is_read_only(const struct nvmem_cell *cell)
{
	return cell->read_only;
}

/**
 * @brief Read a little-endian 16-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_le16(const struct nvmem_cell *cell, uint16_t *val, off_t off)
{
	uint8_t buf[sizeof(uint16_t)];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_le16(buf);
	}

	return ret;
}

/**
 * @brief Read a big-endian 16-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_be16(const struct nvmem_cell *cell, uint16_t *val, off_t off)
{
	uint8_t buf[sizeof(uint16_t)];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_be16(buf);
	}

	return ret;
}

/**
 * @brief Read a little-endian 32-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_le32(const struct nvmem_cell *cell, uint32_t *val, off_t off)
{
	uint8_t buf[sizeof(uint32_t)];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_le32(buf);
	}

	return ret;
}

/**
 * @brief Read a big-endian 32-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_be32(const struct nvmem_cell *cell, uint32_t *val, off_t off)
{
	uint8_t buf[sizeof(uint32_t)];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_be32(buf);
	}

	return ret;
}

/**
 * @brief Read a little-endian 48-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_le48(const struct nvmem_cell *cell, uint64_t *val, off_t off)
{
	uint8_t buf[6];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_le48(buf);
	}

	return ret;
}

/**
 * @brief Read a big-endian 48-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_be48(const struct nvmem_cell *cell, uint64_t *val, off_t off)
{
	uint8_t buf[6];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_be48(buf);
	}

	return ret;
}

/**
 * @brief Read a little-endian 64-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_le64(const struct nvmem_cell *cell, uint64_t *val, off_t off)
{
	uint8_t buf[sizeof(uint64_t)];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_le64(buf);
	}

	return ret;
}

/**
 * @brief Read a big-endian 64-bit value from an NVMEM cell.
 *
 * @param cell NVMEM cell to read from.
 * @param[out] val Pointer to store the host-endian result.
 * @param off Offset within the cell to start reading from, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_read_be64(const struct nvmem_cell *cell, uint64_t *val, off_t off)
{
	uint8_t buf[sizeof(uint64_t)];
	int ret;

	ret = nvmem_cell_read(cell, buf, off, sizeof(buf));
	if (ret == 0) {
		*val = sys_get_be64(buf);
	}

	return ret;
}

/**
 * @brief Write a little-endian 16-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_le16(const struct nvmem_cell *cell, uint16_t val, off_t off)
{
	uint8_t buf[sizeof(uint16_t)];

	sys_put_le16(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

/**
 * @brief Write a big-endian 16-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_be16(const struct nvmem_cell *cell, uint16_t val, off_t off)
{
	uint8_t buf[sizeof(uint16_t)];

	sys_put_be16(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

/**
 * @brief Write a little-endian 32-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_le32(const struct nvmem_cell *cell, uint32_t val, off_t off)
{
	uint8_t buf[sizeof(uint32_t)];

	sys_put_le32(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

/**
 * @brief Write a big-endian 32-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_be32(const struct nvmem_cell *cell, uint32_t val, off_t off)
{
	uint8_t buf[sizeof(uint32_t)];

	sys_put_be32(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

/**
 * @brief Write a little-endian 48-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_le48(const struct nvmem_cell *cell, uint64_t val, off_t off)
{
	uint8_t buf[6];

	sys_put_le48(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

/**
 * @brief Write a big-endian 48-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_be48(const struct nvmem_cell *cell, uint64_t val, off_t off)
{
	uint8_t buf[6];

	sys_put_be48(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

/**
 * @brief Write a little-endian 64-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_le64(const struct nvmem_cell *cell, uint64_t val, off_t off)
{
	uint8_t buf[sizeof(uint64_t)];

	sys_put_le64(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

/**
 * @brief Write a big-endian 64-bit value to an NVMEM cell.
 *
 * @param cell NVMEM cell to write to.
 * @param val Host-endian value to write.
 * @param off Offset within the cell to start writing to, in bytes.
 *
 * @return 0 on success, or a negative error code.
 */
static inline int nvmem_cell_write_be64(const struct nvmem_cell *cell, uint64_t val, off_t off)
{
	uint8_t buf[sizeof(uint64_t)];

	sys_put_be64(val, buf);

	return nvmem_cell_write(cell, buf, off, sizeof(buf));
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NVMEM_H_ */
