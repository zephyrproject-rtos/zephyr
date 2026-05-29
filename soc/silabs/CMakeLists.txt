# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2017 Christian Taedcke

zephyr_include_directories(${SOC_FAMILY})
zephyr_include_directories(${SOC_FAMILY}/${SOC_SERIES})

add_subdirectory(common)

add_subdirectory_ifdef(CONFIG_SOC_FAMILY_SILABS_S2      silabs_s2)
add_subdirectory_ifdef(CONFIG_SOC_FAMILY_SILABS_SIWX91X silabs_siwx91x)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_SIM3U          silabs_sim3/sim3u)
