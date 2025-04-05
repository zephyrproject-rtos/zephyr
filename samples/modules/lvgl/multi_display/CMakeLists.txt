# Copyright (c) 2025 Abderrahmane JARMOUNI
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lvgl_multi_display)

set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
    ${app_sources}
    ${LVGL_DIR}/demos/widgets/assets/img_clothes.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_avatar.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_needle.c
    ${LVGL_DIR}/demos/widgets/assets/img_lvgl_logo.c
    ${LVGL_DIR}/demos/widgets/lv_demo_widgets.c

    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_main.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_view.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_flex_loader.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_view_child_node.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_view_ctrl_pad.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_ctrl_pad.c
)

target_include_directories(app PRIVATE
    ${LVGL_DIR}/demos/
)
