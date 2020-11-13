# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.7)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(wsen_itds)

target_sources(app PRIVATE src/main.c)
