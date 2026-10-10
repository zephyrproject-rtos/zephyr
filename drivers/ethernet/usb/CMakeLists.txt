# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/usb/host)
zephyr_library_sources_ifdef(CONFIG_ETH_LAN78XX eth_lan78xx.c)
