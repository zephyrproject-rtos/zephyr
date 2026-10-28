# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dma_chan_blen_transfer)

target_sources(app PRIVATE src/test_chan_blen_transfer.c ../common/dma_test.c)
target_include_directories(app PRIVATE ../common)
