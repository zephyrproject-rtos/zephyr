
set(CLANG_ROOT $ENV{CLANG_ROOT_DIR})

if(CONFIG_ARM)
set(triple arm-none-eabi)
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")
elseif(CONFIG_X86)
set(triple i686-pc-none-elf)
endif()
#set(triple i386-pc-elfiamcu)

set(CMAKE_C_COMPILER          ${CLANG_ROOT}/bin/clang)
set(CMAKE_C_COMPILER_TARGET   ${triple})
#set(CMAKE_ASM_COMPILER       ${CLANG_ROOT}/bin/llvm-as)
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER        ${CLANG_ROOT}/bin/clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_AR                  "${CLANG_ROOT}/bin/llvm-ar"  CACHE INTERNAL " " FORCE)
set(CMAKE_LINKER              "${CLANG_ROOT}/bin/llvm-link" CACHE INTERNAL " " FORCE)
SET(CMAKE_NM                  "${CLANG_ROOT}/bin/llvm-nm"       CACHE INTERNAL " " FORCE)
SET(CMAKE_OBJDUMP             "${CLANG_ROOT}/bin/llvm-objdump" CACHE INTERNAL " " FORCE)
SET(CMAKE_RANLIB              "${CLANG_ROOT}/bin/llvm-ranlib" CACHE INTERNAL " " FORCE)
set(CMAKE_OBJCOPY             objcopy CACHE INTERNAL " " FORCE)
set(CMAKE_READELF             readelf CACHE INTERNAL " " FORCE)




set(NOSTDINC "")

foreach(file_name include include-fixed)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    )
  string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()

foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem ${isystem_include_dir})
endforeach()

set(CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags} -Wl,--unresolved-symbols=ignore-in-object-files)
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
