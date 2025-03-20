# SPDX-License-Identifier: Apache-2.0
# Copyright 2024-2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

if(CONFIG_BOARD_MPS2_AN385)
  set(SUPPORTED_EMU_PLATFORMS qemu armfvp)
  set(QEMU_CPU_TYPE_${ARCH} cortex-m3)
  set(QEMU_FLAGS_${ARCH}
    -cpu ${QEMU_CPU_TYPE_${ARCH}}
    -machine mps2-an385
    -nographic
    -vga none
    )
  set(ARMFVP_BIN_NAME FVP_MPS2_Cortex-M3)
elseif(CONFIG_BOARD_MPS2_AN383)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_MPS2_Cortex-M0plus)
  set(ARMFVP_FLAGS
  -C armcortexm0plusct.NUM_MPU_REGION=8
  -C armcortexm0plusct.USER=1
  -C armcortexm0plusct.VTOR=1
  )
elseif(CONFIG_BOARD_MPS2_AN386)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_MPS2_Cortex-M4)
elseif(CONFIG_BOARD_MPS2_AN500)
  set(SUPPORTED_EMU_PLATFORMS armfvp)
  set(ARMFVP_BIN_NAME FVP_MPS2_Cortex-M7)
elseif(CONFIG_BOARD_MPS2_AN521_CPU0 OR CONFIG_BOARD_MPS2_AN521_CPU0_NS OR CONFIG_BOARD_MPS2_AN521_CPU1)
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_CPU_TYPE_${ARCH} cortex-m33)
  set(QEMU_FLAGS_${ARCH}
    -cpu ${QEMU_CPU_TYPE_${ARCH}}
    -machine mps2-an521
    -nographic
    -m 16
    -vga none
    )

  # To enable a host tty switch between serial and pty
  #  -chardev serial,path=/dev/ttyS0,id=hostS0
  # pty is not available on Windows.
  if(CMAKE_HOST_UNIX)
    list(APPEND QEMU_EXTRA_FLAGS -chardev pty,id=hostS0 -serial chardev:hostS0)
  endif()

  if(CONFIG_BUILD_WITH_TFM)
    # Override the binary used by qemu, to use the combined
    # TF-M (Secure) & Zephyr (Non Secure) image (when running
    # in-tree tests).
    set(QEMU_KERNEL_OPTION "-device;loader,file=${CMAKE_BINARY_DIR}/zephyr/tfm_merged.hex")

    set(ARMFVP_FLAGS ${ARMFVP_FLAGS} -a ${APPLICATION_BINARY_DIR}/zephyr/tfm_merged.hex)
  elseif(CONFIG_OPENAMP)
    set(QEMU_EXTRA_FLAGS "-device;loader,file=${REMOTE_ZEPHYR_DIR}/zephyr.elf")
  elseif(CONFIG_BOARD_MPS2_AN521_CPU1)
    set(CPU0_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/zephyr/boards/arm/mps2/empty_cpu0-prefix/src/empty_cpu0-build/zephyr)
    set(QEMU_KERNEL_OPTION "-device;loader,file=${CPU0_BINARY_DIR}/zephyr.elf")
    list(APPEND QEMU_EXTRA_FLAGS "-device;loader,file=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}")
  endif()
endif()

board_set_debugger_ifnset(qemu)

set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
-C fvp_mps2.telnetterminal0.start_telnet=0
-C fvp_mps2.telnetterminal1.start_telnet=0
-C fvp_mps2.telnetterminal2.start_telnet=0
-C fvp_mps2.UART0.out_file=-
-C fvp_mps2.UART0.unbuffered_output=1
-C fvp_mps2.UART1.out_file=-
-C fvp_mps2.UART1.unbuffered_output=1
-C fvp_mps2.UART2.out_file=-
-C fvp_mps2.UART2.unbuffered_output=1
-C fvp_mps2.mps2_visualisation.disable-visualisation=1
)
