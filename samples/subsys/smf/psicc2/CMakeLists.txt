# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smf_psicc2)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/psicc2_console_cmds.c)
target_sources(app PRIVATE src/psicc2_thread.c)
