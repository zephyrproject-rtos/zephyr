# Copyright 2021 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Add the Zephyr package and create the project
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mock_fff)

# Glob the sources and add them to the app
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
