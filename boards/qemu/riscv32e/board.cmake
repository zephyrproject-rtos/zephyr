# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

set(riscv_isa_extensions)
set(riscv_isa_base)
dt_prop(riscv_isa_extensions PATH "/cpus/cpu@0" PROPERTY "riscv,isa-extensions" REQUIRED)
dt_prop(riscv_isa_base PATH "/cpus/cpu@0" PROPERTY "riscv,isa-base" REQUIRED)

set(qemu_riscv_cpu "${riscv_isa_base}")
foreach(ext IN LISTS riscv_isa_extensions)
  if(ext)
    string(APPEND qemu_riscv_cpu ",${ext}=on")
  endif()
endforeach()

if(CONFIG_RISCV_PMP)
  string(APPEND qemu_riscv_cpu ",pmp=on,u=on")
endif()

if(CONFIG_64BIT)
  set(QEMU_binary_suffix riscv64)
else()
  set(QEMU_binary_suffix riscv32)
endif()

set(QEMU_CPU_TYPE_${ARCH} "${qemu_riscv_cpu}")

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine virt
  -bios none
  -m 256
  -cpu ${qemu_riscv_cpu}
  )

board_set_debugger_ifnset(qemu)
