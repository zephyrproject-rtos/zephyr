# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(log_core_additional)

if(CONFIG_TEST_USERSPACE)
target_sources(app PRIVATE src/log_test_user.c)
else()
target_sources(app PRIVATE src/log_test.c)
endif()
