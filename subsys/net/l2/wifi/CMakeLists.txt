# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories_ifdef(
  CONFIG_NET_L2_WIFI_SHELL ${ZEPHYR_BASE}/subsys/net/lib/shell
  )
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_MGMT wifi_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_SHELL wifi_shell.c)
zephyr_library_sources_ifdef(CONFIG_WIFI_NM wifi_nm.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_UTILS wifi_utils.c)

# Linker section placement for wifi_nm_instance iterable structure
zephyr_linker_sources_ifdef(CONFIG_WIFI_NM DATA_SECTIONS wifi_nm.ld)
if (CONFIG_WIFI_NM)
zephyr_iterable_section(NAME wifi_nm_instance GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN CONFIG_LINKER_ITERABLE_SUBALIGN)
endif()
