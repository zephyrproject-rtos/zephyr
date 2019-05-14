/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_USB_USB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_USB_USB_H_

/* Ideally we'd generate this enum to match what's coming out of the YAML,
 * however, we dont have a good way to know how to name such an enum from
 * the generation point of view, so for now we just hand code the enum.  This
 * enum is expected to match the order in the yaml (dts/bindings/usb/usb.yaml)
 */

enum dt_usb_maximum_speed {
	DT_USB_MAXIMUM_SPEED_LOW_SPEED,
	DT_USB_MAXIMUM_SPEED_FULL_SPEED,
	DT_USB_MAXIMUM_SPEED_HIGH_SPEED,
	DT_USB_MAXIMUM_SPEED_SUPER_SPEED,
};

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_USB_USB_H_ */
