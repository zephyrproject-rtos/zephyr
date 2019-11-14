# Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)
set(QEMU_ARCH xilinx-aarch64)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/fdt-single_arch-zcu102-arm.dtb)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${CMAKE_C_COMPILER}
    -E -nostdinc
    -I${ZEPHYR_BASE}/boards/${ARCH}/${BOARD}/qemu-devicetrees/include
    -x assembler-with-cpp
    -o - ${ZEPHYR_BASE}/boards/${ARCH}/${BOARD}/qemu-devicetrees/zcu102-arm.dts |
    ${DTC}
    -q -O dtb -I dts
    -o ${CMAKE_CURRENT_BINARY_DIR}/fdt-single_arch-zcu102-arm.dtb
  )
endif()

set(QEMU_CPU_TYPE_${ARCH} cortex-a53)
set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine arm-generic-fdt
  -dtb ${CMAKE_CURRENT_BINARY_DIR}/fdt-single_arch-zcu102-arm.dtb
  )

set(QEMU_KERNEL_OPTION
  "-device;loader,file=$<TARGET_FILE:zephyr_final>,cpu-num=0"
  "-device;loader,addr=0xfd1a0104,data=0x8000000e,data-len=4"
  )

board_set_debugger_ifnset(qemu)
