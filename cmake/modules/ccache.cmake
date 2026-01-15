# SPDX-License-Identifier: Apache-2.0

#[=======================================================================[.rst:
ccache
######

Use ccache if it is installed.

Behavior
********

This module will check if ``ccache`` is installed. If installed, it will configure the build system
to use it for compilation and linking.

Variables
*********

* :cmake:variable:`USE_CCACHE`

#]=======================================================================]

include_guard(GLOBAL)

if(USE_CCACHE STREQUAL "0")
else()
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    ccache)
  endif()
endif()
