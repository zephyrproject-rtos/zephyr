# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(log_core_additional)

if(${USERSPACE_TEST})
target_sources(app PRIVATE src/log_test_user.c)
else()
target_sources(app PRIVATE src/log_test.c)
endif()
