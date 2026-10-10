# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Fabien Proriol

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(knx_layer7_application)

# Unlike l2_framing/l4_transitions, src/main.c does NOT #include its
# targets: none of this suite's cases need layer7_application.c's or
# knx_group_object.c's private statics (both are driven entirely through
# their public API), and #including both into one translation unit doesn't
# work anyway — each carries its own LOG_MODULE_REGISTER, which can appear
# only once per translation unit. All five are therefore ordinary compiled
# sources. object_group_table.c is real (not stubbed) because
# object_group_table_properties_written() is part of what this suite must
# exercise for real, not through a stub.
# object_memory.c is real too, for genuine __memory offsets/bounds, same
# reasoning as the load_state_machine suite. dpt.c is real and
# dependency-free — knx_group_object.c's typed accessors call its
# encode/decode functions at link time.
target_sources(app PRIVATE
    src/main.c
    ${ZEPHYR_BASE}/subsys/knx/object_memory.c
    ${ZEPHYR_BASE}/subsys/knx/object_group_table.c
    ${ZEPHYR_BASE}/subsys/knx/dpt.c
    ${ZEPHYR_BASE}/subsys/knx/layer7_application.c
    ${ZEPHYR_BASE}/subsys/knx/knx_group_object.c
)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/knx)

# layer7_application.c / knx_group_object.c / object_group_table.c pass this
# to LOG_MODULE_REGISTER; the symbol comes from the KNX subsystem's Kconfig,
# which is not part of this build.
#
# A small, test-legible Group Object segment: enough ASAPs and data bytes to
# cover the I+C read-on-init sweep without the production defaults (1 object,
# 1 byte of group object data).
#
# CONFIG_KNX_DPT enables dpt.h's declarations (zephyr/include/zephyr/knx/
# dpt.h gates every dpt<N>_encode/decode() prototype behind #ifdef
# CONFIG_KNX_DPT) — knx_group_object.c's typed accessors need them, and
# CONFIG_KNX_STACK being off means the real Kconfig symbol isn't defined.
target_compile_definitions(app PRIVATE
    CONFIG_KNX_STACK_LOG_LEVEL=0
    CONFIG_KNX_GROUP_OBJECT_COUNT=4
    CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE=8
    CONFIG_KNX_DPT=1
)
