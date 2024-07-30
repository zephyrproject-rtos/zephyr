# SPDX-License-Identifier: Apache-2.0

# Flags not supported by llext linker
# (regexps are supported and match whole word)
set(LLEXT_REMOVE_FLAGS
  -fno-pic
  -fno-pie
  -ffunction-sections
  -fdata-sections
  -g.*
  -Os
  -mcpu=.*
)

# Flags to be added to llext code compilation
set(LLEXT_APPEND_FLAGS
  -fPIC
  -nostdlib
  -nodefaultlibs
)
