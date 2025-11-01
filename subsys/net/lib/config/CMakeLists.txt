# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources_ifdef(CONFIG_NET_CONFIG_SETTINGS init.c)

# Disable optimization for only init.c in order to avoid false
# positives from bounds checking in optimized builds.
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/init.c
  PROPERTIES COMPILE_OPTIONS "-O0"
)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

if(CONFIG_NET_CONFIG_SETTINGS)
  zephyr_library_sources_ifdef(
    CONFIG_NET_L2_IEEE802154
    ieee802154_settings.c
    )
endif()

zephyr_library_sources_ifdef(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT init_clock_sntp.c)
