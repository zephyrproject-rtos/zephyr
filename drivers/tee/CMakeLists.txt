# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TEE)
zephyr_library()
zephyr_library_sources(tee.c)
endif()
