# Copyright 2025 - 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Resolve the DSP remote board for the currently selected primary board.
#
# Sets the variable named by <out_var> in the caller's scope to the matching
# DSP board target. The remote board keeps the board name and SoC qualifier of
# the primary and swaps the CPU cluster qualifier for the DSP core paired with
# that specific primary core, mirroring the SECOND_CORE_MCUX launcher.
#
# The primary-to-DSP core pairing is SoC-specific and cannot be derived by
# blindly rewriting the trailing qualifier: a single SoC can expose several
# primary cores, each paired with a different DSP core (e.g. on the RT700 the
# mimxrt798s pairs cm33_cpu0 with hifi4 and cm33_cpu1 with hifi1). The mapping
# is therefore keyed on "<soc>/<primary_core>" below; add a row here to support
# a new SoC or primary core.
function(nxp_rtxxx_amp_dsp_remote_board out_var)
  # BOARD_QUALIFIERS may or may not carry a leading separator depending on the
  # CMake scope; strip a leading one, then split into the SoC path (everything
  # but the last component) and the primary CPU cluster (the last component).
  string(REGEX REPLACE "^/" "" quals "${BOARD_QUALIFIERS}")
  string(REGEX REPLACE "/[^/]+$" "" soc "${quals}")
  string(REGEX REPLACE ".*/" "" primary_core "${quals}")

  # Map each supported "<soc>/<primary_core>" to its paired DSP core.
  set(dsp_core_mimxrt685s/cm33      "hifi4")
  set(dsp_core_mimxrt798s/cm33_cpu0 "hifi4")
  set(dsp_core_mimxrt798s/cm33_cpu1 "hifi1")

  set(key "${soc}/${primary_core}")
  if(NOT DEFINED dsp_core_${key})
    message(FATAL_ERROR
      "No DSP remote core mapping for primary board '${BOARD}/${quals}'. "
      "Add a 'dsp_core_${key}' entry to dsp_board_select.cmake.")
  endif()

  set(${out_var} "${BOARD}/${soc}/${dsp_core_${key}}" PARENT_SCOPE)
endfunction()
