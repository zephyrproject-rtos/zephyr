# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(ticker.c)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic)
