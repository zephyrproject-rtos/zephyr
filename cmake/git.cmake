# Use git if it is installed, to set BUILD_VERSION

# https://cmake.org/cmake/help/latest/module/FindGit.html
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=12
    WORKING_DIRECTORY                ${ZEPHYR_BASE}
    OUTPUT_VARIABLE                  BUILD_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
  )
  if(return_code)
    message(STATUS "git describe failed: ${stderr}; ${KERNEL_VERSION_STRING} will be used instead")
  elseif(CMAKE_VERBOSE_MAKEFILE)
    message(STATUS "git describe stderr: ${stderr}")
  endif()
endif()
