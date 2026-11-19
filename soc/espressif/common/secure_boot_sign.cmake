# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# Post-elf2image Secure Boot v2 checks and signature.
# UNSIGNED_BIN is the unsigned MCUboot image. espsecure pads that body to
# 4 KiB and appends one 4 KiB signature sector (--version 2).

if(NOT EXISTS "${SIGNING_KEY}")
  message(FATAL_ERROR "Secure Boot v2 signing key was not found: ${SIGNING_KEY}")
endif()

if(NOT EXISTS "${UNSIGNED_BIN}")
  message(FATAL_ERROR "Secure Boot v2 input image was not found: ${UNSIGNED_BIN}")
endif()

file(SIZE "${UNSIGNED_BIN}" unsigned_size)
math(EXPR aligned_size "(${unsigned_size} + 4095) & ~4095")
math(EXPR signed_size "${aligned_size} + 4096")

if(aligned_size GREATER ROM_LIMIT)
  message(FATAL_ERROR
    "MCUboot image body does not fit in the ROM Secure Boot buffer.\n"
    "  unsigned image: ${unsigned_size} bytes\n"
    "  4 KiB aligned body: ${aligned_size} bytes\n"
    "  ROM limit: ${ROM_LIMIT} bytes\n"
    "A larger boot_partition does not raise this limit.")
endif()

if(signed_size GREATER BOOT_PARTITION_SIZE)
  message(FATAL_ERROR
    "Signed MCUboot image does not fit in boot_partition.\n"
    "  unsigned image: ${unsigned_size} bytes\n"
    "  4 KiB aligned body: ${aligned_size} bytes\n"
    "  signed image (body + 4 KiB signature): ${signed_size} bytes\n"
    "  ROM body limit: ${ROM_LIMIT} bytes\n"
    "  boot_partition: ${BOOT_PARTITION_SIZE} bytes")
endif()

set(signed_bin "${UNSIGNED_BIN}.signed")
execute_process(
  COMMAND ${ESPSECURE} sign-data --version 2
          --keyfile ${SIGNING_KEY}
          --output ${signed_bin}
          ${UNSIGNED_BIN}
  RESULT_VARIABLE sign_result
  ERROR_VARIABLE sign_error
  OUTPUT_VARIABLE sign_output
)
if(NOT sign_result EQUAL 0)
  message(FATAL_ERROR
    "espsecure sign-data failed (${sign_result}): ${sign_error}${sign_output}")
endif()

file(REMOVE "${UNSIGNED_BIN}")
file(RENAME "${signed_bin}" "${UNSIGNED_BIN}")
