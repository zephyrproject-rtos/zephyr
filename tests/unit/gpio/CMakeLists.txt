# SPDX-License-Identifier: Apache-2.0

project(gpio)
set(SOURCES main.c)
list(APPEND INCLUDE tests/unit/gpio drivers/gpio)
include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
