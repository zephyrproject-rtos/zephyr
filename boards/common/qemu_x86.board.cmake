# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Shared by the QEMU x86 boards, which all emulate the same q35 PC machine
# and differ only in the CPU model handed to -cpu. A board sets
# QEMU_CPU_TYPE, plus any QEMU_EXTRA_FLAGS it needs, before including this
# file; everything derived from Kconfig and devicetree is handled here.

set(SUPPORTED_EMU_PLATFORMS qemu)

if(CONFIG_X86_64)
  set(QEMU_BINARY_SUFFIX x86_64)
endif()

if(CONFIG_SRAM_DEPRECATED_KCONFIG_SET)
  math(EXPR RAM_SIZE "${CONFIG_SRAM_SIZE} / 1024" OUTPUT_FORMAT HEXADECIMAL)
else()
  dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
  dt_reg_size(RAM_SIZE PATH "${chosen_sram_path}")
  math(EXPR RAM_SIZE "${RAM_SIZE} / 1024 / 1024" OUTPUT_FORMAT HEXADECIMAL)
endif()

if(CONFIG_XIP)
  # Extra 4MB to emulate flash area
  math(EXPR QEMU_MEMORY_SIZE_MB "${RAM_SIZE} + 4")
else()
  math(EXPR QEMU_MEMORY_SIZE_MB "${RAM_SIZE}")
endif()

# Advertise <feature> to the guest when the build enabled CONFIG_<kconfig>,
# so the emulated CPU matches the instruction set the code was compiled for.
function(qemu_x86_cpu_feature kconfig feature)
  if(CONFIG_${kconfig})
    set(QEMU_CPU_FLAGS "${QEMU_CPU_FLAGS},${feature}" PARENT_SCOPE)
  endif()
endfunction()

set(QEMU_CPU_FLAGS "")
qemu_x86_cpu_feature(X86_MMX mmx)
qemu_x86_cpu_feature(X86_MMX mmxext)
qemu_x86_cpu_feature(X86_SSE sse)
qemu_x86_cpu_feature(X86_SSE2 sse2)
qemu_x86_cpu_feature(X86_SSE3 pni)
qemu_x86_cpu_feature(X86_SSSE3 ssse3)
qemu_x86_cpu_feature(X86_SSE41 sse4.1)
qemu_x86_cpu_feature(X86_SSE42 sse4.2)
qemu_x86_cpu_feature(X86_SSE4A sse4a)
if(NOT CONFIG_X86_64)
  qemu_x86_cpu_feature(CACHE_MANAGEMENT clflush)
endif()

if(CONFIG_ENTROPY_VIRTIO)
  set(QEMU_VIRTIO_ENTROPY_FLAGS -device virtio-rng-pci)
endif()

if(CONFIG_INPUT_VIRTIO)
  if(CONFIG_INPUT_VIRTIO_DEVICE_TYPE_KEYBOARD)
    set(QEMU_VIRTIO_INPUT_FLAGS -device virtio-keyboard-pci,addr=05.0,id=input0)
  elseif(CONFIG_INPUT_VIRTIO_DEVICE_TYPE_TABLET)
    set(QEMU_VIRTIO_INPUT_FLAGS -device virtio-tablet-pci,addr=05.0,id=input0)
  else()
    message(WARNING "No virtio input device type selected; QEMU_VIRTIO_INPUT_FLAGS will be empty")
  endif()
endif()

set(QEMU_BOARD_FLAGS
  -m ${QEMU_MEMORY_SIZE_MB}
  -cpu ${QEMU_CPU_TYPE}${QEMU_CPU_FLAGS}
  -machine q35
  -device isa-debug-exit,iobase=0xf4,iosize=0x04
  ${QEMU_VIRTIO_ENTROPY_FLAGS}
  ${QEMU_VIRTIO_INPUT_FLAGS}
  )

if(NOT CONFIG_ACPI)
  list(APPEND QEMU_BOARD_FLAGS -machine acpi=off)
endif()

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
