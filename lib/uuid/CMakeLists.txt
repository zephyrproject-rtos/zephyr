# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_UUID uuid.c)

zephyr_library_link_libraries_ifdef(CONFIG_UUID_V5 mbedTLS)
