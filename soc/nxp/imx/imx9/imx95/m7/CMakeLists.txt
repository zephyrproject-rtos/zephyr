# Copyright 2024 NXP

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(soc.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
