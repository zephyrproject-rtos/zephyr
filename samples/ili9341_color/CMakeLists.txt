# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ili9341_color)

FILE(GLOB_RECURSE app_sources src/main.c src/generated/*.c src/custom/*.c)
target_include_directories(app PRIVATE src/generated src/custom src/generated/guider_customer_fonts src/generated/guider_fonts src/generated/images)
target_sources(app PRIVATE ${app_sources})
target_compile_definitions(app PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
