# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Verify that SEARCH_STRING is present or absent (per EXPECT) in BIN_FILE. Used
# to confirm whether the embedded swapped application is stored encrypted.

if(NOT DEFINED BIN_FILE OR NOT DEFINED SEARCH_STRING OR NOT DEFINED EXPECT)
  message(FATAL_ERROR "BIN_FILE, SEARCH_STRING and EXPECT must be defined")
endif()

file(STRINGS "${BIN_FILE}" matches REGEX "${SEARCH_STRING}")

if(EXPECT STREQUAL "absent" AND matches)
  message(FATAL_ERROR
    "Found plaintext string '${SEARCH_STRING}' in ${BIN_FILE}; the embedded "
    "swapped application does not appear to be encrypted")
elseif(EXPECT STREQUAL "present" AND NOT matches)
  message(FATAL_ERROR
    "Did not find string '${SEARCH_STRING}' in ${BIN_FILE}; the embedded "
    "unencrypted swapped application is missing")
endif()
