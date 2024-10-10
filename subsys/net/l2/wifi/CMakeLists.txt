# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories_ifdef(
  CONFIG_NET_L2_WIFI_SHELL ${ZEPHYR_BASE}/subsys/net/lib/shell
  )
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )
zephyr_library_include_directories_ifdef(
  CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE test_certs
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

# Wi-Fi Enterprise test certificates handling
set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated)
set(gen_dir ${gen_inc_dir}/wifi_enterprise_test_certs)

# convert .pem files to array data at build time
zephyr_include_directories(${gen_inc_dir})

generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/wifi/test_certs/client.pem
    ${gen_dir}/client.pem.inc
    )

generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/wifi/test_certs/client-key.pem
    ${gen_dir}/client-key.pem.inc
    )

generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/wifi/test_certs/ca.pem
    ${gen_dir}/ca.pem.inc
    )