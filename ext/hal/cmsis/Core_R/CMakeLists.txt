# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(Include)

# As of CMSIS v5.6.0, __PROGRAM_START is to indicate whether the
# ARM vendor or the OS supplies data/bss init routine, otherwise
# the default data/bss init routine for the selected toolchain is
# added. We set the macro in build-time to guarantee compatibility
# with all existing ARM platforms.

zephyr_compile_definitions(__PROGRAM_START)
