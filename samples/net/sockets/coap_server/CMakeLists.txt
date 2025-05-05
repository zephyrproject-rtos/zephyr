# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_server)

if(CONFIG_NET_SOCKETS_ENABLE_DTLS AND
   CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED AND
   (CONFIG_NET_SAMPLE_PSK_HEADER_FILE STREQUAL "dummy_psk.h"))
  add_custom_target(development_psk
    COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
    COMMAND ${CMAKE_COMMAND} -E echo "--- WARNING: Using dummy PSK! Only suitable for        ---"
    COMMAND ${CMAKE_COMMAND} -E echo "--- development. Set NET_SAMPLE_PSK_HEADER_FILE to use ---"
    COMMAND ${CMAKE_COMMAND} -E echo "--- own pre-shared key.                                ---"
    COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
  )
  add_dependencies(app development_psk)
endif()

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)

# Support LD linker template
zephyr_linker_sources(DATA_SECTIONS sections-ram.ld)

# Support CMake linker generator
zephyr_iterable_section(
  NAME coap_resource_coap_server
  GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT}
  SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(inc_file
	ca.der
	server.der
	server_privkey.der
	coaps-server-cert.der
	coaps-server-key.der
)
  generate_inc_file_for_target(
    app
    src/certs/${inc_file}
    ${gen_dir}/${inc_file}.inc
  )
endforeach()
