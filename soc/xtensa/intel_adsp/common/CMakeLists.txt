# Intel CAVS SoC family CMake file
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(INTEL_ADSP_COMMON)

zephyr_library_named(intel_adsp_common)
zephyr_library_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_library_sources(soc.c)
zephyr_library_sources(soc_mp.c)
zephyr_library_sources(trace_out.c)
zephyr_library_sources(rimage_modules.c)
zephyr_library_sources(boot.c)

zephyr_library_link_libraries(INTEL_ADSP_COMMON)

target_include_directories(INTEL_ADSP_COMMON INTERFACE include)
target_link_libraries(INTEL_ADSP_COMMON INTERFACE intel_adsp_common)

set(ELF_FIX ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/common/fix_elf_addrs.py)
set(KERNEL_REMAPPED ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}-remapped.elf)

# Generate rimage modules from the base kernel ELF file.  Note the
# warning squashing on the objcopy steps.  Binutils has a misfeature
# where if the copy removes all the sections from an input ELF program
# header (our link generates lots of phdrs because of the disjoint
# cachability addresses), it will warn about an "empty" segment even
# though it was TOLD to drop the contents!
#
# Also note that rimage is picky with section flags: it will try to
# include a section in the output data (even its own metadata in
# .module!) if it has any of ALLOC, WRITABLE or EXEC flags in the ELF
# file.  The GNU linker will set these automatically based on the
# flags of the sections coming out of C code, so this is fragile and
# breaks easily.  Set noload flags explicitly here.
add_custom_target(
  gen_modules ALL
  DEPENDS ${ZEPHYR_FINAL_EXECUTABLE}

  # Remap uncached section addresses so they appear contiguous
  COMMAND ${CMAKE_COMMAND} -E
      copy ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}.elf ${KERNEL_REMAPPED}

  COMMAND ${ELF_FIX} ${CMAKE_OBJCOPY} ${KERNEL_REMAPPED}

  # Extract modules for rimage
  COMMAND ${CMAKE_OBJCOPY}
      --only-section .imr*
      --only-section .module.boot
      --set-section-flags .module.boot=noload,readonly
      --rename-section .module.boot=.module
      ${KERNEL_REMAPPED} ${CMAKE_BINARY_DIR}/zephyr/boot.mod 2>/dev/null

  COMMAND ${CMAKE_OBJCOPY}
      --remove-section .imr*
      --remove-section .module.boot
      --set-section-flags .module.main=noload,readonly
      --set-section-flags .static_uuid_entries=noload,readonly
      --set-section-flags .static_log_entries=noload,readonly
      --set-section-flags .fw_metadata=noload,readonly
      --rename-section .module.main=.module
      ${KERNEL_REMAPPED} ${CMAKE_BINARY_DIR}/zephyr/main.mod 2>/dev/null
)
