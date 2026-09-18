# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Fabien Proriol

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(knx_load_state_machine)

# Unlike l2_framing/l4_transitions, this suite does NOT #include its targets
# into src/main.c: it drives the Load/Run State Machines entirely through
# their public API (load_state_machine_event(), interface_write_property(),
# interface_read_property(), object_application_program_properties_written(),
# object_interface_program_properties_written()) and never needs a file's
# private statics. The five table-object files plus object_memory.c and
# object_interface.c are therefore ordinary compiled sources, giving genuine
# struct Memory offsets/capacities. CONFIG_KNX_STACK stays off (=n): only
# object_device.c (GPIO/hwinfo/input drivers) and knx_core.c (SYS_INIT
# starting L2 threads and resetting the NCN5130) are excluded — src/main.c
# stubs the two symbols object_interface.c needs from the Device Object.
target_sources(app PRIVATE
    src/main.c
    ${ZEPHYR_BASE}/subsys/knx/object_memory.c
    ${ZEPHYR_BASE}/subsys/knx/object_interface.c
    ${ZEPHYR_BASE}/subsys/knx/object_address_table.c
    ${ZEPHYR_BASE}/subsys/knx/object_association_table.c
    ${ZEPHYR_BASE}/subsys/knx/object_group_table.c
    ${ZEPHYR_BASE}/subsys/knx/object_application_program.c
    ${ZEPHYR_BASE}/subsys/knx/object_interface_program.c
)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/knx)

# Every object_*.c file above passes this to LOG_MODULE_REGISTER; the symbol
# comes from the KNX subsystem's Kconfig, which is not part of this build.
#
# The Address Table and Application Program segments are shrunk so the
# realistic-download and allocation-clamping tests can reason about full,
# easy-to-overflow segments without the production defaults (256 addresses /
# 1 byte of application data).
target_compile_definitions(app PRIVATE
    CONFIG_KNX_STACK_LOG_LEVEL=0
    CONFIG_KNX_MAX_ADDRESS_GROUP=8
    CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE=16
)
