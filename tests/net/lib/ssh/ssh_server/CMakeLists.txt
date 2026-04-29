# Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ssh_server_test)

target_sources(app PRIVATE src/main.c)

zephyr_include_directories(.)
