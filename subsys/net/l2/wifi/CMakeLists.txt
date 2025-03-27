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
zephyr_iterable_section(NAME wifi_nm_instance GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})
endif()

if(CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE AND CONFIG_NET_L2_WIFI_SHELL)
  # Wi-Fi Enterprise test certificates handling
  set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated)
  set(gen_dir ${gen_inc_dir}/wifi_enterprise_test_certs)
  if(NOT DEFINED WIFI_TEST_CERTS_DIR)
    set(WIFI_TEST_CERTS_DIR ${ZEPHYR_BASE}/samples/net/wifi/test_certs/rsa3k)
  endif()
  # Create output directory for test certs
  file(MAKE_DIRECTORY ${gen_dir})

  # convert .pem files to array data at build time
  zephyr_include_directories(${gen_inc_dir})

  foreach(cert_file IN ITEMS
      ${WIFI_TEST_CERTS_DIR}/client.pem
      ${WIFI_TEST_CERTS_DIR}/client-key.pem
      ${WIFI_TEST_CERTS_DIR}/ca.pem
      ${WIFI_TEST_CERTS_DIR}/client2.pem
      ${WIFI_TEST_CERTS_DIR}/client-key2.pem
      ${WIFI_TEST_CERTS_DIR}/ca2.pem
      ${WIFI_TEST_CERTS_DIR}/server.pem
      ${WIFI_TEST_CERTS_DIR}/server-key.pem
      )
    if(EXISTS ${cert_file})
      get_filename_component(cert_name ${cert_file} NAME)
      generate_inc_file_for_target(
          app
          ${cert_file}
          ${gen_dir}/${cert_name}.inc
          )
    else()
      get_filename_component(cert_name ${cert_file} NAME)
      file(WRITE ${gen_dir}/${cert_name}.inc "// Empty file generated because ${cert_file} does not exist\n")
    endif()
  endforeach()

  # Add explicit dependency on app target for ZEPHYR_CURRENT_LIBRARY, so these
  # headers are generated at the correct point in the build
  add_dependencies(${ZEPHYR_CURRENT_LIBRARY} app)
endif() #CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE AND CONFIG_NET_L2_WIFI_SHELL
