#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)
set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH xilinx-aarch64)
set(QEMU_CPU_TYPE_${ARCH} cortexa78)

set(QEMU_FLAGS_${ARCH}
  -machine arm-generic-fdt
  -hw-dtb ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
  -device loader,addr=0xEC200300,data=0x3EE,data-len=4
  -device loader,addr=0xEC200300,data=0x3EE,data-len=4
  -nographic
  -m 2g
)

# Set TF-A platform for ARM Trusted Firmware builds
if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "versal_net")
  # Add Versal NET specific TF-A build parameters
  set(TFA_EXTRA_ARGS "TFA_NO_PM=1;PRELOADED_BL33_BASE=0x0")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
endif()

set(QEMU_KERNEL_OPTION
  -device loader,cpu-num=0,file=${PROJECT_BINARY_DIR}/../tfa/versal_net/${BUILD_FOLDER}/bl31/bl31.elf
  -device loader,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
)
