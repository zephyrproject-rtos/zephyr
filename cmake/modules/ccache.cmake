# SPDX-License-Identifier: Apache-2.0

# Use ccache if it is installed, unless the user explicitly disables
# it by setting USE_CCACHE=0.

include_guard(GLOBAL)

if(USE_CCACHE STREQUAL "0")
else()
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    ccache)
  endif()
endif()
