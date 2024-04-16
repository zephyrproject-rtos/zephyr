# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

# 'SCA_ROOT' is a prioritized list of directories where SCA tools may
# be found. It always includes ${ZEPHYR_BASE} at the lowest priority.
list(APPEND SCA_ROOT ${ZEPHYR_BASE})

zephyr_get(ZEPHYR_SCA_VARIANT)

if(ScaTools_FIND_REQUIRED AND NOT DEFINED ZEPHYR_SCA_VARIANT)
  message(FATAL_ERROR "ScaTools required but 'ZEPHYR_SCA_VARIANT' is not set. "
                      "Please set 'ZEPHYR_SCA_VARIANT' to desired tool."
  )
endif()

if(NOT DEFINED ZEPHYR_SCA_VARIANT)
  return()
endif()

foreach(root ${SCA_ROOT})
  if(EXISTS ${root}/cmake/sca/${ZEPHYR_SCA_VARIANT}/sca.cmake)
    include(${root}/cmake/sca/${ZEPHYR_SCA_VARIANT}/sca.cmake)
    return()
  endif()
endforeach()

message(FATAL_ERROR "ZEPHYR_SCA_VARIANT set to '${ZEPHYR_SCA_VARIANT}' but no "
                    "implementation for '${ZEPHYR_SCA_VARIANT}' found. "
                    "SCA_ROOTs searched: ${SCA_ROOT}"
)
