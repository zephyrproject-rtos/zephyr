# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

###########################################################################
# XC32 Symbol Aliases / Compatibility Layer
#
# These definitions bridge the gap between Microchip's XC32 runtime
# expectations and Zephyr's Picolibc/Kernel environment.
#
# 1. _std[in|out|err]: XC32 libraries expect underscores; Zephyr provides
#    standard names. Aliasing ensures printf/scanf route to console.
# 2. __ctype_get_mb_cur_max: Redirects XC32's locale-aware character checks
#    to Zephyr's static C-locale configuration.
# 3. __atomic_load_ungetc: Specifically for Cortex-M0+ (e.g., PL10).
#    Resolves missing libatomic references in XC32's C++ Standard Library
#    by mapping them to Zephyr's internal atomic routines.
###########################################################################
set_linker_property(APPEND PROPERTY base
  ${LINKERFLAGPREFIX},--defsym,_stdin=stdin
  ${LINKERFLAGPREFIX},--defsym,_stdout=stdout
  ${LINKERFLAGPREFIX},--defsym,_stderr=stderr
  ${LINKERFLAGPREFIX},--defsym,__ctype_get_mb_cur_max=__locale_mb_cur_max
)

if(CONFIG_CPP AND CONFIG_CPU_CORTEX_M0PLUS)
  set_linker_property(APPEND PROPERTY base ${LINKERFLAGPREFIX},--defsym,__atomic_load_ungetc=atomic_get)
endif()
