# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_client)

if(CONFIG_COAP_OSCORE)
  target_sources(app PRIVATE src/coap-client-oscore.c)
else()
  target_sources(app PRIVATE src/coap-client.c)
endif()
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
