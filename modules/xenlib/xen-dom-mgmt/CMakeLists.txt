# SPDX-License-Identifier: Apache-2.0

add_library(XENDOM_MGMT INTERFACE)

target_include_directories(XENDOM_MGMT INTERFACE
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/include)

zephyr_library()
zephyr_library_sources(
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/src/xen-dom-mgmt.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/src/xen-dom-fdt.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/src/mem-mgmt.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/src/xl_parser.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/src/xen-dom-xs.c)
if(CONFIG_XEN_DOMCFG_SECTION)
    zephyr_linker_sources(DATA_SECTIONS
      ${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/linker.ld)
endif()
zephyr_library_link_libraries(XENDOM_MGMT)
zephyr_include_directories(${ZEPHYR_XENLIB_MODULE_DIR}/xen-dom-mgmt/include)
