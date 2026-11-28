# Copyright (c) 2026 Priyanshu Singh <singhpriyanshu9838@gmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sysworkq_stack_aggregation)

if(DEFINED EXPECT_USER_VALUE_RAISED)
  get_property(is_user_val_raised GLOBAL PROPERTY SYSTEM_WORKQUEUE_STACK_SIZE_USER_VALUE_RAISED)
  if(EXPECT_USER_VALUE_RAISED AND NOT is_user_val_raised)
    message(FATAL_ERROR "Expected SYSTEM_WORKQUEUE_STACK_SIZE_USER_VALUE_RAISED to be TRUE, but was FALSE")
  elseif(NOT EXPECT_USER_VALUE_RAISED AND is_user_val_raised)
    message(FATAL_ERROR "Expected SYSTEM_WORKQUEUE_STACK_SIZE_USER_VALUE_RAISED to be FALSE, but was TRUE")
  endif()
endif()

target_sources(app PRIVATE src/main.c)
