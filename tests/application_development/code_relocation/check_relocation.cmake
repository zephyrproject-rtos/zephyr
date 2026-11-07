# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

file(READ "${RELOCATION_DICT}" directives)
if(directives MATCHES "\\$<")
  message(FATAL_ERROR "Relocation directives contain unevaluated generator expressions")
endif()

set(expected
  "\nSRAM2:COPY:${TEST_SOURCE_DIR}/src/test_file4.c;${TEST_SOURCE_DIR}/src/test_file5.c,"
)
string(FIND "${directives}" "${expected}" position)
if(position EQUAL -1)
  message(FATAL_ERROR "Missing expected generator-expression relocation directive: ${expected}")
endif()

file(GLOB replies
  "${BUILD_DIR}/.cmake/api/v1/reply/target-code_data_relocation_target-*.json"
)
list(LENGTH replies reply_count)
if(NOT reply_count EQUAL 1)
  message(FATAL_ERROR
    "Expected one File API reply for code_data_relocation_target, found ${reply_count}"
  )
endif()

file(READ "${replies}" reply)
string(JSON target_name GET "${reply}" name)
if(NOT target_name STREQUAL "code_data_relocation_target")
  message(FATAL_ERROR "Unexpected File API target: ${target_name}")
endif()

string(JSON member_count LENGTH "${reply}")
math(EXPR last_member "${member_count} - 1")
foreach(index RANGE ${last_member})
  string(JSON member MEMBER "${reply}" ${index})
  if(member STREQUAL "sources" OR member STREQUAL "interfaceSources")
    string(JSON source_count LENGTH "${reply}" "${member}")
    if(source_count GREATER 0)
      message(FATAL_ERROR "Relocation metadata must not appear in File API ${member}")
    endif()
  endif()
endforeach()
