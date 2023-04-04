# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(
	CONFIG_USB_HOST_MSC_DRIVER
	usbh_msc.c
)
