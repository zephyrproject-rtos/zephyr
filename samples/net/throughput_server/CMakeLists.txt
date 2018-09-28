cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(                     app PRIVATE src/server.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/udp.c)

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)
