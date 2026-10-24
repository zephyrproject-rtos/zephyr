# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(logging_flash_backend)

# The application is the logging sample, unchanged: what this sample adds is a
# flash log backend under it and the shell to read the log back. Building its
# sources from here rather than copying them keeps the two from drifting.
set(logger_src ${ZEPHYR_BASE}/samples/subsys/logging/logger/src)

target_sources(app PRIVATE
  ${logger_src}/ext_log_system.c
  ${logger_src}/ext_log_system_adapter.c
  ${logger_src}/main.c
  ${logger_src}/sample_instance.c
  ${logger_src}/sample_module.c
)
target_include_directories(app PRIVATE ${logger_src})
