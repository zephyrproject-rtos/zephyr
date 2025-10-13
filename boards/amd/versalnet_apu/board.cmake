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

# Add SMP support if configured maxcpus parameter value is aligned with QEMU DT
if(CONFIG_SMP AND CONFIG_MP_MAX_NUM_CPUS GREATER 1)
  list(APPEND QEMU_SMP_FLAGS -smp maxcpus=20)
endif()

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
endif()

set(QEMU_KERNEL_OPTION
  -device loader,cpu-num=0,file=${PROJECT_BINARY_DIR}/../tfa/versal_net/${BUILD_FOLDER}/bl31/bl31.elf
  -device loader,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>
)
