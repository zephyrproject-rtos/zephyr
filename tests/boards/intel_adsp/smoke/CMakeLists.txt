# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(intel_adsp)

target_sources(app PRIVATE src/main.c src/smpboot.c src/hostipc.c src/cpus.c src/ipm.c)
