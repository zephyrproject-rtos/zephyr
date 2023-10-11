# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/usb/usb_bc12.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USB_BC12_PI3USB9201	bc12_pi3usb9201.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_BC12_PI3USB9201 emul_bc12_pi3usb9201.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_BC12_PI3USB9201 .)
