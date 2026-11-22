# Copyright (c) 2026 StructCore Embedded
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(snmp_ztests)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/snmp)

target_sources(app PRIVATE
    src/main.c
    src/test_ber.c
    src/test_oid.c
    src/test_mib.c
    src/test_mib2.c
    src/test_multi_instance.c
    src/test_set_2pc.c
    src/test_vacm.c
    src/test_usm.c
    src/test_trap.c
)
