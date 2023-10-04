# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets_echo_client_self_test)

if(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED AND
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

set(echo_client_path ${ZEPHYR_BASE}/samples/net/sockets/echo_client)

target_include_directories(app PRIVATE ${echo_client_path}/src/)

target_sources(app PRIVATE
	src/echo_test.c
	src/test_main.c
)

target_sources(                     app PRIVATE ${echo_client_path}/src/echo-client.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE ${echo_client_path}/src/udp.c)
target_sources_ifdef(CONFIG_NET_TCP app PRIVATE ${echo_client_path}/src/tcp.c)
target_sources_ifdef(CONFIG_NET_VLAN app PRIVATE ${echo_client_path}/src/vlan.c)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    ${echo_client_path}/src/echo-apps-cert.der
    ${gen_dir}/echo-apps-cert.der.inc
    )

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
  )
