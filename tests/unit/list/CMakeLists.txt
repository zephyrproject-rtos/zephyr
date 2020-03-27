# SPDX-License-Identifier: Apache-2.0

project(list)
set(SOURCES
  main.c
  slist.c
  dlist.c
  sflist.c
  )
find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
