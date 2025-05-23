#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(wifi_credentials)
zephyr_library_sources(wifi_credentials.c)

if (CONFIG_WIFI_CREDENTIALS_BACKEND_PSA)
zephyr_library_include_directories(
	$<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
)
endif()

zephyr_library_sources_ifdef(
	CONFIG_WIFI_CREDENTIALS_BACKEND_SETTINGS
	wifi_credentials_backend_settings.c)

zephyr_library_sources_ifdef(
	CONFIG_WIFI_CREDENTIALS_BACKEND_PSA
	wifi_credentials_backend_psa.c)

zephyr_library_sources_ifdef(
	CONFIG_WIFI_CREDENTIALS_BACKEND_NONE
	wifi_credentials_backend_none.c)

zephyr_library_sources_ifdef(
	CONFIG_WIFI_CREDENTIALS_SHELL
	wifi_credentials_shell.c)

if(WIFI_CREDENTIALS_STATIC_SSID)
	message(WARNING
		"Static Wi-Fi configuration is used, please remove before deployment!"
	)
endif()

if(DEFINED CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE AND NOT DEFINED CONFIG_NET_L2_WIFI_SHELL)
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
endif()
