# Copyright 2022 Google LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(emul)

target_sources(app PRIVATE src/main.c)
