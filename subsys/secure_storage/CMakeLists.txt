# SPDX-License-Identifier: Apache-2.0

add_subdirectory(src)

zephyr_include_directories(
	include
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
