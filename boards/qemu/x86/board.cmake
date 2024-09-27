# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corp.

set(SUPPORTED_EMU_PLATFORMS qemu)

if(NOT CONFIG_REBOOT)
  set(REBOOT_FLAG -no-reboot)
endif()

if(CONFIG_X86_64)
  set(QEMU_binary_suffix x86_64)
  set(QEMU_CPU_TYPE_${ARCH} qemu64,+x2apic)
  if("${CONFIG_MP_MAX_NUM_CPUS}" STREQUAL "1")
    # icount works with 1 CPU so we can enable it here.
    # FIXME: once this works across configs, remove this line and set
    # CONFIG_QEMU_ICOUNT_SHIFT in defconfig instead.
    list(APPEND QEMU_EXTRA_FLAGS -icount shift=5,align=off,sleep=off -rtc clock=vm)
  endif()
else()
  set(QEMU_CPU_TYPE_${ARCH} qemu32,+nx,+pae)
endif()

if(CONFIG_XIP)
  # Extra 4MB to emulate flash area
  math(EXPR QEMU_MEMORY_SIZE_MB "${CONFIG_SRAM_SIZE} / 1024 + 4")
elseif(CONFIG_BOARD_QEMU_X86_TINY AND CONFIG_DEMAND_PAGING
       AND NOT CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
  # Flash is at 4MB-8MB, so need this to be large enough
  math(EXPR QEMU_MEMORY_SIZE_MB "8")
else()
  math(EXPR QEMU_MEMORY_SIZE_MB "${CONFIG_SRAM_SIZE} / 1024")
endif()

set(QEMU_CPU_FLAGS "")
if(CONFIG_X86_MMX)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "mmx")
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "mmxext")
endif()
if(CONFIG_X86_SSE)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "sse")
endif()
if(CONFIG_X86_SSE2)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "sse2")
endif()
if(CONFIG_X86_SSE3)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "pni")
endif()
if(CONFIG_X86_SSSE3)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "ssse3")
endif()
if(CONFIG_X86_SSE41)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "sse4.1")
endif()
if(CONFIG_X86_SSE42)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "sse4.2")
endif()
if(CONFIG_X86_SSE4A)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "sse4a")
endif()
if(NOT CONFIG_X86_64 AND CONFIG_CACHE_MANAGEMENT)
  string(JOIN "," QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS}" "clflush")
endif()

set(QEMU_FLAGS_${ARCH}
  -m ${QEMU_MEMORY_SIZE_MB}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}${QEMU_CPU_FLAGS}
  -machine q35
  -device isa-debug-exit,iobase=0xf4,iosize=0x04
  ${REBOOT_FLAG}
  -nographic
  )

if(NOT CONFIG_ACPI)
  list(APPEND QEMU_FLAGS_${ARCH} -machine acpi=off)
endif()

# TODO: Support debug
# board_set_debugger_ifnset(qemu)
# debugserver: QEMU_EXTRA_FLAGS += -s -S
# debugserver: qemu

if(CONFIG_MMU)
  if(CONFIG_BOARD_QEMU_X86_TINY AND CONFIG_DEMAND_PAGING
      AND NOT CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
    # This is to map the flash so it is accessible.
    math(EXPR QEMU_FLASH_SIZE_KB "${CONFIG_FLASH_SIZE} * 1024")
    set(X86_EXTRA_GEN_MMU_ARGUMENTS
        --map ${CONFIG_FLASH_BASE_ADDRESS},${QEMU_FLASH_SIZE_KB},W)
  endif()

  if(DEFINED CONFIG_FLASH_SIMULATOR_BASE_ADDRESS
      AND DEFINED CONFIG_FLASH_SIMULATOR_SIZE)

    # Map the simulated flash to a memory file on QEMU targets.
    set(X86_EXTRA_GEN_MMU_ARGUMENTS
        --map ${CONFIG_FLASH_SIMULATOR_BASE_ADDRESS},${CONFIG_FLASH_SIMULATOR_SIZE},W)
  endif()
endif()
