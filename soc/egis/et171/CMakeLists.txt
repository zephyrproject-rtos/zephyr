# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc.c
  start.S
  soc_irq.S
)

zephyr_linker_sources(ROM_START SORT_KEY 0x0 common_linker/init.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_ANDES_V5_EXECIT RODATA SORT_KEY 0x0 common_linker/execit.ld)
zephyr_linker_sources_ifdef(CONFIG_XIP RAM_SECTIONS SORT_KEY 0x0 common_linker/ram_start_nonzero.ld)

# Note: AndeStar V5 DSP needs custom Andes V5 toolchain
if(CONFIG_SOC_ANDES_V5_HWDSP)
  zephyr_cc_option(-mext-dsp)
endif()

# Note: AndeStar V5 EXEC.IT needs custom Andes V5 toolchain
if(CONFIG_SOC_ANDES_V5_EXECIT)
  zephyr_cc_option(-mexecit)
  zephyr_ld_options(-Wl,--mexecit)
endif()

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

# Pad the file size to the nearest multiple of 32 and write it to offset 0x10 in the header
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/update_bin_size.py ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
  )
