#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# check_kconfig_ranges_not_overlap(<prefix> <config_vars>)
#
# Ensures there is no overlap between any ranges defined by a pair of Kconfig
# options using the following naming pattern.
# - ${prefix}_<name>_RANGE_START and
# - ${prefix}_<name>_RANGE_SIZE.
# The list of all option names to be considered is passed as "${config_vars}".
function(check_kconfig_ranges_not_overlap prefix config_vars)
  # Construct the list of ranges: "${start_addr}.${end_addr}.${range_name}".
  foreach(start_opt ${config_vars})
    if(NOT start_opt MATCHES "^${prefix}_(.+)_RANGE_START$")
      continue()
    endif()
    set(range_name "${CMAKE_MATCH_1}")
    set(size_opt "${prefix}_${range_name}_RANGE_SIZE")
    if(NOT DEFINED ${size_opt})
      message(FATAL_ERROR "Range option ${start_opt} has no matching ${size_opt}")
    endif()
    math(EXPR start_addr "${${start_opt}}")
    math(EXPR end_addr "${${start_opt}} + ${${size_opt}}")
    list(APPEND ranges "${start_addr}.${end_addr}.${range_name}")
  endforeach()

  # Sort the ranges by the start address.
  list(SORT ranges COMPARE NATURAL)

  # Ensure that no range overlaps with the previous one.
  set(prev_end_addr -1)
  set(prev_range_name "")
  foreach(range ${ranges})
    string(REPLACE "." ";" range_list "${range}")
    list(GET range_list 0 start_addr)
    list(GET range_list 1 end_addr)
    list(GET range_list 2 range_name)
    if(start_addr LESS prev_end_addr)
      message(FATAL_ERROR "Overlapping ranges found: ${prefix}_${prev_range_name}_RANGE_(START|SIZE) and ${prefix}_${range_name}_RANGE_(START|SIZE)")
    endif()
    set(prev_end_addr ${end_addr})
    set(prev_range_name ${range_name})
  endforeach()
endfunction()

# Import CONFIG_PSA_* variables once and reuse for all range checks.
set(psa_config_vars "")
if(CONFIG_PSA_PS OR CONFIG_PSA_ITS OR CONFIG_PSA_CRYPTO_PERSISTENT_KEYS)
  import_kconfig(CONFIG_PSA_ ${DOTCONFIG} psa_config_vars)
endif()

if(CONFIG_PSA_PS)
  check_kconfig_ranges_not_overlap(CONFIG_PSA_PS_UID "${psa_config_vars}")
endif()

if(CONFIG_PSA_ITS)
  check_kconfig_ranges_not_overlap(CONFIG_PSA_ITS_UID "${psa_config_vars}")
endif()

if(CONFIG_PSA_CRYPTO_PERSISTENT_KEYS)
  check_kconfig_ranges_not_overlap(CONFIG_PSA_KEY_ID "${psa_config_vars}")
endif()