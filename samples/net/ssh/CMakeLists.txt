# Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ssh)

target_sources(app PRIVATE src/main.c)

zephyr_include_directories(.)

if(CONFIG_NET_SAMPLE_SSH_USE_PASSWORD)
  message(STATUS "------------------------------------------------------")
  message(STATUS "SSH server password: ${CONFIG_NET_SAMPLE_SSH_PASSWORD}")
  message(STATUS "------------------------------------------------------")
endif()
