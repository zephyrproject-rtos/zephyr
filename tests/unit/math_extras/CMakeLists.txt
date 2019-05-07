# SPDX-License-Identifier: Apache-2.0

project(math_extras)
set(SOURCES main.c portable.c)
include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
