# SPDX-License-Identifier: Apache-2.0

# Parameter names identical to the execute_process() CMake command, and
# "ARGS" for the process command-line arguments.
# Use set(ARGS ...) to build the ARGS list and then quote the list
# when invoking the COMMAND. Example:
# set(ARGS a b c)
# -DARGS="${ARGS}"

if(NOT DEFINED COMMAND)
  message(FATAL_ERROR "No COMMAND argument supplied")
endif()

if(NOT DEFINED ARGS)
  set(ARGS )
else()
  separate_arguments(ARGS)
endif()

if(DEFINED OUTPUT_FILE)
  set(OF OUTPUT_FILE ${OUTPUT_FILE})
endif()

if(DEFINED ERROR_FILE)
  set(EF ERROR_FILE ${ERROR_FILE})
endif()

if(DEFINED WORKING_DIRECTORY)
  set(WD WORKING_DIRECTORY ${WORKING_DIRECTORY})
endif()

execute_process(
  COMMAND ${COMMAND}
  ${ARGS}
  ${OF}
  ${EF}
  ${WD}
  RESULT_VARIABLE ret
)

if(NOT "${ret}" STREQUAL "0")
  message(FATAL_ERROR "Process failed: '${ret}'")
endif()
