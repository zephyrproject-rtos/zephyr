# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(zbus.c)

if(CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE GREATER 0)
    zephyr_library_sources(zbus_runtime_observers.c)
endif()

if(CONFIG_ZBUS_STRUCTS_ITERABLE_ACCESS)
    zephyr_library_sources(zbus_iterable_sections.c)
    zephyr_linker_sources(DATA_SECTIONS zbus.ld)
endif()
