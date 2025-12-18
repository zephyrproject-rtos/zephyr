# Copyright (c) 2025, Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH xilinx-aarch64)
set(QEMU_CPU_TYPE_${ARCH} cortexa72)

set(QEMU_FLAGS_${ARCH}
    -machine arm-generic-fdt
    -hw-dtb ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
    -device loader,addr=0xFD1A0300,data=0x8000000e,data-len=4
    -nographic
    -m 2g
)

# Set TF-A platform for ARM Trusted Firmware builds
if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "versal")
  # Add Versal specific TF-A build parameters
  set(TFA_EXTRA_ARGS "TFA_NO_PM=1;PRELOADED_BL33_BASE=${CONFIG_SRAM_BASE_ADDRESS}")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
  set(XSDB_BL31_PATH ${PROJECT_BINARY_DIR}/../tfa/versal/${BUILD_FOLDER}/bl31/bl31.elf)
  board_runner_args(xsdb "--bl31=${XSDB_BL31_PATH}")
endif()

set(QEMU_KERNEL_OPTION
    "-device;loader,cpu-num=0,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>"
)

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)
