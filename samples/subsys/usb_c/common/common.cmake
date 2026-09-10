# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

# Common USB-C sample code
# Include this file from sample CMakeLists.txt to add shared power control driver

target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/power_ctrl.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR})
