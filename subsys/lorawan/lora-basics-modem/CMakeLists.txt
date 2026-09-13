# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Add include path for local headers (lbm_priv.h)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_sources(lbm_engine.c)
zephyr_library_sources(lbm_priv.c)
zephyr_library_sources(lorawan.c)
