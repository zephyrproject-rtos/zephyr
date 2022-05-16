# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(usbc_sink)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/usbc_snk.c)
target_sources(app PRIVATE src/stm32g081b_eval_board.c)
