# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(w6300_http_server)

target_sources(app PRIVATE src/main.c src/http_server.c)

# Add driver directory to include path
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/ethernet)