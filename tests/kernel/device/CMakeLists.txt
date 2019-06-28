# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(device)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# device_get_binding() compares pointers first before doing strcmp().
# However, enabling coverage forces -O0 to disable any compiler
# optimizations. There would be multiple copies of the same string,
# and the code pathing doing pointer comparison would not be tested
# at all. So add this flag to merge string constants such that
# the pointer comparison would be exercised.
zephyr_cc_option_ifdef(CONFIG_COVERAGE -fmerge-constants)
