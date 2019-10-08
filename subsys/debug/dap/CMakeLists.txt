# Copyright (c) 2019, PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/subsys/debug/dap)

zephyr_sources_ifdef(CONFIG_CMSIS_DAP cmsis_dap.c)
zephyr_sources_ifdef(CONFIG_CMSIS_DAP_USB_HID_INTERFACE dap_hid_iface.c)
