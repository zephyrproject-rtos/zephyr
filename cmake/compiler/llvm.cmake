# Configuration for host installed llvm
#

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

