# SPDX-License-Identifier: Apache-2.0

project(base64)
set(SOURCES
  main.c
  )
include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
