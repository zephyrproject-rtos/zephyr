# Copyright 2025 - 2026 NXP
# SPDX-License-Identifier: Apache-2.0

# Resolve the HiFi4 DSP remote board for the currently selected primary board.
#
# Sets the variable named by <out_var> in the caller's scope to the matching
# DSP (hifi4) board target. Aborts configuration with a fatal error when the
# selected board has no known DSP counterpart.
function(nxp_rtxxx_amp_dsp_remote_board out_var)
  set(target "${BOARD}/${BOARD_QUALIFIERS}")

  if(target STREQUAL "mimxrt685_evk/mimxrt685s/cm33")
    set(${out_var} "mimxrt685_evk/mimxrt685s/hifi4" PARENT_SCOPE)
  elseif(target STREQUAL "mimxrt700_evk/mimxrt798s/cm33_cpu0")
    set(${out_var} "mimxrt700_evk/mimxrt798s/hifi4" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "NXP i.MX RT DSP support is not available on board '${target}'.")
  endif()
endfunction()
