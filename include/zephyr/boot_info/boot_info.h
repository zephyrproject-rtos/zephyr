/*
 * Copyright (c) 2023 Laczen
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for boot_info subsys
 */

#ifndef ZEPHYR_INCLUDE_BOOT_INFO_BOOT_INFO_H_
#define ZEPHYR_INCLUDE_BOOT_INFO_BOOT_INFO_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief boot_info api
 *
 * @defgroup boot_info_api boot_info Interface
 * @ingroup storage_apis
 * @{
 */

/**
 * @brief boot_info: subsystem to share information between a bootloader and
 * and application started from the bootloader. The boot_info subsystem stores
 * its information in a backend that can be a fixed-partition in flash, a bbram
 * device or a eeprom device. It can also be a memory-region (RAM) by using the
 * flash simulator device.
 *
 * The usage pattern of the boot_info system is:
 * a. Create a array for the boot_info data:
 *    uint8_t data[boot_info_get_size(DT_NODELABEL(boot_info))];
 * b. Get the boot_info data:
 *    int rc = boot_info_get(DT_NODELABEL(boot_info), data);
 * c. Modify the data (optional)
 * d. Write back the data (optional):
 *    int rc = boot_info_set(DT_NODELABEL(boot_info), data);
 *
 * The boot_info is defined in the dts as:
 * / {
 *      boot_info: boot_info {
 *              compatible = "zephyr,boot-info-flash";
 *              backend = <&backend-node>; # phandle to fixed partition on flash
 *		size = <...>; # optional
 *		offset = <...>; # optional
 *      };
 * }
 *
 * / {
 *      boot_info: boot_info {
 *              compatible = "zephyr,boot-info-bbram";
 *              backend = <&backend-node>; # phandle to bbram device
 *		size = <...>; # optional
 *		offset = <...>; # optional
 *      };
 * }
 *
 * / {
 *      boot_info: boot_info {
 *              compatible = "zephyr,boot-info-eeprom";
 *              backend = <&backend-node>; # phandle to eeprom device
 *		size = <...>; # optional
 *		offset = <...>; # optional
 *      };
 * }
 *
 */

/**
 * @brief boot_info_get_device: Get the device that is used as backend for
 *	  the boot_info area.
 *
 * @param _node: nodelabel of the boot_info area (e.g. DT_NODELABEL(boot_info)).
 * @retval size of the boot_info area.
 */
#define boot_info_get_device(_node) _CONCAT(bi_get_device_, _node)()

/**
 * @brief boot_info_get_size: Get the size of the boot_info area.
 *
 * @param _node: nodelabel of the boot_info area (e.g. DT_NODELABEL(boot_info)).
 * @retval size of the boot_info area.
 */
#define boot_info_get_size(_node) _CONCAT(bi_get_size_, _node)()

/**
 * @brief boot_info_get: Get the boot_info area.
 *
 * @param _node: nodelabel of the boot_info area (e.g. DT_NODELABEL(boot_info)).
 * @param [out] _data: buffer for the boot_info area.
 * @retval 0 on success, negative error code if error.
 */
#define boot_info_get(_node, _data) _CONCAT(bi_get_, _node)(_data)

/**
 * @brief boot_info_set: Set the boot_info area.
 *
 * @param _node: nodelabel of the boot_info area (e.g. DT_NODELABEL(boot_info)).
 * @param [in] _data: buffer for the boot_info area.
 * @retval 0 on success, negative error code if error.
 */
#define boot_info_set(_node, _data) _CONCAT(bi_set_, _node)(_data)

/**
 * @}
 */

#define DEFINE_BOOT_INFO_PROTO(inst)                                            \
	const struct device *bi_get_device_##inst(void);			\
	size_t bi_get_size_##inst(void);                                        \
	int bi_get_##inst(void *data);                                          \
	int bi_set_##inst(const void *data);

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_bbram, DEFINE_BOOT_INFO_PROTO)

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_flash, DEFINE_BOOT_INFO_PROTO)

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_eeprom, DEFINE_BOOT_INFO_PROTO)

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_retained_mem, DEFINE_BOOT_INFO_PROTO)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BOOT_INFO_BOOT_INFO_H_ */
