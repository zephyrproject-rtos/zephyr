# SPDX-License-Identifier: Apache-2.0

# FIXME: Temporary workaround for the alignment-related test failure reported
#        in the GitHub issue zephyrproject-rtos/zephyr#48992.
zephyr_cc_option(-mfaster-structs)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-sparc")
add_subdirectory(core)
