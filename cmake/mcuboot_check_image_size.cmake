# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Post-build script to verify a signed MCUboot image fits within the available
# slot space after accounting for trailer / swap-workspace overhead.
#
# Invoked via:
#   cmake -DSIGNED_BIN=<path> -DSLOT_SIZE=<bytes> -DFOOTER_SIZE=<bytes> -P mcuboot_check_image_size.cmake
#
# Input variables (passed via -D):
#   SIGNED_BIN  - Path to the signed binary file (.signed.bin)
#   SLOT_SIZE   - Slot size in bytes (from devicetree)
#   FOOTER_SIZE - MCUboot trailer + swap-workspace overhead in bytes

if(NOT EXISTS "${SIGNED_BIN}")
  return()
endif()

if(NOT DEFINED SLOT_SIZE OR NOT DEFINED FOOTER_SIZE)
  message(FATAL_ERROR "mcuboot_check_image_size.cmake: SLOT_SIZE and FOOTER_SIZE must be defined")
endif()

file(SIZE "${SIGNED_BIN}" signed_size)
math(EXPR max_image_size "${SLOT_SIZE} - ${FOOTER_SIZE}")

if(signed_size GREATER max_image_size)
  math(EXPR overflow "${signed_size} - ${max_image_size}")
  message(FATAL_ERROR
    "MCUboot signed image exceeds maximum size for swap upgrade.\n"
    "  Signed image: ${SIGNED_BIN}\n"
    "  Image size:   ${signed_size} bytes\n"
    "  Max allowed:  ${max_image_size} bytes (slot ${SLOT_SIZE} - footer ${FOOTER_SIZE})\n"
    "  Overflow:     ${overflow} bytes\n"
    "Reduce the application size or increase the slot partition.")
endif()

math(EXPR margin "${max_image_size} - ${signed_size}")
message(STATUS "MCUboot image size check passed: ${signed_size} / ${max_image_size} bytes"
        " (${margin} bytes free)")
