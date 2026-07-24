# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Slice the DSP remote ELF into the raw reset/text/data binaries consumed
# by the primary image (see nxp_rtxxx_dsp_load()).
#
# The section lists mirror the HiFi4 SoC linker script. This must be invoked
# from the remote (DSP) domain's CMake scope, where CONFIG_KERNEL_BIN_NAME and
# the zephyr.elf output are available.
function(nxp_imxrt_amp_dsp_remote_imgs)
  set(bin_base "${APPLICATION_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}")

  add_custom_command(
    OUTPUT ${bin_base}.reset.bin
    DEPENDS ${bin_base}.elf
    COMMAND ${CMAKE_OBJCOPY}
    -Obinary ${bin_base}.elf ${bin_base}.reset.bin
    --only-section=.ResetVector.text
  )

  add_custom_command(
    OUTPUT ${bin_base}.text.bin
    DEPENDS ${bin_base}.elf
    COMMAND ${CMAKE_OBJCOPY}
    -Obinary ${bin_base}.elf ${bin_base}.text.bin
    --only-section=.WindowVectors.text
    --only-section=.*Vector.text
    --only-section=!.ResetVector.text
    --only-section=.iram.text
    --only-section=.text
  )

  add_custom_command(
    OUTPUT ${bin_base}.data.bin
    DEPENDS ${bin_base}.elf
    COMMAND ${CMAKE_OBJCOPY}
    -Obinary ${bin_base}.elf ${bin_base}.data.bin
    --only-section=.rodata
    --only-section=initlevel
    --only-section=sw_isr_table
    --only-section=device_area
    --only-section=device_states
    --only-section=service_area
    --only-section=.noinit
    --only-section=.data
    --only-section=.bss
    --only-section=log_*_area
    --only-section=k_*_area
    --only-section=*_api_area
    --only-section=nocache_load
  )

  add_custom_target(
    dsp_bin ALL
    DEPENDS
    ${bin_base}.reset.bin
    ${bin_base}.text.bin
    ${bin_base}.data.bin
  )
endfunction()
