# Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
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
  dt_nodelabel(m33s_header_node NODELABEL "m33s_header")
  dt_nodelabel(m33s_xip_node NODELABEL "m33s_xip")
  dt_reg_addr(header_addr PATH ${m33s_header_node})
  dt_reg_size(header_size PATH ${m33s_header_node})
  dt_reg_size(m33s_xip_size PATH ${m33s_xip_node})
  math(EXPR slot_size "${m33s_xip_size} + ${header_size}" OUTPUT_FORMAT HEXADECIMAL)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version "0.0.0+0"
      --header-size ${header_size} --erased-val 0xff --pad-header
      --slot-size ${slot_size} --hex-addr ${header_addr}
      ${INPUT_FILE} ${OUTPUT_FILE}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${OUTPUT_FILE})
endfunction()
