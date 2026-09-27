# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mem_write_fault)

# the test provides z_mem_write_fault() and calls the kernel-arch interface
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/kernel/include ${ARCH_DIR}/${ARCH}/include)

target_sources(app PRIVATE src/main.c)
