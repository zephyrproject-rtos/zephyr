# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(factorydata)

target_sources(app PRIVATE src/main.c src/factory_data_parser.c)
