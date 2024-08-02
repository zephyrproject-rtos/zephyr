# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_client_test)

set(APP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add test sources
target_sources(app PRIVATE ${APP_SRC_DIR}/main.c)
target_sources(app PRIVATE ${APP_SRC_DIR}/stubs.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/coap/coap_client.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/coap/coap.c)

# Add includes directories
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/)

target_compile_definitions(app PRIVATE _POSIX_C_SOURCE=200809L)

add_compile_definitions(CONFIG_NET_SOCKETS_POLL_MAX=3)
add_compile_definitions(CONFIG_COAP_CLIENT=y)
add_compile_definitions(CONFIG_COAP_CLIENT_BLOCK_SIZE=256)
add_compile_definitions(CONFIG_COAP_CLIENT_MESSAGE_SIZE=256)
add_compile_definitions(CONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE=48)
add_compile_definitions(CONFIG_COAP_CLIENT_STACK_SIZE=1024)
add_compile_definitions(CONFIG_COAP_CLIENT_THREAD_PRIORITY=10)
add_compile_definitions(CONFIG_COAP_LOG_LEVEL=4)
add_compile_definitions(CONFIG_COAP_INIT_ACK_TIMEOUT_MS=200)
add_compile_definitions(CONFIG_COAP_CLIENT_MAX_REQUESTS=2)
add_compile_definitions(CONFIG_COAP_CLIENT_MAX_INSTANCES=2)
add_compile_definitions(CONFIG_COAP_MAX_RETRANSMIT=4)
add_compile_definitions(CONFIG_COAP_BACKOFF_PERCENT=200)
