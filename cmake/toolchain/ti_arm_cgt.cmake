# Derive TOOLCHAIN_HOME from environment var setting:
set_ifndef(TIARMCGT_INSTALLATION_PATH "$ENV{TIARMCGT_INSTALLATION_PATH}")
set(       TIARMCGT_INSTALLATION_PATH  ${TIARMCGT_INSTALLATION_PATH} CACHE PATH "")
assert(    TIARMCGT_INSTALLATION_PATH  "TIARMCGT_INSTALLATION_PATH is not set")

set(TOOLCHAIN_HOME ${TIARMCGT_INSTALLATION_PATH})

# Invokes compiler/ti_arm.cmake, which calls find-program() to set the
#  CMAKE_ vars like CMAKE_C_COMPILER, etc.
set(COMPILER ti_arm)

# Now, set cross compile (prefix):
set(CROSS_COMPILE  ${TOOLCHAIN_HOME}/bin/arm)

# and the real compiler name (suffix):
set(CC cl)
set(C++ cl)

# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extentions.cmake)
foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()
