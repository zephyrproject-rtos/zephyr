# Merges a list of files into a destination file.
# Usage: list of files as arguments, first argument is the destination file

MATH(EXPR ARGC "${CMAKE_ARGC}-1")
# First 3 arguments are "cmake", "-P", and "process.cmake"
if( ${CMAKE_ARGC} LESS 5)
  message(FATAL_ERROR "Not enough arguments")
endif()

set(DEST_FILE ${CMAKE_ARGV3})
# Empty the file
file(REMOVE ${DEST_FILE})

foreach(i RANGE 4 ${ARGC})
	file(READ ${CMAKE_ARGV${i}} BUF)
	file(APPEND ${DEST_FILE} ${BUF})
endforeach()
