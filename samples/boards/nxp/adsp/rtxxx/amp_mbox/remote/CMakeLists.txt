# SPDX-License-Identifier: Apache-2.0

set(BOARD mimxrt685_evk/mimxrt685s/hifi4)

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(remote)

target_sources(app PRIVATE src/main.c)

include("${CMAKE_CURRENT_LIST_DIR}/../../common/remote-adsp-imgs.cmake")
