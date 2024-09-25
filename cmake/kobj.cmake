# SPDX-License-Identifier: Apache-2.0

function(gen_kobj gen_dir_out)
  if (PROJECT_BINARY_DIR)
    set(gen_dir ${PROJECT_BINARY_DIR}/include/generated/zephyr)
  else ()
    set(gen_dir ${CMAKE_BINARY_DIR}/include/generated/zephyr)
  endif ()

  set(KOBJ_TYPES ${gen_dir}/kobj-types-enum.h)
  set(KOBJ_OTYPE ${gen_dir}/otype-to-str.h)
  set(KOBJ_SIZE ${gen_dir}/otype-to-size.h)

  file(MAKE_DIRECTORY ${gen_dir})

  add_custom_command(
    OUTPUT ${KOBJ_TYPES} ${KOBJ_OTYPE} ${KOBJ_SIZE}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_kobject_list.py
    --kobj-types-output ${KOBJ_TYPES}
    --kobj-otype-output ${KOBJ_OTYPE}
    --kobj-size-output ${KOBJ_SIZE}
    ${gen_kobject_list_include_args}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    ${ZEPHYR_BASE}/scripts/build/gen_kobject_list.py
    ${PARSE_SYSCALLS_TARGET}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(${KOBJ_TYPES_H_TARGET} DEPENDS ${KOBJ_TYPES} ${KOBJ_OTYPE})

  cmake_path(GET gen_dir PARENT_PATH gen_dir)
  set(${gen_dir_out} ${gen_dir} PARENT_SCOPE)

endfunction ()
