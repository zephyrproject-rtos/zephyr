# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_ots)

target_sources(app PRIVATE src/main.c)

# The listing content is read through the server internals, as there is no connection
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth/services/ots)
