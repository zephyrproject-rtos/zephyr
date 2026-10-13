# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lorawan_services_class_test)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# lorawan_services.h is internal to the LoRaWAN services
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/lorawan/services)
