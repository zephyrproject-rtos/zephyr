# Configures CMake for using GCC

find_program(CMAKE_C_COMPILER   gcc    )
find_program(CMAKE_OBJCOPY      objcopy)
find_program(CMAKE_OBJDUMP      objdump)
#find_program(CMAKE_LINKER      ld     ) # Not in use yet
find_program(CMAKE_AR           ar     )
find_program(CMAKE_RANLILB      ranlib )
find_program(CMAKE_READELF      readelf)
find_program(CMAKE_GDB          gdb    )

# -march={pentium,lakemont,...} do not automagically imply -m32, so
# adding it here.

# There's only one 64bits ARCH (actually: -mx32). Let's exclude it to
# avoid a confusing game of "who's last on the command line wins".
# Maybe the -m32/-miamcu FLAGS should all be next to -march= in the
# longer term?
if(NOT CONFIG_X86_64)
  set(CMAKE_ASM_FLAGS           -m32 )
  set(CMAKE_C_FLAGS             -m32 )
  set(CMAKE_CXX_FLAGS           -m32 )
  set(CMAKE_SHARED_LINKER_FLAGS -m32 ) # unused?
endif()

if(CONFIG_CPLUSPLUS)
  set(cplusplus_compiler g++)
else()
  if(EXISTS g++)
    set(cplusplus_compiler g++)
  else()
    # When the toolchain doesn't support C++, and we aren't building
    # with C++ support just set it to something so CMake doesn't
    # crash, it won't actually be called
    set(cplusplus_compiler ${CMAKE_C_COMPILER})
  endif()
endif()
find_program(CMAKE_CXX_COMPILER ${cplusplus_compiler}     CACHE INTERNAL " " FORCE)

# The x32 version of libgcc is usually not available (can't trust gcc
# -mx32 --print-libgcc-file-name) so don't fail to build for something
# that is currently not needed. See comments in compiler/gcc/target.cmake
if (NOT CONFIG_X86_64)
  # This libgcc code is partially duplicated in compiler/*/target.cmake
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} --print-libgcc-file-name
    OUTPUT_VARIABLE LIBGCC_FILE_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  assert_exists(LIBGCC_FILE_NAME)

# While most x86_64 Linux distributions implement "multilib" and have
# 32 bits libraries off the shelf, things like
# "/usr/lib/gcc/x86_64-linux-gnu/7/IAMCU/libgcc.a" are unheard of.
# So this fails CONFIG_X86_IAMCU=y with a "cannot find -lgcc" error which
# is clearer than "undefined reference to __udivdi3, etc."
  LIST(APPEND TOOLCHAIN_LIBS gcc)
endif()

# Load toolchain_cc-family macros
# Significant overlap with freestanding gcc compiler so reuse it
include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_security_fortify.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_security_canaries.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_optimizations.cmake)
