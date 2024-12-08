# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/arch/xtensa/arch.h)
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-xtensa-le)
add_subdirectory(core)

if (CONFIG_XTENSA_INSECURE_USERSPACE)
  message(WARNING "
  This userspace implementation uses the window ABI this means that the kernel
  will spill registers on behalf of the userspace. Use it carefully.")
endif()
