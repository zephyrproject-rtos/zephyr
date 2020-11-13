# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15.7)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(hello_world_user)

target_sources(app PRIVATE src/main.c)
