# Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ssh_pkt_test)

target_sources(app PRIVATE src/main.c)

set(SSH_SOURCE_DIR ../../../../../subsys/net/lib/ssh)

target_sources(app PRIVATE ${SSH_SOURCE_DIR}/ssh_pkt.c)

# Allow access to private ssh headers
zephyr_include_directories(${SSH_SOURCE_DIR})

# We add ssh_pkt.c from the ssh subsystem, which uses logging but this test
# does not activate any logging so avoid compile error by setting log level to 0 (none).
add_compile_definitions(CONFIG_SSH_LOG_LEVEL=0)
