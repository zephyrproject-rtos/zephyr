# Copyright (c) 2019 Lexmark International, Inc.
#
# SPDX-License-Identifier: Apache-2.0

# FIXME++: This forces the CI to use the Xilinx QEMU included in the Zephyr SDK
#          0.11.0-alpha-8 for testing. This should be removed once the Zephyr
#          SDK 0.11.0 release is mainlined.
set(CI_QEMU_PATH
  /opt/sdk/zephyr-sdk-0.11.0-alpha-8/sysroots/x86_64-pokysdk-linux/usr/bin
  )

if(EXISTS ${CI_QEMU_PATH})
  set(ENV{QEMU_BIN_PATH} ${CI_QEMU_PATH})
endif()
# FIXME--

set(EMU_PLATFORM qemu)
set(QEMU_ARCH xilinx-aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-r5)
set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine arm-generic-fdt
  -dtb ${ZEPHYR_BASE}/boards/${ARCH}/${BOARD}/fdt-single_arch-zcu102-arm.dtb
  )

set(QEMU_KERNEL_OPTION
  "-device;loader,file=$<TARGET_FILE:zephyr_final>,cpu-num=4"
  "-device;loader,addr=0xff5e023c,data=0x80008fde,data-len=4"
  "-device;loader,addr=0xff9a0000,data=0x80000218,data-len=4"
  )

board_set_debugger_ifnset(qemu)
