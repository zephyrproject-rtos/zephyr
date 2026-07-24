# SPDX-License-Identifier: Apache-2.0

# For the smp_large variant the Zephyr image lives at 0x100000000 but QEMU
# virt's reset stub always jumps to VIRT_DRAM (0x80000000).  Add a small
# trampoline placed at 0x80000000 that performs an absolute jump to the
# real Zephyr entry point.
if("${BOARD_QUALIFIERS}" MATCHES "smp_large")
  zephyr_library()
  zephyr_library_sources(trampoline_large.S)
  zephyr_linker_sources(SECTIONS trampoline_large.ld)
endif()
