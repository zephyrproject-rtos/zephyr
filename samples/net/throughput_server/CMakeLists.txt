cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(throughput_server)

target_sources(                     app PRIVATE src/server.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/udp.c)

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)
