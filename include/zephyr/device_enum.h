/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_ENUM_H_
#define ZEPHYR_INCLUDE_DEVICE_ENUM_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <device_enum_generated.h>

/**
 * @brief Get the enumeration of a device from device class and index
 *
 * DEVICE_ENUM(rtc, 0)
 *
 * expands to
 *
 * DE_rtc_0
 */
#define DEVICE_ENUM(device_class, index) \
	_CONCAT(DE_, _CONCAT(device_class, _CONCAT(_, index)))

/**
 * @brief Get device symbol name from device enumeration
 *
 * DEVICE_NAME_FROM_ENUM(DEVICE_ENUM(uart, 0))
 *
 * could expand to
 *
 * __device_dts_ord_10
 */
#define DEVICE_NAME_FROM_ENUM(device_enum) _CONCAT(device_enum, _SYM)

/**
 * @brief Get pointer to device by class and index
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
#define DEVICE_POINTER_FROM_ENUM(device_enum) \
	(&DEVICE_NAME_FROM_ENUM(device_enum))

#define DEVICE_CLASS_FROM_ENUM(device_enum) _CONCAT(device_enum, _CLASS)

/**
 * @brief Get the device enumeration from a devicetree node identifier
 *
 * DEVICE_CLASS_ENUM_FROM_DT(DT_ALIAS(console))
 *
 * could expand to
 *
 * DC_E_uart_0
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_ENUM_FROM_DT_NODE(node_id) \
	_CONCAT(DE_DT_NODE_RL_, node_id)

/**
 * @brief Get the device enumeration from a devicetree node identifier
 *
 * DEVICE_DT_NODE_FROM_ENUM(DEVICE_CLASS_ENUM(uart, 0))
 *
 * could expand to
 *
 * DT_N_S_soc_S_serial_40004800
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_DT_NODE_FROM_ENUM(device_enum) \
	_CONCAT(device_enum, _DT_NODE)

/**
 * @brief Get the class of a device from device enumeration
 *
 * DEVICE_CLASS_ENUM_GET_CLASS(DEVICE_CLASS_ENUM(uart, 0))
 *
 * expands to
 *
 * uart
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_CLASS_FROM_ENUM(device_enum) \
	_CONCAT(device_enum, _CLASS)

/**
 * @brief Get the index of a device within its class from its enumeration
 *
 * DEVICE_CLASS_ENUM_GET_INDEX(DEVICE_CLASS_ENUM(uart, 0))
 *
 * expands to
 *
 * 0
 *
 * given the device exists and belongs to the UART class
 */
#define DEVICE_INDEX_FROM_ENUM(device_enum) \
	_CONCAT(device_enum, _INDEX)

/**
 * @brief Iterate through all devices of a specified class
 *
 * SOME_UART_FN(device_enum)
 *
 * DEVICE_CLASS_FOREACH_DEVICE_OF_CLASS(uart, SOME_UART_FN)
 */
#define DEVICE_FOREACH_DEVICE_OF_CLASS(device_class, fn)                \
	COND_CODE_1(IS_ENABLED(_CONCAT(DE_FOREACH_, device_class)),     \
		    (_CONCAT(DE_FOREACH_, device_class)(fn)),           \
		    ())

/**
 * @brief Iterate through all devices regardless of class
 *
 * SOME_DEVICE_FN(device_enum)
 *
 * DEVICE_CLASS_FOREACH_DEVICE(SOME_DEVICE_FN)
 */
#define DEVICE_FOREACH_DEVICE(fn) DE_FOREACH(fn)

#endif /* ZEPHYR_INCLUDE_DEVICE_ENUM_H_ */
