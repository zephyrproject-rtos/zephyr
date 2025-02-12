# SPDX-License-Identifier: Apache-2.0

set(GEN_KOBJ_LIST_SCRIPT ${ZEPHYR_BASE}/scripts/build/gen_kobject_list.py)
set(GEN_KOBJ_LIST ${PYTHON_EXECUTABLE} ${GEN_KOBJ_LIST_SCRIPT})

# Invokes gen_kobject_list.py with the given SCRIPT_ARGS, creating a TARGET that depends on the
# script's OUTPUTS
function(gen_kobject_list)
  cmake_parse_arguments(PARSE_ARGV 0 arg
    ""
    "TARGET"
    "OUTPUTS;SCRIPT_ARGS;INCLUDES;DEPENDS"
  )
  foreach(include ${arg_INCLUDES})
    list(APPEND arg_SCRIPT_ARGS --include-subsystem-list ${include})
  endforeach()
  add_custom_command(
    OUTPUT ${arg_OUTPUTS}
    COMMAND
      ${GEN_KOBJ_LIST}
      ${arg_SCRIPT_ARGS}
      $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
      ${arg_DEPENDS}
      ${GEN_KOBJ_LIST_SCRIPT}
      ${PARSE_SYSCALLS_TARGET}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(${arg_TARGET} DEPENDS ${arg_OUTPUTS})
endfunction()

function(gen_kobject_list_gperf)
  cmake_parse_arguments(PARSE_ARGV 0 arg
    ""
    "TARGET;OUTPUT;KERNEL_TARGET"
    "INCLUDES"
  )
  gen_kobject_list(
    TARGET ${arg_TARGET}
    OUTPUTS ${arg_OUTPUT}
    SCRIPT_ARGS
      --kernel $<TARGET_FILE:${arg_KERNEL_TARGET}}>
      --gperf-output ${arg_OUTPUT}
    INCLUDES ${arg_INCLUDES}
    DEPENDS ${arg_KERNEL_TARGET}
    )
endfunction()

function(gen_kobject_list_headers)
  cmake_parse_arguments(PARSE_ARGV 0 arg
    ""
    "GEN_DIR_UPVAR"
    "INCLUDES"
  )
  if (PROJECT_BINARY_DIR)
    set(gen_dir ${PROJECT_BINARY_DIR}/include/generated/zephyr)
  else ()
    set(gen_dir ${CMAKE_BINARY_DIR}/include/generated/zephyr)
  endif ()

  set(KOBJ_TYPES ${gen_dir}/kobj-types-enum.h)
  set(KOBJ_OTYPE ${gen_dir}/otype-to-str.h)
  set(KOBJ_SIZE ${gen_dir}/otype-to-size.h)

  file(MAKE_DIRECTORY ${gen_dir})

  gen_kobject_list(
    TARGET ${KOBJ_TYPES_H_TARGET}
    OUTPUTS ${KOBJ_TYPES} ${KOBJ_OTYPE} ${KOBJ_SIZE}
    SCRIPT_ARGS
      --kobj-types-output ${KOBJ_TYPES}
      --kobj-otype-output ${KOBJ_OTYPE}
      --kobj-size-output ${KOBJ_SIZE}
    INCLUDES ${arg_INCLUDES}
  )

  if(GEN_DIR_UPVAR)
    cmake_path(GET gen_dir PARENT_PATH gen_dir)
    set(${GEN_DIR_UPVAR} ${gen_dir} PARENT_SCOPE)
  endif()
endfunction ()
