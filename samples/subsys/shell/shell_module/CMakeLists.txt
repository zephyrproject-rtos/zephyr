# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(shell_module)

target_sources(app PRIVATE src/main.c src/test_module.c)
target_sources_ifdef(CONFIG_SHELL_DYNAMIC_CMDS app PRIVATE src/dynamic_cmd.c src/qsort.c)
target_sources_ifdef(CONFIG_SHELL_BACKEND_SERIAL app PRIVATE src/uart_reinit.c)
