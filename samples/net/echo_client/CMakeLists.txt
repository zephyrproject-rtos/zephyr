include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(                     app PRIVATE src/echo-client.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/udp.c)
target_sources_ifdef(CONFIG_NET_TCP app PRIVATE src/tcp.c)

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)

target_link_libraries_ifdef(CONFIG_MBEDTLS app mbedTLS)
