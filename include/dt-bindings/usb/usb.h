/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_USB_USB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_USB_USB_H_

/* Use macros since we need to use them in the preprocessor conditionals.
 * These macros are expected to match the order in the yaml
 * (dts/bindings/usb/usb.yaml)
 */
#define DT_USB_MAXIMUM_SPEED_LOW_SPEED		0
#define DT_USB_MAXIMUM_SPEED_FULL_SPEED		1
#define DT_USB_MAXIMUM_SPEED_HIGH_SPEED		2
#define DT_USB_MAXIMUM_SPEED_SUPER_SPEED	3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_USB_USB_H_ */
