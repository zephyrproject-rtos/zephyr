/**
 * @file
 * @brief NVMEM Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2024, Andriy Gelman
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
 * @brief Get the pointer to the nvmem device from the nvmem-cells property
 *
 * Example devicetree fragment:
 *
 *	mac_eeprom: mac_eeprom@2 {
 *		#address-cells = <1>;
 *		#size-cells = <1>;
 *		mac_address: mac_address@fa {
 *			reg = <0xfa 0x06>;
 *		};
 *	};
 *
 *      eth: ethernet {
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *	};
 *
 * Example usage:
 *
 *     DT_NVMEM_DEV_BY_IDX(DT_NODELABEL(eth), 0) // DEVICE_DT_GET(DT_NODELABEL(mac_eeprom))
 *
 * @param node_id node identifier for a node with nvmem-cells property
 * @param idx index into the nvmem-cells proprty
 * @return the pointer to the nvmem device at index idx
 */

#define DT_NVMEM_DEV_BY_IDX(node_id, idx)                                                          \
	DEVICE_DT_GET(DT_PARENT(DT_PHANDLE_BY_IDX(node_id, nvmem_cells, idx)))

/**
 * @brief Get the target address referenced by the nvmem-cells property
 *
 * Example devicetree fragment:
 *
 *	mac_eeprom: mac_eeprom@2 {
 *		status = "okay";
 *
 *		#address-cells = <1>;
 *		#size-cells = <1>;
 *
 *		mac_address: mac_address@fa {
 *			reg = <0xfa 0x06>;
 *		};
 *	};
 *
 *     eth: ethernet@ {
 *		...
 *		nvmem-cells = <&mac_address>;
 *		nvmem-cell-names = "mac-address";
 *		...
 *     };
 *
 * Example usage:
 *
 *     DT_NVMEM_ADDR_BY_IDX(DT_NODELABEL(eth), 0) // 0xfa
 *
 * @param node_id node identifier for a node with nvmem-cells property
 * @param idx index into the nvmem-cells property
 * @return the address of the nvmem device where the information is stored
 */

#define DT_NVMEM_ADDR_BY_IDX(node_id, idx) DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, nvmem_cells, idx))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_DEVICETREE_NVMEM_H_ */
