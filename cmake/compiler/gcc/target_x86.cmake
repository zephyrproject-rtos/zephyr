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

# GNU Assembler, by default on non-Linux targets, treats slashes as
# start of comments on i386.
# (https://sourceware.org/binutils/docs-2.33.1/as/i386_002dChars.html#i386_002dChars)
# In order to use division, `--divide` needs to be passed to
# the assembler.
list(APPEND TOOLCHAIN_C_FLAGS -Wa,--divide)
