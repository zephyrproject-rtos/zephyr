# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dma_scatter_gather)

target_sources(app PRIVATE src/test_scatter_gather.c ../common/dma_test.c)
target_include_directories(app PRIVATE ../common)
