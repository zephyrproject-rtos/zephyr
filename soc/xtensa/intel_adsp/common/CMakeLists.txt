# Intel CAVS SoC family CMake file
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(INTEL_ADSP_COMMON)

zephyr_library_named(intel_adsp_common)
zephyr_library_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

set_source_files_properties(adsp.c PROPERTIES COMPILE_FLAGS -std=gnu99)
zephyr_library_sources(adsp.c)

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

# Generate rimage modules from the base kernel ELF file
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
      --rename-section .module.boot=.module
      ${KERNEL_REMAPPED} ${CMAKE_BINARY_DIR}/zephyr/boot.mod

  COMMAND ${CMAKE_OBJCOPY}
      --remove-section .imr*
      --remove-section .module.boot
      --rename-section .module.main=.module
      ${KERNEL_REMAPPED} ${CMAKE_BINARY_DIR}/zephyr/main.mod
)
