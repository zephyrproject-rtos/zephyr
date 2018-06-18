# base64 support is need from mbedtls
target_include_directories(subsys_net PUBLIC ${CMAKE_CURRENT_LIST_DIR}
                           $ENV{ZEPHYR_BASE}/ext/lib/crypto/mbedtls/include/)

target_sources(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR}/websocket.c)
