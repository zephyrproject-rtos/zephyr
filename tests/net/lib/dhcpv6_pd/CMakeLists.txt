# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dhcpv6_pd)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/ip
  ${ZEPHYR_BASE}/subsys/net/lib/dhcpv6)
target_sources(app PRIVATE src/main.c)

# White-box test, see src/main.c. CONFIG_NET_DHCPV6 is injected here rather
# than enabled in prj.conf so that the implementation is not compiled into the
# networking library as well, which would collide at link time. The same is
# done in tests/net/dhcpv6 and tests/net/lib/dhcpv6_server.
zephyr_compile_definitions(CONFIG_NET_DHCPV6)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_LOG_LEVEL=LOG_LEVEL_ERR)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_MAX_DOWNSTREAM=4)
zephyr_compile_definitions(CONFIG_NET_DHCPV6_SEND_RELEASE_ON_STOP=1)
