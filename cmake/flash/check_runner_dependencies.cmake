# SPDX-License-Identifier: Apache-2.0

# The purpose of this script is to ensure that no runners targets contains
# additional dependencies.
#
# If the target contains dependencies, an error will be printed.
#
# Arguments to the script
#
# TARGET:       The target being checked.
# DEPENDENCIES: List containing dependencies on target.

if(DEPENDENCIES)
  string(REPLACE ";" " " DEPENDENCIES "${DEPENDENCIES}")
  message(FATAL_ERROR
          "`${TARGET}` cannot have dependencies, please remove "
          "`add_dependencies(${TARGET} ${DEPENDENCIES})` in build system."
  )
endif()
