# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(lis2dux12.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DUX12_TRIGGER    lis2dux12_trigger.c)

zephyr_library_include_directories(../stmemsc)
