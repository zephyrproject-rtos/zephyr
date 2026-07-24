# SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated)

zephyr_library_sources(
  config.c
  dhcp.c
  bmc_init.c
  net.c
  power.c
  synch.c
  vpd.c
)

zephyr_library_sources_ifdef(CONFIG_RTC
  rtc.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_APP_SENSORS
  sensors.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_APP_NTP
  ntp.c
)

zephyr_library_sources_ifdef(CONFIG_HTTP_SERVER
  http.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_APP_WEB
  web.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_JTAG
  jtag.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_APP_CONSOLE_LOGGER
  console_logger.c
)

zephyr_library_sources_ifdef(CONFIG_CONSOLE_BRIDGE
  console_bridge.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_APP_WEB_TERMINAL_HOST_SERIAL
  console_bridge_ws.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_PERSISTENT_STORAGE
  button.c
  fs.c
)

zephyr_library_sources_ifdef(CONFIG_REDFISH
  redfish.c
)

if(CONFIG_REDFISH)
  foreach(redfish_resource
      redfish_metadata.xml
    )

    generate_inc_file_for_target(
      app
      static_web_resources/${redfish_resource}
      ${gen_dir}/${redfish_resource}.gz.inc
      --gzip
    )
  endforeach()
endif()

if(CONFIG_BMC_APP_WEB)
  foreach(web_resource
      index.html
      logo.jpeg
      favicon.png
    )

    generate_inc_file_for_target(
      app
      static_web_resources/${web_resource}
      ${gen_dir}/${web_resource}.gz.inc
      --gzip
    )
  endforeach()
endif()

if(CONFIG_BMC_APP_CRYPTO)
  find_program(OPENSSL_EXECUTABLE openssl)
  if(NOT OPENSSL_EXECUTABLE)
    message(FATAL_ERROR "CONFIG_BMC_APP_CRYPTO requires openssl in PATH")
  endif()
  find_program(BASH_EXECUTABLE bash)
  if(NOT BASH_EXECUTABLE)
    message(FATAL_ERROR "Unable to find bash, needed to generate certificates")
  endif()

  set(generated_certs_dir ${CMAKE_CURRENT_BINARY_DIR}/generated_certs)
  set(generated_server_cert_der ${generated_certs_dir}/server_cert.der)
  set(generated_server_privkey_der ${generated_certs_dir}/server_privkey.der)

  add_custom_command(
    OUTPUT
      ${generated_server_cert_der}
      ${generated_server_privkey_der}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_certs_dir}
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_ca_cert.sh
            ${OPENSSL_EXECUTABLE}
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_server_cert.sh
            ${OPENSSL_EXECUTABLE}
    WORKING_DIRECTORY ${generated_certs_dir}
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_ca_cert.sh
      ${CMAKE_CURRENT_SOURCE_DIR}/certs/gen_server_cert.sh
    BYPRODUCTS
      ${generated_certs_dir}/ca_cert.pem
      ${generated_certs_dir}/ca_privkey.pem
      ${generated_certs_dir}/ca.srl
      ${generated_certs_dir}/server_cert.pem
      ${generated_certs_dir}/server_privkey.pem
      ${generated_certs_dir}/server_csr.pem
      ${generated_certs_dir}/server_csr.ext
    COMMENT "Generating TLS certificates for BMC"
    VERBATIM
  )

  add_custom_target(app_generate_certs
    DEPENDS
    ${generated_server_cert_der}
    ${generated_server_privkey_der}
  )
  add_dependencies(app app_generate_certs)

  foreach(inc_file
      server_cert.der
      server_privkey.der
    )
    generate_inc_file_for_target(
      app
      ${generated_certs_dir}/${inc_file}
      ${gen_dir}/${inc_file}.inc
    )
  endforeach()
endif()

if(CONFIG_BMC_PERSISTENT_STORAGE)
  if(CONFIG_CODE_DATA_RELOCATION_SRAM AND CONFIG_SOC_FLASH_STM32)
    include(${ZEPHYR_BASE}/cmake/modules/extensions.cmake)
    file(GLOB flash_driver ${ZEPHYR_BASE}/drivers/flash/flash_stm32*.c)
    zephyr_code_relocate(FILES ${flash_driver} LOCATION RAM)
  else()
    message(FATAL_ERROR "flash storage handling is not defined for this platform")
  endif()
endif()

zephyr_linker_sources(SECTIONS ${CMAKE_CURRENT_SOURCE_DIR}/sections-rom.ld)
zephyr_linker_section_ifdef(CONFIG_HTTP_SERVER
  NAME http_resource_desc_https_service
  KVMA RAM_REGION GROUP RODATA_REGION
#  SUBALIGN Z_LINK_ITERABLE_SUBALIGN
)

zephyr_linker_section_ifdef(CONFIG_HTTP_SERVER
  NAME http_resource_desc_http_service
  KVMA RAM_REGION GROUP RODATA_REGION
#  SUBALIGN Z_LINK_ITERABLE_SUBALIGN
)

find_package(Git QUIET)
find_program(BASH_EXECUTABLE bash)

set(git_sha_header ${gen_dir}/bmc_git_sha.h)
add_custom_target(app_git_sha ALL
  COMMAND ${CMAKE_COMMAND}
    -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
    -DSOURCE_DIR=${ZEPHYR_BASE}
    -DOUTPUT_FILE=${git_sha_header}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_git_sha.cmake
  BYPRODUCTS ${git_sha_header}
  COMMENT "Generating BMC git SHA header"
)
add_dependencies(app app_git_sha)
