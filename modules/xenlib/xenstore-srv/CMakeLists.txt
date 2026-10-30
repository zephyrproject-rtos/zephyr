# SPDX-License-Identifier: Apache-2.0

add_library(XENSTORE_SRV INTERFACE)

target_include_directories(XENSTORE_SRV INTERFACE
  ${ZEPHYR_XENLIB_MODULE_DIR}/xenstore-srv/include)

zephyr_library()
zephyr_library_sources(
  ${ZEPHYR_XENLIB_MODULE_DIR}/xenstore-srv/src/xenstore_srv.c)
zephyr_library_link_libraries(XENSTORE_SRV XEN_STORE_COMMON)
zephyr_include_directories(${ZEPHYR_XENLIB_MODULE_DIR}/xenstore-srv/include)
