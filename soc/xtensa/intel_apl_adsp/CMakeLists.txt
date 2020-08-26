# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(${ARCH}_${SOC_NAME})
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_sources(adsp.c)
zephyr_library_sources(soc.c)
zephyr_library_sources(main_entry.S)

zephyr_library_sources_ifdef(CONFIG_SMP soc_mp.c)

add_subdirectory(common)
