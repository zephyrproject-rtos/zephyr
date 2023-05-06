/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_CLASS_H_
#define ZEPHYR_INCLUDE_DEVICE_CLASS_H_

#include <zephyr/sys/util.h>

#include <device_class_generated.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the enumeration of a device from device class and index
 *
 * @example
 *
 * DEVICE_CLASS_ENUM(rtc, 0)
 *
 * expands to
 *
 * DC_E_rtc_0
 */
#define DEVICE_CLASS_ENUM(device_class, index) \
	_CONCAT(DC_E_, _CONCAT(_class, _CONCAT(_, _index)))

/**
 * @brief Get pointer to device by class and index
 *
 * @example
 *
 * to get a struct device by enumeration
 *
 * const struct device *rtc = DEVICE_CLASS_GET_DEVICE(DEVICE_CLASS_ENUM(rtc, 0));
 *
 * or to get a struct device from devicetree
 *
 * const struct device *rtc =
 *         DEVICE_CLASS_GET_DEVICE(DEVICE_CLASS_ENUM_FROM_DT(DT_ALIAS(console)));
 */
#define DEVICE_CLASS_GET_DEVICE(device_enum) \
	(&_CONCAT(device_enum, _SYM))

/**
 * @brief Get the number of devices which belong to specific device class
 * @example size_t number_of_rtc_devices = DEVICE_CLASS_NUM(rtc);
 */
#define DEVICE_CLASS_COUNT(device_class) \
	_CONCAT(DC_CNT_, device_class)

/**
 * @brief Get the device enumeration from a devicetree node identifier
 *
 * @example
 *
 * DEVICE_CLASS_ENUM_FROM_DT(DT_ALIAS(console))
 *
 * could expand to
 *
 * DEVICE_CLASS_E_uart_0
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_CLASS_ENUM_FROM_DT(node_id) \
	_CONCAT(DC_E_DT_RL_, node_id)

/**
 * @brief Get the device enumeration from a devicetree node identifier
 *
 * @example
 *
 * DEVICE_CLASS_DT_NODE_FROM_ENUM(DEVICE_CLASS_ENUM(uart, 0))
 *
 * could expand to
 *
 * DT_N_S_soc_S_serial_40004800
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_CLASS_DT_NODE_FROM_ENUM(device_enum) \
	_CONCAT(device_enum, _DT_NODE)

/**
 * @brief Get the class of a device  from device enumeration
 *
 * @example
 *
 * DEVICE_CLASS_ENUM_GET_CLASS(DEVICE_CLASS_ENUM(uart, 0))
 *
 * expands to
 *
 * uart
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_CLASS_ENUM_GET_CLASS(device_enum) \
	_CONCAT(device_enum, _CLASS)

/**
 * @brief Get the index of a device within its class from its enumeration
 *
 * @example
 *
 * DEVICE_CLASS_ENUM_GET_INDEX(DEVICE_CLASS_ENUM(uart, 0))
 *
 * expands to
 *
 * 0
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_CLASS_ENUM_GET_INDEX(device_enum) \
	_CONCAT(device_enum, _INDEX)

/**
 * @brief Iterate through all devices of a specified class
 *
 * @example
 *
 * #define SOME_UART_FN(device_enum)
 *
 * DEVICE_CLASS_FOREACH_DEVICE_OF_CLASS(uart, SOME_UART_FN)
 */
#define DEVICE_CLASS_FOREACH_DEVICE_OF_CLASS(device_class, fn) \
	_CONCAT(DC_E_FOREACH_, device_class)(fn)

/**
 * @brief Iterate through all devices regardless of class
 *
 * @example
 *
 * #define SOME_DEVICE_FN(device_enum)
 *
 * DEVICE_CLASS_FOREACH_DEVICE(SOME_DEVICE_FN)
 */
#define DEVICE_CLASS_FOREACH_DEVICE(fn) \
	DC_E_FOREACH(fn)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICE_CLASS_H_ */
