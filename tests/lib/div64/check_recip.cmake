# SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

# cmake -DNM=<nm> -DOBJECTS=<objects> -DEXPECT=recip|plain -P check_recip.cmake
#
# Fails if the objects call a 64-bit division routine and EXPECT is recip,
# or call none and EXPECT is plain.

execute_process(
  COMMAND ${NM} -u ${OBJECTS}
  OUTPUT_VARIABLE symbols
  RESULT_VARIABLE result
)
if(result)
  message(FATAL_ERROR "div64: ${NM} failed on ${OBJECTS}")
endif()

string(REGEX MATCHALL "__aeabi_u?ldivmod|__u?divdi3|__u?moddi3|__u?divmoddi4" calls "${symbols}")
list(REMOVE_DUPLICATES calls)

if(EXPECT STREQUAL "recip" AND calls)
  message(FATAL_ERROR "div64: ${calls} called where a reciprocal was expected")
elseif(EXPECT STREQUAL "plain" AND NOT calls)
  message(FATAL_ERROR "div64: no 64-bit division call without the optimizer, "
                      "so this check would not see one")
endif()
