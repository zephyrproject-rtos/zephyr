cmake_minimum_required (VERSION 2.6)
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

list (APPEND CMAKE_MODULE_PATH
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms")

include (syscheck)
project (metal C)

include (CheckIncludeFiles)
include (CheckCSourceCompiles)
include (collect)
include (options)
include (depends)

foreach(_inc_path ${CMAKE_INCLUDE_PATH})
  collect (PROJECT_INC_DIRS "${_inc_path}")
endforeach()

enable_testing ()

add_subdirectory (lib)

if (WITH_TESTS)
  add_subdirectory (test)
endif (WITH_TESTS)

if (WITH_DOC)
  add_subdirectory (doc)
endif (WITH_DOC)

if (WITH_EXAMPLES)
  add_subdirectory (examples)
endif (WITH_EXAMPLES)

# vim: expandtab:ts=2:sw=2:smartindent
