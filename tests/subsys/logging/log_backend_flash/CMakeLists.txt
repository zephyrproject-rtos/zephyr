# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(log_backend_flash)

# The backend is built as part of Zephyr, not of the application, so the header
# holding the build id has to be on Zephyr's include path.
zephyr_include_directories(src)

target_sources(app PRIVATE src/main.c)
