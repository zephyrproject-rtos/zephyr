# SPDX-License-Identifier: Apache-2.0

add_library(XEN_LIBFDT INTERFACE)

target_include_directories(XEN_LIBFDT INTERFACE
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt)

zephyr_library()
zephyr_library_sources(
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_addresses.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_check.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_empty_tree.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_overlay.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_ro.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_rw.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_strerror.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_sw.c
  ${ZEPHYR_XENLIB_MODULE_DIR}/libfdt/fdt_wip.c)
zephyr_library_link_libraries(XEN_LIBFDT)
zephyr_include_directories(${ZEPHYR_XENLIB_MODULE_DIR}/libfdt)
