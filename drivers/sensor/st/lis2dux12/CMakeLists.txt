# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(lis2dux12.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DUX12_TRIGGER    lis2dux12_trigger.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_LIS2DUX12_ENABLED  lis2dux12_api.c )
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_LIS2DUXS12_ENABLED lis2duxs12_api.c )

zephyr_library_include_directories(../stmemsc)
