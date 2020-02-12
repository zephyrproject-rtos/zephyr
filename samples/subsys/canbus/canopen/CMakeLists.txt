# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(CANopen)

target_sources(app PRIVATE src/main.c objdict/CO_OD.c)
zephyr_include_directories(objdict)
