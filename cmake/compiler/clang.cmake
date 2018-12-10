# Configuration for host installed llvm
#

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

set(CMAKE_C_COMPILER   ${CLANG_ROOT}/bin/clang)
set(CMAKE_CXX_COMPILER ${CLANG_ROOT}/bin/clang++)
set(CMAKE_AR           ${CLANG_ROOT}/bin/llvm-ar		CACHE INTERNAL " " FORCE)
set(CMAKE_LINKER       ${CLANG_ROOT}/bin/llvm-link		CACHE INTERNAL " " FORCE)
set(CMAKE_NM           ${CLANG_ROOT}/bin/llvm-nm		CACHE INTERNAL " " FORCE)
set(CMAKE_OBJDUMP      ${CLANG_ROOT}/bin/llvm-objdump	CACHE INTERNAL " " FORCE)
set(CMAKE_RANLIB       ${CLANG_ROOT}/bin/llvm-ranlib	CACHE INTERNAL " " FORCE)
set(CMAKE_OBJCOPY                    objcopy			CACHE INTERNAL " " FORCE)
set(CMAKE_READELF                    readelf			CACHE INTERNAL " " FORCE)

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
