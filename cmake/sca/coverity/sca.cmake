
find_program(COVERITY_BUILD NAMES cov-build REQUIRED)
find_program(COVERITY_CONFIGURE NAMES cov-configure REQUIRED)
message(STATUS "Found SCA: Coverity (${COVERITY_BUILD})")


zephyr_get(COVERITY_OUTPUT_DIR)

if(NOT COVERITY_OUTPUT_DIR)
  set(output_dir ${CMAKE_BINARY_DIR}/sca/coverity)
else()
  set(output_dir ${COVERITY_OUTPUT_DIR})
endif()

file(MAKE_DIRECTORY ${output_dir})

set(output_arg --dir=${output_dir})


set(CMAKE_C_COMPILER_LAUNCHER ${COVERITY_BUILD} ${output_arg} CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_LAUNCHER ${COVERITY_BUILD} ${output_arg} CACHE INTERNAL "")
