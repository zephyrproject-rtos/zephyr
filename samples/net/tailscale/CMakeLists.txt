# SPDX-FileCopyrightText: FoBE Studio
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tailscale)

zephyr_include_directories(include)

set(TAILSCALE_CA_FILE "" CACHE FILEPATH "DER trust anchor for the Tailscale sample")
if(TAILSCALE_CA_FILE)
  if(NOT EXISTS "${TAILSCALE_CA_FILE}")
    message(FATAL_ERROR "TAILSCALE_CA_FILE must point to a DER certificate")
  endif()
  generate_inc_file_for_target(app "${TAILSCALE_CA_FILE}"
    "${ZEPHYR_BINARY_DIR}/include/generated/tailscale_ca.inc")
  target_compile_definitions(app PRIVATE TAILSCALE_SAMPLE_EMBEDDED_CA)
endif()
target_sources(app PRIVATE src/main.c)
