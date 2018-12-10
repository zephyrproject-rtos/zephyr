set(CLANG_ROOT $ENV{CLANG_ROOT_DIR})
set_ifndef(CLANG_ROOT /usr)

set(TOOLCHAIN_HOME ${CLANG_ROOT}/bin/)

set(COMPILER clang)

if("${ARCH}" STREQUAL "arm")
  set(triple arm-none-eabi)
  set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")
elseif("${ARCH}" STREQUAL "x86")
  set(triple i686-pc-none-elf)
endif()

set(CMAKE_C_COMPILER_TARGET   ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})
