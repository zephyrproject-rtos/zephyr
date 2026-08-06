# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(auxdisplay_digits)

target_sources(app PRIVATE src/main.c)
