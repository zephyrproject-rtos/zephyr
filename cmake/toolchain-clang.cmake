
#set(triple arm-none-eabi)
set(triple i686-pc-none-elf)
#set(triple i386-pc-elfiamcu)

set(CMAKE_C_COMPILER /opt/llvm4.0.1/bin/clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
#set(CMAKE_ASM_COMPILER llvm-as)
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_AR           "/opt/llvm4.0.1/bin/llvm-ar"  CACHE INTERNAL " " FORCE)
set(CMAKE_LINKER       "/opt/llvm4.0.1/bin/llvm-link" CACHE INTERNAL " " FORCE)
SET(CMAKE_NM      "/opt/llvm4.0.1/bin/llvm-nm"       CACHE INTERNAL " " FORCE)
SET(CMAKE_OBJDUMP "/opt/llvm4.0.1/bin/llvm-objdump" CACHE INTERNAL " " FORCE)
SET(CMAKE_RANLIB  "/opt/llvm4.0.1/bin/llvm-ranlib" CACHE INTERNAL " " FORCE)
set(CMAKE_OBJCOPY      objcopy CACHE INTERNAL " " FORCE)
set(CMAKE_READELF      readelf CACHE INTERNAL " " FORCE)


#set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")


set(NOSTDINC "")

foreach(file_name include include-fixed)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    )
  string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})

  if(MSYS)
    # TODO: Remove this when
    # https://github.com/zephyrproject-rtos/zephyr/issues/4687 is
    # resolved
    execute_process(
      COMMAND cygpath -u ${_OUTPUT}
      OUTPUT_VARIABLE _OUTPUT
      )
    string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})
  endif()

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()

foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem ${isystem_include_dir})
endforeach()
set(CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags} -Wl,--unresolved-symbols=ignore-in-object-files)
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
