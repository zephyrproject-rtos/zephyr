# Copyright (c) 2025 Antmicro
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_QEMU_TARGET)
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_BINARY_SUFFIX riscv64)

  # Hart 0 is the E51, the U54 cores follow it. The machine has two harts at least.
  if(CONFIG_SOC_POLARFIRE_U54)
    set(QEMU_CPU_TYPE sifive-u54)
    math(EXPR qemu_harts "${CONFIG_MP_MAX_NUM_CPUS} + 1")
  else()
    set(QEMU_CPU_TYPE sifive-e51)
    set(qemu_harts 2)
  endif()
  set(QEMU_SMP_FLAGS -smp ${qemu_harts})

  # The machine selects the cores itself. Without HSS in the eNVM, the harts are
  # started at the entry point of the image by one loader device each.
  set(QEMU_BOARD_FLAGS
    -machine microchip-icicle-kit
    -bios none
  )

  set(QEMU_KERNEL_OPTION "")
  math(EXPR max_hart "${qemu_harts} - 1")
  foreach(hart RANGE 0 ${max_hart})
    list(APPEND QEMU_KERNEL_OPTION
      "-device;loader,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>,cpu-num=${hart}"
    )
  endforeach()

  include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
else()
  set(SUPPORTED_EMU_PLATFORMS renode)
  if(CONFIG_BOARD_MPFS_ICICLE_POLARFIRE_U54)
    set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/mpfs_icicle_polarfire_u54.resc)
    set(RENODE_UART sysbus.mmuart1)
  elseif(CONFIG_BOARD_MPFS_ICICLE_POLARFIRE_U54_SMP)
    set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/mpfs_icicle_polarfire_u54_smp.resc)
    set(RENODE_UART sysbus.mmuart1)
  elseif(CONFIG_BOARD_MPFS_ICICLE_POLARFIRE_E51)
    set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/mpfs_icicle_polarfire_e51.resc)
    set(RENODE_UART sysbus.mmuart0)
  endif()

  set(OPENOCD_USE_LOAD_IMAGE NO)
endif()
