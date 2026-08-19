# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

# Adds mcuboot metadata to the PSE84 CM33 secure image using imgtool
# with parameters derived from the DT memory map (header address,
# header size, and slot size). Uses default version 0.0.0+0 as imgtool
# requires a version parameter.
# Registers the add metadata command and output file via
# extra_post_build properties.
#
# Usage: pse84_add_metadata_secure_hex(<input_hex> <output_hex>)
function(pse84_add_metadata_secure_hex INPUT_FILE OUTPUT_FILE)
  # flash0_s and flash0_sahb are two DT views (CBUS / SAHB) of the same
  # physical external flash. Partitions are declared under flash0_s, so
  # dt_reg_addr returns the CBUS-translated partition address. Translate
  # from CBUS view to SAHB view by rebasing against the two flash node
  # base addresses.
  dt_nodelabel(flash_sahb NODELABEL "flash0_sahb")
  dt_nodelabel(flash_s    NODELABEL "flash0_s")
  dt_reg_addr(flash_sahb_addr PATH ${flash_sahb})
  dt_reg_addr(flash_s_addr    PATH ${flash_s})

  dt_nodelabel(m33s_header_node NODELABEL "m33s_header")
  dt_nodelabel(m33s_xip_node    NODELABEL "m33s_xip")
  dt_reg_addr(part_addr   PATH ${m33s_header_node})
  dt_reg_size(header_size PATH ${m33s_header_node})
  dt_reg_size(m33s_xip_size PATH ${m33s_xip_node})

  math(EXPR slot_size "${m33s_xip_size} + ${header_size}" OUTPUT_FORMAT HEXADECIMAL)

  # Rebase partition address from flash0_s space to flash0_sahb space.
  math(EXPR header_sahb_addr
       "${part_addr} - ${flash_s_addr} + ${flash_sahb_addr}"
       OUTPUT_FORMAT HEXADECIMAL)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version "0.0.0+0"
      --header-size ${header_size} --erased-val 0xff --pad-header
      --slot-size ${slot_size} --hex-addr ${header_sahb_addr}
      ${INPUT_FILE} ${OUTPUT_FILE}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${OUTPUT_FILE})
endfunction()
