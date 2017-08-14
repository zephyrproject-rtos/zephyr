# Generate offsets.h from offsets.o

add_library(offsets OBJECT offsets.c)
target_object_link_libraries(offsets zephyr)

set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/offsets.h)
add_custom_command(
  OUTPUT ${OFFSETS_H_PATH}
  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/gen_offset_header.py
  -i ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/offsets.dir/offsets.c.obj
  -o ${OFFSETS_H_PATH}
  DEPENDS offsets
)

add_custom_target(offsets_h DEPENDS ${OFFSETS_H_PATH})
add_dependencies(zephyr offsets_h)
