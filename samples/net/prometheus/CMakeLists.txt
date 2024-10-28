# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Python REQUIRED COMPONENTS Interpreter)

project(prometheus_sample)

if(CONFIG_NET_SOCKETS_SOCKOPT_TLS AND
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


target_sources(app PRIVATE src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

target_link_libraries(app PRIVATE zephyr_interface zephyr)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTPS_SERVICE NAME
				http_resource_desc_test_https_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN Z_LINK_ITERABLE_SUBALIGN)
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_HTTP_SERVICE NAME
				http_resource_desc_test_http_service
				KVMA RAM_REGION GROUP RODATA_REGION
				SUBALIGN Z_LINK_ITERABLE_SUBALIGN)

foreach(inc_file
	ca.der
	server.der
	server_privkey.der
	https-server-cert.der
	https-server-key.der
    )
  generate_inc_file_for_target(
    app
    src/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()
