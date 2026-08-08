# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esmf_traffic_light)

target_sources(app PRIVATE src/main.c src/traffic_shell.c src/traffic_controller.c)
