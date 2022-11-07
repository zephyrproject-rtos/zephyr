# SPDX-License-Identifier: Apache-2.0

if(NOT SYSROOT_DIR)
  # When using cross-compile, the generic toolchain will be identical to the
  # target toolchain, hence let's ask the compiler if it knows its own sysroot.
  # Some gcc based compilers do have this information built-in.
  execute_process(COMMAND ${CMAKE_C_COMPILER} --print-sysroot
                  RESULT_VARIABLE return_val
                  OUTPUT_VARIABLE reported_sysroot
                  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT return_val AND NOT "${reported_sysroot}" STREQUAL "")
    set(SYSROOT_DIR "${reported_sysroot}" CACHE INTERNAL "Sysroot reported by ${CMAKE_C_COMPILER}")
  else()
    # Let's try to lookup a sysroot.
    # Compiler is expected to be located in `bin/` so we go one folder up for search.
    cmake_path(GET CMAKE_C_COMPILER PARENT_PATH search_path)
    cmake_path(SET search_path NORMALIZE "${search_path}/..")
    file(GLOB_RECURSE libc_dirs RELATIVE ${search_path} ${search_path}/**/libc.a )

    # Did we find any candidates ?
    list(LENGTH libc_dirs libc_dirs_length)
    if(${libc_dirs_length} GREATER 0)
      list(TRANSFORM libc_dirs REPLACE "libc.a$" "")
      list(SORT libc_dirs COMPARE NATURAL)
      list(GET libc_dirs 0 sysroot)
      set(sysroot_candidates ${sysroot})

      foreach(dir ${libc_dirs})
        string(FIND "${dir}" "${sysroot}" index)
        if(${index} EQUAL -1)
          set(sysroot ${dir})
          list(APPEND sysroot_candidates ${sysroot})
        endif()
      endforeach()

      # We detect the lib folders, the actual sysroot is up one level.
      list(TRANSFORM sysroot_candidates REPLACE "/lib[/]?$" "")

      list(LENGTH sysroot_candidates sysroot_candidates_length)
      if(sysroot_candidates_length GREATER 1)
        list(TRANSFORM sysroot_candidates PREPEND "${search_path}")
        string(REPLACE ";" "\n" sysroot_candidates_str "${sysroot_candidates}")

        message(WARNING "Multiple sysroot candidates found: ${sysroot_candidates_str}\n"
                        "Use `-DSYSROOT_DIR=<sysroot-dir>` to select sysroot.\n"
        )
      endif()

      # Use first candidate as sysroot.
      list(GET sysroot_candidates 0 chosen_sysroot)
      cmake_path(SET SYSROOT_DIR NORMALIZE "${search_path}/${chosen_sysroot}")
    endif()

    if(SYSROOT_DIR)
      set(SYSROOT_DIR "${SYSROOT_DIR}" CACHE INTERNAL "Sysroot discovered")
      message(STATUS "Found sysroot: ${SYSROOT_DIR}")
    else()
      message(WARNING "No sysroot found, build may not work.\n"
                      "Use `-DSYSROOT_DIR=<sysroot-dir>` to specify sysroot to use.\n"
      )
    endif()
  endif()
endif()
