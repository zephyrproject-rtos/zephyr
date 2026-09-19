# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED TARGET_HOST)
  if(NATIVE_TARGET_HOST) # Allow users to manually select the target for cross-compiling use cases
    set(TARGET_HOST ${NATIVE_TARGET_HOST})
  else()
    # NOTE: As this is included before project(), CMAKE_HOST_SYSTEM_PROCESSOR is not yet set
    # but this will produce the same result for Linux
    cmake_host_system_information(RESULT host_processor QUERY OS_PLATFORM)
    if(host_processor MATCHES "arm.*")
      # All 32bit arm variants
      set(TARGET_HOST "arm")
    elseif(host_processor MATCHES ".*86.*")
      # x86_64/i*86
      set(TARGET_HOST "x86_64")
    else()
      set(TARGET_HOST ${host_processor})
    endif()
  endif()

  if((TARGET_HOST STREQUAL "x86_64") AND (NOT CONFIG_64BIT))
    set(NATIVE_TARGET_ARCH "i686")
  else()
    set(NATIVE_TARGET_ARCH ${TARGET_HOST})
  endif()
endif()
