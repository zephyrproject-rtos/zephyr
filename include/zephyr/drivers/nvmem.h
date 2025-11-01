/*
 * This device driver model is imspired by Linux's NVMEM subsystem, which is
 * Copyright (C) 2015 Srinivas Kandagatla <srinivas.kandagatla@linaro.org>
 * Copyright (C) 2013 Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * Referred Zephyr EEPROM device driver API during the design process, which is
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Heavily based on drivers/flash.h which is:
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * Macros was copied from Zephyr's nvmem subsystem implementation, which is
 * Copyright (c) 2025 Basalte bv
 *
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup nvmem_model_interface
 * @brief NVMEM provider driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NVMEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_NVMEM_H_

/**
 * @brief Interfaces for NVMEM provider devices (EEPROM, OTP, FUSE, FRAM, etc.).
 * @defgroup nvmem_model_interface NVMEM providers
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <sys/types.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/nvmem.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-Volatile Memory cell representation.
 *
 * Example usage:
 *
 * @code{.c}
 *	const struct nvmem_cell cell = NVMEM_CELL_INIT(DT_NODELABEL(mac_address));
 *
 *	// Read the whole cell (up to buffer size) using the provider API
 *	uint8_t mac[6];
 *
 *	int ret = nvmem_read(cell.dev, cell.offset, mac,
 *						MIN(cell.size, sizeof(mac)));
 *
 *	if (ret < 0) {
 *		// Handle read error
 *	}
 *
 *	// Read 3 bytes from within the cell (offset inside cell = 0)
 *	uint8_t oui[3];
 *	
 *	ret = nvmem_read(cell.dev, cell.offset + 0, oui, 3);
 *	
 *	if (ret < 0) {
 *		// Handle read error
 *	}
 * @endcode
 */
struct nvmem_cell {
	/** NVMEM parent controller device instance. */
	const struct device *dev;
	/**
	 * Byte offset of this cell within the provider's logical data space.
	 *
	 * For fixed-layout cells, this equals the first value of the
	 * cell's reg = <offset size> in Devicetree and represents a byte
	 * offset from the start (0) of the NVMEM provider's data space.
	 */
	off_t offset;
	/** Size of the NVMEM cell */
	size_t size;
	/** Indicator if the NVMEM cell is read-write or read-only */
	bool read_only;
};

/** @brief Storage type classification. */
enum nvmem_type {
	NVMEM_TYPE_UNKNOWN = 0,
	NVMEM_TYPE_EEPROM,
	NVMEM_TYPE_OTP,
	NVMEM_TYPE_FRAM,
};

/**
 * @brief NVMEM provider information
 *
 * @type:	Storage classification hint (e.g. EEPROM, OTP, FUSE, FRAM). Used by
 *		upper layers for diagnostics and policy defaults; providers
 *		still enforce hardware semantics (such as OTP 0->1 programming)
 *		in their read/write paths.
 *
 * @read_only:	Effective runtime mutability of the device. When true,
 *		providers must reject writes with -EROFS. This may reflect
 *		hardware/state such as WP GPIOs, fuse/lifecycle locks, or
 *		build-time write gates, and can change at runtime.
 */
struct nvmem_info {
	enum nvmem_type type;
	bool read_only;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/** @brief Callback to read from an NVMEM provider. */
typedef int (*nvmem_read_t)(const struct device *dev, off_t offset,
			void *buf, size_t len);

/** @brief Callback to write to an NVMEM provider. */
typedef int (*nvmem_write_t)(const struct device *dev, off_t offset,
			const void *buf, size_t len);

/** @brief Callback to get total size from an NVMEM provider. */
typedef size_t (*nvmem_get_size_t)(const struct device *dev);

/** @brief Optional provider hook to expose information to the core. */
typedef const struct nvmem_info *(*nvmem_get_info_t)(const struct device *dev);

/** @brief NVMEM provider driver API structure. */
__subsystem struct nvmem_driver_api {
	nvmem_read_t read;
	nvmem_write_t write;
	nvmem_get_size_t get_size;
	nvmem_get_info_t get_info;
};

/** @endcond */

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
#define NVMEM_CELL_INIT(node_id)					\
	{								\
		.dev = DEVICE_DT_GET(DT_MTD_FROM_NVMEM_CELL(node_id)),	\
		.offset = DT_REG_ADDR(node_id),				\
		.size = DT_REG_SIZE(node_id),				\
		.read_only = DT_PROP(node_id, read_only),		\
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
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined
 *             by the node's nvmem-cell-names property.
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
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined
 *             by the node's nvmem-cell-names property.
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
 * 'nvmem-cells', this expands to <tt>NVMEM_CELL_GET_BY_NAME(node_id, name)</tt>.
 * The @p default_value parameter is not expanded in this case. Otherwise, this
 * expands to @p default_value.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined
 *             by the node's nvmem-cell-names property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct nvmem_cell for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see NVMEM_CELL_INST_GET_BY_NAME_OR
 */
#define NVMEM_CELL_GET_BY_NAME_OR(node_id, name, default_value)	\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, nvmem_cells),	\
		(NVMEM_CELL_GET_BY_NAME(node_id, name)),	\
		(default_value))

/**
 * @brief Like NVMEM_CELL_INST_GET_BY_NAME(), with a fallback to a default value.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Lowercase-and-underscores name of an nvmem-cells element as defined
 *             by the node's nvmem-cell-names property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct nvmem_cell for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see NVMEM_CELL_GET_BY_NAME_OR
 */
#define NVMEM_CELL_INST_GET_BY_NAME_OR(inst, name, default_value)	\
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
 * 'nvmem-cells', this expands to <tt>NVMEM_CELL_GET_BY_IDX(node_id, idx)</tt>.
 * The @p default_value parameter is not expanded in this case. Otherwise, this
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
#define NVMEM_CELL_GET_BY_IDX_OR(node_id, idx, default_value)	\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, nvmem_cells),	\
		(NVMEM_CELL_GET_BY_IDX(node_id, idx)),		\
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
#define NVMEM_CELL_INST_GET_BY_IDX_OR(inst, idx, default_value)		\
	NVMEM_CELL_GET_BY_IDX_OR(DT_DRV_INST(inst), idx, default_value)

/**
 * @brief Read bytes from an NVMEM provider.
 *
 * Providers should implement device-specific semantics and alignment rules.
 *
 * @param dev		NVMEM provider device instance.
 * @param offset	Start byte offset within the provider's logical data space.
 *
 * @param buf		Destination buffer.
 * @param len		Number of bytes to read.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
__syscall int nvmem_read(const struct device *dev, off_t offset,
			void *buf, size_t len);
static inline int z_impl_nvmem_read(const struct device *dev, off_t offset,
			void *buf, size_t len)
{
	return DEVICE_API_GET(nvmem, dev)->read(dev, offset, buf, len);
}

/**
 * @brief Write bytes to an NVMEM provider.
 *
 * @note: Providers must enforce their semantics (e.g., OTP 0->1 only) and
 *	  return -EROFS for read-only configurations.
 *
 * @param dev		NVMEM provider device instance.
 * @param offset	Start byte offset within the provider's logical data space.
 *
 * @param buf		Source buffer.
 * @param len		Number of bytes to write.
 *
 * @retval 0 on success.
 * @retval negative errno on failure.
 */
__syscall int nvmem_write(const struct device *dev, off_t offset,
			const void *buf, size_t len);
static inline int z_impl_nvmem_write(const struct device *dev, off_t offset,
			const void *buf, size_t len)
{
	return DEVICE_API_GET(nvmem, dev)->write(dev, offset, buf, len);
}

/**
 * @brief Get total size in bytes of an NVMEM provider.
 *
 * @param dev	NVMEM provider device instance.
 *
 * @return Size in bytes.
 */
__syscall size_t nvmem_get_size(const struct device *dev);
static inline size_t z_impl_nvmem_get_size(const struct device *dev)
{
	return DEVICE_API_GET(nvmem, dev)->get_size(dev);
}

/**
 * @brief Get NVMEM provider information when available.
 *
 * @param dev	NVMEM provider device instance.
 *
 * @note Providers may return NULL to indicate no exported information.
 *
 * @retval A pointer to the nvmem_info structure.
 * @retval NULL if no information is available.
 */
__syscall const struct nvmem_info * nvmem_get_info(const struct device *dev);
static inline const struct nvmem_info *z_impl_nvmem_get_info(const struct device *dev)
{
	const struct nvmem_driver_api *api = DEVICE_API_GET(nvmem, dev);
	return api->get_info ? api->get_info(dev) : NULL;
}

/**
 * @brief Validate that the NVMEM cell is ready.
 *
 * @param cell The NVMEM cell.
 *
 * @retval true if the NVMEM cell is ready for use.
 * @retval false if the NVMEM cell is NOT ready for use.
 */
static inline bool nvmem_cell_is_ready(const struct nvmem_cell *cell)
{
	return cell != NULL && device_is_ready(cell->dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/nvmem.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_NVMEM_H_ */
