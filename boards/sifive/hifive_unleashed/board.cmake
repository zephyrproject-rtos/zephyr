# SPDX-License-Identifier: Apache-2.0

if(CONFIG_QEMU_TARGET)
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_BINARY_SUFFIX riscv64)

  # Hart 0 is the E51, the U54 cores follow it. The machine has two harts at least.
  if(CONFIG_SOC_FU540_U54)
    set(QEMU_CPU_TYPE sifive-u54)
    math(EXPR qemu_harts "${CONFIG_MP_MAX_NUM_CPUS} + 1")
  else()
    set(QEMU_CPU_TYPE sifive-e51)
    set(qemu_harts 2)
  endif()
  set(QEMU_SMP_FLAGS -smp ${qemu_harts})

  # The machine selects the cores itself. All harts start in the L2 LIM (msel=6),
  # where the image is loaded.
  set(QEMU_BOARD_FLAGS
    -machine sifive_u,msel=6
    -bios none
    -m 256
    )

  include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
else()
  set(SUPPORTED_EMU_PLATFORMS renode)
  set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/hifive_unleashed.resc)
  set(RENODE_UART sysbus.uart0)
  set(OPENOCD_USE_LOAD_IMAGE NO)

  board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_hifive_unleashed.cfg")

  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()
