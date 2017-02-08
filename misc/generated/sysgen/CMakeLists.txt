if(MDEF_FILE)
  file(READ ${APPLICATION_SOURCE_DIR}/${MDEF_FILE} MDEF_FILE_CONTENT)
endif()

if(NOT CONFIG_NUM_COMMAND_PACKETS)
  set(CONFIG_NUM_COMMAND_PACKETS 0)
endif()
if(NOT CONFIG_NUM_TIMER_PACKETS)
  set(CONFIG_NUM_TIMER_PACKETS 0)
endif()
if(NOT CONFIG_NUM_TASK_PRIORITIES)
  set(CONFIG_NUM_TASK_PRIORITIES ${CONFIG_NUM_PREEMPT_PRIORITIES})
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/prj.mdef.in prj.mdef)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sysgen.h
         ${CMAKE_CURRENT_BINARY_DIR}/kernel_main.c
  COMMAND python ${PROJECT_SOURCE_DIR}/scripts/sysgen
    -i ${CMAKE_CURRENT_BINARY_DIR}/prj.mdef
    -o ${CMAKE_CURRENT_BINARY_DIR}/
)

add_custom_target(sysgen_h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sysgen.h)
add_dependencies(zephyr sysgen_h)
add_dependencies(main sysgen_h)
target_sources(zephyr PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/kernel_main.c)
