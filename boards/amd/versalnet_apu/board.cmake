#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# Set TF-A platform for ARM Trusted Firmware builds
if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "versal_net")
  # Configure TF-A memory location for Versal NET platform
  # TF-A runs from DDR at address 0xf000000 (changed from default 0xbbf00000).
  # This DDR location (VERSAL_NET_ATF_MEM_BASE=0xf000000) works for all possible designs.
  # Note: If TF-A needs to run from OCM instead of DDR, PDI changes would be required.
  set(TFA_EXTRA_ARGS "RESET_TO_BL31=1;PRELOADED_BL33_BASE=0x0;TFA_NO_PM=1;VERSAL_NET_ATF_MEM_BASE=0xf000000;VERSAL_NET_ATF_MEM_SIZE=0x50000")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
  set(XSDB_BL31_PATH ${PROJECT_BINARY_DIR}/../tfa/versal_net/${BUILD_FOLDER}/bl31/bl31.elf)
  board_runner_args(xsdb "--bl31=${XSDB_BL31_PATH}")
endif()

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)

# QEMU emulation uses the Xilinx arm-generic-fdt machine and a hardware device
# tree ("hw-dtb") built by CMakeLists.txt from the optional Xilinx/qemu-devicetrees
# west module (group "optional"). It is enabled only when that module is present.
# The Xilinx QEMU is shipped by the Zephyr SDK as qemu-system-xilinx-aarch64.
# Zephyr runs as BL33; QEMU loads BL31 (built via TF-A) on the first APU core.
if(EXISTS "${WEST_TOPDIR}/modules/hal/qemu_devicetrees/Makefile")
  set(SUPPORTED_EMU_PLATFORMS qemu)
  set(QEMU_BINARY_SUFFIX xilinx-aarch64)
  set(QEMU_CPU_TYPE cortexa78)

  set(QEMU_BOARD_FLAGS
    -machine arm-generic-fdt
    -hw-dtb ${PROJECT_BINARY_DIR}/${BOARD}-qemu.dtb
    -device loader,addr=0xEC200300,data=0x3EE,data-len=4
    -device loader,addr=0xEC200300,data=0x3EE,data-len=4
    -nographic
    -m 2g
  )

  if(CONFIG_SMP AND CONFIG_MP_MAX_NUM_CPUS GREATER 1)
    list(APPEND QEMU_SMP_FLAGS -smp maxcpus=20)
  endif()

  set(QEMU_KERNEL_OPTION
    -device loader,cpu-num=0,file=${PROJECT_BINARY_DIR}/../tfa/versal_net/${BUILD_FOLDER}/bl31/bl31.elf
    -device loader,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
  )

  # Zephyr boots as BL33, so the run targets must build bl31.elf first; a bare
  # "west build -t run" would otherwise skip the TF-A external project.
  if(CONFIG_BUILD_WITH_TFA)
    set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_TARGET_DEPENDS tfa)
  endif()
endif()
