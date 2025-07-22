# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/fuel_gauge.h)

zephyr_library()

add_subdirectory_ifdef(CONFIG_SBS_GAUGE_NEW_API		sbs_gauge)
add_subdirectory_ifdef(CONFIG_FUEL_GAUGE_COMPOSITE	composite)
add_subdirectory_ifdef(CONFIG_MAX17048		max17048)
add_subdirectory_ifdef(CONFIG_BQ27Z746		bq27z746)
add_subdirectory_ifdef(CONFIG_FUEL_GAUGE_AXP2101	axp2101)
add_subdirectory_ifdef(CONFIG_LC709203F		lc709203f)
add_subdirectory_ifdef(CONFIG_SY24561  sy24561)

zephyr_library_sources_ifdef(CONFIG_USERSPACE fuel_gauge_syscall_handlers.c)

if (CONFIG_EMUL AND CONFIG_USERSPACE)
  zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/emul_fuel_gauge.h)
  zephyr_library_sources(emul_fuel_gauge_syscall_handlers.c)
endif()
