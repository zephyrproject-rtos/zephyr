# Copyright (c) 2026 Renesas Electronics Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Determines what argument to give to --core= based on the
# KConfig'uration and sets this to ICCRH850_CPU

if("${ARCH}" STREQUAL "rh850")
  if(CONFIG_CPU_G4MH)
    set(ICCRH850_CPU G4MH)
  elseif(CONFIG_CPU_G4KH)
    set(ICCRH850_CPU G4KH)
  endif()
endif()
