# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Post-build script to verify a signed MCUboot image fits within the available
# slot space after accounting for trailer / swap-workspace overhead.
#
# Invoked via:
#   cmake -DSIGNED_BIN=<path> -DSLOT_SIZE=<bytes> -DFOOTER_SIZE=<bytes>
#         -P mcuboot_check_image_size.cmake
#
# Input variables (passed via -D):
#   SIGNED_BIN  - Path to the signed binary file (.signed.bin)
#   SLOT_SIZE   - Slot size in bytes (from devicetree)
#   FOOTER_SIZE - MCUboot trailer + swap-workspace overhead in bytes
#                 (from mcuboot_image_footer_size)

# Right-align a string within a field of the given width.
function(ralign out_var value width)
  string(LENGTH "${value}" len)
  math(EXPR pad "${width} - ${len}")
  if(pad GREATER 0)
    string(REPEAT " " ${pad} spaces)
    set(${out_var} "${spaces}${value}" PARENT_SCOPE)
  else()
    set(${out_var} "${value}" PARENT_SCOPE)
  endif()
endfunction()

if(NOT DEFINED SIGNED_BIN OR NOT DEFINED SLOT_SIZE OR NOT DEFINED FOOTER_SIZE)
  message(FATAL_ERROR
    "mcuboot_check_image_size.cmake: SIGNED_BIN, SLOT_SIZE and FOOTER_SIZE must be defined")
endif()

if(NOT EXISTS "${SIGNED_BIN}")
  message(FATAL_ERROR
    "mcuboot_check_image_size.cmake: signed binary not found: ${SIGNED_BIN}")
endif()

file(SIZE "${SIGNED_BIN}" signed_size)
math(EXPR max_image_size "${SLOT_SIZE} - ${FOOTER_SIZE}")

math(EXPR pct_x100 "(${signed_size} * 10000) / ${SLOT_SIZE}")
math(EXPR pct_whole "${pct_x100} / 100")
math(EXPR pct_frac  "${pct_x100} - ${pct_whole} * 100")
if(pct_frac LESS 10)
  set(pct_frac "0${pct_frac}")
endif()

ralign(col_signed "${signed_size} B" 11)
ralign(col_footer "${FOOTER_SIZE} B" 11)
ralign(col_slot   "${SLOT_SIZE} B"   11)
ralign(col_pct    "${pct_whole}.${pct_frac}%" 10)

message(STATUS
  "MCUboot slot      Signed size  Footer size   Slot size  %age Used\n"
  "           FLASH:${col_signed}  ${col_footer}  ${col_slot}  ${col_pct}")

if(signed_size GREATER max_image_size)
  math(EXPR overflow "${signed_size} - ${max_image_size}")
  message(FATAL_ERROR
    "MCUboot signed image overflow: ${overflow} bytes.\n"
    "Reduce the application size or increase the slot partition.")
endif()
