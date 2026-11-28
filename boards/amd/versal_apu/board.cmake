# Copyright (c) 2025, Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFA)
  if(CONFIG_SRAM_DEPRECATED_KCONFIG_SET)
    set(RAM_ADDR ${CONFIG_SRAM_BASE_ADDRESS})
  else()
    dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
    dt_reg_addr(RAM_ADDR PATH "${chosen_sram_path}")
  endif()

  set(TFA_PLAT "versal")
  # Add Versal specific TF-A build parameters
  set(TFA_EXTRA_ARGS "TFA_NO_PM=1;PRELOADED_BL33_BASE=${RAM_ADDR}")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
  set(XSDB_BL31_PATH ${PROJECT_BINARY_DIR}/../tfa/versal/${BUILD_FOLDER}/bl31/bl31.elf)
  board_runner_args(xsdb "--bl31=${XSDB_BL31_PATH}")
endif()

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)

# QEMU emulation uses the Xilinx arm-generic-fdt machine and a hardware device
# tree ("hw-dtb") built by CMakeLists.txt from the optional Xilinx/qemu-devicetrees
# west module (group "optional"). It is enabled only when that module is present.
# The Xilinx QEMU is shipped by the Zephyr SDK as qemu-system-xilinx-aarch64.
if(EXISTS "${WEST_TOPDIR}/modules/hal/qemu_devicetrees/Makefile")
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_BINARY_SUFFIX xilinx-aarch64)
  set(QEMU_CPU_TYPE cortexa72)

  set(QEMU_BOARD_FLAGS
    -machine arm-generic-fdt
    -hw-dtb ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
    -device loader,addr=0xFD1A0300,data=0x8000000e,data-len=4
    -nographic
    -m 2g
  )

  set(QEMU_KERNEL_OPTION
    -device loader,cpu-num=0,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
  )
endif()
