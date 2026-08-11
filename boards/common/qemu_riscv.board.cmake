# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Helpers shared by the QEMU RISC-V boards, which all derive the -cpu argument
# and the qemu-system-riscv* binary from devicetree and Kconfig the same way.

# Build the QEMU -cpu argument for /cpus/cpu@0 out of its riscv,isa-base and
# riscv,isa-extensions devicetree properties, plus CONFIG_RISCV_PMP.
#
# Anything that is not derivable from those, such as the S-mode options which
# depend on the privilege modes advertised in devicetree, stays in the board
# that needs it.
function(qemu_riscv_cpu_from_dt result)
  set(riscv_isa_base)
  set(riscv_isa_extensions)
  dt_prop(riscv_isa_base PATH "/cpus/cpu@0" PROPERTY "riscv,isa-base" REQUIRED)
  dt_prop(riscv_isa_extensions PATH "/cpus/cpu@0" PROPERTY "riscv,isa-extensions" REQUIRED)

  set(cpu "${riscv_isa_base}")
  foreach(ext IN LISTS riscv_isa_extensions)
    if(ext)
      string(APPEND cpu ",${ext}=on")
    endif()
  endforeach()

  if(CONFIG_RISCV_PMP)
    string(APPEND cpu ",pmp=on,u=on")
  endif()

  set(${result} "${cpu}" PARENT_SCOPE)
endfunction()

# Select the qemu-system-riscv* binary matching the target word size.
function(qemu_riscv_binary_suffix result)
  if(CONFIG_64BIT)
    set(${result} riscv64 PARENT_SCOPE)
  else()
    set(${result} riscv32 PARENT_SCOPE)
  endif()
endfunction()
