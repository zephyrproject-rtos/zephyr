# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(x86_mmu_api)

file(GLOB source_for_app src/*.c)

target_sources(app PRIVATE ${source_for_app})
