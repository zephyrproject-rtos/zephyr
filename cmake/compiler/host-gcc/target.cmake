# Configures CMake for using GCC

find_program(CMAKE_C_COMPILER   gcc    )
find_program(CMAKE_OBJCOPY      objcopy)
find_program(CMAKE_OBJDUMP      objdump)
#find_program(CMAKE_LINKER      ld     ) # Not in use yet
find_program(CMAKE_AR           ar     )
find_program(CMAKE_RANLILB      ranlib )
find_program(CMAKE_READELF      readelf)
find_program(CMAKE_GDB          gdb    )

set(CMAKE_C_FLAGS 	          -m32 )
set(CMAKE_CXX_FLAGS 	      -m32 )
set(CMAKE_SHARED_LINKER_FLAGS -m32 )

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
