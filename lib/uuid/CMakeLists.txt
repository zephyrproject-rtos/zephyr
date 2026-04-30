# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_UUID uuid.c)

if(CONFIG_UUID_V5)
  zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS_BUILTIN mbedTLS)
endif()
