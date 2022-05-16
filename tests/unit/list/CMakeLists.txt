# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(list)
set(SOURCES
  main.c
  slist.c
  dlist.c
  sflist.c
  )
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
