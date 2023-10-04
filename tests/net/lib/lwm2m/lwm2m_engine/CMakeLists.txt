# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_engine_test)

set(APP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add test sources
target_sources(app PRIVATE ${APP_SRC_DIR}/main.c)
target_sources(app PRIVATE ${APP_SRC_DIR}/stubs.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/lwm2m/lwm2m_engine.c)

# Add includes directories
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/include/)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/lwm2m/)

add_compile_definitions(CONFIG_LWM2M_ENGINE_MAX_PENDING=2)
add_compile_definitions(CONFIG_LWM2M_ENGINE_MAX_REPLIES=2)
add_compile_definitions(CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE=512)
add_compile_definitions(CONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE=512)
add_compile_definitions(CONFIG_LWM2M_ENGINE_MAX_OBSERVER=10)
add_compile_definitions(CONFIG_LWM2M_ENGINE_STACK_SIZE=2048)
add_compile_definitions(CONFIG_LWM2M_NUM_BLOCK1_CONTEXT=3)
add_compile_definitions(CONFIG_LWM2M_COAP_BLOCK_SIZE=256)
add_compile_definitions(CONFIG_LWM2M_COAP_MAX_MSG_SIZE=512)
add_compile_definitions(CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME=60)
add_compile_definitions(CONFIG_LWM2M_SECURITY_INSTANCE_COUNT=1)
add_compile_definitions(CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY=30)
add_compile_definitions(CONFIG_LWM2M_QUEUE_MODE_UPTIME=30)
add_compile_definitions(CONFIG_LWM2M_LOG_LEVEL=4)
add_compile_definitions(CONFIG_NET_SOCKETS_POLL_MAX=3)
add_compile_definitions(CONFIG_LWM2M_DTLS_SUPPORT)
add_compile_definitions(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
add_compile_definitions(CONFIG_TLS_CREDENTIALS)
add_compile_definitions(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
