# base64 support is need from mbedtls
zephyr_include_directories(. $ENV{ZEPHYR_BASE}/ext/lib/crypto/mbedtls/include/)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_WEBSOCKET websocket.c)
