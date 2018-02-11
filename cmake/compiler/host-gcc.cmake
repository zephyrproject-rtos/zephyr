# Configures CMake for using GCC

set(CMAKE_C_COMPILER   gcc     CACHE INTERNAL " " FORCE)
set(CMAKE_OBJCOPY      objcopy CACHE INTERNAL " " FORCE)
set(CMAKE_OBJDUMP      objdump CACHE INTERNAL " " FORCE)
#set(CMAKE_LINKER      ld      CACHE INTERNAL " " FORCE) # Not in use yet
set(CMAKE_AR           ar      CACHE INTERNAL " " FORCE)
set(CMAKE_RANLILB      ranlib  CACHE INTERNAL " " FORCE)
set(CMAKE_READELF      readelf CACHE INTERNAL " " FORCE)
set(CMAKE_GDB          gdb     CACHE INTERNAL " " FORCE)
set(CMAKE_C_FLAGS 	-m32   CACHE INTERNAL " " FORCE)
set(CMAKE_CXX_FLAGS 	-m32   CACHE INTERNAL " " FORCE)
set(CMAKE_SHARED_LINKER_FLAGS -m32 CACHE INTERNAL " " FORCE)

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
set(CMAKE_CXX_COMPILER ${cplusplus_compiler}     CACHE INTERNAL " " FORCE)

