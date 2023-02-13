# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_sources(capture.c)
