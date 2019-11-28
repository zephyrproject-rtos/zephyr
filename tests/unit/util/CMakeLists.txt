# SPDX-License-Identifier: Apache-2.0

project(util)
set(SOURCES main.c ../../../lib/os/dec.c)
include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
