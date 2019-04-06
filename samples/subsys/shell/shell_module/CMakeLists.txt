# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(shell_module)

target_sources(app PRIVATE src/main.c src/test_module.c)
target_sources_ifdef(CONFIG_SHELL_DYNAMIC_CMDS app PRIVATE src/dynamic_cmd.c src/qsort.c)
