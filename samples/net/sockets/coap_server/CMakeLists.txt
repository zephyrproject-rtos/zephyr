# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_server)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)

# Support LD linker template
zephyr_linker_sources(DATA_SECTIONS sections-ram.ld)

# Support CMake linker generator
zephyr_iterable_section(
  NAME coap_resource_coap_server
  GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT}
  SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
