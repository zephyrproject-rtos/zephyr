# SPDX-License-Identifier: Apache-2.0

project(timeutil)
set(SOURCES main.c test_gmtime.c  test_s32.c test_s64.c)
include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
