# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_conformance)

target_sources(app PRIVATE src/main.c)

# COAP_RESOURCE_DEFINE puts the resources in an iterable section named after
# the service, which the application has to make room for.
zephyr_linker_sources(DATA_SECTIONS sections-ram.ld)
zephyr_iterable_section(
  NAME coap_resource_conformance_server
  GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})
