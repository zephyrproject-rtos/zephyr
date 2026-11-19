# Copyright (c) 2026 Priyanshu Singh <singhpriyanshu9838@gmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sysworkq_stack_aggregation)

if(DEFINED EXPECT_USER_CONFIGURED)
  get_property(is_user_conf GLOBAL PROPERTY SYSTEM_WORKQUEUE_USER_CONFIGURED)
  if(EXPECT_USER_CONFIGURED AND NOT is_user_conf)
    message(FATAL_ERROR "Expected SYSTEM_WORKQUEUE_USER_CONFIGURED to be TRUE, but was FALSE")
  elseif(NOT EXPECT_USER_CONFIGURED AND is_user_conf)
    message(FATAL_ERROR "Expected SYSTEM_WORKQUEUE_USER_CONFIGURED to be FALSE, but was TRUE")
  endif()
endif()

target_sources(app PRIVATE src/main.c)
