# SPDX-License-Identifier: Apache-2.0

project(list)
set(SOURCES
  main.c
  slist.c
  dlist.c
  sflist.c
  )
include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
