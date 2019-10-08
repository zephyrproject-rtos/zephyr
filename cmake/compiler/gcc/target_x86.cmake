# SPDX-License-Identifier: Apache-2.0

if(CONFIG_X86_64)
  string(PREPEND CMAKE_ASM_FLAGS "-m64 ")
  string(PREPEND CMAKE_C_FLAGS   "-m64 ")
  string(PREPEND CMAKE_CXX_FLAGS "-m64 ")
else()
  string(PREPEND CMAKE_ASM_FLAGS "-m32 ")
  string(PREPEND CMAKE_C_FLAGS   "-m32 ")
  string(PREPEND CMAKE_CXX_FLAGS "-m32 ")
endif()
