# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(list)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
target_sources(testbinary
  PRIVATE
    main.c
    slist.c
    dlist.c
    sflist.c
)
