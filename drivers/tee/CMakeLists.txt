# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_OPTEE optee)

zephyr_library()
zephyr_library_sources(tee.c)
