# Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

# Adds ext-boot metadata header to a PSE84 CM33 secure hex image using imgtool.
# Derives partition addresses and sizes from the devicetree.
#
# When CONFIG_MCUBOOT is set (building MCUBoot itself), uses boot_partition
# for address/size and ROM_START_OFFSET for header size. The header space is
# pre-reserved in the ELF so --pad-header is not used.
#
# Apps chain-loaded by MCUBoot (slot0/slot2) do NOT use this function —
# their signing is handled by cmake/mcuboot.cmake.
#
# Otherwise (standalone secure image), uses m33s_header/m33s_xip partitions.
# The header is separate from the code partition so --pad-header is used to
# prepend it.
#
# Usage: pse84_add_extboot_metadata(<input_hex> <output_hex>)
function(pse84_add_extboot_metadata INPUT_FILE OUTPUT_FILE)
  if(CONFIG_MCUBOOT)
    # MCUBoot bootloader image: use boot_partition, header from ROM_START_OFFSET.
    dt_nodelabel(part_node NODELABEL "boot_partition")
    dt_reg_addr(part_addr PATH ${part_node})
    dt_reg_size(slot_size PATH ${part_node})
    set(header_size ${CONFIG_ROM_START_OFFSET})
    set(pad_header_arg "")
  else()
    # Standalone secure image: use m33s_header + m33s_xip partitions.
    dt_nodelabel(header_node NODELABEL "m33s_header")
    dt_nodelabel(xip_node NODELABEL "m33s_xip")
    dt_reg_addr(part_addr PATH ${header_node})
    dt_reg_size(header_size PATH ${header_node})
    dt_reg_size(xip_size PATH ${xip_node})
    math(EXPR slot_size "${xip_size} + ${header_size}" OUTPUT_FORMAT HEXADECIMAL)
    set(pad_header_arg --pad-header)
  endif()

  # All partitions (boot_partition, m33s_header, m33s_xip) are in the CBUS
  # address space. Add BUILD_OUTPUT_ADJUST_LMA to convert to SAHB space for
  # flashing.
  math(EXPR hex_addr "${part_addr} + ${CONFIG_BUILD_OUTPUT_ADJUST_LMA}" OUTPUT_FORMAT HEXADECIMAL)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version "0.0.0+0"
      --header-size ${header_size} --erased-val 0xff ${pad_header_arg}
      --slot-size ${slot_size} --hex-addr ${hex_addr}
      ${INPUT_FILE} ${OUTPUT_FILE}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${OUTPUT_FILE})
endfunction()
