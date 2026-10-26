# SPDX-License-Identifier: Apache-2.0

add_library(XEN_SHELL INTERFACE)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_XEN_DOMAIN_MANAGEMENT_SHELL_CMDS
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-shell-cmd/src/xen_cmds.c)
zephyr_library_sources_ifdef(CONFIG_XEN_STORE_SRV_SHELL_CMDS
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-shell-cmd/src/xenstore_cmds.c)
zephyr_library_sources_ifdef(CONFIG_XSTAT_SHELL_CMDS
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-shell-cmd/src/xstat_cmds.c)
zephyr_library_link_libraries(XEN_SHELL)
