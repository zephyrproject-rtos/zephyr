# SPDX-License-Identifier: Apache-2.0
project(winstream)
cmake_minimum_required(VERSION 3.20.0)
set(SOURCES main.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
