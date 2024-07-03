# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_OPTEE optee)

if (CONFIG_TEE)
zephyr_library()
zephyr_library_sources(tee.c)
endif()
